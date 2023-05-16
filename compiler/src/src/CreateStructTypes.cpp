#include "Pass.hpp"

void Heartbeat::createHBMemoryStructType(Noelle &noelle, bool Enable_Rollforward, bool Chunk_Loop_Iterations, bool Adaptive_Chunksize_Control) {

  auto tm = noelle.getTypesManager();

  std::vector<Type *> task_memory_type_fields = {
    tm->getIntegerType(64)  // uint64_t startingLevel
  };

  if (!Enable_Rollforward) {
    if (Chunk_Loop_Iterations) {
      if (Adaptive_Chunksize_Control) {
        task_memory_type_fields.push_back(tm->getIntegerType(64));
      }
    }
  }

  this->task_memory_t = StructType::create(
    noelle.getProgramContext(),
    task_memory_type_fields,
    "task_memory_t"
  );

  return;
}
