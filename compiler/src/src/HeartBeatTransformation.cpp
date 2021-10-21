#include "HeartBeatTask.hpp"
#include "HeartBeatTransformation.hpp"

using namespace llvm;
using namespace llvm::noelle;

HeartBeatTransformation::HeartBeatTransformation (
  Noelle &noelle
  ) : 
    DOALL{*noelle.getProgram(), *noelle.getProfiles(), noelle.getVerbosity()}
  , n{noelle} 
{

  /*
   * Fetch the dispatcher to use to jump to a parallelized DOALL loop.
   */
  this->taskDispatcher = this->module.getFunction("handler_for_fork2");
  if (this->taskDispatcher == nullptr){
    errs() << "NOELLE: ERROR = function \"handler_for_fork2\" couldn't be found\n";
    abort();
  }

  /*
   * Create the task signature
   */
  auto tm = noelle.getTypesManager();
  auto int8 = tm->getIntegerType(8);
  auto int64 = tm->getIntegerType(64);
  auto funcArgTypes = ArrayRef<Type*>({
    int64,
    int64,
    PointerType::getUnqual(int8)
  });
  this->taskSignature = FunctionType::get(tm->getVoidType(), funcArgTypes, false);

  return ;
}

bool HeartBeatTransformation::apply (
  LoopDependenceInfo *loop,
  Noelle &noelle,
  Heuristics *h
  ){

  /*
   * Fetch the program
   */
  auto program = noelle.getProgram();

  /*
   * Fetch the function that contains the loop.
   */
  auto ls = loop->getLoopStructure();
  auto loopFunction = ls->getFunction();

  /*
   * For now, let's assume all iterations are independent
   *
   * Fetch the environment of the loop.
   */
  auto loopEnvironment = loop->environment;

  /*
   * Generate an empty task for the heartbeat execution.
   */
  auto hbTask = new HeartBeatTask(this->taskSignature, this->module);
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(loop, { hbTask });

  /*
   * Allocate memory for all environment variables
   */
  auto preEnvRange = loopEnvironment->getEnvIndicesOfLiveInVars();
  auto postEnvRange = loopEnvironment->getEnvIndicesOfLiveOutVars();
  std::set<int> nonReducableVars(preEnvRange.begin(), preEnvRange.end());
  std::set<int> reducableVars(postEnvRange.begin(), postEnvRange.end());
  this->initializeEnvironmentBuilder(loop, nonReducableVars, reducableVars);

  /*
   * Load all loop live-in values at the entry point of the task.
   */
  auto envUser = this->envBuilder->getUser(0);
  for (auto envIndex : loopEnvironment->getEnvIndicesOfLiveInVars()) {
    envUser->addLiveInIndex(envIndex);
  }
  for (auto envIndex : loopEnvironment->getEnvIndicesOfLiveOutVars()) {
    envUser->addLiveOutIndex(envIndex);
  }
  this->generateCodeToLoadLiveInVariables(loop, 0);

  /*
   * Clone loop into the single task used by DOALL
   */
  this->cloneSequentialLoop(loop, 0);
 
  /*
   * Fix the data flow within the parallelized loop by redirecting operands of
   * cloned instructions to refer to the other cloned instructions. Currently,
   * they still refer to the original loop's instructions.
   */
  this->adjustDataFlowToUseClones(loop, 0);

  /*
   * Add the jump from the entry of the function after loading all live-ins to the header of the cloned loop.
   */
  auto loopHeader = ls->getHeader();
  auto headerClone = hbTask->getCloneOfOriginalBasicBlock(loopHeader);
  IRBuilder<> entryBuilder(hbTask->getEntry());
  auto temporaryBrToLoop = entryBuilder.CreateBr(headerClone);
  entryBuilder.SetInsertPoint(temporaryBrToLoop);
  
  /*
   * Add the final return to the single task's exit block.
   */
  IRBuilder<> exitB(hbTask->getExit());
  exitB.CreateRetVoid();
  this->adjustDataFlowToUseClones(loop, 0);

  /*
   * Store final results to loop live-out variables. Note this occurs after
   * all other code is generated. Propagated PHIs through the generated
   * outer loop might affect the values stored
   */
  this->generateCodeToStoreLiveOutVariables(loop, 0);

  /*
   * Add the call to the function that includes the heartbeat loop from the pre-header of the original loop.
   */
  this->invokeHeartBeatFunctionAsideOriginalLoop(loop);

  return true; //FIXME we need the rest as well

  /*
   * Fetch the loop handler function
   */
  auto loopHandlerFunction = program->getFunction("loop_handler");
  assert(loopHandlerFunction != nullptr);

  /*
   * Create a new basic block to invoke the loop handler
   */
  auto loopHandlerBB = BasicBlock::Create(loopFunction->getContext(), "loopHandlerBB", loopFunction);
  IRBuilder<> bbBuilder(loopHandlerBB);
  bbBuilder.CreateCall(loopHandlerFunction);
  bbBuilder.CreateUnreachable();

  /*
   * Fetch the entry of the body of the loop
   */
  auto bodyBB = ls->getFirstLoopBasicBlockAfterTheHeader();
  assert(bodyBB != nullptr);
  auto entryBodyInst = bodyBB->getFirstNonPHI();

  /*
   * Split the basic block
   *
   * From 
   * -------
   * | PHI |
   * | A   |
   * | br X|
   *
   * to
   * ------------------------------------
   * | PHI                              |
   * | %t = load i32*, heartbeatGlobal  |
   * | br %t loopHandlerBB Y            |
   * ------------------------------------
   *
   * ---Y---
   * | A   |
   * | br X|
   * -------
   */
  auto bottomHalfBB = bodyBB->splitBasicBlock(entryBodyInst);
  IRBuilder<> topHalfBuilder(bodyBB);
  auto lastInstInBodyBB = bodyBB->getTerminator();
  auto heartBeatGlobalPtr = program->getGlobalVariable("heartbeat");
  assert(heartBeatGlobalPtr != nullptr);
  auto wasHeartBeatGlobalSet = topHalfBuilder.CreateLoad(heartBeatGlobalPtr);
  wasHeartBeatGlobalSet->moveBefore(lastInstInBodyBB);
  auto typeManager = noelle.getTypesManager();
  auto const0 = ConstantInt::get(typeManager->getIntegerType(32), 0);
  auto cmpInst = cast<Instruction>(topHalfBuilder.CreateICmpEQ(wasHeartBeatGlobalSet, const0));
  cmpInst->moveBefore(lastInstInBodyBB);
  auto condBr = topHalfBuilder.CreateCondBr(cmpInst, bottomHalfBB, loopHandlerBB);
  condBr->moveBefore(lastInstInBodyBB);
  lastInstInBodyBB->eraseFromParent();

  return true;
}

