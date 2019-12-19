//===--- PILLowerAggregateInstrs.cpp - Aggregate insts to Scalar insts  ---===//
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
//
// Simplify aggregate instructions into scalar instructions.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-lower-aggregate-instrs"

#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "polarphp/pil/lang/TypeLowering.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"

using namespace polar;
using namespace polar::lowering;

STATISTIC(NumExpand, "Number of instructions expanded");

//===----------------------------------------------------------------------===//
//                      Higher Level Operation Expansion
//===----------------------------------------------------------------------===//

/// Lower copy_addr into loads/stores/retain/release if we have a
/// non-address only type. We do this here so we can process the resulting
/// loads/stores.
///
/// This peephole implements the following optimizations:
///
/// copy_addr %0 to %1 : $*T
/// ->
///     %new = load %0 : $*T        // Load the new value from the source
///     %old = load %1 : $*T        // Load the old value from the destination
///     strong_retain %new : $T     // Retain the new value
///     strong_release %old : $T    // Release the old
///     store %new to %1 : $*T      // Store the new value to the destination
///
/// copy_addr [take] %0 to %1 : $*T
/// ->
///     %new = load %0 : $*T
///     %old = load %1 : $*T
///     // no retain of %new!
///     strong_release %old : $T
///     store %new to %1 : $*T
///
/// copy_addr %0 to [initialization] %1 : $*T
/// ->
///     %new = load %0 : $*T
///     strong_retain %new : $T
///     // no load/release of %old!
///     store %new to %1 : $*T
///
/// copy_addr [take] %0 to [initialization] %1 : $*T
/// ->
///     %new = load %0 : $*T
///     // no retain of %new!
///     // no load/release of %old!
///     store %new to %1 : $*T
static bool expandCopyAddr(CopyAddrInst *CA) {
   PILModule &M = CA->getModule();
   PILFunction *F = CA->getFunction();
   PILValue Source = CA->getSrc();

   // If we have an address only type don't do anything.
   PILType SrcType = Source->getType();
   if (SrcType.isAddressOnly(*F))
      return false;

   bool expand = shouldExpand(M, SrcType.getObjectType());
   using TypeExpansionKind = lowering::TypeLowering::TypeExpansionKind;
   auto expansionKind = expand ? TypeExpansionKind::MostDerivedDescendents
                               : TypeExpansionKind::None;

   PILBuilderWithScope Builder(CA);

   // %new = load %0 : $*T
   LoadInst *New = Builder.createLoad(CA->getLoc(), Source,
                                      LoadOwnershipQualifier::Unqualified);

   PILValue Destination = CA->getDest();

   // If our object type is not trivial, we may need to release the old value and
   // retain the new one.

   auto &TL = F->getTypeLowering(SrcType);

   // If we have a non-trivial type...
   if (!TL.isTrivial()) {

      // If we are not initializing:
      // %old = load %1 : $*T
      IsInitialization_t IsInit = CA->isInitializationOfDest();
      LoadInst *Old = nullptr;
      if (IsInitialization_t::IsNotInitialization == IsInit) {
         Old = Builder.createLoad(CA->getLoc(), Destination,
                                  LoadOwnershipQualifier::Unqualified);
      }

      // If we are not taking and have a reference type:
      //   strong_retain %new : $*T
      // or if we have a non-trivial non-reference type.
      //   retain_value %new : $*T
      IsTake_t IsTake = CA->isTakeOfSrc();
      if (IsTake_t::IsNotTake == IsTake) {
         TL.emitLoweredCopyValue(Builder, CA->getLoc(), New, expansionKind);
      }

      // If we are not initializing:
      // strong_release %old : $*T
      //   *or*
      // release_value %old : $*T
      if (Old) {
         TL.emitLoweredDestroyValue(Builder, CA->getLoc(), Old, expansionKind);
      }
   }

   // Create the store.
   Builder.createStore(CA->getLoc(), New, Destination,
                       StoreOwnershipQualifier::Unqualified);

   ++NumExpand;
   return true;
}

