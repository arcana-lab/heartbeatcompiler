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
  auto HeartbeatLoops = this->selectHeartbeatLoops(noelle, allLoops);
  errs() << this->outputPrefix << HeartbeatLoops.size() << " loops will be parallelized\n";
  for (auto selectedLoop : HeartbeatLoops) {
    errs() << this->outputPrefix << selectedLoop->getLoopStructure()->getFunction()->getName() << "\n";
  }

  /*
   * Determine the level of the targeted Heartbeat loop, and the root loop for each targeted loop
   */
  this->performLoopLevelAnalysis(noelle, HeartbeatLoops);

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
  this->handleLiveOut(noelle, HeartbeatLoops);

  /*
   * Constant live-in analysis to determine the set of constant live-ins of each loop
   */
  this->performConstantLiveInAnalysis(noelle, HeartbeatLoops);

  /*
   * Create constant live-ins global pointer
   */
  this->createConstantLiveInsGlobalPointer(noelle);

  /*
   * Parallelize the selected loop.
   */
  this->parallelizeRootLoop(noelle, this->rootLoop);
  if (modified){
    errs() << this->outputPrefix << "  The root loop has been modified\n";
  }

  for (auto loop : HeartbeatLoops) {
    if (loop == this->rootLoop) {
      // root loop has already been parallelized, skip it
      continue;
    } else {
      modified |= this->parallelizeNestedLoop(noelle, loop);
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
    createSliceTasksWrapper(noelle);

    // now we've created all the Heartbeat loops, it's time to create the leftover tasks per gap between Heartbeat loops
    createLeftoverTasks(noelle, HeartbeatLoops);

    // all leftover tasks have been created
    // 1. need to fix the call to loop_handler to use the leftoverTasks global
    // 2. need to fix the call to loop_handler to use the leftoverTaskIndexSelector
    for (auto pair : this->loopToHeartbeatTransformation) {
      auto callInst = cast<CallInst>(pair.second->getCallToLoopHandler());

      IRBuilder<> builder{ callInst };
      auto sliceTasksWrapperGEP = builder.CreateInBoundsGEP(
        noelle.getProgram()->getNamedGlobal("sliceTasksWrapper"),
        ArrayRef<Value *>({
          builder.getInt64(0),
          builder.getInt64(0),
        })
      );
      auto leftoverTasksGEP = builder.CreateInBoundsGEP(
        noelle.getProgram()->getNamedGlobal("leftoverTasks"),
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

  errs() << this->outputPrefix << "Exit\n";
  return modified;
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
