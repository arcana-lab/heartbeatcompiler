#include "HeartBeatLoopEnvironmentBuilder.hpp"
#include "noelle/core/Architecture.hpp"

HeartBeatLoopEnvironmentBuilder::HeartBeatLoopEnvironmentBuilder(
  LLVMContext &cxt,
  LoopEnvironment *environment,
  std::function<bool(uint32_t variableID, bool isLiveOut)>
      shouldThisVariableBeReduced,
  std::function<bool(uint32_t variableID, bool isLiveOut)>
      shouldThisVariableBeSkipped,
  std::function<bool(uint32_t variableID, bool isLiveOut)>
      isConstantLiveInVariable,
  uint64_t reducerCount,
  uint64_t numberOfUsers,
  uint64_t numLevels)
  : LoopEnvironmentBuilder{ cxt },
    numLevels { numLevels },
    constantVars {} {

  assert(environment != nullptr);

  std::set<uint32_t> singleVars;
  std::set<uint32_t> reducibleVars;
  for (auto liveInVariableID : environment->getEnvIDsOfLiveInVars()) {
    if (isConstantLiveInVariable(liveInVariableID, false)) {
      this->constantVars.insert(liveInVariableID);
      continue;
    }
    if (shouldThisVariableBeSkipped(liveInVariableID, false)) {
      continue;
    }
    if (shouldThisVariableBeReduced(liveInVariableID, false)) {
      reducibleVars.insert(liveInVariableID);
    } else {
      singleVars.insert(liveInVariableID);
    }
  }
  for (auto liveOutVariableID : environment->getEnvIDsOfLiveOutVars()) {
    if (shouldThisVariableBeSkipped(liveOutVariableID, true)) {
      continue;
    }
    if (shouldThisVariableBeReduced(liveOutVariableID, true)) {
      reducibleVars.insert(liveOutVariableID);
    } else {
      singleVars.insert(liveOutVariableID);
    }
  }

  if (environment->getExitBlockID() >= 0) {
    singleVars.insert(environment->getExitBlockID());
  }

  errs() << "constant live-ins with their ID and value\n";
  for (auto constantVarID : constantVars) {
    errs() << constantVarID << ": " << *(environment->getProducer(constantVarID)) << "\n";
  }
  errs() << "live-ins with their ID and value\n";
  for (auto singleVarID : singleVars) {
    errs() << singleVarID << ": " << *(environment->getProducer(singleVarID)) << "\n";
  }
  errs() << "live-outs with their ID and value\n";
  for (auto reducibleVarID : reducibleVars) {
    errs() << reducibleVarID << ": " << *(environment->getProducer(reducibleVarID)) << "\n";
  }

  this->initializeBuilder(environment->getTypesOfEnvironmentLocations(),
                          singleVars,
                          reducibleVars,
                          reducerCount,
                          numberOfUsers);

  return;
}

