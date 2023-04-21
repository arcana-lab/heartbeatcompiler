#include "Pass.hpp"
#include "HeartbeatTransformation.hpp"

void Heartbeat::executeLoopInChunk(LoopDependenceInfo *ldi, HeartbeatTransformation *hbt, bool isLeafLoop) {
  // errs() << "current task before chunking\n";
  // errs() << *(hbt->hbTask->getTaskBody()) << "\n";

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
  //     break;
  //   }
  // }

  auto ls = ldi->getLoopStructure();
  auto loopHeader = ls->getHeader();
  auto loopHeaderTerminator = loopHeader->getTerminator();
  auto loopHeaderTerminatorCloned = cast<BranchInst>(hbt->hbTask->getCloneOfOriginalInstruction(loopHeaderTerminator));
  auto loopBodyBlockCloned = loopHeaderTerminatorCloned->getSuccessor(0);
  auto loopExits = ls->getLoopExitBasicBlocks();
  assert(loopExits.size() == 1 && "hb pass doesn't handle multiple exit blocks");
  auto loopExit = *(loopExits.begin());
  auto loopExitCloned = hbt->hbTask->getCloneOfOriginalBasicBlock(loopExit);

  // Step 1: Create blocks for the chunk-executed loop
  auto chunkLoopHeaderBlock = BasicBlock::Create(hbt->hbTask->getTaskBody()->getContext(), "chunk_loop_header", hbt->hbTask->getTaskBody());
  IRBuilder chunkLoopHeaderBlockBuilder{ chunkLoopHeaderBlock };
  auto chunkLoopLatchBlock = BasicBlock::Create(hbt->hbTask->getTaskBody()->getContext(), "chunk_loop_latch", hbt->hbTask->getTaskBody());
  IRBuilder chunkLoopLatchBlockBuilder{ chunkLoopLatchBlock };
  auto chunkLoopExitBlock = BasicBlock::Create(hbt->hbTask->getTaskBody()->getContext(), "chunk_loop_exit", hbt->hbTask->getTaskBody());
  IRBuilder chunkLoopExitBlockBuilder{ chunkLoopExitBlock };

  // Step 2: Create uint64_t low
  //    first get the induction variable in the original loop
  auto IV_attr = ldi->getLoopGoverningIVAttribution();
  auto IV = IV_attr->getValueToCompareAgainstExitConditionValue();
  auto IV_cloned = hbt->hbTask->getCloneOfOriginalInstruction(IV);
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
  replaceAllUsesInsideLoopBody(ldi, hbt, IV_cloned, lowPhiNode);

  // Step: update the stride of the outer loop's IV with chunksize
  auto IV_cloned_update = cast<PHINode>(IV_cloned)->getIncomingValue(1);
  assert(isa<BinaryOperator>(IV_cloned_update));
  errs() << "update of induction variable " << *IV_cloned_update << "\n";
  cast<Instruction>(IV_cloned_update)->setOperand(1, chunkLoopHeaderBlockBuilder.getInt64(this->loopToChunksize[ldi]));

  // Step: ensure the instruction that use to determine whether keep running the loop is icmp slt but not icmp ne
  auto exitCmpInst = IV_attr->getHeaderCompareInstructionToComputeExitCondition();
  auto exitCmpInstCloned = hbt->hbTask->getCloneOfOriginalInstruction(exitCmpInst);
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

  // Step: replace the original successor to polling block with the chunk_loop_latch block
  auto lastBodyBlock = hbt->getPollingBlock()->getSinglePredecessor();
  auto pollingBlockBr = dyn_cast<BranchInst>(lastBodyBlock->getTerminator());
  if (isLeafLoop) {
    pollingBlockBr->setSuccessor(0, chunkLoopLatchBlock);
  } else {
    // if this is not a leaf loop, then the terminator at the moment should be
    // if (nested_loop_return_code > 0) {
    //   break
    // } else {
    // go to polling

    // TODO
    // here we break from the chunking loop directly out of the outer loop,
    // this could introduce a potential bug that the live-out values aren't
    // correctly populated.
    // Another solution is to modify the first successor from going to the loop_exit_block to chunk_loop_exit_block
    pollingBlockBr->setSuccessor(1, chunkLoopLatchBlock);
  }

  // Step: compare low == maxIter and jump either to the exit block of outer loop or the loop_handler_block
  auto lowMaxIterCmpInst = chunkLoopExitBlockBuilder.CreateICmpEQ(
    lowPhiNode,
    hbt->maxIteration
  );
  chunkLoopExitBlockBuilder.CreateCondBr(
    lowMaxIterCmpInst,
    loopExitCloned,
    hbt->getPollingBlock()
  );

  // Step: add incoming value for returnCode from the chunk_loop_exit block
  hbt->returnCodePhiInst->addIncoming(
    ConstantInt::get(chunkLoopExitBlockBuilder.getInt64Ty(), 0),
    chunkLoopExitBlock
  );

  // Step 4: Create phis for all live-out variables and populates into the outer loop exit block
  auto loopEnv = ldi->getEnvironment();
  for (auto liveOutVarID : loopEnv->getEnvIDsOfLiveOutVars()) {
    auto producer = loopEnv->getProducer(liveOutVarID);
    auto producerCloned = hbt->hbTask->getCloneOfOriginalInstruction(cast<Instruction>(producer));
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

    // replace the incoming value from the loop_handler block to be the new liveOutPhiNode
    cast<PHINode>(hbt->liveOutVariableToAccumulatedPrivateCopy[liveOutVarID])->setIncomingValueForBlock(hbt->loopHandlerBlock, liveOutPhiNode);

    // add another incoming value to the propagated value in the exit block of the outer loop
    cast<PHINode>(hbt->liveOutVariableToAccumulatedPrivateCopy[liveOutVarID])->addIncoming(liveOutPhiNode, chunkLoopExitBlock);

    // Replace all uses of the cloned liveOut inside the loop body with liveOutPhiNode
    replaceAllUsesInsideLoopBody(ldi, hbt, producerCloned, liveOutPhiNode);
  }

  // Step 5: determine the max value
  auto startIterPlusChunk = chunkLoopHeaderBlockBuilder.CreateAdd(
    IV_cloned,
    chunkLoopHeaderBlockBuilder.getInt64(this->loopToChunksize[ldi])
  );
  auto highCompareInst = chunkLoopHeaderBlockBuilder.CreateICmpSLT(
    hbt->maxIteration,
    startIterPlusChunk
  );
  auto high = chunkLoopHeaderBlockBuilder.CreateSelect(
    highCompareInst,
    hbt->maxIteration,
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

  // replace the store of currentIteration with low - 1
  IRBuilder loopHandlerBlockBuilder{ hbt->getLoopHandlerBlock() };
  loopHandlerBlockBuilder.SetInsertPoint(hbt->storeCurrentIterationAtBeginningOfHandlerBlock);
  auto lowSubOneInst = loopHandlerBlockBuilder.CreateSub(
    lowPhiNode,
    loopHandlerBlockBuilder.getInt64(1),
    "lowSubOne"
  );
  hbt->storeCurrentIterationAtBeginningOfHandlerBlock->setOperand(0, lowSubOneInst);

  errs() << "current task after chunking\n";
  errs() << *(hbt->hbTask->getTaskBody()) << "\n";

  return;
}

void Heartbeat::replaceAllUsesInsideLoopBody(LoopDependenceInfo *ldi, HeartbeatTransformation *hbt, Value *originalVal, Value *replacedVal) {
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
    bbsCloned.insert(hbt->hbTask->getCloneOfOriginalBasicBlock(bb));
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
