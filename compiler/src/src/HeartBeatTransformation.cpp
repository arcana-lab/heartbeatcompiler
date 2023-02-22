#include "HeartBeatTask.hpp"
#include "HeartBeatTransformation.hpp"
#include "noelle/core/BinaryReductionSCC.hpp"
#include "noelle/core/Architecture.hpp"

using namespace llvm;
using namespace llvm::noelle;

HeartBeatTransformation::HeartBeatTransformation (
  Noelle &noelle,
  LoopDependenceInfo *ldi,
  uint64_t numLevels,
  bool containsLiveOut,
  std::unordered_map<LoopDependenceInfo *, uint64_t> &loopToLevel,
  std::unordered_map<LoopDependenceInfo *, std::unordered_set<Value *>> &loopToSkippedLiveIns,
  std::unordered_map<int, int> &constantLiveInsArgIndexToIndex,
  std::unordered_map<LoopDependenceInfo *, std::unordered_map<Value *, int>> &loopToConstantLiveIns,
  std::unordered_map<LoopDependenceInfo *, HeartBeatTransformation *> &loopToHeartBeatTransformation,
  std::unordered_map<LoopDependenceInfo *, LoopDependenceInfo *> &loopToCallerLoop,
  std::unordered_map<LoopDependenceInfo *, uint64_t> &loopToChunksize
) : DOALL{noelle},
    ldi{ldi},
    n{noelle},
    numLevels{numLevels},
    containsLiveOut{containsLiveOut},
    loopToLevel{loopToLevel},
    loopToSkippedLiveIns{loopToSkippedLiveIns},
    constantLiveInsArgIndexToIndex{constantLiveInsArgIndexToIndex},
    loopToConstantLiveIns{loopToConstantLiveIns},
    hbTask{nullptr},
    loopToHeartBeatTransformation{loopToHeartBeatTransformation},
    loopToCallerLoop{loopToCallerLoop},
    loopToChunksize{loopToChunksize} {

  // set cacheline element size
  this->valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);

  /*
   * Create the slice task signature
   */
  auto tm = noelle.getTypesManager();
  std::vector<Type *> sliceTaskSignatureTypes;
  sliceTaskSignatureTypes.push_back(tm->getVoidPointerType());  // pointer to compressed environment
  if (containsLiveOut) {
    sliceTaskSignatureTypes.push_back(tm->getIntegerType(64));  // index variable
  }
  // loop level starts from 0, so +1 here
  for (auto i = 0; i < loopToLevel[ldi] + 1; i++) {
    sliceTaskSignatureTypes.push_back(tm->getIntegerType(64));  // startIter
    sliceTaskSignatureTypes.push_back(tm->getIntegerType(64));  // maxIter
  }

  this->sliceTaskSignature = FunctionType::get(tm->getIntegerType(64), ArrayRef<Type *>(sliceTaskSignatureTypes), false);

  errs() << "create function signature for slice task " << ldi->getLoopStructure()->getFunction()->getName() << ": " << *(this->sliceTaskSignature) << "\n";

  return ;
}

