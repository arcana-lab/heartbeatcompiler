#pragma once

#include "llvm/Pass.h"
#include "noelle/core/Noelle.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

enum class LNAVerbosity { Disabled, Enabled };

typedef struct {
  uint64_t nestID;
  uint64_t level;
  uint64_t index;
} LoopID;

class LoopNestAnalysis {
public:
  LoopNestAnalysis(
    StringRef functionPrefix,
    Noelle *noelle,
    std::set<LoopContent *> heartbeatLoops
  );

private:
  LNAVerbosity verbose;
  StringRef outputPrefix;
  StringRef functionPrefix;
  Noelle *noelle;
  std::set<LoopContent *> heartbeatLoops;

  arcana::noelle::CallGraph *callGraph;
  std::unordered_map<LoopContent *, CallGraphFunctionNode *> loopToCallGraphNode;
  std::unordered_map<CallGraphFunctionNode *, LoopContent *> callGraphNodeToLoop;
  uint64_t nestID = 0;
  std::unordered_map<LoopContent *, uint64_t> rootLoopToNestID;
  std::unordered_map<uint64_t, LoopContent *> nestIDToRootLoop;
  std::unordered_map<uint64_t, std::set<LoopContent *>> nestIDToLoops;
  std::unordered_map<LoopContent *, LoopID> loopToLoopID;

  void performLoopNestAnalysis();
  void buildLoopToCallGraphNodeMapping();
  void collectRootLoops();
  void assignLoopIDs();
  void assignLoopIDsCalledByLoop(LoopContent *callerLC, LoopID &callerLID);

};

} // namespace arcana::heartbeat
