#pragma once

#include "HeartbeatTransformation.hpp"
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
using namespace arcana::noelle ;

class Heartbeat : public ModulePass {
  public:
    static char ID; 

    Heartbeat();

    bool doInitialization (Module &M) override ;

    bool runOnModule (Module &M) override ;

    void getAnalysisUsage(AnalysisUsage &AU) const override ;

  private:
    StringRef outputPrefix;
    StringRef functionSubString;

    void createHBMemoryStructType(Noelle &noelle, bool Enable_Rollforward, bool Chunk_Loop_Iterations, bool Adaptive_Chunksize_Control);
    StructType *task_memory_t;
    void createHBStartFunction(Noelle &noelle);
    void createPollingFunction(Noelle &noelle);
    void createLoopHandlerFunction(Noelle &noelle);
    void createRFHandlerFunction(Noelle &noelle);
    void createGetChunksizeFunction(Noelle &noelle);
    void createUpdateAndHasRemainingChunksizeFunction(Noelle &noelle);
    void createHasRemainingChunksizeFunction(Noelle &noelle);

    /*
     * Step 1: Identify all loops in functions starts with "Heartbeat_"
     */
    std::set<LoopContent *> selectHeartbeatLoops (
      Noelle &noelle,
      const std::vector<LoopStructure *> *allLoops
    );

    /*
     * Step 1.5: Loop nest analysis, identify all loops grouped per loop nest
     */
    void performLoopNestAnalysis (
      Noelle &noelle,
      const std::set<LoopContent *> &heartbeatLoops
    );

    void setLoopNestAndRoot (
      LoopContent *ldi,
      std::unordered_map<LoopContent *, CallGraphFunctionNode *> &loopToCallGraphNode,
      std::unordered_map<CallGraphFunctionNode *, LoopContent *> &callGraphNodeToLoop,
      arcana::noelle::CallGraph &callGraph
    );

    /*
     * Results for loop-nest analysis
     */
    uint64_t nestID = 0;
    std::unordered_map<LoopContent *, uint64_t> rootToNestID;
    std::unordered_map<uint64_t, LoopContent *> nestIDToRoot;
    std::unordered_map<uint64_t, std::set<LoopContent *>> nestIDToLoops;

    void reset();

    /*
     * Step 2
     */
    void performLoopLevelAnalysis (
      Noelle &noelle,
      const std::set<LoopContent *> &heartbeatLoops
    );

    void setLoopLevelAndRoot (
      LoopContent *ldi,
      std::unordered_map<LoopContent *, CallGraphFunctionNode *> &loopToCallGraphNode,
      std::unordered_map<CallGraphFunctionNode *, LoopContent *> &callGraphNodeToLoop,
      arcana::noelle::CallGraph &callGraph
    );

    /*
     * Results for loop-level analysis
     */
    std::unordered_map<Function *, LoopContent *> functionToLoop;
    std::unordered_map<LoopContent *, uint64_t> loopToLevel;
    std::unordered_map<uint64_t, LoopContent *> levelToLoop;
    std::unordered_map<LoopContent *, LoopContent *> loopToRoot;
    std::unordered_map<LoopContent *, LoopContent *> loopToCallerLoop;
    LoopContent *rootLoop = nullptr;
    uint64_t numLevels = 0;

    /*
     * Step 3
     */
    void handleLiveOut (
      Noelle &noelle,
      const std::set<LoopContent *> &heartbeatLoops
    );

    bool containsLiveOut = false;

    /*
     * Step 4: Const live-in analysis
     */
    void performConstantLiveInAnalysis (
      Noelle &noelle,
      const std::set<LoopContent *> &heartbeatLoops
    );

    void constantLiveInToLoop(
      llvm::Argument &arg,
      int arg_index,
      LoopContent *ldi
    );

    bool isArgStartOrExitValue(
      llvm::Argument &arg,
      LoopContent *ldi
    );

    std::unordered_map<LoopContent *, std::unordered_set<Value *>> loopToSkippedLiveIns;
    std::unordered_map<LoopContent *, std::unordered_map<Value *, int>> loopToConstantLiveIns;
    std::unordered_set<int> constantLiveInsArgIndex;
    std::unordered_map<int, int> constantLiveInsArgIndexToIndex;

    /*
     * Step 5: parallelize root loop into Heartbeat form
     */
    bool parallelizeRootLoop (
      Noelle &noelle,
      uint64_t nestID,
      LoopContent *ldi
    );

    void linkTransformedLoopToOriginalFunction(
      BasicBlock *originalPreHeader,
      BasicBlock *startOfParLoopInOriginalFunc,
      BasicBlock *endOfParLoopInOriginalFunc,
      Value *envArray,
      Value *envIndexForExitVariable,
      std::vector<BasicBlock *> &loopExitBlocks,
      TypesManager *tm,
      Noelle &noelle);

    /*
     * Step 6: parallelize all nested loops under the root loop
     */
    bool parallelizeNestedLoop (
      Noelle &noelle,
      uint64_t nestID,
      LoopContent *ldi
    );

    bool createHeartbeatLoop (
      Noelle &noelle,
      LoopContent *ldi,
      ParallelizationTechnique **usedTechnique
      );

    std::unordered_map<LoopContent *, HeartbeatTransformation *> loopToHeartbeatTransformation;

    /*
     * Chunking transformation
     */
    void executeLoopInChunk(
      Noelle &noelle,
      LoopContent *,
      HeartbeatTransformation *,
      bool isLeafLoop
    );

    void replaceAllUsesInsideLoopBody(LoopContent *, HeartbeatTransformation *, Value *, Value *);

    bool chunkLoopIterations = false;
    uint64_t chunksize;

    /*
     * Step 7: create slice tasks wrapper
     */
    void createSliceTasks(
      Noelle &noelle,
      uint64_t nestID
    );

    std::vector<Constant *> sliceTasks;

    /*
     * Step 8: create leftover tasks
     */
    void createLeftoverTasks(
      Noelle &noelle,
      uint64_t nestID,
      std::set<LoopContent *> &heartbeatLoops
    );

    std::vector<Constant *> leftoverTasks;
    Function *leftoverTaskIndexSelector = nullptr;

    /*
     * Replace call to rollforward_handler if Enable_Rollforward is specified
     */
    void replaceWithRollforwardHandler(Noelle &noelle);
    void eraseHBResetCall();

    /*
     * Disable heartbeat transformation
     */
    void replaceBrToPollingBlockToLatchBlock(HeartbeatTransformation *hbt);

};
