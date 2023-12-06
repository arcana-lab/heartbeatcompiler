#include "Pass.hpp"

using namespace arcana::noelle;

std::set<LoopDependenceInfo *> Heartbeat::selectHeartbeatLoops (
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
       * Ignoing loops that are not the root loop inside the function
       */
      if (ls->getNestingLevel() != 1) {
        continue;
      }

      /*
       * Compute LoopDependenceInfo
       */
      auto ldi = noelle.getLoop(ls, {  LoopDependenceInfoOptimization::MEMORY_CLONING_ID, LoopDependenceInfoOptimization::THREAD_SAFE_LIBRARY_ID });

      selectedLoops.insert(ldi);
    }
  }

  return selectedLoops;
}
