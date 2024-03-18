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
#include "noelle/core/LoopContent.hpp"
#include "noelle/core/Noelle.hpp"
// #include "noelle/tools/HeuristicsPass.hpp"
#include "noelle/tools/ParallelizationTechnique.hpp"
#include "noelle/tools/DOALL.hpp"

#include "HeartbeatLoopEnvironmentBuilder.hpp"
#include "HeartbeatLoopEnvironmentUser.hpp"
#include "HeartbeatTask.hpp"

using namespace arcana::noelle;

class HeartbeatTransformation : public DOALL {
  public:

    /*
     * Methods
     */
    HeartbeatTransformation (
      Noelle &noelle,
      StructType *task_memory_t,
      uint64_t nestID,
      LoopContent *ldi,
      uint64_t numLevels,
      std::unordered_map<LoopContent *, uint64_t> &loopToLevel,
      std::unordered_map<LoopContent *, std::unordered_set<Value *>> &loopToSkippedLiveIns,
      std::unordered_map<int, int> &constantLiveInsArgIndexToIndex,
      std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> &loopToConstantLiveIns,
      std::unordered_map<LoopContent *, HeartbeatTransformation *> &loopToHeartbeatTransformation,
      std::unordered_map<LoopContent *, LoopContent *> &loopToCallerLoop
    );

    bool apply (
      LoopContent *ldi,
      Heuristics *h
    ) override ;

    inline HeartbeatTask *getHeartbeatTask() {
      assert(this->hbTask != nullptr && "hbTask hasn't been created yet\n");
      return this->hbTask;
    }

    inline HeartbeatLoopEnvironmentBuilder *getEnvBuilder() {
      return (HeartbeatLoopEnvironmentBuilder *)this->envBuilder;
    }

    inline Value * getContextBitCastInst() {
      return this->contextBitcastInst;
    }

    inline CallInst * getCallToHBResetFunction() {
      return this->callToHBResetFunction;
    }

    inline void eraseCallToHBResetFunction() {
      this->callToHBResetFunction->eraseFromParent();
    }

    inline BasicBlock * getPollingBlock() {
      return this->pollingBlock;
    }

    inline CallInst * getCallToPollingFunction() {
      return this->callToPollingFunction;
    }

    inline BasicBlock * getLoopHandlerBlock() {
      return this->loopHandlerBlock;
    }

    inline CallInst * getCallToLoopHandler() {
      return this->callToLoopHandler;
    }

    inline PHINode * getReturnCodePhiInst() {
      return this->returnCodePhiInst;
    }

  // protected:
    Noelle &n;
    StructType *task_memory_t;
    uint64_t nestID;
    LoopContent *ldi;
    uint64_t numLevels;
    std::unordered_map<LoopContent *, uint64_t> &loopToLevel;
    std::unordered_map<LoopContent *, std::unordered_set<Value *>> &loopToSkippedLiveIns;
    std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> &loopToConstantLiveIns;
    std::unordered_map<int, int> &constantLiveInsArgIndexToIndex;
    uint64_t valuesInCacheLine;
    uint64_t startIterationIndex =  0;
    uint64_t maxIterationIndex =    1;
    uint64_t liveInEnvIndex =       2;
    uint64_t liveOutEnvIndex =      3;

    /*
     * Helpers
     */
    void invokeHeartbeatFunctionAsideOriginalLoop (
        LoopContent *LDI
        );

    void invokeHeartbeatFunctionAsideCallerLoop (
        LoopContent *LDI
        );

  // private:
    void initializeEnvironmentBuilder(LoopContent *LDI, 
                                      std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeReduced,
                                      std::function<bool(uint32_t variableID, bool isLiveOut)> shouldThisVariableBeSkipped,
                                      std::function<bool(uint32_t variableID, bool isLiveOut)> isConstantLiveInVariable);
    void initializeLoopEnvironmentUsers() override;
    void generateCodeToLoadLiveInVariables(LoopContent *LDI, int taskIndex) override;
    void setReducableVariablesToBeginAtIdentityValue(LoopContent *LDI, int taskIndex) override;
    void generateCodeToStoreLiveOutVariables(LoopContent *LDI, int taskIndex) override;
    void allocateNextLevelReducibleEnvironmentInsideTask(LoopContent *LDI, int taskIndex);
    BasicBlock *performReductionAfterCallingLoopHandler(LoopContent *LDI, int taskIndex, BasicBlock *loopHandlerBB, Instruction *cmpInst, BasicBlock *bottomHalfBB, Value *numOfReducerV);
    void allocateEnvironmentArray(LoopContent *LDI) override;
    void allocateEnvironmentArrayInCallerTask(HeartbeatTask *callerHBTask);
    void populateLiveInEnvironment(LoopContent *LDI) override;
    BasicBlock * performReductionWithInitialValueToAllReducibleLiveOutVariables(LoopContent *LDI);

    FunctionType *sliceTaskSignature;
    PHINode *returnCodePhiInst;
    std::unordered_map<uint32_t, Value *> liveOutVariableToAccumulatedPrivateCopy;
    HeartbeatTask *hbTask;
    std::unordered_map<LoopContent *, HeartbeatTransformation *> &loopToHeartbeatTransformation;
    std::unordered_map<LoopContent *, LoopContent *> &loopToCallerLoop;

    AllocaInst *contextArrayAlloca;  // context array is allocated in the original root loop, this
                                      // field is available to the transformation of the root loop only
    Value *contextBitcastInst;
    Value *startIterationAddress;
    Value *startIteration;
    PHINode *currentIteration;
    Value *maxIterationAddress;
    Value *maxIteration;
    CallInst *callToHBResetFunction;
    BasicBlock *pollingBlock;
    CallInst *callToPollingFunction;
    BasicBlock *loopHandlerBlock;
    StoreInst *storeCurrentIterationAtBeginningOfHandlerBlock;
    CallInst *callToLoopHandler;
};
