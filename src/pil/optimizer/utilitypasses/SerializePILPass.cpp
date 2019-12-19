//===--- SerializePILPass.cpp ---------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "serialize-pil"

#include "polarphp/global/NameStrings.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/BasicBlockOptUtils.h"

using namespace polar;

namespace {
/// In place map opaque archetypes to their underlying type in a function.
/// This needs to happen when a function changes from serializable to not
/// serializable.
class MapOpaqueArchetypes : public PILCloner<MapOpaqueArchetypes> {
  using SuperTy = PILCloner<MapOpaqueArchetypes>;

  PILBasicBlock *origEntryBlock;
  PILBasicBlock *clonedEntryBlock;
public:
  friend class PILCloner<MapOpaqueArchetypes>;
  friend class PILCloner<MapOpaqueArchetypes>;
  friend class PILInstructionVisitor<MapOpaqueArchetypes>;

  MapOpaqueArchetypes(PILFunction &fun) : SuperTy(fun) {
    origEntryBlock = fun.getEntryBlock();
    clonedEntryBlock = fun.createBasicBlock();
  }

  PILType remapType(PILType Ty) {
    if (!Ty.getAstType()->hasOpaqueArchetype() ||
        !getBuilder()
             .getTypeExpansionContext()
             .shouldLookThroughOpaqueTypeArchetypes())
      return Ty;

    return getBuilder().getTypeLowering(Ty).getLoweredType().getCategoryType(
        Ty.getCategory());
  }

  CanType remapASTType(CanType ty) {
    if (!ty->hasOpaqueArchetype() ||
        !getBuilder()
             .getTypeExpansionContext()
             .shouldLookThroughOpaqueTypeArchetypes())
      return ty;
    // Remap types containing opaque result types in the current context.
    return getBuilder()
        .getTypeLowering(PILType::getPrimitiveObjectType(ty))
        .getLoweredType()
        .getAstType();
  }

  InterfaceConformanceRef remapConformance(Type ty,
                                          InterfaceConformanceRef conf) {
    auto context = getBuilder().getTypeExpansionContext();
    auto conformance = conf;
    if (ty->hasOpaqueArchetype() &&
        context.shouldLookThroughOpaqueTypeArchetypes()) {
      conformance =
          substOpaqueTypesWithUnderlyingTypes(conformance, ty, context);
    }
    return conformance;
  }

  void replace();
};
} // namespace

void MapOpaqueArchetypes::replace() {
  // Map the function arguments.
  SmallVector<PILValue, 4> entryArgs;
  entryArgs.reserve(origEntryBlock->getArguments().size());
  for (auto &origArg : origEntryBlock->getArguments()) {
    PILType mappedType = remapType(origArg->getType());
    auto *NewArg = clonedEntryBlock->createFunctionArgument(
        mappedType, origArg->getDecl(), true);
    entryArgs.push_back(NewArg);
  }

  getBuilder().setInsertionPoint(clonedEntryBlock);
  auto &fn = getBuilder().getFunction();
  cloneFunctionBody(&fn, clonedEntryBlock, entryArgs,
                    true /*replaceOriginalFunctionInPlace*/);
  // Insert the new entry block at the beginning.
  fn.getBlocks().splice(fn.getBlocks().begin(), fn.getBlocks(),
                        clonedEntryBlock);
  removeUnreachableBlocks(fn);
}

static bool opaqueArchetypeWouldChange(TypeExpansionContext context,
                                       CanType ty) {
  if (!ty->hasOpaqueArchetype())
    return false;

  return ty.findIf([=](Type type) -> bool {
    if (auto opaqueTy = type->getAs<OpaqueTypeArchetypeType>()) {
      auto opaque = opaqueTy->getDecl();
      auto module = context.getContext()->getParentModule();
      OpaqueSubstitutionKind subKind =
          ReplaceOpaqueTypesWithUnderlyingTypes::shouldPerformSubstitution(
              opaque, module, context.getResilienceExpansion());
      return subKind != OpaqueSubstitutionKind::DontSubstitute;
    }
    return false;
  });
}

static bool hasOpaqueArchetypeOperand(TypeExpansionContext context,
                                      PILInstruction &inst) {
  // Check the operands for opaque types.
  for (auto &opd : inst.getAllOperands())
    if (opaqueArchetypeWouldChange(context, opd.get()->getType().getAstType()))
      return true;
  return false;
}

static bool hasOpaqueArchetypeResult(TypeExpansionContext context,
                                     PILInstruction &inst) {
  // Check the results for opaque types.
  for (const auto &res : inst.getResults())
    if (opaqueArchetypeWouldChange(context, res->getType().getAstType()))
      return true;
  return false;
}

