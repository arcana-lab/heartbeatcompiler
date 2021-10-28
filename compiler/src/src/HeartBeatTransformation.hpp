/*
 * Copyright 2021   Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include "SystemHeaders.hpp"
#include "noelle/core/PDG.hpp"
#include "noelle/core/SCC.hpp"
#include "noelle/core/SCCDAG.hpp"
#include "noelle/core/PDGAnalysis.hpp"
#include "noelle/core/IVStepperUtility.hpp"
#include "noelle/core/LoopDependenceInfo.hpp"
#include "noelle/core/Noelle.hpp"
#include "noelle/tools/HeuristicsPass.hpp"
#include "noelle/tools/ParallelizationTechnique.hpp"
#include "noelle/tools/DOALL.hpp"

using namespace llvm::noelle;

class HeartBeatTransformation : public DOALL {
  public:

    /*
     * Methods
     */
    HeartBeatTransformation (
        Noelle &noelle
        );

    bool apply (
        LoopDependenceInfo *LDI,
        Noelle &par,
        Heuristics *h
        ) override ;

  protected:
    Noelle &n;

    /*
     * Helpers
     */
    void invokeHeartBeatFunctionAsideOriginalLoop (
        LoopDependenceInfo *LDI
        );
};
