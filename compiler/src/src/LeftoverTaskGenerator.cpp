#include "Pass.hpp"

using namespace llvm::noelle;

bool HeartBeat::createLeftoverTasks(
  Noelle &noelle,
  std::set<LoopDependenceInfo *> &heartbeatLoops
) {

  auto tm = noelle.getTypesManager();
  auto &cxt = noelle.getProgram()->getContext();
  // leftover task type
  std::vector<Type *> leftoverTaskTypes{ tm->getVoidPointerType() };  // void *cxt
  if (this->containsLiveOut) {
    leftoverTaskTypes.push_back(tm->getIntegerType(64));  // myIndex
  }
  leftoverTaskTypes.push_back(tm->getVoidPointerType());

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
        std::string("HEARTBEAT_loop_")
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
      auto iterationsArrayArgIndex = this->containsLiveOut ? 2 : 1;
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
      std::vector<BasicBlock *> invokingBlocks;

      // iterating from receivingLevel to splittingLevel + 1
      for (auto level = receivingLevel; level > splittingLevel; level--) {
        // load startIteration from the iterationsArray
        auto startIterationGEPInst = entryBlockBuilder.CreateInBoundsGEP(
          iterationsArray,
          ArrayRef<Value *>({
            entryBlockBuilder.getInt64(0),
            entryBlockBuilder.getInt64(level * 2),  // index of startIteration
          })
        );
        auto startIteration = entryBlockBuilder.CreateLoad(
          startIterationGEPInst,
          std::string("startIteration_").append(std::to_string(level))
        );

        // load maxIteration from the iterationsArray
        auto maxIterationGEPInst = entryBlockBuilder.CreateInBoundsGEP(
          iterationsArray,
          ArrayRef<Value *>({
            entryBlockBuilder.getInt64(0),
            entryBlockBuilder.getInt64(level * 2 + 1),  // index of maxIteration
          })
        );
        auto maxIteration = entryBlockBuilder.CreateLoad(
          maxIterationGEPInst,
          std::string("maxIteration_").append(std::to_string(level))
        );

        // register it to the level of loop
        levelToStartIteration[level] = startIteration;
        levelToMaxIteration[level] = maxIteration;

        // create a block to invoke the loop slice corresponding to level
        auto invokingBlock = BasicBlock::Create(cxt, std::string("loop_").append(std::to_string(level)), leftoverTask);
        IRBuilder<> invokingBlockBuilder{ invokingBlock };

        // find the slice task at the corresponding level
        auto loop = this->levelToLoop[level];
        auto hbTask = this->loopToHeartBeatTransformation[loop]->getHeartBeatTask();
        assert(hbTask != nullptr && "No Heartbeat task found for this loop\n");

        // Create the call to the heartbeat loop
        std::vector<Value *> loopSliceParameters{ &*(leftoverTask->arg_begin()) }; // void *cxt
        if (this->containsLiveOut) {
          loopSliceParameters.push_back(&*(leftoverTask->arg_begin() + 1));  // myIndex
        }
        // put 0 as start/max iteration starting from level [0 to splittingLevel]
        for (auto i = 0; i <= splittingLevel; i++) {
          loopSliceParameters.push_back(invokingBlockBuilder.getInt64(0));
          loopSliceParameters.push_back(invokingBlockBuilder.getInt64(0));
        }
        // use the start/max iteration previous generated starting from [splittingLevel + 1 to receivingLevel]
        for (auto i = splittingLevel + 1; i <= level; i++) {
          loopSliceParameters.push_back(levelToStartIteration[i]);
          loopSliceParameters.push_back(levelToMaxIteration[i]);
        }
        invokingBlockBuilder.CreateCall(
          hbTask->getTaskBody(),
          ArrayRef<Value *>({
            loopSliceParameters
          })
        );

        // now the call to the loop slice has been created, creating a branch to the exit block
        invokingBlockBuilder.CreateBr(
          exitBlock
        );
        invokingBlocks.push_back(invokingBlock);  // register this invoking block as the order of invocation needs to be fixed next

        // errs() << "leftover task after loading start/max iteration" << *leftoverTask << "\n";
      }

      // all invoking blocks has been created, need to fix the control flow to correctly invoke them
      assert(invokingBlocks.size() >= 1 && "no invoking blocks created");
      entryBlockBuilder.CreateBr(
        *(invokingBlocks.begin())
      );

      for (auto i = 0; i < invokingBlocks.size() - 1; i++) {
        auto previousTerminator = invokingBlocks[i]->getTerminator();
        IRBuilder<> builder{ previousTerminator };
        builder.CreateBr(
          invokingBlocks[i + 1]
        );
        previousTerminator->eraseFromParent();
        // TODO
        // the right transformation needs to modify start/max iteration of any loop between if heartbeat promotion happens
        // right now ignore the logic because prototype benchmark is a 2-level nested loop
      }

      errs() << "leftover task after fixing control flow of invoking blocks" << *leftoverTask << "\n";
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
  noelle.getProgram()->getOrInsertGlobal(
    "leftoverTasks",
    leftoverTasksType
  );
  auto leftoverTasksGlobal = noelle.getProgram()->getNamedGlobal("leftoverTasks");
  leftoverTasksGlobal->setAlignment(8);
  leftoverTasksGlobal->setInitializer(
    ConstantArray::get(
      leftoverTasksType,
      ArrayRef<Constant *>({
        this->leftoverTasks
      })
    )
  );
  errs() << "created global leftoverTasks " << *leftoverTasksGlobal << "\n";

  return false;

}