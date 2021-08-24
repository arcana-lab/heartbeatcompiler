#pragma once

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

namespace {

 class HeartBeat : public ModulePass {
   public:
     static char ID; 

     HeartBeat();

     bool doInitialization (Module &M) override ;

     bool runOnModule (Module &M) override ;

     void getAnalysisUsage(AnalysisUsage &AU) const override ;

    private:

  };

}