void HeartBeatLoopEnvironmentBuilder::initializeBuilder(
  const std::vector<Type *> &varTypes,
  const std::set<uint32_t> &singleVarIDs,
  const std::set<uint32_t> &reducibleVarIDs,
  uint64_t reducerCount,
  uint64_t numberOfUsers) {

  uint32_t index = 0;
  for (auto singleVarID : singleVarIDs) {
    this->singleEnvIDToIndex[singleVarID] = index;
    this->indexToSingleEnvID[index] = singleVarID;
    this->singleEnvTypes.push_back(varTypes.at(singleVarID)); // here's the assumption made here, 
                                                              // singleVarID equals the index in the varTypes,
                                                              // same assumption for below
    index++;
  }
  index = 0;
  for (auto reducibleVarID : reducibleVarIDs) {
    this->reducibleEnvIDToIndex[reducibleVarID] = index;
    this->indexToReducibleEnvID[index] = reducibleVarID;
    this->reducibleEnvTypes.push_back(varTypes.at(reducibleVarID));
    index++;
  }

  this->envSize = singleVarIDs.size() + reducibleVarIDs.size();
  this->numReducers = reducerCount;

  this->singleEnvArray = nullptr;
  this->singleEnvArrayInt8Ptr = nullptr;
  this->singleEnvArrayType = nullptr;

  this->reducibleEnvArray = nullptr;
  this->reducibleEnvArrayInt8Ptr = nullptr;
  this->reducibleEnvArrayType = nullptr;

  // for (uint32_t i = 0; i < singleVarIDs.size(); i++) {
  //   auto index = this->singleEnvIDToIndex[i];
  //   this->singleEnvTypes.push_back(varTypes.at(index));
  // }
  // for (uint32_t i = 0; i < reducibleVarIDs.size(); i++) {
  //   auto index = this->reducibleEnvIDToIndex[i];
  //   this->reducibleEnvTypes.push_back(varTypes.at(index));
  // }
  assert(this->singleEnvTypes.size() + this->reducibleEnvTypes.size() == this->envSize && "Environment size doesn't match\n");

  this->valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);

  auto int64 = IntegerType::get(this->CXT, 64);
  this->singleEnvArrayType = ArrayType::get(int64, singleVarIDs.size() * this->valuesInCacheLine);
  this->reducibleEnvArrayType = ArrayType::get(int64, reducibleVarIDs.size() * this->valuesInCacheLine);

  auto int8 = IntegerType::get(this->CXT, 8);
  auto int8_ptr = PointerType::getUnqual(int8);
  // this->contextArrayType = ArrayType::get(int8_ptr, this->numLevels * this->valuesInCacheLine);
  this->contextArrayType = ArrayType::get(int64, this->numLevels * this->valuesInCacheLine);

  for (auto envID : singleVarIDs) {
    auto envIndex = this->singleEnvIDToIndex[envID];
    this->envIndexToVar[envIndex] = nullptr; 
  }
  for (auto envID : reducibleVarIDs) {
    auto envIndex = this->reducibleEnvIDToIndex[envID];
    this->envIndexToReducableVar[envIndex] = std::vector<Value *>();
  }

  this->createUsers(numberOfUsers);

  return;
}

void HeartBeatLoopEnvironmentBuilder::createUsers(uint32_t numUsers) {
  for (auto i = 0; i < numUsers; i++) {
    this->envUsers.push_back(new HeartBeatLoopEnvironmentUser(this->singleEnvIDToIndex, this->reducibleEnvIDToIndex, this->constantVars));
  }
}

bool HeartBeatLoopEnvironmentBuilder::hasVariableBeenReduced(uint32_t id) const {
  assert(this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end() || this->reducibleEnvIDToIndex.find(id) != this->reducibleEnvIDToIndex.end() && "This environment variable is not included in the builder\n");
  bool isSingle = false;
  bool isReduce = false;

  // We're looking at a single env variable
  if (this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end()) {
    auto ind = this->singleEnvIDToIndex.at(id);
    isSingle = this->envIndexToVar.find(ind) != this->envIndexToVar.end();
  }

  // We're looking at a reducible env variable
  if (this->reducibleEnvIDToIndex.find(id) != this->reducibleEnvIDToIndex.end()) {
    auto ind = this->reducibleEnvIDToIndex.at(id);
    isReduce = this->envIndexToReducableVar.find(ind) !=  this->envIndexToReducableVar.end();
  }

  assert(isSingle || isReduce);

  return isReduce;
}

void HeartBeatLoopEnvironmentBuilder::allocateNextLevelReducibleEnvironmentArray(IRBuilder<> builder) {
  if (this->getReducibleEnvironmentSize() > 0) {
    auto int8 = IntegerType::get(builder.getContext(), 8);
    auto int8_ptr = PointerType::getUnqual(int8);
    this->reducibleEnvArrayNextLevel = builder.CreateAlloca(
      this->reducibleEnvArrayType,
      nullptr,
      "liveOutEnvForKids"
    );

    auto envUser = (HeartBeatLoopEnvironmentUser *)this->getUser(0);
    builder.CreateStore(
      this->reducibleEnvArrayNextLevel,
      envUser->getLiveOutEnvBitcastInst()
    );
    // this->reducibleEnvArrayInt8PtrNextLevel = cast<Value>(builder.CreateBitCast(this->reducibleEnvArrayNextLevel, int8_ptr));
  }
  
  return;
}

