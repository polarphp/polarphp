//===--- LoadableByAddress.cpp - Lower PIL address-only types. ------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
// This pass lowers loadable PILTypes. On completion, the PILType of every
// function argument is an address instead of the type itself.
// This reduces the code size.
// Consequently, this pass is required for IRGen.
// It is a mandatory IRGen preparation pass (not a diagnostic pass).
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "loadable-address"

#include "polarphp/irgen/internal/FixedTypeInfo.h"
#include "polarphp/irgen/internal/IRGenMangler.h"
#include "polarphp/irgen/internal/IRGenModule.h"
#include "polarphp/irgen/internal/NativeConventionSchema.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/irgen/IRGenPILPasses.h"
#include "polarphp/pil/lang/DebugUtils.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace polar;
using namespace polar::irgen;

static GenericEnvironment *getGenericEnvironment(CanPILFunctionType loweredTy) {
   return loweredTy->getSubstGenericSignature()->getGenericEnvironment();
}

class LargePILTypeMapper {
public:
   LargePILTypeMapper() {}

public:
   PILType getNewPILType(GenericEnvironment *GenericEnv, PILType storageType,
                         irgen::IRGenModule &Mod);
   bool shouldTransformResults(GenericEnvironment *env,
                               CanPILFunctionType fnType,
                               irgen::IRGenModule &IGM);
   bool shouldTransformFunctionType(GenericEnvironment *env,
                                    CanPILFunctionType fnType,
                                    irgen::IRGenModule &IGM);
   PILParameterInfo getNewParameter(GenericEnvironment *env,
                                    PILParameterInfo param,
                                    irgen::IRGenModule &IGM);
   bool shouldTransformParameter(GenericEnvironment *env, PILParameterInfo param,
                                 irgen::IRGenModule &IGM);
   SmallVector<PILParameterInfo, 4> getNewParameters(GenericEnvironment *env,
                                                     CanPILFunctionType fnType,
                                                     irgen::IRGenModule &IGM);
   SmallVector<PILYieldInfo, 2> getNewYields(GenericEnvironment *env,
                                             CanPILFunctionType fnType,
                                             irgen::IRGenModule &IGM);
   SmallVector<PILResultInfo, 2> getNewResults(GenericEnvironment *GenericEnv,
                                               CanPILFunctionType fnType,
                                               irgen::IRGenModule &Mod);
   CanPILFunctionType getNewPILFunctionType(GenericEnvironment *env,
                                            CanPILFunctionType fnType,
                                            irgen::IRGenModule &IGM);
   PILType getNewOptionalFunctionType(GenericEnvironment *GenericEnv,
                                      PILType storageType,
                                      irgen::IRGenModule &Mod);
   PILType getNewTupleType(GenericEnvironment *GenericEnv,
                           irgen::IRGenModule &Mod,
                           const PILType &nonOptionalType,
                           const PILType &storageType);
   bool newResultsDiffer(GenericEnvironment *GenericEnv,
                         ArrayRef<PILResultInfo> origResults,
                         irgen::IRGenModule &Mod);
   bool shouldConvertBBArg(PILArgument *arg, irgen::IRGenModule &Mod);
   bool containsDifferentFunctionSignature(GenericEnvironment *genEnv,
                                           irgen::IRGenModule &Mod,
                                           PILType storageType,
                                           PILType newPILType);

private:
   // Cache of already computed type transforms
   llvm::MapVector<std::pair<GenericEnvironment *, PILType>, PILType>
      oldToNewTypeMap;
};

/// Utility to determine if this is a large loadable type
static bool isLargeLoadableType(GenericEnvironment *GenericEnv, PILType t,
                                irgen::IRGenModule &Mod) {
   if (t.isAddress() || t.isClassOrClassMetatype()) {
      return false;
   }

   auto canType = t.getAstType();
   if (canType->hasTypeParameter()) {
      assert(GenericEnv && "Expected a GenericEnv");
      canType = GenericEnv->mapTypeIntoContext(canType)->getCanonicalType();
   }

   if (canType.getAnyGeneric()) {
      assert(t.isObject() && "Expected only two categories: address and object");
      assert(!canType->hasTypeParameter());
      const TypeInfo &TI = Mod.getTypeInfoForLowered(canType);
      auto &nativeSchemaOrigParam = TI.nativeParameterValueSchema(Mod);
      return nativeSchemaOrigParam.requiresIndirect();
   }
   return false;
}

static bool modifiableFunction(CanPILFunctionType funcType) {
   if (funcType->getLanguage() == PILFunctionLanguage::C) {
      // C functions should use the old ABI
      return false;
   }
   return true;
}

bool LargePILTypeMapper::shouldTransformParameter(GenericEnvironment *env,
                                                  PILParameterInfo param,
                                                  irgen::IRGenModule &IGM) {
   auto newParam = getNewParameter(env, param, IGM);
   return (param != newParam);
}

static bool isFuncOrOptionalFuncType(PILType Ty) {
   PILType nonOptionalType = Ty;
   if (auto optType = Ty.getOptionalObjectType()) {
      nonOptionalType = optType;
   }
   return nonOptionalType.is<PILFunctionType>();
}

bool LargePILTypeMapper::shouldTransformFunctionType(GenericEnvironment *env,
                                                     CanPILFunctionType fnType,
                                                     irgen::IRGenModule &IGM) {
   if (shouldTransformResults(env, fnType, IGM))
      return true;

   for (auto param : fnType->getParameters()) {
      if (shouldTransformParameter(env, param, IGM))
         return true;
   }

   for (auto yield : fnType->getYields()) {
      if (shouldTransformParameter(env, yield, IGM))
         return true;
   }

   return false;
}

// Get the function type or the optional function type
static CanPILFunctionType getInnerFunctionType(PILType storageType) {
   if (auto currPILFunctionType = storageType.getAs<PILFunctionType>()) {
      return currPILFunctionType;
   }
   if (auto optionalType = storageType.getOptionalObjectType()) {
      if (auto currPILFunctionType = optionalType.getAs<PILFunctionType>()) {
         return currPILFunctionType;
      }
   }
   return CanPILFunctionType();
}

static PILType getNonOptionalType(PILType t) {
   PILType nonOptionalType = t;
   if (auto optType = t.getOptionalObjectType()) {
      nonOptionalType = optType;
   }
   return nonOptionalType;
}

bool LargePILTypeMapper::containsDifferentFunctionSignature(
   GenericEnvironment *genEnv, irgen::IRGenModule &Mod, PILType storageType,
   PILType newPILType) {
   if (storageType == newPILType) {
      return false;
   }
   if (getInnerFunctionType(storageType)) {
      return true;
   }
   PILType nonOptionalType = getNonOptionalType(storageType);
   if (nonOptionalType.getAs<TupleType>()) {
      auto origType = nonOptionalType.getAs<TupleType>();
      for (TupleTypeElt canElem : origType->getElements()) {
         auto origCanType = CanType(canElem.getRawType());
         auto elem = PILType::getPrimitiveObjectType(origCanType);
         auto newElem = getNewPILType(genEnv, elem, Mod);
         if (containsDifferentFunctionSignature(genEnv, Mod, elem, newElem)) {
            return true;
         }
      }
   }
   return false;
}

bool LargePILTypeMapper::newResultsDiffer(GenericEnvironment *GenericEnv,
                                          ArrayRef<PILResultInfo> origResults,
                                          irgen::IRGenModule &Mod) {
   SmallVector<PILResultInfo, 2> newResults;
   for (auto result : origResults) {
      PILType currResultTy = result.getPILStorageInterfaceType();
      PILType newPILType = getNewPILType(GenericEnv, currResultTy, Mod);
      // We (currently) only care about function signatures
      if (containsDifferentFunctionSignature(GenericEnv, Mod, currResultTy,
                                             newPILType)) {
         return true;
      }
   }
   return false;
}

static bool modNonFuncTypeResultType(GenericEnvironment *genEnv,
                                     CanPILFunctionType loweredTy,
                                     irgen::IRGenModule &Mod) {
   if (!modifiableFunction(loweredTy)) {
      return false;
   }
   if (loweredTy->getNumResults() != 1) {
      return false;
   }
   auto singleResult = loweredTy->getSingleResult();
   auto resultStorageType = singleResult.getPILStorageInterfaceType();
   if (isLargeLoadableType(genEnv, resultStorageType, Mod)) {
      return true;
   }
   return false;
}

SmallVector<PILResultInfo, 2>
LargePILTypeMapper::getNewResults(GenericEnvironment *GenericEnv,
                                  CanPILFunctionType fnType,
                                  irgen::IRGenModule &Mod) {
   // Get new PIL Function results - same as old results UNLESS:
   // 1) Function type results might have a different signature
   // 2) Large loadables are replaced by @out version
   auto origResults = fnType->getResults();
   SmallVector<PILResultInfo, 2> newResults;
   for (auto result : origResults) {
      PILType currResultTy = result.getPILStorageInterfaceType();
      PILType newPILType = getNewPILType(GenericEnv, currResultTy, Mod);
      if (modNonFuncTypeResultType(GenericEnv, fnType, Mod)) {
         // Case (2) Above
         PILResultInfo newPILResultInfo(newPILType.getAstType(),
                                        ResultConvention::Indirect);
         newResults.push_back(newPILResultInfo);
      } else if (containsDifferentFunctionSignature(GenericEnv, Mod, currResultTy,
                                                    newPILType)) {
         // Case (1) Above
         PILResultInfo newResult(newPILType.getAstType(), result.getConvention());
         newResults.push_back(newResult);
      } else {
         newResults.push_back(result);
      }
   }
   return newResults;
}

CanPILFunctionType
LargePILTypeMapper::getNewPILFunctionType(GenericEnvironment *env,
                                          CanPILFunctionType fnType,
                                          irgen::IRGenModule &IGM) {
   if (!modifiableFunction(fnType)) {
      return fnType;
   }
   auto newParams = getNewParameters(env, fnType, IGM);
   auto newYields = getNewYields(env, fnType, IGM);
   auto newResults = getNewResults(env, fnType, IGM);
   auto newFnType = PILFunctionType::get(
      fnType->getSubstGenericSignature(),
      fnType->getExtInfo(),
      fnType->getCoroutineKind(),
      fnType->getCalleeConvention(),
      newParams,
      newYields,
      newResults,
      fnType->getOptionalErrorResult(),
      fnType->getSubstitutions(),
      fnType->isGenericSignatureImplied(),
      fnType->getAstContext(),
      fnType->getWitnessMethodConformanceOrInvalid());
   return newFnType;
}

PILType
LargePILTypeMapper::getNewOptionalFunctionType(GenericEnvironment *GenericEnv,
                                               PILType storageType,
                                               irgen::IRGenModule &Mod) {
   PILType newPILType = storageType;
   if (auto objectType = storageType.getOptionalObjectType()) {
      if (auto fnType = objectType.getAs<PILFunctionType>()) {
         if (shouldTransformFunctionType(GenericEnv, fnType, Mod)) {
            auto newFnType = getNewPILFunctionType(GenericEnv, fnType, Mod);
            newPILType =
               PILType::getPrimitiveType(newFnType, storageType.getCategory());
            newPILType = PILType::getOptionalType(newPILType);
         }
      }
   }
   return newPILType;
}

bool LargePILTypeMapper::shouldTransformResults(GenericEnvironment *genEnv,
                                                CanPILFunctionType loweredTy,
                                                irgen::IRGenModule &Mod) {
   if (!modifiableFunction(loweredTy)) {
      return false;
   }

   if (loweredTy->getNumResults() != 1) {
      auto resultType = loweredTy->getAllResultsInterfaceType();
      auto newResultType = getNewPILType(genEnv, resultType, Mod);
      return resultType != newResultType;
   }

   auto singleResult = loweredTy->getSingleResult();
   auto resultStorageType = singleResult.getPILStorageInterfaceType();
   auto newResultStorageType = getNewPILType(genEnv, resultStorageType, Mod);
   if (resultStorageType != newResultStorageType) {
      return true;
   }
   return modNonFuncTypeResultType(genEnv, loweredTy, Mod);
}

static bool modResultType(PILFunction *F, irgen::IRGenModule &Mod,
                          LargePILTypeMapper &Mapper) {
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   auto loweredTy = F->getLoweredFunctionType();

   return Mapper.shouldTransformResults(genEnv, loweredTy, Mod);
}

static bool shouldTransformYields(GenericEnvironment *genEnv,
                                  CanPILFunctionType loweredTy,
                                  irgen::IRGenModule &Mod,
                                  LargePILTypeMapper &Mapper) {
   if (!modifiableFunction(loweredTy)) {
      return false;
   }
   for (auto &yield : loweredTy->getYields()) {
      auto yieldStorageType = yield.getPILStorageInterfaceType();
      auto newYieldStorageType =
         Mapper.getNewPILType(genEnv, yieldStorageType, Mod);
      if (yieldStorageType != newYieldStorageType)
         return true;
   }
   return false;
}

static bool modYieldType(PILFunction *F, irgen::IRGenModule &Mod,
                         LargePILTypeMapper &Mapper) {
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   auto loweredTy = F->getLoweredFunctionType();

   return shouldTransformYields(genEnv, loweredTy, Mod, Mapper);
}

PILParameterInfo LargePILTypeMapper::getNewParameter(GenericEnvironment *env,
                                                     PILParameterInfo param,
                                                     irgen::IRGenModule &IGM) {
   PILType storageType = param.getPILStorageInterfaceType();
   PILType newOptFuncType =
      getNewOptionalFunctionType(env, storageType, IGM);
   if (newOptFuncType != storageType) {
      return param.getWithInterfaceType(newOptFuncType.getAstType());
   }

   if (auto paramFnType = storageType.getAs<PILFunctionType>()) {
      if (shouldTransformFunctionType(env, paramFnType, IGM)) {
         auto newFnType = getNewPILFunctionType(env, paramFnType, IGM);
         return param.getWithInterfaceType(newFnType);
      } else {
         return param;
      }
   } else if (isLargeLoadableType(env, storageType, IGM)) {
      if (param.getConvention() == ParameterConvention::Direct_Guaranteed)
         return PILParameterInfo(storageType.getAstType(),
                                 ParameterConvention::Indirect_In_Guaranteed);
      else
         return PILParameterInfo(storageType.getAstType(),
                                 ParameterConvention::Indirect_In_Constant);
   } else {
      auto newType = getNewPILType(env, storageType, IGM);
      return PILParameterInfo(newType.getAstType(),
                              param.getConvention());
   }
}