bool HeartBeatTransformation::apply (
  LoopDependenceInfo *loop,
  Heuristics *h
) {

  /*
   * Set the number of cores we target
   * every heartbeat spawns two new children tasks from the parent task
   */
  this->numTaskInstances = 2;

  /*
   * Fetch the program
   */
  auto program = this->n.getProgram();

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
   * Print original function
   */
  errs() << "original function:" << *loopFunction << "\n";
  errs() << "pre-header:" << *(ls->getPreHeader()) << "\n";
  errs() << "header:" << *(ls->getHeader()) << "\n";
  errs() << "first body:" << *(ls->getFirstLoopBasicBlockAfterTheHeader()) << "\n";
  errs() << "latches:";
  for (auto bb : ls->getLatches()) {
    errs() << *bb;
  }
  errs() << "exits:";
  for (auto bb : ls->getLoopExitBasicBlocks()) {
    errs() << *bb;
  }

  /*
   * Generate an empty task for the heartbeat execution.
   */
  this->hbTask = new HeartBeatTask(this->sliceTaskSignature, *program, this->loopToLevel[this->ldi], this->containsLiveOut,
    std::string("HEARTBEAT_loop").append(std::to_string(this->loopToLevel[ldi])).append("_slice")
  );
  // hbTask->getTaskBody()->setName(std::string("HEARTBEAT_loop").append(std::to_string(this->loopToLevel[ldi])).append("_slice"));
  errs() << "initial task body:" << *(hbTask->getTaskBody()) << "\n";
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(
    loop,
    { hbTask }
  );
  errs() << "task after adding predecessor and successors bb:" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Allocate memory for all environment variables
   */
  auto isReducible = [](uint32_t id, bool isLiveOut) -> bool {
    if (!isLiveOut){
      return false;
    }
    return true;
  };
  auto shouldThisVariableBeSkipped = [&](uint32_t variableID, bool isLiveOut) -> bool {
    // if a variable is a skipped live-in, then skip it here
    if (this->loopToSkippedLiveIns[this->ldi].find(loopEnvironment->getProducer(variableID)) != this->loopToSkippedLiveIns[this->ldi].end()) {
      errs() << "skip " << *(loopEnvironment->getProducer(variableID)) << " of id " << variableID << "\n";
      return true;
    }
    return false;
  };
  auto isConstantLiveInVariable = [&](uint32_t variableID, bool isLiveOut) -> bool {
    auto envProducer = loopEnvironment->getProducer(variableID);
    if (this->loopToConstantLiveIns[this->ldi].find(envProducer) != this->loopToConstantLiveIns[this->ldi].end()) {
      errs() << "constant live-in " << *envProducer << " of id " << variableID << "\n";
      return true;
    }
    return false;
  };
  this->initializeEnvironmentBuilder(loop, isReducible, shouldThisVariableBeSkipped, isConstantLiveInVariable);

  /*
   * Clone loop into the single task used by HeartBeat
   */
  this->cloneSequentialLoop(loop, 0);
  errs() << "task after cloning sequential loop: " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Load all loop live-in values at the entry point of the task.
   */
  auto envUser = (HeartBeatLoopEnvironmentUser *)this->envBuilder->getUser(0);
  for (auto envID : loopEnvironment->getEnvIDsOfLiveInVars()) {
    envUser->addLiveIn(envID);
  }
  for (auto envID : loopEnvironment->getEnvIDsOfLiveOutVars()) {
    envUser->addLiveOut(envID);
  }
  this->generateCodeToLoadLiveInVariables(loop, 0);
  errs() << "task after loading live-in variables: " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Clone memory objects that are not blocked by RAW data dependences
   */
  auto ltm = loop->getLoopTransformationsManager();
  if (ltm->isOptimizationEnabled(LoopDependenceInfoOptimization::MEMORY_CLONING_ID)) {
    this->cloneMemoryLocationsLocallyAndRewireLoop(loop, 0);
  }
  errs() << "task after cloning memory objects: " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Fix the data flow within the parallelized loop by redirecting operands of
   * cloned instructions to refer to the other cloned instructions. Currently,
   * they still refer to the original loop's instructions.
   */
  this->adjustDataFlowToUseClones(loop, 0);
  errs() << "task after fixing data flow: " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Handle the reduction variables.
   */
  this->setReducableVariablesToBeginAtIdentityValue(loop, 0);
  errs() << "task after seting reducible variables: " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Add the jump from the entry of the function after loading all live-ins to the header of the cloned loop.
   */
  this->addJumpToLoop(loop, hbTask);
  
  /*
   * Add the final return to the single task's exit block.
   */
  IRBuilder<> exitB(hbTask->getExit());
  exitB.CreateRetVoid();
  errs() << "task after adding jump and return to loop " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Store final results to loop live-out variables. Note this occurs after
   * all other code is generated. Propagated PHIs through the generated
   * outer loop might affect the values stored
   */
  this->generateCodeToStoreLiveOutVariables(loop, 0);
  errs() << "live out id to accumulated private copy\n";
  for (auto pair : this->liveOutVariableToAccumulatedPrivateCopy) {
    errs() << "id: " << pair.first << ": " << *(pair.second) << "\n";
  }
  errs() << "task after store live-out variables " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Allocate the reduction environment array for the next level inside task
   */
  this->allocateNextLevelReducibleEnvironmentInsideTask(loop, 0);
  errs() << "task after allodate reducible environment for kids " << *(hbTask->getTaskBody()) << "\n";

  /*
   * Fetch the loop handler function
   */
  auto loopHandlerFunction = program->getFunction("loop_handler");
  assert(loopHandlerFunction != nullptr);
  errs() << "loop_handler function" << *loopHandlerFunction << "\n";

  /*
   * Create a new bb to invoke the loop handler after the body, and a new basic block to modify the exit condition
   */
  this->loopHandlerBlock = BasicBlock::Create(hbTask->getTaskBody()->getContext(), "loop_handler_block", hbTask->getTaskBody());
  this->modifyExitConditionBlock = BasicBlock::Create(hbTask->getTaskBody()->getContext(), "modify_exit_condition", hbTask->getTaskBody());
  
  auto bbs = ls->getLatches();
  assert(bbs.size() == 1 && "assumption: only has one latch of the loop\n");
  auto latchBB = *(bbs.begin());
  auto latchBBClone = hbTask->getCloneOfOriginalBasicBlock(latchBB);
  auto bodyBB = latchBB->getSinglePredecessor();
  auto bodyBBClone = hbTask->getCloneOfOriginalBasicBlock(bodyBB);
  assert(bodyBB != nullptr && "latch doesn't have a single predecessor\n");

  // modify the bodyBB to jump to the loop_handler block
  IRBuilder<> bodyBuilder{ bodyBBClone };
  auto bodyTerminator = bodyBBClone->getTerminator();
  bodyBuilder.SetInsertPoint(bodyTerminator);
  bodyBuilder.CreateBr(
    loopHandlerBlock
  );
  bodyTerminator->eraseFromParent();

  // modify the exit condition block to jump to the latch block
  IRBuilder<> exitConditionBlockBuilder { modifyExitConditionBlock };
  exitConditionBlockBuilder.CreateBr(
    latchBBClone
  );
  errs() << "task after creating two new basic blocks and fixing control flow" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Modify the exit condition inside the exit condition block
   * also add a phi node in the to use the incoming value from this block
   */
  // step one, add a phi node inside the header for the exitCondition
  auto GIV_attr = loop->getLoopGoverningIVAttribution();
  assert(GIV_attr != nullptr);
  assert(GIV_attr->isSCCContainingIVWellFormed());
  // get the exit condition value
  auto originalExitConditionValue = GIV_attr->getExitConditionValue();
  errs() << "original exit condition value " << *originalExitConditionValue << "\n";
  // add a phi node representing the new exit conditio value
  auto headerBB = ls->getHeader();
  auto headerBBClone = hbTask->getCloneOfOriginalBasicBlock(headerBB);
  auto firstNonPhiInst = headerBBClone->getFirstNonPHI();
  IRBuilder<> headerBuilder{ headerBBClone };
  headerBuilder.SetInsertPoint(firstNonPhiInst);
  auto exitConditionPhiInst = headerBuilder.CreatePHI(
    originalExitConditionValue->getType(),
    1,
    "exitCondition"
  );
  // store this exitConditionPhiInstruction in the corresponding hbTask
  this->hbTask->setMaxIteration(exitConditionPhiInst);
  exitConditionPhiInst->addIncoming(
    originalExitConditionValue,
    hbTask->getEntry()
  );
  // maxIter = startIter + 1
  auto IV = GIV_attr->getInductionVariable().getLoopEntryPHI();
  auto IVClone = cast<PHINode>(hbTask->getCloneOfOriginalInstruction(IV));
  // store the currentIVPhiInstruction in the corresponding hbTask
  this->hbTask->setCurrentIteration(IVClone);
  errs() << "induction variable: " << *IVClone << "\n";
  exitConditionBlockBuilder.SetInsertPoint(modifyExitConditionBlock->getTerminator());
  auto newExitCondtionInst = exitConditionBlockBuilder.CreateAdd(
    IVClone,
    ConstantInt::get(originalExitConditionValue->getType(), 1),
    "newExitCondition"
  );
  // create a phi at the latch for the exit condition
  IRBuilder<> latchBuilder { &*(latchBBClone->begin()) };
  auto exitConditionUpdatedInst = latchBuilder.CreatePHI(
    originalExitConditionValue->getType(),
    2,
    "exitConditionUpdated"
  );
  exitConditionUpdatedInst->addIncoming(
    exitConditionPhiInst,
    loopHandlerBlock
  );
  exitConditionUpdatedInst->addIncoming(
    newExitCondtionInst,
    modifyExitConditionBlock
  );
  exitConditionPhiInst->addIncoming(
    exitConditionUpdatedInst,
    latchBBClone
  );
  // last step is to update the icmp instrution in the loop header to use the exitCondition phi instruction
  auto GIVCmpInst = GIV_attr->getHeaderCompareInstructionToComputeExitCondition();
  auto GIVCmpInstClone = hbTask->getCloneOfOriginalInstruction(GIVCmpInst);
  errs() << "cmp inst to determine whether continue execution of the loop: " << *GIVCmpInstClone << "\n";
  cast<CmpInst>(GIVCmpInstClone)->setOperand(1, exitConditionPhiInst);
  errs() << "task after adjusting the exit condition value" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Call the loop_hander in the loop_handler block and compare the return code
   * to decide whether need to modify the exit condition
   */
  IRBuilder<> loopHandlerBuilder{ loopHandlerBlock };
  // create the vector to represent arguments
  std::vector<Value *> loopHandlerParameters{ hbTask->getContextArg(),
                                              loopHandlerBuilder.getInt64(this->loopToLevel[loop]),
                                              nullptr // pointer to leftover tasks, change this value later
                                            };
  // copy the parent loop's start/max iteration
  for (auto i = 0; i < this->loopToLevel[loop]; i++) {
    loopHandlerParameters.push_back(hbTask->getIterationsVector()[i * 2]);
    loopHandlerParameters.push_back(hbTask->getIterationsVector()[i * 2 + 1]);
  }
  // push the current start/max iteration
  loopHandlerParameters.push_back(hbTask->getCurrentIteration());
  loopHandlerParameters.push_back(hbTask->getMaxIteration());
  // supply 0 as start/max iteration for the rest of levels
  for (auto i = this->loopToLevel[loop] + 1; i <= this->numLevels - 1; i++) {
    loopHandlerParameters.push_back(loopHandlerBuilder.getInt64(0));
    loopHandlerParameters.push_back(loopHandlerBuilder.getInt64(0));
  }

  auto callToHandler = loopHandlerBuilder.CreateCall(
    loopHandlerFunction,
    ArrayRef<Value *>({
      loopHandlerParameters
    }),
    "loop_handler_return_code"
  );
  // cache this call to loop_handler
  this->callToLoopHandler = callToHandler;

  // if (rc > 0) {
  //   modify exit condition
  //   goto latch
  // } else {
  //   goto latch
  // }
  auto cmpInst = loopHandlerBuilder.CreateICmpSGT(
    callToHandler,
    ConstantInt::get(loopHandlerBuilder.getInt64Ty(), 0)
  );
  auto condBr = loopHandlerBuilder.CreateCondBr(
    cmpInst,
    modifyExitConditionBlock,
    latchBBClone
  );
  errs() << "task after invoking loop_handler function and checking return code" << *(hbTask->getTaskBody()) << "\n";

  // /*
  //  * Create a new basic block to invoke the loop handler
  //  *
  //  * Step 1: fetch the variable that holds the current loop-governing IV value
  //  */
  // auto GIV_attr = loop->getLoopGoverningIVAttribution();
  // assert(GIV_attr != nullptr);
  // assert(GIV_attr->isSCCContainingIVWellFormed());
  // auto currentIVValue = GIV_attr->getValueToCompareAgainstExitConditionValue();
  // assert(currentIVValue != nullptr);
  // auto cloneCurrentIVValue = this->fetchClone(currentIVValue);
  // errs() << "cloned current IV value " << *cloneCurrentIVValue << "\n";
  // assert(cloneCurrentIVValue != nullptr);

  // /*
  //  * Step 2: fetch the first instruction of the body of the loop in the task.
  //  */
  // auto bodyBB = ls->getFirstLoopBasicBlockAfterTheHeader();
  // assert(bodyBB != nullptr);
  // auto entryBodyInst = bodyBB->getFirstNonPHI();
  // auto entryBodyInstInTask = hbTask->getCloneOfOriginalInstruction(entryBodyInst);
  // assert(entryBodyInstInTask != nullptr);
  // auto entryBodyInTask = entryBodyInstInTask->getParent();

  // /*
  //  * Step 3: call the loop handler
  //  */
  // IRBuilder<> bbBuilder(entryBodyInstInTask);
  // auto argIter = hbTask->getTaskBody()->arg_begin();
  // auto firstIterationValue = &*(argIter++);
  // auto lastIterationValue = &*(argIter++);
  // auto singleEnvPtr = &*(argIter++);
  // auto reducibleEnvPtr = &*(argIter++);
  // auto taskID = &*(argIter++);
  // auto hbEnvBuilder = (HeartBeatLoopEnvironmentBuilder *)this->envBuilder;
  // auto callToHandler = cast<CallInst>(bbBuilder.CreateCall(loopHandlerFunction, ArrayRef<Value *>({
  //   bbBuilder.CreateZExtOrTrunc(cloneCurrentIVValue, firstIterationValue->getType()),
  //   lastIterationValue,
  //   singleEnvPtr,
  //   // if there's no reducible live-out environment, which means the next level reducible environment won't be allocated
  //   // use the original reducible environment instead
  //   hbEnvBuilder->getReducibleEnvironmentSize() > 0 ? hbEnvBuilder->getNextLevelEnvironmentArrayVoidPtr() : reducibleEnvPtr,
  //   taskID,
  //   hbTask->getTaskBody()
  //       })));

  // /*
  //  * Split the body basic block
  //  *
  //  * From 
  //  * -------
  //  * | PHI                        |
  //  * | %Y = call loop_handler()   |
  //  * | A                          | 
  //  * | br X                       |
  //  *
  //  * to
  //  * ------------------------------------
  //  * | PHI                              |
  //  * | %Y = call loop_handler()         |
  //  * | %Y2 = icmp %Y, 0                 |
  //  * | br %Y2 normalBody RET            |
  //  * ------------------------------------
  //  *
  //  * ---normalBody ---
  //  * | A   |
  //  * | br X|
  //  * -------
  //  */
  // auto bottomHalfBB = entryBodyInTask->splitBasicBlock(callToHandler->getNextNode());
  // auto addedFakeTerminatorOfEntryBodyInTask = entryBodyInTask->getTerminator();
  // IRBuilder<> topHalfBuilder(entryBodyInTask);
  // auto typeManager = noelle.getTypesManager();
  // auto const0 = ConstantInt::get(typeManager->getIntegerType(32), 0);
  // auto cmpInst = cast<Instruction>(topHalfBuilder.CreateICmpEQ(callToHandler, const0));
  // auto exitBasicBlockInTask = hbTask->getExit();
  // auto condBr = topHalfBuilder.CreateCondBr(cmpInst, bottomHalfBB, exitBasicBlockInTask);
  // addedFakeTerminatorOfEntryBodyInTask->eraseFromParent();

  // /*
  //  * Create the bitcast instructions at the entry block of the task to match the type of the GIV
  //  */
  // IRBuilder<> entryTaskBuilder{ hbTask->getEntry() };
  // entryTaskBuilder.SetInsertPoint(&(*hbTask->getEntry()->begin()));
  // auto firstIterationValueCasted = entryTaskBuilder.CreateZExtOrTrunc(firstIterationValue, cloneCurrentIVValue->getType());
  // auto lastIterationValueCasted = entryTaskBuilder.CreateZExtOrTrunc(lastIterationValue, cloneCurrentIVValue->getType());

  // /*
  //  * Adjust the first starting value of the loop-governing IV to use the first parameter of the task.
  //  */
  // auto& GIV = GIV_attr->getInductionVariable();
  // auto originalPHI = GIV.getLoopEntryPHI();
  // auto clonePHI = cast<PHINode>(this->fetchClone(originalPHI));
  // assert(clonePHI != nullptr);
  // clonePHI->setIncomingValueForBlock(hbTask->getEntry(), firstIterationValueCasted);

  // /*
  //  * Adjust the exit condition value of the loop-governing IV to use the second parameter of the task.
  //  *
  //  * Step 1: find the Use of the exit value used in the compare instruction of the loop-governing IV.
  //  */
  // auto LGIV_cmpInst = GIV_attr->getHeaderCompareInstructionToComputeExitCondition();
  // auto LGIV_lastValue = GIV_attr->getExitConditionValue();
  // auto LGIV_currentValue = GIV_attr->getValueToCompareAgainstExitConditionValue();
  // int32_t operandNumber = -1;
  // for (auto &use: LGIV_currentValue->uses()){
  //   auto user = use.getUser();
  //   auto userInst = dyn_cast<Instruction>(user);
  //   if (userInst == nullptr){
  //     continue ;
  //   }
  //   if (userInst == LGIV_cmpInst){

  //     /*
  //      * We found the Use we are interested.
  //      */
  //     switch (use.getOperandNo()){
  //       case 0:
  //         operandNumber = 1;
  //         break ;
  //       case 1:
  //         operandNumber = 0;
  //         break ;
  //       default:
  //         abort();
  //     }
  //     break ;
  //   }
  // }
  // assert(operandNumber != -1);
  // auto cloneCmpInst = cast<CmpInst>(this->fetchClone(LGIV_cmpInst));
  // auto cloneLastValue = this->fetchClone(LGIV_lastValue);
  // auto cloneCurrentValue = cast<Instruction>(this->fetchClone(LGIV_currentValue));
  // cloneCmpInst->setOperand(operandNumber, lastIterationValueCasted);

  // Adjust the start and max value of the loop-goverining IV to use the corresponding startIter
  auto iterationsVector = hbTask->getIterationsVector();
  auto startIter = iterationsVector[this->loopToLevel[this->ldi] * 2 + 0];
  errs() << "startIter: " << *startIter << "\n";
  auto maxIter = iterationsVector[this->loopToLevel[this->ldi] * 2 + 1];
  errs() << "maxIter: " << *maxIter << "\n";
  IVClone->setIncomingValueForBlock(hbTask->getEntry(), startIter);
  exitConditionPhiInst->setIncomingValueForBlock(hbTask->getEntry(), maxIter);

  errs() << "task after using start/max iterations from the argument" << *(hbTask->getTaskBody()) << "\n";

  // /*
  //  * Create reduction loop here
  //  * 1. changes, after calling the loop_handler and exit, instead of going to the exitBB directly,
  //  * go to the reduction body first, reduce on the next-level array and store the final result into
  //  * the heartbeat.environment_variable.live_out.reducible.update_private_copy
  //  * 2. The initial value is not the identity value but the correspoinding phi node value from the
  //  * clone loop body if the loop_handler function wasn't invoked
  //  */
  // auto tm = this->n.getTypesManager();
  // auto numOfReducer = ConstantInt::get(tm->getIntegerType(32), this->numTaskInstances);
  // auto afterReductionBBAfterCallingLoopHandler = this->performReductionAfterCallingLoopHandler(loop, 0, callToHandler->getParent(), cmpInst, bottomHalfBB, numOfReducer);
  
  // IRBuilder<> afterReductionBB{ afterReductionBBAfterCallingLoopHandler };
  // if (afterReductionBBAfterCallingLoopHandler->getTerminator() == nullptr) {
  //   afterReductionBB.CreateBr(exitBasicBlockInTask);
  // }

  // assumption: for now, only dealing with loops that have a single exitBlock
  assert(ls->getLoopExitBasicBlocks().size() == 1 && "loop has multiple exit blocks\n");

  // step 1: create a phi node to determine the return code of the loop
  // this return code needs to be created inside the header
  headerBuilder.SetInsertPoint(&*(headerBBClone->begin()));
  this->returnCodePhiInst = cast<PHINode>(headerBuilder.CreatePHI(
    headerBuilder.getInt64Ty(),
    2,
    "returnCode"
  ));
  this->returnCodePhiInst->addIncoming(
    ConstantInt::get(headerBuilder.getInt64Ty(), 0),
    hbTask->getEntry()
  );
  this->returnCodePhiInst->addIncoming(
    callToHandler,
    latchBBClone
  );

  if (loopEnvironment->getLiveOutSize() > 0) {
    auto exitBB = *(ls->getLoopExitBasicBlocks().begin());
    auto exitBBClone = hbTask->getCloneOfOriginalBasicBlock(exitBB);
    auto exitBBTerminator = exitBBClone->getTerminator();
    IRBuilder<> exitBBBuilder{ exitBBClone };
    exitBBBuilder.SetInsertPoint(exitBBTerminator);
    // this->returnCodePhiInst = cast<PHINode>(exitBBBuilder.CreatePHI(
    //   exitBBBuilder.getInt64Ty(),
    //   1,
    //   "returnCode"
    // ));
    // this->returnCodePhiInst->addIncoming(
    //   ConstantInt::get(exitBBBuilder.getInt64Ty(), 0),
    //   hbTask->getEntry()
    // );

    // the following code does the following things
    // initialize the private copy of the reduction array and handles the reduction based on the return code
    // if (rc == 1) { // splittingLevel == myLevel
    //   redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
    // } else if (rc > 1) { // splittingLevel < myLevel
    //   redArrLiveOut0[myIndex * CACHELINE] += r_private + redArrLiveOut0Kids[0 * CACHELINE];
    // } else { // no heartbeat promotion happens or splittingLevel > myLevel
    //   redArrLiveOut0[myIndex * CACHELINE] += r_private;
    // }
    // step 2: create 3 basic blocks to do the reduction
    auto exitBlock = hbTask->getExit();
    // first block
    auto hbReductionBlock = BasicBlock::Create(this->noelle.getProgramContext(), "HB_Reduction_Block", hbTask->getTaskBody());
    IRBuilder<> hbReductionBlockBuilder{ hbReductionBlock };
    auto hbReductionBlockBrInst = hbReductionBlockBuilder.CreateBr(exitBlock);
    hbReductionBlockBuilder.SetInsertPoint(hbReductionBlockBrInst);
    // second block
    auto leftoverReductionBlock = BasicBlock::Create(this->noelle.getProgramContext(), "Leftover_Reduction_Block", hbTask->getTaskBody());
    IRBuilder<> leftoverReductionBlockBuilder { leftoverReductionBlock };
    auto leftoverReductionBlockBrInst = leftoverReductionBlockBuilder.CreateBr(exitBlock);
    leftoverReductionBlockBuilder.SetInsertPoint(leftoverReductionBlockBrInst);
    // third block
    auto secondCheckBlock = BasicBlock::Create(this->noelle.getProgramContext(), "Second_Check_Block", hbTask->getTaskBody());
    IRBuilder<> secondCheckBlockBuilder { secondCheckBlock };
    auto secondCheckCmpInst = secondCheckBlockBuilder.CreateICmpSGT(
      this->returnCodePhiInst,
      ConstantInt::get(secondCheckBlockBuilder.getInt64Ty(), 1)
    );
    auto secondCheckCondBr = secondCheckBlockBuilder.CreateCondBr(
      secondCheckCmpInst,
      leftoverReductionBlock,
      exitBlock
    );
    // conditions from the loop exit block
    auto exitBBCmpInst = exitBBBuilder.CreateICmpEQ(
      this->returnCodePhiInst,
      ConstantInt::get(secondCheckBlockBuilder.getInt64Ty(), 1)
    );
    auto exitBBCondBr = exitBBBuilder.CreateCondBr(
      exitBBCmpInst,
      hbReductionBlock,
      secondCheckBlock
    );
    exitBBTerminator->eraseFromParent();
    errs() << "task after adding three reduction blocks: " << *(hbTask->getTaskBody()) << "\n";

    // do the final reduction and store into reduction array for all live-outs
    for (auto liveOutEnvID : envUser->getEnvIDsOfLiveOutVars()) {
      auto reductionArrayForKids = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getLiveOutReductionArray(liveOutEnvID);
      errs() << "reduction array for liveOutEnv id: " << liveOutEnvID << " " << *reductionArrayForKids << "\n";

      auto int64 = IntegerType::get(this->noelle.getProgramContext(), 64);
      auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
      auto privateAccumulatedValue = this->liveOutVariableToAccumulatedPrivateCopy[liveOutEnvID];

      // heartbeat reduction [0] + [1]
      auto firstElementPtr = hbReductionBlockBuilder.CreateInBoundsGEP(
        reductionArrayForKids,
        ArrayRef<Value *>({ zeroV, cast<Value>(ConstantInt::get(int64, 0 * this->valuesInCacheLine)) }),
        "reductionArrayForKids_firstElement_Ptr"
      );
      auto firstElement = hbReductionBlockBuilder.CreateLoad(
        privateAccumulatedValue->getType(),
        firstElementPtr
      );
      auto secondElementPtr = hbReductionBlockBuilder.CreateInBoundsGEP(
        reductionArrayForKids,
        ArrayRef<Value *>({ zeroV, cast<Value>(ConstantInt::get(int64, 1 * this->valuesInCacheLine)) }),
        "reductionArrayForKids_secondElement_Ptr"
      );
      auto secondElement = hbReductionBlockBuilder.CreateLoad(
        privateAccumulatedValue->getType(),
        secondElementPtr
      );
      Value *secondAddInst = nullptr;
      if (isa<IntegerType>(privateAccumulatedValue->getType())) {
        auto firstAddInst = hbReductionBlockBuilder.CreateAdd(
          privateAccumulatedValue,
          firstElement
        );
        secondAddInst = hbReductionBlockBuilder.CreateAdd(
          firstAddInst,
          secondElement,
          "reduction_result_for_heartbeat"
        );
      } else {
        auto firstAddInst = hbReductionBlockBuilder.CreateFAdd(
          privateAccumulatedValue,
          firstElement
        );
        secondAddInst = hbReductionBlockBuilder.CreateFAdd(
          firstAddInst,
          secondElement,
          "reduction_result_for_heartbeat"
        );
      }

      // leftover reduction [0]
      auto firstElementLeftoverPtr = leftoverReductionBlockBuilder.CreateInBoundsGEP(
        reductionArrayForKids,
        ArrayRef<Value *>({ zeroV, cast<Value>(ConstantInt::get(int64, 0 * this->valuesInCacheLine)) }),
        "reductionArrayForKids_firstElement_Ptr"
      );
      auto firstElementLeftover = leftoverReductionBlockBuilder.CreateLoad(
        privateAccumulatedValue->getType(),
        firstElementLeftoverPtr
      );
      Value *firstAddLeftoverInst = nullptr;
      if (isa<IntegerType>(privateAccumulatedValue->getType())) {
        firstAddLeftoverInst= leftoverReductionBlockBuilder.CreateAdd(
          privateAccumulatedValue,
          firstElementLeftover,
          "reduction_result_for_leftover"
        );
      } else {
        firstAddLeftoverInst= leftoverReductionBlockBuilder.CreateFAdd(
          privateAccumulatedValue,
          firstElementLeftover,
          "reduction_result_for_leftover"
        );
      }

      // store the final reduction result into upper level's reduction array
      IRBuilder<> exitBlockBuilder{ exitBlock };
      exitBlockBuilder.SetInsertPoint( exitBlock->getFirstNonPHI() );
      // create phi to select from reduction result
      auto reductionResultPHI = exitBlockBuilder.CreatePHI(
        privateAccumulatedValue->getType(),
        3,
        "reductionResult"
      );
      // reductionResultPHI->addIncoming(privateAccumulatedValue, cast<Instruction>(privateAccumulatedValue)->getParent());
      reductionResultPHI->addIncoming(privateAccumulatedValue, secondCheckBlock);
      reductionResultPHI->addIncoming(secondAddInst, hbReductionBlock);
      reductionResultPHI->addIncoming(firstAddLeftoverInst, leftoverReductionBlock);

      exitBlockBuilder.CreateStore(
        reductionResultPHI,
        envUser->getEnvPtr(liveOutEnvID)
      );
    }
    errs() << "task after doing reduction: " << *(hbTask->getTaskBody()) << "\n";
  }

  // Fix the return code to use rc - 1
  auto exitBlock = hbTask->getExit();
  IRBuilder<> exitBlockBuilder{ exitBlock };
  exitBlockBuilder.SetInsertPoint( exitBlock->getTerminator() );
  auto returnRCInst = exitBlockBuilder.CreateSub(
    this->returnCodePhiInst,
    ConstantInt::get(exitBlockBuilder.getInt64Ty(), 1)
  );
  // return this new inst
  exitBlockBuilder.CreateRet(
    returnRCInst
  );
  exitBlock->getTerminator()->eraseFromParent();

  errs() << "task after changing the return value: " << *(hbTask->getTaskBody()) << "\n";
  errs() << "original function:" << *loopFunction << "\n";

  // this->tasks[0]->getTaskBody()->print(errs() << "Final Task\n");

  /*
   * Add the call to the function that includes the heartbeat loop from the pre-header of the original loop.
   */
  if (this->loopToLevel[ldi] == 0) {  // the loop we're dealing with is a root loop, invoke this root loop in the original function
    this->invokeHeartBeatFunctionAsideOriginalLoop(loop);
  } else {
    errs() << "the loop is not a root loop, need to invoke this loop from it's parent loop\n";
    this->invokeHeartBeatFunctionAsideCallerLoop(loop);
  }

  /*
   * Execute the leaf loop in chunk
   * Chunksize is supposed to be passed from the command line
   */
  if (this->loopToLevel[ldi] == this->numLevels - 1) {
    // if (this->loopToChunksize[ldi] > 1) {
      errs() << "found a loop that needs to be executed in chunk, do the chunking transformation\n";
      this->executeLoopInChunk(ldi);
    // }
  }

  return true;
}

