#include "HeartbeatTransformation.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

static cl::opt<int> HBTVerbose(
    "heartbeat-transformation-verbose",
    cl::ZeroOrMore,
    cl::Hidden,
    cl::desc("Heartbeat Transformation verbose output (0: disabled, 1: enabled)"));

HeartbeatTransformation::HeartbeatTransformation(
  Noelle *noelle,
  HeartbeatRuntimeManager *hbrm,
  LoopNestAnalysis *lna,
  uint64_t nestID,
  LoopContent *lc,
  std::unordered_map<LoopContent *, std::unordered_set<Value *>> &loopToSkippedLiveIns,
  std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> &loopToConstantLiveIns,
  std::unordered_map<int, int> &constantLiveInsArgIndexToIndex
) : DOALL(*noelle),
    outputPrefix("Heartbeat Transformation: "),
    noelle(noelle),
    hbrm(hbrm),
    lna(lna),
    nestID(nestID),
    lc(lc),
    loopToSkippedLiveIns(loopToSkippedLiveIns),
    loopToConstantLiveIns(loopToConstantLiveIns),
    constantLiveInsArgIndexToIndex(constantLiveInsArgIndexToIndex) {
  /*
   * Fetch command line options.
   */
  this->verbose = static_cast<HBTVerbosity>(HBTVerbose.getValue());

  /*
   * Set the index values used per LST context.
   */
  this->valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);
  this->startIteartionIndex = 0;
  this->endIterationIndex = 1;
  this->liveInEnvIndex = 2;
  this->liveOutEnvIndex = 3;

  /*
   * Create loop-slice task signature.
   */
  auto tm = this->noelle->getTypesManager();
  std::vector<Type *> loopSliceTaskSignatureTypes {
    PointerType::getUnqual(hbrm->getTaskMemoryStructType()),  // *tmem
    tm->getIntegerType(64),                                   // task_index
    PointerType::getUnqual(tm->getIntegerType(64)),           // *cxts
    PointerType::getUnqual(tm->getIntegerType(64))            // *invariants
  };
  this->loopSliceTaskSignature = FunctionType::get(
    tm->getIntegerType(64),
    ArrayRef<Type *>(loopSliceTaskSignatureTypes),
    false
  );

  if (this->verbose > HBTVerbosity::Disabled) {
    errs() << this->outputPrefix << "Create loop-slice task function signature\n";
    errs() << this->outputPrefix << "  \"" << *this->loopSliceTaskSignature << "\"\n";
  }

  return;
}

bool HeartbeatTransformation::apply(LoopContent *lc, Heuristics *h) {
  /*
   * Clone and generate a loop-slice task from the original loop.
   */
  this->createLoopSliceTaskFromOriginalLoop(lc);

  /*
   * Generate loop closure.
   */
  this->generateLoopClosure(lc);

  /*
   * Parameterize loop iteratons.
   */
  this->parameterizeLoopIterations(lc);

  /*
   * Loop chunking transformation,
   * only if a loop is a leaf loop
   */
  // if (this->lna->isLeafLoop(lc)) {
  //   this->chunkLoopIterations(lc);
  // }

  /*
   * Insert promotion handler,
   * only if a loop is a leaf loop.
   */
  if (this->lna->isLeafLoop(lc)) {
    this->insertPromotionHandler(lc);
  }

  /*
   * Link the loop-slice task with the parent loop-slice task or the original caller function
   * 1) If link with parent loop-slice task, lst-contexts are available through the function arguments.
   * 2) If link with the original caller function (for root loop only), lst-contexts needs to be stack allocated.
   */

  return true;
}

