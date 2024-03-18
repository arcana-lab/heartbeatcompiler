#include "Pass.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

void Heartbeat::parallelizeLoopNest(uint64_t nestID) {
  if (this->verbose > Verbosity::Disabled) {
    errs() << this->outputPrefix << "Start parallelizing nestID " << nestID << "\n";
  }

  /*
   * Perform loop invariant analysis.
   */

  /*
   * Parallelize loops starting from the root loop.
   */
  for (uint64_t level = 0; level < this->lna->getLoopNestNumLevels(nestID); level++) {
    for (auto lc : this->lna->getLoopsAtLevel(nestID, level)) {
      parallelizeLoop(nestID, lc);
    }
  }

  /*
   * Link parallelized loop nest.
   */

  if (this->verbose > Verbosity::Disabled) {
    errs() << this->outputPrefix << "End parallelizing nestID " << nestID << "\n";
  }

  return;
}

void Heartbeat::parallelizeLoop(uint64_t nestID, LoopContent *lc) {
  auto ls = lc->getLoopStructure();
  auto fn = ls->getFunction();

  // if (this->verbose > Verbosity::Disabled) {
  //   errs() << this->outputPrefix << "start parallelizing loop " << this->lna->printLoopID(lc) << "\n";
  // }

  /*
   * Construct a new heartbeat transformation object for the loop.
   */
  // TODO: delete later
  std::unordered_map<LoopContent *, std::unordered_set<Value *>> loopToSkippedLiveIns;
  std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> loopToConstantLiveIns;
  std::unordered_map<int, int> constantLiveInsArgIndexToIndex; 
  HeartbeatTransformation *hbt = new HeartbeatTransformation(
    this->noelle,
    this->hbrm,
    this->lna,
    nestID,
    lc,
    loopToSkippedLiveIns,
    loopToConstantLiveIns,
    constantLiveInsArgIndexToIndex
  );
  this->loopToHeartbeatTransformation[lc] = hbt;

  /*
   * Apply the heartbeat transformation on the loop.
   */
  hbt->apply(lc, nullptr);

  // if (this->verbose > Verbosity::Disabled) {
  //   errs() << this->outputPrefix << "end parallelizing loop " << this->lna->printLoopID(lc) << "\n";
  // }

  return;
}

} // namespace arcana::heartbeat
