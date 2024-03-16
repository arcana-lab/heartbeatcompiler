#include "HeartbeatRuntimeManager.hpp"

using namespace llvm;
using namespace arcana::noelle;

namespace arcana::heartbeat {

static cl::opt<int> HBRMVerbose(
    "heartbeat-runtime-manager-verbose",
    cl::ZeroOrMore,
    cl::Hidden,
    cl::desc("Heartbeat Runtime Manager verbose output (0: disabled, 1: enabled)"));

HeartbeatRuntimeManager::HeartbeatRuntimeManager(
  Noelle *noelle
) : outputPrefix("Heartbeat Runtime Manager: "),
    noelle(noelle) {
  /*
   * Fetch command line options.
   */
  this->verbose = static_cast<HBRMVerbosity>(HBRMVerbose.getValue());

  if (this->verbose > HBRMVerbosity::Disabled) {
    errs() << this->outputPrefix << "Start\n";
  }

  this->createTaskMemoryStructType();

  if (this->verbose > HBRMVerbosity::Disabled) {
    errs() << this->outputPrefix << "End\n";
  }

  return;
}

void HeartbeatRuntimeManager::createTaskMemoryStructType() {
  auto tm = this->noelle->getTypesManager();

  std::vector<Type *> taskMemoryStructFieldTypes = {
    tm->getIntegerType(64),   // starting_level
    tm->getIntegerType(64),   // chunk_size
    tm->getIntegerType(64),   // remaining_chunk_size
    tm->getIntegerType(64),   // polling_count
  };

  this->taskMemoryStructType = StructType::create(
    this->noelle->getProgramContext(),
    taskMemoryStructFieldTypes,
    "task_memory_t"
  );

  if (this->verbose > HBRMVerbosity::Disabled) {
    errs() << this->outputPrefix << "Create task memory struct\n";
    errs() << this->outputPrefix << "  \"" << *this->taskMemoryStructType << "\"\n";
  }

  return;
}

} // namespace arcana::heartbeat
