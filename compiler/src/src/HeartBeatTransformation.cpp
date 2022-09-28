#include "HeartBeatTask.hpp"
#include "HeartBeatTransformation.hpp"
#include "noelle/core/Reduction.hpp"

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
    tm->getVoidPointerType(), // pointer to all live-in variables environment
    tm->getVoidPointerType(), // pointer to all reducible live-out variables array environment
    tm->getIntegerType(64)
  });
  this->taskSignature = FunctionType::get(tm->getVoidType(), funcArgTypes, false);

  return ;
}

bool HeartBeatTransformation::apply (
  LoopDependenceInfo *loop,
  Heuristics *h
  ){

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
  loopEnvironment->printEnvironmentInfo();

  /*
   * Generate an empty task for the heartbeat execution.
   */
  auto hbTask = new HeartBeatTask(this->taskSignature, *program);
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(loop, { hbTask });

  /*
   * Allocate memory for all environment variables
   */
  auto isReducible = [](uint32_t id, bool isLiveOut) -> bool {
    if (!isLiveOut){
      return false;
    }
    return true;
  };
  auto shouldThisVariableBeSkipped = [](uint32_t variableID, bool isLiveOut) -> bool { 
    return false; 
  };
  this->initializeEnvironmentBuilder(loop, isReducible, shouldThisVariableBeSkipped);

  /*
   * Clone loop into the single task used by DOALL
   */
  this->cloneSequentialLoop(loop, 0);

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

  /*
   * Clone memory objects that are not blocked by RAW data dependences
   */
  auto ltm = loop->getLoopTransformationsManager();
  if (ltm->isOptimizationEnabled(LoopDependenceInfoOptimization::MEMORY_CLONING_ID)) {
    this->cloneMemoryLocationsLocallyAndRewireLoop(loop, 0);
  }

  /*
   * Fix the data flow within the parallelized loop by redirecting operands of
   * cloned instructions to refer to the other cloned instructions. Currently,
   * they still refer to the original loop's instructions.
   */
  this->adjustDataFlowToUseClones(loop, 0);

  /*
   * Handle the reduction variables.
   */
  this->setReducableVariablesToBeginAtIdentityValue(loop, 0);

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
   * Allocate the reduction environment array for the next level inside task
   */
  this->allocateNextLevelReducibleEnvironmentInsideTask(loop, 0);

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
  auto singleEnvPtr = &*(argIter++);
  auto reducibleEnvPtr = &*(argIter++);
  auto taskID = &*(argIter++);
  auto hbEnvBuilder = (HeartBeatLoopEnvironmentBuilder *)this->envBuilder;
  auto callToHandler = cast<CallInst>(bbBuilder.CreateCall(loopHandlerFunction, ArrayRef<Value *>({
    bbBuilder.CreateZExtOrTrunc(cloneCurrentIVValue, firstIterationValue->getType()),
    lastIterationValue,
    singleEnvPtr,
    // if there's no reducible live-out environment, which means the next level reducible environment won't be allocated
    // use the original reducible environment instead
    hbEnvBuilder->getReducibleEnvironmentSize() > 0 ? hbEnvBuilder->getNextLevelEnvironmentArrayVoidPtr() : reducibleEnvPtr,
    taskID,
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
   * Create the bitcast instructions at the entry block of the task to match the type of the GIV
   */
  IRBuilder<> entryTaskBuilder{ hbTask->getEntry() };
  entryTaskBuilder.SetInsertPoint(&(*hbTask->getEntry()->begin()));
  auto firstIterationValueCasted = entryTaskBuilder.CreateZExtOrTrunc(firstIterationValue, cloneCurrentIVValue->getType());
  auto lastIterationValueCasted = entryTaskBuilder.CreateZExtOrTrunc(lastIterationValue, cloneCurrentIVValue->getType());

  /*
   * Adjust the first starting value of the loop-governing IV to use the first parameter of the task.
   */
  auto& GIV = GIV_attr->getInductionVariable();
  auto originalPHI = GIV.getLoopEntryPHI();
  auto clonePHI = cast<PHINode>(this->fetchClone(originalPHI));
  assert(clonePHI != nullptr);
  clonePHI->setIncomingValueForBlock(hbTask->getEntry(), firstIterationValueCasted);

  /*
   * Adjust the exit condition value of the loop-governing IV to use the second parameter of the task.
   *
   * Step 1: find the Use of the exit value used in the compare instruction of the loop-governing IV.
   */
  auto LGIV_cmpInst = GIV_attr->getHeaderCompareInstructionToComputeExitCondition();
  auto LGIV_lastValue = GIV_attr->getExitConditionValue();
  auto LGIV_currentValue = GIV_attr->getValueToCompareAgainstExitConditionValue();
  int32_t operandNumber = -1;
  for (auto &use: LGIV_currentValue->uses()){
    auto user = use.getUser();
    auto userInst = dyn_cast<Instruction>(user);
    if (userInst == nullptr){
      continue ;
    }
    if (userInst == LGIV_cmpInst){

      /*
       * We found the Use we are interested.
       */
      switch (use.getOperandNo()){
        case 0:
          operandNumber = 1;
          break ;
        case 1:
          operandNumber = 0;
          break ;
        default:
          abort();
      }
      break ;
    }
  }
  assert(operandNumber != -1);
  auto cloneCmpInst = cast<CmpInst>(this->fetchClone(LGIV_cmpInst));
  auto cloneLastValue = this->fetchClone(LGIV_lastValue);
  auto cloneCurrentValue = cast<Instruction>(this->fetchClone(LGIV_currentValue));
  cloneCmpInst->setOperand(operandNumber, lastIterationValueCasted);

  /*
   * Create reduction loop here
   * 1. changes, after calling the loop_handler and exit, instead of going to the exitBB directly,
   * go to the reduction body first, reduce on the next-level array and store the final result into
   * the heartbeat.environment_variable.live_out.reducible.update_private_copy
   * 2. The initial value is not the identity value but the correspoinding phi node value from the
   * clone loop body if the loop_handler function wasn't invoked
   */
  auto tm = this->n.getTypesManager();
  auto numOfReducer = ConstantInt::get(tm->getIntegerType(32), this->numTaskInstances);
  auto afterReductionBBAfterCallingLoopHandler = this->performReductionAfterCallingLoopHandler(loop, 0, callToHandler->getParent(), cmpInst, bottomHalfBB, numOfReducer);
  
  IRBuilder<> afterReductionBB{ afterReductionBBAfterCallingLoopHandler };
  if (afterReductionBBAfterCallingLoopHandler->getTerminator() == nullptr) {
    afterReductionBB.CreateBr(exitBasicBlockInTask);
  }

  this->tasks[0]->getTaskBody()->print(errs() << "Final Task\n");

  /*
   * Add the call to the function that includes the heartbeat loop from the pre-header of the original loop.
   */
  this->invokeHeartBeatFunctionAsideOriginalLoop(loop);

  return true;
}

void HeartBeatTransformation::initializeEnvironmentBuilder(LoopDependenceInfo *LDI, 
  std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeReduced,
  std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeSkipped) {

  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  if (this->tasks.size() == 0) {
    errs()
        << "ERROR: Parallelization technique tasks haven't been created yet!\n"
        << "\tTheir environment builders can't be initialized until they are.\n";
    abort();
  }

  auto program = this->noelle.getProgram();
  this->envBuilder = new HeartBeatLoopEnvironmentBuilder(program->getContext(),
                                                         environment,
                                                         shouldThisVariableBeReduced,
                                                         shouldThisVariableBeSkipped,
                                                         this->numTaskInstances,
                                                         this->tasks.size());
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
    
    // Don't load from the environment pointer if the size is 
    // 0 for either single or reducible environment
    if (envBuilder->getSingleEnvironmentSize() > 0) {
      auto singleEnvironmentBitcastInst = entryBuilder.CreateBitCast(
        task->getSingleEnvironment(),
        PointerType::getUnqual(envBuilder->getSingleEnvironmentArrayType()),
        "heartbeat.single_environment_variable.pointer");
      envUser->setSingleEnvironmentArray(singleEnvironmentBitcastInst);
    }

    if (envBuilder->getReducibleEnvironmentSize() > 0) {
      auto reducibleEnvironmentBitcastInst = entryBuilder.CreateBitCast(
        task->getReducibleEnvironment(),
        PointerType::getUnqual(envBuilder->getReducibleEnvironmentArrayType()),
        "heartbeat.reducible_environment_variable.pointer");
      envUser->setReducibleEnvironmentArray(reducibleEnvironmentBitcastInst);
    }
  }

  return;
}

void HeartBeatTransformation::generateCodeToLoadLiveInVariables(LoopDependenceInfo *LDI, int taskIndex) {
  auto task = this->tasks[taskIndex];
  auto env = LDI->getEnvironment();
  auto envUser = (HeartBeatLoopEnvironmentUser *)this->envBuilder->getUser(taskIndex);

  IRBuilder<> builder{ task->getEntry() };
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
    auto reductionVar = static_cast<Reduction *>(sccManager->getSCCAttrs(producerSCC));
    auto loopEntryProducerPHI = reductionVar->getPhiThatAccumulatesValuesBetweenLoopIterations();
    assert(loopEntryProducerPHI != nullptr);

    auto producerClone = cast<PHINode>(task->getCloneOfOriginalInstruction(loopEntryProducerPHI));
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

    auto envType = producer->getType();
    auto isReduced = (HeartBeatLoopEnvironmentBuilder *)this->envBuilder->hasVariableBeenReduced(envID);
    if (isReduced) {
      envUser->createReducableEnvPtr(entryBuilder, envID, envType, this->numTaskInstances, task->getTaskInstanceID());
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
        static_cast<Reduction *>(sccManager->getSCCAttrs(producerSCC));
    assert(reductionVariable != nullptr);

    // Reducible live-out initialization
    auto identityV = reductionVariable->getIdentityValue();
    auto newStore = entryBuilder.CreateStore(identityV, envPtr);
    mm->addMetadata(newStore, "heartbeat.environment_variable.live_out.reducible.initialize_private_copy", std::to_string(envID));

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
      for (auto BB : insertBBs) {
        auto producerValueToStore = this->fetchOrCreatePHIForIntermediateProducerValueOfReducibleLiveOutVariable(LDI, taskIndex, envID, BB, *taskDS);

        IRBuilder<> liveOutBuilder(BB);
        auto store = liveOutBuilder.CreateStore(producerValueToStore, envPtr);
        store->removeFromParent();

        store->insertBefore(BB->getTerminator());
        mm->addMetadata(store, "heartbeat.environment_variable.live_out.reducible.update_private_copy", std::to_string(envID));
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
    auto producerSCCAttributes = static_cast<Reduction *>(sccManager->getSCCAttrs(producerSCC));
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
  auto singleEnvPtr = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArrayPointer();
  auto reducibleEnvPtr = ((HeartBeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArrayPointer();

  /*
   * Call the dispatcher function that will invoke the parallelized loop.
   */
  IRBuilder<> doallBuilder(this->entryPointOfParallelizedLoop);
  auto program = this->n.getProgram();
  auto loopDispatcherFunction = program->getFunction("loop_dispatcher");
  assert(loopDispatcherFunction != nullptr);
  auto argIter = loopDispatcherFunction->arg_begin();
  auto firstIterationArgument = &*(argIter++);
  auto lastIteratioinArgument = &*(argIter++);
  auto taskBody = this->tasks[0]->getTaskBody();
  assert(taskBody != nullptr);
  auto hbEnvBuilder = (HeartBeatLoopEnvironmentBuilder *)this->envBuilder;
  doallBuilder.CreateCall(loopDispatcherFunction, ArrayRef<Value *>({
    doallBuilder.CreateZExtOrTrunc(firstIterationGoverningIVValue, firstIterationArgument->getType()),
    doallBuilder.CreateZExtOrTrunc(lastIterationGoverningIVValue, lastIteratioinArgument->getType()),
    singleEnvPtr,
    // Hacking for now, if there's no reducible environment, which won't be alloacted,
    // then use the singleEnvPtr for valid code, this pointer won't get's loaded in the
    // child since there's no reducible environment variables
    // 
    // Potential bug: handle the live-in is empty
    hbEnvBuilder->getReducibleEnvironmentSize() > 0 ? reducibleEnvPtr : singleEnvPtr,
    taskBody
  }));

  /*
   * Propagate the last value of live-out variables to the code outside the parallelized loop.
   */
  auto latestBBAfterDOALLCall = this->performReductionWithInitialValueToAllReducibleLiveOutVariables(LDI);

  IRBuilder<> afterDOALLBuilder{ latestBBAfterDOALLCall };
  afterDOALLBuilder.CreateBr(this->exitPointOfParallelizedLoop);

  return ;
}

void HeartBeatTransformation::allocateEnvironmentArray(LoopDependenceInfo *LDI) {
  auto loopStructure = LDI->getLoopStructure();
  auto loopFunction = loopStructure->getFunction();

  auto firstBB = loopFunction->begin();
  auto firstI = firstBB->begin();

  /*
   * The loop_dispatcher function will only invoke one instance (with the taskID = 0) of heartbeat loop,
   * so at the top level there's only one reducer to do the reduction in the end with
   * 1. the initial value
   * 2. the accumulated value (since there's a global boolean control the execution of any heartbeat loop), a
   * heartbeat loop is either running as original or parallelized.
   * ==> solution: use the initial value
   */
  uint32_t reducerCount = 1;
  IRBuilder<> builder{ &*firstI };
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
        static_cast<Reduction *>(sccManager->getSCCAttrs(producerSCC));
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