static bool hasOpaqueArchetype(TypeExpansionContext context,
                               PILInstruction &inst) {
  // Check operands and results.
  if (hasOpaqueArchetypeOperand(context, inst))
    return true;
  if (hasOpaqueArchetypeResult(context, inst))
    return true;

  // Check substitution maps.
  switch (inst.getKind()) {
  case PILInstructionKind::AllocStackInst:
  case PILInstructionKind::AllocRefInst:
  case PILInstructionKind::AllocRefDynamicInst:
  case PILInstructionKind::AllocValueBufferInst:
  case PILInstructionKind::AllocBoxInst:
  case PILInstructionKind::AllocExistentialBoxInst:
  case PILInstructionKind::IndexAddrInst:
  case PILInstructionKind::TailAddrInst:
  case PILInstructionKind::IndexRawPointerInst:
  case PILInstructionKind::FunctionRefInst:
  case PILInstructionKind::DynamicFunctionRefInst:
  case PILInstructionKind::PreviousDynamicFunctionRefInst:
  case PILInstructionKind::GlobalAddrInst:
  case PILInstructionKind::GlobalValueInst:
  case PILInstructionKind::IntegerLiteralInst:
  case PILInstructionKind::FloatLiteralInst:
  case PILInstructionKind::StringLiteralInst:
  case PILInstructionKind::ClassMethodInst:
  case PILInstructionKind::SuperMethodInst:
  case PILInstructionKind::ObjCMethodInst:
  case PILInstructionKind::ObjCSuperMethodInst:
  case PILInstructionKind::WitnessMethodInst:
  case PILInstructionKind::UpcastInst:
  case PILInstructionKind::AddressToPointerInst:
  case PILInstructionKind::PointerToAddressInst:
  case PILInstructionKind::UncheckedRefCastInst:
  case PILInstructionKind::UncheckedAddrCastInst:
  case PILInstructionKind::UncheckedTrivialBitCastInst:
  case PILInstructionKind::UncheckedBitwiseCastInst:
  case PILInstructionKind::RefToRawPointerInst:
  case PILInstructionKind::RawPointerToRefInst:
#define LOADABLE_REF_STORAGE(Name, ...)                                        \
  case PILInstructionKind::RefTo##Name##Inst:                                  \
  case PILInstructionKind::Name##ToRefInst:
#include "polarphp/ast/ReferenceStorageDef.h"
#undef LOADABLE_REF_STORAGE_HELPER
  case PILInstructionKind::ConvertFunctionInst:
  case PILInstructionKind::ConvertEscapeToNoEscapeInst:
  case PILInstructionKind::ThinFunctionToPointerInst:
  case PILInstructionKind::PointerToThinFunctionInst:
  case PILInstructionKind::RefToBridgeObjectInst:
  case PILInstructionKind::BridgeObjectToRefInst:
  case PILInstructionKind::BridgeObjectToWordInst:
  case PILInstructionKind::ThinToThickFunctionInst:
  // @todo
//  case PILInstructionKind::ThickToObjCMetatypeInst:
//  case PILInstructionKind::ObjCToThickMetatypeInst:
//  case PILInstructionKind::ObjCMetatypeToObjectInst:
//  case PILInstructionKind::ObjCExistentialMetatypeToObjectInst:
  case PILInstructionKind::UnconditionalCheckedCastValueInst:
  case PILInstructionKind::UnconditionalCheckedCastInst:
  case PILInstructionKind::ClassifyBridgeObjectInst:
  case PILInstructionKind::ValueToBridgeObjectInst:
  case PILInstructionKind::MarkDependenceInst:
  case PILInstructionKind::CopyBlockInst:
  case PILInstructionKind::CopyBlockWithoutEscapingInst:
  case PILInstructionKind::CopyValueInst:
#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#include "polarphp/ast/ReferenceStorageDef.h"
#undef UNCHECKED_REF_STORAGE
#undef ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE
  case PILInstructionKind::UncheckedOwnershipConversionInst:
  case PILInstructionKind::IsUniqueInst:
  case PILInstructionKind::IsEscapingClosureInst:
  case PILInstructionKind::LoadInst:
  case PILInstructionKind::LoadBorrowInst:
  case PILInstructionKind::BeginBorrowInst:
  case PILInstructionKind::StoreBorrowInst:
  case PILInstructionKind::BeginAccessInst:
#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, name, ...)       \
  case PILInstructionKind::Load##Name##Inst:
#include "polarphp/ast/ReferenceStorageDef.h"
#undef NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE
  case PILInstructionKind::MarkUninitializedInst:
  case PILInstructionKind::ProjectValueBufferInst:
  case PILInstructionKind::ProjectBoxInst:
  case PILInstructionKind::ProjectExistentialBoxInst:
  case PILInstructionKind::BuiltinInst:
  case PILInstructionKind::MetatypeInst:
  case PILInstructionKind::ValueMetatypeInst:
  case PILInstructionKind::ExistentialMetatypeInst:
  // @todo
//  case PILInstructionKind::ObjCInterfaceInst:
  case PILInstructionKind::ObjectInst:
  case PILInstructionKind::TupleInst:
  case PILInstructionKind::TupleExtractInst:
  case PILInstructionKind::TupleElementAddrInst:
  case PILInstructionKind::StructInst:
  case PILInstructionKind::StructExtractInst:
  case PILInstructionKind::StructElementAddrInst:
  case PILInstructionKind::RefElementAddrInst:
  case PILInstructionKind::RefTailAddrInst:
  case PILInstructionKind::EnumInst:
  case PILInstructionKind::UncheckedEnumDataInst:
  case PILInstructionKind::InitEnumDataAddrInst:
  case PILInstructionKind::UncheckedTakeEnumDataAddrInst:
  case PILInstructionKind::SelectEnumInst:
  case PILInstructionKind::SelectEnumAddrInst:
  case PILInstructionKind::SelectValueInst:
  case PILInstructionKind::InitExistentialAddrInst:
  case PILInstructionKind::InitExistentialValueInst:
  case PILInstructionKind::OpenExistentialAddrInst:
  case PILInstructionKind::InitExistentialRefInst:
  case PILInstructionKind::OpenExistentialRefInst:
  case PILInstructionKind::InitExistentialMetatypeInst:
  case PILInstructionKind::OpenExistentialMetatypeInst:
  case PILInstructionKind::OpenExistentialBoxInst:
  case PILInstructionKind::OpenExistentialValueInst:
  case PILInstructionKind::OpenExistentialBoxValueInst:
  case PILInstructionKind::ProjectBlockStorageInst:
  case PILInstructionKind::InitBlockStorageHeaderInst:
  case PILInstructionKind::KeyPathInst:
  case PILInstructionKind::UnreachableInst:
  case PILInstructionKind::ReturnInst:
  case PILInstructionKind::ThrowInst:
  case PILInstructionKind::YieldInst:
  case PILInstructionKind::UnwindInst:
  case PILInstructionKind::BranchInst:
  case PILInstructionKind::CondBranchInst:
  case PILInstructionKind::SwitchValueInst:
  case PILInstructionKind::SwitchEnumInst:
  case PILInstructionKind::SwitchEnumAddrInst:
  case PILInstructionKind::DynamicMethodBranchInst:
  case PILInstructionKind::CheckedCastBranchInst:
  case PILInstructionKind::CheckedCastAddrBranchInst:
  case PILInstructionKind::CheckedCastValueBranchInst:
  case PILInstructionKind::DeallocStackInst:
  case PILInstructionKind::DeallocRefInst:
  case PILInstructionKind::DeallocPartialRefInst:
  case PILInstructionKind::DeallocValueBufferInst:
  case PILInstructionKind::DeallocBoxInst:
  case PILInstructionKind::DeallocExistentialBoxInst:
  case PILInstructionKind::StrongRetainInst:
  case PILInstructionKind::StrongReleaseInst:
  case PILInstructionKind::UnmanagedRetainValueInst:
  case PILInstructionKind::UnmanagedReleaseValueInst:
  case PILInstructionKind::UnmanagedAutoreleaseValueInst:
#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  case PILInstructionKind::StrongRetain##Name##Inst:                           \
  case PILInstructionKind::Name##RetainInst:                                   \
  case PILInstructionKind::Name##ReleaseInst:
#include "polarphp/ast/ReferenceStorageDef.h"
#undef ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE
  case PILInstructionKind::RetainValueInst:
  case PILInstructionKind::RetainValueAddrInst:
  case PILInstructionKind::ReleaseValueInst:
  case PILInstructionKind::ReleaseValueAddrInst:
  case PILInstructionKind::SetDeallocatingInst:
  case PILInstructionKind::AutoreleaseValueInst:
  case PILInstructionKind::BindMemoryInst:
  case PILInstructionKind::FixLifetimeInst:
  case PILInstructionKind::DestroyValueInst:
  case PILInstructionKind::EndBorrowInst:
  case PILInstructionKind::EndAccessInst:
  case PILInstructionKind::BeginUnpairedAccessInst:
  case PILInstructionKind::EndUnpairedAccessInst:
  case PILInstructionKind::StoreInst:
  case PILInstructionKind::AssignInst:
  case PILInstructionKind::AssignByWrapperInst:
  case PILInstructionKind::MarkFunctionEscapeInst:
  case PILInstructionKind::DebugValueInst:
  case PILInstructionKind::DebugValueAddrInst:
#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)             \
  case PILInstructionKind::Store##Name##Inst:
#include "polarphp/ast/ReferenceStorageDef.h"
  case PILInstructionKind::CopyAddrInst:
  case PILInstructionKind::DestroyAddrInst:
  case PILInstructionKind::EndLifetimeInst:
  case PILInstructionKind::InjectEnumAddrInst:
  case PILInstructionKind::DeinitExistentialAddrInst:
  case PILInstructionKind::DeinitExistentialValueInst:
  case PILInstructionKind::UnconditionalCheckedCastAddrInst:
  case PILInstructionKind::UncheckedRefCastAddrInst:
  case PILInstructionKind::AllocGlobalInst:
  case PILInstructionKind::EndApplyInst:
  case PILInstructionKind::AbortApplyInst:
  case PILInstructionKind::CondFailInst:
  case PILInstructionKind::DestructureStructInst:
  case PILInstructionKind::DestructureTupleInst:
    // Handle by operand and result check.
    break;

  case PILInstructionKind::ApplyInst:
  case PILInstructionKind::PartialApplyInst:
  case PILInstructionKind::TryApplyInst:
  case PILInstructionKind::BeginApplyInst:
    // Check substitution map.
    auto apply = ApplySite(&inst);
    auto subs = apply.getSubstitutionMap();
    for (auto ty: subs.getReplacementTypes()) {
       if (opaqueArchetypeWouldChange(context, ty->getCanonicalType()))
         return true;
    }
    break;
  }

  return false;
}

