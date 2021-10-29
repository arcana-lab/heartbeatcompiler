#include "Pass.hpp"
#include "HeartBeatTransformation.hpp"

using namespace llvm;
using namespace llvm::noelle;

std::set<LoopDependenceInfo *> HeartBeat::selectLoopsToTransform (
  Noelle &noelle,
  const std::vector<LoopStructure *> &allLoops
  ){
  std::set<LoopDependenceInfo *> selectedLoops;

  /*
   * Organize all program loops into their nesting hierarchy
   */


  /*
   * For now, let's just consider programs with a single loop
   */
  auto loop = allLoops[0];
  auto ldi = noelle.getLoop(loop);

  return selectedLoops;
}
