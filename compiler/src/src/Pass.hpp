#pragma once

#include "HeartBeatTransformation.hpp"
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
    std::unordered_map<uint64_t, LoopDependenceInfo *> levelToLoop;
    std::unordered_map<LoopDependenceInfo *, LoopDependenceInfo *> loopToRoot;
    std::unordered_map<LoopDependenceInfo *, LoopDependenceInfo *> loopToCallerLoop;
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
     * Step 5: parallelize root loop into heartbeat form
     */
    bool parallelizeRootLoop (
      Noelle &noelle,
      LoopDependenceInfo *ldi
    );

    void linkTransformedLoopToOriginalFunction(
      BasicBlock *originalPreHeader,
      BasicBlock *startOfParLoopInOriginalFunc,
      BasicBlock *endOfParLoopInOriginalFunc,
      Value *envArray,
      Value *envIndexForExitVariable,
      std::vector<BasicBlock *> &loopExitBlocks,
      llvm::noelle::TypesManager *tm,
      Noelle &noelle);

    /*
     * Step 6: parallelize all nested loops under the root loop
     */
    bool parallelizeNestedLoop (
      Noelle &noelle,
      LoopDependenceInfo *ldi
    );

    bool createHeartBeatLoop (
      Noelle &noelle,
      LoopDependenceInfo *ldi,
      ParallelizationTechnique **usedTechnique
      );

    std::unordered_map<LoopDependenceInfo *, HeartBeatTransformation *> loopToHeartBeatTransformation;

    /*
     * Step 7: create leftover tasks
     */
    bool createLeftoverTasks(
      Noelle &noelle,
      std::set<LoopDependenceInfo *> &heartbeatLoops
    );

    std::vector<Constant *> leftoverTasks;

};