void HeartbeatTransformation::createLoopSliceTaskFromOriginalLoop(LoopContent *lc) {
  /*
   * Get program from NOELLE
   */
  auto program = this->noelle->getProgram();

  /*
   * Generate an empty loop-slice task for heartbeat execution.
   */
  this->lsTask = new LoopSliceTask(
    program,
    this->loopSliceTaskSignature,
    std::string("LST_").append(this->lna->getLoopIDString(lc))
  );

  /*
   * Initialize the loop-slice task with an entry and exit basic blocks.
   */
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(lc, { this->lsTask });

  /*
   * NOELLE's ParallelizationTechnique abstraction creates two basic blocks
   * in the original loop function.
   * For heartbeat, these two basic blocks are not needed.
   */
  this->getParLoopEntryPoint()->eraseFromParent();
  this->getParLoopExitPoint()->eraseFromParent();

  /*
   * Clone the loop into the loop-slice task.
   */
  this->cloneSequentialLoop(lc, 0);

  if (this->verbose > HBTVerbosity::Disabled) {
    errs() << this->outputPrefix << "loop-slice task after cloning from original loop\n";
    errs() << *this->lsTask->getTaskBody() << "\n";
  }

  return;
}

void HeartbeatTransformation::parameterizeLoopIterations(LoopContent *lc) {
  /*
   * Create an IRBuilder that insert before the terminator instruction
   * of the entry basic block of the loop-slice task.
   */
  auto lsTaskEntryBlock = this->lsTask->getEntry();
  IRBuilder<> iterationBuilder{ lsTaskEntryBlock };
  iterationBuilder.SetInsertPoint(lsTaskEntryBlock->getTerminator());

  /*
   * Get the loop-governing induction variable.
   */
  auto ivm = lc->getInductionVariableManager();
  auto giv = ivm->getLoopGoverningInductionVariable();
  auto iv = giv->getInductionVariable()->getLoopEntryPHI();
  auto ivClone = cast<PHINode>(this->lsTask->getCloneOfOriginalInstruction(iv));
  this->inductionVariable = ivClone;

  /*
   * Adjust the start iteartion of the induction variable by
   * loading the value stored by the caller.
   */
  this->startIterationPointer = iterationBuilder.CreateInBoundsGEP(
    iterationBuilder.getInt64Ty(),
    this->lsTask->getLSTContextPointerArg(),
    iterationBuilder.getInt64(this->lna->getLoopLevel(lc) * this->valuesInCacheLine + this->startIteartionIndex),
    "start_iteration_pointer"
  );
  this->startIteration = iterationBuilder.CreateLoad(
    iterationBuilder.getInt64Ty(),
    this->startIterationPointer,
    "start_iteration"
  );
  this->inductionVariable->setIncomingValueForBlock(lsTaskEntryBlock, this->startIteration);

  /*
   * Adjust the end iteration (a.k.a. exit condition) by
   * loading the value stored by the caller.
   */
  this->endIterationPointer = iterationBuilder.CreateInBoundsGEP(
    iterationBuilder.getInt64Ty(),
    this->lsTask->getLSTContextPointerArg(),
    iterationBuilder.getInt64(this->lna->getLoopLevel(lc) * this->valuesInCacheLine + this->endIterationIndex),
    "end_iteration_pointer"
  );
  this->endIteration = iterationBuilder.CreateLoad(
    iterationBuilder.getInt64Ty(),
    this->endIterationPointer,
    "end_iteration"
  );

  /*
   * Replace the original exit condition value to
   * use the loaded end iteration value.
   */
  auto exitCmpInst = giv->getHeaderCompareInstructionToComputeExitCondition();
  this->exitCmpInstClone = dyn_cast<CmpInst>(this->lsTask->getCloneOfOriginalInstruction(exitCmpInst));
  auto exitConditionValue = giv->getExitConditionValue();
  auto firstOperand = exitCmpInst->getOperand(0);
  if (firstOperand == exitConditionValue) {
    this->exitCmpInstClone->setOperand(0, this->endIteration);
  } else {
    this->exitCmpInstClone->setOperand(1, this->endIteration);
  }

  if (this->verbose > HBTVerbosity::Disabled) {
    errs() << this->outputPrefix << "loop-slice task after parameterizing loop iterations\n";
    errs() << *this->lsTask->getTaskBody() << "\n";
  }

  return;
}