void HeartBeatLoopEnvironmentBuilder::generateNextLevelReducibleEnvironmentVariables(IRBuilder <> builder) {
  if (this->getReducibleEnvironmentSize() <= 0) {
    return;
  }

  if (!this->reducibleEnvArrayNextLevel) {
    errs() << "The next level environment array for task has not been generated\n";
    abort();
  }

  auto int64 = IntegerType::get(builder.getContext(), 64);
  auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
  auto fetchCastedEnvPtr = [&](Value *arr, int envIndex, Type *ptrType) -> Value * {
    auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);
    auto indValue = cast<Value>(ConstantInt::get(int64, envIndex * valuesInCacheLine));
    auto envPtr = builder.CreateInBoundsGEP(arr, ArrayRef<Value *>({ zeroV, indValue }));
    auto cast = builder.CreateBitCast(envPtr, ptrType);

    return cast;
  };

  std::set<uint32_t> reducibleIndices;
  for (auto indexVarPair : this->envIndexToReducableVar) {
    auto envIndex = indexVarPair.first;

    auto varType = this->reducibleEnvTypes[envIndex];
    auto ptrType = PointerType::getUnqual(varType);

    auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);
    auto reducerArrType = ArrayType::get(varType, this->numReducers * valuesInCacheLine);

    auto reduceArrAlloca = builder.CreateAlloca(
      reducerArrType,
      nullptr,
      std::string("reductionArrayLiveOut_").append(std::to_string(0)).append("_ForKids")
    );
    this->envIndexToVectorOfReducableVarNextLevel[envIndex] = reduceArrAlloca;

    auto reduceArrPtrType = PointerType::getUnqual(reduceArrAlloca->getType());
    auto envPtr = fetchCastedEnvPtr(this->reducibleEnvArrayNextLevel, envIndex, reduceArrPtrType);
    builder.CreateStore(reduceArrAlloca, envPtr);
  }

  return;
}

