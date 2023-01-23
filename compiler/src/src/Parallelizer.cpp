#include "Pass.hpp"
#include "HeartBeatTransformation.hpp"

using namespace llvm::noelle;

bool HeartBeat::parallelizeLoop (
  Noelle &noelle,
  LoopDependenceInfo *ldi
) {
  errs() << "Start to parallelize loop in function " << ldi->getLoopStructure()->getFunction()->getName() << "\n";
  HeartBeatTransformation HB(noelle, ldi, this->containsLiveOut, this->loopToLevel, this->loopToSkippedLiveIns);

  /*
   * Check if the loop is a DOALL
   */
  if (!HB.canBeAppliedToLoop(ldi, nullptr)){
    errs() << this->outputPrefix << "WARNING: we cannot guarantee the outermost loop in " << ldi->getLoopStructure()->getFunction()->getName() << " is a DOALL loop\n";
  }

  /*
   * Clone the original loop and make the clone to be in the heartbeat form.
   */
  auto codeModified = HB.apply(ldi, nullptr);
  if (!codeModified){
    return false;
  }

  return false;

  // /*
  //  * Link the heartbeat loop with the original code.
  //  *
  //  * Step 1: Fetch the environment array where the exit block ID has been stored.
  //  */
  // auto envArray = HB.getEnvArray();
  // assert(envArray != nullptr);

  // /*
  //  * Step 2: Fetch entry and exit point executed by the parallelized loop.
  //  */
  // auto entryPoint = HB.getParLoopEntryPoint();
  // auto exitPoint = HB.getParLoopExitPoint();
  // assert(entryPoint != nullptr && exitPoint != nullptr);

  // /*
  //  * Step 3: Link the parallelized loop within the original function that includes the sequential loop.
  //  */
  // auto tm = noelle.getTypesManager();
  // auto loopEnv = loop->getEnvironment();
  // auto exitID = loopEnv->getExitBlockID();
  // auto exitIndex = ConstantInt::get(
  //     tm->getIntegerType(64),
  //     exitID >= 0
  //         ? HB.getIndexOfEnvironmentVariable(exitID)
  //         : -1);
  // auto loopStructure = loop->getLoopStructure();
  // auto loopPreHeader = loopStructure->getPreHeader();
  // auto loopFunction = loopStructure->getFunction();
  // auto loopExitBlocks = loopStructure->getLoopExitBasicBlocks();
  // auto linker = noelle.getLinker();
  // linker->linkTransformedLoopToOriginalFunction(
  //     loopPreHeader,
  //     entryPoint,
  //     exitPoint, 
  //     envArray,
  //     exitIndex,
  //     loopExitBlocks,
  //     0
  //     );
  // loop->getLoopStructure()->getFunction()->print(errs() << "Final Original Loop\n");
  // assert(noelle.verifyCode());

  // return true;
}
