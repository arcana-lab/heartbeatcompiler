#include "Pass.hpp"

using namespace llvm;
using namespace llvm::noelle;

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
   * Determine if any heartbeat loop contains live-out variables
   */
  this->handleLiveOut(noelle, heartbeatLoops);

  /*
   * Constant live-in analysis to determine the set of constant live-ins of each loop
   */
  this->performConstantLiveInAnalysis(noelle, heartbeatLoops);

  /*
   * Parallelize the selected loop.
   */
  for (auto loop : heartbeatLoops){
    modified |= this->parallelizeLoop(noelle, loop);
  }
  if (modified){
    errs() << this->outputPrefix << "  The code has been modified\n";
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
