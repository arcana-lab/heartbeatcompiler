#include "Pass.hpp"

using namespace llvm;
using namespace llvm::noelle;

bool HeartBeat::parallelizeLoop (
  Noelle &noelle,
  LoopDependenceInfo *loop
  ){

  /*
   * Clone the original loop and make the clone to be in the heartbeat form.
   */
  ParallelizationTechnique *usedTechnique = nullptr;
  auto codeModified = this->createHeartBeatLoop(noelle, loop, &usedTechnique);
  if (!codeModified){
    return false;
  }

  /*
   * Link the heartbeat loop with the original code.
   *
   * Step 1: Fetch the environment array where the exit block ID has been stored.
   */
  assert(usedTechnique != nullptr);
  auto envArray = usedTechnique->getEnvArray();
  assert(envArray != nullptr);

  /*
   * Step 2: Fetch entry and exit point executed by the parallelized loop.
   */
  auto entryPoint = usedTechnique->getParLoopEntryPoint();
  auto exitPoint = usedTechnique->getParLoopExitPoint();
  assert(entryPoint != nullptr && exitPoint != nullptr);

  /*
   * Step 3: Link the parallelized loop within the original function that includes the sequential loop.
   */
  auto tm = noelle.getTypesManager();
  auto exitIndex = ConstantInt::get(tm->getIntegerType(64), loop->environment->indexOfExitBlockTaken());
  auto loopStructure = loop->getLoopStructure();
  auto loopPreHeader = loopStructure->getPreHeader();
  auto loopFunction = loopStructure->getFunction();
  auto loopExitBlocks = loopStructure->getLoopExitBasicBlocks();
  noelle.linkTransformedLoopToOriginalFunction(
      loopFunction->getParent(),
      loopPreHeader,
      entryPoint,
      exitPoint, 
      envArray,
      exitIndex,
      loopExitBlocks
      );
  assert(noelle.verifyCode());

  return true;
}
