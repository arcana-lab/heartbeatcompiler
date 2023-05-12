#include "Pass.hpp"

void Heartbeat::createHBMemoryStructType(Noelle &noelle, bool Enable_Rollforward, bool Chunk_Loop_Iterations, bool Adaptive_Chunksize_Control) {

  auto tm = noelle.getTypesManager();

  std::vector<Type *> heartbeat_memory_type_fields = {
    tm->getIntegerType(64)  // uint64_t startingLevel
  };

  if (!Enable_Rollforward) {
    if (Chunk_Loop_Iterations) {
      if (Adaptive_Chunksize_Control) {
        heartbeat_memory_type_fields.push_back(tm->getIntegerType(64));
      }
    }
  }

  this->heartbeat_memory_type = StructType::create(
    noelle.getProgramContext(),
    heartbeat_memory_type_fields,
    "heartbeat_memory_t"
  );

  return;
}
