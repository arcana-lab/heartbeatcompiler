#include "Pass.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

static cl::opt<int> Verbose(
    "heartbeat-verbose",
    cl::ZeroOrMore,
    cl::Hidden,
    cl::desc("Heartbeat verbose output (0: disabled, 1: enabled)"));

Heartbeat::Heartbeat() : ModulePass(ID), outputPrefix("Heartbeat: "), functionPrefix("HEARTBEAT_") {
  return;
}

bool Heartbeat::doInitialization(Module &M) {
  /*
   * Fetch command line options.
   */
  this->verbose = static_cast<Verbosity>(Verbose.getValue());

  return false;
}

bool Heartbeat::runOnModule(Module &M) {
  if (this->verbose > Verbosity::Disabled) {
    errs() << this->outputPrefix << "Start\n";
  }

  /*
   * Identify all Heartbeat loops.
   */
  this->noelle = &getAnalysis<Noelle>();
  auto allLoops = noelle->getLoopStructures();
  auto heartbeatLoops = this->selectHeartbeatLoops(allLoops);
  if (heartbeatLoops.size() == 0) {
    return false;
  }

  /*
   * Create Heartbeat runtime manager
   */
  this->hbrm = new HeartbeatRuntimeManager(this->noelle);

  /*
   * Perform loop nest analysis.
   */
  this->lna = new LoopNestAnalysis(this->functionPrefix, this->noelle, heartbeatLoops);

  /*
   * Perform Heartbeat transformation per loop nest.
   */
  for (uint64_t i = 0; i < lna->getNumLoopNests(); i++) {
    this->parallelizeLoopNest(i);
  }

  if (this->verbose > Verbosity::Disabled) {
    errs() << this->outputPrefix << "End\n";
  }
  return true;
}

void Heartbeat::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<Noelle>();

  return;
}

/*
 * Next there is code to register your pass to "opt".
 */
char Heartbeat::ID = 0;
static RegisterPass<Heartbeat> X("heartbeat", "The Heartbeat Compiler");

/*
 * Next there is code to register your pass to "clang".
 */
static Heartbeat * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new Heartbeat());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new Heartbeat()); }}); // ** for -O0

} // namespace arcana::heartbeat
