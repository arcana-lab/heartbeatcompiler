#include "HeartbeatTransformation.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

void HeartbeatTransformation::chunkLoopIterations(LoopContent *lc) {


  if (this->verbose > HBTVerbosity::Disabled) {
    errs() << this->outputPrefix << "loop-slice task after chunking loop iterations\n";
    errs() << *this->lsTask->getTaskBody() << "\n";
  }

  return;
}

} // namespace arcana::heartbeat
