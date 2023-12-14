#include "Pass.hpp"

using namespace arcana::noelle;

void Heartbeat::performLoopNestAnalysis (
  Noelle &noelle,
  const std::set<LoopContent *> &heartbeatLoops
) {

  errs() << this->outputPrefix << "Loop nest analysis starts\n";

  auto callGraph = noelle.getFunctionsManager()->getProgramCallGraph();
  assert(callGraph != nullptr && "Program call graph is not found");

  /*
   * Compute the callgraph function to corresponding loop
   */
  std::unordered_map<LoopContent *, CallGraphFunctionNode *> loopToCallGraphNode;
  std::unordered_map<CallGraphFunctionNode *, LoopContent *> callGraphNodeToLoop;
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
      continue;
    }

    /*
     * Set the level and root recursively
     */
    setLoopNestAndRoot(ldi, loopToCallGraphNode, callGraphNodeToLoop, *callGraph);
  }

  assert(heartbeatLoops.size() == this->loopToLevel.size() && heartbeatLoops.size() == this->loopToRoot.size() && "Number of loop information recorded doen't match");

  /*
   * Print loop nest info
   */
  for (auto pair : this->nestIDToLoops) {
    errs() << "nestID: " << pair.first << "\n";

    auto rootLoop = this->nestIDToRoot[pair.first];
    errs() << "\trootLoop: " << rootLoop->getLoopStructure()->getFunction()->getName() << "\n";

    for (auto loop : pair.second) {
      if (loop == rootLoop) {
        continue;
      }
      errs() << "\tlevel " << this->loopToLevel[loop] << ", " << loop->getLoopStructure()->getFunction()->getName() << "\n";
    }
  }

  errs() << this->outputPrefix << "Loop nest analysis completes\n";

  return;
}

void Heartbeat::setLoopNestAndRoot(
    LoopContent *ldi,
    std::unordered_map<LoopContent *, CallGraphFunctionNode *> &loopToCallGraphNode,
    std::unordered_map<CallGraphFunctionNode *, LoopContent *> &callGraphNodeToLoop,
    arcana::noelle::CallGraph &callGraph
) {

  auto calleeNode = loopToCallGraphNode[ldi];
  assert(calleeNode != nullptr && "Callee node not found for loop");
  auto calleefunction = calleeNode->getFunction();
  assert(calleefunction != nullptr && "Function not found for callee node");

  auto callerEdges = callGraph.getIncomingEdges(calleeNode);
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
    
    this->rootToNestID[ldi] = this->nestID;
    this->nestIDToRoot[this->nestID] = ldi;
    this->nestIDToLoops[this->nestID].insert(ldi);
    this->nestID++;

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
    setLoopNestAndRoot(callerLDI, loopToCallGraphNode, callGraphNodeToLoop, callGraph);
  }

  /*
   * We are the nested loop and already have the parent loop information recorded
   */
  this->loopToLevel[ldi] = this->loopToLevel[callerLDI] + 1;
  this->levelToLoop[this->loopToLevel[callerLDI] + 1] = ldi;
  this->loopToRoot[ldi] = this->loopToRoot[callerLDI];

  this->nestIDToLoops[this->rootToNestID[this->loopToRoot[ldi]]].insert(ldi);

  return;
}