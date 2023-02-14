#include "Pass.hpp"
#include "HeartBeatTransformation.hpp"
#include "noelle/core/Architecture.hpp"

using namespace llvm::noelle;

bool HeartBeat::parallelizeRootLoop (
  Noelle &noelle,
  LoopDependenceInfo *ldi
) {
  errs() << "Start to parallelize loop in function " << ldi->getLoopStructure()->getFunction()->getName() << "\n";
  HeartBeatTransformation *HB = new HeartBeatTransformation(
    noelle,
    ldi,
    this->numLevels,
    this->containsLiveOut,
    this->loopToLevel,
    this->loopToSkippedLiveIns,
    this->constantLiveInsArgIndexToIndex,
    this->loopToConstantLiveIns,
    this->loopToHeartBeatTransformation,
    this->loopToCallerLoop,
    this->loopToChunksize
  );
  this->loopToHeartBeatTransformation[ldi] = HB;

  /*
   * Check if the loop is a DOALL
   */
  if (!HB->canBeAppliedToLoop(ldi, nullptr)){
    errs() << this->outputPrefix << "WARNING: we cannot guarantee the outermost loop in " << ldi->getLoopStructure()->getFunction()->getName() << " is a DOALL loop\n";
  }

  /*
   * Clone the original loop and make the clone to be in the heartbeat form.
   */
  auto codeModified = HB->apply(ldi, nullptr);
  if (!codeModified){
    return false;
  }

  // /*
  //  * Link the heartbeat loop with the original code.
  //  *
  //  * Step 1: Fetch the environment array where the exit block ID has been stored.
  //  */
  // auto envArray = HB.getEnvArray();
  // assert(envArray != nullptr);

  /*
   * Step 2: Fetch entry and exit point executed by the parallelized loop.
   */
  auto entryPoint = HB->getParLoopEntryPoint();
  auto exitPoint = HB->getParLoopExitPoint();
  assert(entryPoint != nullptr && exitPoint != nullptr);

  /*
   * Step 3: Link the parallelized loop within the original function that includes the sequential loop.
   */
  auto tm = noelle.getTypesManager();
  auto loopEnv = ldi->getEnvironment();
  auto exitID = loopEnv->getExitBlockID();
  auto exitIndex = ConstantInt::get(
      tm->getIntegerType(64),
      exitID >= 0
          ? HB->getIndexOfEnvironmentVariable(exitID)
          : -1);
  auto loopStructure = ldi->getLoopStructure();
  auto loopPreHeader = loopStructure->getPreHeader();
  auto loopFunction = loopStructure->getFunction();
  auto loopExitBlocks = loopStructure->getLoopExitBasicBlocks();
  // auto linker = noelle.getLinker();
  // linker->linkTransformedLoopToOriginalFunction(
  this->linkTransformedLoopToOriginalFunction(
      loopPreHeader,
      entryPoint,
      exitPoint, 
      nullptr,
      exitIndex,
      loopExitBlocks,
      tm,
      noelle
      );
  ldi->getLoopStructure()->getFunction()->print(errs() << "Final Original Loop\n");
  // assert(noelle.verifyCode());

  return true;
}

