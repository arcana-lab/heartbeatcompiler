#pragma once

#include "llvm/Pass.h"
#include "noelle/core/Noelle.hpp"
#include "noelle/core/Task.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

class LoopSliceTask : public Task {
public:
  LoopSliceTask(
    Module *module,
    FunctionType *loopSliceTaskSignature,
    std::string &loopSliceTaskName
  );

  inline Value * getTaskMemoryPointerArg() { return this->taskMemoryPointerArg; };
  inline Value * getTaskIndexArg() { return this->taskIndexArg; };
  inline Value * getLSTContextPointerArg() { return this->LSTContextPointerArg; };
  inline Value * getInvariantsPointerArg() { return this->invariantsPointerArg; };

private:
  Value *taskMemoryPointerArg;
  Value *taskIndexArg;
  Value *LSTContextPointerArg;
  Value *invariantsPointerArg;

};

} // namespace arcana::heartbeat
