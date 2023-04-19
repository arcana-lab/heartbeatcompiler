#include "Pass.hpp"

using namespace llvm::noelle;

void Heartbeat::createSliceTasksWrapper(
  Noelle &noelle,
  uint64_t nestID
) {

  auto tm = noelle.getTypesManager();
  auto &cxt = noelle.getProgram()->getContext();

  // wrapper function type
  std::vector<Type *> sliceTaskWrapperTypes{
    tm->getVoidPointerType(),   // void *cxt
    tm->getIntegerType(64),     // uint64_t myIndex
    tm->getIntegerType(64),     // uint64_t startIter
    tm->getIntegerType(64)      // uint64_t maxIter
  };

  for (uint64_t level = 0; level < this->numLevels; level++) {
    // find the slice task at the corresponding level
    auto loop = this->levelToLoop[level];
    auto hbTask = this->loopToHeartbeatTransformation[loop]->getHeartbeatTask();
    assert(hbTask != nullptr && "No Heartbeat task found for this loop\n");

    // create the slice task wrapper function
    auto sliceTaskWrapperCallee = noelle.getProgram()->getOrInsertFunction(
      std::string(hbTask->getTaskBody()->getName()).append("_wrapper"),
      FunctionType::get(
        tm->getVoidType(),
        ArrayRef<Type *>({
          sliceTaskWrapperTypes
        }),
        false /* isVarArg */
      )
    );
    auto sliceTaskWrapper = cast<Function>(sliceTaskWrapperCallee.getCallee());
    this->sliceTasksWrapper.push_back(sliceTaskWrapper);

    // create a block to invoke the loop slice
    auto invokingBlock = BasicBlock::Create(cxt, "invoking_block", sliceTaskWrapper);
    IRBuilder<> invokingBlockBuilder{ invokingBlock };

    // create the call to the loop slice
    auto argIter = sliceTaskWrapper->arg_begin();
    std::vector<Value *> loopSliceParameters{
      &*(argIter++),    // void *cxt
      &*(argIter++)     // uint64_t myIndex
    };
    // supply 0 as start and max iteration from 0 to level - 1
    for (uint64_t i = 0; i < level; i++) {
      loopSliceParameters.push_back(invokingBlockBuilder.getInt64(0));
      loopSliceParameters.push_back(invokingBlockBuilder.getInt64(0));
    }
    // supply my start and max iterations from the parameters
    loopSliceParameters.push_back(&*(argIter++));
    loopSliceParameters.push_back(&*(argIter++));
    invokingBlockBuilder.CreateCall(
      hbTask->getTaskBody(),
      ArrayRef<Value *>({
        loopSliceParameters
      })
    );

    // return void
    invokingBlockBuilder.CreateRet(
      nullptr
    );

    errs() << "slice task wrapper of level " << level << "\n" << *sliceTaskWrapper << "\n";
  }

  // now all slice tasks wrapper have been generated, initialize the sliceTasksWrapper global array
  auto sliceTasksWrapperType = ArrayType::get(
    PointerType::getUnqual(
      FunctionType::get(
        tm->getVoidType(),
        ArrayRef<Type *>({
          sliceTaskWrapperTypes
        }),
        false
      )
    ),
    this->sliceTasksWrapper.size()
  );
  std::string sliceTasksWrapperName = std::string("sliceTasksWrapper_nest").append(std::to_string(nestID));
  noelle.getProgram()->getOrInsertGlobal(
    sliceTasksWrapperName,
    sliceTasksWrapperType
  );
  auto sliceTasksWrapperGlobal = noelle.getProgram()->getNamedGlobal(sliceTasksWrapperName);
  sliceTasksWrapperGlobal->setAlignment(Align(8));
  sliceTasksWrapperGlobal->setInitializer(
    ConstantArray::get(
      sliceTasksWrapperType,
      ArrayRef<Constant *>({
        this->sliceTasksWrapper
      })
    )
  );
  errs() << "created global slice tasks wrapper\n" << *sliceTasksWrapperGlobal << "\n";

  return;
}