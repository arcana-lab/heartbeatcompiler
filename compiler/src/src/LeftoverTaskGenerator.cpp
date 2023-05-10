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
    PointerType::getUnqual(tm->getIntegerType(64)),   // uint64_t *cxt
    PointerType::getUnqual(tm->getIntegerType(64)),   // uint64_t *constLiveIns
    tm->getIntegerType(64),                           // uint64_t startingLevel
    tm->getIntegerType(64)                            // uint64_t myIndex
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

      // create entry block and cast the cxts to the correct type
      auto entryBlock = BasicBlock::Create(cxt, "entry", leftoverTask);
      IRBuilder<> entryBlockBuilder{ entryBlock };
      auto contextArrayCasted = entryBlockBuilder.CreateBitCast(
        cast<Value>(&*(leftoverTask->arg_begin())),
        PointerType::getUnqual(this->loopToHeartbeatTransformation[receivingLoop]->getEnvBuilder()->getContextArrayType()),
        "contextArray"
      );

      // create exit block and return void
      auto exitBlock = BasicBlock::Create(cxt, "exit", leftoverTask);
      IRBuilder<> exitBlockBuilder{ exitBlock };
      exitBlockBuilder.CreateRet(
        nullptr
      );
      errs() << "leftover task created\n" << *leftoverTask << "\n";

      // map from level invoking block
      std::unordered_map<uint64_t, BasicBlock *> levelToInvokingBlock;

      // iterating from splittingLevel to receivingLevel to create a block to invoke the loop slice
      for (uint64_t level = receivingLevel; level >= splittingLevel && level <= receivingLevel; level--) {
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
      // here use two constraints in the for loop since we're using uint64_t
      for (uint64_t level = receivingLevel; level >= splittingLevel && level <= receivingLevel; level--) {
        auto invokingBlock = levelToInvokingBlock[level];
        auto invokingBlockTerminator = invokingBlock->getTerminator();
        IRBuilder<> invokingBlockBuilder { invokingBlockTerminator };

        // find the slice task at the corresponding level
        auto loop = this->levelToLoop[level];
        auto hbTransformation = this->loopToHeartbeatTransformation[loop];
        assert(hbTransformation != nullptr && "No Heartbeat transformation found for this loop\n");
        auto hbTask = hbTransformation->getHeartbeatTask();
        assert(hbTask != nullptr && "No Heartbeat task found for this loop\n");

        // create the call to the heartbeat loop
        std::vector<Value *> loopSliceParameters{ 
          &*(leftoverTask->arg_begin()),    // *cxts
          &*(leftoverTask->arg_begin()+1),  // *constLiveIns
          &*(leftoverTask->arg_begin()+2),  // startingLevel
          &*(leftoverTask->arg_begin()+3)   // myIndex
        };
        if (level != receivingLevel) {
          // store startIter + 1
          auto startIterAddr = invokingBlockBuilder.CreateInBoundsGEP(
            hbTransformation->getEnvBuilder()->getContextArrayType(),
            contextArrayCasted,
            ArrayRef<Value *>({
              invokingBlockBuilder.getInt64(0),
              invokingBlockBuilder.getInt64(this->loopToLevel[loop] * hbTransformation->valuesInCacheLine + hbTransformation->startIterationIndex)
            }),
            std::string("startIteration_").append(std::to_string(level)).append("_addr")
          );
          auto startIter = invokingBlockBuilder.CreateLoad(
            invokingBlockBuilder.getInt64Ty(),
            startIterAddr,
            std::string("startIteration_").append(std::to_string(level))
          );
          auto startIterPlusOne = invokingBlockBuilder.CreateAdd(
            startIter,
            invokingBlockBuilder.getInt64(1),
            std::string("startIteration_").append(std::to_string(level)).append("_plusone")
          );
          invokingBlockBuilder.CreateStore(
            startIterPlusOne,
            startIterAddr
          );
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
  errs() << "created global leftoverTasks\n" << *leftoverTasksGlobal << "\n";

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
  errs() << "leftover task index selector created\n" << *leftoverTaskIndexSelector << "\n";

  return;
}
