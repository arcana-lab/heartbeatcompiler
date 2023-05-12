#include "Pass.hpp"

/*
 * heartbeat_reset
 */
void Heartbeat::createHBResetFunction(Noelle &noelle) {
  auto tm = noelle.getTypesManager();

  std::vector<Type *> args{
    PointerType::getUnqual(this->heartbeat_memory_type),  // hbmem
    tm->getIntegerType(64)                                // startingLevel
  };

  FunctionType *funcType = FunctionType::get(tm->getVoidType(), args, false);
  Function::Create(
    funcType,
    GlobalValue::ExternalLinkage,
    "heartbeat_reset",
    *noelle.getProgram()
  );
}

/*
 * heartbeat_polling
 */
void Heartbeat::createPollingFunction(Noelle &noelle) {
  auto tm = noelle.getTypesManager();

  std::vector<Type *> args{
    PointerType::getUnqual(this->heartbeat_memory_type) // hbmem
  };

  FunctionType *funcType = FunctionType::get(tm->getIntegerType(1), args, false);
  Function::Create(
    funcType,
    GlobalValue::ExternalLinkage,
    "heartbeat_polling",
    *noelle.getProgram()
  );
}

/*
 * loop_handler
 */
void Heartbeat::createLoopHandlerFunction(Noelle &noelle) {
  auto tm = noelle.getTypesManager();

  std::vector<Type *> args{
    PointerType::getUnqual(tm->getIntegerType(64)),     // cxts
    PointerType::getUnqual(tm->getIntegerType(64)),     // constLiveIns
    tm->getIntegerType(64),                             // receivingLevel
    tm->getIntegerType(64),                             // numLevels
    PointerType::getUnqual(this->heartbeat_memory_type) // hbmem
  };
  // slice tasks
  FunctionType *sliceTasksType = FunctionType::get(
    tm->getIntegerType(64),
    ArrayRef<Type *>({
      PointerType::getUnqual(tm->getIntegerType(64)),
      PointerType::getUnqual(tm->getIntegerType(64)),
      tm->getIntegerType(64),
      PointerType::getUnqual(this->heartbeat_memory_type)
    }),
    false
  );
  args.push_back(PointerType::getUnqual(PointerType::getUnqual(sliceTasksType)));

  // leftover tasks
  FunctionType *leftoverTasksType = FunctionType::get(
    tm->getVoidType(),
    ArrayRef<Type *>({
      PointerType::getUnqual(tm->getIntegerType(64)),
      PointerType::getUnqual(tm->getIntegerType(64)),
      tm->getIntegerType(64),
      PointerType::getUnqual(this->heartbeat_memory_type)
    }),
    false
  );
  args.push_back(PointerType::getUnqual(PointerType::getUnqual(leftoverTasksType)));

  // leftover selector
  FunctionType *leftoverSelectorType = FunctionType::get(
    tm->getIntegerType(64),
    ArrayRef<Type *>({
      tm->getIntegerType(64),
      tm->getIntegerType(64)
    }),
    false
  );
  args.push_back(PointerType::getUnqual(leftoverSelectorType));

  FunctionType *funcType = FunctionType::get(tm->getIntegerType(64), args, false);
  Function::Create(
    funcType,
    GlobalValue::ExternalLinkage,
    "loop_handler",
    *noelle.getProgram()
  );
}

/*
 * __rf_handle_wrapper
 */
void Heartbeat::createRFHandlerFunction(Noelle &noelle) {
  auto tm = noelle.getTypesManager();

  std::vector<Type *> args{
    PointerType::getUnqual(tm->getIntegerType(64)),     // &rc
    PointerType::getUnqual(tm->getIntegerType(64)),     // cxts
    PointerType::getUnqual(tm->getIntegerType(64)),     // constLiveIns
    tm->getIntegerType(64),                             // receivingLevel
    tm->getIntegerType(64),                             // numLevels
    PointerType::getUnqual(this->heartbeat_memory_type) // hbmem
  };
  // slice tasks
  FunctionType *sliceTasksType = FunctionType::get(
    tm->getIntegerType(64),
    ArrayRef<Type *>({
      PointerType::getUnqual(tm->getIntegerType(64)),
      PointerType::getUnqual(tm->getIntegerType(64)),
      tm->getIntegerType(64),
      PointerType::getUnqual(this->heartbeat_memory_type)
    }),
    false
  );
  args.push_back(PointerType::getUnqual(PointerType::getUnqual(sliceTasksType)));

  // leftover tasks
  FunctionType *leftoverTasksType = FunctionType::get(
    tm->getVoidType(),
    ArrayRef<Type *>({
      PointerType::getUnqual(tm->getIntegerType(64)),
      PointerType::getUnqual(tm->getIntegerType(64)),
      tm->getIntegerType(64),
      PointerType::getUnqual(this->heartbeat_memory_type)
    }),
    false
  );
  args.push_back(PointerType::getUnqual(PointerType::getUnqual(leftoverTasksType)));

  // leftover selector
  FunctionType *leftoverSelectorType = FunctionType::get(
    tm->getIntegerType(64),
    ArrayRef<Type *>({
      tm->getIntegerType(64),
      tm->getIntegerType(64)
    }),
    false
  );
  args.push_back(PointerType::getUnqual(leftoverSelectorType));

  FunctionType *funcType = FunctionType::get(tm->getVoidType(), args, false);
  Function::Create(
    funcType,
    GlobalValue::ExternalLinkage,
    "__rf_handle_wrapper",
    *noelle.getProgram()
  );
}
