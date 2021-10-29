#include "Pass.hpp"

using namespace llvm;
using namespace llvm::noelle;

HeartBeat::HeartBeat () 
  : ModulePass(ID) {
  return ;
}

bool HeartBeat::doInitialization (Module &M) {
  return false;
}

bool HeartBeat::runOnModule (Module &M) {
  auto modified = false;
  errs() << "Heartbeat pass\n";

  /*
   * Fetch NOELLE.
   */
  auto& noelle = getAnalysis<Noelle>();

  /*
   * Fetch all program loops
   */
  auto loops = noelle.getLoopStructures();

  /*
   * Select the loops to parallelize
   */
  auto selectedLoops = this->selectLoopsToTransform(noelle, *loops);

  /*
   * Parallelize the selected loop.
   */
  for (auto loop : selectedLoops){
    modified |= this->parallelizeLoop(noelle, loop);
  }

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
