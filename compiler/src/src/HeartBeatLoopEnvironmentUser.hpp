#pragma once

#include "noelle/core/LoopEnvironmentUser.hpp"

using namespace llvm::noelle;
using namespace llvm;

class HeartBeatLoopEnvironmentUser : public LoopEnvironmentUser {

public:
  HeartBeatLoopEnvironmentUser(std::unordered_map<uint32_t, uint32_t> &singleEnvIDToIndex,
                               std::unordered_map<uint32_t, uint32_t> &reducibleEnvIDToIndex);

  inline void setSingleEnvironmentArray(Value *singleEnvArray) { this->singleEnvArray = singleEnvArray; };
  
  inline void setReducibleEnvironmentArray(Value *reducibleEnvArray) { this->reducibleEnvArray = reducibleEnvArray; };
  
  inline void addLiveIn(uint32_t id) override {
    if (this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end()) {
      this->liveInIDs.insert(id);
    }
  };
  
  inline void addLiveOut(uint32_t id) override {
    if (this->reducibleEnvIDToIndex.find(id) != this->reducibleEnvIDToIndex.end()) {
      this->liveOutIDs.insert(id);
    }
  };
  
  Instruction * createSingleEnvironmentVariablePointer(IRBuilder<> builder, uint32_t envID, Type *type);
  
  void createReducableEnvPtr(IRBuilder<> builder, uint32_t envID, Type *type, uint32_t reducerCount, Value *reducerIndV) override;
  
  Instruction * getEnvPtr(uint32_t id) override;

private:
  std::unordered_map<uint32_t, uint32_t> &singleEnvIDToIndex;
  std::unordered_map<uint32_t, uint32_t> &reducibleEnvIDToIndex;
  std::unordered_map<uint32_t, Instruction *> singleEnvIndexToPtr;
  std::unordered_map<uint32_t, Instruction *> reducibleEnvIndexToPtr;

  Value *singleEnvArray;
  Value *reducibleEnvArray;

};