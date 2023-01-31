#include "Pass.hpp"

using namespace llvm::noelle;

std::set<LoopDependenceInfo *> HeartBeat::selectHeartbeatLoops (
  Noelle &noelle,
  const std::vector<LoopStructure *> *allLoops
) {

  std::set<LoopDependenceInfo *> selectedLoops;

  for (auto ls : *allLoops) {
    /*
     * Select loops within functions that starts with "HEARTBEAT_"
     */
    if (ls->getFunction()->getName().contains(this->functionSubString)) {
      /*
       * Compute LoopDependenceInfo
       */
      auto ldi = noelle.getLoop(ls, {  LoopDependenceInfoOptimization::MEMORY_CLONING_ID, LoopDependenceInfoOptimization::THREAD_SAFE_LIBRARY_ID });

      selectedLoops.insert(ldi);
    }
  }

  return selectedLoops;
}
