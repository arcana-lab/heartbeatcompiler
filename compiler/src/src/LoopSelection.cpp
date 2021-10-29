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
  auto forest = noelle.organizeLoopsInTheirNestingForest(allLoops);

  /*
   * Remove loops that are not inside the pre-defined functions.
   */
  auto filter = [this](StayConnectedNestedLoopForestNode *node, uint32_t depth) -> bool {
    auto loop = node->getLoop();
    auto loopFunction = loop->getFunction();
    auto loopFunctionName = loopFunction->getName();
    if (!loopFunctionName.contains(this->functionSubString)){
      delete node ;
    }
      
    return false;
  };
  for (auto tree : forest->getTrees()){
    tree->visitPreOrder(filter);
  }

  /*
   * Consider all outermost loops left in the forest.
   */
  for (auto tree : forest->getTrees()){

    /*
     * Fetch the loop
     */
    auto loop = tree->getLoop();

    /*
     * Compute the LoopDependenceInfo
     */
    auto ldi = noelle.getLoop(loop);

    /*
     * Add the loop to the list
     */
    selectedLoops.insert(ldi);
  }

  return selectedLoops;
}
