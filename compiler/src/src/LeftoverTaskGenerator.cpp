#include "Pass.hpp"

using namespace llvm::noelle;

void Heartbeat::createLeftoverTasks(
  Noelle &noelle,
  uint64_t nestID,
  std::set<LoopDependenceInfo *> &heartbeatLoops
) {

  auto tm = noelle.getTypesManager();
  auto &cxt = noelle.getProgram()->getContext();

  // leftover task type
  std::vector<Type *> leftoverTaskTypes{ 
    tm->getVoidPointerType(),   // void *cxt
    tm->getIntegerType(64),     // uint64_t myIndex
    tm->getVoidPointerType()    // void *itersArr
  };

  for (auto receivingLoop : heartbeatLoops) {
    // no leftover task generated for root loop
    if (this->loopToLevel[receivingLoop] == 0) {
      continue;
    }

    for (auto splittingLoop : heartbeatLoops) {
      // only consider splittingLoops from a lower nesting level
      if (this->loopToLevel[splittingLoop] >= this->loopToLevel[receivingLoop]) {
        continue;
      }

      // create the leftover task from receivingLoop to splittingLoop
      auto receivingLevel = this->loopToLevel[receivingLoop];
      auto splittingLevel = this->loopToLevel[splittingLoop];
      errs() << "Creating leftover task from " << receivingLevel << " to " << splittingLevel << "\n";

      auto leftoverTaskCallee = noelle.getProgram()->getOrInsertFunction(
        std::string("HEARTBEAT_nest")
          .append(std::to_string(nestID))
          .append("_loop_")
          .append(std::to_string(receivingLevel))
          .append("_")
          .append(std::to_string(splittingLevel))
          .append("_leftover"),
        FunctionType::get(
          tm->getVoidType(),
          ArrayRef<Type *>({
            leftoverTaskTypes
          }),
          false /* isVarArg */
        )
      );
      auto leftoverTask = cast<Function>(leftoverTaskCallee.getCallee());
      this->leftoverTasks.push_back(leftoverTask);

      // create entry block and cast the iterationsArray to the correct type
      auto entryBlock = BasicBlock::Create(cxt, "entry", leftoverTask);
      IRBuilder<> entryBlockBuilder{ entryBlock };
      auto iterationsArrayArgIndex = 2;
      auto iterationsArray = entryBlockBuilder.CreateBitCast(
        cast<Value>(&*(leftoverTask->arg_begin() + iterationsArrayArgIndex)),
        PointerType::getUnqual(ArrayType::get(
          entryBlockBuilder.getInt64Ty(),
          (receivingLevel + 1) * 2  /* every level contains startIteration and maxIteration */
        )),
        "iterationsArray"
      );

      // create exit block and return void
      auto exitBlock = BasicBlock::Create(cxt, "exit", leftoverTask);
      IRBuilder<> exitBlockBuilder{ exitBlock };
      exitBlockBuilder.CreateRet(
        nullptr
      );
      errs() << "leftover task created" << *leftoverTask << "\n";

      // map from level to start/max iteration
      std::unordered_map<uint64_t, Value *> levelToStartIteration;
      std::unordered_map<uint64_t, Value *> levelToMaxIteration;
      std::unordered_map<uint64_t, BasicBlock *> levelToInvokingBlock;

      // iterating from splittingLevel to receivingLevel to collect start/max iteration
      for (uint64_t level = receivingLevel; level >= splittingLevel && level <= receivingLevel; level--) {
        auto int64Ty = entryBlockBuilder.getInt64Ty();

        // load startIteration from the iterationsArray
        auto startIterationGEPInst = entryBlockBuilder.CreateInBoundsGEP(
          iterationsArray->getType()->getPointerElementType(),
          iterationsArray,
          ArrayRef<Value *>({
            entryBlockBuilder.getInt64(0),
            entryBlockBuilder.getInt64(level * 2),  // index of startIteration
          })
        );
        auto startIteration = entryBlockBuilder.CreateLoad(
          int64Ty,
          startIterationGEPInst,
          std::string("startIteration_").append(std::to_string(level))
        );

        // load maxIteration from the iterationsArray
        auto maxIterationGEPInst = entryBlockBuilder.CreateInBoundsGEP(
          iterationsArray->getType()->getPointerElementType(),
          iterationsArray,
          ArrayRef<Value *>({
            entryBlockBuilder.getInt64(0),
            entryBlockBuilder.getInt64(level * 2 + 1),  // index of maxIteration
          })
        );
        auto maxIteration = entryBlockBuilder.CreateLoad(
          int64Ty,
          maxIterationGEPInst,
          std::string("maxIteration_").append(std::to_string(level))
        );

        // register start/max iteration to the level of loop
        levelToStartIteration[level] = startIteration;
        levelToMaxIteration[level] = maxIteration;

        // create a block to invoke the loop slice corresponding to level
        auto invokingBlock = BasicBlock::Create(cxt, std::string("loop_").append(std::to_string(level)), leftoverTask);
        IRBuilder<> invokingBlockBuilder{ invokingBlock };

        // create a branch to the exit block
        invokingBlockBuilder.CreateBr(
          exitBlock
        );

        // register invoking block to the level of loop
        levelToInvokingBlock[level] = invokingBlock;
      }

      // create branch from entry block to the invoking block at receivingLevel
      entryBlockBuilder.CreateBr(levelToInvokingBlock[receivingLevel]);

      // iterating from receivingLevel to splittingLevel to invoke the loop slice
      // here use two constraints since we're using uint64_t
      for (uint64_t level = receivingLevel; level >= splittingLevel && level <= receivingLevel; level--) {
        auto invokingBlock = levelToInvokingBlock[level];
        auto invokingBlockTerminator = invokingBlock->getTerminator();
        IRBuilder<> invokingBlockBuilder { invokingBlockTerminator };

        // find the slice task at the corresponding level
        auto loop = this->levelToLoop[level];
        auto hbTask = this->loopToHeartbeatTransformation[loop]->getHeartbeatTask();
        assert(hbTask != nullptr && "No Heartbeat task found for this loop\n");

        // create the call to the heartbeat loop
        std::vector<Value *> loopSliceParameters{ 
          &*(leftoverTask->arg_begin()),  // void *cxts
          &*(leftoverTask->arg_begin()+1) // myIndex
        };
        // put 0 as start/max iteration starting from level [0 to splittingLevel)
        for (auto i = 0; i < splittingLevel; i++) {
          loopSliceParameters.push_back(invokingBlockBuilder.getInt64(0));
          loopSliceParameters.push_back(invokingBlockBuilder.getInt64(0));
        }
        // use the start/max iteration previous generated starting from [splittingLevel to receivingLevel - 1]
        for (auto i = splittingLevel; i < level; i++) {
          loopSliceParameters.push_back(levelToStartIteration[i]);
          loopSliceParameters.push_back(levelToMaxIteration[i]);
        }
        if (level == receivingLevel) {
          // use our own start/max iterations
          loopSliceParameters.push_back(levelToStartIteration[level]);
          loopSliceParameters.push_back(levelToMaxIteration[level]);
        } else {
          // use startIter + 1
          loopSliceParameters.push_back(
            invokingBlockBuilder.CreateAdd(
              levelToStartIteration[level],
              invokingBlockBuilder.getInt64(1),
              std::string("startIteration_").append(std::to_string(level)).append("_plus_one")
            )
          );
          loopSliceParameters.push_back(levelToMaxIteration[level]);
        }
        auto sliceTaskCallInst = invokingBlockBuilder.CreateCall(
          hbTask->getTaskBody(),
          ArrayRef<Value *>({
            loopSliceParameters
          })
        );

        if (level != splittingLevel) {
          // exit the leftover task if rc > 0
          invokingBlockBuilder.CreateCondBr(
            invokingBlockBuilder.CreateICmpSGT(
              sliceTaskCallInst,
              invokingBlockBuilder.getInt64(0)
            ),
            exitBlock,
            levelToInvokingBlock[level - 1]
          );
          invokingBlockTerminator->eraseFromParent();
        }
      }

      errs() << "leftover task from receivingLevel " << receivingLevel << " to splittingLevel " << splittingLevel << "\n";
      errs() << *leftoverTask << "\n";
    }
  }

  // now all leftover tasks have been generated, initialize the leftoverTasks global array
  auto leftoverTasksType = ArrayType::get(
    PointerType::getUnqual(
      FunctionType::get(
        tm->getVoidType(),
        ArrayRef<Type *>({
          leftoverTaskTypes
        }),
        false
      )
    ),
    this->leftoverTasks.size()
  );
  std::string leftoverTasksGlobalName = std::string("leftoverTasks_nest").append(std::to_string(nestID));
  noelle.getProgram()->getOrInsertGlobal(
    leftoverTasksGlobalName,
    leftoverTasksType
  );
  auto leftoverTasksGlobal = noelle.getProgram()->getNamedGlobal(leftoverTasksGlobalName);
  leftoverTasksGlobal->setAlignment(Align(8));
  leftoverTasksGlobal->setInitializer(
    ConstantArray::get(
      leftoverTasksType,
      ArrayRef<Constant *>({
        this->leftoverTasks
      })
    )
  );
  errs() << "created global leftoverTasks " << *leftoverTasksGlobal << "\n";

  // now leftoverTasks global array has been generated, create a leftover selector function
  // that returns the index of the leftover task giving receivingLevel and splittingLevel
  std::vector<Type *> leftoverTaskIndexSelectorTypes{
    tm->getIntegerType(64), // uint64_t receivingLevel
    tm->getIntegerType(64)  // uint64_t splittingLevel
  };

  auto leftoverTaskIndexSelectorCallee = noelle.getProgram()->getOrInsertFunction(
    std::string("leftover_task_index_selector_nest").append(std::to_string(nestID)),
    FunctionType::get(
      tm->getIntegerType(64),
      ArrayRef<Type *>({
        leftoverTaskIndexSelectorTypes
      }),
      false /* isVarArg */
    )
  );
  this->leftoverTaskIndexSelector = cast<Function>(leftoverTaskIndexSelectorCallee.getCallee());

  // create exit block
  auto exitBlock = BasicBlock::Create(cxt, "exit", leftoverTaskIndexSelector);
  IRBuilder <> exitBlockBuilder{ exitBlock };

  if (this->leftoverTasks.size() == 1) {
    exitBlockBuilder.CreateRet(
      exitBlockBuilder.getInt64(0)
    );
  } else {
    // TODO
    // when there're multiple leftover tasks, given the fact
    // that the loop nest we're considering have at least level 3
    // need to differentiate them
  }
  errs() << "leftover task index selector created" << *leftoverTaskIndexSelector << "\n";

  return;
}
