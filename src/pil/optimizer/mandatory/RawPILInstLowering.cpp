//===--- RawPILInstLowering.cpp -------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "raw-pil-inst-lowering"

#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "llvm/ADT/Statistic.h"

STATISTIC(numAssignRewritten, "Number of assigns rewritten");

using namespace polar;

/// Emit the sequence that an assign instruction lowers to once we know
/// if it is an initialization or an assignment.  If it is an assignment,
/// a live-in value can be provided to optimize out the reload.
static void lowerAssignInstruction(PILBuilderWithScope &b, AssignInst *inst) {
   LLVM_DEBUG(llvm::dbgs() << "  *** Lowering [isInit="
                           << unsigned(inst->getOwnershipQualifier())
                           << "]: " << *inst << "\n");

   ++numAssignRewritten;

   PILValue src = inst->getSrc();
   PILValue dest = inst->getDest();
   PILLocation loc = inst->getLoc();
   AssignOwnershipQualifier qualifier = inst->getOwnershipQualifier();

   // Unknown qualifier is considered unprocessed. Just lower it as [reassign],
   // but if the destination type is trivial, treat it as [init].
   //
   // Unknown should not be lowered because definite initialization should
   // always set an initialization kind for assign instructions, but there exists
   // some situations where PILGen doesn't generate a mark_uninitialized
   // instruction for a full mark_uninitialized. Thus definite initialization
   // doesn't set an initialization kind for some assign instructions.
   //
   // TODO: Fix PILGen so that this is an assert preventing the lowering of
   //       Unknown init kind.
   if (qualifier == AssignOwnershipQualifier::Unknown)
      qualifier = AssignOwnershipQualifier::Reassign;

   if (qualifier == AssignOwnershipQualifier::Init ||
       inst->getDest()->getType().isTrivial(*inst->getFunction())) {

      // If this is an initialization, or the storage type is trivial, we
      // can just replace the assignment with a store.
      assert(qualifier != AssignOwnershipQualifier::Reinit);
      b.createTrivialStoreOr(loc, src, dest, StoreOwnershipQualifier::Init);
      inst->eraseFromParent();
      return;
   }

   if (qualifier == AssignOwnershipQualifier::Reinit) {
      // We have a case where a convenience initializer on a class
      // delegates to a factory initializer from a protocol extension.
      // Factory initializers give us a whole new instance, so the existing
      // instance, which has not been initialized and never will be, must be
      // freed using dealloc_partial_ref.
      PILValue pointer = b.createLoad(loc, dest, LoadOwnershipQualifier::Take);
      b.createStore(loc, src, dest, StoreOwnershipQualifier::Init);

      auto metatypeTy = CanMetatypeType::get(
         dest->getType().getAstType(), MetatypeRepresentation::Thick);
      auto silMetatypeTy = PILType::getPrimitiveObjectType(metatypeTy);
      PILValue metatype = b.createValueMetatype(loc, silMetatypeTy, pointer);

      b.createDeallocPartialRef(loc, pointer, metatype);
      inst->eraseFromParent();
      return;
   }

   assert(qualifier == AssignOwnershipQualifier::Reassign);
   // Otherwise, we need to replace the assignment with a store [assign] which
   // lowers to the load/store/release dance. Note that the new value is already
   // considered to be retained (by the semantics of the storage type),
   // and we're transferring that ownership count into the destination.

   b.createStore(loc, src, dest, StoreOwnershipQualifier::Assign);
   inst->eraseFromParent();
}

