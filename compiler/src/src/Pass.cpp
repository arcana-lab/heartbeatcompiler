#include "Pass.hpp"

using namespace llvm;
using namespace llvm::noelle;

static cl::list<std::string> Chunksize("chunksize", cl::desc("Specify the chunksize for each level of nested loop"), cl::OneOrMore);

HeartBeat::HeartBeat () 
  : ModulePass(ID) 
  , outputPrefix("HeartBeat: ")
  , functionSubString("HEARTBEAT_")
{
  return ;
}

bool HeartBeat::doInitialization (Module &M) {
  return false;
}

bool HeartBeat::runOnModule (Module &M) {
  auto modified = false;
  errs() << this->outputPrefix << "Start\n";

  /*
   * Fetch NOELLE and select heartbeat loops
   */
  auto &noelle = getAnalysis<Noelle>();
  auto allLoops = noelle.getLoopStructures();
  auto heartbeatLoops = this->selectHeartbeatLoops(noelle, allLoops);
  errs() << this->outputPrefix << heartbeatLoops.size() << " loops will be parallelized\n";
  for (auto selectedLoop : heartbeatLoops) {
    errs() << this->outputPrefix << selectedLoop->getLoopStructure()->getFunction()->getName() << "\n";
  }

  /*
   * Determine the level of the targeted heartbeat loop, and the root loop for each targeted loop
   */
  this->performLoopLevelAnalysis(noelle, heartbeatLoops);

  /*
   * Set the chunksize using command line arguments
   */
  uint64_t index = 0;
  for (auto &chunksize : Chunksize) {
    this->loopToChunksize[this->levelToLoop[index]] = stoi(chunksize);
    index++;
  }

  /*
   * Determine if any heartbeat loop contains live-out variables
   */
  this->handleLiveOut(noelle, heartbeatLoops);

  /*
   * Constant live-in analysis to determine the set of constant live-ins of each loop
   */
  this->performConstantLiveInAnalysis(noelle, heartbeatLoops);

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

  for (auto loop : heartbeatLoops) {
    if (loop == this->rootLoop) {
      // root loop has already been parallelized, skip it
      continue;
    } else {
      modified |= this->parallelizeNestedLoop(noelle, loop);
    }
  }

  errs() << "parallelization completed!\n";
  errs() << "original root loop after parallelization" << *(this->rootLoop->getLoopStructure()->getFunction()) << "\n";
  errs() << "cloned root loop after parallelization" << *(this->loopToHeartBeatTransformation[rootLoop]->getHeartBeatTask()->getTaskBody()) << "\n";
  for (auto pair : this->loopToHeartBeatTransformation) {
    if (pair.first == this->rootLoop) {
      continue;
    }
    errs() << "cloned loop of level " << this->loopToLevel[pair.first] << *(pair.second->getHeartBeatTask()->getTaskBody()) << "\n";
  }

  // now we've created all the heartbeat loops, it's time to create the leftover tasks per gap between heartbeat loops
  modified |= createLeftoverTasks(noelle, heartbeatLoops);

  // all leftover tasks have been created, need to fix the call to loop_handler to use the leftoverTasks global
  for (auto pair : this->loopToHeartBeatTransformation) {
    auto callInst = cast<CallInst>(pair.second->getCallToLoopHandler());

    IRBuilder<> builder{ callInst };
    auto leftoverTasksGEP = builder.CreateInBoundsGEP(
      noelle.getProgram()->getNamedGlobal("leftoverTasks"),
      ArrayRef<Value *>({
        builder.getInt64(0),
        builder.getInt64(0)
      })
    );

    callInst->setArgOperand(2, leftoverTasksGEP);
    errs() << "updated loop_handler function in loop level " << this->loopToLevel[pair.first] << *callInst << "\n";
  }

  errs() << this->outputPrefix << "Exit\n";
  return modified;
}

void HeartBeat::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<Noelle>(); // so that LLVM knows this pass depends on noelle
}

// Next there is code to register your pass to "opt"
char HeartBeat::ID = 0;
static RegisterPass<HeartBeat> X("heartbeat", "Heartbeat transformation");

// Next there is code to register your pass to "clang"
static HeartBeat * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new HeartBeat());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new HeartBeat()); }}); // ** for -O0
