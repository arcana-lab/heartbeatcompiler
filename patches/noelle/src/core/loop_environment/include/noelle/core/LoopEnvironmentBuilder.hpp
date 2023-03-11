/*
 * Copyright 2016 - 2022  Angelo Matni, Simone Campanoni
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 of the Software, and to permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include "noelle/core/SystemHeaders.hpp"
#include "noelle/core/LoopEnvironment.hpp"
#include "noelle/core/LoopEnvironmentUser.hpp"

namespace llvm::noelle {

class LoopEnvironmentBuilder {
public:
  /*
   * 08/24/2022 by Yian
   * This is a specialized consturctor for HeartBeat compiler only
   * Should be removed after the generalization
   */
  LoopEnvironmentBuilder(LLVMContext &cxt);

  LoopEnvironmentBuilder(
      LLVMContext &cxt,
      LoopEnvironment *env,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          shouldThisVariableBeReduced,
      uint64_t reducerCount,
      uint64_t numberOfUsers);

  LoopEnvironmentBuilder(
      LLVMContext &cxt,
      LoopEnvironment *env,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          shouldThisVariableBeReduced,
      std::function<bool(uint32_t variableID, bool isLiveOut)>
          shouldThisVariableBeSkipped,
      uint64_t reducerCount,
      uint64_t numberOfUsers);

  LoopEnvironmentBuilder(LLVMContext &CXT,
                         const std::vector<Type *> &varTypes,
                         const std::set<uint32_t> &singleVarIDs,
                         const std::set<uint32_t> &reducableVarIDs,
                         uint64_t reducerCount,
                         uint64_t numberOfUsers);

  LoopEnvironmentBuilder() = delete;

  void addVariableToEnvironment(uint64_t varID, Type *varType);

  /*
   * Generate code to create environment array/variable allocations
   */
  void allocateEnvironmentArray(IRBuilder<> builder);
  void generateEnvVariables(IRBuilder<> builder);

  /*
   * Reduce live out variables given binary operators to reduce
   * with and initial values to start at
   */
  BasicBlock *reduceLiveOutVariables(
      BasicBlock *bb,
      IRBuilder<> builder,
      const std::unordered_map<uint32_t, Instruction::BinaryOps>
          &reducableBinaryOps,
      const std::unordered_map<uint32_t, Value *> &initialValues,
      Value *numberOfThreadsExecuted);

  /*
   * As all users of the environment know its structure, pass around the
   * equivalent of a void pointer
   */
  Value *getEnvironmentArrayVoidPtr(void) const;
  Value *getEnvironmentArray(void) const;
  ArrayType *getEnvironmentArrayType(void) const;

  LoopEnvironmentUser *getUser(uint32_t user) const;
  uint32_t getNumberOfUsers(void) const;

  virtual Value *getEnvironmentVariable(uint32_t id) const;
  uint32_t getIndexOfEnvironmentVariable(uint32_t id) const;
  virtual bool isIncludedEnvironmentVariable(uint32_t id) const;
  virtual Value *getAccumulatedReducedEnvironmentVariable(uint32_t id) const;
  Value *getReducedEnvironmentVariable(uint32_t id, uint32_t reducerInd) const;
  virtual bool hasVariableBeenReduced(uint32_t id) const;

  virtual ~LoopEnvironmentBuilder();

protected:
  LLVMContext &CXT;
  uint64_t envSize;
  uint64_t numReducers;
  std::unordered_map<uint32_t, Value *>
      envIndexToVar; // envIndex to sigle environment variable
  std::unordered_map<uint32_t, std::vector<Value *>>
      envIndexToReducableVar; // envIndex to reducible environment variable
  std::unordered_map<uint32_t, AllocaInst *>
      envIndexToVectorOfReducableVar; // envIndex to reducible envrionment
                                      // variable array
  std::unordered_map<uint32_t, Value *>
      envIndexToAccumulatedReducableVar; // envIndex to latest reduction result

  /*
   * Information on a specific user (a function, stage, chunk, etc...)
   */
  std::vector<LoopEnvironmentUser *> envUsers;

private:
  /*
   * The environment array, owned by this builder
   */
  Value *envArray;
  Value *envArrayInt8Ptr;

  /*
   * Map and reverse map from envID to index
   */
  std::unordered_map<uint32_t, uint32_t> envIDToIndex;
  std::unordered_map<uint32_t, uint32_t> indexToEnvID;

  /*
   * The environment variable types and their allocations
   */
  ArrayType *envArrayType;
  std::vector<Type *> envTypes;

  virtual void initializeBuilder(const std::vector<Type *> &varTypes,
                                 const std::set<uint32_t> &singleVarIDs,
                                 const std::set<uint32_t> &reducableVarIDs,
                                 uint64_t reducerCount,
                                 uint64_t numberOfUsers);

  virtual void createUsers(uint32_t numUsers);
};

} // namespace llvm::noelle