void HeartBeatTransformation::initializeEnvironmentBuilder(
  LoopDependenceInfo *LDI, 
  std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeReduced,
  std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeSkipped,
  std::function<bool(uint32_t variableID, bool isLiveOut)> isConstantLiveInVariable
) {

  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  if (this->tasks.size() == 0) {
    errs()
        << "ERROR: Parallelization technique tasks haven't been created yet!\n"
        << "\tTheir environment builders can't be initialized until they are.\n";
    abort();
  }

  auto program = this->noelle.getProgram();
  // envBuilder is a field in ParallelizationTechnique
  this->envBuilder = new HeartBeatLoopEnvironmentBuilder(program->getContext(),
                                                         environment,
                                                         shouldThisVariableBeReduced,
                                                         shouldThisVariableBeSkipped,
                                                         isConstantLiveInVariable,
                                                         this->numTaskInstances,
                                                         this->tasks.size(),
                                                         this->numLevels);
  this->initializeLoopEnvironmentUsers();

  return;
}

void HeartBeatTransformation::initializeLoopEnvironmentUsers() {
  for (auto i = 0; i < this->tasks.size(); ++i) {
    auto task = (HeartBeatTask *)this->tasks[i];
    assert(task != nullptr);
    auto envBuilder = (HeartBeatLoopEnvironmentBuilder *)this->envBuilder;
    assert(envBuilder != nullptr);
    auto envUser = (HeartBeatLoopEnvironmentUser *)this->envBuilder->getUser(i);
    assert(envUser != nullptr);

    auto entryBlock = task->getEntry();
    IRBuilder<> entryBuilder{ entryBlock };

    // Create a bitcast instruction from i8* pointer to context array type
    this->contextBitcastInst = entryBuilder.CreateBitCast(
      task->getContextArg(),
      PointerType::getUnqual(envBuilder->getContextArrayType()),
      "contextArrayCasted"
    );

    // Calculate the base index of my context
    auto int64 = IntegerType::get(entryBuilder.getContext(), 64);
    auto myLevelIndexV = cast<Value>(ConstantInt::get(int64, this->loopToLevel[this->ldi] * 8));
    auto zeroV = entryBuilder.getInt64(0);

    // Don't load from the environment pointer if the size is
    // 0 for either single or reducible environment
    if (envBuilder->getSingleEnvironmentSize() > 0) {
      auto liveInEnvPtrGEPInst = entryBuilder.CreateInBoundsGEP(
        (Value *)contextBitcastInst,
        ArrayRef<Value *>({
          zeroV,
          ConstantInt::get(entryBuilder.getInt64Ty(), this->loopToLevel[this->ldi] * 8)
        }),
        // ArrayRef<Value *>({ myLevelIndexV, cast<Value>(ConstantInt::get(int64, 0)) }),
        "liveInEnvPtr"
      );
      auto liveInEnvBitcastInst = entryBuilder.CreateBitCast(
        cast<Value>(liveInEnvPtrGEPInst),
        PointerType::getUnqual(PointerType::getUnqual(envBuilder->getSingleEnvironmentArrayType())),
        "liveInEnvPtrCasted"
      );
      auto liveInEnvLoadInst = entryBuilder.CreateLoad(
        (Value *)liveInEnvBitcastInst,
        "liveInEnv"
      );
      envUser->setSingleEnvironmentArray(liveInEnvLoadInst);
    }
    if (envBuilder->getReducibleEnvironmentSize() > 0) {
      auto liveOutEnvPtrGEPInst = entryBuilder.CreateInBoundsGEP(
        (Value *)contextBitcastInst,
        ArrayRef<Value *>({
          zeroV,
          ConstantInt::get(entryBuilder.getInt64Ty(), this->loopToLevel[this->ldi] * 8 + 1)
        }),
        // ArrayRef<Value *>({ myLevelIndexV, cast<Value>(ConstantInt::get(int64, 1)) }),
        "liveOutEnvPtr"
      );
      auto liveOutEnvBitcastInst = entryBuilder.CreateBitCast(
        cast<Value>(liveOutEnvPtrGEPInst),
        PointerType::getUnqual(PointerType::getUnqual(envBuilder->getReducibleEnvironmentArrayType())),
        "liveOutEnvPtrCasted"
      );
      // save this liveOutEnvBitcastInst as it will be used later by the envBuilder to
      // set the liveOutEnv for kids
      envUser->setLiveOutEnvBitcastInst(liveOutEnvBitcastInst);
      auto liveOutEnvLoadInst = entryBuilder.CreateLoad(
        (Value *)liveOutEnvBitcastInst,
        "liveOutEnv"
      );
      envUser->setReducibleEnvironmentArray(liveOutEnvLoadInst);
    }

    // // Don't load from the environment pointer if the size is
    // // 0 for either single or reducible environment
    // if (envBuilder->getSingleEnvironmentSize() > 0) {
    //   auto singleEnvironmentBitcastInst = entryBuilder.CreateBitCast(
    //     task->getSingleEnvironment(),
    //     PointerType::getUnqual(envBuilder->getSingleEnvironmentArrayType()),
    //     "heartbeat.single_environment_variable.pointer");
    //   envUser->setSingleEnvironmentArray(singleEnvironmentBitcastInst);
    // }

    // if (envBuilder->getReducibleEnvironmentSize() > 0) {
    //   auto reducibleEnvironmentBitcastInst = entryBuilder.CreateBitCast(
    //     task->getReducibleEnvironment(),
    //     PointerType::getUnqual(envBuilder->getReducibleEnvironmentArrayType()),
    //     "heartbeat.reducible_environment_variable.pointer");
    //   envUser->setReducibleEnvironmentArray(reducibleEnvironmentBitcastInst);
    // }
  }

  return;
}