/// Construct the argument list for the assign_by_wrapper initializer or setter.
///
/// Usually this is only a single value and a single argument, but in case of
/// a tuple, the initializer/setter expect the tuple elements as separate
/// arguments. The purpose of this function is to recursively visit tuple
/// elements and add them to the argument list \p arg.
static void getAssignByWrapperArgsRecursively(SmallVectorImpl<PILValue> &args,
                                              PILValue src, unsigned &argIdx, const PILFunctionConventions &convention,
                                              PILBuilder &forProjections, PILBuilder &forCleanup) {

   PILLocation loc = (*forProjections.getInsertionPoint()).getLoc();
   PILType srcTy = src->getType();
   if (auto tupleTy = srcTy.getAs<TupleType>()) {
      // In case the source is a tuple, we have to destructure the tuple and pass
      // the tuple elements separately.
      if (srcTy.isAddress()) {
         for (unsigned idx = 0, n = tupleTy->getNumElements(); idx < n; ++idx) {
            auto *TEA = forProjections.createTupleElementAddr(loc, src, idx);
            getAssignByWrapperArgsRecursively(args, TEA, argIdx, convention,
                                              forProjections, forCleanup);
         }
      } else {
         auto *DTI = forProjections.createDestructureTuple(loc, src);
         for (PILValue elmt : DTI->getAllResults()) {
            getAssignByWrapperArgsRecursively(args, elmt, argIdx, convention,
                                              forProjections, forCleanup);
         }
      }
      return;
   }
   assert(argIdx < convention.getNumPILArguments() &&
          "initializer or setter has too few arguments");

   PILArgumentConvention argConv = convention.getPILArgumentConvention(argIdx);
   if (srcTy.isAddress() && !argConv.isIndirectConvention()) {
      // In case of a tuple where one element is loadable, but the other is
      // address only, we get the whole tuple as address.
      // For the loadable element, the argument is passed directly, but the
      // tuple element is in memory. For this case we have to insert a load.
      src = forProjections.createTrivialLoadOr(loc, src,
                                               LoadOwnershipQualifier::Take);
   }
   switch (argConv) {
      case PILArgumentConvention::Indirect_In_Guaranteed:
         forCleanup.createDestroyAddr(loc, src);
         break;
      case PILArgumentConvention::Direct_Guaranteed:
         forCleanup.createDestroyValue(loc, src);
         break;
      case PILArgumentConvention::Direct_Unowned:
      case PILArgumentConvention::Indirect_In:
      case PILArgumentConvention::Indirect_In_Constant:
      case PILArgumentConvention::Direct_Owned:
         break;
      case PILArgumentConvention::Indirect_Inout:
      case PILArgumentConvention::Indirect_InoutAliasable:
      case PILArgumentConvention::Indirect_Out:
      case PILArgumentConvention::Direct_Deallocating:
         llvm_unreachable("wrong convention for setter/initializer src argument");
   }
   args.push_back(src);
   ++argIdx;
}

static void getAssignByWrapperArgs(SmallVectorImpl<PILValue> &args,
                                   PILValue src, const PILFunctionConventions &convention,
                                   PILBuilder &forProjections, PILBuilder &forCleanup) {
   unsigned argIdx = convention.getPILArgIndexOfFirstParam();
   getAssignByWrapperArgsRecursively(args, src, argIdx, convention,
                                     forProjections, forCleanup);
   assert(argIdx == convention.getNumPILArguments() &&
          "initializer or setter has too many arguments");
}

static void lowerAssignByWrapperInstruction(PILBuilderWithScope &b,
                                            AssignByWrapperInst *inst,
                                            SmallVectorImpl<BeginAccessInst *> &accessMarkers) {
   LLVM_DEBUG(llvm::dbgs() << "  *** Lowering [isInit="
                           << unsigned(inst->getOwnershipQualifier())
                           << "]: " << *inst << "\n");

   ++numAssignRewritten;

   PILValue src = inst->getSrc();
   PILValue dest = inst->getDest();
   PILLocation loc = inst->getLoc();
   PILBuilderWithScope forCleanup(std::next(inst->getIterator()));

   switch (inst->getOwnershipQualifier()) {
      case AssignOwnershipQualifier::Init: {
         PILValue initFn = inst->getInitializer();
         CanPILFunctionType fTy = initFn->getType().castTo<PILFunctionType>();
         PILFunctionConventions convention(fTy, inst->getModule());
         SmallVector<PILValue, 4> args;
         if (convention.hasIndirectPILResults()) {
            args.push_back(dest);
            getAssignByWrapperArgs(args, src, convention, b, forCleanup);
            b.createApply(loc, initFn, SubstitutionMap(), args);
         } else {
            getAssignByWrapperArgs(args, src, convention, b, forCleanup);
            PILValue wrappedSrc = b.createApply(loc, initFn, SubstitutionMap(),
                                                args);
            b.createTrivialStoreOr(loc, wrappedSrc, dest,
                                   StoreOwnershipQualifier::Init);
         }
         // TODO: remove the unused setter function, which usually is a dead
         // partial_apply.
         break;
      }
      case AssignOwnershipQualifier::Unknown:
      case AssignOwnershipQualifier::Reassign: {
         PILValue setterFn = inst->getSetter();
         CanPILFunctionType fTy = setterFn->getType().castTo<PILFunctionType>();
         PILFunctionConventions convention(fTy, inst->getModule());
         assert(!convention.hasIndirectPILResults());
         SmallVector<PILValue, 4> args;
         getAssignByWrapperArgs(args, src, convention, b, forCleanup);
         b.createApply(loc, setterFn, SubstitutionMap(), args);

         // The destination address is not used. Remove it if it is a dead access
         // marker. This is important, because also the setter function contains
         // access marker. In case those markers are dynamic it would cause a
         // nested access violation.
         if (auto *BA = dyn_cast<BeginAccessInst>(dest))
            accessMarkers.push_back(BA);
         // TODO: remove the unused init function, which usually is a dead
         // partial_apply.
         break;
      }
      case AssignOwnershipQualifier::Reinit:
         llvm_unreachable("wrong qualifier for assign_by_wrapper");
   }
   inst->eraseFromParent();
}

