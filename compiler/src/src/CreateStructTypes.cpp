#include "Pass.hpp"

void Heartbeat::createHBMemoryStructType(Noelle &noelle, bool Enable_Rollforward, bool Chunk_Loop_Iterations, bool Adaptive_Chunksize_Control) {

  auto tm = noelle.getTypesManager();

  std::vector<Type *> task_memory_type_fields = {
    tm->getIntegerType(64)  // uint64_t startingLevel
  };

  if (Chunk_Loop_Iterations) {
    task_memory_type_fields.push_back(tm->getIntegerType(64));  // chunksize
    task_memory_type_fields.push_back(tm->getIntegerType(64));  // remaining_chunksize
    task_memory_type_fields.push_back(tm->getIntegerType(1));   // has_remaining_chunksize
  }

  if (!Enable_Rollforward) {
    if (Chunk_Loop_Iterations) {
      if (Adaptive_Chunksize_Control) {
        task_memory_type_fields.push_back(tm->getIntegerType(64));  // polling_count
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