SmallVector<PILParameterInfo, 4>
LargePILTypeMapper::getNewParameters(GenericEnvironment *env,
                                     CanPILFunctionType fnType,
                                     irgen::IRGenModule &IGM) {
   SmallVector<PILParameterInfo, 4> newParams;
   for (PILParameterInfo param : fnType->getParameters()) {
      auto newParam = getNewParameter(env, param, IGM);
      newParams.push_back(newParam);
   }
   return newParams;
}

SmallVector<PILYieldInfo, 2>
LargePILTypeMapper::getNewYields(GenericEnvironment *env,
                                 CanPILFunctionType fnType,
                                 irgen::IRGenModule &IGM) {
   SmallVector<PILYieldInfo, 2> newYields;
   for (auto oldYield : fnType->getYields()) {
      auto newYieldAsParam = getNewParameter(env, oldYield, IGM);
      newYields.push_back(PILYieldInfo(newYieldAsParam.getInterfaceType(),
                                       newYieldAsParam.getConvention()));
   }
   return newYields;
}

PILType LargePILTypeMapper::getNewTupleType(GenericEnvironment *GenericEnv,
                                            irgen::IRGenModule &Mod,
                                            const PILType &nonOptionalType,
                                            const PILType &storageType) {
   auto origType = nonOptionalType.getAs<TupleType>();
   assert(origType && "Expected a tuple type");
   SmallVector<TupleTypeElt, 2> newElems;
   for (TupleTypeElt canElem : origType->getElements()) {
      auto origCanType = CanType(canElem.getRawType());
      auto elem = PILType::getPrimitiveObjectType(origCanType);
      auto newElem = getNewPILType(GenericEnv, elem, Mod);
      auto newTupleType =
         TupleTypeElt(newElem.getAstType(), canElem.getName(),
                      canElem.getParameterFlags());
      newElems.push_back(newTupleType);
   }
   auto type = TupleType::get(newElems, nonOptionalType.getAstContext());
   auto canType = CanType(type);
   PILType newPILType = PILType::getPrimitiveObjectType(canType);
   if (nonOptionalType.isAddress()) {
      newPILType = newPILType.getAddressType();
   }
   if (nonOptionalType != storageType) {
      newPILType = PILType::getOptionalType(newPILType);
   }
   if (storageType.isAddress()) {
      newPILType = newPILType.getAddressType();
   }
   return newPILType;
}

PILType LargePILTypeMapper::getNewPILType(GenericEnvironment *GenericEnv,
                                          PILType storageType,
                                          irgen::IRGenModule &Mod) {
   // See if the type is already in the cache:
   auto typePair = std::make_pair(GenericEnv, storageType);
   if (oldToNewTypeMap.find(typePair) != oldToNewTypeMap.end()) {
      return oldToNewTypeMap[typePair];
   }

   PILType nonOptionalType = storageType;
   if (auto optType = storageType.getOptionalObjectType()) {
      nonOptionalType = optType;
   }
   if (nonOptionalType.getAs<TupleType>()) {
      PILType newPILType =
         getNewTupleType(GenericEnv, Mod, nonOptionalType, storageType);
      auto typeToRet = isLargeLoadableType(GenericEnv, newPILType, Mod)
                       ? newPILType.getAddressType()
                       : newPILType;
      oldToNewTypeMap[typePair] = typeToRet;
      return typeToRet;
   }
   PILType newPILType = getNewOptionalFunctionType(GenericEnv, storageType, Mod);
   if (newPILType != storageType) {
      oldToNewTypeMap[typePair] = newPILType;
      return newPILType;
   }
   if (auto fnType = storageType.getAs<PILFunctionType>()) {
      if (shouldTransformFunctionType(GenericEnv, fnType, Mod)) {
         auto newFnType = getNewPILFunctionType(GenericEnv, fnType, Mod);
         newPILType = PILType::getPrimitiveType(newFnType,
                                                storageType.getCategory());
      }
   } else if (isLargeLoadableType(GenericEnv, storageType, Mod)) {
      newPILType = storageType.getAddressType();
   }
   oldToNewTypeMap[typePair] = newPILType;
   return newPILType;
}

//===----------------------------------------------------------------------===//
// StructLoweringState: shared state for the pass's analysis and transforms.
//===----------------------------------------------------------------------===//

namespace {
struct StructLoweringState {
   PILFunction *F;
   irgen::IRGenModule &Mod;
   LargePILTypeMapper &Mapper;

   // All large loadable function arguments that we modified
   SmallVector<PILValue, 16> largeLoadableArgs;
   // All modified function signature function arguments
   SmallVector<PILValue, 16> funcSigArgs;
   // All args for which we did a load
   llvm::MapVector<PILValue, PILValue> argsToLoadedValueMap;
   // All applies for which we did an alloc
   llvm::MapVector<PILInstruction *, PILValue> applyRetToAllocMap;
   // recerse map of the one above
   llvm::MapVector<PILInstruction *, PILInstruction *> allocToApplyRetMap;
   // All call sites with PILArgument that needs to be re-written
   // Calls are removed from the set when rewritten.
   SmallVector<PILInstruction *, 16> applies;
   // All MethodInst that use the large struct
   SmallVector<MethodInst *, 16> methodInstsToMod;
   // Large loadable store instrs should call the outlined copy
   SmallVector<StoreInst *, 16> storeInstsToMod;
   // All switch_enum instrs that should be converted to switch_enum_addr
   SmallVector<SwitchEnumInst *, 16> switchEnumInstsToMod;
   // All struct_extract instrs that should be converted to struct_element_addr
   SmallVector<StructExtractInst *, 16> structExtractInstsToMod;
   // All tuple instructions for which the return type is a function type
   SmallVector<SingleValueInstruction *, 8> tupleInstsToMod;
   // All allock stack instructions to modify
   SmallVector<AllocStackInst *, 8> allocStackInstsToMod;
   // All pointer to address instructions to modify
   SmallVector<PointerToAddressInst *, 8> pointerToAddrkInstsToMod;
   // All Retain and release instrs should be replaced with _addr version
   SmallVector<RetainValueInst *, 16> retainInstsToMod;
   SmallVector<ReleaseValueInst *, 16> releaseInstsToMod;
   // All result types instrs for which we need to convert the ResultTy
   llvm::SetVector<SingleValueInstruction *> resultTyInstsToMod;
   // All instructions that use the large struct that are not covered above
   SmallVector<PILInstruction *, 16> instsToMod;
   // All function-exiting terminators (return or throw instructions).
   SmallVector<TermInst *, 8> returnInsts;
   // All (large type) return instructions that are modified
   SmallVector<ReturnInst *, 8> modReturnInsts;
   // All (large type) yield instructions that are modified
   SmallVector<YieldInst *, 8> modYieldInsts;
   // All destroy_value instrs should be replaced with _addr version
   SmallVector<PILInstruction *, 16> destroyValueInstsToMod;
   // All debug instructions.
   // to be modified *only if* the operands are used in "real" instructions
   SmallVector<DebugValueInst *, 16> debugInstsToMod;

   StructLoweringState(PILFunction *F, irgen::IRGenModule &Mod,
                       LargePILTypeMapper &Mapper)
      : F(F), Mod(Mod), Mapper(Mapper) {}

   bool isLargeLoadableType(PILType type) {
      return ::isLargeLoadableType(F->getGenericEnvironment(), type, Mod);
   }

   PILType getNewPILType(PILType type) {
      return Mapper.getNewPILType(F->getGenericEnvironment(), type, Mod);
   }

   bool containsDifferentFunctionSignature(PILType type) {
      return Mapper.containsDifferentFunctionSignature(
         F->getGenericEnvironment(), Mod, type, getNewPILType(type));
   }

   bool hasLargeLoadableYields() {
      auto fnType = F->getLoweredFunctionType();
      if (!fnType->isCoroutine()) return false;

      auto env = F->getGenericEnvironment();
      for (auto yield : fnType->getYields()) {
         if (Mapper.shouldTransformParameter(env, yield, Mod))
            return true;
      }
      return false;
   }
};
} // end anonymous namespace

//===----------------------------------------------------------------------===//
// LargeValueVisitor: Map large loadable values to ValueStorage.
//===----------------------------------------------------------------------===//

namespace {
class LargeValueVisitor {
   StructLoweringState &pass;
   PostOrderFunctionInfo postorderInfo;

public:
   explicit LargeValueVisitor(StructLoweringState &pass)
      : pass(pass), postorderInfo(pass.F) {}

   void mapReturnInstrs();
   void mapValueStorage();

protected:
   void visitApply(ApplySite applySite);
   void visitMethodInst(MethodInst *instr);
   void visitStoreInst(StoreInst *instr);
   void visitSwitchEnumInst(SwitchEnumInst *instr);
   void visitStructExtractInst(StructExtractInst *instr);
   void visitRetainInst(RetainValueInst *instr);
   void visitReleaseInst(ReleaseValueInst *instr);
   void visitResultTyInst(SingleValueInstruction *instr);
   void visitDebugValueInst(DebugValueInst *instr);
   void visitDestroyValueInst(DestroyValueInst *instr);
   void visitTupleInst(SingleValueInstruction *instr);
   void visitAllocStackInst(AllocStackInst *instr);
   void visitPointerToAddressInst(PointerToAddressInst *instr);
   void visitReturnInst(ReturnInst *instr);
   void visitYieldInst(YieldInst *instr);
   void visitDeallocInst(DeallocStackInst *instr);
   void visitInstr(PILInstruction *instr);
};
} // end anonymous namespace

void LargeValueVisitor::mapReturnInstrs() {
   for (auto *BB : postorderInfo.getReversePostOrder()) {
      if (BB->getTerminator()->isFunctionExiting())
         pass.returnInsts.push_back(BB->getTerminator());
   }
}

void LargeValueVisitor::mapValueStorage() {
   for (auto *BB : postorderInfo.getReversePostOrder()) {
      for (auto &II : *BB) {
         PILInstruction *currIns = &II;
         switch (currIns->getKind()) {
            case PILInstructionKind::ApplyInst:
            case PILInstructionKind::TryApplyInst:
            case PILInstructionKind::BeginApplyInst:
            case PILInstructionKind::PartialApplyInst: {
               visitApply(ApplySite(currIns));
               break;
            }
            case PILInstructionKind::ClassMethodInst:
            case PILInstructionKind::SuperMethodInst:
            case PILInstructionKind::ObjCMethodInst:
            case PILInstructionKind::ObjCSuperMethodInst:
            case PILInstructionKind::WitnessMethodInst: {
               // TODO Any more instructions to add here?
               auto *MI = cast<MethodInst>(currIns);
               visitMethodInst(MI);
               break;
            }
            case PILInstructionKind::StructExtractInst:
            case PILInstructionKind::StructElementAddrInst:
            case PILInstructionKind::RefTailAddrInst:
            case PILInstructionKind::RefElementAddrInst:
            case PILInstructionKind::BeginAccessInst:
            case PILInstructionKind::EnumInst: {
               // TODO Any more instructions to add here?
               visitResultTyInst(cast<SingleValueInstruction>(currIns));
               break;
            }
            case PILInstructionKind::StoreInst: {
               auto *SI = cast<StoreInst>(currIns);
               visitStoreInst(SI);
               break;
            }
            case PILInstructionKind::RetainValueInst: {
               auto *RETI = cast<RetainValueInst>(currIns);
               visitRetainInst(RETI);
               break;
            }
            case PILInstructionKind::ReleaseValueInst: {
               auto *RELI = cast<ReleaseValueInst>(currIns);
               visitReleaseInst(RELI);
               break;
            }
            case PILInstructionKind::DebugValueInst: {
               auto *DI = cast<DebugValueInst>(currIns);
               visitDebugValueInst(DI);
               break;
            }
            case PILInstructionKind::DestroyValueInst: {
               auto *DI = cast<DestroyValueInst>(currIns);
               visitDestroyValueInst(DI);
               break;
            }
            case PILInstructionKind::SwitchEnumInst: {
               auto *SEI = cast<SwitchEnumInst>(currIns);
               visitSwitchEnumInst(SEI);
               break;
            }
            case PILInstructionKind::TupleElementAddrInst:
            case PILInstructionKind::TupleExtractInst: {
               visitTupleInst(cast<SingleValueInstruction>(currIns));
               break;
            }
            case PILInstructionKind::AllocStackInst: {
               auto *ASI = cast<AllocStackInst>(currIns);
               visitAllocStackInst(ASI);
               break;
            }
            case PILInstructionKind::PointerToAddressInst: {
               auto *PTA = cast<PointerToAddressInst>(currIns);
               visitPointerToAddressInst(PTA);
               break;
            }
            case PILInstructionKind::ReturnInst: {
               auto *RI = cast<ReturnInst>(currIns);
               visitReturnInst(RI);
               break;
            }
            case PILInstructionKind::YieldInst: {
               auto *YI = cast<YieldInst>(currIns);
               visitYieldInst(YI);
               break;
            }
            case PILInstructionKind::DeallocStackInst: {
               auto *DI = cast<DeallocStackInst>(currIns);
               visitDeallocInst(DI);
               break;
            }
            default: {
               assert(!ApplySite::isa(currIns) && "Did not expect an ApplySite");
               assert(!isa<MethodInst>(currIns) && "Unhandled Method Inst");
               visitInstr(currIns);
               break;
            }
         }
      }
   }
}

static bool modifiableApply(ApplySite applySite, irgen::IRGenModule &Mod) {
   // If the callee is a method then use the old ABI
   if (applySite.getSubstCalleeType()->getLanguage() == PILFunctionLanguage::C) {
      return false;
   }
   PILValue callee = applySite.getCallee();
   if (auto site = ApplySite::isa(callee)) {
      return modifiableApply(site, Mod);
   }
   return true;
}

void LargeValueVisitor::visitApply(ApplySite applySite) {
   if (!modifiableApply(applySite, pass.Mod)) {
      return visitInstr(applySite.getInstruction());
   }
   for (Operand &operand : applySite.getArgumentOperands()) {
      PILValue currOperand = operand.get();
      PILType silType = currOperand->getType();
      PILType newSilType = pass.getNewPILType(silType);
      if (silType != newSilType ||
          std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                    currOperand) != pass.largeLoadableArgs.end() ||
          std::find(pass.funcSigArgs.begin(), pass.funcSigArgs.end(),
                    currOperand) != pass.funcSigArgs.end()) {
         pass.applies.push_back(applySite.getInstruction());
         return;
      }
   }

   // For coroutines, we need to consider the yields, not the direct result
   // (which should always be void).
   if (auto beginApply = dyn_cast<BeginApplyInst>(applySite)) {
      for (auto yield : beginApply->getYieldedValues()) {
         auto oldYieldType = yield->getType();
         auto newYieldType = pass.getNewPILType(oldYieldType);
         if (oldYieldType != newYieldType) {
            pass.applies.push_back(applySite.getInstruction());
            return;
         }
      }
      return;
   }

   PILType currType = applySite.getType();
   PILType newType = pass.getNewPILType(currType);
   // We only care about function type results
   if (!pass.isLargeLoadableType(currType) &&
       (currType != newType)) {
      pass.applies.push_back(applySite.getInstruction());
      return;
   }
   // Check callee - need new generic env:
   CanPILFunctionType origPILFunctionType = applySite.getSubstCalleeType();
   GenericEnvironment *genEnvCallee = nullptr;
   auto newPILFunctionType = pass.Mapper.getNewPILFunctionType(
      genEnvCallee, origPILFunctionType, pass.Mod);
   if (origPILFunctionType != newPILFunctionType) {
      pass.applies.push_back(applySite.getInstruction());
   }
}