void HeartBeat::linkTransformedLoopToOriginalFunction(
    BasicBlock *originalPreHeader,
    BasicBlock *startOfParLoopInOriginalFunc,
    BasicBlock *endOfParLoopInOriginalFunc,
    Value *envArray,
    Value *envIndexForExitVariable,
    std::vector<BasicBlock *> &loopExitBlocks,
    llvm::noelle::TypesManager *tm,
    Noelle &noelle) {

  /*
   * Create the constants.
   */
  auto integerType = tm->getIntegerType(32);

  /*
   * Fetch the terminator of the preheader.
   */
  auto originalTerminator = originalPreHeader->getTerminator();

  /*
   * Fetch the header of the original loop.
   */
  auto originalHeader = originalTerminator->getSuccessor(0);

  IRBuilder<> loopSwitchBuilder(originalTerminator);
  auto runHeartbeatBoolGlobal = noelle.getProgram()->getNamedGlobal("run_heartbeat");
  auto loadRunHeartbeatBool = loopSwitchBuilder.CreateLoad(runHeartbeatBoolGlobal);
  auto cmpInst = loopSwitchBuilder.CreateICmpEQ(
    loadRunHeartbeatBool,
    ConstantInt::get(loadRunHeartbeatBool->getType(), 1)
  );
  loopSwitchBuilder.CreateCondBr(
    cmpInst,
    startOfParLoopInOriginalFunc,
    originalHeader
  );
  originalTerminator->eraseFromParent();

  /*
   * Load exit block environment variable and branch to the correct loop exit
   * block
   */
  IRBuilder<> endBuilder(endOfParLoopInOriginalFunc);
  if (loopExitBlocks.size() == 1) {
    endBuilder.CreateBr(loopExitBlocks[0]);

  } else {

    /*
     * Compute how many values can fit in a cache line.
     */
    auto valuesInCacheLine =
        Architecture::getCacheLineBytes() / sizeof(int64_t);

    auto int64 = tm->getIntegerType(64);
    auto exitEnvPtr = endBuilder.CreateInBoundsGEP(
        envArray,
        ArrayRef<Value *>({ cast<Value>(ConstantInt::get(int64, 0)),
                            endBuilder.CreateMul(
                                envIndexForExitVariable,
                                ConstantInt::get(int64, valuesInCacheLine)) }));
    auto exitEnvCast =
        endBuilder.CreateIntCast(endBuilder.CreateLoad(exitEnvPtr),
                                 integerType,
                                 /*isSigned=*/false);
    auto exitSwitch = endBuilder.CreateSwitch(exitEnvCast, loopExitBlocks[0]);
    for (int i = 1; i < loopExitBlocks.size(); ++i) {
      auto constantInt = cast<ConstantInt>(ConstantInt::get(integerType, i));
      exitSwitch->addCase(constantInt, loopExitBlocks[i]);
    }
  }

  /*
   * NOTE(angelo): LCSSA constants need to be replicated for parallelized code
   * path
   */
  for (auto bb : loopExitBlocks) {
    for (auto &I : *bb) {
      if (auto phi = dyn_cast<PHINode>(&I)) {
        auto bbIndex = phi->getBasicBlockIndex(originalHeader);
        if (bbIndex == -1) {
          continue;
        }
        auto val = phi->getIncomingValue(bbIndex);
        if (isa<Constant>(val)) {
          phi->addIncoming(val, endOfParLoopInOriginalFunc);
        }
        continue;
      }
      break;
    }
  }

  return;
}


bool HeartBeat::parallelizeNestedLoop (
  Noelle &noelle,
  LoopDependenceInfo *ldi
) {
  errs() << "Start to parallelize loop in function " << ldi->getLoopStructure()->getFunction()->getName() << "\n";
  HeartBeatTransformation *HB = new HeartBeatTransformation(
    noelle,
    ldi,
    this->numLevels,
    this->containsLiveOut,
    this->loopToLevel,
    this->loopToSkippedLiveIns,
    this->constantLiveInsArgIndexToIndex,
    this->loopToConstantLiveIns,
    this->loopToHeartBeatTransformation,
    this->loopToCallerLoop,
    this->loopToChunksize
  );
  this->loopToHeartBeatTransformation[ldi] = HB;

  /*
   * Check if the loop is a DOALL
   */
  if (!HB->canBeAppliedToLoop(ldi, nullptr)){
    errs() << this->outputPrefix << "WARNING: we cannot guarantee the outermost loop in " << ldi->getLoopStructure()->getFunction()->getName() << " is a DOALL loop\n";
  }

  /*
   * Clone the original loop and make the clone to be in the heartbeat form.
   */
  auto codeModified = HB->apply(ldi, nullptr);
  if (!codeModified){
    return false;
  }

  // these two bbs are unused, safe to erase them
  HB->getParLoopEntryPoint()->eraseFromParent();
  HB->getParLoopExitPoint()->eraseFromParent();

  return true;
}