static bool expandDestroyAddr(DestroyAddrInst *DA) {
   PILFunction *F = DA->getFunction();
   PILModule &Module = DA->getModule();
   PILBuilderWithScope Builder(DA);

   // Strength reduce destroy_addr inst into release/store if
   // we have a non-address only type.
   PILValue Addr = DA->getOperand();

   // If we have an address only type, do nothing.
   PILType Type = Addr->getType();
   if (Type.isAddressOnly(*F))
      return false;

   bool expand = shouldExpand(Module, Type.getObjectType());

   // If we have a non-trivial type...
   if (!Type.isTrivial(*F)) {
      // If we have a type with reference semantics, emit a load/strong release.
      LoadInst *LI = Builder.createLoad(DA->getLoc(), Addr,
                                        LoadOwnershipQualifier::Unqualified);
      auto &TL = F->getTypeLowering(Type);
      using TypeExpansionKind = lowering::TypeLowering::TypeExpansionKind;
      auto expansionKind = expand ? TypeExpansionKind::MostDerivedDescendents
                                  : TypeExpansionKind::None;
      TL.emitLoweredDestroyValue(Builder, DA->getLoc(), LI, expansionKind);
   }

   ++NumExpand;
   return true;
}

static bool expandReleaseValue(ReleaseValueInst *DV) {
   PILFunction *F = DV->getFunction();
   PILModule &Module = DV->getModule();
   PILBuilderWithScope Builder(DV);

   // Strength reduce destroy_addr inst into release/store if
   // we have a non-address only type.
   PILValue Value = DV->getOperand();

   // If we have an address only type, do nothing.
   PILType Type = Value->getType();
   assert(!PILModuleConventions(Module).useLoweredAddresses()
          || Type.isLoadable(*F) &&
             "release_value should never be called on a non-loadable type.");

   if (!shouldExpand(Module, Type.getObjectType()))
      return false;

   auto &TL = F->getTypeLowering(Type);
   TL.emitLoweredDestroyValueMostDerivedDescendents(Builder, DV->getLoc(),
                                                    Value);

   LLVM_DEBUG(llvm::dbgs() << "    Expanding Destroy Value: " << *DV);

   ++NumExpand;
   return true;
}

static bool expandRetainValue(RetainValueInst *CV) {
   PILFunction *F = CV->getFunction();
   PILModule &Module = CV->getModule();
   PILBuilderWithScope Builder(CV);

   // Strength reduce destroy_addr inst into release/store if
   // we have a non-address only type.
   PILValue Value = CV->getOperand();

   // If we have an address only type, do nothing.
   PILType Type = Value->getType();
   assert(!PILModuleConventions(Module).useLoweredAddresses()
          || Type.isLoadable(*F) &&
             "Copy Value can only be called on loadable types.");

   if (!shouldExpand(Module, Type.getObjectType()))
      return false;

   auto &TL = F->getTypeLowering(Type);
   TL.emitLoweredCopyValueMostDerivedDescendents(Builder, CV->getLoc(), Value);

   LLVM_DEBUG(llvm::dbgs() << "    Expanding Copy Value: " << *CV);

   ++NumExpand;
   return true;
}

//===----------------------------------------------------------------------===//
//                              Top Level Driver
//===----------------------------------------------------------------------===//

static bool processFunction(PILFunction &Fn) {
   bool Changed = false;
   for (auto BI = Fn.begin(), BE = Fn.end(); BI != BE; ++BI) {
      auto II = BI->begin(), IE = BI->end();
      while (II != IE) {
         PILInstruction *Inst = &*II;

         LLVM_DEBUG(llvm::dbgs() << "Visiting: " << *Inst);

         if (auto *CA = dyn_cast<CopyAddrInst>(Inst))
            if (expandCopyAddr(CA)) {
               ++II;
               CA->eraseFromParent();
               Changed = true;
               continue;
            }

         if (auto *DA = dyn_cast<DestroyAddrInst>(Inst))
            if (expandDestroyAddr(DA)) {
               ++II;
               DA->eraseFromParent();
               Changed = true;
               continue;
            }

         if (auto *CV = dyn_cast<RetainValueInst>(Inst))
            if (expandRetainValue(CV)) {
               ++II;
               CV->eraseFromParent();
               Changed = true;
               continue;
            }

         if (auto *DV = dyn_cast<ReleaseValueInst>(Inst))
            if (expandReleaseValue(DV)) {
               ++II;
               DV->eraseFromParent();
               Changed = true;
               continue;
            }

         ++II;
      }
   }
   return Changed;
}

namespace {
class PILLowerAggregate : public PILFunctionTransform {

   /// The entry point to the transformation.
   void run() override {
      PILFunction *F = getFunction();
      // FIXME: Can we support ownership?
      if (F->hasOwnership())
         return;
      LLVM_DEBUG(llvm::dbgs() << "***** LowerAggregate on function: " <<
                              F->getName() << " *****\n");
      bool Changed = processFunction(*F);
      if (Changed) {
         invalidateAnalysis(PILAnalysis::InvalidationKind::CallsAndInstructions);
      }
   }

};
} // end anonymous namespace

PILTransform *polar::createLowerAggregateInstrs() {
   return new PILLowerAggregate();
}
