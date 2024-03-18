#pragma once

#include "llvm/Pass.h"
#include "noelle/core/Noelle.hpp"
#include "noelle/core/Architecture.hpp"
#include "noelle/tools/DOALL.hpp"
#include "HeartbeatRuntimeManager.hpp"
#include "LoopNestAnalysis.hpp"
#include "LoopSliceTask.hpp"
#include "HeartbeatLoopEnvironmentBuilder.hpp"
#include "HeartbeatLoopEnvironmentUser.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

enum class HBTVerbosity { Disabled, Enabled };

class HeartbeatTransformation : public DOALL {
public:
  HeartbeatTransformation(
    Noelle *noelle,
    HeartbeatRuntimeManager *hbrm,
    LoopNestAnalysis *lna,
    uint64_t nestID,
    LoopContent *lc,
    std::unordered_map<LoopContent *, std::unordered_set<Value *>> &loopToSkippedLiveIns,
    std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> &loopToConstantLiveIns,
    std::unordered_map<int, int> &constantLiveInsArgIndexToIndex
  );

  bool apply(LoopContent *lc, Heuristics *h) override;

private:
  HBTVerbosity verbose;
  StringRef outputPrefix;
  Noelle *noelle;
  HeartbeatRuntimeManager *hbrm;
  LoopNestAnalysis *lna;
  uint64_t nestID;
  LoopContent *lc;

  uint64_t valuesInCacheLine;
  uint64_t liveInEnvIndex;
  uint64_t liveOutEnvIndex;
  FunctionType *loopSliceTaskSignature;
  LoopSliceTask *lsTask;
  Value *LSTContextBitCastInst;

  /*
   * Loop-slice task generation.
   */
  void createLoopSliceTaskFromOriginalLoop(LoopContent *lc);

  /*
   * TODO: refactorization
   * Loop closure generation.
   */
  void generateLoopClosure(LoopContent *lc);
  void initializeEnvironmentBuilder(
    LoopContent *LDI,
    std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeReduced,
    std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeSkipped,
    std::function<bool(uint32_t variableID, bool isLiveOut)> isConstantLiveInVariable
  );
  void initializeLoopEnvironmentUsers() override;
  void generateCodeToLoadLiveInVariables(LoopContent *LDI, int taskIndex) override;
  void setReducableVariablesToBeginAtIdentityValue(LoopContent *LDI, int taskIndex) override;
  void generateCodeToStoreLiveOutVariables(LoopContent *LDI, int taskIndex) override;
  void allocateNextLevelReducibleEnvironmentInsideTask(LoopContent *LDI, int taskIndex);

  std::unordered_map<LoopContent *, std::unordered_set<Value *>> &loopToSkippedLiveIns;
  std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> &loopToConstantLiveIns;
  std::unordered_map<int, int> &constantLiveInsArgIndexToIndex;
  std::unordered_map<uint32_t, Value *> liveOutVariableToAccumulatedPrivateCopy;

  /*
   * Loop iterations parameterization.
   */
  void parameterizeLoopIterations(LoopContent *lc);

  /*
   * Promotion handler insertion.
   */
  void insertPromotionHandler(LoopContent *lc);

};

} // namespace arcana::heartbeat
