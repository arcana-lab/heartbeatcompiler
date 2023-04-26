#include "Pass.hpp"

using namespace llvm;
using namespace llvm::noelle;

static cl::opt<bool> Chunk_Loop_Iterations("chunk_loop_iterations", cl::desc("Execute loop iterations in chunk"));
static cl::list<std::string> Chunksize("chunksize", cl::desc("Specify the chunksize for each level of nested loop"), cl::OneOrMore);
static cl::opt<bool> Enable_Rollforward("enable_rollforward", cl::desc("Enable rollforward compilation"));

Heartbeat::Heartbeat () 
  : ModulePass(ID) 
  , outputPrefix("Heartbeat: ")
  , functionSubString("HEARTBEAT_")
{
  return ;
}

bool Heartbeat::doInitialization (Module &M) {
  return false;
}

bool Heartbeat::runOnModule (Module &M) {
  auto modified = false;
  errs() << this->outputPrefix << "Start\n";

  /*
   * Fetch NOELLE and select Heartbeat loops
   */
  auto &noelle = getAnalysis<Noelle>();
  auto allLoops = noelle.getLoopStructures();
  auto heartbeatLoops = this->selectHeartbeatLoops(noelle, allLoops);
  errs() << this->outputPrefix << heartbeatLoops.size() << " loops will be parallelized\n";
  for (auto selectedLoop : heartbeatLoops) {
    errs() << this->outputPrefix << selectedLoop->getLoopStructure()->getFunction()->getName() << "\n";
  }

  // create heartbeat_reset, polling function and loop_handler function
  createHBResetFunction(noelle);
  createPollingFunction(noelle);
  createLoopHandlerFunction(noelle);
  if (Enable_Rollforward) {
    createRFHandlerFunction(noelle);
  }

  /*
   * Determine the loop nest and all loops in that nest
   */
  this->performLoopNestAnalysis(noelle, heartbeatLoops);

  for (auto pair : this->nestIDToLoops) {
    this->reset();

    uint64_t nestID = pair.first;

    /*
     * Determine the level of the targeted Heartbeat loop, and the root loop for each targeted loop
     */
    this->performLoopLevelAnalysis(noelle, pair.second);

    /*
     * Set the chunksize using command line arguments
     */
    if (Chunk_Loop_Iterations) {
      this->chunkLoopIterations = true;
      uint64_t index = 0;
      for (auto &chunksize : Chunksize) {
        this->loopToChunksize[this->levelToLoop[index]] = stoi(chunksize);
        index++;
      }
    }

    /*
     * Determine if any Heartbeat loop contains live-out variables
     */
    this->handleLiveOut(noelle, pair.second);

    /*
     * Constant live-in analysis to determine the set of constant live-ins of each loop
     */
    this->performConstantLiveInAnalysis(noelle, pair.second);

    /*
     * Create constant live-ins global pointer
     */
    this->createConstantLiveInsGlobalPointer(noelle, nestID);

    /*
     * Parallelize the selected loop.
     */
    this->parallelizeRootLoop(noelle, nestID, this->rootLoop);
    if (modified){
      errs() << this->outputPrefix << "  The root loop has been modified\n";
    }

    for (auto loop : pair.second) {
      if (loop == this->rootLoop) {
        // root loop has already been parallelized, skip it
        continue;
      } else {
        modified |= this->parallelizeNestedLoop(noelle, nestID, loop);
      }
    }

    errs() << "parallelization completed!\n";
    errs() << "original root loop after parallelization\n" << *(this->rootLoop->getLoopStructure()->getFunction()) << "\n";
    errs() << "cloned root loop after parallelization\n" << *(this->loopToHeartbeatTransformation[rootLoop]->getHeartbeatTask()->getTaskBody()) << "\n";
    for (auto pair : this->loopToHeartbeatTransformation) {
      if (pair.first == this->rootLoop) {
        continue;
      }
      errs() << "cloned loop of level " << this->loopToLevel[pair.first] << "\n" << *(pair.second->getHeartbeatTask()->getTaskBody()) << "\n";
    }

    /*
     * Chunking transformation
     * Chunksize is supposed to be passed from the command line
     */
    if (this->chunkLoopIterations) {
      for (auto pair : this->loopToHeartbeatTransformation) {
        auto ldi = pair.first;
        if (this->loopToChunksize[ldi] > 0) {
          errs() << "found a loop that needs to be executed in chunk, do the chunking transformation\n";
          this->executeLoopInChunk(ldi, pair.second, this->loopToLevel[ldi] == this->numLevels - 1);
        }
      }
    }

    if (this->numLevels > 1) {
      // all Heartbeat loops are generated,
      // create slice tasks global
      createSliceTasks(noelle, nestID);

      // now we've created all the Heartbeat loops, it's time to create the leftover tasks per gap between Heartbeat loops
      createLeftoverTasks(noelle, nestID, pair.second);

      // all leftover tasks have been created
      // 1. need to fix the call to loop_handler to use the leftoverTasks global
      // 2. need to fix the call to loop_handler to use the leftoverTaskIndexSelector
      for (auto pair : this->loopToHeartbeatTransformation) {
        auto callInst = cast<CallInst>(pair.second->getCallToLoopHandler());

        IRBuilder<> builder{ callInst };
        auto sliceTasksGlobal = noelle.getProgram()->getNamedGlobal(std::string("sliceTasks_nest").append(std::to_string(nestID)));
        auto sliceTasksGEP = builder.CreateInBoundsGEP(
          sliceTasksGlobal->getType()->getPointerElementType(),
          sliceTasksGlobal,
          ArrayRef<Value *>({
            builder.getInt64(0),
            builder.getInt64(0),
          })
        );
        auto leftoverTasksGlobal = noelle.getProgram()->getNamedGlobal(std::string("leftoverTasks_nest").append(std::to_string(nestID)));
        auto leftoverTasksGEP = builder.CreateInBoundsGEP(
          leftoverTasksGlobal->getType()->getPointerElementType(),
          leftoverTasksGlobal,
          ArrayRef<Value *>({
            builder.getInt64(0),
            builder.getInt64(0)
          })
        );

        callInst->setArgOperand(3, sliceTasksGEP);
        callInst->setArgOperand(4, leftoverTasksGEP);
        callInst->setArgOperand(5, this->leftoverTaskIndexSelector);
        errs() << "updated loop_handler function in loop level " << this->loopToLevel[pair.first] << "\n" << *callInst << "\n";
      }
    } else {
      // only need to replace the sliceTasks pointer in the loop_handler call
      assert(this->numLevels == 1);
      createSliceTasks(noelle, nestID);

      auto callInst = loopToHeartbeatTransformation[this->rootLoop]->getCallToLoopHandler();
      IRBuilder<> builder{ callInst };
      auto sliceTasksGlobal = noelle.getProgram()->getNamedGlobal(std::string("sliceTasks_nest").append(std::to_string(nestID)));
      auto sliceTasksGEP = builder.CreateInBoundsGEP(
        sliceTasksGlobal->getType()->getPointerElementType(),
        sliceTasksGlobal,
        ArrayRef<Value *>({
          builder.getInt64(0),
          builder.getInt64(0),
        })
      );
      callInst->setArgOperand(3, sliceTasksGEP);

      errs() << "updated loop_handler function in root loop\n" << *callInst << "\n";
    }

    if (Enable_Rollforward) {
      replaceWithRollforwardHandler(noelle);
      eraseHBResetCall();
    }
  }

  errs() << this->outputPrefix << "Exit\n";
  return modified;
}

