#include "Pass.hpp"

using namespace llvm::noelle;

void HeartBeat::handleLiveOut(
  Noelle &noelle,
  const std::set<LoopDependenceInfo *> &heartbeatLoops
) {

  errs() << this->outputPrefix << "live-out analysis starts\n";

  for (auto ldi : heartbeatLoops) {
    errs() << this->outputPrefix << ldi->getLoopStructure()->getFunction()->getName() << "\n";
    auto loopEnv = ldi->getEnvironment();
    loopEnv->printEnvironmentInfo();

    if (loopEnv->getLiveOutSize() > 0) {
      this->containsLiveOut = true;
    }
  }

  if (this->containsLiveOut) {
    errs() << this->outputPrefix << "need to handle live-out\n";
  } else {
    errs() << this->outputPrefix << "doesn't need to handle live-out\n";
  }

  errs() << this->outputPrefix << "live-out analysis completes\n";

  return;
}