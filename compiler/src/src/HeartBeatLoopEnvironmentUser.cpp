#include "HeartBeatLoopEnvironmentUser.hpp"
#include "noelle/core/Architecture.hpp"

HeartBeatLoopEnvironmentUser::HeartBeatLoopEnvironmentUser(
  std::unordered_map<uint32_t, uint32_t> &singleEnvIDToIndex,
  std::unordered_map<uint32_t, uint32_t> &reducibleEnvIDToIndex,
  std::set<uint32_t> &constLiveInIDs)
  : LoopEnvironmentUser{ *(new std::unordered_map<uint32_t, uint32_t>()) },
    singleEnvIDToIndex { singleEnvIDToIndex },
    reducibleEnvIDToIndex { reducibleEnvIDToIndex },
    constLiveInIDs{ constLiveInIDs } {
  singleEnvIndexToPtr.clear();
  reducibleEnvIndexToPtr.clear();

  return;
}

Instruction * HeartBeatLoopEnvironmentUser::createSingleEnvironmentVariablePointer(IRBuilder<> builder, uint32_t envID, Type *type) {
  if (!this->singleEnvArray) {
    errs() << "A bitcast inst to the single environment array has not been set for this user!\n";
  }

  assert(this->singleEnvIDToIndex.find(envID) != this->singleEnvIDToIndex.end() && "The single environment variable is not included in the user\n");
  auto singleEnvIndex = this->singleEnvIDToIndex[envID];

  auto int64 = IntegerType::get(builder.getContext(), 64);
  auto zeroV = cast<Value>(ConstantInt::get(int64, 0));

  auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);
  auto envIndV = cast<Value>(ConstantInt::get(int64, singleEnvIndex * valuesInCacheLine));
  auto envGEP = builder.CreateInBoundsGEP(this->singleEnvArray, ArrayRef<Value *>({ zeroV, envIndV }));
  auto envPtr = builder.CreateBitCast(envGEP, PointerType::getUnqual(type));

  auto ptrInst = cast<Instruction>(envPtr);
  this->singleEnvIndexToPtr[singleEnvIndex] = ptrInst;

  return ptrInst;
}

void HeartBeatLoopEnvironmentUser::createReducableEnvPtr(IRBuilder<> builder, uint32_t envID, Type *type, uint32_t reducerCount, Value *reducerIndV) {
  if (!this->reducibleEnvArray) {
    errs() << "A reference to the environment array has not been set for this user!\n";
    abort();
  }

  errs() << "reducible environment array (array that stores the reduction array for all live-outs): " << *reducibleEnvArray << "\n";

  assert(this->reducibleEnvIDToIndex.find(envID) != this->reducibleEnvIDToIndex.end() && "The reducible environment variable is not included in the user\n");
  auto envIndex = this->reducibleEnvIDToIndex[envID];
  errs() << "live-out index in the live-out environment array: " << envIndex << "\n";

  auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);
  auto int64 = IntegerType::get(builder.getContext(), 64);
  auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
  auto envIndV = cast<Value>(ConstantInt::get(int64, envIndex * valuesInCacheLine));
  auto envReduceGEP = builder.CreateInBoundsGEP(
    this->reducibleEnvArray,
    ArrayRef<Value *>({ zeroV, envIndV }),
    std::string("reductionArrayLiveOut_").append(std::to_string(envIndex)).append("_Ptr")
  );

  auto envReducePtr = builder.CreateBitCast(
    envReduceGEP,
    PointerType::getUnqual(PointerType::getUnqual(ArrayType::get(int64, reducerCount * valuesInCacheLine))),
    std::string("reductionArrayLiveOut_").append(std::to_string(envIndex)).append("_PtrCasted")
  );
  auto reducibleArrayPtr = builder.CreateLoad(
    envReducePtr,
    std::string("reductionArrayLiveOut_").append(std::to_string(envIndex))
  );

  auto reduceIndAlignedV = builder.CreateMul(
    reducerIndV,
    ConstantInt::get(int64, valuesInCacheLine),
    std::string("reductionArrayLiveOut_").append(std::to_string(envIndex)).append("_index")
  );
  auto envGEP = builder.CreateInBoundsGEP(
    reducibleArrayPtr,
    ArrayRef<Value *>({ zeroV, reduceIndAlignedV }),
    std::string("liveOut_").append(std::to_string(envIndex)).append("_Ptr")
  );
  auto envPtr = builder.CreateBitCast(
    envGEP,
    PointerType::getUnqual(type),
    std::string("liveOut_").append(std::to_string(envIndex)).append("_PtrCasted")
  );
  errs() << "reducible variable pointer (envPtr): " << *envPtr << "\n";

  this->reducibleEnvIndexToPtr[envIndex] = cast<Instruction>(envPtr);

  return;
}

Instruction * HeartBeatLoopEnvironmentUser::getEnvPtr(uint32_t id) {

  assert((this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end() || this->reducibleEnvIDToIndex.find(id) != this->reducibleEnvIDToIndex.end()) && "The environment variable is not included in the user\n");

  errs() << "inside HeartBeatLoopEnvironmentUser::getEnvPtr\n";
  for (auto pair : this->reducibleEnvIDToIndex) {
    errs() << pair.first << ", " << pair.second << "\n";
  }

  Instruction *ptr = nullptr;
  if (this->liveInIDs.find(id) != this->liveInIDs.end()) {
    auto ind = this->singleEnvIDToIndex[id];
    ptr = this->singleEnvIndexToPtr[ind];
    assert(ptr != nullptr);
  } else {
    assert(this->liveOutIDs.find(id) != this->liveOutIDs.end() && "envID is not included in both live-in and live-out in the user\n");
    auto ind = this->reducibleEnvIDToIndex[id];
    ptr = this->reducibleEnvIndexToPtr[ind];
    assert(ptr != nullptr);
  }

  return ptr;
}

iterator_range<std::set<uint32_t>::iterator> HeartBeatLoopEnvironmentUser::getEnvIDsOfConstantLiveInVars() {
  return make_range(this->constLiveInIDs.begin(), this->constLiveInIDs.end());
}