#include "HeartBeatTask.hpp"
#include "HeartBeatTransformation.hpp"

using namespace llvm;
using namespace llvm::noelle;

HeartBeatTransformation::HeartBeatTransformation (
  Noelle &noelle
  ) : 
    DOALL{noelle}
  , n{noelle} 
{

  /*
   * Create the task signature
   */
  auto tm = noelle.getTypesManager();
  auto funcArgTypes = ArrayRef<Type *>({
    tm->getIntegerType(64),
    tm->getIntegerType(64),
    tm->getVoidPointerType()
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
  auto loopEnvironment = loop->getEnvironment();

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
  this->addJumpToLoop(loop, hbTask);
  
  /*
   * Add the final return to the single task's exit block.
   */
  IRBuilder<> exitB(hbTask->getExit());
  exitB.CreateRetVoid();

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

  /*
   * Fetch the loop handler function
   */
  auto loopHandlerFunction = program->getFunction("loop_handler");
  assert(loopHandlerFunction != nullptr);

  /*
   * Create a new basic block to invoke the loop handler
   *
   * Step 1: fetch the variable that holds the current loop-governing IV value
   */
  auto GIV_attr = loop->getLoopGoverningIVAttribution();
  assert(GIV_attr != nullptr);
  assert(GIV_attr->isSCCContainingIVWellFormed());
  auto currentIVValue = GIV_attr->getValueToCompareAgainstExitConditionValue();
  assert(currentIVValue != nullptr);
  auto cloneCurrentIVValue = this->fetchClone(currentIVValue);
  assert(cloneCurrentIVValue != nullptr);

  /*
   * Step 2: fetch the first instruction of the body of the loop in the task.
   */
  auto bodyBB = ls->getFirstLoopBasicBlockAfterTheHeader();
  assert(bodyBB != nullptr);
  auto entryBodyInst = bodyBB->getFirstNonPHI();
  auto entryBodyInstInTask = hbTask->getCloneOfOriginalInstruction(entryBodyInst);
  assert(entryBodyInstInTask != nullptr);
  auto entryBodyInTask = entryBodyInstInTask->getParent();

  /*
   * Step 3: call the loop handler
   */
  IRBuilder<> bbBuilder(entryBodyInstInTask);
  auto argIter = hbTask->getTaskBody()->arg_begin();
  auto firstIterationValue = &*(argIter++);
  auto lastIterationValue = &*(argIter++);
  auto taskEnvPtr = &*(argIter++);
  auto callToHandler = cast<CallInst>(bbBuilder.CreateCall(loopHandlerFunction, ArrayRef<Value *>({
    cloneCurrentIVValue,
    lastIterationValue,
    taskEnvPtr,
    hbTask->getTaskBody()
        })));

  /*
   * Split the body basic block
   *
   * From 
   * -------
   * | PHI                        |
   * | %Y = call loop_handler()   |
   * | A                          | 
   * | br X                       |
   *
   * to
   * ------------------------------------
   * | PHI                              |
   * | %Y = call loop_handler()         |
   * | %Y2 = icmp %Y, 0                 |
   * | br %Y2 normalBody RET            |
   * ------------------------------------
   *
   * ---normalBody ---
   * | A   |
   * | br X|
   * -------
   */
  auto bottomHalfBB = entryBodyInTask->splitBasicBlock(callToHandler->getNextNode());
  auto addedFakeTerminatorOfEntryBodyInTask = entryBodyInTask->getTerminator();
  IRBuilder<> topHalfBuilder(entryBodyInTask);
  auto typeManager = noelle.getTypesManager();
  auto const0 = ConstantInt::get(typeManager->getIntegerType(32), 0);
  auto cmpInst = cast<Instruction>(topHalfBuilder.CreateICmpEQ(callToHandler, const0));
  auto exitBasicBlockInTask = hbTask->getExit();
  auto condBr = topHalfBuilder.CreateCondBr(cmpInst, bottomHalfBB, exitBasicBlockInTask);
  addedFakeTerminatorOfEntryBodyInTask->eraseFromParent();

  /*
   * Adjust the first starting value of the loop-governing IV to use the first parameter of the task.
   */
  auto& GIV = GIV_attr->getInductionVariable();
  auto originalPHI = GIV.getLoopEntryPHI();
  auto clonePHI = cast<PHINode>(this->fetchClone(originalPHI));
  assert(clonePHI != nullptr);
  clonePHI->setIncomingValueForBlock(hbTask->getEntry(), firstIterationValue);

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
  auto currentIVValue = GIV_attr->getValueToCompareAgainstExitConditionValue();
  
  /*
   * Fetch the pointer to the environment.
   */
  auto envPtr = envBuilder->getEnvArrayInt8Ptr();

  /*
   * Call the function that incudes the parallelized loop.
   */
  IRBuilder<> doallBuilder(this->entryPointOfParallelizedLoop);
  doallBuilder.CreateCall(this->tasks[0]->getTaskBody(), ArrayRef<Value *>({
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
