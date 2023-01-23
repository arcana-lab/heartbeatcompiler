#pragma once

#include "noelle/core/LoopEnvironmentBuilder.hpp"
#include "HeartBeatLoopEnvironmentUser.hpp"

using namespace llvm::noelle;

class HeartBeatLoopEnvironmentBuilder : public LoopEnvironmentBuilder {

public:
  HeartBeatLoopEnvironmentBuilder(
    LLVMContext &cxt,
    LoopEnvironment *env,
    std::function<bool(uint32_t variableID, bool isLiveOut)>
        shouldThisVariableBeReduced,
    std::function<bool(uint32_t variableID, bool isLiveOut)>
        shouldThisVariableBeSkipped,
    uint64_t reducerCount,
    uint64_t numberOfUsers);

  inline uint32_t getSingleEnvironmentSize() { return this->singleEnvIDToIndex.size(); };

  inline uint32_t getReducibleEnvironmentSize() { return this->reducibleEnvIDToIndex.size(); };

  inline ArrayType * getSingleEnvironmentArrayType() { return this->singleEnvArrayType; };

  inline ArrayType * getReducibleEnvironmentArrayType() { return this->reducibleEnvArrayType; };

  bool hasVariableBeenReduced(uint32_t id) const override;

  void allocateNextLevelReducibleEnvironmentArray(IRBuilder<> builder);

  void generateNextLevelReducibleEnvironmentVariables(IRBuilder <> builder);

  inline Value * getNextLevelEnvironmentArrayVoidPtr() {
    assert(this->reducibleEnvArrayInt8PtrNextLevel != nullptr && "Next level envrionment array void ptr hasn't been set yet\n");
    return this->reducibleEnvArrayInt8PtrNextLevel;
  };

  BasicBlock * reduceLiveOutVariablesInTask(
    int taskIndex,
    BasicBlock *loopHandlerBB,
    IRBuilder<> loopHandlerBuilder,
    const std::unordered_map<uint32_t, Instruction::BinaryOps> &reducibleBinaryOps,
    const std::unordered_map<uint32_t, Value *> &initialValues,
    Instruction *cmpInst,
    BasicBlock *bottomHalfBB,
    Value *numOfReducerV);

  void allocateSingleEnvironmentArray(IRBuilder<> builder);

  void allocateReducibleEnvironmentArray(IRBuilder<> builder);

  void generateEnvVariables(IRBuilder<> builder, uint32_t reducerCount);

  inline bool isIncludedEnvironmentVariable(uint32_t id) const override {
    return (this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end()) || (this->reducibleEnvIDToIndex.find(id) != this->reducibleEnvIDToIndex.end());
  }

  Value * getEnvironmentVariable(uint32_t id) const override;

  inline Value * getSingleEnvironmentArrayPointer() { return this->singleEnvArrayInt8Ptr; };

  inline Value * getReducibleEnvironmentArrayPointer() { return this->reducibleEnvArrayInt8Ptr; };

  BasicBlock * reduceLiveOutVariablesWithInitialValue(
    BasicBlock *bb,
    IRBuilder<> &builder,
    const std::unordered_map<uint32_t, Instruction::BinaryOps> &reducibleBinaryOps,
    const std::unordered_map<uint32_t, Value *> &initialValues);
  
  Value * getAccumulatedReducedEnvironmentVariable(uint32_t id) const override;

private:
  void initializeBuilder(const std::vector<Type *> &varTypes,
                         const std::set<uint32_t> &singleVarIDs,
                         const std::set<uint32_t> &reducibleVarIDs,
                         uint64_t reducerCount,
                         uint64_t numberOfUsers) override;
  void createUsers(uint32_t numUsers) override;

  std::unordered_map<uint32_t, uint32_t> singleEnvIDToIndex;
  std::unordered_map<uint32_t, uint32_t> reducibleEnvIDToIndex;
  std::unordered_map<uint32_t, uint32_t> indexToSingleEnvID;
  std::unordered_map<uint32_t, uint32_t> indexToReducibleEnvID;

  Value *singleEnvArray;
  Value *singleEnvArrayInt8Ptr;
  ArrayType *singleEnvArrayType;
  Value *reducibleEnvArray;
  Value *reducibleEnvArrayInt8Ptr;
  Value *reducibleEnvArrayNextLevel;
  Value *reducibleEnvArrayInt8PtrNextLevel;
  ArrayType *reducibleEnvArrayType;
  std::unordered_map<uint32_t, AllocaInst *> envIndexToVectorOfReducableVarNextLevel;  // envIndex to reducible envrionment variable array
  std::unordered_map<uint32_t, Value *> envIndexToAccumulatedReducableVarForTask;  // envIndex to accumulated of reduction result

  std::vector<Type *> singleEnvTypes;
  std::vector<Type *> reducibleEnvTypes;

  uint64_t valuesInCacheLine;
};