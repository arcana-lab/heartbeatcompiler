#include "Pass.hpp"
#include "HeartbeatTransformation.hpp"

void Heartbeat::executeLoopInChunk(Noelle &noelle, LoopDependenceInfo *ldi, HeartbeatTransformation *hbt, bool isLeafLoop) {
  // errs() << "current task before chunking\n";
  // errs() << *(hbt->hbTask->getTaskBody()) << "\n";

  /*
   * Only does the loop_chunking on leaf loop,
   * for other loops, invok the has_remaining_iterations function before jump to the polling_block
   */

  // How the loop looks like after making the chunking transformation
  // uint64_t chunksize;
  // for (; startIter < maxIter; startIter += chunksize) {
  //   chunksize = get_chunksize(tmem);
  //   uint64_t low = startIter;
  //   uint64_t high = maxIter < startIter + chunksize ? maxIter : startIter + chunksize;
  //   for (; low < high; low++) {
  //     r_private += a[i][low];
  //   }
  //
  //   update_remaining_chunksize(tmem, high - low);
  //   if (has_remaining_chunksize(tmem)) {
  //     break;
  //   }

  //   rc = loop_handler(...);
  //   if (rc > 0) {
  //     break;
  //   }
  // }

  // only chunking execute the leaf loop
  if (!isLeafLoop) {
    return;
  }

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
  auto chunkLoopPreheaderBlock = BasicBlock::Create(hbt->hbTask->getTaskBody()->getContext(), "chunk_loop_pre_header", hbt->hbTask->getTaskBody());
  IRBuilder chunkLoopPreheaderBlockBuilder{ chunkLoopPreheaderBlock };
  auto chunkLoopHeaderBlock = BasicBlock::Create(hbt->hbTask->getTaskBody()->getContext(), "chunk_loop_header", hbt->hbTask->getTaskBody());
  IRBuilder chunkLoopHeaderBlockBuilder{ chunkLoopHeaderBlock };
  auto chunkLoopLatchBlock = BasicBlock::Create(hbt->hbTask->getTaskBody()->getContext(), "chunk_loop_latch", hbt->hbTask->getTaskBody());
  IRBuilder chunkLoopLatchBlockBuilder{ chunkLoopLatchBlock };
  auto chunkLoopExitBlock = BasicBlock::Create(hbt->hbTask->getTaskBody()->getContext(), "chunk_loop_exit", hbt->hbTask->getTaskBody());
  IRBuilder chunkLoopExitBlockBuilder{ chunkLoopExitBlock };

  // Step 1.5: update the cond br in the outer loop to jump to the chunk_loop_pre_header
  loopHeaderTerminatorCloned->setSuccessor(0, chunkLoopPreheaderBlock);

  // Step 2: load the chunksize at the preheader of the chunk loop
  auto getChunksizeFunction = noelle.getProgram()->getFunction("get_chunksize");
  assert(getChunksizeFunction != nullptr);
  auto getChunksizeCallInst = chunkLoopPreheaderBlockBuilder.CreateCall(
    getChunksizeFunction,
    ArrayRef<Value *>({
      hbt->hbTask->getHBMemoryArg()
    }),
    "chunksize"
  );

  // Step 3: calculate high value = min(startIter + chunksize, maxIter)
  auto IVM = ldi->getInductionVariableManager();
  auto GIV = IVM->getLoopGoverningInductionVariable();
  auto IV = GIV->getValueToCompareAgainstExitConditionValue();
  auto IV_cloned = hbt->hbTask->getCloneOfOriginalInstruction(IV);
  auto startIterPlusChunksize = chunkLoopPreheaderBlockBuilder.CreateAdd(
    IV_cloned,
    getChunksizeCallInst,
    "startIterPlusChunksize"
  );
  auto llvmUMINFunction = Intrinsic::getDeclaration(noelle.getProgram(), Intrinsic::umin, ArrayRef<Type *>({ chunkLoopPreheaderBlockBuilder.getInt64Ty() }));
  auto high = chunkLoopPreheaderBlockBuilder.CreateCall(
    llvmUMINFunction,
    ArrayRef<Value *>({
      startIterPlusChunksize,
      hbt->maxIteration
    }),
    "high"
  );

  // Step 3.1: update the stride of the outer loop's IV with chunksize
  auto IV_cloned_update = cast<PHINode>(IV_cloned)->getIncomingValue(1);
  assert(isa<BinaryOperator>(IV_cloned_update));
  errs() << "update of induction variable " << *IV_cloned_update << "\n";
  cast<Instruction>(IV_cloned_update)->setOperand(1, getChunksizeCallInst);

  // Step 3.2: ensure the instruction that use to determine whether keep running the outer loop is icmp slt but not icmp ne
  auto exitCmpInst = GIV->getHeaderCompareInstructionToComputeExitCondition();
  auto exitCmpInstCloned = hbt->hbTask->getCloneOfOriginalInstruction(exitCmpInst);
  errs() << "exit compare inst" << *exitCmpInstCloned << "\n";
  assert(isa<CmpInst>(exitCmpInstCloned) && "exit condtion determination isn't a compare inst\n");
  cast<CmpInst>(exitCmpInstCloned)->setPredicate(CmpInst::Predicate::ICMP_ULT);
  errs() << "exit compare inst after modifying the predict" << *exitCmpInstCloned << "\n";

  // branch the preheader of the chunk loop to the header of the chunk loop
  // this marks the finish of preheader
  chunkLoopPreheaderBlockBuilder.CreateBr(chunkLoopHeaderBlock);

  // Step 4: Create uint64_t low
  auto lowPhiNode = chunkLoopHeaderBlockBuilder.CreatePHI(
    IV_cloned->getType(),
    2,
    "low"
  );
  lowPhiNode->addIncoming(
    IV_cloned,
    chunkLoopPreheaderBlock
  );

  // replace all uses of the cloned IV inside the loop body with low
  replaceAllUsesInsideLoopBody(ldi, hbt, IV_cloned, lowPhiNode);

  // Step 5: update induction variable low in latch block
  // update branch to the chunk loop header
  // adding incoming value for updated low
  auto lowUpdate = chunkLoopLatchBlockBuilder.CreateAdd(
    lowPhiNode,
    chunkLoopLatchBlockBuilder.getInt64(1),
    "lowUpdate"
  );
  chunkLoopLatchBlockBuilder.CreateBr(chunkLoopHeaderBlock);
  lowPhiNode->addIncoming(lowUpdate, chunkLoopLatchBlock);

  // Step 6: replace the original successor of last loop body to the chunk_loop_latch block
  auto lastBodyBlock = hbt->getPollingBlock()->getSinglePredecessor();
  auto lastBodyTerminator = dyn_cast<BranchInst>(lastBodyBlock->getTerminator());
  lastBodyTerminator->setSuccessor(0, chunkLoopLatchBlock);

  // Step 7: add incoming value for returnCode from the chunk_loop_exit block
  hbt->returnCodePhiInst->addIncoming(
    ConstantInt::get(chunkLoopExitBlockBuilder.getInt64Ty(), 0),
    chunkLoopExitBlock
  );

  // Step 8: Create phis for all live-out variables and populates into the outer loop exit block
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
      chunkLoopPreheaderBlock
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

  // create compare instruction for the chunk loop execution
  // if (low < high) go to body, otherwise go to chunk_loop_exit
  // this needs to be updated after populating all the live-outs
  auto loopCompareInst = chunkLoopHeaderBlockBuilder.CreateICmpULT(
    lowPhiNode,
    high
  );
  chunkLoopHeaderBlockBuilder.CreateCondBr(
    loopCompareInst,
    loopBodyBlockCloned,
    chunkLoopExitBlock
  );

  // step 9: update the chunksize in the chunk_loop_exit block
  // Step 10: call has_remaining_chunksize and jump either to the exit block of outer loop or the software polling block
  auto highSubStartIter = chunkLoopExitBlockBuilder.CreateSub(
    high, IV_cloned,
    "highSubStartIter"
  );
  auto updateAndHasRemainingChunksizeFunction = noelle.getProgram()->getFunction("update_and_has_remaining_chunksize");
  assert(updateAndHasRemainingChunksizeFunction != nullptr);
  auto updateAndHasRemainingChunksizeCall = chunkLoopExitBlockBuilder.CreateCall(
    updateAndHasRemainingChunksizeFunction,
    ArrayRef<Value *>({
      hbt->hbTask->getHBMemoryArg(),
      highSubStartIter,
      getChunksizeCallInst
    })
  );
  chunkLoopExitBlockBuilder.CreateCondBr(
    updateAndHasRemainingChunksizeCall,
    loopExitCloned,
    hbt->getPollingBlock()
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