void HeartbeatTransformation::insertPromotionHandler(LoopContent *lc) {
  auto program = this->noelle->getProgram();
  auto ls = lc->getLoopStructure();
  auto loopLatches = ls->getLatches();
  assert(loopLatches.size() == 1 && "Original loop have multiple loop latches");
  auto loopLatch = *(loopLatches.begin());
  auto loopExits = ls->getLoopExitBasicBlocks();
  assert(loopExits.size() == 1 && "Original loop has multiple loop exits");
  auto loopExit = *(loopExits.begin());

  /*
   * Find all cloned basic blocks.
   */
  auto loopLatchClone = this->lsTask->getCloneOfOriginalBasicBlock(loopLatch);
  auto loopLastBodyClone = loopLatchClone->getUniquePredecessor();
  auto loopExitClone = this->lsTask->getCloneOfOriginalBasicBlock(loopExit);

  /*
   * Create all necessary basic blocks for parallel promotion.
   */
  auto heartbeatPollingBlock = BasicBlock::Create(
    program->getContext(),
    "heartbeat_polling_block",
    this->lsTask->getTaskBody()
  );
  auto promotionHandlerBlock = BasicBlock::Create(
    program->getContext(),
    "promotion_handler_block",
    this->lsTask->getTaskBody()
  );

  /*
   * Initialzie all IRBuilder.
   */
  IRBuilder<> heartbeatPollingBlockBuilder{ heartbeatPollingBlock };
  IRBuilder<> promotionHandlerBlockBuilder{ promotionHandlerBlock };

  /*
   * Update the branch instruction which points to the loop latch
   * to the heartbeat polling block.
   */
  auto loopLastBodyTerminatorClone = loopLastBodyClone->getTerminator();
  dyn_cast<BranchInst>(loopLastBodyTerminatorClone)->replaceSuccessorWith(loopLatchClone, heartbeatPollingBlock);

  /*
   * Invoke heartbeat polling function in the heartbeat polling block.
   */
  auto heartbeatPollingFunction = program->getFunction("heartbeat_polling");
  assert(heartbeatPollingFunction != nullptr && "heartbeat_polling function not found");
  auto hasHeartbeatArrived = heartbeatPollingBlockBuilder.CreateCall(
    heartbeatPollingFunction,
    ArrayRef<Value *>({
      this->lsTask->getTaskMemoryPointerArg()
    }),
    "has_heartbeat_arrived"
  );
  // TODO: figure out the llvm::expect function semantics.
  auto llvmExpectFunction = Intrinsic::getDeclaration(
    program,
    Intrinsic::expect,
    ArrayRef<Type *>({
      heartbeatPollingBlockBuilder.getInt1Ty()
    })
  );
  auto llvmExpect = heartbeatPollingBlockBuilder.CreateCall(
    llvmExpectFunction,
    ArrayRef<Value *>({
      hasHeartbeatArrived,
      heartbeatPollingBlockBuilder.getInt1(0)
    })
  );

  /*
   * Create a conditional branch in the heartbeat polling block,
   * which the first successor is the promotion handler block,
   * and the second successor is the loop latch.
   */
  heartbeatPollingBlockBuilder.CreateCondBr(
    llvmExpect,
    promotionHandlerBlock,
    loopLatchClone
  );

  /*
   * Store the current iteration into the start iteration pointer.
   */
  // TODO: if doing loop chunking before, we need to subtract 1 from the induction variable.
  promotionHandlerBlockBuilder.CreateStore(
    this->inductionVariable,
    this->startIterationPointer
  );

  /*
   * Create a call to promotion handler.
   */
  auto promotionHandlerFunction = program->getFunction("promotion_handler");
  assert(promotionHandlerFunction != nullptr && "promotion_handler function not found");
  this->promotionHandlerCallInst = promotionHandlerBlockBuilder.CreateCall(
    promotionHandlerFunction,
    ArrayRef<Value *>({
      this->lsTask->getTaskMemoryPointerArg(),
      this->lsTask->getLSTContextPointerArg(),
      this->lsTask->getInvariantsPointerArg(),
      
    }),
    "promotion_handler_return_code"
  );

  if (this->verbose > HBTVerbosity::Disabled) {
    errs() << this->outputPrefix << "loop-slice task after inserting promotion handler\n";
    errs() << *this->lsTask->getTaskBody() << "\n";
  }

  return;
}

} // namespace arcana::heartbeat
