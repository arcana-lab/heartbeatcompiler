#include "Pass.hpp"

using namespace llvm::noelle;

void Heartbeat::performConstantLiveInAnalysis (
  Noelle &noelle,
  const std::set<LoopDependenceInfo *> &heartbeatLoops
) {

  errs() << this->outputPrefix << "Constant live-in analysis starts\n";

  /*
   * Find the function arguments list of the root loop
   */
  auto root_ldi = this->loopToRoot[*(heartbeatLoops.begin())];
  auto root_ls = root_ldi->getLoopStructure();
  auto root_fn = root_ls ->getFunction();
  assert(root_fn->getName().contains(this->functionSubString) && "Root loop function doesn't start with HEARTBEAT_");

  auto arg_index = 0;
  for (auto &arg : root_fn->args()) {
    errs() << "======\n";
    errs() << "perform constant live-in analysis of arg " << arg << ", of index " << arg.getArgNo() << "\n";
    constantLiveInToLoop(arg, arg_index, root_ldi);
    arg_index++;
  }

  errs() << "---------- Skipped live-ins\n";
  for (auto &loop_pair : this->loopToSkippedLiveIns) {
    errs() << loop_pair.first->getLoopStructure()->getFunction()->getName() << "\n";
    for (auto skipped_val : loop_pair.second) {
      errs() << "  " << *skipped_val << "\n";
    }
  }

  errs() << "---------- Constant live-ins\n";
  for (auto &loop_pair : this->loopToConstantLiveIns) {
    errs() << loop_pair.first->getLoopStructure()->getFunction()->getName() << "\n";
    for (auto pair : loop_pair.second) {
      errs() << "  constant live-in: " << *pair.first << ", index at root loop: " << pair.second << "\n";
    }
  }

  errs() << "----------\n";
  errs() << "there are " << this->constantLiveInsArgIndex.size() << " constant live-ins\n";
  uint64_t index = 0;
  for (auto argIndex : this->constantLiveInsArgIndex) {
    errs() << argIndex << " has index " << index << " when loading from constnatLiveIns pointer\n";
    this->constantLiveInsArgIndexToIndex[argIndex] = index;
    index++;
  }

  errs() << this->outputPrefix << "Constant live-in analysis completes\n";
  return ;
}

