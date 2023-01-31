#include "Pass.hpp"

using namespace llvm::noelle;

void HeartBeat::performLoopLevelAnalysis (
  Noelle &noelle,
  const std::set<LoopDependenceInfo *> &heartbeatLoops
) {

  errs() << this->outputPrefix << "Loop level analysis starts\n";

  auto callGraph = noelle.getFunctionsManager()->getProgramCallGraph();
  assert(callGraph != nullptr && "Program call graph is not found");

  /*
   * Compute the callgraph function to corresponding loop
   */
  std::unordered_map<LoopDependenceInfo *, CallGraphFunctionNode *> loopToCallGraphNode;
  std::unordered_map<CallGraphFunctionNode *, LoopDependenceInfo *> callGraphNodeToLoop;
  for (auto ldi : heartbeatLoops) {
    auto loopStructure = ldi->getLoopStructure();
    assert(loopStructure != nullptr && "LoopStructure not found");
    auto loopFunction = loopStructure->getFunction();
    assert(loopFunction != nullptr && "Loop function not found");
    assert(loopFunction->getName().contains(this->functionSubString) && "Selected loop is not outlined into a function");
    auto callGraphNode = callGraph->getFunctionNode(loopFunction);
    assert(callGraphNode != nullptr && "CallGraphNode nout found");

    loopToCallGraphNode[ldi] = callGraphNode;
    callGraphNodeToLoop[callGraphNode] = ldi;
    this->functionToLoop[loopFunction] = ldi;
  }

  /*
   * Go through each loop to determine its level and root loop
   */
  for (auto ldi : heartbeatLoops) {
    /*
     * The level and root loop has already been determined
     */
    if (this->loopToLevel.find(ldi) != this->loopToLevel.end()) {
      assert(this->loopToRoot.find(ldi) != this->loopToRoot.end() && "Loop level has been set but the root loop isn't correctly yet");
      /*
       * Print result
       */
      errs() << this->outputPrefix << ldi->getLoopStructure()->getFunction()->getName() << " of level " << this->loopToLevel[ldi] << ", and root loop " << this->loopToRoot[ldi]->getLoopStructure()->getFunction()->getName() << "\n";
      continue;
    }

    /*
     * Set the level and root recursively
     */
    setLoopLevelAndRoot(ldi, loopToCallGraphNode, callGraphNodeToLoop, *callGraph);

    /*
     * Print result
     */
    errs() << this->outputPrefix << ldi->getLoopStructure()->getFunction()->getName() << " of level " << this->loopToLevel[ldi] << ", and root loop " << this->loopToRoot[ldi]->getLoopStructure()->getFunction()->getName() << "\n";
  }

  assert(heartbeatLoops.size() == this->loopToLevel.size() && heartbeatLoops.size() == this->loopToRoot.size() && "Number of loop information recorded doen't match");

  /*
   * Determine numLevels
   */
  uint64_t highestLevel = 0;
  for (auto pair : this->loopToLevel) {
    if (pair.second > highestLevel) {
      highestLevel = pair.second;
    }
  }
  this->numLevels = highestLevel + 1;
  errs() << this->outputPrefix << "numLevels: " << this->numLevels << "\n";

  /*
   * Determine root loop
   */
  this->rootLoop = (*(this->loopToRoot.begin())).second;
  errs() << this->outputPrefix << "root loop in function: " << this->rootLoop->getLoopStructure()->getFunction()->getName() << "\n";

  /*
   * Caller information
   */
  for (auto pair : this->loopToCallerLoop) {
    errs() << "loopFunction: " << pair.first->getLoopStructure()->getFunction()->getName() << " and caller function: " << pair.second->getLoopStructure()->getFunction()->getName() << "\n";
  }

  errs() << this->outputPrefix << "Loop level analysis completes\n";

  return;
}

void HeartBeat::setLoopLevelAndRoot(
    LoopDependenceInfo *ldi,
    std::unordered_map<LoopDependenceInfo *, CallGraphFunctionNode *> &loopToCallGraphNode,
    std::unordered_map<CallGraphFunctionNode *, LoopDependenceInfo *> &callGraphNodeToLoop,
    llvm::noelle::CallGraph &callGraph
) {

  auto calleeNode = loopToCallGraphNode[ldi];
  assert(calleeNode != nullptr && "Callee node not found for loop");
  auto calleefunction = calleeNode->getFunction();
  assert(calleefunction != nullptr && "Function not found for callee node");

  auto callerEdges = calleeNode->getIncomingEdges();
  assert(callerEdges.size() == 1 && "Outlined loop is called by multiple callers");

  auto callerNode = (*callerEdges.begin())->getCaller();
  assert(callerNode != nullptr && "Caller node not found");
  auto callerFunction = callerNode->getFunction();
  assert(callerFunction != nullptr && "Caller function not found");

  /*
   * Reach the top of root loop caller function
   */
  if (!callerFunction->getName().contains(this->functionSubString)) {
    assert(this->loopToLevel.find(ldi) == this->loopToLevel.end() && this->loopToRoot.find(ldi) == this->loopToRoot.end() && "The root loop has already been set before");
    this->loopToLevel[ldi] = 0;
    this->levelToLoop[0] = ldi;
    this->loopToRoot[ldi] = ldi;

    return;
  }

  /*
   * The caller is a HEARTBEAT_ suffix function
   * We're in a nested loop situation
   */
  auto callerLDI = callGraphNodeToLoop[callerNode];
  assert(callerLDI != nullptr && "Caller loop hasn't been set yet");
  
  // set the caller loop for this loop
  assert(this->loopToCallerLoop.find(ldi) == this->loopToCallerLoop.end() && "assumption is that heartbeat loop can only be called from only one caller\n"); 
  this->loopToCallerLoop[ldi] = callerLDI;

  /*
   * Information for the parent loop hasn't been recorded yet
   */
  if (this->loopToLevel.find(callerLDI) == this->loopToLevel.end()) {
    assert(this->loopToRoot.find(callerLDI) == this->loopToRoot.end() && "Caller loop doesn't have level info but have root info set");
    setLoopLevelAndRoot(callerLDI, loopToCallGraphNode, callGraphNodeToLoop, callGraph);
  }

  /*
   * We are the nested loop and already have the parent loop information recorded
   */
  this->loopToLevel[ldi] = this->loopToLevel[callerLDI] + 1;
  this->levelToLoop[this->loopToLevel[callerLDI] + 1] = ldi;
  this->loopToRoot[ldi] = this->loopToRoot[callerLDI];

  return;
}