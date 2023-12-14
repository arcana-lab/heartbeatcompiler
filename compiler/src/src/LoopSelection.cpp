#include "Pass.hpp"

using namespace arcana::noelle;

std::set<LoopContent *> Heartbeat::selectHeartbeatLoops (
  Noelle &noelle,
  const std::vector<LoopStructure *> *allLoops
) {

  std::set<LoopContent *> selectedLoops;

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
       * Compute LoopContent
       */
      auto ldi = noelle.getLoopContent(ls, {  LoopContentOptimization::MEMORY_CLONING_ID, LoopContentOptimization::THREAD_SAFE_LIBRARY_ID });

      selectedLoops.insert(ldi);
    }
  }

  return selectedLoops;
}
