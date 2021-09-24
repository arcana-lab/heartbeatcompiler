#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include "Pass.hpp"

#include "Noelle.hpp"

using namespace llvm;
using namespace llvm::noelle;

HeartBeat::HeartBeat () 
  : ModulePass(ID) {
  return ;
}

bool HeartBeat::doInitialization (Module &M) {
  return false;
}

bool HeartBeat::runOnModule (Module &M) {
  errs() << "Heartbeat pass\n";

  /*
   * Fetch NOELLE.
   */
  auto& noelle = getAnalysis<Noelle>();

  /*
   * Fetch all program loops
   */
  auto loops = noelle.getLoopStructures();

  // For now, let's just consider programs with a single loop
  auto loop = (*loops)[0];
  auto loopFunction = loop->getFunction();

  // For now, let's assume all iterations are independent

  // todo: collect live in / out

  /*
   * Fetch the loop handler function
   */
  auto loopHandlerFunction = M.getFunction("loop_handler");
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
  auto bodyBB = loop->getFirstLoopBasicBlockAfterTheHeader();
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
  auto heartBeatGlobalPtr = M.getGlobalVariable("heartbeat");
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

void HeartBeat::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<Noelle>(); // so that LLVM knows this pass depends on noelle
}

// Next there is code to register your pass to "opt"
char HeartBeat::ID = 0;
static RegisterPass<HeartBeat> X("heartbeat", "Heartbeat transformation");

// Next there is code to register your pass to "clang"
static HeartBeat * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new HeartBeat());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new HeartBeat()); }}); // ** for -O0
