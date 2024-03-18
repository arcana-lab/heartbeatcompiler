#pragma once

#include "llvm/Pass.h"
#include "noelle/core/Noelle.hpp"
#include "HeartbeatRuntimeManager.hpp"
#include "LoopNestAnalysis.hpp"
#include "HeartbeatTransformation.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

enum class Verbosity { Disabled, Enabled };

class Heartbeat : public ModulePass {
public:
  static char ID;

  Heartbeat();

  bool doInitialization(Module &M) override;

  bool runOnModule(Module &M) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override;

private:
  Verbosity verbose;
  StringRef outputPrefix;
  StringRef functionPrefix;
  Noelle *noelle;
  HeartbeatRuntimeManager *hbrm;
  LoopNestAnalysis *lna;
  std::unordered_map<LoopContent *, HeartbeatTransformation *> loopToHeartbeatTransformation;

  std::set<LoopContent *> selectHeartbeatLoops(
    const std::vector<LoopStructure *> *allLoops
  );

  void parallelizeLoopNest(uint64_t nestID);
  void parallelizeLoop(uint64_t nestID, LoopContent *lc);

};

} // namespace arcana::heartbeat
