#include "HeartbeatTransformation.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

void HeartbeatTransformation::chunkLoopIterations(LoopContent *lc) {
  auto program = this->noelle->getProgram();
  auto ls = lc->getLoopStructure();
  auto loopHeader = ls->getHeader();
  auto loopFirstBody = loopHeader->getTerminator()->getSuccessor(0);
  auto loopLatches = ls->getLatches();
  assert(loopLatches.size() == 1 && "Original loop have multiple loop latches");
  auto loopLatch = *(loopLatches.begin());
  auto loopExits = ls->getLoopExitBasicBlocks();
  assert(loopExits.size() == 1 && "Original loop has multiple loop exits");
  auto loopExit = *(loopExits.begin());

  /*
   * Find all cloned basic blocks.
   */
  auto loopHeaderClone = this->lsTask->getCloneOfOriginalBasicBlock(loopHeader);
  auto loopFirstBodyClone = this->lsTask->getCloneOfOriginalBasicBlock(loopFirstBody);
  auto loopLatchClone = this->lsTask->getCloneOfOriginalBasicBlock(loopLatch);
  auto loopLastBodyClone = loopLatchClone->getUniquePredecessor();
  auto loopExitClone = this->lsTask->getCloneOfOriginalBasicBlock(loopExit);

  /*
   * Create all necessary basic blocks for loop chunking execution.
   */
  auto chunkLoopHeader = BasicBlock::Create(
    program->getContext(),
    "chunk_loop_header",
    this->lsTask->getTaskBody()
  );
  auto chunkLoopLatch = BasicBlock::Create(
    program->getContext(),
    "chunk_loop_latch",
    this->lsTask->getTaskBody()
  );
  auto chunkLoopExit = BasicBlock::Create(
    program->getContext(),
    "chunk_loop_exit",
    this->lsTask->getTaskBody()
  );

  /*
   * Initialzie all IRBuilder.
   */
  IRBuilder<> chunkLoopPreheaderBuilder{ loopHeaderClone };
  chunkLoopPreheaderBuilder.SetInsertPoint(loopHeaderClone->getFirstNonPHI());
  IRBuilder<> chunkLoopHeaderBuilder{ chunkLoopHeader };
  IRBuilder<> chunkLoopLatchBuilder{ chunkLoopLatch };
  IRBuilder<> chunkLoopExitBuilder{ chunkLoopExit };

  /*
   * Load the chunk_size at the chunk loop preheader via
   * a call to the get_chunk_size function.
   */
  auto getChunkSizeFunction = program->getFunction("get_chunk_size");
  assert(getChunkSizeFunction != nullptr && "get_chunk_size function not found");
  auto chunkSize = chunkLoopPreheaderBuilder.CreateCall(
    getChunkSizeFunction,
    ArrayRef<Value *>({
      this->lsTask->getTaskMemoryPointerArg()
    }),
    "chunk_size"
  );

  /*
   * Update the stride of the original loop with chunk size
   */
  auto inductionVariableUpdateInst = this->inductionVariable->getIncomingValue(1);
  dyn_cast<Instruction>(inductionVariableUpdateInst)->setOperand(1, chunkSize);

  /*
   * Ensure the compare instruction that uses to determine
   * whether keep running the outer loop is icmp slt but not icmp ne
   */
  this->exitCmpInstClone->setPredicate(CmpInst::Predicate::ICMP_SLE);

  /*
   * Calculate the exit condition value (high) used by the chunk loop.
   * high = min(inductionVariable + chunk_size, endIteration),
   * where startIteration and endIteration are from the original loop.
   */
  auto inductionVariablePlusChunkSize = chunkLoopPreheaderBuilder.CreateAdd(
    this->inductionVariable,
    chunkSize,
    "induction_variable_plus_chunk_size"
  );
  auto llvmUMINFunction = Intrinsic::getDeclaration(
    program,
    Intrinsic::umin,
    ArrayRef<Type *>({
      chunkLoopPreheaderBuilder.getInt64Ty()
    })
  );
  auto chunkLoopExitConditionValue = chunkLoopPreheaderBuilder.CreateCall(
    llvmUMINFunction,
    ArrayRef<Value *>({
      inductionVariablePlusChunkSize,
      this->endIteration
    }),
    "chunk_loop_exit_condition_value"
  );

  /*
   * Update the branch instruction of the loop header to point
   * to the chunk loop header.
   */
  auto preheaderTerminatorClone = loopHeaderClone->getTerminator();
  preheaderTerminatorClone->setSuccessor(0, chunkLoopHeader);

  /*
   * Create a chunk loop loop-governing induction variable
   * and set an incoming value from chunk loop preheader.
   */
  auto chunkLoopInductionVariable = chunkLoopHeaderBuilder.CreatePHI(
    chunkLoopHeaderBuilder.getInt64Ty(),
    2,
    "chunk_loop_induction_variable"
  );
  chunkLoopInductionVariable->addIncoming(this->inductionVariable, loopHeaderClone);

  /*
   * Replace all uses of the induction variable inside the loop
   * body with the chunk loop induction variable.
   */
  replaceAllUsesInsideLoopBody(lc, this->inductionVariable, chunkLoopInductionVariable);

  /*
   * Set the induction variable used by the loop-slice task
   * to be the chunk loop induction variable.
   */
  auto outerLoopInductionVariableClone = this->inductionVariable;
  this->inductionVariable = chunkLoopInductionVariable;

  /*
   * Create an exit condition compare instruction
   * in the chunk loop header.
   */
  auto chunkLoopExitCmpInst = chunkLoopHeaderBuilder.CreateICmpSLT(
    this->inductionVariable,
    chunkLoopExitConditionValue
  );

  /*
   * Create a conditional branch instruction in the chunk loop header,
   * which the first successor is the loopFirstBodyClone,
   * the second successor is the loopLatchClone.
   */
  chunkLoopHeaderBuilder.CreateCondBr(
    chunkLoopExitCmpInst,
    loopFirstBodyClone,
    chunkLoopExit
  );

  /*
   * Update the branch instruction of the last body block
   * to the chunk loop latch.
   */
  auto loopLastBodyTerminatorClone = loopLastBodyClone->getTerminator();
  dyn_cast<BranchInst>(loopLastBodyTerminatorClone)->setSuccessor(0, chunkLoopLatch);

  /*
   * Update the chunk loop induction variable in the
   * chunk loop latch.
   */
  auto chunkLoopInductionVariableUpdate = chunkLoopLatchBuilder.CreateAdd(
    this->inductionVariable,
    chunkLoopLatchBuilder.getInt64(1),
    "chunk_loop_induction_variable_update"
  );

  /*
   * Add the updated chunk loop induction variable into
   * the PHINode in the chunk loop header.
   */
  this->inductionVariable->addIncoming(chunkLoopInductionVariableUpdate, chunkLoopLatch);

  /*
   * Create a branch instruction to the chunk loop header.
   */
  chunkLoopLatchBuilder.CreateBr(
    chunkLoopHeader
  );

  /*
   * Create call to update remianing chunk size in the
   * chunk loop exit block.
   */
  auto chunkLoopIterationsExecuted = chunkLoopExitBuilder.CreateSub(
    chunkLoopExitConditionValue,
    outerLoopInductionVariableClone,
    "chunk_loop_iterations_executed"
  );
  auto hasRemainingChunkSizeFunction = program->getFunction("has_remaining_chunk_size");
  assert(hasRemainingChunkSizeFunction != nullptr && "has_remaining_chunk_size function not found");
  auto hasRemainingChunkSize = chunkLoopExitBuilder.CreateCall(
    hasRemainingChunkSizeFunction,
    ArrayRef<Value *>({
      this->lsTask->getTaskMemoryPointerArg(),
      chunkLoopIterationsExecuted,
      chunkSize
    })
  );

  /*
   * Create a conditional branch to break the loop if
   * has remaining chunk size, or to the loop latch block.
   */
  chunkLoopExitBuilder.CreateCondBr(
    hasRemainingChunkSize,
    loopExitClone,
    loopLatchClone
  );

  if (this->verbose > HBTVerbosity::Disabled) {
    errs() << this->outputPrefix << "loop-slice task after chunking loop iterations\n";
    errs() << *this->lsTask->getTaskBody() << "\n";
  }

  return;
}

