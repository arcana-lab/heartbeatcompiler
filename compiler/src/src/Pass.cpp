#include "Pass.hpp"

using namespace llvm;
using namespace llvm::noelle;

static cl::list<std::string> Chunksize("chunksize", cl::desc("Specify the chunksize for each level of nested loop"), cl::OneOrMore);

Heartbeat::Heartbeat () 
  : ModulePass(ID) 
  , outputPrefix("Heartbeat: ")
  , functionSubString("HEARTBEAT_")
{
  return ;
}

bool Heartbeat::doInitialization (Module &M) {
  return false;
}

bool Heartbeat::runOnModule (Module &M) {
  auto modified = false;
  errs() << this->outputPrefix << "Start\n";

  /*
   * Fetch NOELLE and select Heartbeat loops
   */
  auto &noelle = getAnalysis<Noelle>();
  auto allLoops = noelle.getLoopStructures();
  auto heartbeatLoops = this->selectHeartbeatLoops(noelle, allLoops);
  errs() << this->outputPrefix << heartbeatLoops.size() << " loops will be parallelized\n";
  for (auto selectedLoop : heartbeatLoops) {
    errs() << this->outputPrefix << selectedLoop->getLoopStructure()->getFunction()->getName() << "\n";
  }

  /*
   * Determine the loop nest and all loops in that nest
   */
  this->performLoopNestAnalysis(noelle, heartbeatLoops);

  for (auto pair : this->nestIDToLoops) {
    this->reset();

    uint64_t nestID = pair.first;

    /*
     * Determine the level of the targeted Heartbeat loop, and the root loop for each targeted loop
     */
    this->performLoopLevelAnalysis(noelle, pair.second);

    /*
     * Set the chunksize using command line arguments
     */
    uint64_t index = 0;
    for (auto &chunksize : Chunksize) {
      this->loopToChunksize[this->levelToLoop[index]] = stoi(chunksize);
      index++;
    }

    /*
     * Determine if any Heartbeat loop contains live-out variables
     */
    this->handleLiveOut(noelle, pair.second);

    /*
     * Constant live-in analysis to determine the set of constant live-ins of each loop
     */
    this->performConstantLiveInAnalysis(noelle, pair.second);

    /*
     * Create constant live-ins global pointer
     */
    this->createConstantLiveInsGlobalPointer(noelle, nestID);

    /*
     * Parallelize the selected loop.
     */
    this->parallelizeRootLoop(noelle, nestID, this->rootLoop);
    if (modified){
      errs() << this->outputPrefix << "  The root loop has been modified\n";
    }

    for (auto loop : pair.second) {
      if (loop == this->rootLoop) {
        // root loop has already been parallelized, skip it
        continue;
      } else {
        modified |= this->parallelizeNestedLoop(noelle, nestID, loop);
      }
    }

    errs() << "parallelization completed!\n";
    errs() << "original root loop after parallelization" << *(this->rootLoop->getLoopStructure()->getFunction()) << "\n";
    errs() << "cloned root loop after parallelization" << *(this->loopToHeartbeatTransformation[rootLoop]->getHeartbeatTask()->getTaskBody()) << "\n";
    for (auto pair : this->loopToHeartbeatTransformation) {
      if (pair.first == this->rootLoop) {
        continue;
      }
      errs() << "cloned loop of level " << this->loopToLevel[pair.first] << *(pair.second->getHeartbeatTask()->getTaskBody()) << "\n";
    }

    if (this->numLevels > 1) {
      // all Heartbeat loops are generated,
      // create the slices wrapper function and the global
      createSliceTasksWrapper(noelle, nestID);

      // now we've created all the Heartbeat loops, it's time to create the leftover tasks per gap between Heartbeat loops
      createLeftoverTasks(noelle, nestID, pair.second);

      // all leftover tasks have been created
      // 1. need to fix the call to loop_handler to use the leftoverTasks global
      // 2. need to fix the call to loop_handler to use the leftoverTaskIndexSelector
      for (auto pair : this->loopToHeartbeatTransformation) {
        auto callInst = cast<CallInst>(pair.second->getCallToLoopHandler());

        IRBuilder<> builder{ callInst };
        auto sliceTasksWrapperGEP = builder.CreateInBoundsGEP(
          noelle.getProgram()->getNamedGlobal(std::string("sliceTasksWrapper_nest").append(std::to_string(nestID))),
          ArrayRef<Value *>({
            builder.getInt64(0),
            builder.getInt64(0),
          })
        );
        auto leftoverTasksGEP = builder.CreateInBoundsGEP(
          noelle.getProgram()->getNamedGlobal(std::string("leftoverTasks_nest").append(std::to_string(nestID))),
          ArrayRef<Value *>({
            builder.getInt64(0),
            builder.getInt64(0)
          })
        );

        callInst->setArgOperand(2, sliceTasksWrapperGEP);
        callInst->setArgOperand(3, leftoverTasksGEP);
        callInst->setArgOperand(4, this->leftoverTaskIndexSelector);
        errs() << "updated loop_handler function in loop level " << this->loopToLevel[pair.first] << *callInst << "\n";
      }
    }
  }

  errs() << this->outputPrefix << "Exit\n";
  return modified;
}

void Heartbeat::reset() {
  this->functionToLoop.clear();
  this->loopToLevel.clear();
  this->levelToLoop.clear();
  this->loopToRoot.clear();
  this->loopToCallerLoop.clear();
  this->rootLoop = nullptr;
  this->numLevels = 0;

  this->containsLiveOut = false;

  this->loopToSkippedLiveIns.clear();
  this->loopToConstantLiveIns.clear();
  this->constantLiveInsArgIndex.clear();
  this->constantLiveInsArgIndexToIndex.clear();

  this->loopToHeartbeatTransformation.clear();

  this->sliceTasksWrapper.clear();

  this->leftoverTasks.clear();
  this->leftoverTaskIndexSelector = nullptr;

  this->loopToChunksize.clear();
}

void Heartbeat::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<Noelle>(); // so that LLVM knows this pass depends on noelle
}

// Next there is code to register your pass to "opt"
char Heartbeat::ID = 0;
static RegisterPass<Heartbeat> X("heartbeat", "Heartbeat transformation");

// Next there is code to register your pass to "clang"
static Heartbeat * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new Heartbeat());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new Heartbeat()); }}); // ** for -O0
