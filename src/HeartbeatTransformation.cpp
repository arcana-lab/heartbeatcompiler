#include "HeartbeatTask.hpp"
#include "HeartbeatTransformation.hpp"
#include "noelle/core/BinaryReductionSCC.hpp"
#include "noelle/core/Architecture.hpp"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/Casting.h"

using namespace llvm;
using namespace arcana::noelle;

HeartbeatTransformation::HeartbeatTransformation (
  Noelle &noelle,
  StructType *task_memory_t,
  uint64_t nestID,
  LoopContent *ldi,
  uint64_t numLevels,
  std::unordered_map<LoopContent *, uint64_t> &loopToLevel,
  std::unordered_map<LoopContent *, std::unordered_set<Value *>> &loopToSkippedLiveIns,
  std::unordered_map<int, int> &constantLiveInsArgIndexToIndex,
  std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> &loopToConstantLiveIns,
  std::unordered_map<LoopContent *, HeartbeatTransformation *> &loopToHeartbeatTransformation,
  std::unordered_map<LoopContent *, LoopContent *> &loopToCallerLoop
) : DOALL{noelle},
    task_memory_t{task_memory_t},
    nestID{nestID},
    ldi{ldi},
    n{noelle},
    numLevels{numLevels},
    loopToLevel{loopToLevel},
    loopToSkippedLiveIns{loopToSkippedLiveIns},
    constantLiveInsArgIndexToIndex{constantLiveInsArgIndexToIndex},
    loopToConstantLiveIns{loopToConstantLiveIns},
    hbTask{nullptr},
    loopToHeartbeatTransformation{loopToHeartbeatTransformation},
    loopToCallerLoop{loopToCallerLoop} {

  // set cacheline element size
  this->valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);

  /*
   * Create the slice task signature
   */
  auto tm = noelle.getTypesManager();
  std::vector<Type *> sliceTaskSignatureTypes {
    PointerType::getUnqual(tm->getIntegerType(64)),     // uint64_t *cxt
    PointerType::getUnqual(tm->getIntegerType(64)),     // uint64_t *constLiveIns
    tm->getIntegerType(64),                             // uint64_t myIndex
    PointerType::getUnqual(this->task_memory_t) // tmem
  };
  this->sliceTaskSignature = FunctionType::get(tm->getIntegerType(64), ArrayRef<Type *>(sliceTaskSignatureTypes), false);

  errs() << "create function signature for slice task " << ldi->getLoopStructure()->getFunction()->getName() << ": " << *(this->sliceTaskSignature) << "\n";

  return ;
}