void HeartBeatTransformation::generateCodeToLoadLiveInVariables(LoopDependenceInfo *LDI, int taskIndex) {
  auto task = this->tasks[taskIndex];
  auto env = LDI->getEnvironment();
  auto envUser = (HeartBeatLoopEnvironmentUser *)this->envBuilder->getUser(taskIndex);

  IRBuilder<> builder{ task->getEntry() };

  // Cast constLiveInsPointer to its correct type
  auto constantLiveInsPointerGlobal = this->noelle.getProgram()->getNamedGlobal("constantLiveInsPointer");
  assert(constantLiveInsPointerGlobal != nullptr);
  errs() << "constant live-ins pointer " << *constantLiveInsPointerGlobal << "\n";
  auto constantLiveInsArrayBitCastInst = builder.CreateBitCast(
    cast<Value>(constantLiveInsPointerGlobal),
    PointerType::getUnqual(PointerType::getUnqual(ArrayType::get(builder.getInt64Ty(), this->constantLiveInsArgIndexToIndex.size()))),
    "constantLiveInsPtrCasted"
  );
  auto constantLiveInsLoadInst = builder.CreateLoad(
    constantLiveInsArrayBitCastInst,
    "constantLiveIns"
  );

  // Load constant live-in variables
  for (const auto constEnvID : envUser->getEnvIDsOfConstantLiveInVars()) {
    auto producer = env->getProducer(constEnvID);
    errs() << "constEnvID: " << constEnvID << ", value: " << *producer << "\n";
    // get the index of constant live-in in the array
    auto arg_index = this->loopToConstantLiveIns[this->ldi][producer];
    auto const_index = this->constantLiveInsArgIndexToIndex[arg_index];
    auto int64 = builder.getInt64Ty();
    auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
    auto const_index_V = cast<Value>(ConstantInt::get(int64, const_index));
    // create gep in to the constant live-in
    auto constLiveInGEPInst = builder.CreateInBoundsGEP(
      cast<Value>(constantLiveInsLoadInst),
      ArrayRef<Value *>({ zeroV, const_index_V }),
      std::string("constantLiveIn_").append(std::to_string(const_index)).append("_Ptr")
    );

    // cast to the correct type of constant variable
    auto constLiveInPointerInst = builder.CreateBitCast(
      constLiveInGEPInst, 
      PointerType::getUnqual(producer->getType()),
      std::string("constantLiveIn_").append(std::to_string(const_index)).append("_Casted")
    );

    // load from casted instruction
    auto constLiveInLoadInst = builder.CreateLoad(
      constLiveInPointerInst,
      std::string("constantLiveIn_").append(std::to_string(const_index))
    );

    task->addLiveIn(producer, constLiveInLoadInst);
  }

  // Load live-in variables pointer, then load from the pointer to get the live-in variable
  for (auto envID : envUser->getEnvIDsOfLiveInVars()) {
    auto producer = env->getProducer(envID);
    auto envPointer = envUser->createSingleEnvironmentVariablePointer(builder, envID, producer->getType());
    auto metaString = std::string{ "heartbeat_environment_variable_" }.append(std::to_string(envID));
    auto envLoad = builder.CreateLoad(envPointer, metaString);

    task->addLiveIn(producer, envLoad);
  }

  return;
}

void HeartBeatTransformation::setReducableVariablesToBeginAtIdentityValue(LoopDependenceInfo *LDI, int taskIndex) {
  assert(taskIndex < this->tasks.size());
  auto task = this->tasks[taskIndex];
  assert(task != nullptr);

  auto loopStructure = LDI->getLoopStructure();
  auto loopPreHeader = loopStructure->getPreHeader();
  auto loopPreHeaderClone = task->getCloneOfOriginalBasicBlock(loopPreHeader);
  assert(loopPreHeaderClone != nullptr);
  auto loopHeader = loopStructure->getHeader();
  auto loopHeaderClone = task->getCloneOfOriginalBasicBlock(loopHeader);
  assert(loopHeaderClone != nullptr);

  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  /*
   * Fetch the SCCDAG.
   */
  auto sccManager = LDI->getSCCManager();
  auto sccdag = sccManager->getSCCDAG();

  for (auto envID : environment->getEnvIDsOfLiveOutVars()) {
    auto isThisLiveOutVariableReducible = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
    if (!isThisLiveOutVariableReducible) {
      continue;
    }

    auto producer = environment->getProducer(envID);
    assert(producer != nullptr);
    auto producerSCC = sccdag->sccOfValue(producer);
    auto reductionVar = static_cast<BinaryReductionSCC *>(sccManager->getSCCAttrs(producerSCC));
    auto loopEntryProducerPHI = reductionVar->getPhiThatAccumulatesValuesBetweenLoopIterations();
    assert(loopEntryProducerPHI != nullptr);

    auto producerClone = cast<PHINode>(task->getCloneOfOriginalInstruction(loopEntryProducerPHI));
    errs() << "phi that accumulates values between loop iterations: " << *producerClone << "\n";
    assert(producerClone != nullptr);

    auto incomingIndex = producerClone->getBasicBlockIndex(loopPreHeaderClone);
    assert(incomingIndex != -1 && "Doesn't find loop preheader clone as an entry for the reducible phi instruction\n");

    auto identityV = reductionVar->getIdentityValue();

    producerClone->setIncomingValue(incomingIndex, identityV);
  }

  return;
}