static void deleteDeadAccessMarker(BeginAccessInst *BA) {
   SmallVector<PILInstruction *, 4> Users;
   for (Operand *Op : BA->getUses()) {
      PILInstruction *User = Op->getUser();
      if (!isa<EndAccessInst>(User))
         return;

      Users.push_back(User);
   }
   for (PILInstruction *User: Users) {
      User->eraseFromParent();
   }
   BA->eraseFromParent();
}

/// lowerRawPILOperations - There are a variety of raw-sil instructions like
/// 'assign' that are only used by this pass.  Now that definite initialization
/// checking is done, remove them.
static bool lowerRawPILOperations(PILFunction &fn) {
   bool changed = false;

   for (auto &bb : fn) {
      SmallVector<BeginAccessInst *, 8> accessMarkers;

      auto i = bb.begin(), e = bb.end();
      while (i != e) {
         PILInstruction *inst = &*i;
         ++i;

         // Lower 'assign' depending on initialization kind defined by definite
         // initialization.
         //
         // * Unknown is considered unprocessed and is treated as [reassign] or
         //   [init] if the destination type is trivial.
         // * Init becomes a store [init] or a store [trivial] if the destination's
         //   type is trivial.
         // * Reinit becomes a load [take], store [init], and a
         //   dealloc_partial_ref.
         // * Reassign becomes a store [assign].
         if (auto *ai = dyn_cast<AssignInst>(inst)) {
            PILBuilderWithScope b(ai);
            lowerAssignInstruction(b, ai);
            // Assign lowering may split the block. If it did,
            // reset our iteration range to the block after the insertion.
            if (b.getInsertionBB() != &bb)
               i = e;
            changed = true;
            continue;
         }

         if (auto *ai = dyn_cast<AssignByWrapperInst>(inst)) {
            PILBuilderWithScope b(ai);
            lowerAssignByWrapperInstruction(b, ai, accessMarkers);
            changed = true;
            continue;
         }

         // mark_uninitialized just becomes a noop, resolving to its operand.
         if (auto *mui = dyn_cast<MarkUninitializedInst>(inst)) {
            mui->replaceAllUsesWith(mui->getOperand());
            mui->eraseFromParent();
            changed = true;
            continue;
         }

         // mark_function_escape just gets zapped.
         if (isa<MarkFunctionEscapeInst>(inst)) {
            inst->eraseFromParent();
            changed = true;
            continue;
         }
      }
      for (BeginAccessInst *BA : accessMarkers) {
         deleteDeadAccessMarker(BA);
      }
   }
   return changed;
}

//===----------------------------------------------------------------------===//
//                            Top Level Entrypoint
//===----------------------------------------------------------------------===//

namespace {

class RawPILInstLowering : public PILFunctionTransform {
   void run() override {
      // Do not try to relower raw instructions in canonical PIL. There won't be
      // any there.
      if (getFunction()->wasDeserializedCanonical()) {
         return;
      }

      // Lower raw-sil only instructions used by this pass, like "assign".
      if (lowerRawPILOperations(*getFunction()))
         invalidateAnalysis(PILAnalysis::InvalidationKind::FunctionBody);
   }
};

} // end anonymous namespace

PILTransform *polar::createRawPILInstLowering() {
   return new RawPILInstLowering();
}