bool HeartbeatTransformation::apply (
  LoopContent *loop,
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
  errs() << "original function:\n" << *loopFunction << "\n";
  errs() << "pre-header:" << *(ls->getPreHeader()) << "\n";
  errs() << "header:" << *(ls->getHeader()) << "\n";
  errs() << "first body:" << *(ls->getSuccessorWithinLoopOfTheHeader()) << "\n";
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
  this->hbTask = new HeartbeatTask(this->sliceTaskSignature, *program, this->loopToLevel[this->ldi],
    std::string("HEARTBEAT_nest").append(std::to_string(this->nestID)).append("_loop").append(std::to_string(this->loopToLevel[ldi])).append("_slice")
  );
  // 12/14/2023 by Yian:
  // The lastest NOELLE task abstraction inserts a 'ret void' instruction everytime a task function is created.
  // However, this 'ret void' is not used by the loop-slice task. (loop-slice task has a return type of i64).
  // Therefore erase the 'ret void' instruction from the task after its creation.
  this->hbTask->getExit()->getFirstNonPHI()->eraseFromParent();

  errs() << "initial task body:\n" << *(hbTask->getTaskBody()) << "\n";
  this->fromTaskIDToUserID[this->hbTask->getID()] = 0;
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(
    loop,
    { hbTask }
  );
  hbTask->extractFuncArgs();
  errs() << "task after adding predecessor and successors bb:\n" << *(hbTask->getTaskBody()) << "\n";

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
      errs() << "skip " << *(loopEnvironment->getProducer(variableID)) << " of id " << variableID << " because of the skipped liveIns\n";
      return true;
    }
    // if a variable is passed as a parameter to a callee and used only as initial/exit condition of the loop, then skip it here
    auto producer = loopEnvironment->getProducer(variableID);
    auto IVM = loop->getInductionVariableManager();
    auto GIV = IVM->getLoopGoverningInductionVariable();
    auto IV = GIV->getInductionVariable();
    auto startValue = IV->getStartValue();
    auto exitValue = GIV->getExitConditionValue();
    if (producer->getNumUses() == 1 && (producer == startValue || producer == exitValue)) {
      errs() << "skip " << *(loopEnvironment->getProducer(variableID)) << " of id " << variableID << " because of initial/exit condition of the callee loop\n";
      return true;
    }
    // if a variable is a constant live-in variable (loop invariant), skip it
    auto envProducer = loopEnvironment->getProducer(variableID);
    if (this->loopToConstantLiveIns[this->ldi].find(envProducer) != this->loopToConstantLiveIns[this->ldi].end()) {
      errs() << "skip " << *envProducer << " of id " << variableID << " because of constant live-in\n";
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
   * Clone loop into the single task used by Heartbeat
   */
  this->cloneSequentialLoop(loop, 0);
  errs() << "task after cloning sequential loop:\n" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Load all loop live-in values at the entry point of the task.
   */
  auto envUser = (HeartbeatLoopEnvironmentUser *)this->envBuilder->getUser(0);
  for (auto envID : loopEnvironment->getEnvIDsOfLiveInVars()) {
    envUser->addLiveIn(envID);
  }
  for (auto envID : loopEnvironment->getEnvIDsOfLiveOutVars()) {
    envUser->addLiveOut(envID);
  }
  this->generateCodeToLoadLiveInVariables(loop, 0);
  errs() << "task after loading live-in variables:\n" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Clone memory objects that are not blocked by RAW data dependences
   */
  auto ltm = loop->getLoopTransformationsManager();
  if (ltm->isOptimizationEnabled(LoopContentOptimization::MEMORY_CLONING_ID)) {
    this->cloneMemoryLocationsLocallyAndRewireLoop(loop, 0);
  }
  errs() << "task after cloning memory objects:\n" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Fix the data flow within the parallelized loop by redirecting operands of
   * cloned instructions to refer to the other cloned instructions. Currently,
   * they still refer to the original loop's instructions.
   */
  this->hbTask->adjustDataAndControlFlowToUseClones();
  errs() << "task after fixing data flow:\n" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Handle the reduction variables.
   */
  this->setReducableVariablesToBeginAtIdentityValue(loop, 0);
  errs() << "task after seting reducible variables:\n" << *(hbTask->getTaskBody()) << "\n";

  /*
   * Add the jump from the entry of the function after loading all live-ins to the header of the cloned loop.
   */
  auto headerClone = this->hbTask->getCloneOfOriginalBasicBlock(ls->getHeader());
  IRBuilder<> entryBuilder(this->hbTask->getEntry());
  entryBuilder.CreateBr(headerClone);
  
  /*
   * Add the final return to the single task's exit block.
   */
  IRBuilder<> exitB(hbTask->getExit());
  exitB.CreateRetVoid();
  errs() << "task after adding jump and return to loop\n" << *(hbTask->getTaskBody()) << "\n";

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
  errs() << "task after allocate reducible environment for kids\n" << *(hbTask->getTaskBody()) << "\n";


  // iteration builder
  IRBuilder<> iterationBuilder{ cast<Instruction>(this->contextBitcastInst)->getParent() };
  iterationBuilder.SetInsertPoint(cast<Instruction>(this->contextBitcastInst)->getNextNode());

  // Adjust the start value of the loop-goverining IV to use the value stored by the parent
  auto IVM = loop->getInductionVariableManager();
  auto GIV = IVM->getLoopGoverningInductionVariable();
  assert(GIV != nullptr);
  assert(GIV->isSCCContainingIVWellFormed());
  auto IV = GIV->getInductionVariable()->getLoopEntryPHI();
  auto IVClone = cast<PHINode>(hbTask->getCloneOfOriginalInstruction(IV));
  this->currentIteration = IVClone;
  this->startIterationAddress = iterationBuilder.CreateInBoundsGEP(
    ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getContextArrayType(),
    this->contextBitcastInst,
    ArrayRef<Value *>({
      iterationBuilder.getInt64(0),
      iterationBuilder.getInt64(this->hbTask->getLevel() * this->valuesInCacheLine + this->startIterationIndex)
    }),
    "startIteration_addr"
  );
  this->startIteration = iterationBuilder.CreateLoad(
    iterationBuilder.getInt64Ty(),
    this->startIterationAddress,
    "startIteration"
  );
  errs() << "startIteration: " << *this->startIteration << "\n";
  IVClone->setIncomingValueForBlock(hbTask->getEntry(), this->startIteration);

  /*
   * Adjust the exit condition value of the loop-governing IV to use the value stored by the parent
   */
  this->maxIterationAddress = iterationBuilder.CreateInBoundsGEP(
    ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getContextArrayType(),
    this->contextBitcastInst,
    ArrayRef<Value *>({
      iterationBuilder.getInt64(0),
      iterationBuilder.getInt64(this->hbTask->getLevel() * this->valuesInCacheLine + this->maxIterationIndex)
    }),
    "maxIteration_addr"
  );
  this->maxIteration = iterationBuilder.CreateLoad(
    iterationBuilder.getInt64Ty(),
    this->maxIterationAddress,
    "maxIteration"
  );
  errs() << "maxIteration: " << *maxIteration << "\n";
  auto LGIV_cmpInst = GIV->getHeaderCompareInstructionToComputeExitCondition();
  auto LGIV_lastValue = GIV->getExitConditionValue();
  auto LGIV_currentValue = GIV->getValueToCompareAgainstExitConditionValue();
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
  auto cloneCmpInst = cast<CmpInst>(this->hbTask->getCloneOfOriginalInstruction(cast<Instruction>(LGIV_cmpInst)));
  cloneCmpInst->setOperand(operandNumber, maxIteration);

  errs() << "task after using start/max iterations set by the parent\n" << *(hbTask->getTaskBody()) << "\n";

  auto bbs = ls->getLatches();
  assert(bbs.size() == 1 && "assumption: only has one latch of the loop\n");
  auto latchBB = *(bbs.begin());
  auto latchBBClone = hbTask->getCloneOfOriginalBasicBlock(latchBB);
  auto bodyBB = latchBB->getSinglePredecessor();
  assert(bodyBB != nullptr && "latch doesn't have a single predecessor\n");
  auto bodyBBClone = hbTask->getCloneOfOriginalBasicBlock(bodyBB);
  auto loopExitBBs = ls->getLoopExitBasicBlocks();
  assert(loopExitBBs.size() == 1 && "loop has multiple exit blocks!\n");
  auto loopExitBB = *(loopExitBBs.begin());
  auto loopExitBBClone = hbTask->getCloneOfOriginalBasicBlock(loopExitBB);

  // 07/30 By Yian: only insert the promotion_handler call for leaf loops
  if (this->loopToLevel[ldi] == this->numLevels - 1) {
    /*
     * Fetch the heartbeat polling and loop handler function
     */
    auto pollingFunction = program->getFunction("heartbeat_polling");
    assert(pollingFunction != nullptr);
    errs() << "polling function\n" << *pollingFunction << "\n";
    auto loopHandlerFunction = program->getFunction("promotion_handler");
    assert(loopHandlerFunction != nullptr);
    errs() << "promotion_handler function\n" << *loopHandlerFunction << "\n";

    /*
     * Create a new bb to invoke the polling after the body
     */
    this->pollingBlock = BasicBlock::Create(hbTask->getTaskBody()->getContext(), "polling_block", hbTask->getTaskBody());

    /*
     * Create a new bb to invoke the loop handler after the polling block
     */
    this->loopHandlerBlock = BasicBlock::Create(hbTask->getTaskBody()->getContext(), "promotion_handler_block", hbTask->getTaskBody());

    // modify the bodyBB to jump to the polling block
    IRBuilder<> bodyBuilder{ bodyBBClone };
    auto bodyTerminator = bodyBBClone->getTerminator();
    bodyBuilder.SetInsertPoint(bodyTerminator);
    bodyBuilder.CreateBr(
      pollingBlock
    );
    bodyTerminator->eraseFromParent();

    // invoking the polling function to either jump to the promotion_handler_block or latch block
    IRBuilder<> pollingBlockBuilder{ pollingBlock };
    this->callToPollingFunction = pollingBlockBuilder.CreateCall(
      pollingFunction,
      ArrayRef<Value *>({
        hbTask->getHBMemoryArg()
      })
    );
    // invoke llvm.expect.i1 function
    auto llvmExpectFunction = Intrinsic::getDeclaration(program, Intrinsic::expect, ArrayRef<Type *>({ pollingBlockBuilder.getInt1Ty() }));
    assert(llvmExpectFunction != nullptr);
    errs() << "llvm.expect function\n" << *llvmExpectFunction << "\n";
    auto llvmExpectCall = pollingBlockBuilder.CreateCall(
      llvmExpectFunction,
      ArrayRef<Value *>({
        callToPollingFunction,
        pollingBlockBuilder.getInt1(0)
      })
    );
    pollingBlockBuilder.CreateCondBr(
      llvmExpectCall,
      loopHandlerBlock,
      latchBBClone
    );

    /*
     * Call the loop_hander in the promotion_handler block and compare the return code
     * to decide whether to exit the loop directly
     */
    IRBuilder<> loopHandlerBuilder{ loopHandlerBlock };
    // first store the current iteration into startIterationAddress
    this->storeCurrentIterationAtBeginningOfHandlerBlock = loopHandlerBuilder.CreateStore(
      IVClone,
      this->startIterationAddress
    );
    // create the vector to represent arguments
    std::vector<Value *> loopHandlerParameters{ 
      hbTask->getContextArg(),
      hbTask->getConstLiveInsArg(),
      loopHandlerBuilder.getInt64(this->loopToLevel[loop]),
      loopHandlerBuilder.getInt64(this->numLevels),
      hbTask->getHBMemoryArg(),
      Constant::getNullValue(loopHandlerFunction->getArg(5)->getType()),  // pointer to slice tasks, change this value later
      Constant::getNullValue(loopHandlerFunction->getArg(6)->getType()),  // pointer to leftover tasks, change this value later
      Constant::getNullValue(loopHandlerFunction->getArg(7)->getType())   // pointer to leftover task selector, change this value later
    };

    this->callToLoopHandler = loopHandlerBuilder.CreateCall(
      loopHandlerFunction,
      ArrayRef<Value *>({
        loopHandlerParameters
      }),
      "promotion_handler_return_code"
    );

    // if (rc > 0) {
    //   break;
    // } else {
    //   goto latch
    // }
    auto cmpInst = loopHandlerBuilder.CreateICmpSGT(
      this->callToLoopHandler,
      ConstantInt::get(loopHandlerBuilder.getInt64Ty(), 0)
    );
    auto condBr = loopHandlerBuilder.CreateCondBr(
      cmpInst,
      loopExitBBClone,
      latchBBClone
    );

    // since now promotion_handler block may jump to loop exit directly
    // need to populate the intermediate value of live-out variable
    // through the promotion_handler block
    for (auto pair : this->liveOutVariableToAccumulatedPrivateCopy) {
      PHINode *liveOutPhiAtLoopExitBlock = cast<PHINode>(pair.second);
      assert(liveOutPhiAtLoopExitBlock->getNumIncomingValues() == 1 && "liveOutPhiAtLoopExitBlock should be untouched at the moment\n");

      PHINode *liveOutPhiInsideLoopPreHeader = cast<PHINode>(liveOutPhiAtLoopExitBlock->getIncomingValue(0));
      assert(liveOutPhiInsideLoopPreHeader->getNumIncomingValues() == 2 && "liveOutPhiInsideLoopPreHeader should have two incoming values\n");

      liveOutPhiAtLoopExitBlock->addIncoming(
        liveOutPhiInsideLoopPreHeader->getIncomingValue(1),
        loopHandlerBlock
      );
    }

    errs() << "task after invoking polling, promotion_handler function and checking return code\n" << *(hbTask->getTaskBody()) << "\n";
  }

  // create a phi node to determine the return code of the loop
  // this return code needs to be created in the exit block of the loop
  // assumption: for now, only dealing with loops that have a single exitBlock
  assert(ls->getLoopExitBasicBlocks().size() == 1 && "loop has multiple exit blocks\n");

  IRBuilder loopExitBBBuilder{ loopExitBBClone };
  loopExitBBBuilder.SetInsertPoint(loopExitBBClone->getTerminator());
  this->returnCodePhiInst = cast<PHINode>(loopExitBBBuilder.CreatePHI(
    loopExitBBBuilder.getInt64Ty(),
    2,
    "returnCode"
  ));
  this->returnCodePhiInst->addIncoming(
    ConstantInt::get(loopExitBBBuilder.getInt64Ty(), 0),
    hbTask->getCloneOfOriginalBasicBlock(ls->getHeader())
  );
  if (this->loopToLevel[ldi] == this->numLevels - 1) {
    this->returnCodePhiInst->addIncoming(
      this->callToLoopHandler,
      loopHandlerBlock
    );
  }

  /*
   * Do the reduction for using the value from the reduction array for kids
   */
  if (loopEnvironment->getNumberOfLiveOuts() > 0) {
    auto loopExitBBCloneTerminator = loopExitBBClone->getTerminator();
    loopExitBBBuilder.SetInsertPoint(loopExitBBCloneTerminator);

    // // following code does the following things
    // if (rc < 1) {
    //   redArrLiveOut0[myIndex * CACHELINE] = r_private;
    // } else if (rc > 1) {
    //   redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE];
    // } else {  // rc == 1 case
    //   redArrLiveOut0[myIndex * CACHELINE] = r_private + redArrLiveOut0Kids[0 * CACHELINE] + redArrLiveOut0Kids[1 * CACHELINE];
    // }

    auto exitBlock = hbTask->getExit();
    // leftover reduction block
    auto leftoverReductionBlock = BasicBlock::Create(this->noelle.getProgramContext(), "leftover_reduction_block", hbTask->getTaskBody());
    IRBuilder<> leftoverReductionBlockBuilder{ leftoverReductionBlock };
    // heartbeat reduction block
    auto hbReductionBlock = BasicBlock::Create(this->noelle.getProgramContext(), "heartbeat_reduction_block", hbTask->getTaskBody());
    IRBuilder<> hbReductionBlockBuilder{ hbReductionBlock };

    // if rc < 1 then jump to the exit block directly.
    auto loopExitBBCmpInst = loopExitBBBuilder.CreateCondBr(
      loopExitBBBuilder.CreateICmpSLT(
        this->returnCodePhiInst,
        ConstantInt::get(loopExitBBBuilder.getInt64Ty(), 1)
      ),
      exitBlock,
      leftoverReductionBlock
    );
    loopExitBBCloneTerminator->eraseFromParent();
    
    // if rc > 1 then jump to the exit block directly from leftover reduction block
    auto leftoverReductionBrInst = leftoverReductionBlockBuilder.CreateCondBr(
      leftoverReductionBlockBuilder.CreateICmpSGT(
        this->returnCodePhiInst,
        ConstantInt::get(loopExitBBBuilder.getInt64Ty(), 1)
      ),
      exitBlock,
      hbReductionBlock
    );
    leftoverReductionBlockBuilder.SetInsertPoint(leftoverReductionBrInst);
    
    // else, assert(rc == 1), jump to the exit block directly from the hbReduction block
    auto hbReductionBrInst = hbReductionBlockBuilder.CreateBr(
      exitBlock
    );
    hbReductionBlockBuilder.SetInsertPoint(hbReductionBrInst);

    // do the final reduction and store into reduction array for all live-outs
    for (auto liveOutEnvID : envUser->getEnvIDsOfLiveOutVars()) {
      auto reductionArrayForKids = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getLiveOutReductionArray(liveOutEnvID);
      errs() << "reduction array for liveOutEnv id: " << liveOutEnvID << " " << *reductionArrayForKids << "\n";

      auto int64 = IntegerType::get(this->noelle.getProgramContext(), 64);
      auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
      auto privateAccumulatedValue = this->liveOutVariableToAccumulatedPrivateCopy[liveOutEnvID];

      // leftover reduction, private_copy + [0]
      auto firstElementPtr = leftoverReductionBlockBuilder.CreateInBoundsGEP(
        reductionArrayForKids->getType()->getPointerElementType(),
        reductionArrayForKids,
        ArrayRef<Value *>({ zeroV, cast<Value>(ConstantInt::get(int64, 0 * this->valuesInCacheLine)) }),
        std::string("reductionArrayForKids_firstElement_addr_").append(std::to_string(liveOutEnvID))
      );
      auto firstElement = leftoverReductionBlockBuilder.CreateLoad(
        privateAccumulatedValue->getType(),
        firstElementPtr,
        std::string("reductionArrayForKids_firstElement_").append(std::to_string(liveOutEnvID))
      );
      Value *firstAddInst = nullptr;
      if (isa<IntegerType>(privateAccumulatedValue->getType())) {
        firstAddInst = leftoverReductionBlockBuilder.CreateAdd(
          privateAccumulatedValue,
          firstElement,
          std::string("leftover_addition_").append(std::to_string(liveOutEnvID))
        );
      } else {
        // TODO: add assertion check for floating point value
        firstAddInst = leftoverReductionBlockBuilder.CreateFAdd(
          privateAccumulatedValue,
          firstElement,
          std::string("leftover_addition_").append(std::to_string(liveOutEnvID))
        );
      }

      // heartbeat reduction, private_copy + [0] + [1]
      auto secondElementPtr = hbReductionBlockBuilder.CreateInBoundsGEP(
        reductionArrayForKids->getType()->getPointerElementType(),
        reductionArrayForKids,
        ArrayRef<Value *>({ zeroV, cast<Value>(ConstantInt::get(int64, 1 * this->valuesInCacheLine)) }),
        std::string("reductionArrayForKids_secondElement_addr_").append(std::to_string(liveOutEnvID))
      );
      auto secondElement = hbReductionBlockBuilder.CreateLoad(
        privateAccumulatedValue->getType(),
        secondElementPtr,
        std::string("reductionArrayForKids_secondElement_").append(std::to_string(liveOutEnvID))
      );
      Value *secondAddInst = nullptr;
      if (isa<IntegerType>(privateAccumulatedValue->getType())) {
        secondAddInst = hbReductionBlockBuilder.CreateAdd(
          firstAddInst,
          secondElement,
          std::string("heartbeat_addition_").append(std::to_string(liveOutEnvID))
        );
      } else {
        // TODO: add assertion check for floating point value
        secondAddInst = hbReductionBlockBuilder.CreateFAdd(
          firstAddInst,
          secondElement,
          std::string("heartbeat_addition_").append(std::to_string(liveOutEnvID))
        );
      }

      // store the final reduction result into upper level's reduction array
      IRBuilder<> exitBlockBuilder{ exitBlock };
      exitBlockBuilder.SetInsertPoint( exitBlock->getFirstNonPHI() );
      // create phi to select from reduction result
      auto reductionResultPHI = exitBlockBuilder.CreatePHI(
        privateAccumulatedValue->getType(),
        3,
        std::string("reductionResult_").append(std::to_string(liveOutEnvID))
      );
      // reductionResultPHI->addIncoming(privateAccumulatedValue, cast<Instruction>(privateAccumulatedValue)->getParent());
      reductionResultPHI->addIncoming(privateAccumulatedValue, loopExitBBClone);
      reductionResultPHI->addIncoming(firstAddInst, leftoverReductionBlock);
      reductionResultPHI->addIncoming(secondAddInst, hbReductionBlock);

      exitBlockBuilder.CreateStore(
        reductionResultPHI,
        envUser->getEnvPtr(liveOutEnvID)
      );
    }
    errs() << "task after doing reduction\n" << *(hbTask->getTaskBody()) << "\n";
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
    this->invokeHeartbeatFunctionAsideOriginalLoop(loop);
  } else {
    errs() << "the loop is not a root loop, need to invoke this loop from it's parent loop\n";
    this->invokeHeartbeatFunctionAsideCallerLoop(loop);
  }

  return true;
}

