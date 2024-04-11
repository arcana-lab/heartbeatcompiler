#pragma once

#include "llvm/Pass.h"
#include "noelle/core/Noelle.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

enum class HBRMVerbosity { Disabled, Enabled };

class HeartbeatRuntimeManager {
public:
  HeartbeatRuntimeManager(
    Noelle *noelle
  );

  inline StructType * getTaskMemoryStructType() { return this->taskMemoryStructType; };

private:
  HBRMVerbosity verbose;
  StringRef outputPrefix;
  Noelle *noelle;

  StructType *taskMemoryStructType;
  Function *getChunkSizeFunction;
  Function *hasRemainingChunkSizeFunction;
  Function *heartbeatPollingFunction;

  void createTaskMemoryStructType();
  void createGetChunkSizeFunction();
  void createHasRemainingChunkSizeFunction();
  void createHeartbeatPollingFunction();

};

} // namespace arcana::heartbeat
