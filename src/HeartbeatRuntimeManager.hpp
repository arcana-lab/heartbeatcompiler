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

  void createTaskMemoryStructType();
  void createGetChunkSizeFunction();

};

} // namespace arcana::heartbeat