void HeartBeatTransformation::invokeHeartBeatFunctionAsideOriginalLoop (
  LoopDependenceInfo *LDI
) {

  /*
   * Create the environment.
   */
  this->allocateEnvironmentArray(LDI);
  this->populateLiveInEnvironment(LDI);

  /*
   * Fetch the first value of the loop-governing IV
   */
  auto GIV_attr = LDI->getLoopGoverningIVAttribution();
  assert(GIV_attr != nullptr);
  assert(GIV_attr->isSCCContainingIVWellFormed());
  auto& GIV = GIV_attr->getInductionVariable();
  auto firstIterationGoverningIVValue = GIV.getStartValue();

  /*
   * Fetch the last value of the loop-governing IV
   */
  auto lastIterationGoverningIVValue = GIV_attr->getExitConditionValue();
  
  /*
   * Fetch the pointer to the environment.
   */
  auto envPtr = envBuilder->getEnvArrayInt8Ptr();

  /*
   * Call the function that incudes the parallelized loop.
   */
  IRBuilder<> doallBuilder(this->entryPointOfParallelizedLoop);
  auto doallCallInst = doallBuilder.CreateCall(this->tasks[0]->getTaskBody(), ArrayRef<Value *>({
    firstIterationGoverningIVValue,
    lastIterationGoverningIVValue,
    envPtr
  }));

  /*
   * Jump to the unique successor of the loop.
   */
  doallBuilder.CreateBr(this->exitPointOfParallelizedLoop);

  return ;
}

Value * HeartBeatTransformation::fetchClone (Value *original) const {
  auto task = (HeartBeatTask *)this->tasks[0];
  if (isa<ConstantData>(original)) return original;

  if (task->isAnOriginalLiveIn(original)){
    return task->getCloneOfOriginalLiveIn(original);
  }

  assert(isa<Instruction>(original));
  auto iClone = task->getCloneOfOriginalInstruction(cast<Instruction>(original));
  assert(iClone != nullptr);
  return iClone;
}