static bool isMethodInstUnmodifiable(MethodInst *instr) {
   for (auto *user : instr->getUses()) {
      if (ApplySite::isa(user->getUser())) {
         ApplySite applySite = ApplySite(user->getUser());
         if (applySite.getSubstCalleeType()->getLanguage() ==
             PILFunctionLanguage::C) {
            return true;
         }
      }
   }
   return false;
}

void LargeValueVisitor::visitMethodInst(MethodInst *instr) {
   if (isMethodInstUnmodifiable(instr)) {
      // Do not change the method!
      visitInstr(instr);
      return;
   }
   PILType currPILType = instr->getType();
   auto fnType = currPILType.castTo<PILFunctionType>();

   GenericEnvironment *genEnv = nullptr;
   if (fnType->isPolymorphic()) {
      genEnv = getGenericEnvironment(fnType);
   }
   if (pass.Mapper.shouldTransformFunctionType(genEnv, fnType, pass.Mod)) {
      pass.methodInstsToMod.push_back(instr);
      return;
   }
   if (pass.Mapper.newResultsDiffer(genEnv, fnType->getResults(), pass.Mod)) {
      pass.methodInstsToMod.push_back(instr);
   }
}

void LargeValueVisitor::visitStoreInst(StoreInst *instr) {
   PILValue src = instr->getSrc();
   if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                 src) != pass.largeLoadableArgs.end()) {
      pass.storeInstsToMod.push_back(instr);
   }
}

bool LargePILTypeMapper::shouldConvertBBArg(PILArgument *arg,
                                            irgen::IRGenModule &Mod) {
   auto *F = arg->getFunction();
   PILType storageType = arg->getType();
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   auto currCanType = storageType.getAstType();
   if (auto funcType = dyn_cast<PILFunctionType>(currCanType)) {
      if (funcType->isPolymorphic()) {
         genEnv = getGenericEnvironment(funcType);
      }
   }
   PILType newPILType = getNewPILType(genEnv, storageType, Mod);
   // We (currently) only care about function signatures
   if (containsDifferentFunctionSignature(genEnv, Mod, storageType,
                                          newPILType)) {
      return true;
   }
   return false;
}

void LargeValueVisitor::visitSwitchEnumInst(SwitchEnumInst *instr) {
   PILValue operand = instr->getOperand();
   if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                 operand) != pass.largeLoadableArgs.end()) {
      pass.switchEnumInstsToMod.push_back(instr);
      return;
   }
   // In case we converted the target BB type of this enum,
   // to an address based one - need to modify
   unsigned numOfCases = instr->getNumCases();
   SmallVector<std::pair<EnumElementDecl *, PILBasicBlock *>, 16> caseBBs;
   for (unsigned i = 0; i < numOfCases; ++i) {
      auto currCase = instr->getCase(i);
      auto *currBB = currCase.second;
      for (PILArgument *arg : currBB->getArguments()) {
         if (pass.Mapper.shouldConvertBBArg(arg, pass.Mod)) {
            PILType storageType = arg->getType();
            PILType newPILType = pass.getNewPILType(storageType);
            if (newPILType.isAddress()) {
               pass.switchEnumInstsToMod.push_back(instr);
               return;
            }
         }
      }
   }
}

void LargeValueVisitor::visitStructExtractInst(StructExtractInst *instr) {
   PILValue operand = instr->getOperand();
   if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                 operand) != pass.largeLoadableArgs.end()) {
      pass.structExtractInstsToMod.push_back(instr);
   }
}

void LargeValueVisitor::visitRetainInst(RetainValueInst *instr) {
   for (Operand &operand : instr->getAllOperands()) {
      if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                    operand.get()) != pass.largeLoadableArgs.end()) {
         pass.retainInstsToMod.push_back(instr);
         return;
      }
   }
}

void LargeValueVisitor::visitReleaseInst(ReleaseValueInst *instr) {
   for (Operand &operand : instr->getAllOperands()) {
      if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                    operand.get()) != pass.largeLoadableArgs.end()) {
         pass.releaseInstsToMod.push_back(instr);
         return;
      }
   }
}

void LargeValueVisitor::visitDebugValueInst(DebugValueInst *instr) {
   for (Operand &operand : instr->getAllOperands()) {
      if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                    operand.get()) != pass.largeLoadableArgs.end()) {
         pass.debugInstsToMod.push_back(instr);
      }
   }
}

void LargeValueVisitor::visitDestroyValueInst(DestroyValueInst *instr) {
   for (Operand &operand : instr->getAllOperands()) {
      if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                    operand.get()) != pass.largeLoadableArgs.end()) {
         pass.destroyValueInstsToMod.push_back(instr);
      }
   }
}

void LargeValueVisitor::visitResultTyInst(SingleValueInstruction *instr) {
   PILType currPILType = instr->getType().getObjectType();
   PILType newPILType = pass.getNewPILType(currPILType);
   if (currPILType != newPILType) {
      pass.resultTyInstsToMod.insert(instr);
   }
   auto *SEI = dyn_cast<StructExtractInst>(instr);
   if (SEI) {
      visitStructExtractInst(SEI);
   } else {
      visitInstr(instr);
   }
}

void LargeValueVisitor::visitTupleInst(SingleValueInstruction *instr) {
   PILType currPILType = instr->getType().getObjectType();
   if (auto funcType = getInnerFunctionType(currPILType)) {
      GenericEnvironment *genEnv = instr->getFunction()->getGenericEnvironment();
      if (!genEnv && funcType->isPolymorphic()) {
         genEnv = getGenericEnvironment(funcType);
      }
      auto newPILFunctionType =
         pass.Mapper.getNewPILFunctionType(genEnv, funcType, pass.Mod);
      if (funcType != newPILFunctionType) {
         pass.tupleInstsToMod.push_back(instr);
      }
   }
   visitInstr(instr);
}

void LargeValueVisitor::visitAllocStackInst(AllocStackInst *instr) {
   PILType currPILType = instr->getType().getObjectType();
   if (getInnerFunctionType(currPILType)) {
      pass.allocStackInstsToMod.push_back(instr);
   }
}

void LargeValueVisitor::visitPointerToAddressInst(PointerToAddressInst *instr) {
   PILType currPILType = instr->getType().getObjectType();
   if (getInnerFunctionType(currPILType)) {
      pass.pointerToAddrkInstsToMod.push_back(instr);
   }
}

static bool modNonFuncTypeResultType(PILFunction *F, irgen::IRGenModule &Mod) {
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   auto loweredTy = F->getLoweredFunctionType();
   return modNonFuncTypeResultType(genEnv, loweredTy, Mod);
}

void LargeValueVisitor::visitReturnInst(ReturnInst *instr) {
   if (!modResultType(pass.F, pass.Mod, pass.Mapper)) {
      visitInstr(instr);
   } else if (modNonFuncTypeResultType(pass.F, pass.Mod)) {
      pass.modReturnInsts.push_back(instr);
   } // else: function signature return instructions remain as-is
}

void LargeValueVisitor::visitYieldInst(YieldInst *instr) {
   if (!modYieldType(pass.F, pass.Mod, pass.Mapper)) {
      visitInstr(instr);
   } else {
      pass.modYieldInsts.push_back(instr);
   } // else: function signature return instructions remain as-is
}

void LargeValueVisitor::visitDeallocInst(DeallocStackInst *instr) {
   auto opInstr = instr->getOperand();
   if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                 opInstr) != pass.largeLoadableArgs.end()) {
      auto *opAsInstr = dyn_cast<AllocStackInst>(opInstr);
      assert(opAsInstr && "Expected an alloc stack instruction");
      assert(pass.allocToApplyRetMap.find(opAsInstr) !=
             pass.allocToApplyRetMap.end() &&
             "Unexpected dealloc instr!");
      (void)opAsInstr;
   }
}

void LargeValueVisitor::visitInstr(PILInstruction *instr) {
   for (Operand &operand : instr->getAllOperands()) {
      if (std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                    operand.get()) != pass.largeLoadableArgs.end()) {
         pass.instsToMod.push_back(instr);
         // will be replaced later by the load / alloc_stack:
         pass.argsToLoadedValueMap[operand.get()] = operand.get();
      }
   }
}

//===----------------------------------------------------------------------===//
// LoadableStorageAllocation: Generate alloc_stack and address projections
// for all loadable types we pass around.
//===----------------------------------------------------------------------===//

namespace {
class LoadableStorageAllocation {
public:
   StructLoweringState &pass;

   explicit LoadableStorageAllocation(StructLoweringState &pass) : pass(pass) {}

   void allocateLoadableStorage();
   void replaceLoad(LoadInst *unoptimizableLoad);

protected:
   void replaceLoadWithCopyAddr(LoadInst *optimizableLoad);
   void replaceLoadWithCopyAddrForModifiable(LoadInst *unoptimizableLoad);
   void convertIndirectFunctionArgs();
   void insertIndirectReturnArgs();
   void convertIndirectFunctionPointerArgsForUnmodifiable();
   void convertIndirectBasicBlockArgs();
   void convertApplyResults();
   void allocateForArg(PILValue value);
   AllocStackInst *allocateForApply(PILInstruction *apply, PILType type);
   PILArgument *replaceArgType(PILBuilder &argBuilder, PILArgument *arg,
                               PILType newPILType);
};
} // end anonymous namespace

static AllocStackInst *allocate(StructLoweringState &pass, PILType type) {
   assert(type.isObject());

   // Insert an alloc_stack at the beginning of the function.
   PILBuilderWithScope allocBuilder(&*pass.F->begin());
   // Don't put any variable debug info into the alloc_stack, there will be a
   // debug_value_addr insterted later. TODO: It may be more elegant to insert
   // the variable info into the alloc_stack instead of additionally generating a
   // debug_value_addr.
   AllocStackInst *alloc = allocBuilder.createAllocStack(
      RegularLocation::getAutoGeneratedLocation(), type);

   // Insert dealloc_stack at the end(s) of the function.
   for (TermInst *termInst : pass.returnInsts) {
      PILBuilderWithScope deallocBuilder(termInst);
      deallocBuilder.createDeallocStack(
         RegularLocation::getAutoGeneratedLocation(), alloc);
   }

   return alloc;
}

static StoreOwnershipQualifier
getStoreInitOwnership(StructLoweringState &pass, PILType type) {
   if (!pass.F->hasOwnership()) {
      return StoreOwnershipQualifier::Unqualified;
   } else if (type.isTrivial(*pass.F)) {
      return StoreOwnershipQualifier::Trivial;
   } else {
      return StoreOwnershipQualifier::Init;
   }
}

static StoreInst *createStoreInit(StructLoweringState &pass,
                                  PILBasicBlock::iterator where,
                                  PILLocation loc,
                                  PILValue value, PILValue address) {
   PILBuilderWithScope storeBuilder(where);
   return storeBuilder.createStore(loc, value, address,
                                   getStoreInitOwnership(pass, value->getType()));
}

static PILInstruction *createOutlinedCopyCall(PILBuilder &copyBuilder,
                                              PILValue src, PILValue tgt,
                                              StructLoweringState &pass,
                                              PILLocation *loc = nullptr) {
   PILLocation locToUse = loc ? *loc : copyBuilder.getInsertionPoint()->getLoc();
   auto *copy =
      copyBuilder.createCopyAddr(locToUse, src, tgt, IsTake, IsInitialization);
   return copy;
}

void LoadableStorageAllocation::replaceLoadWithCopyAddr(
   LoadInst *optimizableLoad) {
   PILValue value = optimizableLoad->getOperand();

   auto allocInstr = allocate(pass, value->getType().getObjectType());

   PILBuilderWithScope outlinedBuilder(optimizableLoad);
   createOutlinedCopyCall(outlinedBuilder, value, allocInstr, pass);

   for (auto *user : optimizableLoad->getUses()) {
      PILInstruction *userIns = user->getUser();
      switch (userIns->getKind()) {
         case PILInstructionKind::CopyAddrInst:
         case PILInstructionKind::DeallocStackInst:
            break;
         case PILInstructionKind::ApplyInst:
         case PILInstructionKind::TryApplyInst:
         case PILInstructionKind::BeginApplyInst:
         case PILInstructionKind::PartialApplyInst: {
            if (std::find(pass.applies.begin(), pass.applies.end(), userIns) ==
                pass.applies.end()) {
               pass.applies.push_back(userIns);
            }
            break;
         }
         case PILInstructionKind::YieldInst:
            // The rewrite is enough.
            break;
         case PILInstructionKind::RetainValueInst: {
            auto *insToInsert = cast<RetainValueInst>(userIns);
            pass.retainInstsToMod.push_back(insToInsert);
            break;
         }
         case PILInstructionKind::ReleaseValueInst: {
            auto *insToInsert = cast<ReleaseValueInst>(userIns);
            pass.releaseInstsToMod.push_back(insToInsert);
            break;
         }
         case PILInstructionKind::StoreInst: {
            auto *insToInsert = cast<StoreInst>(userIns);
            pass.storeInstsToMod.push_back(insToInsert);
            break;
         }
         case PILInstructionKind::DebugValueInst: {
            auto *insToInsert = cast<DebugValueInst>(userIns);
            pass.debugInstsToMod.push_back(insToInsert);
            break;
         }
         case PILInstructionKind::DestroyValueInst: {
            auto *insToInsert = cast<DestroyValueInst>(userIns);
            pass.destroyValueInstsToMod.push_back(insToInsert);
            break;
         }
         case PILInstructionKind::StructExtractInst: {
            auto *instToInsert = cast<StructExtractInst>(userIns);
            if (std::find(pass.structExtractInstsToMod.begin(),
                          pass.structExtractInstsToMod.end(),
                          instToInsert) == pass.structExtractInstsToMod.end()) {
               pass.structExtractInstsToMod.push_back(instToInsert);
            }
            break;
         }
         case PILInstructionKind::SwitchEnumInst: {
            auto *instToInsert = cast<SwitchEnumInst>(userIns);
            if (std::find(pass.switchEnumInstsToMod.begin(),
                          pass.switchEnumInstsToMod.end(),
                          instToInsert) == pass.switchEnumInstsToMod.end()) {
               pass.switchEnumInstsToMod.push_back(instToInsert);
            }
            break;
         }
         default:
            llvm_unreachable("Unexpected instruction");
      }
   }

   optimizableLoad->replaceAllUsesWith(allocInstr);
   optimizableLoad->getParent()->erase(optimizableLoad);
}

