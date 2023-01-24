#pragma once

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "noelle/core/Noelle.hpp"
#include "noelle/tools/ParallelizationTechnique.hpp"

/* Interface for TPAL runtime 
 *   - assumptions: leaf loop (non nested), no reduction
 *   - return nonzero value if promotion happened, zero otherwise
 *   - indvar: loop induction variable, maxval: loop maximum value
 *   - ...
 *   int handler_for_fork2(int64_t indvar, int64_t maxval, 
 *                         void** livein, void(*f)(int64_t,int64_t,void**)) {
 *     ... here, we call a C++ function in the TPAL runtime
 *   }
 */

/*
 * Heartbeat Transformation
 * - assumptions: one nested loop structure
 * -              one loop at every level
 */

using namespace llvm ;
using namespace llvm::noelle ;

class HeartBeat : public ModulePass {
  public:
    static char ID; 

    HeartBeat();

    bool doInitialization (Module &M) override ;

    bool runOnModule (Module &M) override ;

    void getAnalysisUsage(AnalysisUsage &AU) const override ;

  private:
    StringRef outputPrefix;
    StringRef functionSubString;

    /*
     * Step 1: Identify all loops in functions starts with "HEARTBEAT_"
     */
    std::set<LoopDependenceInfo *> selectHeartbeatLoops (
      Noelle &noelle,
      const std::vector<LoopStructure *> *allLoops
    );

    /*
     * Step 2
     */
    void performLoopLevelAnalysis (
      Noelle &noelle,
      const std::set<LoopDependenceInfo *> &heartbeatLoops
    );

    void setLoopLevelAndRoot (
      LoopDependenceInfo *ldi,
      std::unordered_map<LoopDependenceInfo *, CallGraphFunctionNode *> &loopToCallGraphNode,
      std::unordered_map<CallGraphFunctionNode *, LoopDependenceInfo *> &callGraphNodeToLoop,
      llvm::noelle::CallGraph &callGraph
    );

    /*
     * Results for loop-level analysis
     */
    std::unordered_map<Function *, LoopDependenceInfo *> functionToLoop;
    std::unordered_map<LoopDependenceInfo *, uint64_t> loopToLevel;
    std::unordered_map<LoopDependenceInfo *, LoopDependenceInfo *> loopToRoot;
    LoopDependenceInfo *rootLoop = nullptr;
    uint64_t numLevels = 0;

    /*
     * Step 3
     */
    void handleLiveOut (
      Noelle &noelle,
      const std::set<LoopDependenceInfo *> &heartbeatLoops
    );

    bool containsLiveOut = false;

    /*
     * Step 4: Const live-in analysis
     */
    void performConstantLiveInAnalysis (
      Noelle &noelle,
      const std::set<LoopDependenceInfo *> &heartbeatLoops
    );

    void constantLiveInToLoop(
      llvm::Argument &arg,
      int arg_index,
      LoopDependenceInfo *ldi
    );

    bool isArgStartOrExitValue(
      llvm::Argument &arg,
      LoopDependenceInfo *ldi
    );

    void createConstantLiveInsGlobalPointer(
      Noelle &noelle
    );

    std::unordered_map<LoopDependenceInfo *, std::unordered_set<Value *>> loopToSkippedLiveIns;
    std::unordered_map<LoopDependenceInfo *, std::unordered_map<Value *, int>> loopToConstantLiveIns;
    std::unordered_set<int> constantLiveInsArgIndex;
    std::unordered_map<int, int> constantLiveInsArgIndexToIndex;

    /*
     * Step 5: parallelize loops into heartbeat form
     */
    bool parallelizeLoop (
      Noelle &noelle,
      LoopDependenceInfo *ldi
    );

    bool createHeartBeatLoop (
      Noelle &noelle,
      LoopDependenceInfo *ldi,
      ParallelizationTechnique **usedTechnique
      );

};