void Heartbeat::constantLiveInToLoop(llvm::Argument &arg, int arg_index, LoopDependenceInfo *ldi) {
  auto fn = ldi->getLoopStructure()->getFunction();
  errs() << "inside function " << fn->getName() << "\n";

  auto arg_val = static_cast<Value *>(&arg);
  this->loopToSkippedLiveIns[ldi].insert(arg_val);
  errs() << "adding " << *arg_val << " to " << fn->getName() << " skippedLiveIns\n";

  if (arg.getNumUses() == 0) {
    errs() << "edge case, should not take this branch as we have a dead argument supply to a function/loop\n";
    return;
  }

  // known bug here, if the arg is used to compute the start or exit condition,
  // then this is a const live-in to the loop
  // however, the parent loop should be aware of the computation and set the
  // start and exit condition correctly at the call site
  auto ivManager = ldi->getInductionVariableManager();
  auto ivAttr = ivManager->getLoopGoverningIVAttribution(*(ldi->getLoopStructure()));
  auto iv = ivAttr->getInductionVariable();
  auto startValue = iv.getStartValue();
  auto exitValue = ivAttr->getExitConditionValue();

  for (auto use_it = arg.use_begin() ; use_it != arg.use_end(); use_it++) {
    auto arg_user = (*(use_it)).getUser();

    errs() << "User of arg " << arg << ", " << *arg_user << "\n";

    if (iv.getSCC()->isInternal(cast<Value>(arg_user))) {
      // the logic to detecet whether a arg is start or exit condition
      // shall be checked by caller (parent loop)
      errs() << "  " << arg << " is either start/exit condition of the current loop\n";
      continue;
    }

    /*
     * If an argument is used inside a loop and through a call to another
     * heartbeat function, it depends on whether this arg is acted as
     * a start/exit condition value in the callee function
     */
    if (auto callInst = dyn_cast<CallInst>(arg_user)) {
      auto callee = callInst->getCalledFunction();

      if (callee->getName().contains(this->functionSubString)) {  // this is call to an outlined heartbeat loop
        auto callee_ldi = this->functionToLoop[callee];
        // to get the index of arg in the callInst
        // https://llvm.org/doxygen/InstrTypes_8h_source.html#l01382
        auto callInst_index = &(*use_it) - callInst->arg_begin();
        // to get the corresponding arg in callee
        // https://llvm.org/doxygen/Function_8h_source.html#l00784
        auto callee_arg_it = callee->arg_begin() + callInst_index;

        errs() << "  " << arg << " is passed through a call to another heartbeat function: " << callee->getName() << "\n";
        errs() << "    index at caller: " << callInst_index << "\n";
        errs() << "    arg at callee: " << *callee_arg_it << "\n";

        if (isArgStartOrExitValue(*callee_arg_it, callee_ldi)) {
          this->loopToConstantLiveIns[ldi][static_cast<Value *>(&arg)] = arg_index;
          this->constantLiveInsArgIndex.insert(arg_index);
          errs() << "arg " << arg << " is a const live-in to function " << fn->getName() << ", arg_index is " << arg_index << "\n";
        }

        constantLiveInToLoop(*callee_arg_it, arg_index, callee_ldi);

      } else {  // not a call to an outlined heartbeat loop, the arg is a const live-in for the current loop
        this->loopToConstantLiveIns[ldi][static_cast<Value *>(&arg)] = arg_index;
        this->constantLiveInsArgIndex.insert(arg_index);
        errs() << "arg " << arg << " is a const live-in to function " << fn->getName() << ", arg_index is " << arg_index << "\n";
      }
    } else {  // user is not a call inst, start, nor exit condition value
              // this arg is a constant live-in
      this->loopToConstantLiveIns[ldi][static_cast<Value *>(&arg)] = arg_index;
      this->constantLiveInsArgIndex.insert(arg_index);
      errs() << "arg " << arg << " is a const live-in to function " << fn->getName() << ", arg_index is " << arg_index << "\n";
    }
  }

  return;
}

bool Heartbeat::isArgStartOrExitValue(llvm::Argument &arg, LoopDependenceInfo *ldi) {
  auto ls = ldi->getLoopStructure();

  auto ivManager = ldi->getInductionVariableManager();
  auto ivAttr = ivManager->getLoopGoverningIVAttribution(*ls);
  auto iv = ivAttr->getInductionVariable();

  auto startValue = iv.getStartValue();
  auto exitValue = ivAttr->getExitConditionValue();

  if (&arg == startValue || &arg == exitValue) {
    return true;
  }

  return false;
}

void Heartbeat::createConstantLiveInsGlobalPointer(Noelle &noelle, uint64_t nestID) {
  auto M = noelle.getProgram();
  IRBuilder<> builder { M->getContext() };

  std::string constantLiveInsGlobalName = std::string("constantLiveInsPointer_nest").append(std::to_string(nestID));
  M->getOrInsertGlobal(constantLiveInsGlobalName, PointerType::getUnqual(builder.getInt64Ty()));
  
  auto constantLiveInsGlobalPointer = M->getNamedGlobal(constantLiveInsGlobalName);
  constantLiveInsGlobalPointer->setDSOLocal(true);
  constantLiveInsGlobalPointer->setInitializer(Constant::getNullValue(PointerType::getUnqual(builder.getInt64Ty())));
  constantLiveInsGlobalPointer->setAlignment(8);

  errs() << "create constant live-ins global pointer in the module " << *constantLiveInsGlobalPointer << "\n";

  return;
}