static bool isYieldUseRewriteable(StructLoweringState &pass,
                                  YieldInst *inst, Operand *operand) {
   assert(inst == operand->getUser());
   return pass.isLargeLoadableType(operand->get()->getType());
}

/// Does the value's uses contain instructions that *must* be rewrites?
static bool hasMandatoryRewriteUse(StructLoweringState &pass,
                                   PILValue value) {
   for (auto *user : value->getUses()) {
      PILInstruction *userIns = user->getUser();
      switch (userIns->getKind()) {
         case PILInstructionKind::ApplyInst:
         case PILInstructionKind::TryApplyInst:
         case PILInstructionKind::BeginApplyInst:
         case PILInstructionKind::PartialApplyInst: {
            ApplySite site(userIns);
            PILValue callee = site.getCallee();
            if (callee == value) {
               break;
            }
            PILType currType = value->getType().getObjectType();
            PILType newPILType = pass.getNewPILType(currType);
            if (currType == newPILType) {
               break;
            }
            return true;
         }
         case PILInstructionKind::YieldInst:
            if (isYieldUseRewriteable(pass, cast<YieldInst>(userIns), user))
               return true;
            break;
         default:
            break;
      }
   }
   return false;
}

void LoadableStorageAllocation::replaceLoadWithCopyAddrForModifiable(
   LoadInst *unoptimizableLoad) {
   if (!hasMandatoryRewriteUse(pass, unoptimizableLoad)) {
      return;
   }
   PILValue value = unoptimizableLoad->getOperand();

   AllocStackInst *alloc = allocate(pass, value->getType().getObjectType());

   PILBuilderWithScope outlinedBuilder(unoptimizableLoad);
   createOutlinedCopyCall(outlinedBuilder, value, alloc, pass);

   SmallVector<Operand *, 8> usesToMod;
   for (auto *use : unoptimizableLoad->getUses()) {
      PILInstruction *userIns = use->getUser();
      switch (userIns->getKind()) {
         case PILInstructionKind::CopyAddrInst:
         case PILInstructionKind::DeallocStackInst:
            break;
         case PILInstructionKind::ApplyInst:
         case PILInstructionKind::TryApplyInst:
         case PILInstructionKind::BeginApplyInst:
         case PILInstructionKind::PartialApplyInst: {
            ApplySite site(userIns);
            if (!modifiableApply(site, pass.Mod)) {
               break;
            }
            PILValue callee = site.getCallee();
            if (callee == unoptimizableLoad) {
               break;
            }
            PILType currType = unoptimizableLoad->getType().getObjectType();
            PILType newPILType = pass.getNewPILType(currType);
            if (currType == newPILType) {
               break;
            }
            if (std::find(pass.applies.begin(), pass.applies.end(), userIns) ==
                pass.applies.end()) {
               pass.applies.push_back(userIns);
            }
            usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::YieldInst: {
            if (isYieldUseRewriteable(pass, cast<YieldInst>(userIns), use))
               usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::RetainValueInst: {
            auto *insToInsert = cast<RetainValueInst>(userIns);
            pass.retainInstsToMod.push_back(insToInsert);
            usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::ReleaseValueInst: {
            auto *insToInsert = cast<ReleaseValueInst>(userIns);
            pass.releaseInstsToMod.push_back(insToInsert);
            usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::StoreInst: {
            auto *insToInsert = cast<StoreInst>(userIns);
            pass.storeInstsToMod.push_back(insToInsert);
            usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::DebugValueInst: {
            auto *insToInsert = cast<DebugValueInst>(userIns);
            pass.debugInstsToMod.push_back(insToInsert);
            usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::DestroyValueInst: {
            auto *insToInsert = cast<DestroyValueInst>(userIns);
            pass.destroyValueInstsToMod.push_back(insToInsert);
            usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::StructExtractInst: {
            auto *instToInsert = cast<StructExtractInst>(userIns);
            pass.structExtractInstsToMod.push_back(instToInsert);
            usesToMod.push_back(use);
            break;
         }
         case PILInstructionKind::SwitchEnumInst: {
            auto *instToInsert = cast<SwitchEnumInst>(userIns);
            pass.switchEnumInstsToMod.push_back(instToInsert);
            usesToMod.push_back(use);
            break;
         }
         default:
            break;
      }
   }
   while (!usesToMod.empty()) {
      auto *use = usesToMod.pop_back_val();
      use->set(alloc);
   }
}

void LoadableStorageAllocation::allocateLoadableStorage() {
   // We need to map all functions exits
   // required for Apply result's allocations
   // Else we might get the following error:
   // "stack dealloc does not match most recent stack alloc"
   // When we dealloc later
   LargeValueVisitor(pass).mapReturnInstrs();
   if (modifiableFunction(pass.F->getLoweredFunctionType())) {
      // Turn by-value function args to by-address ones
      convertIndirectFunctionArgs();
   } else {
      convertIndirectFunctionPointerArgsForUnmodifiable();
   }
   convertApplyResults();

   // Populate the pass' data structs
   LargeValueVisitor(pass).mapValueStorage();

   // Turn by-value BB args to by-address ones
   convertIndirectBasicBlockArgs();

   // Create an AllocStack for every used large loadable type in the function.
   for (auto &argToAlloc : pass.argsToLoadedValueMap) {
      assert(argToAlloc.first == argToAlloc.second);
      allocateForArg(argToAlloc.first);
   }
}

PILArgument *LoadableStorageAllocation::replaceArgType(PILBuilder &argBuilder,
                                                       PILArgument *arg,
                                                       PILType newPILType) {
   PILValue undef = PILUndef::get(newPILType, *pass.F);
   SmallVector<Operand *, 8> useList(arg->use_begin(), arg->use_end());
   for (auto *use : useList) {
      use->set(undef);
   }

   // Make sure that this is an argument we want to replace.
   assert(std::find(pass.largeLoadableArgs.begin(), pass.largeLoadableArgs.end(),
                    arg) == pass.largeLoadableArgs.end());

   arg = arg->getParent()->replaceFunctionArgument(
      arg->getIndex(), newPILType, ValueOwnershipKind::None, arg->getDecl());

   for (auto *use : useList) {
      use->set(arg);
   }

   return arg;
}

void LoadableStorageAllocation::insertIndirectReturnArgs() {
   GenericEnvironment *genEnv = pass.F->getGenericEnvironment();
   auto loweredTy = pass.F->getLoweredFunctionType();
   PILType resultStorageType = loweredTy->getAllResultsInterfaceType();
   auto canType = resultStorageType.getAstType();
   if (canType->hasTypeParameter()) {
      assert(genEnv && "Expected a GenericEnv");
      canType = genEnv->mapTypeIntoContext(canType)->getCanonicalType();
   }
   resultStorageType = PILType::getPrimitiveObjectType(canType);
   auto newResultStorageType = pass.getNewPILType(resultStorageType);

   auto &ctx = pass.F->getModule().getAstContext();
   auto var = new (ctx) ParamDecl(
      SourceLoc(), SourceLoc(),
      ctx.getIdentifier("$return_value"), SourceLoc(),
      ctx.getIdentifier("$return_value"),
      pass.F->getDeclContext());
   var->setSpecifier(ParamSpecifier::InOut);
   pass.F->begin()->insertFunctionArgument(
      0, newResultStorageType.getAddressType(), ValueOwnershipKind::None, var);
}

void LoadableStorageAllocation::convertIndirectFunctionArgs() {
   PILBasicBlock *entry = pass.F->getEntryBlock();
   PILBuilderWithScope argBuilder(entry->begin());

   for (PILArgument *arg : entry->getArguments()) {
      PILType storageType = arg->getType();
      PILType newPILType = pass.getNewPILType(storageType);
      if (newPILType != storageType) {
         ValueOwnershipKind ownership = arg->getOwnershipKind();
         arg = replaceArgType(argBuilder, arg, newPILType);
         if (pass.isLargeLoadableType(storageType)) {
            // Add to largeLoadableArgs if and only if it wasn't a modified function
            // signature arg
            pass.largeLoadableArgs.push_back(arg);
         } else {
            arg->setOwnershipKind(ownership);
            pass.funcSigArgs.push_back(arg);
         }
      }
   }

   // Convert the result type to indirect if necessary:
   if (modNonFuncTypeResultType(pass.F, pass.Mod)) {
      insertIndirectReturnArgs();
   }
}

static void convertBBArgType(PILBuilder &argBuilder, PILType newPILType,
                             PILArgument *arg) {
   PILValue undef = PILUndef::get(newPILType, argBuilder.getFunction());
   SmallVector<Operand *, 8> useList(arg->use_begin(), arg->use_end());
   for (auto *use : useList) {
      use->set(undef);
   }

   arg = arg->getParent()->replacePhiArgument(arg->getIndex(), newPILType,
                                              arg->getOwnershipKind());
   for (auto *use : useList) {
      use->set(arg);
   }
}

static bool containsFunctionType(CanType ty) {
   if (auto tuple = dyn_cast<TupleType>(ty)) {
      for (auto elt : tuple.getElementTypes()) {
         if (containsFunctionType(elt))
            return true;
      }
      return false;
   }
   if (auto optionalType = ty.getOptionalObjectType()) {
      return containsFunctionType(optionalType);
   }
   return isa<PILFunctionType>(ty);
}

void LoadableStorageAllocation::convertApplyResults() {
   for (auto &BB : *pass.F) {
      for (auto &II : BB) {
         auto *currIns = &II;
         auto applySite = FullApplySite::isa(currIns);
         if (!applySite) {
            continue;
         }
         if (!modifiableApply(applySite, pass.Mod)) {
            continue;
         }

         CanPILFunctionType origPILFunctionType = applySite.getSubstCalleeType();
         GenericEnvironment *genEnv = nullptr;
         if (!pass.Mapper.shouldTransformResults(genEnv, origPILFunctionType,
                                                 pass.Mod)) {
            continue;
         }
         auto resultStorageType = origPILFunctionType->getAllResultsInterfaceType();
         if (!pass.isLargeLoadableType(resultStorageType)) {
            // Make sure it contains a function type
            auto numFuncTy = llvm::count_if(origPILFunctionType->getResults(),
                                            [](const PILResultInfo &origResult) {
                                               auto resultStorageTy = origResult.getPILStorageInterfaceType();
                                               return containsFunctionType(resultStorageTy.getAstType());
                                            });
            assert(numFuncTy != 0 &&
                   "Expected a PILFunctionType inside the result Type");
            (void)numFuncTy;
            continue;
         }
         auto newPILType = pass.getNewPILType(resultStorageType);
         auto *newVal = allocateForApply(currIns, newPILType.getObjectType());
         if (auto apply = dyn_cast<ApplyInst>(currIns)) {
            apply->replaceAllUsesWith(newVal);
         } else {
            auto tryApplyIns = cast<TryApplyInst>(currIns);
            auto *normalBB = tryApplyIns->getNormalBB();
            PILBuilderWithScope argBuilder(normalBB->begin());
            assert(normalBB->getNumArguments() == 1 &&
                   "Expected only one arg for try_apply normal BB");
            auto arg = normalBB->getArgument(0);
            arg->replaceAllUsesWith(newVal);
            auto emptyTy = PILType::getPrimitiveObjectType(
               TupleType::getEmpty(argBuilder.getModule().getAstContext()));
            convertBBArgType(argBuilder, emptyTy, arg);
         }
      }
   }
}

void LoadableStorageAllocation::
convertIndirectFunctionPointerArgsForUnmodifiable() {
   PILBasicBlock *entry = pass.F->getEntryBlock();
   PILBuilderWithScope argBuilder(entry->begin());

   for (PILArgument *arg : entry->getArguments()) {
      PILType storageType = arg->getType();
      PILType newPILType = pass.getNewPILType(storageType);
      if (pass.containsDifferentFunctionSignature(storageType)) {
         auto *castInstr = argBuilder.createUncheckedBitCast(
            RegularLocation(const_cast<ValueDecl *>(arg->getDecl())), arg,
            newPILType);
         arg->replaceAllUsesWith(castInstr);
         castInstr->setOperand(0, arg);
      }
   }
}

void LoadableStorageAllocation::convertIndirectBasicBlockArgs() {
   PILBasicBlock *entry = pass.F->getEntryBlock();
   for (PILBasicBlock &BB : *pass.F) {
      if (&BB == entry) {
         // Already took care of function args
         continue;
      }
      PILBuilderWithScope argBuilder(BB.begin());
      for (PILArgument *arg : BB.getArguments()) {
         if (!pass.Mapper.shouldConvertBBArg(arg, pass.Mod)) {
            continue;
         }
         PILType storageType = arg->getType();
         PILType newPILType = pass.getNewPILType(storageType);
         // We don't change the type from object to address for function args:
         // a tuple with both a large type and a function arg should remain
         // as an object type for now
         if (storageType.isObject()) {
            newPILType = newPILType.getObjectType();
         }
         convertBBArgType(argBuilder, newPILType, arg);
      }
   }
}

void LoadableStorageAllocation::allocateForArg(PILValue value) {
   if (auto *allocInstr = dyn_cast<AllocStackInst>(value)) {
      // Special case: the value was already an Alloc
      // This happens in case of values from apply results (for example)
      // we *should* add a load for the current uses.
      // Said load should happen before the first use
      // As such add it right after the apply()
      LoadInst *load = nullptr;
      assert(pass.allocToApplyRetMap.find(allocInstr) !=
             pass.allocToApplyRetMap.end() &&
             "Alloc is not for apply results");
      auto *applyInst = pass.allocToApplyRetMap[allocInstr];
      assert(applyInst && "Value is not an apply");
      auto II = applyInst->getIterator();
      PILBuilderWithScope loadBuilder(II);
      if (auto *tryApply = dyn_cast<TryApplyInst>(applyInst)) {
         auto *tgtBB = tryApply->getNormalBB();
         assert(tgtBB && "Could not find try apply's target BB");
         loadBuilder.setInsertionPoint(tgtBB->begin());
      } else {
         ++II;
         loadBuilder.setInsertionPoint(II);
      }
      if (!pass.F->hasOwnership()) {
         load = loadBuilder.createLoad(applyInst->getLoc(), value,
                                       LoadOwnershipQualifier::Unqualified);
      } else {
         load = loadBuilder.createLoad(applyInst->getLoc(), value,
                                       LoadOwnershipQualifier::Take);
      }
      pass.argsToLoadedValueMap[value] = load;
      return;
   }

   assert(!ApplySite::isa(value) && "Unexpected instruction");

   // Find the first non-AllocStackInst and use its scope when creating
   // the new PILBuilder. An AllocStackInst does not directly cause any
   // code to be generated. The location of an AllocStackInst carries information
   // about the source variable; it doesn't matter where in the instruction
   // stream the AllocStackInst is located.
   auto BBIter = pass.F->begin()->begin();
   PILInstruction *FirstNonAllocStack = &*BBIter;
   while (isa<AllocStackInst>(FirstNonAllocStack) &&
          BBIter != pass.F->begin()->end()) {
      BBIter++;
      FirstNonAllocStack = &*BBIter;
   }
   PILBuilderWithScope allocBuilder(&*pass.F->begin()->begin(),
                                    FirstNonAllocStack);

   AllocStackInst *allocInstr = allocBuilder.createAllocStack(
      RegularLocation::getAutoGeneratedLocation(), value->getType());

   LoadInst *loadCopy = nullptr;
   auto *applyOutlinedCopy =
      createOutlinedCopyCall(allocBuilder, value, allocInstr, pass);

   if (!pass.F->hasOwnership()) {
      loadCopy = allocBuilder.createLoad(applyOutlinedCopy->getLoc(), allocInstr,
                                         LoadOwnershipQualifier::Unqualified);
   } else {
      loadCopy = allocBuilder.createLoad(applyOutlinedCopy->getLoc(), allocInstr,
                                         LoadOwnershipQualifier::Take);
   }
   pass.argsToLoadedValueMap[value] = loadCopy;

   // Insert stack deallocations.
   for (TermInst *termInst : pass.returnInsts) {
      PILBuilderWithScope deallocBuilder(termInst);
      deallocBuilder.createDeallocStack(allocInstr->getLoc(), allocInstr);
   }
}

AllocStackInst *
LoadableStorageAllocation::allocateForApply(PILInstruction *apply,
                                            PILType type) {
   PILBuilderWithScope allocBuilder(&*pass.F->begin());
   PILLocation Loc = apply->getLoc();
   if (dyn_cast_or_null<VarDecl>(Loc.getAsAstNode<Decl>()))
      // FIXME: Remove this. This is likely indicative of a bug earlier in the
      // pipeline. An apply instruction should not have a VarDecl as location.
      Loc = RegularLocation::getAutoGeneratedLocation();
   auto *allocInstr = allocBuilder.createAllocStack(Loc, type);

   pass.largeLoadableArgs.push_back(allocInstr);
   pass.allocToApplyRetMap[allocInstr] = apply;
   pass.applyRetToAllocMap[apply] = allocInstr;

   for (TermInst *termInst : pass.returnInsts) {
      PILBuilderWithScope deallocBuilder(termInst);
      deallocBuilder.createDeallocStack(allocInstr->getLoc(), allocInstr);
   }

   return allocInstr;
}

//===----------------------------------------------------------------------===//
// LoadableByAddress: Top-Level Function Transform.
//===----------------------------------------------------------------------===//

namespace {
class LoadableByAddress : public PILModuleTransform {
   /// The entry point to this function transformation.
   void run() override;

   void runOnFunction(PILFunction *F);

private:
   void updateLoweredTypes(PILFunction *F);
   void recreateSingleApply(PILInstruction *applyInst,
                            SmallVectorImpl<PILInstruction *> &Delete);
   bool recreateApply(PILInstruction &I,
                      SmallVectorImpl<PILInstruction *> &Delete);
   bool recreateTupleInstr(PILInstruction &I,
                           SmallVectorImpl<PILInstruction *> &Delete);
   bool recreateConvInstr(PILInstruction &I,
                          SmallVectorImpl<PILInstruction *> &Delete);
   bool recreateBuiltinInstr(PILInstruction &I,
                             SmallVectorImpl<PILInstruction *> &Delete);
   bool recreateLoadInstr(PILInstruction &I,
                          SmallVectorImpl<PILInstruction *> &Delete);
   bool recreateUncheckedEnumDataInstr(PILInstruction &I,
                                       SmallVectorImpl<PILInstruction *> &Delete);
   bool recreateUncheckedTakeEnumDataAddrInst(PILInstruction &I,
                                              SmallVectorImpl<PILInstruction *> &Delete);
   bool fixStoreToBlockStorageInstr(PILInstruction &I,
                                    SmallVectorImpl<PILInstruction *> &Delete);

private:
   llvm::SetVector<PILFunction *> modFuncs;
   llvm::SetVector<SingleValueInstruction *> conversionInstrs;
   llvm::SetVector<BuiltinInst *> builtinInstrs;
   llvm::SetVector<LoadInst *> loadInstrsOfFunc;
   llvm::SetVector<UncheckedEnumDataInst *> uncheckedEnumDataOfFunc;
   llvm::SetVector<UncheckedTakeEnumDataAddrInst *>
      uncheckedTakeEnumDataAddrOfFunc;
   llvm::SetVector<StoreInst *> storeToBlockStorageInstrs;
   llvm::SetVector<PILInstruction *> modApplies;
   llvm::MapVector<PILInstruction *, PILValue> allApplyRetToAllocMap;
   LargePILTypeMapper MapperCache;
};
} // end anonymous namespace

/// Given that we've allocated space to hold a scalar value, try to rewrite
/// the uses of the scalar to be uses of the address.
static void rewriteUsesOfSscalar(StructLoweringState &pass,
                                 PILValue address, PILValue scalar,
                                 StoreInst *storeToAddress) {
   // Copy the uses, since we're going to edit them.
   SmallVector<Operand *, 8> uses(scalar->getUses());
   for (Operand *scalarUse : uses) {
      PILInstruction *user = scalarUse->getUser();

      if (ApplySite::isa(user)) {
         ApplySite site(user);
         if (modifiableApply(site, pass.Mod)) {
            // Just rewrite the operand in-place.  This will produce a temporary
            // type error, but we should fix that up when we rewrite the apply's
            // function type.
            scalarUse->set(address);
         }
      } else if (isa<YieldInst>(user)) {
         // The rules for the yield are changing anyway, so we can just rewrite
         // it in-place.
         scalarUse->set(address);
      } else if (auto *storeUser = dyn_cast<StoreInst>(user)) {
         // Don't rewrite the store to the allocation.
         if (storeUser == storeToAddress)
            continue;

         // Optimization: replace with copy_addr to reduce code size
         assert(std::find(pass.storeInstsToMod.begin(), pass.storeInstsToMod.end(),
                          storeUser) == pass.storeInstsToMod.end() &&
                "Did not expect this instr in storeInstsToMod");
         PILBuilderWithScope copyBuilder(storeUser);
         PILValue dest = storeUser->getDest();
         createOutlinedCopyCall(copyBuilder, address, dest, pass);
         storeUser->eraseFromParent();
      } else if (auto *dbgInst = dyn_cast<DebugValueInst>(user)) {
         PILBuilderWithScope dbgBuilder(dbgInst);
         // Rewrite the debug_value to point to the variable in the alloca.
         dbgBuilder.createDebugValueAddr(dbgInst->getLoc(), address,
                                         *dbgInst->getVarInfo());
         dbgInst->eraseFromParent();
      }
   }
}

static void allocateAndSetForInstResult(StructLoweringState &pass,
                                        PILValue instResult,
                                        PILInstruction *inst) {
   auto alloc = allocate(pass, instResult->getType());

   auto II = inst->getIterator();
   ++II;
   auto store = createStoreInit(pass, II, inst->getLoc(), instResult, alloc);

   // Traverse all the uses of instResult - see if we can replace
   rewriteUsesOfSscalar(pass, alloc, instResult, store);
}

static void allocateAndSetForArgument(StructLoweringState &pass,
                                      PILArgument *value,
                                      PILInstruction *user) {
   AllocStackInst *alloc = allocate(pass, value->getType());

   PILLocation loc = user->getLoc();
   loc.markAutoGenerated();

   // Store the value into the allocation.
   auto II = value->getParent()->begin();
   if (II == alloc->getParent()->begin()) {
      // Store should happen *after* the allocation.
      ++II;
   }
   auto store = createStoreInit(pass, II, loc, value, alloc);

   // Traverse all the uses of value - see if we can replace
   rewriteUsesOfSscalar(pass, alloc, value, store);
}

static bool allUsesAreReplaceable(StructLoweringState &pass,
                                  SingleValueInstruction *instr) {
   for (auto *user : instr->getUses()) {
      PILInstruction *userIns = user->getUser();
      switch (userIns->getKind()) {
         case PILInstructionKind::RetainValueInst:
         case PILInstructionKind::ReleaseValueInst:
         case PILInstructionKind::StoreInst:
         case PILInstructionKind::DebugValueInst:
         case PILInstructionKind::DestroyValueInst:
            break;
         case PILInstructionKind::ApplyInst:
         case PILInstructionKind::TryApplyInst:
         case PILInstructionKind::BeginApplyInst:
         case PILInstructionKind::PartialApplyInst: {
            // Replaceable only if it is not the function pointer
            ApplySite site(userIns);
            if (!modifiableApply(site, pass.Mod)) {
               return false;
            }
            PILValue callee = site.getCallee();
            if (callee == instr) {
               return false;
            }
            PILType currType = instr->getType().getObjectType();
            PILType newPILType = pass.getNewPILType(currType);
            if (currType == newPILType) {
               return false;
            }
            break;
         }
         case PILInstructionKind::YieldInst:
            if (!isYieldUseRewriteable(pass, cast<YieldInst>(userIns), user))
               return false;
            break;
         case PILInstructionKind::StructExtractInst:
         case PILInstructionKind::SwitchEnumInst:
            break;
         default:
            return false;
      }
   }
   return true;
}

void LoadableStorageAllocation::replaceLoad(LoadInst *load) {
   if (allUsesAreReplaceable(pass, load)) {
      replaceLoadWithCopyAddr(load);
   } else {
      replaceLoadWithCopyAddrForModifiable(load);
   }
}

static void allocateAndSet(StructLoweringState &pass,
                           LoadableStorageAllocation &allocator,
                           PILValue operand, PILInstruction *user) {
   auto inst = operand->getDefiningInstruction();
   if (!inst) {
      allocateAndSetForArgument(pass, cast<PILArgument>(operand), user);
      return;
   }

   if (auto *load = dyn_cast<LoadInst>(operand)) {
      allocator.replaceLoad(load);
   } else {
      // TODO: peephole: special handling of known cases:
      // ApplyInst, TupleExtractInst
      allocateAndSetForInstResult(pass, operand, inst);
   }
}

/// Rewrite all of the large-loadable operands in the given list.
static void allocateAndSetAll(StructLoweringState &pass,
                              LoadableStorageAllocation &allocator,
                              PILInstruction *user,
                              MutableArrayRef<Operand> operands) {
   for (Operand &operand : operands) {
      PILValue value = operand.get();
      PILType silType = value->getType();
      if (pass.isLargeLoadableType(silType)) {
         allocateAndSet(pass, allocator, value, user);
      }
   }
}

static void castTupleInstr(SingleValueInstruction *instr, IRGenModule &Mod,
                           LargePILTypeMapper &Mapper) {
   PILType currPILType = instr->getType();
   auto funcType = getInnerFunctionType(currPILType);
   assert(funcType && "Expected a function Type");
   GenericEnvironment *genEnv = instr->getFunction()->getGenericEnvironment();
   if (!genEnv && funcType->getSubstGenericSignature()) {
      genEnv = getGenericEnvironment(funcType);
   }
   PILType newPILType = Mapper.getNewPILType(genEnv, currPILType, Mod);
   if (currPILType == newPILType) {
      return;
   }

   auto II = instr->getIterator();
   ++II;
   PILBuilderWithScope castBuilder(II);
   SingleValueInstruction *castInstr = nullptr;
   switch (instr->getKind()) {
      // Add cast to the new sil function type:
      case PILInstructionKind::TupleExtractInst: {
         castInstr = castBuilder.createUncheckedBitCast(instr->getLoc(), instr,
                                                        newPILType.getObjectType());
         break;
      }
      case PILInstructionKind::TupleElementAddrInst: {
         castInstr = castBuilder.createUncheckedAddrCast(
            instr->getLoc(), instr, newPILType.getAddressType());
         break;
      }
      default:
         llvm_unreachable("Unexpected instruction inside tupleInstsToMod");
   }
   instr->replaceAllUsesWith(castInstr);
   castInstr->setOperand(0, instr);
}

static PILValue createCopyOfEnum(StructLoweringState &pass,
                                 SwitchEnumInst *orig) {
   auto value = orig->getOperand();
   auto type = value->getType();
   if (type.isObject()) {
      // support for non-address operands / enums
      auto *alloc = allocate(pass, type);
      createStoreInit(pass, orig->getIterator(), orig->getLoc(), value, alloc);
      return alloc;
   }

   auto alloc = allocate(pass, type.getObjectType());

   PILBuilderWithScope copyBuilder(orig);
   createOutlinedCopyCall(copyBuilder, value, alloc, pass);

   return alloc;
}

static void createResultTyInstrAndLoad(LoadableStorageAllocation &allocator,
                                       SingleValueInstruction *instr,
                                       StructLoweringState &pass) {
   bool updateResultTy = pass.resultTyInstsToMod.count(instr) != 0;
   if (updateResultTy) {
      pass.resultTyInstsToMod.remove(instr);
   }
   PILBuilderWithScope builder(instr);
   auto *currStructExtractInst = dyn_cast<StructExtractInst>(instr);
   assert(currStructExtractInst && "Expected StructExtractInst");
   SingleValueInstruction *newInstr = builder.createStructElementAddr(
      currStructExtractInst->getLoc(), currStructExtractInst->getOperand(),
      currStructExtractInst->getField(),
      currStructExtractInst->getType().getAddressType());
   // Load the struct element then see if we can get rid of the load:
   LoadInst *loadArg = nullptr;
   if (!pass.F->hasOwnership()) {
      loadArg = builder.createLoad(newInstr->getLoc(), newInstr,
                                   LoadOwnershipQualifier::Unqualified);
   } else {
      loadArg = builder.createLoad(newInstr->getLoc(), newInstr,
                                   LoadOwnershipQualifier::Take);
   }
   instr->replaceAllUsesWith(loadArg);
   instr->getParent()->erase(instr);

   // If the load is of a function type - do not replace it.
   if (isFuncOrOptionalFuncType(loadArg->getType())) {
      return;
   }

   allocator.replaceLoad(loadArg);

   if (updateResultTy) {
      pass.resultTyInstsToMod.insert(newInstr);
   }
}

static void rewriteFunction(StructLoweringState &pass,
                            LoadableStorageAllocation &allocator) {

   bool repeat = false;
   llvm::SetVector<PILInstruction *> currentModApplies;
   do {
      while (!pass.switchEnumInstsToMod.empty()) {
         auto *instr = pass.switchEnumInstsToMod.pop_back_val();
         /* unchecked_take_enum_data_addr can be destructive.
          * work on a copy instead of the original enum */
         auto copiedValue = createCopyOfEnum(pass, instr);
         PILBuilderWithScope enumBuilder(instr);
         unsigned numOfCases = instr->getNumCases();
         SmallVector<std::pair<EnumElementDecl *, PILBasicBlock *>, 16> caseBBs;
         for (unsigned i = 0; i < numOfCases; ++i) {
            auto currCase = instr->getCase(i);
            auto *currBB = currCase.second;
            PILBuilderWithScope argBuilder(currBB->begin());
            assert(currBB->getNumArguments() <= 1 && "Unhandled BB Type");
            EnumElementDecl *decl = currCase.first;
            for (PILArgument *arg : currBB->getArguments()) {
               PILType storageType = arg->getType();
               PILType newPILType = pass.getNewPILType(storageType);
               if (storageType == newPILType) {
                  newPILType = newPILType.getAddressType();
               }

               auto *newArg = argBuilder.createUncheckedTakeEnumDataAddr(
                  instr->getLoc(), copiedValue, decl, newPILType.getAddressType());
               arg->replaceAllUsesWith(newArg);
               currBB->eraseArgument(0);

               // Load the enum addr then see if we can get rid of the load:
               LoadInst *loadArg = nullptr;
               if (!pass.F->hasOwnership()) {
                  loadArg = argBuilder.createLoad(
                     newArg->getLoc(), newArg, LoadOwnershipQualifier::Unqualified);
               } else {
                  loadArg = argBuilder.createLoad(newArg->getLoc(), newArg,
                                                  LoadOwnershipQualifier::Take);
               }
               newArg->replaceAllUsesWith(loadArg);
               loadArg->setOperand(newArg);

               // If the load is of a function type - do not replace it.
               if (isFuncOrOptionalFuncType(loadArg->getType())) {
                  continue;
               }

               allocator.replaceLoad(loadArg);
            }
            caseBBs.push_back(std::make_pair(decl, currBB));
         }
         PILBasicBlock *defaultBB =
            instr->hasDefault() ? instr->getDefaultBB() : nullptr;
         enumBuilder.createSwitchEnumAddr(
            instr->getLoc(), copiedValue, defaultBB, caseBBs);
         instr->getParent()->erase(instr);
      }

      while (!pass.structExtractInstsToMod.empty()) {
         auto *instr = pass.structExtractInstsToMod.pop_back_val();
         createResultTyInstrAndLoad(allocator, instr, pass);
      }

      while (!pass.applies.empty()) {
         auto *applyInst = pass.applies.pop_back_val();
         if (currentModApplies.count(applyInst) == 0) {
            currentModApplies.insert(applyInst);
         }
         ApplySite applySite = ApplySite(applyInst);
         allocateAndSetAll(pass, allocator, applyInst,
                           applySite.getArgumentOperands());
      }

      repeat = !pass.switchEnumInstsToMod.empty() ||
               !pass.structExtractInstsToMod.empty();
      assert(pass.applies.empty());
      pass.applies.append(currentModApplies.begin(), currentModApplies.end());
   } while (repeat);

   for (PILInstruction *instr : pass.instsToMod) {
      for (Operand &operand : instr->getAllOperands()) {
         auto currOperand = operand.get();
         if (std::find(pass.largeLoadableArgs.begin(),
                       pass.largeLoadableArgs.end(),
                       currOperand) != pass.largeLoadableArgs.end()) {
            PILValue newOperand = pass.argsToLoadedValueMap[currOperand];
            assert(newOperand != currOperand &&
                   "Did not allocate storage and convert operand");
            operand.set(newOperand);
         }
      }
   }

   for (SingleValueInstruction *instr : pass.tupleInstsToMod) {
      castTupleInstr(instr, pass.Mod, pass.Mapper);
   }

   while (!pass.allocStackInstsToMod.empty()) {
      auto *instr = pass.allocStackInstsToMod.pop_back_val();
      PILBuilderWithScope allocBuilder(instr);
      PILType currPILType = instr->getType();
      PILType newPILType = pass.getNewPILType(currPILType);
      auto *newInstr = allocBuilder.createAllocStack(instr->getLoc(), newPILType,
                                                     instr->getVarInfo());
      instr->replaceAllUsesWith(newInstr);
      instr->getParent()->erase(instr);
   }

   while (!pass.pointerToAddrkInstsToMod.empty()) {
      auto *instr = pass.pointerToAddrkInstsToMod.pop_back_val();
      PILBuilderWithScope pointerBuilder(instr);
      PILType currPILType = instr->getType();
      PILType newPILType = pass.getNewPILType(currPILType);
      auto *newInstr = pointerBuilder.createPointerToAddress(
         instr->getLoc(), instr->getOperand(), newPILType.getAddressType(),
         instr->isStrict());
      instr->replaceAllUsesWith(newInstr);
      instr->getParent()->erase(instr);
   }

   for (DebugValueInst *instr : pass.debugInstsToMod) {
      assert(instr->getAllOperands().size() == 1 &&
             "Debug instructions have one operand");
      for (Operand &operand : instr->getAllOperands()) {
         auto currOperand = operand.get();
         if (pass.argsToLoadedValueMap.find(currOperand) !=
             pass.argsToLoadedValueMap.end()) {
            PILValue newOperand = pass.argsToLoadedValueMap[currOperand];
            assert(newOperand != currOperand &&
                   "Did not allocate storage and convert operand");
            operand.set(newOperand);
         } else {
            assert(currOperand->getType().isAddress() &&
                   "Expected an address type");
            PILBuilderWithScope debugBuilder(instr);
            debugBuilder.createDebugValueAddr(instr->getLoc(), currOperand,
                                              *instr->getVarInfo());
            instr->getParent()->erase(instr);
         }
      }
   }

   for (PILInstruction *instr : pass.destroyValueInstsToMod) {
      assert(instr->getAllOperands().size() == 1 &&
             "destroy_value instructions have one operand");
      for (Operand &operand : instr->getAllOperands()) {
         auto currOperand = operand.get();
         assert(currOperand->getType().isAddress() && "Expected an address type");
         PILBuilderWithScope destroyBuilder(instr);
         destroyBuilder.createDestroyAddr(instr->getLoc(), currOperand);
         instr->getParent()->erase(instr);
      }
   }

   for (StoreInst *instr : pass.storeInstsToMod) {
      PILValue src = instr->getSrc();
      PILValue tgt = instr->getDest();
      PILType srcType = src->getType();
      PILType tgtType = tgt->getType();
      assert(srcType && "Expected an address-type source");
      assert(tgtType.isAddress() && "Expected an address-type target");
      assert(srcType == tgtType && "Source and target type do not match");
      (void)srcType;
      (void)tgtType;

      PILBuilderWithScope copyBuilder(instr);
      createOutlinedCopyCall(copyBuilder, src, tgt, pass);
      instr->getParent()->erase(instr);
   }

   for (RetainValueInst *instr : pass.retainInstsToMod) {
      PILBuilderWithScope retainBuilder(instr);
      retainBuilder.createRetainValueAddr(
         instr->getLoc(), instr->getOperand(), instr->getAtomicity());
      instr->getParent()->erase(instr);
   }

   for (ReleaseValueInst *instr : pass.releaseInstsToMod) {
      PILBuilderWithScope releaseBuilder(instr);
      releaseBuilder.createReleaseValueAddr(
         instr->getLoc(), instr->getOperand(), instr->getAtomicity());
      instr->getParent()->erase(instr);
   }

   for (SingleValueInstruction *instr : pass.resultTyInstsToMod) {
      // Update the return type of these instrs
      // Note: The operand was already updated!
      PILType currPILType = instr->getType().getObjectType();
      PILType newPILType = pass.getNewPILType(currPILType);
      PILBuilderWithScope resultTyBuilder(instr);
      PILLocation Loc = instr->getLoc();
      SingleValueInstruction *newInstr = nullptr;
      switch (instr->getKind()) {
         case PILInstructionKind::StructExtractInst: {
            auto *convInstr = cast<StructExtractInst>(instr);
            newInstr = resultTyBuilder.createStructExtract(
               Loc, convInstr->getOperand(), convInstr->getField(),
               newPILType.getObjectType());
            break;
         }
         case PILInstructionKind::StructElementAddrInst: {
            auto *convInstr = cast<StructElementAddrInst>(instr);
            newInstr = resultTyBuilder.createStructElementAddr(
               Loc, convInstr->getOperand(), convInstr->getField(),
               newPILType.getAddressType());
            break;
         }
         case PILInstructionKind::UncheckedTakeEnumDataAddrInst: {
            auto *convInstr = cast<UncheckedTakeEnumDataAddrInst>(instr);
            newInstr = resultTyBuilder.createUncheckedTakeEnumDataAddr(
               Loc, convInstr->getOperand(), convInstr->getElement(),
               newPILType.getAddressType());
            break;
         }
         case PILInstructionKind::RefTailAddrInst: {
            auto *convInstr = cast<RefTailAddrInst>(instr);
            newInstr = resultTyBuilder.createRefTailAddr(Loc, convInstr->getOperand(),
                                                         newPILType.getAddressType());
            break;
         }
         case PILInstructionKind::RefElementAddrInst: {
            auto *convInstr = cast<RefElementAddrInst>(instr);
            newInstr = resultTyBuilder.createRefElementAddr(
               Loc, convInstr->getOperand(), convInstr->getField(),
               newPILType.getAddressType());
            break;
         }
         case PILInstructionKind::BeginAccessInst: {
            auto *convInstr = cast<BeginAccessInst>(instr);
            newInstr = resultTyBuilder.createBeginAccess(
               Loc, convInstr->getOperand(), convInstr->getAccessKind(),
               convInstr->getEnforcement(), convInstr->hasNoNestedConflict(),
               convInstr->isFromBuiltin());
            break;
         }
         case PILInstructionKind::EnumInst: {
            auto *convInstr = cast<EnumInst>(instr);
            PILValue operand =
               convInstr->hasOperand() ? convInstr->getOperand() : PILValue();
            newInstr = resultTyBuilder.createEnum(
               Loc, operand, convInstr->getElement(), newPILType.getObjectType());
            break;
         }
         default:
            llvm_unreachable("Unhandled aggrTy instr");
      }
      instr->replaceAllUsesWith(newInstr);
      instr->eraseFromParent();
   }

   for (MethodInst *instr : pass.methodInstsToMod) {
      PILType currPILType = instr->getType();
      auto currPILFunctionType = currPILType.castTo<PILFunctionType>();
      GenericEnvironment *genEnvForMethod = nullptr;
      if (currPILFunctionType->isPolymorphic()) {
         genEnvForMethod = getGenericEnvironment(currPILFunctionType);
      }
      PILType newPILType =
         PILType::getPrimitiveObjectType(pass.Mapper.getNewPILFunctionType(
            genEnvForMethod, currPILFunctionType, pass.Mod));
      auto member = instr->getMember();
      auto loc = instr->getLoc();
      PILBuilderWithScope methodBuilder(instr);
      MethodInst *newInstr = nullptr;

      switch (instr->getKind()) {
         case PILInstructionKind::ClassMethodInst: {
            PILValue selfValue = instr->getOperand(0);
            newInstr = methodBuilder.createClassMethod(loc, selfValue, member,
                                                       newPILType);
            break;
         }
         case PILInstructionKind::SuperMethodInst: {
            PILValue selfValue = instr->getOperand(0);
            newInstr = methodBuilder.createSuperMethod(loc, selfValue, member,
                                                       newPILType);
            break;
         }
         case PILInstructionKind::WitnessMethodInst: {
            auto *WMI = cast<WitnessMethodInst>(instr);
            newInstr = methodBuilder.createWitnessMethod(
               loc, WMI->getLookupType(), WMI->getConformance(), member, newPILType);
            break;
         }
         default:
            llvm_unreachable("Expected known MethodInst ValueKind");
      }

      instr->replaceAllUsesWith(newInstr);
      instr->getParent()->erase(instr);
   }

   while (!pass.modReturnInsts.empty()) {
      auto *instr = pass.modReturnInsts.pop_back_val();
      auto loc = instr->getLoc(); // PILLocation::RegularKind
      auto regLoc = RegularLocation(loc.getSourceLoc());
      PILBuilderWithScope retBuilder(instr);
      assert(modNonFuncTypeResultType(pass.F, pass.Mod) &&
             "Expected a regular type");
      // Before we return an empty tuple, init return arg:
      auto *entry = pass.F->getEntryBlock();
      auto *retArg = entry->getArgument(0);
      auto retOp = instr->getOperand();
      auto storageType = retOp->getType();
      if (storageType.isAddress()) {
         // There *might* be a dealloc_stack that already released this value
         // we should create the copy *before* the epilogue's deallocations
         auto IIR = instr->getReverseIterator();
         for (++IIR; IIR != instr->getParent()->rend(); ++IIR) {
            auto *currIIInstr = &(*IIR);
            if (currIIInstr->getKind() != PILInstructionKind::DeallocStackInst) {
               // got the right location - stop.
               --IIR;
               break;
            }
         }
         auto II = (IIR != instr->getParent()->rend())
                   ? IIR->getIterator()
                   : instr->getParent()->begin();
         PILBuilderWithScope retCopyBuilder(II);
         createOutlinedCopyCall(retCopyBuilder, retOp, retArg, pass, &regLoc);
      } else {
         retBuilder.createStore(regLoc, retOp, retArg,
                                getStoreInitOwnership(pass, retOp->getType()));
      }
      auto emptyTy = PILType::getPrimitiveObjectType(
         retBuilder.getModule().getAstContext().TheEmptyTupleType);
      auto newRetTuple = retBuilder.createTuple(regLoc, emptyTy, {});
      retBuilder.createReturn(newRetTuple->getLoc(), newRetTuple);
      instr->eraseFromParent();
   }

   while (!pass.modYieldInsts.empty()) {
      YieldInst *inst = pass.modYieldInsts.pop_back_val();
      allocateAndSetAll(pass, allocator, inst, inst->getAllOperands());
   }
}

// Rewrite function return argument if it is a "function pointer"
// If it is a large type also return true - will be re-written later
// Returns true if the return argument needed re-writing
static bool rewriteFunctionReturn(StructLoweringState &pass) {
   auto loweredTy = pass.F->getLoweredFunctionType();
   PILFunction *F = pass.F;
   PILType resultTy = loweredTy->getAllResultsInterfaceType();
   PILType newPILType = pass.getNewPILType(resultTy);
   // We (currently) only care about function signatures
   if (pass.isLargeLoadableType(resultTy)) {
      return true;
   } else if (pass.containsDifferentFunctionSignature(resultTy)) {
      llvm::SmallVector<PILResultInfo, 2> newPILResultInfo;
      if (auto tupleType = newPILType.getAs<TupleType>()) {
         auto originalResults = loweredTy->getResults();
         for (unsigned int i = 0; i < originalResults.size(); ++i) {
            auto origResultInfo = originalResults[i];
            auto canElem = tupleType.getElementType(i);
            PILType objectType = PILType::getPrimitiveObjectType(canElem);
            auto newResult = PILResultInfo(objectType.getAstType(), origResultInfo.getConvention());
            newPILResultInfo.push_back(newResult);
         }
      } else {
         assert(loweredTy->getNumResults() == 1 && "Expected a single result");
         auto origResultInfo = loweredTy->getSingleResult();
         auto newResult = PILResultInfo(newPILType.getAstType(), origResultInfo.getConvention());
         newPILResultInfo.push_back(newResult);
      }

      auto NewTy = PILFunctionType::get(
         loweredTy->getSubstGenericSignature(),
         loweredTy->getExtInfo(),
         loweredTy->getCoroutineKind(),
         loweredTy->getCalleeConvention(),
         loweredTy->getParameters(),
         loweredTy->getYields(),
         newPILResultInfo, loweredTy->getOptionalErrorResult(),
         loweredTy->getSubstitutions(), loweredTy->isGenericSignatureImplied(),
         F->getModule().getAstContext(),
         loweredTy->getWitnessMethodConformanceOrInvalid());
      F->rewriteLoweredTypeUnsafe(NewTy);
      return true;
   }
   return false;
}

void LoadableByAddress::runOnFunction(PILFunction *F) {
   CanPILFunctionType funcType = F->getLoweredFunctionType();
   IRGenModule *currIRMod = getIRGenModule()->IRGen.getGenModule(F);

   if (F->isExternalDeclaration()) {
      if (!modifiableFunction(funcType)) {
         return;
      }
      // External function - re-write external declaration - this is ABI!
      GenericEnvironment *genEnv = F->getGenericEnvironment();
      auto loweredTy = F->getLoweredFunctionType();
      if (!genEnv && loweredTy->getSubstGenericSignature()) {
         genEnv = getGenericEnvironment(loweredTy);
      }
      if (MapperCache.shouldTransformFunctionType(genEnv, loweredTy,
                                                  *currIRMod)) {
         modFuncs.insert(F);
      }
      return;
   }

   StructLoweringState pass(F, *currIRMod, MapperCache);

   // Rewrite function args and insert allocs.
   LoadableStorageAllocation allocator(pass);
   allocator.allocateLoadableStorage();

   bool rewrittenReturn = false;
   if (modifiableFunction(funcType)) {
      rewrittenReturn = rewriteFunctionReturn(pass);
   }

   LLVM_DEBUG(llvm::dbgs() << "\nREWRITING: " << F->getName();
                 F->print(llvm::dbgs()));

   // Rewrite instructions relating to the loadable struct.
   rewriteFunction(pass, allocator);

   invalidateAnalysis(F, PILAnalysis::InvalidationKind::Instructions);

   // If we modified the function arguments - add to list of functions to clone
   if (modifiableFunction(funcType) &&
       (rewrittenReturn ||
        !pass.largeLoadableArgs.empty() ||
        !pass.funcSigArgs.empty() ||
        pass.hasLargeLoadableYields())) {
      modFuncs.insert(F);
   }
   // If we modified any applies - add them to the global list for recreation
   if (!pass.applies.empty()) {
      modApplies.insert(pass.applies.begin(), pass.applies.end());
   }
   if (!pass.applyRetToAllocMap.empty()) {
      for (auto elm : pass.applyRetToAllocMap) {
         allApplyRetToAllocMap.insert(elm);
      }
   }
}

static PILValue
getOperandTypeWithCastIfNecessary(PILInstruction *containingInstr, PILValue op,
                                  IRGenModule &Mod, PILBuilder &builder,
                                  LargePILTypeMapper &Mapper) {
   PILType currPILType = op->getType();
   PILType nonOptionalType = currPILType;
   if (auto optType = currPILType.getOptionalObjectType()) {
      nonOptionalType = optType;
   }
   if (auto funcType = nonOptionalType.getAs<PILFunctionType>()) {
      GenericEnvironment *genEnv =
         containingInstr->getFunction()->getGenericEnvironment();
      if (!genEnv && funcType->isPolymorphic()) {
         genEnv = getGenericEnvironment(funcType);
      }
      auto newFnType = Mapper.getNewPILFunctionType(genEnv, funcType, Mod);
      PILType newPILType = PILType::getPrimitiveObjectType(newFnType);
      if (nonOptionalType.isAddress()) {
         newPILType = newPILType.getAddressType();
      }
      if (nonOptionalType != currPILType) {
         newPILType = PILType::getOptionalType(newPILType);
      }
      if (currPILType.isAddress()) {
         newPILType = newPILType.getAddressType();
      }
      if (currPILType.isAddress()) {
         if (newPILType != currPILType) {
            auto castInstr = builder.createUncheckedAddrCast(
               containingInstr->getLoc(), op, newPILType);
            return castInstr;
         }
         return op;
      }
      assert(currPILType.isObject() && "Expected an object type");
      if (newPILType != currPILType) {
         auto castInstr = builder.createUncheckedBitCast(containingInstr->getLoc(),
                                                         op, newPILType);
         return castInstr;
      }
   }
   return op;
}

void LoadableByAddress::recreateSingleApply(
   PILInstruction *applyInst, SmallVectorImpl<PILInstruction *> &Delete) {
   auto *F = applyInst->getFunction();
   IRGenModule *currIRMod = getIRGenModule()->IRGen.getGenModule(F);
   // Collect common info
   ApplySite applySite = ApplySite(applyInst);
   PILValue callee = applySite.getCallee();
   if (auto site = ApplySite::isa(callee)) {
      // We need to re-create the callee's apply before recreating this one
      // else verification will fail with wrong SubstCalleeType
      auto calleInstr = site.getInstruction();
      if (modApplies.remove(calleInstr)) {
         recreateSingleApply(calleInstr, Delete);
         callee = applySite.getCallee();
      }
   }
   CanPILFunctionType origPILFunctionType = applySite.getSubstCalleeType();
   GenericEnvironment *genEnv = nullptr;
   CanPILFunctionType newPILFunctionType = MapperCache.getNewPILFunctionType(
      genEnv, origPILFunctionType, *currIRMod);
   PILFunctionConventions newPILFunctionConventions(newPILFunctionType,
                                                    *getModule());
   SmallVector<PILValue, 8> callArgs;
   PILBuilderWithScope applyBuilder(applyInst);
   // If we turned a direct result into an indirect parameter
   // Find the new alloc we created earlier.
   // and pass it as first parameter:
   if ((isa<ApplyInst>(applyInst) || isa<TryApplyInst>(applyInst)) &&
       modNonFuncTypeResultType(genEnv, origPILFunctionType, *currIRMod) &&
       modifiableApply(applySite, *getIRGenModule())) {
      assert(allApplyRetToAllocMap.find(applyInst) !=
             allApplyRetToAllocMap.end());
      auto newAlloc = allApplyRetToAllocMap.find(applyInst)->second;
      callArgs.push_back(newAlloc);
   }

   // Collect arg operands
   for (Operand &operand : applySite.getArgumentOperands()) {
      PILValue currOperand = operand.get();
      currOperand = getOperandTypeWithCastIfNecessary(
         applyInst, currOperand, *currIRMod, applyBuilder, MapperCache);
      callArgs.push_back(currOperand);
   }
   // Recreate apply with new operands due to substitution-type cache
   switch (applyInst->getKind()) {
      case PILInstructionKind::ApplyInst: {
         auto *castedApply = cast<ApplyInst>(applyInst);
         PILValue newApply =
            applyBuilder.createApply(castedApply->getLoc(), callee,
                                     applySite.getSubstitutionMap(),
                                     callArgs, castedApply->isNonThrowing());
         castedApply->replaceAllUsesWith(newApply);
         break;
      }
      case PILInstructionKind::TryApplyInst: {
         auto *castedApply = cast<TryApplyInst>(applyInst);
         applyBuilder.createTryApply(
            castedApply->getLoc(), callee,
            applySite.getSubstitutionMap(), callArgs,
            castedApply->getNormalBB(), castedApply->getErrorBB());
         break;
      }
      case PILInstructionKind::BeginApplyInst: {
         auto oldApply = cast<BeginApplyInst>(applyInst);
         auto newApply =
            applyBuilder.createBeginApply(oldApply->getLoc(), callee,
                                          applySite.getSubstitutionMap(), callArgs,
                                          oldApply->isNonThrowing());

         // Use the new token result.
         oldApply->getTokenResult()->replaceAllUsesWith(newApply->getTokenResult());

         // Rewrite all the yields.
         auto oldYields = oldApply->getOrigCalleeType()->getYields();
         auto oldYieldedValues = oldApply->getYieldedValues();
         auto newYields = newApply->getOrigCalleeType()->getYields();
         auto newYieldedValues = newApply->getYieldedValues();
         assert(oldYields.size() == newYields.size() &&
                oldYields.size() == oldYieldedValues.size() &&
                newYields.size() == newYieldedValues.size());
         (void)newYields;
         for (auto i : indices(oldYields)) {
            PILValue oldValue = oldYieldedValues[i];
            PILValue newValue = newYieldedValues[i];

            // For now, just replace the value with an immediate load if the old value
            // was direct.
            if (oldValue->getType() != newValue->getType() &&
                !oldValue->getType().isAddress()) {
               LoadOwnershipQualifier ownership;
               if (!F->hasOwnership()) {
                  ownership = LoadOwnershipQualifier::Unqualified;
               } else if (newValue->getType().isTrivial(*F)) {
                  ownership = LoadOwnershipQualifier::Trivial;
               } else {
                  assert(oldYields[i].isConsumed() &&
                         "borrowed yields not yet supported here");
                  ownership = LoadOwnershipQualifier::Take;
               }
               newValue = applyBuilder.createLoad(applyInst->getLoc(), newValue,
                                                  ownership);
            }
            oldValue->replaceAllUsesWith(newValue);
         }
         break;
      }
      case PILInstructionKind::PartialApplyInst: {
         auto *castedApply = cast<PartialApplyInst>(applyInst);
         // Change the type of the Closure
         auto partialApplyConvention = castedApply->getType()
            .getAs<PILFunctionType>()
            ->getCalleeConvention();

         auto newApply = applyBuilder.createPartialApply(
            castedApply->getLoc(), callee, applySite.getSubstitutionMap(), callArgs,
            partialApplyConvention, castedApply->isOnStack());
         castedApply->replaceAllUsesWith(newApply);
         break;
      }
      default:
         llvm_unreachable("Unexpected instr: unknown apply type");
   }
   Delete.push_back(applyInst);
}

bool LoadableByAddress::recreateApply(
   PILInstruction &I, SmallVectorImpl<PILInstruction *> &Delete) {
   if (!modApplies.count(&I))
      return false;
   recreateSingleApply(&I, Delete);
   modApplies.remove(&I);
   return true;
}

bool LoadableByAddress::recreateLoadInstr(
   PILInstruction &I, SmallVectorImpl<PILInstruction *> &Delete) {
   auto *loadInstr = dyn_cast<LoadInst>(&I);
   if (!loadInstr || !loadInstrsOfFunc.count(loadInstr))
      return false;

   PILBuilderWithScope loadBuilder(loadInstr);
   // If this is a load of a function for which we changed the return type:
   // add UncheckedBitCast before the load
   auto loadOp = loadInstr->getOperand();
   loadOp = getOperandTypeWithCastIfNecessary(
      loadInstr, loadOp, *getIRGenModule(), loadBuilder, MapperCache);
   auto *newInstr = loadBuilder.createLoad(loadInstr->getLoc(), loadOp,
                                           loadInstr->getOwnershipQualifier());
   loadInstr->replaceAllUsesWith(newInstr);
   Delete.push_back(loadInstr);
   return true;
}

bool LoadableByAddress::recreateUncheckedEnumDataInstr(
   PILInstruction &I, SmallVectorImpl<PILInstruction *> &Delete) {
   auto enumInstr = dyn_cast<UncheckedEnumDataInst>(&I);
   if (!enumInstr || !uncheckedEnumDataOfFunc.count(enumInstr))
      return false;
   PILBuilderWithScope enumBuilder(enumInstr);
   PILFunction *F = enumInstr->getFunction();
   IRGenModule *currIRMod = getIRGenModule()->IRGen.getGenModule(F);
   PILType origType = enumInstr->getType();
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   PILType newType = MapperCache.getNewPILType(genEnv, origType, *currIRMod);
   auto caseTy = enumInstr->getOperand()->getType().getEnumElementType(
      enumInstr->getElement(), F->getModule(), TypeExpansionContext(*F));
   SingleValueInstruction *newInstr = nullptr;
   if (newType.isAddress()) {
      newType = newType.getObjectType();
   }
   if (caseTy != newType) {
      auto *takeEnum = enumBuilder.createUncheckedEnumData(
         enumInstr->getLoc(), enumInstr->getOperand(), enumInstr->getElement(),
         caseTy);
      newInstr = enumBuilder.createUncheckedBitCast(enumInstr->getLoc(), takeEnum,
                                                    newType);
   } else {
      newInstr = enumBuilder.createUncheckedEnumData(
         enumInstr->getLoc(), enumInstr->getOperand(), enumInstr->getElement(),
         newType);
   }
   enumInstr->replaceAllUsesWith(newInstr);
   Delete.push_back(enumInstr);
   return false;
}

bool LoadableByAddress::recreateUncheckedTakeEnumDataAddrInst(
   PILInstruction &I, SmallVectorImpl<PILInstruction *> &Delete) {
   auto *enumInstr = dyn_cast<UncheckedTakeEnumDataAddrInst>(&I);
   if (!enumInstr || !uncheckedTakeEnumDataAddrOfFunc.count(enumInstr))
      return false;
   PILBuilderWithScope enumBuilder(enumInstr);
   PILFunction *F = enumInstr->getFunction();
   IRGenModule *currIRMod = getIRGenModule()->IRGen.getGenModule(F);
   PILType origType = enumInstr->getType();
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   PILType newType = MapperCache.getNewPILType(genEnv, origType, *currIRMod);
   auto caseTy = enumInstr->getOperand()->getType().getEnumElementType(
      enumInstr->getElement(), F->getModule(), TypeExpansionContext(*F));
   SingleValueInstruction *newInstr = nullptr;
   if (caseTy != origType.getObjectType()) {
      auto *takeEnum = enumBuilder.createUncheckedTakeEnumDataAddr(
         enumInstr->getLoc(), enumInstr->getOperand(), enumInstr->getElement(),
         caseTy.getAddressType());
      newInstr = enumBuilder.createUncheckedAddrCast(
         enumInstr->getLoc(), takeEnum, newType.getAddressType());
   } else {
      newInstr = enumBuilder.createUncheckedTakeEnumDataAddr(
         enumInstr->getLoc(), enumInstr->getOperand(), enumInstr->getElement(),
         newType.getAddressType());
   }
   enumInstr->replaceAllUsesWith(newInstr);
   Delete.push_back(enumInstr);
   return true;
}

bool LoadableByAddress::fixStoreToBlockStorageInstr(
   PILInstruction &I, SmallVectorImpl<PILInstruction *> &Delete) {
   auto *instr = dyn_cast<StoreInst>(&I);
   if (!instr || !storeToBlockStorageInstrs.count(instr))
      return false;
   auto dest = instr->getDest();
   auto destBlock = cast<ProjectBlockStorageInst>(dest);
   PILType destType = destBlock->getType();
   auto src = instr->getSrc();
   PILType srcType = src->getType();
   if (destType.getObjectType() != srcType) {
      // Add cast to destType
      PILBuilderWithScope castBuilder(instr);
      auto *castInstr = castBuilder.createUncheckedBitCast(
         instr->getLoc(), src, destType.getObjectType());
      instr->setOperand(StoreInst::Src, castInstr);
   }
   return true;
}

bool LoadableByAddress::recreateTupleInstr(
   PILInstruction &I, SmallVectorImpl<PILInstruction *> &Delete) {
   auto *tupleInstr = dyn_cast<TupleInst>(&I);
   if (!tupleInstr)
      return false;

   // Check if we need to recreate the tuple:
   auto *F = tupleInstr->getFunction();
   auto *currIRMod = getIRGenModule()->IRGen.getGenModule(F);
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   auto resultTy = tupleInstr->getType();
   auto newResultTy = MapperCache.getNewPILType(genEnv, resultTy, *currIRMod);
   if (resultTy == newResultTy)
      return true;

   // The tuple type have changed based on its members.
   // For example if one or more of them are ‘large’ loadable types
   PILBuilderWithScope tupleBuilder(tupleInstr);
   SmallVector<PILValue, 8> elems;
   for (auto elem : tupleInstr->getElements()) {
      elems.push_back(elem);
   }
   auto *newTuple = tupleBuilder.createTuple(tupleInstr->getLoc(), elems);
   tupleInstr->replaceAllUsesWith(newTuple);
   Delete.push_back(tupleInstr);
   return true;
}

bool LoadableByAddress::recreateConvInstr(PILInstruction &I,
                                          SmallVectorImpl<PILInstruction *> &Delete) {
   auto *convInstr = dyn_cast<SingleValueInstruction>(&I);
   if (!convInstr || !conversionInstrs.count(convInstr))
      return false;
   IRGenModule *currIRMod =
      getIRGenModule()->IRGen.getGenModule(convInstr->getFunction());
   PILType currPILType = convInstr->getType();
   if (auto *thinToPointer = dyn_cast<ThinFunctionToPointerInst>(convInstr)) {
      currPILType = thinToPointer->getOperand()->getType();
   }
   auto currPILFunctionType = currPILType.castTo<PILFunctionType>();
   GenericEnvironment *genEnv =
      convInstr->getFunction()->getGenericEnvironment();
   CanPILFunctionType newFnType = MapperCache.getNewPILFunctionType(
      genEnv, currPILFunctionType, *currIRMod);
   PILType newType = PILType::getPrimitiveObjectType(newFnType);
   PILBuilderWithScope convBuilder(convInstr);
   SingleValueInstruction *newInstr = nullptr;
   switch (convInstr->getKind()) {
      case PILInstructionKind::ThinToThickFunctionInst: {
         auto instr = cast<ThinToThickFunctionInst>(convInstr);
         newInstr = convBuilder.createThinToThickFunction(
            instr->getLoc(), instr->getOperand(), newType);
         break;
      }
      case PILInstructionKind::ThinFunctionToPointerInst: {
         auto instr = cast<ThinFunctionToPointerInst>(convInstr);
         newType =
            MapperCache.getNewPILType(genEnv, instr->getType(), *getIRGenModule());
         newInstr = convBuilder.createThinFunctionToPointer(
            instr->getLoc(), instr->getOperand(), newType);
         break;
      }
      case PILInstructionKind::ConvertFunctionInst: {
         auto instr = cast<ConvertFunctionInst>(convInstr);
         newInstr = convBuilder.createConvertFunction(
            instr->getLoc(), instr->getOperand(), newType,
            instr->withoutActuallyEscaping());
         break;
      }
      case PILInstructionKind::ConvertEscapeToNoEscapeInst: {
         auto instr = cast<ConvertEscapeToNoEscapeInst>(convInstr);
         newInstr = convBuilder.createConvertEscapeToNoEscape(
            instr->getLoc(), instr->getOperand(), newType,
            instr->isLifetimeGuaranteed());
         break;
      }
      case PILInstructionKind::MarkDependenceInst: {
         auto instr = cast<MarkDependenceInst>(convInstr);
         newInstr = convBuilder.createMarkDependence(
            instr->getLoc(), instr->getValue(), instr->getBase());
         break;
      }
      default:
         llvm_unreachable("Unexpected conversion instruction");
   }
   convInstr->replaceAllUsesWith(newInstr);
   Delete.push_back(convInstr);
   return true;
}

bool LoadableByAddress::recreateBuiltinInstr(PILInstruction &I,
                                             SmallVectorImpl<PILInstruction *> &Delete) {
   auto builtinInstr = dyn_cast<BuiltinInst>(&I);
   if (!builtinInstr || !builtinInstrs.count(builtinInstr))
      return false;
   auto *currIRMod =
      getIRGenModule()->IRGen.getGenModule(builtinInstr->getFunction());
   auto *F = builtinInstr->getFunction();
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   auto resultTy = builtinInstr->getType();
   auto newResultTy = MapperCache.getNewPILType(genEnv, resultTy, *currIRMod);

   llvm::SmallVector<PILValue, 5> newArgs;
   for (auto oldArg : builtinInstr->getArguments()) {
      newArgs.push_back(oldArg);
   }

   PILBuilderWithScope builtinBuilder(builtinInstr);
   auto *newInstr = builtinBuilder.createBuiltin(
      builtinInstr->getLoc(), builtinInstr->getName(), newResultTy,
      builtinInstr->getSubstitutions(), newArgs);
   builtinInstr->replaceAllUsesWith(newInstr);
   Delete.push_back(builtinInstr);
   return true;
}

void LoadableByAddress::updateLoweredTypes(PILFunction *F) {
   IRGenModule *currIRMod = getIRGenModule()->IRGen.getGenModule(F);
   CanPILFunctionType funcType = F->getLoweredFunctionType();
   GenericEnvironment *genEnv = F->getGenericEnvironment();
   if (!genEnv && funcType->getSubstGenericSignature()) {
      genEnv = getGenericEnvironment(funcType);
   }
   auto newFuncTy =
      MapperCache.getNewPILFunctionType(genEnv, funcType, *currIRMod);
   F->rewriteLoweredTypeUnsafe(newFuncTy);
}

/// The entry point to this function transformation.
void LoadableByAddress::run() {
   // Set the PIL state before the PassManager has a chance to run
   // verification.
   getModule()->setStage(PILStage::Lowered);

   for (auto &F : *getModule())
      runOnFunction(&F);

   if (modFuncs.empty() && modApplies.empty()) {
      return;
   }

   // Scan the module for all references of the modified functions:
   llvm::SetVector<FunctionRefBaseInst *> funcRefs;
   for (PILFunction &CurrF : *getModule()) {
      for (PILBasicBlock &BB : CurrF) {
         for (PILInstruction &I : BB) {
            if (auto *FRI = dyn_cast<FunctionRefBaseInst>(&I)) {
               PILFunction *RefF = FRI->getInitiallyReferencedFunction();
               if (modFuncs.count(RefF) != 0) {
                  // Go over the uses and add them to lists to modify
                  //
                  // FIXME: Why aren't function_ref uses processed transitively?  And
                  // why is it necessary to visit uses at all if they will be visited
                  // later in this loop?
                  for (auto *user : FRI->getUses()) {
                     PILInstruction *currInstr = user->getUser();
                     switch (currInstr->getKind()) {
                        case PILInstructionKind::ApplyInst:
                        case PILInstructionKind::TryApplyInst:
                        case PILInstructionKind::BeginApplyInst:
                        case PILInstructionKind::PartialApplyInst: {
                           if (modApplies.count(currInstr) == 0) {
                              modApplies.insert(currInstr);
                           }
                           break;
                        }
                        case PILInstructionKind::ConvertFunctionInst:
                        case PILInstructionKind::ConvertEscapeToNoEscapeInst:
                        case PILInstructionKind::MarkDependenceInst:
                        case PILInstructionKind::ThinFunctionToPointerInst:
                        case PILInstructionKind::ThinToThickFunctionInst: {
                           conversionInstrs.insert(
                              cast<SingleValueInstruction>(currInstr));
                           break;
                        }
                        case PILInstructionKind::BuiltinInst: {
                           auto *instr = cast<BuiltinInst>(currInstr);
                           builtinInstrs.insert(instr);
                           break;
                        }
                        case PILInstructionKind::DebugValueAddrInst:
                        case PILInstructionKind::DebugValueInst: {
                           break;
                        }
                        default:
                           llvm_unreachable("Unhandled use of FunctionRefInst");
                     }
                  }
                  funcRefs.insert(FRI);
               }
            } else if (auto *Cvt = dyn_cast<MarkDependenceInst>(&I)) {
               PILValue val = Cvt->getValue();
               PILType currType = val->getType();
               if (auto fType = currType.getAs<PILFunctionType>()) {
                  if (modifiableFunction(fType)) {
                     conversionInstrs.insert(Cvt);
                  }
               }
            } else if (auto *Cvt = dyn_cast<ConvertEscapeToNoEscapeInst>(&I)) {
               PILValue val = Cvt->getConverted();
               PILType currType = val->getType();
               auto fType = currType.getAs<PILFunctionType>();
               assert(fType && "Expected PILFunctionType");
               if (modifiableFunction(fType)) {
                  conversionInstrs.insert(Cvt);
               }
            } else if (auto *CFI = dyn_cast<ConvertFunctionInst>(&I)) {
               PILValue val = CFI->getConverted();
               PILType currType = val->getType();
               auto fType = currType.getAs<PILFunctionType>();
               assert(fType && "Expected PILFunctionType");
               if (modifiableFunction(fType)) {
                  conversionInstrs.insert(CFI);
               }
            } else if (auto *TTI = dyn_cast<ThinToThickFunctionInst>(&I)) {

               auto canType = TTI->getCallee()->getType();
               auto fType = canType.castTo<PILFunctionType>();

               if (modifiableFunction(fType))
                  conversionInstrs.insert(TTI);

            } else if (auto *LI = dyn_cast<LoadInst>(&I)) {
               loadInstrsOfFunc.insert(LI);
            } else if (auto *UED = dyn_cast<UncheckedEnumDataInst>(&I)) {
               uncheckedEnumDataOfFunc.insert(UED);
            } else if (auto *UED = dyn_cast<UncheckedTakeEnumDataAddrInst>(&I)) {
               uncheckedTakeEnumDataAddrOfFunc.insert(UED);
            } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
               auto dest = SI->getDest();
               if (isa<ProjectBlockStorageInst>(dest)) {
                  storeToBlockStorageInstrs.insert(SI);
               }
            } else if (auto *PAI = dyn_cast<PartialApplyInst>(&I)) {
               if (modApplies.count(PAI) == 0) {
                  modApplies.insert(PAI);
               }
            }
         }
      }
   }

   for (auto *F : modFuncs) {
      // Update the lowered type of the Function
      updateLoweredTypes(F);
   }

   // Update all references:
   // Note: We don't need to update the witness tables and vtables
   // They just contain a pointer to the function
   // The pointer does not change
   for (auto *instr : funcRefs) {
      PILFunction *F = instr->getInitiallyReferencedFunction();
      PILBuilderWithScope refBuilder(instr);
      SingleValueInstruction *newInstr =
         refBuilder.createFunctionRef(instr->getLoc(), F, instr->getKind());
      instr->replaceAllUsesWith(newInstr);
      instr->getParent()->erase(instr);
   }

   // Recreate the instructions in topological order. Some instructions inherit
   // their result type from their operand.
   for (PILFunction &CurrF : *getModule()) {
      SmallVector<PILInstruction *, 32> Delete;
      for (PILBasicBlock &BB : CurrF) {
         for (PILInstruction &I : BB) {
            if (recreateTupleInstr(I, Delete))
               continue;
            else if (recreateConvInstr(I, Delete))
               continue;
            else if (recreateBuiltinInstr(I, Delete))
               continue;
            else if (recreateUncheckedEnumDataInstr(I, Delete))
               continue;
            else if (recreateUncheckedTakeEnumDataAddrInst(I, Delete))
               continue;
            else if (recreateLoadInstr(I, Delete))
               continue;
            else if (recreateApply(I, Delete))
               continue;
            else
               fixStoreToBlockStorageInstr(I, Delete);
         }
      }
      for (auto *Inst : Delete)
         Inst->eraseFromParent();
   }

   // Clean up the data structs:
   modFuncs.clear();
   conversionInstrs.clear();
   loadInstrsOfFunc.clear();
   uncheckedEnumDataOfFunc.clear();
   modApplies.clear();
   storeToBlockStorageInstrs.clear();
}

PILTransform *irgen::createLoadableByAddress() {
   return new LoadableByAddress();
}