static bool hasOpaqueArchetypeArgument(TypeExpansionContext context, PILBasicBlock &BB) {
  for (auto *arg : BB.getArguments()) {
    if (opaqueArchetypeWouldChange(context, arg->getType().getAstType()))
      return true;
  }
  return false;
}

static bool hasAnyOpaqueArchetype(PILFunction &F) {
  bool foundOpaqueArchetype = false;
  auto context = F.getTypeExpansionContext();
  for (auto &BB : F) {
    // Check basic block argument types.
    if (hasOpaqueArchetypeArgument(context, BB)) {
      foundOpaqueArchetype = true;
      break;
    }

    // Check instruction results and operands.
    for (auto &inst : BB) {
      if (hasOpaqueArchetype(context, inst)) {
        foundOpaqueArchetype = true;
        break;
      }
    }

    if (foundOpaqueArchetype)
      break;
  }

  return foundOpaqueArchetype;
}

void updateOpaqueArchetypes(PILFunction &F) {
  // Only map if there are opaque archetypes that could change.
  if (!hasAnyOpaqueArchetype(F))
    return;

  MapOpaqueArchetypes(F).replace();
}

/// A utility pass to serialize a PILModule at any place inside the optimization
/// pipeline.
class SerializePILPass : public PILModuleTransform {
  /// Removes [serialized] from all functions. This allows for more
  /// optimizations and for a better dead function elimination.
  void removeSerializedFlagFromAllFunctions(PILModule &M) {
    for (auto &F : M) {
      bool wasSerialized = F.isSerialized() != IsNotSerialized;
      F.setSerialized(IsNotSerialized);

      // We are removing [serialized] from the function. This will change how
      // opaque archetypes are lowered in PIL - they might lower to their
      // underlying type. Update the function's opaque archetypes.
      if (wasSerialized && F.isDefinition()) {
        updateOpaqueArchetypes(F);
        invalidateAnalysis(&F, PILAnalysis::InvalidationKind::Everything);
      }

      // After serialization we don't need to keep @alwaysEmitIntoClient
      // functions alive, i.e. we don't need to treat them as public functions.
      if (F.getLinkage() == PILLinkage::PublicNonABI && M.isWholeModule())
        F.setLinkage(PILLinkage::Shared);
    }

    for (auto &WT : M.getWitnessTables()) {
      WT.setSerialized(IsNotSerialized);
    }

    for (auto &VT : M.getVTables()) {
      VT.setSerialized(IsNotSerialized);
    }
  }

public:
  SerializePILPass() {}
  void run() override {
    auto &M = *getModule();
    // Nothing to do if the module was serialized already.
    if (M.isSerialized())
      return;

    // Mark all reachable functions as "anchors" so that they are not
    // removed later by the dead function elimination pass. This
    // is required, because clients may reference any of the
    // serialized functions or anything referenced from them. Therefore,
    // to avoid linker errors, the object file of the current module should
    // contain all the symbols which were alive at the time of serialization.
    LLVM_DEBUG(llvm::dbgs() << "Serializing PILModule in SerializePILPass\n");
    M.serialize();

    // If we are not optimizing, do not strip the [serialized] flag. We *could*
    // do this since after serializing [serialized] is irrelevent. But this
    // would incur an unnecessary compile time cost since if we are not
    // optimizing we are not going to perform any sort of DFE.
    if (!getOptions().shouldOptimize())
      return;
    removeSerializedFlagFromAllFunctions(M);
  }
};

PILTransform *polar::createSerializePILPass() {
  return new SerializePILPass();
}
