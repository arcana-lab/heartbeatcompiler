#pragma once

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "Noelle.hpp"
#include "ParallelizationTechnique.hpp"

/* Interface for TPAL runtime 
 *   - assumptions: leaf loop (non nested), no reduction
 *   - return nonzero value if promotion happened, zero otherwise
 *   - indvar: loop induction variable, maxval: loop maximum value
 *   - ...
 *   int handler_for_fork2(int64_t indvar, int64_t maxval, 
 *                         void** livein, void(*f)(int64_t,int64_t,void**)) {
 *     ... here, we call a C++ function in the TPAL runtime
 *   }
 */

using namespace llvm ;
using namespace llvm::noelle ;

class HeartBeat : public ModulePass {
 public:
    static char ID; 

    HeartBeat();

    bool doInitialization (Module &M) override ;

    bool runOnModule (Module &M) override ;

    void getAnalysisUsage(AnalysisUsage &AU) const override ;

  private:
    bool parallelizeLoop (
      Noelle &noelle,
      LoopDependenceInfo *ldi
      );

    bool createHeartBeatLoop (
      Noelle &noelle,
      LoopDependenceInfo *ldi,
      ParallelizationTechnique **usedTechnique
      );

};
