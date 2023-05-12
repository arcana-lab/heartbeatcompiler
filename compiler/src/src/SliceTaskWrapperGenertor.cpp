#include "Pass.hpp"

using namespace llvm::noelle;

void Heartbeat::createSliceTasks(
  Noelle &noelle,
  uint64_t nestID
) {

  auto tm = noelle.getTypesManager();
  auto &cxt = noelle.getProgram()->getContext();

  // wrapper function type
  std::vector<Type *> sliceTaskType{
    PointerType::getUnqual(tm->getIntegerType(64)),     // uint64_t *cxt
    PointerType::getUnqual(tm->getIntegerType(64)),     // uint64_t *constLiveIns
    tm->getIntegerType(64),                             // uint64_t myIndex
    PointerType::getUnqual(this->heartbeat_memory_type) // hbmem
  };

  for (uint64_t level = 0; level < this->numLevels; level++) {
    // find the slice task at the corresponding level
    auto loop = this->levelToLoop[level];
    auto hbTask = this->loopToHeartbeatTransformation[loop]->getHeartbeatTask();
    assert(hbTask != nullptr && "No Heartbeat task found for this loop\n");

    this->sliceTasks.push_back(hbTask->getTaskBody());
  }

  // initialize the sliceTasks global array
  auto sliceTasksType = ArrayType::get(
    PointerType::getUnqual(
      FunctionType::get(
        tm->getIntegerType(64),
        ArrayRef<Type *>({
          sliceTaskType
        }),
        false
      )
    ),
    this->sliceTasks.size()
  );
  std::string sliceTasksName = std::string("sliceTasks_nest").append(std::to_string(nestID));
  noelle.getProgram()->getOrInsertGlobal(
    sliceTasksName,
    sliceTasksType
  );
  auto sliceTasksGlobal = noelle.getProgram()->getNamedGlobal(sliceTasksName);
  sliceTasksGlobal->setAlignment(Align(8));
  sliceTasksGlobal->setInitializer(
    ConstantArray::get(
      sliceTasksType,
      ArrayRef<Constant *>({
        this->sliceTasks
      })
    )
  );
  errs() << "created global slice tasks\n" << *sliceTasksGlobal << "\n";

  return;
}