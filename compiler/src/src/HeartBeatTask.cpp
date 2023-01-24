/*
 * Copyright 2021 - 2022  Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include "HeartBeatTask.hpp"

using namespace llvm;
using namespace llvm::noelle;

HeartBeatTask::HeartBeatTask (
  FunctionType *taskSignature,
  Module &M,
  uint64_t level,
  bool containsLiveOut
) : DOALLTask{taskSignature, M},
    // maxGIV{nullptr},
    level(level),
    containsLiveOut(containsLiveOut) {
  return ;
}

void HeartBeatTask::extractFuncArgs (void) {
  auto argIter = this->F->arg_begin();

  // /*
  //  * First parameter: first iteration to execute
  //  */
  // this->coreArg = (Value *) &*(argIter++); 

  // /*
  //  * Second argument: last value of the loop-governing IV
  //  */
  // this->maxGIV = (Value *) &*(argIter++);

  // /*
  //  * Third argument: live-in and live-out variables
  //  */
  // this->singleEnvArg = (Value *) &*(argIter++);

  // /*
  //  * Forth argumemt: live-out (reducible) variables      
  //  */
  // this->reducibleEnvArg = (Value *) &*(argIter++);

  // /*
  //  * Forth argument: task ID 
  //  */
  // this->instanceIndexV = (Value *) &*(argIter++);

  // First argument: context pointer
  this->contextArg = (Value *) &*(argIter++);

  // Second argument (optional): myIndex
  if (this->containsLiveOut) {
    this->myIndexArg = (Value *) &*(argIter++);
  }

  // All startIteration and maxIteration
  for (uint64_t i = 0; i < this->level + 1; i++) {
    this->iterationsVector.push_back((Value *) &*(argIter++));  // startIteration
    this->iterationsVector.push_back((Value *) &*(argIter++));  // maxIteration
  }

  this->startIterationArg = this->iterationsVector[this->level * 2 + 0];
  this->maxiterationArg = this->iterationsVector[this->level * 2 + 1];

  return ;
}