BasicBlock * HeartBeatLoopEnvironmentBuilder::reduceLiveOutVariablesInTask(
    int taskIndex,
    BasicBlock *loopHandlerBB,
    IRBuilder<> loopHandlerBuilder,
    const std::unordered_map<uint32_t, Instruction::BinaryOps> &reducibleBinaryOps,
    const std::unordered_map<uint32_t, Value *> &initialValues,
    Instruction *cmpInst,
    BasicBlock *bottomHalfBB,
    Value *numOfReducerV) {
  assert(loopHandlerBB != nullptr);

  if (initialValues.size() == 0) {
    return loopHandlerBB;
  }

  auto f = loopHandlerBB->getParent();
  assert(f != nullptr);

  auto reductionBodyBB = BasicBlock::Create(this->CXT, "ReductionLoopBody", f);
  auto afterReductionBB = BasicBlock::Create(this->CXT, "AfterReduction", f, reductionBodyBB);

  auto bbTerminator = loopHandlerBB->getTerminator();
  assert(bbTerminator != nullptr && "loopHandlerBB terminator is null\n");
  bbTerminator->eraseFromParent();
  loopHandlerBuilder.CreateCondBr(cmpInst, bottomHalfBB, reductionBodyBB);

  IRBuilder<> reductionBodyBuilder{ reductionBodyBB };

  // Setup loop induction variable
  auto int32Type = IntegerType::get(loopHandlerBuilder.getContext(), 32);
  auto inductionVariablePHI = reductionBodyBuilder.CreatePHI(int32Type, 2);
  auto constantZero = ConstantInt::get(int32Type, 0);
  inductionVariablePHI->addIncoming(constantZero, loopHandlerBB);
  
  std::vector<PHINode *> phiNodes;
  auto count = 0;
  for (auto envIDInitValue : initialValues) {
    auto envID = envIDInitValue.first;
    auto envIndex = this->reducibleEnvIDToIndex[envID];
    auto initialValue = envIDInitValue.second;

    auto variableType = this->reducibleEnvTypes[envIndex];
    auto phiNode = reductionBodyBuilder.CreatePHI(variableType, 2);

    phiNode->addIncoming(initialValue, loopHandlerBB);
    phiNodes.push_back(phiNode);
  }

  auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);

  std::vector<Value *> loadedValues;
  for (auto envIDInitValue : initialValues) {
    auto envID = envIDInitValue.first;
    auto envIndex = this->reducibleEnvIDToIndex[envID];

    auto eightValue = ConstantInt::get(int32Type, valuesInCacheLine);
    auto offsetValue = reductionBodyBuilder.CreateMul(inductionVariablePHI, eightValue);

    /*
     * compute the effective address inside the reduction array
     */
    auto baseAddressOfReducedVar = this->envIndexToVectorOfReducableVarNextLevel[envIndex];
    auto zeroV = cast<Value>(ConstantInt::get(int32Type, 0));
    auto effectiveAddressOfReducedVar = reductionBodyBuilder.CreateInBoundsGEP(baseAddressOfReducedVar, ArrayRef<Value *>({ zeroV, offsetValue }));

    auto varType = this->reducibleEnvTypes[envIndex];
    auto ptrType = PointerType::getUnqual(varType);

    auto effectiveAddressOfReducedVarProperlyCasted = reductionBodyBuilder.CreateBitCast(effectiveAddressOfReducedVar, ptrType);

    auto envVar = reductionBodyBuilder.CreateLoad(effectiveAddressOfReducedVarProperlyCasted);
    loadedValues.push_back(envVar);
  }

  // Reduction accumulation
  count = 0;
  for (auto envIDInitValue : initialValues) {
    auto envID = envIDInitValue.first;
    auto envIndex = this->reducibleEnvIDToIndex[envID];

    auto binOp = reducibleBinaryOps.at(envID);
    auto accumVal = phiNodes[count];

    auto privateCurrentCopy = loadedValues[count];

    auto newAccumulatorValue = reductionBodyBuilder.CreateBinOp(binOp, accumVal, privateCurrentCopy);

    this->envIndexToAccumulatedReducableVarForTask[envIndex] = newAccumulatorValue;

    count++;
  }

  // Fix the phi nodes
  count = 0;
  for (auto envIDInitValue : initialValues) {
    auto envID = envIDInitValue.first;
    auto envIndex = this->reducibleEnvIDToIndex[envID];

    auto phiNode = phiNodes[count];
    phiNode->addIncoming(this->envIndexToAccumulatedReducableVarForTask[envIndex], reductionBodyBB);
    
    count++;
  }

  // update reduction variable
  auto constantOne = ConstantInt::get(int32Type, 1);
  auto updateInductionVariable = reductionBodyBuilder.CreateAdd(inductionVariablePHI, constantOne);
  inductionVariablePHI->addIncoming(updateInductionVariable, reductionBodyBB);
  auto loopCmpInst = reductionBodyBuilder.CreateICmpSLT(updateInductionVariable, numOfReducerV);
  reductionBodyBuilder.CreateCondBr(loopCmpInst, reductionBodyBB, afterReductionBB);

  /*
   * Store the reduction results into the original reduction array
   */
  IRBuilder<> afterReductionBuilder{ afterReductionBB };
  for (auto initialVal : initialValues) {
    auto envID = initialVal.first;
    auto envIndex = this->reducibleEnvIDToIndex[envID];
    auto envVarPtr = this->envUsers[taskIndex]->getEnvPtr(envID);
    afterReductionBuilder.CreateStore(this->envIndexToAccumulatedReducableVarForTask[envIndex], envVarPtr);
  }

  return afterReductionBB;
}

void HeartBeatLoopEnvironmentBuilder::allocateSingleEnvironmentArray(IRBuilder<> builder) {
  if (this->getSingleEnvironmentSize() > 0) {
    auto int8 = IntegerType::get(builder.getContext(), 8);
    auto ptrTy_int8 = PointerType::getUnqual(int8);
    this->singleEnvArray = builder.CreateAlloca(this->singleEnvArrayType, nullptr, "liveInEnv");
    this->singleEnvArrayInt8Ptr = cast<Value>(builder.CreateBitCast(
      this->singleEnvArray,
      ptrTy_int8,
      "liveInEnvCastedToVoidPtr"
    ));
  } else {
    errs() << "there's no liveInEnvironment array for this loop\n";
  }

  return;
}

