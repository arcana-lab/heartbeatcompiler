#include "Pass.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

std::set<LoopContent *> Heartbeat::selectHeartbeatLoops(
  const std::vector<LoopStructure *> *allLoops
) {

  std::set<LoopContent *> heartbeatLoops;

  for (auto ls : *allLoops) {
    /*
     * Select loops within functions that starts with functionPrefix, i.e, "HEARTBEAT_"
     */
    if (ls->getFunction()->getName().contains(this->functionPrefix)) {
      /*
       * Only considering the root loop inside the function
       */
      if (ls->getNestingLevel() == 1) {
        /*
         * Compute LoopContent
         */
        auto lc = this->noelle->getLoopContent(
          ls,
          {
            LoopContentOptimization::MEMORY_CLONING_ID,
            LoopContentOptimization::THREAD_SAFE_LIBRARY_ID
          }
        );

        heartbeatLoops.insert(lc);
      }
    }
  }

  if (this->verbose > Verbosity::Disabled) {
    errs() << this->outputPrefix << heartbeatLoops.size() << " loops will be parallelized\n";
    for (auto heartbeatLoop : heartbeatLoops) {
      errs() << this->outputPrefix << "  Function \"" << heartbeatLoop->getLoopStructure()->getFunction()->getName() << "\"\n";
      errs() << this->outputPrefix << "    Loop entry instruction \"" << *heartbeatLoop->getLoopStructure()->getEntryInstruction() << "\"\n";
    }
  }

  return heartbeatLoops;
}

} // namespace arcana::heartbeat