void Heartbeat::reset() {
  this->functionToLoop.clear();
  this->loopToLevel.clear();
  this->levelToLoop.clear();
  this->loopToRoot.clear();
  this->loopToCallerLoop.clear();
  this->rootLoop = nullptr;
  this->numLevels = 0;

  this->containsLiveOut = false;

  this->loopToSkippedLiveIns.clear();
  this->loopToConstantLiveIns.clear();
  this->constantLiveInsArgIndex.clear();
  this->constantLiveInsArgIndexToIndex.clear();

  this->loopToHeartbeatTransformation.clear();

  this->sliceTasks.clear();

  this->leftoverTasks.clear();
  this->leftoverTaskIndexSelector = nullptr;

  this->loopToChunksize.clear();
}

void Heartbeat::eraseHBResetCall() {
  auto rootTransformation = this->loopToHeartbeatTransformation[rootLoop];
  assert(rootTransformation->getCallToHBResetFunction() != nullptr);
  errs() << "Heartbeat::eraseHBResetCall(): erase the call to heartbeat_reset in the original root loop\n";
  rootTransformation->eraseCallToHBResetFunction();
}

void Heartbeat::replaceWithRollforwardHandler(Noelle &noelle) {
  // Decide to use the rollforward loop handler
  // 1. allocate a stack space for rc (return code) at the beginning of the function
  // 2. initialize rc with value 0 on stack
  // 3. replace polling function call with __rf_test call
  // 4. pass the pointer of the stack space as the first argument to the rollforward loop handler
  // 5. replace loop_hander function call with __rf_handle_wrapper call
  // 6. load the value from the stack space after calling
  // 7. replace all uses with the previous loop_handler_return_code
  for (auto pair : this->loopToHeartbeatTransformation) {
    auto pollingFunctionCall = cast<CallInst>(pair.second->getCallToPollingFunction());
    auto loopHandlerCall = cast<CallInst>(pair.second->getCallToLoopHandler());

    IRBuilder<> entryBuilder{ pair.second->getHeartbeatTask()->getTaskBody()->begin()->getFirstNonPHI() };
    // create a int64 stack space for return code
    auto rcPointer = entryBuilder.CreateAlloca(
      entryBuilder.getInt64Ty(),
      nullptr,
      "rollforward_handler_return_code_pointer"
    );
    entryBuilder.CreateStore(
      entryBuilder.getInt64(0),
      rcPointer
    );

    // set call to the polling function with __rf_test call
    auto rfTestFunction = noelle.getProgram()->getFunction("__rf_test");
    assert(rfTestFunction != nullptr);
    pollingFunctionCall->setCalledFunction(rfTestFunction);

    IRBuilder<> builder{ loopHandlerCall };
    auto rfHandlerFunction = noelle.getProgram()->getFunction("__rf_handle_wrapper");
    assert(rfHandlerFunction != nullptr);
    errs() << "rfHandler function\n" << *rfHandlerFunction << "\n";
    std::vector<Value *> rollforwardHandlerParameters { rcPointer };
    rollforwardHandlerParameters.insert(rollforwardHandlerParameters.end(), loopHandlerCall->arg_begin(), loopHandlerCall->arg_end());
    auto rfCallInst = builder.CreateCall(
      rfHandlerFunction,
      ArrayRef<Value *>({
        rollforwardHandlerParameters
      })
    );

    auto rc = builder.CreateLoad(
      builder.getInt64Ty(),
      rcPointer,
      "rollforward_handler_return_code"
    );

    loopHandlerCall->replaceAllUsesWith(rc);
    loopHandlerCall->eraseFromParent();
    pair.second->callToLoopHandler = rfCallInst;

    errs() << "replace original loop_handler call with rollforward handler call " << *rfCallInst << "\n";
    errs() << "heartbeat task after using rollforward handler " << *(pair.second->getHeartbeatTask()->getTaskBody()) << "\n";
  }
}

void Heartbeat::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<Noelle>(); // so that LLVM knows this pass depends on noelle
}

// Next there is code to register your pass to "opt"
char Heartbeat::ID = 0;
static RegisterPass<Heartbeat> X("heartbeat", "Heartbeat transformation");

// Next there is code to register your pass to "clang"
static Heartbeat * _PassMaker = NULL;
static RegisterStandardPasses _RegPass1(PassManagerBuilder::EP_OptimizerLast,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new Heartbeat());}}); // ** for -Ox
static RegisterStandardPasses _RegPass2(PassManagerBuilder::EP_EnabledOnOptLevel0,
    [](const PassManagerBuilder&, legacy::PassManagerBase& PM) {
        if(!_PassMaker){ PM.add(_PassMaker = new Heartbeat()); }}); // ** for -O0