void HeartBeatLoopEnvironmentBuilder::allocateReducibleEnvironmentArray(IRBuilder<> builder) {
  if (this->getReducibleEnvironmentSize() > 0) {
    auto int8 = IntegerType::get(builder.getContext(), 8);
    auto ptrTy_int8 = PointerType::getUnqual(int8);
    this->reducibleEnvArray = builder.CreateAlloca(this->reducibleEnvArrayType, nullptr, "liveOutEnv");
    this->reducibleEnvArrayInt8Ptr = cast<Value>(builder.CreateBitCast(
      this->reducibleEnvArray,
      ptrTy_int8,
      "liveOutEnvCastedToVoidPtr"
    ));
  }

  return;
}

void HeartBeatLoopEnvironmentBuilder::generateEnvVariables(IRBuilder<> builder, uint32_t reducerCount) {

  auto int64 = IntegerType::get(builder.getContext(), 64);
  auto zeroV = cast<Value>(ConstantInt::get(int64, 0));
  auto fetchCastedEnvPtr =
      [&](Value *arr, int envIndex, Type *ptrType) -> Value * {
    /*
     * Compute the offset of the variable with index "envIndex" that is stored
     * inside the environment.
     *
     * Compute how many values can fit in a cache line.
     */
    auto valuesInCacheLine =
        Architecture::getCacheLineBytes() / sizeof(int64_t);
    auto indValue =
        cast<Value>(ConstantInt::get(int64, envIndex * valuesInCacheLine));

    /*
     * Compute the address of the variable with index "envIndex".
     */
    auto envPtr =
        builder.CreateInBoundsGEP(arr, ArrayRef<Value *>({ zeroV, indValue }));

    /*
     * Cast the pointer to the proper data type.
     */
    auto cast = builder.CreateBitCast(envPtr, ptrType);

    return cast;
  };

  if (this->getSingleEnvironmentSize() > 0 && !this->singleEnvArray) {
    errs() << "Single environment array has not been generated\n";
    abort();
  }

  /*
   * Compute and cache the pointer of each variable that cannot be reduced and
   * that are stored inside the environment.
   *
   * NOTE: Manipulation of the map cannot be done while iterating it
   */
  std::set<uint32_t> singleIndices;
  for (auto indexVarPair : this->envIndexToVar) {
    singleIndices.insert(indexVarPair.first);
  }
  for (auto envIndex : singleIndices) {
    auto ptrType = PointerType::getUnqual(this->singleEnvTypes[envIndex]);
    this->envIndexToVar[envIndex] = fetchCastedEnvPtr(this->singleEnvArray, envIndex, ptrType);
  }

  if (this->getReducibleEnvironmentSize() > 0 && !this->reducibleEnvArray) {
    errs() << "Reducible environment array has not been generated\n";
    abort();
  }

  /*
   * Vectorize reducable variables.
   * Moreover, compute and cache the pointer of each reducable variable that are
   * stored inside the environment.
   *
   * NOTE: No manipulation and iteration at the same time
   */
  std::set<uint32_t> reducableIndices;
  for (auto indexVarPair : this->envIndexToReducableVar) {
    reducableIndices.insert(indexVarPair.first);
  }
  for (auto envIndex : reducableIndices) {

    /*
     * Fetch the type of the current reducable variable.
     */
    auto varType = this->reducibleEnvTypes[envIndex];
    auto ptrType = PointerType::getUnqual(varType);

    /*
     * Define the type of the vectorized form of the reducable variable.
     */
    auto valuesInCacheLine =
        Architecture::getCacheLineBytes() / sizeof(int64_t);
    auto reduceArrType =
        ArrayType::get(varType, reducerCount * valuesInCacheLine);

    /*
     * Allocate the vectorized form of the current reducable variable on the
     * stack.
     */
    auto reduceArrAlloca =
        builder.CreateAlloca(reduceArrType,
                             nullptr,
                             std::string("reductionArrayLiveOut_").append(std::to_string(envIndex)));
    this->envIndexToVectorOfReducableVar[envIndex] = reduceArrAlloca;

    /*
     * Store the pointer of the vector of the reducable variable inside the
     * environment.
     */
    auto reduceArrPtrType = PointerType::getUnqual(reduceArrAlloca->getType());
    auto envPtr = fetchCastedEnvPtr(this->reducibleEnvArray, envIndex, reduceArrPtrType);
    builder.CreateStore(reduceArrAlloca, envPtr);

    /*
     * Compute and cache the pointer of each element of the vectorized variable.
     */
    for (auto i = 0u; i < reducerCount; ++i) {
      auto reducePtr = fetchCastedEnvPtr(reduceArrAlloca, i, ptrType);
      this->envIndexToReducableVar[envIndex].push_back(reducePtr);
    }
  }

  return;
}