void HeartBeatTransformation::generateCodeToStoreLiveOutVariables(LoopDependenceInfo *LDI, int taskIndex) {
  auto mm = this->noelle.getMetadataManager();
  auto env = LDI->getEnvironment();
  auto task = this->tasks[taskIndex];
  assert(task != nullptr);

  // only proceed if we have a live-out environment
  if (env->getLiveOutSize() <= 0) {
    errs() << "loop doesn't have live-out environment, no need to store anything\n";
    return;
  }

  auto entryBlock = task->getEntry();
  assert(entryBlock != nullptr);
  auto entryTerminator = entryBlock->getTerminator();
  assert(entryTerminator != nullptr);
  IRBuilder<> entryBuilder{ entryTerminator };

  auto &taskFunction = *task->getTaskBody();
  auto cfgAnalysis = this->noelle.getCFGAnalysis();

  /*
   * Fetch the loop SCCDAG
   */
  auto sccManager = LDI->getSCCManager();
  auto loopSCCDAG = sccManager->getSCCDAG();

  auto envUser = (HeartBeatLoopEnvironmentUser *)this->envBuilder->getUser(taskIndex);
  for (auto envID : envUser->getEnvIDsOfLiveOutVars()) {
    auto producer = cast<Instruction>(env->getProducer(envID));
    assert(producer != nullptr);
    if (!task->doesOriginalLiveOutHaveManyClones(producer)) {
      auto singleProducerClone = task->getCloneOfOriginalInstruction(producer);
      task->addLiveOut(producer, singleProducerClone);
    }

    auto producerClones = task->getClonesOfOriginalLiveOut(producer);
    errs() << "size of producer clones: " << producerClones.size() << "\n";

    auto envType = producer->getType();
    auto isReduced = (HeartBeatLoopEnvironmentBuilder *)this->envBuilder->hasVariableBeenReduced(envID);
    if (isReduced) {
      envUser->createReducableEnvPtr(entryBuilder, envID, envType, this->numTaskInstances, ((HeartBeatTask *)task)->getMyIndexArg());
    } else {
      errs() << "All heartbeat loop live-outs must be reducible!\n";
      abort();
    }

    // Assumption from now on, all live-out variables should be reducible
    auto envPtr = envUser->getEnvPtr(envID);

    /*
     * Fetch the reduction
     */
    auto producerSCC = loopSCCDAG->sccOfValue(producer);
    auto reductionVariable =
        static_cast<BinaryReductionSCC *>(sccManager->getSCCAttrs(producerSCC));
    assert(reductionVariable != nullptr);

    // // Reducible live-out initialization
    // auto identityV = reductionVariable->getIdentityValue();
    // // OPTIMIZATION: we might not need to initialize the live-out in the reduction array here
    // // can just store the final reduction result
    // auto newStore = entryBuilder.CreateStore(identityV, envPtr);
    // mm->addMetadata(newStore, "heartbeat.environment_variable.live_out.reducible.initialize_private_copy", std::to_string(envID));

    /*
     * Inject store instructions to propagate live-out values back to the caller
     * of the parallelized loop.
     *
     * NOTE: To support storing live outs at exit blocks and not directly where
     * the producer is executed, produce a PHI node at each store point with the
     * following incoming values: the last executed intermediate of the producer
     * that is post-dominated by that incoming block. There should only be one
     * such value assuming that store point is correctly chosen
     *
     * NOTE: This provides flexibility to parallelization schemes with modified
     * prologues or latches that have reducible live outs. Furthermore, this
     * flexibility is ONLY permitted for reducible or IV live outs as other live
     * outs can never store intermediate values of the producer.
     */
    for (auto producerClone : producerClones) {
      auto taskDS = this->noelle.getDominators(&taskFunction);

      auto insertBBs = this->determineLatestPointsToInsertLiveOutStore(LDI, taskIndex, producerClone, isReduced, *taskDS);
      assert(insertBBs.size() == 1 && "found multiple insertion point for accumulated private copy, currently doesn't handle it\n");
      for (auto BB : insertBBs) {
        auto producerValueToStore = this->fetchOrCreatePHIForIntermediateProducerValueOfReducibleLiveOutVariable(LDI, taskIndex, envID, BB, *taskDS);
        this->liveOutVariableToAccumulatedPrivateCopy[envID] = producerValueToStore;
        // IRBuilder<> liveOutBuilder(BB);
        // auto store = liveOutBuilder.CreateStore(producerValueToStore, envPtr);
        // store->removeFromParent();

        // store->insertBefore(BB->getTerminator());
        // mm->addMetadata(store, "heartbeat.environment_variable.live_out.reducible.update_private_copy", std::to_string(envID));
      }

      delete taskDS;
    }
  }

  return;
}

void HeartBeatTransformation::allocateNextLevelReducibleEnvironmentInsideTask(LoopDependenceInfo *LDI, int taskIndex) {
  auto loopStructure = LDI->getLoopStructure();
  auto loopFunction = loopStructure->getFunction();

  auto task = this->tasks[taskIndex];
  assert(task != nullptr);
  auto taskEntry = task->getEntry();
  assert(taskEntry != nullptr);
  auto entryTerminator = taskEntry->getTerminator();
  assert(entryTerminator != nullptr);

  IRBuilder<> builder{ taskEntry };
  builder.SetInsertPoint(entryTerminator);
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->allocateNextLevelReducibleEnvironmentArray(builder);
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->generateNextLevelReducibleEnvironmentVariables(builder);

  // the pointer to the liveOutEnvironment array (if any)
  // is now pointing at the liveOutEnvironment array for kids,
  // need to reset it before returns
  // Fix: only reset the environment when there's a live-out environment for the loop
  if (((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 0) {
    auto taskExit = task->getExit();
    auto exitReturnInst = taskExit->getTerminator();
    errs() << "terminator of the exit block of task " << *exitReturnInst << "\n";
    IRBuilder exitBuilder { taskExit };
    exitBuilder.SetInsertPoint(exitReturnInst);
    ((HeartBeatLoopEnvironmentUser *)this->envBuilder->getUser(0))->resetReducibleEnvironmentArray(exitBuilder);
  }

  return;
}

BasicBlock * HeartBeatTransformation::performReductionAfterCallingLoopHandler(LoopDependenceInfo *LDI, int taskIndex,BasicBlock *loopHandlerBB, Instruction *cmpInst, BasicBlock *bottomHalfBB, Value *numOfReducerV) {
  IRBuilder<> builder { loopHandlerBB };

  auto loopStructure = LDI->getLoopStructure();
  auto sccManager = LDI->getSCCManager();
  auto loopSCCDAG = sccManager->getSCCDAG();

  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  std::unordered_map<uint32_t, Instruction::BinaryOps> reducibleBinaryOps;
  std::unordered_map<uint32_t, Value *> initialValues;
  for (auto envID : environment->getEnvIDsOfLiveOutVars()) {
    auto isReduced = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
    if (!isReduced) {
      errs() << "Heartbeat live-outs must all be reducible\n";
      abort();
    }

    auto producer = environment->getProducer(envID);
    auto producerSCC = loopSCCDAG->sccOfValue(producer);
    auto producerSCCAttributes = static_cast<BinaryReductionSCC *>(sccManager->getSCCAttrs(producerSCC));
    assert(producerSCCAttributes != nullptr);

    auto reducibleOperation = producerSCCAttributes->getReductionOperation();
    reducibleBinaryOps[envID] = reducibleOperation;

    /*
     * The initial value for hb loop form could either be
     * 1. The identity value,
     * 2. The current phi value (the loop has been exectued for several iterations)
     */
    auto loopEntryProducerPHI = producerSCCAttributes->getPhiThatAccumulatesValuesBetweenLoopIterations();
    auto loopEntryProducerPHIClone = this->tasks[0]->getCloneOfOriginalInstruction(loopEntryProducerPHI);
    initialValues[envID] = castToCorrectReducibleType(builder, loopEntryProducerPHIClone, producer->getType());
  }

  auto afterReductionBB = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->reduceLiveOutVariablesInTask(taskIndex, loopHandlerBB, builder, reducibleBinaryOps, initialValues, cmpInst, bottomHalfBB, numOfReducerV);

  return afterReductionBB;
}

void HeartBeatTransformation::invokeHeartBeatFunctionAsideOriginalLoop (
  LoopDependenceInfo *LDI
) {

  // allocate constant live-ins array
  auto loopStructure = LDI->getLoopStructure();
  auto loopFunction = loopStructure->getFunction();
  auto firstBB = loopFunction->begin();
  auto firstI = firstBB->begin();
  uint32_t reducerCount = 1;
  IRBuilder<> builder{ &*firstI };
  auto constantLiveIns = builder.CreateAlloca(
    ArrayType::get(builder.getInt64Ty(), this->constantLiveInsArgIndexToIndex.size()),
    nullptr,
    "constantLiveIns"
  );

  // store constant live-ins array into global
  auto constantLiveInsGlobal = this->noelle.getProgram()->getNamedGlobal("constantLiveInsPointer");
  auto castInst = builder.CreateBitCast(
    constantLiveIns,
    PointerType::getUnqual(builder.getInt8Ty())
  );
  builder.CreateStore(
    castInst,
    constantLiveInsGlobal
  );

  // store the args into constant live-ins array
  for (auto pair : this->constantLiveInsArgIndexToIndex) {
    auto argIndex = pair.first;
    auto arrayIndex = pair.second;
    auto argV = cast<Value>(&*(loopFunction->arg_begin() + argIndex));

    auto constantLiveInArray_element_ptr = builder.CreateInBoundsGEP(
      constantLiveIns,
      ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), 0), ConstantInt::get(builder.getInt64Ty(), arrayIndex) }),
      std::string("constantLiveIn_").append(std::to_string(arrayIndex)).append("_Ptr")
    );
    auto constantLiveInArray_element_ptr_casted = builder.CreateBitCast(
      constantLiveInArray_element_ptr,
      PointerType::getUnqual(argV->getType()),
      std::string("constantLiveIn_").append(std::to_string(arrayIndex)).append("_Ptr_casted")
    );
    builder.CreateStore(
      argV,
      constantLiveInArray_element_ptr_casted
    );
  }

  // allocate context
  auto contextArrayAlloca = builder.CreateAlloca(
    // ArrayType::get(builder.getInt8PtrTy(), this->numLevels * this->valuesInCacheLine),
    ArrayType::get(builder.getInt64Ty(), this->numLevels * this->valuesInCacheLine),
    nullptr,
    "contextArray"
  );
  auto contextArrayCasted = builder.CreateBitCast(
    contextArrayAlloca,
    builder.getInt8PtrTy(),
    "contextArrayCasted"
  );

  /*
   * Create the environment.
   */
  this->allocateEnvironmentArray(LDI);

  // Store the live-in/live-out environment into the context
  auto firstBBTerminator = &*firstBB->getTerminator();
  builder.SetInsertPoint(firstBBTerminator);
  if (((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentSize() > 0) {
    errs() << "we have live-in environments" << "\n";
    // auto liveInEnvCasted = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArrayPointer();
    auto liveInEnv = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArray();
    auto gepInst = builder.CreateInBoundsGEP(
      contextArrayAlloca,
      ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine), ConstantInt::get(builder.getInt64Ty(), 0 /* the index for storing live-in environment */) })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveInEnv->getType())
    );
    builder.CreateStore(liveInEnv, gepCasted);
  }
  if (((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 0) {
    errs() << "we have live-out environments" << "\n";
    // auto liveOutEnvCasted = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArrayPointer();
    auto liveOutEnv = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArray();
    auto gepInst = builder.CreateInBoundsGEP(
      contextArrayAlloca,
      ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine), ConstantInt::get(builder.getInt64Ty(), 1 /* the index for storing live-out environment */) })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveOutEnv->getType())
    );
    builder.CreateStore(liveOutEnv, gepCasted);
  }
  errs() << "original function after allocating constantLiveIns, context and environment array" << *(LDI->getLoopStructure()->getFunction()) << "\n";

  this->populateLiveInEnvironment(LDI);

  /*
   * Fetch the first value of the loop-governing IV
   */
  auto GIV_attr = LDI->getLoopGoverningIVAttribution();
  assert(GIV_attr != nullptr);
  assert(GIV_attr->isSCCContainingIVWellFormed());
  auto& GIV = GIV_attr->getInductionVariable();
  auto firstIterationGoverningIVValue = GIV.getStartValue();
  errs() << "startIter: " << *firstIterationGoverningIVValue << "\n";

  /*
   * Fetch the last value of the loop-governing IV
   */
  auto lastIterationGoverningIVValue = GIV_attr->getExitConditionValue();
  errs() << "maxIter: " << *lastIterationGoverningIVValue << "\n";
  auto currentIVValue = GIV_attr->getValueToCompareAgainstExitConditionValue();
  errs() << "IV: " << *currentIVValue << "\n";
  
  /*
   * Fetch the pointer to the environment.
   */
  auto singleEnvPtr = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArrayPointer();
  if (singleEnvPtr) {
    errs() << "singleEnvPtr" << *singleEnvPtr << "\n";
  }
  auto reducibleEnvPtr = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArrayPointer();
  if (reducibleEnvPtr) {
    errs() << "reducibleEnvPtr" << *reducibleEnvPtr << "\n";
  }

  // /*
  //  * Call the dispatcher function that will invoke the parallelized loop.
  //  */
  // IRBuilder<> doallBuilder(this->entryPointOfParallelizedLoop);
  // auto program = this->n.getProgram();
  // auto loopDispatcherFunction = program->getFunction("loop_dispatcher");
  // assert(loopDispatcherFunction != nullptr);
  // auto argIter = loopDispatcherFunction->arg_begin();
  // auto firstIterationArgument = &*(argIter++);
  // auto lastIteratioinArgument = &*(argIter++);
  // auto taskBody = this->tasks[0]->getTaskBody();
  // assert(taskBody != nullptr);
  // auto hbEnvBuilder = (HeartBeatLoopEnvironmentBuilder *)this->envBuilder;
  // doallBuilder.CreateCall(loopDispatcherFunction, ArrayRef<Value *>({
  //   doallBuilder.CreateZExtOrTrunc(firstIterationGoverningIVValue, firstIterationArgument->getType()),
  //   doallBuilder.CreateZExtOrTrunc(lastIterationGoverningIVValue, lastIteratioinArgument->getType()),
  //   singleEnvPtr,
  //   // Hacking for now, if there's no reducible environment, which won't be alloacted,
  //   // then use the singleEnvPtr for valid code, this pointer won't get's loaded in the
  //   // child since there's no reducible environment variables
  //   // 
  //   // Potential bug: handle the live-in is empty
  //   hbEnvBuilder->getReducibleEnvironmentSize() > 0 ? reducibleEnvPtr : singleEnvPtr,
  //   taskBody
  // }));

  // // invoke the transformed loop slice of root throug loop_dispatcher
  // auto loopDispatcherFunction = this->n.getProgram()->getFunction("loop_dispatcher");
  // assert(loopDispatcherFunction != nullptr && "loop_dispatcher function not found\n");
  // IRBuilder<> doallBuilder(this->entryPointOfParallelizedLoop);
  // std::vector<Value *> loopDispatcherParameters;
  // loopDispatcherParameters.push_back(this->tasks[0]->getTaskBody());
  // loopDispatcherParameters.push_back(contextArrayCasted);
  // if (this->containsLiveOut) {
  //   loopDispatcherParameters.push_back(ConstantInt::get(doallBuilder.getInt64Ty(), 0));
  // }
  // loopDispatcherParameters.push_back(firstIterationGoverningIVValue);
  // loopDispatcherParameters.push_back(lastIterationGoverningIVValue);
  // doallBuilder.CreateCall(
  //   loopDispatcherFunction,
  //   ArrayRef<Value *>({
  //     loopDispatcherParameters
  //   })
  // );
  // errs() << "original function after invoking call to hb loop" << *(LDI->getLoopStructure()->getFunction()) << "\n";

  // we no longer make the call to the loop_dispatcher function through the compiler
  // however, we modify the source code in heartbeat.cpp to wrap the region of interest to taskparts runtime
  IRBuilder<> doallBuilder(this->entryPointOfParallelizedLoop);
  std::vector<Value *> loopSliceParameters;
  loopSliceParameters.push_back(contextArrayCasted);
  if (this->containsLiveOut) {
    loopSliceParameters.push_back(ConstantInt::get(doallBuilder.getInt64Ty(), 0));
  }
  loopSliceParameters.push_back(firstIterationGoverningIVValue);
  loopSliceParameters.push_back(lastIterationGoverningIVValue);
  doallBuilder.CreateCall(
    this->tasks[0]->getTaskBody(),
    ArrayRef<Value *>({
      loopSliceParameters
    })
  );
  errs() << "original function after invoking call to hb loop" << *(LDI->getLoopStructure()->getFunction()) << "\n";


  /*
   * Propagate the last value of live-out variables to the code outside the parallelized loop.
   */
  auto latestBBAfterDOALLCall = this->performReductionWithInitialValueToAllReducibleLiveOutVariables(LDI);

  IRBuilder<> afterDOALLBuilder{ latestBBAfterDOALLCall };
  afterDOALLBuilder.CreateBr(this->exitPointOfParallelizedLoop);
  errs() << "original function after propagating live-out variables with initial value" << *(LDI->getLoopStructure()->getFunction()) << "\n";

  return ;
}

