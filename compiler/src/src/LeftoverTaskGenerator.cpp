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
    PointerType::getUnqual(tm->getIntegerType(64)),     // uint64_t *cxt
    PointerType::getUnqual(tm->getIntegerType(64)),     // uint64_t *constLiveIns
    tm->getIntegerType(64),                             // uint64_t myIndex
    PointerType::getUnqual(this->task_memory_t)         // tmem
  };

  // track the receivingLevel, splittingLevel pair for the leftoverTasks
  std::vector<std::pair<uint64_t, uint64_t>> leftoverTasksPairs;
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
      leftoverTasksPairs.push_back(std::pair(receivingLevel, splittingLevel));

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

      // iterating from receivingLevel to splittingLevel to create a block to invoke the loop slice
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

  // create entry block
  auto entryBlock = BasicBlock::Create(cxt, "entry", leftoverTaskIndexSelector);
  IRBuilder<> entryBlockBuilder{ entryBlock };

  // create exit block
  auto exitBlock = BasicBlock::Create(cxt, "exit", leftoverTaskIndexSelector);
  IRBuilder<> exitBlockBuilder{ exitBlock };

  // create br from entry to exit block
  auto exitBlockBr = entryBlockBuilder.CreateBr(exitBlock);

  if (this->leftoverTasks.size() == 1) {
    exitBlockBuilder.CreateRet(
      exitBlockBuilder.getInt64(0)
    );
  } else {
    // create return
    auto indexPHI = exitBlockBuilder.CreatePHI(
      exitBlockBuilder.getInt64Ty(),
      leftoverTasks.size(),
      "leftover_task_index"
    );
    exitBlockBuilder.CreateRet(indexPHI);

    IRBuilder<> *prevBuilder = &entryBlockBuilder;
    IRBuilder<> *currentBuilder = &entryBlockBuilder;
    Value *prevSelect;

    // have multiple leftover tasks
    // use leftoverTasksPairs to create blocks and determine control flow
    for (auto i = 0; i < this->leftoverTasks.size(); i++) {
      auto receivingLevel = leftoverTasksPairs[i].first;
      auto splittingLevel = leftoverTasksPairs[i].second;

      // create a block to determine the leftover task index
      auto leftoverTaskIndexBlock = BasicBlock::Create(
        cxt,
        std::string("leftover_").append(std::to_string(receivingLevel)).append("_").append(std::to_string(splittingLevel)).append("_index_block"),
        leftoverTaskIndexSelector
      );

      // fix the control flow by creating a condBr inst from the last basic
      if (i == 0) {
        // this is the first block
        entryBlockBuilder.CreateBr(leftoverTaskIndexBlock);
        exitBlockBr->eraseFromParent();
      } else {
        prevBuilder->CreateCondBr(prevSelect, exitBlock, leftoverTaskIndexBlock);
      }

      currentBuilder->SetInsertPoint(leftoverTaskIndexBlock);

      if (i != this->leftoverTasks.size()-1) {
        // create two icmp using function arguments
        auto firstCmp = currentBuilder->CreateICmpEQ(&*(leftoverTaskIndexSelector->arg_begin()), currentBuilder->getInt64(receivingLevel));
        auto secondCmp = currentBuilder->CreateICmpEQ(&*(leftoverTaskIndexSelector->arg_begin()+1), currentBuilder->getInt64(splittingLevel));

        // create selectInst
        // if firstCmp ? secondCmp : false == if firstCmp ? secondCmp : firstCmp
        auto selectInst = currentBuilder->CreateSelect(firstCmp, secondCmp, currentBuilder->getInt1(0));

        // update the prevSelect and preBuilder so the next block can correctly add the condBr inst
        prevSelect = selectInst;
        prevBuilder = currentBuilder;
      } else {
        // ths last index block jumps to the exit block directly
        currentBuilder->CreateBr(exitBlock);
      }

      // add the incoming value in the exit block
      indexPHI->addIncoming(exitBlockBuilder.getInt64(i), leftoverTaskIndexBlock);
    }
  }
  errs() << "leftover task index selector created\n" << *leftoverTaskIndexSelector << "\n";

  return;
}
