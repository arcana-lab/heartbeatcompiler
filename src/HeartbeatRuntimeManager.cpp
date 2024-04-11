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
  this->createGetChunkSizeFunction();
  this->createHasRemainingChunkSizeFunction();
  this->createHeartbeatPollingFunction();

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

void HeartbeatRuntimeManager::createGetChunkSizeFunction() {
  auto tm = this->noelle->getTypesManager();

  std::vector<Type *> getChunkSizeFunctionArguments = {
    PointerType::getUnqual(this->taskMemoryStructType)  // *tmem
  };

  auto getChunkSizeFunctionType = FunctionType::get(
    tm->getIntegerType(64),
    getChunkSizeFunctionArguments,
    false
  );

  this->getChunkSizeFunction = Function::Create(
    getChunkSizeFunctionType,
    GlobalValue::ExternalLinkage,
    "get_chunk_size",
    *this->noelle->getProgram()
  );

  if (this->verbose > HBRMVerbosity::Disabled) {
    errs() << this->outputPrefix << "Create get_chunk_size function\n";
    errs() << this->outputPrefix << "  \"" << *this->getChunkSizeFunction << "\"\n";
  }

  return;
}

void HeartbeatRuntimeManager::createHasRemainingChunkSizeFunction() {
  auto tm = this->noelle->getTypesManager();

  std::vector<Type *> hasRemainingChunkSizeFunctionArguments = {
    PointerType::getUnqual(this->taskMemoryStructType),   // *tmem
    tm->getIntegerType(64),                               // iterations_executed
    tm->getIntegerType(64)                                // chunk_size
  };

  auto hasRemainingChunkSizeFunctionType = FunctionType::get(
    tm->getIntegerType(1),
    hasRemainingChunkSizeFunctionArguments,
    false
  );

  this->hasRemainingChunkSizeFunction = Function::Create(
    hasRemainingChunkSizeFunctionType,
    GlobalValue::ExternalLinkage,
    "has_remaining_chunk_size",
    *this->noelle->getProgram()
  );

  return;
}

void HeartbeatRuntimeManager::createHeartbeatPollingFunction() {
  auto tm = this->noelle->getTypesManager();

  std::vector<Type *> heartbeatPollingFunctionArguments = {
    PointerType::getUnqual(this->taskMemoryStructType)  // *tmem
  };

  auto heartbeatPollingFunctionType = FunctionType::get(
    tm->getIntegerType(1),
    heartbeatPollingFunctionArguments,
    false
  );

  this->heartbeatPollingFunction = Function::Create(
    heartbeatPollingFunctionType,
    GlobalValue::ExternalLinkage,
    "heartbeat_polling",
    *this->noelle->getProgram()
  );

  if (this->verbose > HBRMVerbosity::Disabled) {
    errs() << this->outputPrefix << "Create heartbeat_polling function\n";
    errs() << this->outputPrefix << "  \"" << *this->getChunkSizeFunction << "\"\n";
  }

  return;
}

} // namespace arcana::heartbeat
