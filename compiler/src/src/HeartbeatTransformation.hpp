/*
 * Copyright 2021   Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include "noelle/core/SystemHeaders.hpp"
#include "noelle/core/PDG.hpp"
#include "noelle/core/SCC.hpp"
#include "noelle/core/SCCDAG.hpp"
#include "noelle/core/PDGAnalysis.hpp"
#include "noelle/core/IVStepperUtility.hpp"
#include "noelle/core/LoopDependenceInfo.hpp"
#include "noelle/core/Noelle.hpp"
#include "noelle/tools/HeuristicsPass.hpp"
#include "noelle/tools/ParallelizationTechnique.hpp"
#include "noelle/tools/DOALL.hpp"

#include "HeartbeatLoopEnvironmentBuilder.hpp"
#include "HeartbeatLoopEnvironmentUser.hpp"
#include "HeartbeatTask.hpp"

using namespace llvm::noelle;

class HeartbeatTransformation : public DOALL {
  public:

    /*
     * Methods
     */
    HeartbeatTransformation (
      Noelle &noelle,
      uint64_t nestID,
      LoopDependenceInfo *ldi,
      uint64_t numLevels,
      bool containsLiveOut,
      std::unordered_map<LoopDependenceInfo *, uint64_t> &loopToLevel,
      std::unordered_map<LoopDependenceInfo *, std::unordered_set<Value *>> &loopToSkippedLiveIns,
      std::unordered_map<int, int> &constantLiveInsArgIndexToIndex,
      std::unordered_map<LoopDependenceInfo *, std::unordered_map<Value *, int>> &loopToConstantLiveIns,
      std::unordered_map<LoopDependenceInfo *, HeartbeatTransformation *> &loopToHeartbeatTransformation,
      std::unordered_map<LoopDependenceInfo *, LoopDependenceInfo *> &loopToCallerLoop,
      bool chunkLoopIterations,
      std::unordered_map<LoopDependenceInfo *, uint64_t> &loopToChunksize
    );

    bool apply (
      LoopDependenceInfo *ldi,
      Heuristics *h
    ) override ;

    inline HeartbeatTask *getHeartbeatTask() {
      assert(this->hbTask != nullptr && "hbTask hasn't been created yet\n");
      return this->hbTask;
    }

    inline Value * getContextBitCastInst() {
      return this->contextBitcastInst;
    }

    inline BasicBlock * getLoopHandlerBlock() {
      return this->loopHandlerBlock;
    }

    inline BasicBlock * getModifyExitConditionBlock() {
      return this->modifyExitConditionBlock;
    }

    inline CallInst * getCallToLoopHandler() {
      return this->callToLoopHandler;
    }

    inline PHINode * getReturnCodePhiInst() {
      return this->returnCodePhiInst;
    }

  protected:
    Noelle &n;
    uint64_t nestID;
    LoopDependenceInfo *ldi;
    uint64_t numLevels;
    bool containsLiveOut;
    std::unordered_map<LoopDependenceInfo *, uint64_t> &loopToLevel;
    std::unordered_map<LoopDependenceInfo *, std::unordered_set<Value *>> &loopToSkippedLiveIns;
    std::unordered_map<LoopDependenceInfo *, std::unordered_map<Value *, int>> &loopToConstantLiveIns;
    std::unordered_map<int, int> &constantLiveInsArgIndexToIndex;
    uint64_t valuesInCacheLine;

    /*
     * Helpers
     */
    void invokeHeartbeatFunctionAsideOriginalLoop (
        LoopDependenceInfo *LDI
        );

    void invokeHeartbeatFunctionAsideCallerLoop (
        LoopDependenceInfo *LDI
        );
    
    void executeLoopInChunk(
      LoopDependenceInfo *
    );

    void replaceAllUsesInsideLoopBody(LoopDependenceInfo *, Value *, Value *);

  private:
    void initializeEnvironmentBuilder(LoopDependenceInfo *LDI, 
                                      std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeReduced,
                                      std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeSkipped,
                                      std::function<bool(uint32_t variableID, bool isLiveOut)> isConstantLiveInVariable);
    void initializeLoopEnvironmentUsers() override;
    void generateCodeToLoadLiveInVariables(LoopDependenceInfo *LDI, int taskIndex) override;
    void setReducableVariablesToBeginAtIdentityValue(LoopDependenceInfo *LDI, int taskIndex) override;
    void generateCodeToStoreLiveOutVariables(LoopDependenceInfo *LDI, int taskIndex) override;
    void allocateNextLevelReducibleEnvironmentInsideTask(LoopDependenceInfo *LDI, int taskIndex);
    BasicBlock *performReductionAfterCallingLoopHandler(LoopDependenceInfo *LDI, int taskIndex, BasicBlock *loopHandlerBB, Instruction *cmpInst, BasicBlock *bottomHalfBB, Value *numOfReducerV);
    void allocateEnvironmentArray(LoopDependenceInfo *LDI) override;
    void allocateEnvironmentArrayInCallerTask(HeartbeatTask *callerHBTask);
    void populateLiveInEnvironment(LoopDependenceInfo *LDI) override;
    BasicBlock * performReductionWithInitialValueToAllReducibleLiveOutVariables(LoopDependenceInfo *LDI);

    FunctionType *sliceTaskSignature;
    PHINode *returnCodePhiInst;
    std::unordered_map<uint32_t, Value *> liveOutVariableToAccumulatedPrivateCopy;
    HeartbeatTask *hbTask;
    std::unordered_map<LoopDependenceInfo *, HeartbeatTransformation *> &loopToHeartbeatTransformation;
    std::unordered_map<LoopDependenceInfo *, LoopDependenceInfo *> &loopToCallerLoop;
    bool chunkLoopIterations;
    std::unordered_map<LoopDependenceInfo *, uint64_t> &loopToChunksize;
    Value *contextBitcastInst;
    BasicBlock *loopHandlerBlock;
    BasicBlock *modifyExitConditionBlock;
    CallInst *callToLoopHandler;
};