void HeartBeatTransformation::allocateEnvironmentArray(LoopDependenceInfo *LDI) {
  auto loopStructure = LDI->getLoopStructure();
  auto loopFunction = loopStructure->getFunction();

  auto firstBB = loopFunction->begin();
  // auto firstI = firstBB->begin();
  auto firstBBTerminator = (*firstBB).getTerminator();

  /*
   * The loop_dispatcher function will only invoke one instance (with the taskID = 0) of heartbeat loop,
   * so at the top level there's only one reducer to do the reduction in the end with
   * 1. the initial value
   * 2. the accumulated value (since there's a global boolean control the execution of any heartbeat loop), a
   * heartbeat loop is either running as original or parallelized.
   * ==> solution: use the initial value
   */
  uint32_t reducerCount = 1;
  IRBuilder<> builder{ firstBBTerminator };
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->allocateSingleEnvironmentArray(builder);
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->allocateReducibleEnvironmentArray(builder);
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->generateEnvVariables(builder, reducerCount);
}

void HeartBeatTransformation::allocateEnvironmentArrayInCallerTask(HeartBeatTask *callerHBTask) {
  auto callerFunction = callerHBTask->getTaskBody();

  auto firstBB = callerFunction->begin();
  auto firstBBTerminator = (*firstBB).getTerminator();

  uint32_t reducerCount = 1;
  IRBuilder<> builder{ firstBBTerminator };
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->allocateSingleEnvironmentArray(builder);
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->allocateReducibleEnvironmentArray(builder);
  ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->generateEnvVariables(builder, reducerCount);
}