Value * HeartBeatLoopEnvironmentBuilder::getEnvironmentVariable(uint32_t id) const {
  // Assumption: this function is only invoked to get the live-in environment variable
  assert(this->singleEnvIDToIndex.find(id) != this->singleEnvIDToIndex.end()
         && "The live-in environment variable is not included in the builder\n");
  auto ind = this->singleEnvIDToIndex.at(id);

  auto iter = envIndexToVar.find(ind);
  assert(iter != envIndexToVar.end());
  return (*iter).second;
}

BasicBlock * HeartBeatLoopEnvironmentBuilder::reduceLiveOutVariablesWithInitialValue(
    BasicBlock *bb,
    IRBuilder<> &builder,
    const std::unordered_map<uint32_t, Instruction::BinaryOps> &reducibleBinaryOps,
    const std::unordered_map<uint32_t, Value *> &initialValues) {
  if (initialValues.size() == 0) {
    return bb;
  }

  auto f = bb->getParent();
  assert(f != nullptr);

  auto reductionBodyBB = BasicBlock::Create(this->CXT, "ReductionBody", f);
  auto afterReductionBB = BasicBlock::Create(this->CXT, "AfterReduction", f, reductionBodyBB);
  builder.CreateBr(reductionBodyBB);

  IRBuilder<> reductionBodyBuilder{ reductionBodyBB };
  auto int32Type = IntegerType::get(reductionBodyBuilder.getContext(), 32);
  
  auto valuesInCacheLine = Architecture::getCacheLineBytes() / sizeof(int64_t);

  std::vector<Value *> loadedValues;
  for (auto envIDInitValue : initialValues) {
    auto envID = envIDInitValue.first;
    auto envIndex = this->reducibleEnvIDToIndex[envID];

    auto baseAddressOfReducedVar = this->envIndexToVectorOfReducableVar.at(envIndex);
    auto zeroV = cast<Value>(ConstantInt::get(int32Type, 0));
    auto effectiveAddressOfReducedVar = reductionBodyBuilder.CreateInBoundsGEP(
      baseAddressOfReducedVar,
      ArrayRef<Value *>({ zeroV, zeroV }));

    auto varType = this->reducibleEnvTypes[envIndex];
    auto ptrType = PointerType::getUnqual(varType);
    auto effectiveAddressOfReducedVarProperlyCasted =
        reductionBodyBuilder.CreateBitCast(effectiveAddressOfReducedVar, ptrType);

    loadedValues.push_back(reductionBodyBuilder.CreateLoad(effectiveAddressOfReducedVarProperlyCasted));

  }

  auto count = 0;
  for (auto envIDInitValue : initialValues) {
    auto envID = envIDInitValue.first;
    auto envIndex = this->reducibleEnvIDToIndex[envID];

    auto binOp = reducibleBinaryOps.at(envID);
    auto privateCurrentCopy = loadedValues[count];
    auto newAccumulatorValue = reductionBodyBuilder.CreateBinOp(binOp, envIDInitValue.second, privateCurrentCopy);
    this->envIndexToAccumulatedReducableVar[envIndex] = newAccumulatorValue;

    count++;
  }

  reductionBodyBuilder.CreateBr(afterReductionBB);

  return afterReductionBB;
}

Value * HeartBeatLoopEnvironmentBuilder::getAccumulatedReducedEnvironmentVariable(uint32_t id) const {
  assert(this->reducibleEnvIDToIndex.find(id) != this->reducibleEnvIDToIndex.end()
         && "The environment variable is not included in the builder\n");
  auto ind = this->reducibleEnvIDToIndex.at(id);

  auto iter = envIndexToAccumulatedReducableVar.find(ind);
  assert(iter != envIndexToAccumulatedReducableVar.end());
  return (*iter).second;
}