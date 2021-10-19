#include "Pass.hpp"

using namespace llvm;
using namespace llvm::noelle;

bool HeartBeat::parallelizeLoop (
  Noelle &noelle,
  LoopDependenceInfo *loop
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
   */

  // todo: collect live in / out

  /*
   * Fetch the loop handler function
   */
  auto loopHandlerFunction = program->getFunction("loop_handler");
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
  auto bodyBB = ls->getFirstLoopBasicBlockAfterTheHeader();
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
  auto heartBeatGlobalPtr = program->getGlobalVariable("heartbeat");
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