void HeartBeatTransformation::populateLiveInEnvironment(LoopDependenceInfo *LDI) {
  auto mm = this->noelle.getMetadataManager();
  auto env = LDI->getEnvironment();
  IRBuilder<> builder(this->entryPointOfParallelizedLoop);
  for (auto envID : env->getEnvIDsOfLiveInVars()) {

    /*
     * Skip the environment variable if it's not included in the builder
     */
    if (!((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->isIncludedEnvironmentVariable(envID)) {
      continue;
    }

    auto producerOfLiveIn = env->getProducer(envID);
    auto environmentVariable = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getEnvironmentVariable(envID);
    auto newStore = builder.CreateStore(producerOfLiveIn, environmentVariable);
    mm->addMetadata(newStore, "heartbeat.environment_variable.live_in.store_pointer", std::to_string(envID));
  }

  return;
}

BasicBlock * HeartBeatTransformation::performReductionWithInitialValueToAllReducibleLiveOutVariables(LoopDependenceInfo *LDI) {
  IRBuilder<> builder { this->entryPointOfParallelizedLoop };

  auto loopStructure = LDI->getLoopStructure();
  auto loopPreHeader = loopStructure->getPreHeader();
  auto sccManager = LDI->getSCCManager();
  auto loopSCCDAG = sccManager->getSCCDAG();
  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  /*
   * Collect reduction operation information needed to accumulate reducable
   * variables after parallelization execution
   */
  std::unordered_map<uint32_t, Instruction::BinaryOps> reducableBinaryOps;
  std::unordered_map<uint32_t, Value *> initialValues;
  for (auto envID : environment->getEnvIDsOfLiveOutVars()) {

    auto isReduced = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
    if (!isReduced) {
      errs() << "Heartbeat live-outs must all be reducible\n";
      abort();
    }

    /*
     * The current live-out variable has been reduced.
     *
     * Collect information about the reduction
     */
    auto producer = environment->getProducer(envID);
    auto producerSCC = loopSCCDAG->sccOfValue(producer);
    auto producerSCCAttributes =
        static_cast<BinaryReductionSCC *>(sccManager->getSCCAttrs(producerSCC));
    assert(producerSCCAttributes != nullptr);

    /*
     * Get the accumulator.
     */
    auto reducableOperation = producerSCCAttributes->getReductionOperation();
    reducableBinaryOps[envID] = reducableOperation;

    auto loopEntryProducerPHI = producerSCCAttributes->getPhiThatAccumulatesValuesBetweenLoopIterations();
    auto initValPHIIndex = loopEntryProducerPHI->getBasicBlockIndex(loopPreHeader);
    auto initialValue = loopEntryProducerPHI->getIncomingValue(initValPHIIndex);
    initialValues[envID] = castToCorrectReducibleType(builder, initialValue, producer->getType());
  }

  auto afterReductionB = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->reduceLiveOutVariablesWithInitialValue(
      this->entryPointOfParallelizedLoop,
      builder,
      reducableBinaryOps,
      initialValues);

  /*
   * If reduction occurred, then all environment loads to propagate live outs
   * need to be inserted after the reduction loop
   */
  IRBuilder<> *afterReductionBuilder;
  if (afterReductionB->getTerminator()) {
    afterReductionBuilder->SetInsertPoint(afterReductionB->getTerminator());
  } else {
    afterReductionBuilder = new IRBuilder<>(afterReductionB);
  }

  for (int envID : environment->getEnvIDsOfLiveOutVars()) {
    auto prod = environment->getProducer(envID);

    /*
     * If the environment variable isn't reduced, it is held in allocated memory
     * that needs to be loaded from in order to retrieve the value.
     */
    auto isReduced = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
    Value *envVar;
    if (isReduced) {
      envVar = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getAccumulatedReducedEnvironmentVariable(envID);
    } else {
      errs() << "heartbeat live-outs must all be reducible\n";
      abort();
      // envVar = afterReductionBuilder->CreateLoad(
      //     envBuilder->getEnvironmentVariable(envID));
    }
    assert(envVar != nullptr);

    for (auto consumer : environment->consumersOf(prod)) {
      if (auto depPHI = dyn_cast<PHINode>(consumer)) {
        depPHI->addIncoming(envVar, this->exitPointOfParallelizedLoop);
        continue;
      }
      prod->print(errs() << "Producer of environment variable:\t");
      errs() << "\n";
      errs() << "Loop not in LCSSA!\n";
      abort();
    }
  }

  /*
   * Free the memory.
   */
  delete afterReductionBuilder;

  return afterReductionB;
}

void HeartBeatTransformation::invokeHeartBeatFunctionAsideCallerLoop (
  LoopDependenceInfo *LDI
) {

  auto calleeFunction = LDI->getLoopStructure()->getFunction();

  auto callerLoop = this->loopToCallerLoop[LDI];
  errs() << "caller function is " << callerLoop->getLoopStructure()->getFunction()->getName() << "\n";

  auto callerHBTask = this->loopToHeartBeatTransformation[callerLoop]->getHeartBeatTask();
  assert(callerHBTask != nullptr && "callerHBTask hasn't been generated\n");

  // 2) find the call instruction inside the hbTask
  CallInst *callToLoopInCallerInst;
  for (auto &BB : *(callerHBTask->getTaskBody())) {
    for (auto &I : BB) {
      if (auto callInst = dyn_cast<CallInst>(&I)) {
        if (callInst->getCalledFunction() == calleeFunction) {
          callToLoopInCallerInst = callInst;
          break;
        }
      }
    }
  }
  errs() << "call to loop inside caller " << *callToLoopInCallerInst << "\n";
  IRBuilder<> liveInEnvBuilder{ callToLoopInCallerInst };

  // 3) build the live-in/out array inside the hbTask
  this->allocateEnvironmentArrayInCallerTask(callerHBTask);
  errs() << "callerTask after allocating environment for children" << *callerHBTask->getTaskBody() << "\n";

  auto firstBB = &*(callerHBTask->getTaskBody()->begin());
  IRBuilder<> builder { firstBB->getTerminator() };
  auto zeroV = builder.getInt64(0);

  // Store the live-in/live-out environment into the context
  if (((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentSize() > 0) {
    errs() << "we have live-in environments" << "\n";
    // auto liveInEnvCasted = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArrayPointer();
    auto liveInEnv = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArray();
    auto gepInst = builder.CreateInBoundsGEP(
      this->loopToHeartBeatTransformation[callerLoop]->getContextBitCastInst(),
      ArrayRef<Value *>({
        zeroV,
        ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine)
      })
      // ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine), ConstantInt::get(builder.getInt64Ty(), 0 /* the index for storing live-in environment */) })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveInEnv->getType())
    );
    builder.CreateStore(liveInEnv, gepCasted);
  }
  if (((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 0) {
    errs() << "we have live-out environments" << "\n";
    // auto liveOutEnvCasted = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArrayPointer();
    auto liveOutEnv = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArray();
    auto gepInst = builder.CreateInBoundsGEP(
      this->loopToHeartBeatTransformation[callerLoop]->getContextBitCastInst(),
      ArrayRef<Value *>({
        zeroV,
        ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine + 1)
      })
      // ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine), ConstantInt::get(builder.getInt64Ty(), 1 /* the index for storing live-out environment */) })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveOutEnv->getType())
    );
    builder.CreateStore(liveOutEnv, gepCasted);
  }
  errs() << "callerTask after storing live-in/out environment into context" << *callerHBTask->getTaskBody() << "\n";

  // this->populateLiveInEnvironment(LDI);
  // corretly let the callerHBTask to prepare the live-in environment
  std::set<uint32_t> liveInArgumentIndexInOriginalFunction;
  auto env = LDI->getEnvironment();
  for (auto liveInID : env->getEnvIDsOfLiveInVars()) {
    if (!((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->isIncludedEnvironmentVariable(liveInID)) {
      continue;
    }
    errs() << "found a liveIn with ID: " << liveInID << "\n";
    auto liveInValue = env->getProducer(liveInID);
    assert(isa<Argument>(liveInValue) && "the liveIn variable in the original function is not passed as an argument\n");

    // determine the index of this value in the caller argument
    auto index = cast<Argument>(liveInValue)->getArgNo();
    errs() << "liveIn and it's arg index in the function: " << index << "\n";

    // we found a liveInVariable in the original callee, now we need to find it's corresponding value in the original caller
    CallInst *callInstAtOriginalCaller;
    for (auto &BB : *(callerLoop->getLoopStructure()->getFunction())) {
      for (auto &I : BB) {
        if (auto callInst = dyn_cast<CallInst>(&I)) {
          if (callInst->getCalledFunction() == calleeFunction) {
            callInstAtOriginalCaller = callInst;
            errs() << "found the call in the original function to call callee function" << *callInst << "\n";
            break;
          }
        }
      }
    }

    auto callerParameter = callInstAtOriginalCaller->getArgOperand(index);
    errs() << "found the paramenter at the original caller function: " << *callerParameter << "\n";

    // now we found the orignal value corresponding to the live-in of the callee function,
    // we then need to find it's clone and let the hbTask to prepare the liveIn environment
    auto callerParameterClone = this->loopToHeartBeatTransformation[callerLoop]->getHeartBeatTask()->getCloneOfOriginalInstruction(cast<Instruction>(callerParameter));
    errs() << "clone of the liveIn in caller's hbTask: " << *callerParameterClone << "\n";

    // we finally found the correct liveIn in caller's hbTask, now let caller's hbTask to store the address of this liveIn
    auto liveInEnvArray = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArray();
    errs() << "liveInEnvArray: " << *liveInEnvArray << "\n";
    auto liveInIndex = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getIndexOLiveIn(liveInID);
    errs() << "liveInIndex: " << liveInIndex << "\n";

    // gep in the liveInEnvArray
    auto gepInst = liveInEnvBuilder.CreateInBoundsGEP(
      liveInEnvArray,
      ArrayRef<Value *>({
        liveInEnvBuilder.getInt64(0),
        liveInEnvBuilder.getInt64(liveInIndex),
      })
    );
    liveInEnvBuilder.CreateStore(callerParameterClone, gepInst);
  }
  errs() << "callerTask after storing live-in variables" << *callerHBTask->getTaskBody() << "\n";

  // okay, preparing the environment is done, now replace the call to original code in the caller hbTask
  // to the callee's hbTask
  // first thing: figuring out the global iteration state that needs to be passed
  std::vector<Value *> parametersVec;
  // 1) the context void pointer
  parametersVec.push_back(callerHBTask->getContextArg());
  // 2) myIndex if we contains live-out
  if (this->hbTask->containsLiveOutOrNot()) {
    parametersVec.push_back(liveInEnvBuilder.getInt64(0));
  }
  // push the global iterations state from 0 to myLevel - 2
  auto iterationsVect = this->hbTask->getIterationsVector();
  for (int64_t i = 0; i < (int64_t)this->hbTask->getLevel() - 2; i++) {
    parametersVec.push_back(iterationsVect[i * 2]);     // startIter
    parametersVec.push_back(iterationsVect[i * 2 + 1]); // maxIter
  }
  // 3) push the start/max iteration of the current loop
  parametersVec.push_back(callerHBTask->getCurrentIteration());
  parametersVec.push_back(callerHBTask->getMaxIteration());

  // 4) till the final step to set the startIteration and maxIteration
  //    for the nested loop
  // algorithm, inspect the original callee function and check start/max iteration,
  // if is a constantInt, use the constant int
  // else must be a function argument (the assumption is start/max iteration can be simply retrieved and not formed in any complex computation)
  auto GIV_attr = LDI->getLoopGoverningIVAttribution();
  assert(GIV_attr != nullptr);
  assert(GIV_attr->isSCCContainingIVWellFormed());
  auto IV = GIV_attr->getValueToCompareAgainstExitConditionValue();
  assert(isa<PHINode>(IV) && "the induction variable isn't a phi node\n");
  errs() << "IV: " << *IV << "\n";
  auto startIterationValue = cast<PHINode>(IV)->getIncomingValue(0);
  assert(isa<ConstantInt>(startIterationValue) || isa<Argument>(startIterationValue) && "the startIteration value in the callee hbTask isn't either a constant or function argument");
  if (isa<ConstantInt>(startIterationValue)) {
    parametersVec.push_back(ConstantInt::get(startIterationValue->getType(), cast<ConstantInt>(startIterationValue)->getSExtValue()));
  } else {  // it's an argument
    // algorithm, we know the start iteration is passed through function argument
    auto arg_index = cast<Argument>(startIterationValue)->getArgNo();
    parametersVec.push_back(callToLoopInCallerInst->getArgOperand(arg_index));
  }
  auto maxIterationValue = GIV_attr->getExitConditionValue();
  errs() << "maxIteration: " << *maxIterationValue << "\n";
  assert(isa<ConstantInt>(maxIterationValue) || isa<Argument>(maxIterationValue) && "the maxIteration value in the callee hbTask isn't either a constant or function argument");
  if (isa<ConstantInt>(maxIterationValue)) {
    parametersVec.push_back(ConstantInt::get(maxIterationValue->getType(), cast<ConstantInt>(maxIterationValue)->getSExtValue()));
  } else {
    auto arg_index = cast<Argument>(maxIterationValue)->getArgNo();
    parametersVec.push_back(callToLoopInCallerInst->getArgOperand(arg_index));
  }

  auto calleeHBTaskCallInst = liveInEnvBuilder.CreateCall(
    this->hbTask->getTaskBody(),
    ArrayRef<Value *>({
      parametersVec
    }),
    "nested_loop_return_code"
  );

  if (env->getLiveOutSize() > 0) {
    // if the original callee function has a return value (assumption, can only have at most 1 return value as live-out)
    // we then need to load this value from the reduction array and replace all uses of the original call
    assert(env->getLiveOutSize() == 1 && " invoking a callee function that has multiple live-outs!\n");
    auto liveOutID = *(env->getEnvIDsOfLiveOutVars().begin());
    errs() << "liveOutID: " << liveOutID << "\n";
    auto liveOutIndex = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getIndexOfLiveOut(liveOutID);
    errs() << "liveOutIndex: " << liveOutIndex << "\n";
    // now we have the index of the liveOutIndex, now load the result from the reductionArray allocated for the nest loop
    auto liveOutResult = liveInEnvBuilder.CreateLoad(
      ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleVariableOfIndexGivenEnvIndex(liveOutIndex, 0)
    );

    callToLoopInCallerInst->replaceAllUsesWith(liveOutResult);
  }
  callToLoopInCallerInst->eraseFromParent();

  errs() << "callerTask after calling calleeHBTask" << *callerHBTask->getTaskBody() << "\n";

  // now the call to the nested loop is done, we need to use the return code of the this call to
  // decide whether to update the exitCondition
  auto loopHandlerBlock = this->loopToHeartBeatTransformation[callerLoop]->getLoopHandlerBlock();
  auto modifyExitConditionBlock = this->loopToHeartBeatTransformation[callerLoop]->getModifyExitConditionBlock();
  auto preLoopHandlerBlock = loopHandlerBlock->getSinglePredecessor();
  auto preTerminator = preLoopHandlerBlock->getTerminator();
  liveInEnvBuilder.SetInsertPoint(preTerminator);

  auto cmpInst = liveInEnvBuilder.CreateICmpSGT(
    calleeHBTaskCallInst,
    liveInEnvBuilder.getInt64(0)
  );
  liveInEnvBuilder.CreateCondBr(
    cmpInst,
    modifyExitConditionBlock,
    loopHandlerBlock
  );

  // safe to erase this previous terminator
  preTerminator->eraseFromParent();
  errs() << "callerTask after conditional jumping after calling nested hbTask" << *callerHBTask->getTaskBody() << "\n";


  // since now we have multiple return code to dealing with, adding phi to bbs
  // 1. first phi is inserted inside the modify exit condition block
  IRBuilder<> modifyExitConditionBlockBuilder{ &(*modifyExitConditionBlock->begin()) };
  auto returnCodeTakenFromLoopHandlerBlockPhi = modifyExitConditionBlockBuilder.CreatePHI(
    calleeHBTaskCallInst->getType(),
    2,
    "returnCodeTakenBothFromCallToNestedLoopAndLoopHandler"
  );
  returnCodeTakenFromLoopHandlerBlockPhi->addIncoming(
    calleeHBTaskCallInst,
    calleeHBTaskCallInst->getParent()
  );
  returnCodeTakenFromLoopHandlerBlockPhi->addIncoming(
    this->loopToHeartBeatTransformation[callerLoop]->getCallToLoopHandler(),
    loopHandlerBlock
  );

  // 2. in the latch block, create another phi, with incoming value either from previous created phi or simply the return code from loop_handler
  auto latchBBs = callerLoop->getLoopStructure()->getLatches();
  assert(latchBBs.size() == 1 && "assume there could only be one latch block\n");
  auto latchBB = *(latchBBs.begin());
  auto latchBBClone = callerHBTask->getCloneOfOriginalBasicBlock(latchBB);
  errs() << "Clone of latchBB " << *latchBBClone << "\n";

  IRBuilder<> latchBBBuilder{ &*(latchBBClone->begin()) };
  auto returnCodeWithPotentialNoPromotionPhi = latchBBBuilder.CreatePHI(
    calleeHBTaskCallInst->getType(),
    2,
    "returnCodeWithPotentialNoPromotionPhi"
  );
  returnCodeWithPotentialNoPromotionPhi->addIncoming(
    returnCodeTakenFromLoopHandlerBlockPhi,
    modifyExitConditionBlock
  );
  returnCodeWithPotentialNoPromotionPhi->addIncoming(
    this->loopToHeartBeatTransformation[callerLoop]->getCallToLoopHandler(),
    loopHandlerBlock
  );

  // 3. now found the original phi to determine the return code in the header
  // and replace the incoming value from the latchBB to be the new phi value
  auto originalReturnCodePhi = this->loopToHeartBeatTransformation[callerLoop]->getReturnCodePhiInst();
  originalReturnCodePhi->setIncomingValueForBlock(
    latchBBClone,
    returnCodeWithPotentialNoPromotionPhi
  );
  errs() << "callerTask after fixing the phis to pass the return code value" << *callerHBTask->getTaskBody() << "\n";

  return ;
}

void HeartBeatTransformation::executeLoopInChunk(LoopDependenceInfo *ldi) {
  // errs() << "current task before chunking\n";
  // errs() << *(this->hbTask->getTaskBody()) << "\n";

  // How the loop looks like after making the chunking transformation
  // for (; startIter < maxIter; startIter += CHUNKSIZE_1) {
  //   auto low = startIter;
  //   auto high = std::min(maxIter, startIter + CHUNKSIZE_1);
  //   for (; low < high; low++) {
  //     r_private += a[i][low];
  //   }
  //   if (low == maxIter) {
  //     break;
  //   }

  //   rc = loop_handler(cxts, LEVEL_ONE, leftoverTasks, leafTasks, startIter0, maxIter0, low - 1, maxIter);
  //   if (rc > 0) {
  //     maxIter = startIter + 1;
  //   }
  // }

  auto ls = ldi->getLoopStructure();
  auto loopHeader = ls->getHeader();
  auto loopHeaderTerminator = loopHeader->getTerminator();
  auto loopHeaderTerminatorCloned = cast<BranchInst>(this->hbTask->getCloneOfOriginalInstruction(loopHeaderTerminator));
  auto loopBodyBlockCloned = loopHeaderTerminatorCloned->getSuccessor(0);
  auto loopExits = ls->getLoopExitBasicBlocks();
  assert(loopExits.size() == 1 && "hb pass doesn't handle multiple exit blocks");
  auto loopExit = *(loopExits.begin());
  auto loopExitCloned = this->hbTask->getCloneOfOriginalBasicBlock(loopExit);

  // Step 1: Create blocks for the chunk-executed loop
  auto chunkLoopHeaderBlock = BasicBlock::Create(this->hbTask->getTaskBody()->getContext(), "chunk_loop_header", this->hbTask->getTaskBody());
  IRBuilder chunkLoopHeaderBlockBuilder{ chunkLoopHeaderBlock };
  auto chunkLoopLatchBlock = BasicBlock::Create(this->hbTask->getTaskBody()->getContext(), "chunk_loop_latch", this->hbTask->getTaskBody());
  IRBuilder chunkLoopLatchBlockBuilder{ chunkLoopLatchBlock };
  auto chunkLoopExitBlock = BasicBlock::Create(this->hbTask->getTaskBody()->getContext(), "chunk_loop_exit", this->hbTask->getTaskBody());
  IRBuilder chunkLoopExitBlockBuilder{ chunkLoopExitBlock };

  // Step 2: Create uint64_t low
  //    first get the induction variable in the original loop
  auto IV_attr = ldi->getLoopGoverningIVAttribution();
  auto IV = IV_attr->getValueToCompareAgainstExitConditionValue();
  auto IV_cloned = this->hbTask->getCloneOfOriginalInstruction(IV);
  auto lowPhiNode = chunkLoopHeaderBlockBuilder.CreatePHI(
    IV_cloned->getType(),
    2,
    "low"
  );
  lowPhiNode->addIncoming(
    IV_cloned,
    IV_cloned->getParent()
  );

  // Step 3: Replace all uses of the cloned IV inside the body with low
  replaceAllUsesInsideLoopBody(ldi, IV_cloned, lowPhiNode);

  // Step: update the stride of the outer loop's IV with chunksize
  auto IV_cloned_update = cast<PHINode>(IV_cloned)->getIncomingValue(1);
  assert(isa<BinaryOperator>(IV_cloned_update));
  errs() << "update of induction variable " << *IV_cloned_update << "\n";
  cast<Instruction>(IV_cloned_update)->setOperand(1, chunkLoopHeaderBlockBuilder.getInt64(this->loopToChunksize[ldi]));

  // Step: ensure the instruction that use to determine whether keep running the loop is icmp slt but not icmp ne
  auto exitCmpInst = IV_attr->getHeaderCompareInstructionToComputeExitCondition();
  auto exitCmpInstCloned = this->hbTask->getCloneOfOriginalInstruction(exitCmpInst);
  errs() << "exit compare inst" << *exitCmpInstCloned << "\n";
  assert(isa<CmpInst>(exitCmpInstCloned) && "exit condtion determination isn't a compare inst\n");
  cast<CmpInst>(exitCmpInstCloned)->setPredicate(CmpInst::Predicate::ICMP_ULT);
  errs() << "exit compare inst after modifying the predict" << *exitCmpInstCloned << "\n";

  // Step: update induction variable low in latch block and branch to header
  auto lowUpdate = chunkLoopLatchBlockBuilder.CreateAdd(
    lowPhiNode,
    chunkLoopLatchBlockBuilder.getInt64(1),
    "lowUpdate"
  );
  chunkLoopLatchBlockBuilder.CreateBr(chunkLoopHeaderBlock);
  lowPhiNode->addIncoming(lowUpdate, chunkLoopLatchBlock);

  // Step: update the cond br in the outer loop to jump to the chunk_loop_header
  loopHeaderTerminatorCloned->setSuccessor(0, chunkLoopHeaderBlock);

  // Step: erase the original br to loop handler block to replace is with a br to chunk loop latch
  auto lastBodyBlock = this->getLoopHandlerBlock()->getSinglePredecessor();\
  auto loopHandlerBlockBr = lastBodyBlock->getTerminator();
  IRBuilder brBuilder{ loopHandlerBlockBr };
  brBuilder.CreateBr(chunkLoopLatchBlock);
  loopHandlerBlockBr->eraseFromParent();

  // Step: compare low == maxIter and jump either to the exit block of outer loop or the loop_handler_block
  auto lowMaxIterCmpInst = chunkLoopExitBlockBuilder.CreateICmpEQ(
    lowPhiNode,
    this->hbTask->getMaxIteration()
  );
  chunkLoopExitBlockBuilder.CreateCondBr(
    lowMaxIterCmpInst,
    loopExitCloned,
    this->getLoopHandlerBlock()
  );

  // Step 4: Create phis for all live-out variables
  auto loopEnv = ldi->getEnvironment();
  for (auto liveOutVarID : loopEnv->getEnvIDsOfLiveOutVars()) {
    auto producer = loopEnv->getProducer(liveOutVarID);
    auto producerCloned = this->hbTask->getCloneOfOriginalInstruction(cast<Instruction>(producer));
    errs() << "clone of live-out variable" << *producerCloned << "\n";

    auto liveOutPhiNode = chunkLoopHeaderBlockBuilder.CreatePHI(
      producerCloned->getType(),
      2,
      std::string("liveOut_ID_").append(std::to_string(liveOutVarID))
    );
    liveOutPhiNode->addIncoming(
      producerCloned,
      producerCloned->getParent()
    );
    liveOutPhiNode->addIncoming(
      cast<PHINode>(producerCloned)->getIncomingValue(1),
      chunkLoopLatchBlock
    );

    // replace the incoming value in the cloned header
    cast<PHINode>(producerCloned)->setIncomingValue(1, liveOutPhiNode);

    // add another incoming value to the propagated value in the exit block of the outer loop
    cast<PHINode>(this->liveOutVariableToAccumulatedPrivateCopy[liveOutVarID])->addIncoming(liveOutPhiNode, chunkLoopExitBlock);

    // Replace all uses of the cloned liveOut inside the loop body with liveOutPhiNode
    replaceAllUsesInsideLoopBody(ldi, producerCloned, liveOutPhiNode);
  }

  // Step 5: determine the max value
  auto startIterPlusChunk = chunkLoopHeaderBlockBuilder.CreateAdd(
    IV_cloned,
    chunkLoopHeaderBlockBuilder.getInt64(this->loopToChunksize[ldi])
  );
  auto highCompareInst = chunkLoopHeaderBlockBuilder.CreateICmpSLT(
    this->hbTask->getMaxIteration(),
    startIterPlusChunk
  );
  auto high = chunkLoopHeaderBlockBuilder.CreateSelect(
    highCompareInst,
    this->hbTask->getMaxIteration(),
    startIterPlusChunk,
    "high"
  );
  auto loopCompareInst = chunkLoopHeaderBlockBuilder.CreateICmpSLT(
    lowPhiNode,
    high
  );
  chunkLoopHeaderBlockBuilder.CreateCondBr(
    loopCompareInst,
    loopBodyBlockCloned,
    chunkLoopExitBlock
  );

  // replace the call to loop_handler with low - 1
  IRBuilder loopHandlerBlockBuilder{ this->getLoopHandlerBlock() };
  loopHandlerBlockBuilder.SetInsertPoint(this->getCallToLoopHandler());
  auto lowSubOneInst = loopHandlerBlockBuilder.CreateSub(
    lowPhiNode,
    loopHandlerBlockBuilder.getInt64(1),
    "lowSubOne"
  );
  auto myStartIterIndexInCall = 3 + this->loopToLevel[ldi] * 2;
  this->getCallToLoopHandler()->setArgOperand(myStartIterIndexInCall, lowSubOneInst);

  return;
}

void HeartBeatTransformation::replaceAllUsesInsideLoopBody(LoopDependenceInfo *ldi, Value *originalVal, Value *replacedVal) {
  // Step 1: collect all body basic blocks
  auto ls = ldi->getLoopStructure();
  auto bbs = ls->getBasicBlocks();

  // Step 2: remove collected basic blocks that are
  // preheader, header, latches, exits
  bbs.erase(ls->getPreHeader());
  bbs.erase(ls->getHeader());
  for (auto latch : ls->getLatches()) {
    bbs.erase(latch);
  }
  for (auto exit : ls->getLoopExitBasicBlocks()) {
    bbs.erase(exit);
  }

  // Step 3: create cloned blocks
  std::unordered_set<BasicBlock *> bbsCloned;
  for (auto bb : bbs) {
    bbsCloned.insert(this->hbTask->getCloneOfOriginalBasicBlock(bb));
  }

  std::vector<User *> usersToReplace;
  for (auto &use : originalVal->uses()) {
    auto user = use.getUser();
    auto userBB = cast<Instruction>(user)->getParent();
    if (bbsCloned.find(userBB) != bbsCloned.end()) {
      errs() << "found a user " << *user << "\n";
      usersToReplace.push_back(user);
    }
  }

  for (auto user : usersToReplace) {
    user->replaceUsesOfWith(originalVal, replacedVal);
  }
}
