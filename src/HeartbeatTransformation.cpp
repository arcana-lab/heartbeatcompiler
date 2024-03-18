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
  if (this->lna->isLeafLoop(lc)) {
    this->chunkLoopIterations(lc);
  }

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
    "startIterationPointer"
  );
  this->startIteration = iterationBuilder.CreateLoad(
    iterationBuilder.getInt64Ty(),
    this->startIterationPointer,
    "startIteration"
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
    "endIterationPointer"
  );
  this->endIteration = iterationBuilder.CreateLoad(
    iterationBuilder.getInt64Ty(),
    this->endIterationPointer,
    "endIteration"
  );

  /*
   * Replace the original exit condition value to
   * use the loaded end iteration value.
   */
  auto exitCmpInst = giv->getHeaderCompareInstructionToComputeExitCondition();
  auto exitCmpInstClone = this->lsTask->getCloneOfOriginalInstruction(exitCmpInst);
  auto exitConditionValue = giv->getExitConditionValue();
  auto firstOperand = exitCmpInst->getOperand(0);
  if (firstOperand == exitConditionValue) {
    exitCmpInstClone->setOperand(0, this->endIteration);
  } else {
    exitCmpInstClone->setOperand(1, this->endIteration);
  }

  if (this->verbose > HBTVerbosity::Disabled) {
    errs() << this->outputPrefix << "loop-slice task after parameterizing loop iterations\n";
    errs() << *this->lsTask->getTaskBody() << "\n";
  }

  return;
}

void HeartbeatTransformation::insertPromotionHandler(LoopContent *lc) {
  return;
}

} // namespace arcana::heartbeat
