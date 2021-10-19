#include "HeartBeatTask.hpp"
#include "HeartBeatTransformation.hpp"

using namespace llvm;
using namespace llvm::noelle;

HeartBeatTransformation::HeartBeatTransformation (
  Noelle &noelle
  ) : 
    DOALL{*noelle.getProgram(), *noelle.getProfiles(), noelle.getVerbosity()}
  , n{noelle} 
{

  /*
   * Fetch the dispatcher to use to jump to a parallelized DOALL loop.
   */
  this->taskDispatcher = this->module.getFunction("handler_for_fork2");
  if (this->taskDispatcher == nullptr){
    errs() << "NOELLE: ERROR = function NOELLE_DOALLDispatcher couldn't be found\n";
    abort();
  }

  /*
   * Create the task signature
   */
  auto tm = noelle.getTypesManager();
  auto int8 = tm->getIntegerType(8);
  auto int64 = tm->getIntegerType(64);
  auto funcArgTypes = ArrayRef<Type*>({
    int64,
    int64,
    PointerType::getUnqual(int8),
    PointerType::getUnqual(int8),
  });
  this->taskSignature = FunctionType::get(tm->getVoidType(), funcArgTypes, false);

  return ;
}

bool HeartBeatTransformation::apply (
  LoopDependenceInfo *loop,
  Noelle &noelle,
  Heuristics *h
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
   *
   * Fetch the environment of the loop.
   */
  auto loopEnvironment = loop->environment;

  /*
   * Generate an empty task for the heartbeat execution.
   */
  auto hbTask = new HeartBeatTask(this->taskSignature, this->module);
  this->addPredecessorAndSuccessorsBasicBlocksToTasks(loop, { hbTask });

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

Value * HeartBeatTransformation::fetchClone (Value *original) const {
  auto task = (HeartBeatTask *)this->tasks[0];
  if (isa<ConstantData>(original)) return original;

  if (task->isAnOriginalLiveIn(original)){
    return task->getCloneOfOriginalLiveIn(original);
  }

  assert(isa<Instruction>(original));
  auto iClone = task->getCloneOfOriginalInstruction(cast<Instruction>(original));
  assert(iClone != nullptr);
  return iClone;
}