void HeartbeatTransformation::replaceAllUsesInsideLoopBody(LoopContent *lc, Value *originalValue, Value *newValue) {
  auto ls = lc->getLoopStructure();
  auto bbs = ls->getBasicBlocks();

  /*
   * Remove basic blocks that are preheader, header, latches and exits.
   */
  bbs.erase(ls->getPreHeader());
  bbs.erase(ls->getHeader());
  for (auto latch : ls->getLatches()) {
    bbs.erase(latch);
  }
  for (auto exit : ls->getLoopExitBasicBlocks()) {
    bbs.erase(exit);
  }

  /*
   * Collect all loop body basic block clones.
   */
  std::unordered_set<BasicBlock *> bbsClone;
  for (auto bb : bbs) {
    bbsClone.insert(this->lsTask->getCloneOfOriginalBasicBlock(bb));
  }

  /*
   * Collect all users that use the original value.
   */
  std::vector<User *> usersToReplace;
  for (auto &use : originalValue->uses()) {
    auto user = use.getUser();
    auto userBlock = dyn_cast<Instruction>(user)->getParent();
    if (bbsClone.find(userBlock) != bbsClone.end()) {
      usersToReplace.push_back(user);
    }
  }

  /*
   * Replace all uses of the original value with the new value.
   */
  for (auto user : usersToReplace) {
    user->replaceUsesOfWith(originalValue, newValue);
  }
}

} // namespace arcana::heartbeat