void HeartbeatTransformation::initializeEnvironmentBuilder(
  LoopContent *LDI, 
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
  this->envBuilder = new HeartbeatLoopEnvironmentBuilder(program->getContext(),
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

void HeartbeatTransformation::initializeLoopEnvironmentUsers() {
  for (auto i = 0; i < this->tasks.size(); ++i) {
    auto task = (HeartbeatTask *)this->tasks[i];
    assert(task != nullptr);
    auto envBuilder = (HeartbeatLoopEnvironmentBuilder *)this->envBuilder;
    assert(envBuilder != nullptr);
    auto envUser = (HeartbeatLoopEnvironmentUser *)this->envBuilder->getUser(i);
    assert(envUser != nullptr);

    auto entryBlock = task->getEntry();
    IRBuilder<> entryBuilder{ entryBlock };

    // Create a bitcast instruction from i8* pointer to context array type
    this->contextBitcastInst = entryBuilder.CreateBitCast(
      task->getContextArg(),
      PointerType::getUnqual(envBuilder->getContextArrayType()),
      "contextArray"
    );

    // Calculate the base index of my context
    auto int64 = IntegerType::get(entryBuilder.getContext(), 64);
    auto myLevelIndexV = cast<Value>(ConstantInt::get(int64, this->loopToLevel[this->ldi] * 8));
    auto zeroV = entryBuilder.getInt64(0);

    // Don't load from the environment pointer if the size is
    // 0 for either single or reducible environment
    if (envBuilder->getSingleEnvironmentSize() > 0) {
      auto liveInEnvPtrGEPInst = entryBuilder.CreateInBoundsGEP(
        envBuilder->getContextArrayType(),
        (Value *)contextBitcastInst,
        ArrayRef<Value *>({
          zeroV,
          ConstantInt::get(entryBuilder.getInt64Ty(), this->loopToLevel[this->ldi] * this->valuesInCacheLine + this->liveInEnvIndex)
        }),
        "liveInEnvPtr"
      );
      auto liveInEnvBitcastInst = entryBuilder.CreateBitCast(
        cast<Value>(liveInEnvPtrGEPInst),
        PointerType::getUnqual(PointerType::getUnqual(envBuilder->getSingleEnvironmentArrayType())),
        "liveInEnvPtrCasted"
      );
      auto liveInEnvLoadInst = entryBuilder.CreateLoad(
        PointerType::getUnqual(envBuilder->getSingleEnvironmentArrayType()),
        (Value *)liveInEnvBitcastInst,
        "liveInEnv"
      );
      envUser->setSingleEnvironmentArray(liveInEnvLoadInst);
    }
    if (envBuilder->getReducibleEnvironmentSize() > 0) {
      auto liveOutEnvPtrGEPInst = entryBuilder.CreateInBoundsGEP(
        envBuilder->getContextArrayType(),
        (Value *)contextBitcastInst,
        ArrayRef<Value *>({
          zeroV,
          ConstantInt::get(entryBuilder.getInt64Ty(), this->loopToLevel[this->ldi] * this->valuesInCacheLine + this->liveOutEnvIndex)
        }),
        "liveOutEnv_addr"
      );
      if (envBuilder->getReducibleEnvironmentSize() == 1) {
        // the address to the live-out environment in the context array will be used
        // to store the reduction array
        envUser->setLiveOutEnvAddress(liveOutEnvPtrGEPInst);
      } else {
        auto liveOutEnvBitcastInst = entryBuilder.CreateBitCast(
          cast<Value>(liveOutEnvPtrGEPInst),
          PointerType::getUnqual(PointerType::getUnqual(envBuilder->getReducibleEnvironmentArrayType())),
          "liveOutEnv_addr_correctly_casted"
        );
        // save this liveOutEnvBitcastInst as it will be used later by the envBuilder to
        // set the liveOutEnv for kids
        envUser->setLiveOutEnvAddress(liveOutEnvBitcastInst);
        auto liveOutEnvLoadInst = entryBuilder.CreateLoad(
          PointerType::getUnqual(envBuilder->getReducibleEnvironmentArrayType()),
          (Value *)liveOutEnvBitcastInst,
          "liveOutEnv"
        );
        envUser->setReducibleEnvironmentArray(liveOutEnvLoadInst);
      }
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

void HeartbeatTransformation::generateCodeToLoadLiveInVariables(LoopContent *LDI, int taskIndex) {
  auto task = this->tasks[taskIndex];
  auto env = LDI->getEnvironment();
  auto envUser = (HeartbeatLoopEnvironmentUser *)this->envBuilder->getUser(taskIndex);

  IRBuilder<> builder{ task->getEntry() };

  // Load live-in variables pointer, then load from the pointer to get the live-in variable
  for (auto envID : envUser->getEnvIDsOfLiveInVars()) {
    auto producer = env->getProducer(envID);
    auto envPointer = envUser->createSingleEnvironmentVariablePointer(builder, envID, producer->getType());
    auto metaString = std::string{ "liveIn_variable_" }.append(std::to_string(envID));
    auto envLoad = builder.CreateLoad(producer->getType(), envPointer, metaString);

    task->addLiveIn(producer, envLoad);
  }

  // Cast constLiveIns to its array type
  auto constLiveIns = builder.CreateBitCast(
    this->hbTask->getConstLiveInsArg(),
    PointerType::getUnqual(ArrayType::get(builder.getInt64Ty(), this->constantLiveInsArgIndexToIndex.size())),
    "constantLiveIns"
  );

  // Load constant live-in variables
  // double *a = (double *)constLiveIns[0];
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
      ArrayType::get(builder.getInt64Ty(), this->constantLiveInsArgIndexToIndex.size()),
      cast<Value>(constLiveIns),
      ArrayRef<Value *>({ zeroV, const_index_V }),
      std::string("constantLiveIn_").append(std::to_string(const_index)).append("_addr")
    );

    // load from casted instruction
    auto constLiveInLoadInst = builder.CreateLoad(
      int64,
      constLiveInGEPInst,
      std::string("constantLiveIn_").append(std::to_string(const_index)).append("_casted")
    );

    // cast to the correct type of constant variable
    Value *constLiveInCastedInst;
    switch (producer->getType()->getTypeID()) {
      case Type::PointerTyID:
        constLiveInCastedInst = builder.CreateIntToPtr(
          constLiveInLoadInst,
          producer->getType(),
          std::string("constantLiveIn_").append(std::to_string(const_index))
        );
        break;
      case Type::IntegerTyID:
        constLiveInCastedInst = builder.CreateSExtOrTrunc(
          constLiveInLoadInst, 
          producer->getType(),
          std::string("constantLiveIn_").append(std::to_string(const_index))
        );
        break;
      case Type::FloatTyID:
      case Type::DoubleTyID:
        constLiveInCastedInst = builder.CreateLoad(
          producer->getType(),
          builder.CreateIntToPtr(
            constLiveInLoadInst,
            PointerType::getUnqual(producer->getType())
          ),
          std::string("constantLiveIn_").append(std::to_string(const_index))
        );
        break;
      default:
        assert(false && "constLiveIn is a type other than ptr, int, float or double\n");
        exit(1);
    }

    task->addLiveIn(producer, constLiveInCastedInst);
  }

  return;
}

void HeartbeatTransformation::setReducableVariablesToBeginAtIdentityValue(LoopContent *LDI, int taskIndex) {
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
    auto isThisLiveOutVariableReducible = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
    if (!isThisLiveOutVariableReducible) {
      continue;
    }

    auto producer = environment->getProducer(envID);
    assert(producer != nullptr);
    assert(isa<PHINode>(producer) && "live-out producer must be a phi node\n");
    auto producerSCC = sccdag->sccOfValue(producer);
    // producerSCC->print(errs());
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

void HeartbeatTransformation::generateCodeToStoreLiveOutVariables(LoopContent *LDI, int taskIndex) {
  auto mm = this->noelle.getMetadataManager();
  auto env = LDI->getEnvironment();
  auto task = this->tasks[taskIndex];
  assert(task != nullptr);

  // only proceed if we have a live-out environment
  if (env->getNumberOfLiveOuts() <= 0) {
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

  auto envUser = (HeartbeatLoopEnvironmentUser *)this->envBuilder->getUser(taskIndex);
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
    auto isReduced = (HeartbeatLoopEnvironmentBuilder *)this->envBuilder->hasVariableBeenReduced(envID);
    if (isReduced) {
      envUser->createReducableEnvPtr(entryBuilder, envID, envType, this->numTaskInstances, ((HeartbeatTask *)task)->getMyIndexArg());
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

void HeartbeatTransformation::allocateNextLevelReducibleEnvironmentInsideTask(LoopContent *LDI, int taskIndex) {
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
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->allocateNextLevelReducibleEnvironmentArray(builder);
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->generateNextLevelReducibleEnvironmentVariables(builder);

  // the pointer to the liveOutEnvironment array (if any)
  // is now pointing at the liveOutEnvironment array for kids,
  // need to reset it before returns
  // Fix: only reset the environment when there's a live-out environment for the loop
  if (((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 0) {
    auto taskExit = task->getExit();
    auto exitReturnInst = taskExit->getTerminator();
    errs() << "terminator of the exit block of task " << *exitReturnInst << "\n";
    IRBuilder exitBuilder { taskExit };
    exitBuilder.SetInsertPoint(exitReturnInst);
    ((HeartbeatLoopEnvironmentUser *)this->envBuilder->getUser(0))->resetReducibleEnvironmentArray(exitBuilder);
  }

  return;
}

BasicBlock * HeartbeatTransformation::performReductionAfterCallingLoopHandler(LoopContent *LDI, int taskIndex,BasicBlock *loopHandlerBB, Instruction *cmpInst, BasicBlock *bottomHalfBB, Value *numOfReducerV) {
  IRBuilder<> builder { loopHandlerBB };

  auto loopStructure = LDI->getLoopStructure();
  auto sccManager = LDI->getSCCManager();
  auto loopSCCDAG = sccManager->getSCCDAG();

  auto environment = LDI->getEnvironment();
  assert(environment != nullptr);

  std::unordered_map<uint32_t, Instruction::BinaryOps> reducibleBinaryOps;
  std::unordered_map<uint32_t, Value *> initialValues;
  for (auto envID : environment->getEnvIDsOfLiveOutVars()) {
    auto isReduced = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
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

  auto afterReductionBB = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->reduceLiveOutVariablesInTask(taskIndex, loopHandlerBB, builder, reducibleBinaryOps, initialValues, cmpInst, bottomHalfBB, numOfReducerV);

  return afterReductionBB;
}

void HeartbeatTransformation::invokeHeartbeatFunctionAsideOriginalLoop (
  LoopContent *LDI
) {

  // allocate constant live-ins array
  auto loopStructure = LDI->getLoopStructure();
  auto loopFunction = loopStructure->getFunction();
  auto firstBB = loopFunction->begin();
  auto firstI = firstBB->begin();
  IRBuilder<> builder{ &*firstI };

  // allocate the constLiveIns array using alloca
  // uint64_t constLiveIns[num_of_const_live_ins];
  auto constantLiveIns = builder.CreateAlloca(
    ArrayType::get(builder.getInt64Ty(), this->constantLiveInsArgIndexToIndex.size()),
    nullptr,
    "constantLiveIns"
  );
  constantLiveIns->setAlignment(Align(64));
  auto constantLiveInsCasted = builder.CreateBitCast(
    constantLiveIns,
    PointerType::getUnqual(builder.getInt64Ty()),
    "constantLiveIns_casted"
  );

  // store the args into constant live-ins array
  // constLiveIns[0] = (uint64_t)a;
  for (auto pair : this->constantLiveInsArgIndexToIndex) {
    auto argIndex = pair.first;
    auto arrayIndex = pair.second;
    auto argV = cast<Value>(&*(loopFunction->arg_begin() + argIndex));

    auto constantLiveInArray_element_ptr = builder.CreateInBoundsGEP(
      constantLiveIns->getType()->getPointerElementType(),
      constantLiveIns,
      ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), 0), ConstantInt::get(builder.getInt64Ty(), arrayIndex) }),
      std::string("constantLiveIn_").append(std::to_string(arrayIndex)).append("_addr")
    );
    Value *constLiveInCastedInst = nullptr;
    Value *allocaFloatOrDoubleInst = nullptr;
    std::string constantLiveInNameString = std::string("constantLiveIn_").append(std::to_string(arrayIndex)).append("_casted");
    // build the cast instruction based on the type of the argV
    switch (argV->getType()->getTypeID()) {
      case Type::PointerTyID:
        constLiveInCastedInst = builder.CreatePtrToInt(
          argV,
          builder.getInt64Ty(),
          constantLiveInNameString
        );
        break;
      case Type::IntegerTyID:
        constLiveInCastedInst = builder.CreateSExtOrTrunc(
          argV,
          builder.getInt64Ty(),
          constantLiveInNameString
        );
        break;
      case Type::FloatTyID:
      case Type::DoubleTyID:
        // when the type is float or double, instead of storing the value we store the address
        // constLiveIns[1] = (uint64_t)&b;
        allocaFloatOrDoubleInst = builder.CreateAlloca(
          argV->getType()
        );
        cast<AllocaInst>(allocaFloatOrDoubleInst)->setAlignment(Align(64));
        builder.CreateStore(
          argV,
          allocaFloatOrDoubleInst
        );
        constLiveInCastedInst = builder.CreatePtrToInt(
          allocaFloatOrDoubleInst,
          builder.getInt64Ty(),
          constantLiveInNameString
        );
        break;
      default:
        assert(false && "constLiveIn is a type other than ptr, int, float or double\n");
        exit(1);
    };
    assert(constLiveInCastedInst != nullptr && "constLiveInCastedInst hasn't been set");
    builder.CreateStore(
      constLiveInCastedInst,
      constantLiveInArray_element_ptr
    );
  }

  // allocate context
  this->contextArrayAlloca = builder.CreateAlloca(
    ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getContextArrayType(),
    nullptr,
    "contextArray"
  );
  this->contextArrayAlloca->setAlignment(Align(64));
  auto contextArrayCasted = builder.CreateBitCast(
    this->contextArrayAlloca,
    PointerType::getUnqual(builder.getInt64Ty()),
    "contextArray_casted"
  );

  /*
   * Create the environment.
   */
  this->allocateEnvironmentArray(LDI);

  // Store the live-in/live-out environment into the context
  auto firstBBTerminator = &*firstBB->getTerminator();
  builder.SetInsertPoint(firstBBTerminator);
  if (((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentSize() > 0) {
    errs() << "we have live-in environments" << "\n";
    // auto liveInEnvCasted = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArrayPointer();
    auto liveInEnv = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArray();
    auto gepInst = builder.CreateInBoundsGEP(
      builder.getInt64Ty(),
      this->contextArrayAlloca,
      ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), 0), ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine + this->liveInEnvIndex) })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveInEnv->getType())
    );
    builder.CreateStore(liveInEnv, gepCasted);
  }
  if (((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 0) {
    errs() << "we have live-out environments" << "\n";
    // auto liveOutEnvCasted = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArrayPointer();
    Value *liveOutEnv;
    if (((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 1) {
      liveOutEnv = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArray();
    } else {
      liveOutEnv = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getOnlyReductionArray();
    }
    auto gepInst = builder.CreateInBoundsGEP(
      this->contextArrayAlloca->getType()->getPointerElementType(),
      this->contextArrayAlloca,
      ArrayRef<Value *>({ ConstantInt::get(builder.getInt64Ty(), 0), ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine + this->liveOutEnvIndex) })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveOutEnv->getType())
    );
    builder.CreateStore(liveOutEnv, gepCasted);
  }
  errs() << "original function after allocating constantLiveIns, context and environment array\n" << *(LDI->getLoopStructure()->getFunction()) << "\n";

  // Since all the variables are coming from the function arguments
  // before calling the level-0 loops,
  // there shouldn't be necessary to liveIns at this stage
  // this->populateLiveInEnvironment(LDI);

  IRBuilder<> loopEntryBuilder(this->entryPointOfParallelizedLoop);

  /*
   * Fetch the first value of the loop-governing IV
   */
  auto IVM = LDI->getInductionVariableManager();
  auto GIV = IVM->getLoopGoverningInductionVariable();
  auto IV = GIV->getInductionVariable();
  auto firstIterationGoverningIVValue = IV->getStartValue();
  errs() << "startIter: " << *firstIterationGoverningIVValue << "\n";
  // store the startIter into context
  loopEntryBuilder.CreateStore(
    firstIterationGoverningIVValue,
    loopEntryBuilder.CreateInBoundsGEP(
      this->contextArrayAlloca->getType()->getPointerElementType(),
      this->contextArrayAlloca,
      ArrayRef<Value *>({
        loopEntryBuilder.getInt64(0),
        loopEntryBuilder.getInt64(this->startIterationIndex)
      }),
      "startIteration_rootLoop_addr"
    )
  );

  /*
   * Fetch the last value of the loop-governing IV
   */
  auto lastIterationGoverningIVValue = GIV->getExitConditionValue();
  errs() << "maxIter: " << *lastIterationGoverningIVValue << "\n";
  // store the maxIter into context
  loopEntryBuilder.CreateStore(
    lastIterationGoverningIVValue,
    loopEntryBuilder.CreateInBoundsGEP(
      this->contextArrayAlloca->getType()->getPointerElementType(),
      this->contextArrayAlloca,
      ArrayRef<Value *>({
        loopEntryBuilder.getInt64(0),
        loopEntryBuilder.getInt64(this->maxIterationIndex)
      }),
      "maxIteration_rootLoop_addr"
    )
  );

  /*
   * Allocate the task_memory_t object and initialize the startingLevel
   */
  auto tmem = loopEntryBuilder.CreateAlloca(
    this->task_memory_t,
    nullptr,
    "tmem"
  );

  /*
   * Invoke the heartbeat_start function
   */
  auto hbResetFunction = this->noelle.getProgram()->getFunction("heartbeat_start");
  assert(hbResetFunction != nullptr);
  errs() << "heartbeat_start function " << *hbResetFunction << "\n";
  this->callToHBResetFunction = loopEntryBuilder.CreateCall(
    hbResetFunction,
    ArrayRef<Value *>({
      tmem,
      loopEntryBuilder.CreateSub(
        lastIterationGoverningIVValue,
        firstIterationGoverningIVValue,
        "num_iterations"
      )
    })
  );

  // invoke the root loop slice task
  std::vector<Value *> loopSliceParameters {
    contextArrayCasted,
    constantLiveInsCasted,
    ConstantInt::get(loopEntryBuilder.getInt64Ty(), 0),
    tmem
  };
  loopEntryBuilder.CreateCall(
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

void HeartbeatTransformation::allocateEnvironmentArray(LoopContent *LDI) {
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
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->allocateSingleEnvironmentArray(builder);
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->allocateReducibleEnvironmentArray(builder);
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->generateEnvVariables(builder, reducerCount);
}

void HeartbeatTransformation::allocateEnvironmentArrayInCallerTask(HeartbeatTask *callerHBTask) {
  auto callerFunction = callerHBTask->getTaskBody();

  auto firstBB = callerFunction->begin();
  auto firstBBTerminator = (*firstBB).getTerminator();

  uint32_t reducerCount = 1;
  IRBuilder<> builder{ firstBBTerminator };
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->allocateSingleEnvironmentArray(builder);
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->allocateReducibleEnvironmentArray(builder);
  ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->generateEnvVariables(builder, reducerCount);
}

void HeartbeatTransformation::populateLiveInEnvironment(LoopContent *LDI) {
  auto mm = this->noelle.getMetadataManager();
  auto env = LDI->getEnvironment();
  IRBuilder<> builder(this->entryPointOfParallelizedLoop);
  for (auto envID : env->getEnvIDsOfLiveInVars()) {

    /*
     * Skip the environment variable if it's not included in the builder
     */
    if (!((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->isIncludedEnvironmentVariable(envID)) {
      continue;
    }

    auto producerOfLiveIn = env->getProducer(envID);
    auto environmentVariable = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getEnvironmentVariable(envID);
    auto newStore = builder.CreateStore(producerOfLiveIn, environmentVariable);
    mm->addMetadata(newStore, "heartbeat.environment_variable.live_in.store_pointer", std::to_string(envID));
  }

  return;
}

BasicBlock * HeartbeatTransformation::performReductionWithInitialValueToAllReducibleLiveOutVariables(LoopContent *LDI) {
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

    auto isReduced = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
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

  auto reductionBlock = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->reduceLiveOutVariablesWithInitialValue(
      this->entryPointOfParallelizedLoop,
      builder,
      reducableBinaryOps,
      initialValues);

  /*
   * If reduction occurred, then all environment loads to propagate live outs
   * need to be inserted after the reduction loop
   */
  IRBuilder<> *afterReductionBuilder;
  if (reductionBlock->getTerminator()) {
    afterReductionBuilder->SetInsertPoint(reductionBlock->getTerminator());
  } else {
    afterReductionBuilder = new IRBuilder<>(reductionBlock);
  }

  for (int envID : environment->getEnvIDsOfLiveOutVars()) {
    auto prod = environment->getProducer(envID);

    /*
     * If the environment variable isn't reduced, it is held in allocated memory
     * that needs to be loaded from in order to retrieve the value.
     */
    auto isReduced = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->hasVariableBeenReduced(envID);
    Value *envVar;
    if (isReduced) {
      envVar = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getAccumulatedReducedEnvironmentVariable(envID);
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

  return reductionBlock;
}

void HeartbeatTransformation::invokeHeartbeatFunctionAsideCallerLoop (
  LoopContent *LDI
) {

  auto calleeFunction = LDI->getLoopStructure()->getFunction();

  auto callerLoop = this->loopToCallerLoop[LDI];
  errs() << "caller function is " << callerLoop->getLoopStructure()->getFunction()->getName() << "\n";

  auto callerHBTransformation = this->loopToHeartbeatTransformation[callerLoop];
  assert(callerHBTransformation != nullptr && "callerHBTransformation hasn't been generated\n");
  auto callerHBTask = callerHBTransformation->getHeartbeatTask();
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
  errs() << "callerTask after allocating environment for children\n" << *callerHBTask->getTaskBody() << "\n";

  auto firstBB = &*(callerHBTask->getTaskBody()->begin());
  IRBuilder<> builder { firstBB->getTerminator() };
  auto zeroV = builder.getInt64(0);

  // Store the live-in/live-out environment into the context
  if (((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentSize() > 0) {
    errs() << "we have live-in environments" << "\n";
    // auto liveInEnvCasted = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArrayPointer();
    auto liveInEnv = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArray();
    auto gepInst = builder.CreateInBoundsGEP(
      this->loopToHeartbeatTransformation[callerLoop]->getContextBitCastInst()->getType()->getPointerElementType(),
      this->loopToHeartbeatTransformation[callerLoop]->getContextBitCastInst(),
      ArrayRef<Value *>({
        zeroV,
        ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine + callerHBTransformation->liveInEnvIndex)
      })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveInEnv->getType())
    );
    builder.CreateStore(liveInEnv, gepCasted);
  }
  if (((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 0) {
    errs() << "we have live-out environments" << "\n";
    // auto liveOutEnvCasted = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArrayPointer();
    Value *liveOutEnv;
    if (((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentSize() > 1) {
      liveOutEnv = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleEnvironmentArray();
    } else {
      liveOutEnv = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getOnlyReductionArray();
    }
    auto gepInst = builder.CreateInBoundsGEP(
      this->loopToHeartbeatTransformation[callerLoop]->getContextBitCastInst()->getType()->getPointerElementType(),
      this->loopToHeartbeatTransformation[callerLoop]->getContextBitCastInst(),
      ArrayRef<Value *>({
        zeroV,
        ConstantInt::get(builder.getInt64Ty(), this->loopToLevel[this->ldi] * valuesInCacheLine + callerHBTransformation->liveOutEnvIndex)
      })
    );
    auto gepCasted = builder.CreateBitCast(
      gepInst,
      PointerType::getUnqual(liveOutEnv->getType())
    );
    builder.CreateStore(liveOutEnv, gepCasted);
  }
  errs() << "callerTask after storing live-in/out environment into context\n" << *callerHBTask->getTaskBody() << "\n";

  // this->populateLiveInEnvironment(LDI);
  // corretly let the callerHBTask to prepare the live-in environment
  auto env = LDI->getEnvironment();
  for (auto liveInID : env->getEnvIDsOfLiveInVars()) {
    if (!((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->isIncludedEnvironmentVariable(liveInID)) {
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
    //
    // if the callerParameter is an instruction, then there'll be a clone in original caller's loop body
    // else the callerParameter is an argument, then this parameter is a liveIn to the caller's hbTask,
    // instead of looking at original caller's loop body, we shall look at caller's hbTask to find the liveIn value
    Value *callerParameterClone = nullptr;
    if (isa<Instruction>(callerParameter)) {
      errs() << "the parameter origins from an instruction in the caller's original loop body\n";
      callerParameterClone = this->loopToHeartbeatTransformation[callerLoop]->getHeartbeatTask()->getCloneOfOriginalInstruction(cast<Instruction>(callerParameter));
    } else {
      assert(isa<Argument>(callerParameter));
      errs() << "the parameter origins from an argument in caller's function signature\n";
      callerParameterClone = this->loopToHeartbeatTransformation[callerLoop]->getHeartbeatTask()->getCloneOfOriginalLiveIn(cast<Argument>(callerParameter));
    }
    errs() << "clone of the liveIn in caller's hbTask: " << *callerParameterClone << "\n";

    // we finally found the correct liveIn in caller's hbTask, now let caller's hbTask to store this liveIn's value in the liveInEnv
    auto liveInEnvArray = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getSingleEnvironmentArray();
    errs() << "liveInEnvArray: " << *liveInEnvArray << "\n";
    auto liveInIndex = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getIndexOLiveIn(liveInID);
    errs() << "liveInIndex: " << liveInIndex << "\n";
    auto liveInLocation = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getLocationOfSingleVariable(liveInID);
    errs() << "liveInLocation: " << *liveInLocation << "\n";

    liveInEnvBuilder.CreateStore(callerParameterClone, liveInLocation);
  }
  errs() << "callerTask after storing live-in variables\n" << *callerHBTask->getTaskBody() << "\n";

  // store caller's current iteration into context before calling the nested loop
  liveInEnvBuilder.CreateStore(
    callerHBTransformation->currentIteration,
    callerHBTransformation->startIterationAddress
  );

  // set the startIteration and maxIteration for the nested loop
  // algorithm, inspect the original callee function and check start/max iteration,
  // if is a constantInt, use the constant int
  // else must be a function argument (the assumption is start/max iteration can be simply retrieved and not formed in any complex computation)
  // startIteration
  auto IVM = LDI->getInductionVariableManager();
  auto GIV = IVM->getLoopGoverningInductionVariable();
  assert(GIV != nullptr);
  assert(GIV->isSCCContainingIVWellFormed());
  auto IV = GIV->getValueToCompareAgainstExitConditionValue();
  assert(isa<PHINode>(IV) && "the induction variable isn't a phi node\n");
  errs() << "IV: " << *IV << "\n";
  auto startIterationValue = cast<PHINode>(IV)->getIncomingValue(0);
  errs() << "startIteration: " << *startIterationValue << "\n";
  assert(isa<ConstantInt>(startIterationValue) || isa<Argument>(startIterationValue) && "the startIteration value in the callee hbTask isn't either a constant or function argument");
  auto startIterationNextLevelAddress = liveInEnvBuilder.CreateInBoundsGEP(
    ((HeartbeatLoopEnvironmentBuilder *)callerHBTransformation->envBuilder)->getContextArrayType(),
    callerHBTransformation->contextBitcastInst,
    ArrayRef<Value *>({
      liveInEnvBuilder.getInt64(0),
      liveInEnvBuilder.getInt64((callerHBTask->getLevel() + 1) * this->valuesInCacheLine + callerHBTransformation->startIterationIndex)
    }),
    "startIteration_nextLevel_addr"
  );
  if (isa<ConstantInt>(startIterationValue)) {
    liveInEnvBuilder.CreateStore(
      ConstantInt::get(startIterationValue->getType(), cast<ConstantInt>(startIterationValue)->getSExtValue()),
      startIterationNextLevelAddress
    );
  } else if (isa<Argument>(startIterationValue)) {  // it's an argument
    // algorithm, we know the start iteration is passed through function argument
    auto arg_index = cast<Argument>(startIterationValue)->getArgNo();
    liveInEnvBuilder.CreateStore(
      callToLoopInCallerInst->getArgOperand(arg_index),
      startIterationNextLevelAddress
    );
  } else if (isa<CallInst>(startIterationValue)) {
    auto callInst = dyn_cast<CallInst>(startIterationValue);
    auto callee = callInst->getCalledFunction();
    // llvm inserts an intrinsic call llvm.smax.[type], such as llvm.smax.int64,
    // this might frees vectorization later but it causes a problem.
    if (callee->getName().startswith("llvm.smax")) {
      if (isa<Argument>(callInst->getArgOperand(0))) {
        auto arg_index = cast<Argument>(callInst->getArgOperand(0))->getArgNo();
        liveInEnvBuilder.CreateStore(
          callToLoopInCallerInst->getArgOperand(arg_index),
          startIterationNextLevelAddress
        );
      } else {
        exit(1);
      }
    } else {
      exit(1);
    }
  } else {
    exit(1);
  }

  // maxIteration
  auto maxIterationValue = GIV->getExitConditionValue();
  errs() << "maxIteration: " << *maxIterationValue << "\n";
  // assert(isa<ConstantInt>(maxIterationValue) || isa<Argument>(maxIterationValue) && "the maxIteration value in the callee hbTask isn't either a constant or function argument");
  auto maxIterationNextLevelAddress = liveInEnvBuilder.CreateInBoundsGEP(
    ((HeartbeatLoopEnvironmentBuilder *)callerHBTransformation->envBuilder)->getContextArrayType(),
    callerHBTransformation->contextBitcastInst,
    ArrayRef<Value *>({
      liveInEnvBuilder.getInt64(0),
      liveInEnvBuilder.getInt64((callerHBTask->getLevel() + 1) * this->valuesInCacheLine + callerHBTransformation->maxIterationIndex)
    }),
    "maxIteration_nextLevel_addr"
  );
  if (isa<ConstantInt>(maxIterationValue)) {
    liveInEnvBuilder.CreateStore(
      ConstantInt::get(maxIterationValue->getType(), cast<ConstantInt>(maxIterationValue)->getSExtValue()),
      maxIterationNextLevelAddress
    );
  } else if (isa<Argument>(maxIterationValue)) {
    auto arg_index = cast<Argument>(maxIterationValue)->getArgNo();
    liveInEnvBuilder.CreateStore(
      callToLoopInCallerInst->getArgOperand(arg_index),
      maxIterationNextLevelAddress
    );
  } else if (isa<CallInst>(maxIterationValue)) {
    auto callInst = dyn_cast<CallInst>(maxIterationValue);
    auto callee = callInst->getCalledFunction();
    // llvm inserts an intrinsic call llvm.smax.[type], such as llvm.smax.int64,
    // this might frees vectorization later but it causes a problem.
    if (callee->getName().startswith("llvm.smax")) {
      if (isa<Argument>(callInst->getArgOperand(0))) {
        auto arg_index = cast<Argument>(callInst->getArgOperand(0))->getArgNo();
        liveInEnvBuilder.CreateStore(
          callToLoopInCallerInst->getArgOperand(arg_index),
          maxIterationNextLevelAddress
        );
      } else {
        exit(1);
      }
    } else {
      exit(1);
    }
  } else {
    exit(1);
  }

  // okay, preparing the environment is done, now replace the call to original code in the caller hbTask
  // to the callee's hbTask
  std::vector<Value *> parametersVec {
    callerHBTask->getContextArg(),
    callerHBTask->getConstLiveInsArg(),
    liveInEnvBuilder.getInt64(0),
    callerHBTask->getHBMemoryArg()
  };
  auto calleeHBTaskCallInst = liveInEnvBuilder.CreateCall(
    this->hbTask->getTaskBody(),
    ArrayRef<Value *>({
      parametersVec
    }),
    "nested_loop_return_code"
  );

  if (env->getNumberOfLiveOuts() > 0) {
    // if the original callee function has a return value (assumption, can only have at most 1 return value as live-out)
    // we then need to load this value from the reduction array and replace all uses of the original call
    assert(env->getNumberOfLiveOuts() == 1 && " invoking a callee function that has multiple live-outs!\n");
    auto liveOutID = *(env->getEnvIDsOfLiveOutVars().begin());
    errs() << "liveOutID: " << liveOutID << "\n";
    auto liveOutIndex = ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getIndexOfLiveOut(liveOutID);
    errs() << "liveOutIndex: " << liveOutIndex << "\n";
    // now we have the index of the liveOutIndex, now load the result from the reductionArray allocated for the nest loop
    auto liveOutResult = liveInEnvBuilder.CreateLoad(
      callToLoopInCallerInst->getType(),
      ((HeartbeatLoopEnvironmentBuilder *)this->envBuilder)->getReducibleVariableOfIndexGivenEnvIndex(liveOutIndex, 0)
    );

    callToLoopInCallerInst->replaceAllUsesWith(liveOutResult);
  }
  callToLoopInCallerInst->eraseFromParent();

  errs() << "callerTask after calling calleeHBTask\n" << *callerHBTask->getTaskBody() << "\n";


  // 04/06 by Yian
  // No longer need the concept of modifying exit condition because
  // 1. if there's no tail work left to do, maxIter = startIter + 1 is equal to break the loop directly
  // 2. if there's tail work to do and it doesn't use the new exit condition value, same as above
  // 3. if there's tail work to do and it uses the previous exit condition value, this should be captured
  //    by the loop environment analysis and captured as a live-in variable. In other words, the exit
  //    condition value itself has been cloned
  // 4. if there's tail work to do and it uses the previous exit condition value to indicate the exit 
  //    condition of a child loop, then the parent loop will set the exit condition for this children
  //    loop and the exit condition at this value should be taken as a live-in variable
  // Conclusion: the modify-exit-condition block is unnecessary, simply putting the if (rc > 0) in the
  // end of the loop should be find
  // The following code also assume there's only 1 heartbeat loop at each level
  liveInEnvBuilder.SetInsertPoint(calleeHBTaskCallInst->getNextNode());
  auto cmpInst = liveInEnvBuilder.CreateICmpSGT(
    calleeHBTaskCallInst,
    liveInEnvBuilder.getInt64(0)
  );

  // 07/30 by Yian
  // since we don't do the insertion for any middle loops, therefore
  // there's no need to update the branch to the polling block
  // // find the predecessor to the polling block, in this case it is the last loop body block
  // auto pollingBlock = this->loopToHeartbeatTransformation[callerLoop]->getPollingBlock();
  // auto lastLoopBodyBlock = pollingBlock->getSinglePredecessor();
  // auto pollingBrInst = lastLoopBodyBlock->getTerminator();
  // get the exit block of the caller task
  // this can be fetched through the returnCode instruction
  // auto returnCodePhiInst = this->loopToHeartbeatTransformation[callerLoop]->getReturnCodePhiInst();
  // auto loopExitBlock = returnCodePhiInst->getParent();
  // liveInEnvBuilder.SetInsertPoint(pollingBrInst);
  // liveInEnvBuilder.CreateCondBr(
  //   cmpInst,
  //   loopExitBlock,
  //   pollingBlock
  // );
  // cast<PHINode>(returnCodePhiInst)->addIncoming(calleeHBTaskCallInst, lastLoopBodyBlock);
  // pollingBrInst->eraseFromParent();

  // fetch original terminator of the last body of the loop
  auto lastLoopBodyBlock = calleeHBTaskCallInst->getParent();
  auto bodyTerminator = lastLoopBodyBlock->getTerminator();
  auto latchBlock = dyn_cast<BranchInst>(bodyTerminator)->getSuccessor(0);

  // get the exit block of the caller task
  auto returnCodePhiInst = this->loopToHeartbeatTransformation[callerLoop]->getReturnCodePhiInst();
  auto loopExitBlock = returnCodePhiInst->getParent();
  liveInEnvBuilder.SetInsertPoint(bodyTerminator);
  liveInEnvBuilder.CreateCondBr(
    cmpInst,
    loopExitBlock,
    latchBlock
  );
  cast<PHINode>(returnCodePhiInst)->addIncoming(calleeHBTaskCallInst, lastLoopBodyBlock);
  bodyTerminator->eraseFromParent();

  // now need to update all phi node at the exit block that deals with live-out variable
  // the incoming value is the same from the promotion_handler block
  for (auto pair : this->loopToHeartbeatTransformation[callerLoop]->liveOutVariableToAccumulatedPrivateCopy) {
    PHINode *liveOutPhiAtLoopExitBlock = cast<PHINode>(pair.second);
    assert(liveOutPhiAtLoopExitBlock->getNumIncomingValues() == 2 && "liveOutPhiAtLoopExitBlock should have another incoming value from the promotion_handler block at the moment\n");
  
    auto incomingValueFromLoopHandlerBlock = liveOutPhiAtLoopExitBlock->getIncomingValueForBlock(loopHandlerBlock);
    liveOutPhiAtLoopExitBlock->addIncoming(incomingValueFromLoopHandlerBlock, lastLoopBodyBlock);
  }
  errs() << "callerTask after breaking from the last loop body block\n" << *callerHBTask->getTaskBody() << "\n";

  return ;
}
