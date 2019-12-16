//===--- InstructionUtils.cpp - Utilities for PIL instructions ------------===//
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

#define DEBUG_TYPE "pil-inst-utils"

#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/basic/NullablePtr.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/pil/lang/DebugUtils.h"
#include "polarphp/pil/lang/Projection.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILVisitor.h"

using namespace polar;

PILValue polar::stripOwnershipInsts(PILValue v) {
   while (true) {
      switch (v->getKind()) {
         default:
            return v;
         case ValueKind::CopyValueInst:
         case ValueKind::BeginBorrowInst:
            v = cast<SingleValueInstruction>(v)->getOperand(0);
      }
   }
}

/// Strip off casts/indexing insts/address projections from V until there is
/// nothing left to strip.
///
/// FIXME: Why don't we strip projections after stripping indexes?
PILValue polar::getUnderlyingObject(PILValue v) {
   while (true) {
      PILValue v2 = stripCasts(v);
      v2 = stripAddressProjections(v2);
      v2 = stripIndexingInsts(v2);
      v2 = stripOwnershipInsts(v2);
      if (v2 == v)
         return v2;
      v = v2;
   }
}

/// Strip off casts and address projections into the interior of a value. Unlike
/// getUnderlyingObject, this does not find the root of a heap object--a class
/// property is itself an address root.
PILValue polar::getUnderlyingAddressRoot(PILValue v) {
   while (true) {
      PILValue v2 = stripIndexingInsts(stripCasts(v));
      v2 = stripOwnershipInsts(v2);
      switch (v2->getKind()) {
         case ValueKind::StructElementAddrInst:
         case ValueKind::TupleElementAddrInst:
         case ValueKind::UncheckedTakeEnumDataAddrInst:
            v2 = cast<SingleValueInstruction>(v2)->getOperand(0);
            break;
         default:
            break;
      }
      if (v2 == v)
         return v2;
      v = v2;
   }
}

PILValue polar::getUnderlyingObjectStopAtMarkDependence(PILValue v) {
   while (true) {
      PILValue v2 = stripCastsWithoutMarkDependence(v);
      v2 = stripAddressProjections(v2);
      v2 = stripIndexingInsts(v2);
      v2 = stripOwnershipInsts(v2);
      if (v2 == v)
         return v2;
      v = v2;
   }
}

static bool isRCIdentityPreservingCast(ValueKind Kind) {
   switch (Kind) {
      case ValueKind::UpcastInst:
      case ValueKind::UncheckedRefCastInst:
      case ValueKind::UnconditionalCheckedCastInst:
      case ValueKind::UnconditionalCheckedCastValueInst:
      case ValueKind::RefToBridgeObjectInst:
      case ValueKind::BridgeObjectToRefInst:
         return true;
      default:
         return false;
   }
}

/// Return the underlying PILValue after stripping off identity PILArguments if
/// we belong to a BB with one predecessor.
PILValue polar::stripSinglePredecessorArgs(PILValue V) {
   while (true) {
      auto *A = dyn_cast<PILArgument>(V);
      if (!A)
         return V;

      PILBasicBlock *BB = A->getParent();

      // First try and grab the single predecessor of our parent BB. If we don't
      // have one, bail.
      PILBasicBlock *Pred = BB->getSinglePredecessorBlock();
      if (!Pred)
         return V;

      // Then grab the terminator of Pred...
      TermInst *PredTI = Pred->getTerminator();

      // And attempt to find our matching argument.
      //
      // *NOTE* We can only strip things here if we know that there is no semantic
      // change in terms of upcasts/downcasts/enum extraction since this is used
      // by other routines here. This means that we can only look through
      // cond_br/br.
      //
      // For instance, routines that use stripUpcasts() do not want to strip off a
      // downcast that results from checked_cast_br.
      if (auto *BI = dyn_cast<BranchInst>(PredTI)) {
         V = BI->getArg(A->getIndex());
         continue;
      }

      if (auto *CBI = dyn_cast<CondBranchInst>(PredTI)) {
         if (PILValue Arg = CBI->getArgForDestBB(BB, A)) {
            V = Arg;
            continue;
         }
      }

      return V;
   }
}

PILValue polar::stripCastsWithoutMarkDependence(PILValue V) {
   while (true) {
      V = stripSinglePredecessorArgs(V);

      auto K = V->getKind();
      if (isRCIdentityPreservingCast(K) ||
          K == ValueKind::UncheckedTrivialBitCastInst) {
         V = cast<SingleValueInstruction>(V)->getOperand(0);
         continue;
      }

      return V;
   }
}

PILValue polar::stripCasts(PILValue v) {
   while (true) {
      v = stripSinglePredecessorArgs(v);

      auto k = v->getKind();
      if (isRCIdentityPreservingCast(k)
          || k == ValueKind::UncheckedTrivialBitCastInst
          || k == ValueKind::MarkDependenceInst) {
         v = cast<SingleValueInstruction>(v)->getOperand(0);
         continue;
      }

      PILValue v2 = stripOwnershipInsts(v);
      if (v2 != v) {
         v = v2;
         continue;
      }

      return v;
   }
}

PILValue polar::stripUpCasts(PILValue v) {
   assert(v->getType().isClassOrClassMetatype() &&
          "Expected class or class metatype!");

   v = stripSinglePredecessorArgs(v);

   while (true) {
      if (auto *ui = dyn_cast<UpcastInst>(v)) {
         v = ui->getOperand();
         continue;
      }

      PILValue v2 = stripSinglePredecessorArgs(v);
      v2 = stripOwnershipInsts(v2);
      if (v2 == v) {
         return v2;
      }
      v = v2;
   }
}

PILValue polar::stripClassCasts(PILValue v) {
   while (true) {
      if (auto *ui = dyn_cast<UpcastInst>(v)) {
         v = ui->getOperand();
         continue;
      }

      if (auto *ucci = dyn_cast<UnconditionalCheckedCastInst>(v)) {
         v = ucci->getOperand();
         continue;
      }

      PILValue v2 = stripOwnershipInsts(v);
      if (v2 != v) {
         v = v2;
         continue;
      }

      return v;
   }
}

PILValue polar::stripAddressAccess(PILValue V) {
   while (true) {
      switch (V->getKind()) {
         default:
            return V;
         case ValueKind::BeginBorrowInst:
         case ValueKind::BeginAccessInst:
            V = cast<SingleValueInstruction>(V)->getOperand(0);
      }
   }
}

PILValue polar::stripAddressProjections(PILValue V) {
   while (true) {
      V = stripSinglePredecessorArgs(V);
      if (!Projection::isAddressProjection(V))
         return V;
      V = cast<SingleValueInstruction>(V)->getOperand(0);
   }
}

PILValue polar::stripValueProjections(PILValue V) {
   while (true) {
      V = stripSinglePredecessorArgs(V);
      if (!Projection::isObjectProjection(V))
         return V;
      V = cast<SingleValueInstruction>(V)->getOperand(0);
   }
}

PILValue polar::stripIndexingInsts(PILValue V) {
   while (true) {
      if (!isa<IndexingInst>(V))
         return V;
      V = cast<IndexingInst>(V)->getBase();
   }
}

PILValue polar::stripExpectIntrinsic(PILValue V) {
   auto *BI = dyn_cast<BuiltinInst>(V);
   if (!BI)
      return V;
   if (BI->getIntrinsicInfo().ID != llvm::Intrinsic::expect)
      return V;
   return BI->getArguments()[0];
}

PILValue polar::stripBorrow(PILValue V) {
   if (auto *BBI = dyn_cast<BeginBorrowInst>(V))
      return BBI->getOperand();
   return V;
}

// All instructions handled here must propagate their first operand into their
// single result.
//
// This is guaranteed to handle all function-type converstions: ThinToThick,
// ConvertFunction, and ConvertEscapeToNoEscapeInst.
SingleValueInstruction *polar::getSingleValueCopyOrCast(PILInstruction *I) {
   if (auto *convert = dyn_cast<ConversionInst>(I))
      return convert;

   switch (I->getKind()) {
      default:
         return nullptr;
      case PILInstructionKind::CopyValueInst:
      case PILInstructionKind::CopyBlockInst:
      case PILInstructionKind::CopyBlockWithoutEscapingInst:
      case PILInstructionKind::BeginBorrowInst:
      case PILInstructionKind::BeginAccessInst:
      case PILInstructionKind::MarkDependenceInst:
         return cast<SingleValueInstruction>(I);
   }
}

// Does this instruction terminate a PIL-level scope?
bool polar::isEndOfScopeMarker(PILInstruction *user) {
   switch (user->getKind()) {
      default:
         return false;
      case PILInstructionKind::EndAccessInst:
      case PILInstructionKind::EndBorrowInst:
      case PILInstructionKind::EndLifetimeInst:
         return true;
   }
}

bool polar::isIncidentalUse(PILInstruction *user) {
   return isEndOfScopeMarker(user) || user->isDebugInstruction() ||
          isa<FixLifetimeInst>(user);
}

bool polar::onlyAffectsRefCount(PILInstruction *user) {
   switch (user->getKind()) {
      default:
         return false;
      case PILInstructionKind::AutoreleaseValueInst:
      case PILInstructionKind::DestroyValueInst:
      case PILInstructionKind::ReleaseValueInst:
      case PILInstructionKind::RetainValueInst:
      case PILInstructionKind::StrongReleaseInst:
      case PILInstructionKind::StrongRetainInst:
      case PILInstructionKind::UnmanagedAutoreleaseValueInst:
#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  case PILInstructionKind::Name##RetainValueInst:                              \
  case PILInstructionKind::Name##ReleaseValueInst:                             \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  case PILInstructionKind::Name##RetainInst:                                   \
  case PILInstructionKind::Name##ReleaseInst:                                  \
  case PILInstructionKind::StrongRetain##Name##Inst:                           \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#include "polarphp/ast/ReferenceStorageDef.h"
         return true;
   }
}

bool polar::mayCheckRefCount(PILInstruction *User) {
   return isa<IsUniqueInst>(User) || isa<IsEscapingClosureInst>(User);
}

bool polar::isSanitizerInstrumentation(PILInstruction *Instruction) {
   auto *BI = dyn_cast<BuiltinInst>(Instruction);
   if (!BI)
      return false;

   Identifier Name = BI->getName();
   if (Name == BI->getModule().getAstContext().getIdentifier("tsanInoutAccess"))
      return true;

   return false;
}

PILValue polar::isPartialApplyOfReabstractionThunk(PartialApplyInst *PAI) {
   // A partial_apply of a reabstraction thunk either has a single capture
   // (a function) or two captures (function and dynamic Self type).
   if (PAI->getNumArguments() != 1 &&
       PAI->getNumArguments() != 2)
      return PILValue();

   auto *Fun = PAI->getReferencedFunctionOrNull();
   if (!Fun)
      return PILValue();

   // Make sure we have a reabstraction thunk.
   if (Fun->isThunk() != IsReabstractionThunk)
      return PILValue();

   // The argument should be a closure.
   auto Arg = PAI->getArgument(0);
   if (!Arg->getType().is<PILFunctionType>() ||
       (!Arg->getType().isReferenceCounted(PAI->getFunction()->getModule()) &&
        Arg->getType().getAs<PILFunctionType>()->getRepresentation() !=
        PILFunctionType::Representation::Thick))
      return PILValue();

   return Arg;
}

bool polar::onlyUsedByAssignByWrapper(PartialApplyInst *PAI) {
   bool usedByAssignByWrapper = false;
   for (Operand *Op : PAI->getUses()) {
      PILInstruction *User = Op->getUser();
      if (isa<AssignByWrapperInst>(User) && Op->getOperandNumber() >= 2) {
         usedByAssignByWrapper = true;
         continue;
      }
      if (isa<DestroyValueInst>(User))
         continue;
      return false;
   }
   return usedByAssignByWrapper;
}

/// Given a block used as a noescape function argument, attempt to find all
/// Swift closures that invoking the block will call. The StoredClosures may not
/// actually be partial_apply instructions. They may be copied, block arguments,
/// or conversions. The caller must continue searching up the use-def chain.
static PILValue findClosureStoredIntoBlock(PILValue V) {

   auto FnType = V->getType().castTo<PILFunctionType>();
   assert(FnType->getRepresentation() == PILFunctionTypeRepresentation::Block);
   (void)FnType;

   // Given a no escape block argument to a function,
   // pattern match to find the noescape closure that invoking the block
   // will call:
   //     %noescape_closure = ...
   //     %wae_Thunk = function_ref @$withoutActuallyEscapingThunk
   //     %sentinel =
   //       partial_apply [callee_guaranteed] %wae_thunk(%noescape_closure)
   //     %noescaped_wrapped = mark_dependence %sentinel on %noescape_closure
   //     %storage = alloc_stack
   //     %storage_address = project_block_storage %storage
   //     store %noescaped_wrapped to [init] %storage_address
   //     %block = init_block_storage_header %storage invoke %thunk
   //     %arg = copy_block %block

   InitBlockStorageHeaderInst *IBSHI = dyn_cast<InitBlockStorageHeaderInst>(V);
   if (!IBSHI)
      return nullptr;

   PILValue BlockStorage = IBSHI->getBlockStorage();
   auto *PBSI = BlockStorage->getSingleUserOfType<ProjectBlockStorageInst>();
   assert(PBSI && "Couldn't find block storage projection");

   auto *SI = PBSI->getSingleUserOfType<StoreInst>();
   assert(SI && "Couldn't find single store of function into block storage");

   auto *CV = dyn_cast<CopyValueInst>(SI->getSrc());
   if (!CV)
      return nullptr;
   auto *WrappedNoEscape = dyn_cast<MarkDependenceInst>(CV->getOperand());
   if (!WrappedNoEscape)
      return nullptr;
   auto Sentinel = dyn_cast<PartialApplyInst>(WrappedNoEscape->getValue());
   if (!Sentinel)
      return nullptr;
   auto NoEscapeClosure = isPartialApplyOfReabstractionThunk(Sentinel);
   if (WrappedNoEscape->getBase() != NoEscapeClosure)
      return nullptr;

   // This is the value of the closure to be invoked. To find the partial_apply
   // itself, the caller must search the use-def chain.
   return NoEscapeClosure;
}

/// Find all closures that may be propagated into the given function-type value.
///
/// Searches the use-def chain from the given value upward until a partial_apply
/// is reached. Populates `results` with the set of partial_apply instructions.
///
/// `funcVal` may be either a function type or an Optional function type. This
/// might be called on a directly applied value or on a call argument, which may
/// in turn be applied within the callee.
void polar::findClosuresForFunctionValue(
   PILValue funcVal, TinyPtrVector<PartialApplyInst *> &results) {

   PILType funcTy = funcVal->getType();
   // Handle `Optional<@convention(block) @noescape (_)->(_)>`
   if (auto optionalObjTy = funcTy.getOptionalObjectType())
      funcTy = optionalObjTy;
   assert(funcTy.is<PILFunctionType>());

   SmallVector<PILValue, 4> worklist;
   // Avoid exponential path exploration and prevent duplicate results.
   llvm::SmallDenseSet<PILValue, 8> visited;
   auto worklistInsert = [&](PILValue V) {
      if (visited.insert(V).second)
         worklist.push_back(V);
   };
   worklistInsert(funcVal);

   while (!worklist.empty()) {
      PILValue V = worklist.pop_back_val();

      if (auto *I = V->getDefiningInstruction()) {
         // Look through copies, borrows, and conversions.
         //
         // Handle copy_block and copy_block_without_actually_escaping before
         // calling findClosureStoredIntoBlock.
         if (SingleValueInstruction *SVI = getSingleValueCopyOrCast(I)) {
            worklistInsert(SVI->getOperand(0));
            continue;
         }
      }
      // Look through Optionals.
      if (V->getType().getOptionalObjectType()) {
         auto *EI = dyn_cast<EnumInst>(V);
         if (EI && EI->hasOperand()) {
            worklistInsert(EI->getOperand());
         }
         // Ignore the .None case.
         continue;
      }
      // Look through Phis.
      //
      // This should be done before calling findClosureStoredIntoBlock.
      if (auto *arg = dyn_cast<PILPhiArgument>(V)) {
         SmallVector<std::pair<PILBasicBlock *, PILValue>, 2> blockArgs;
         arg->getIncomingPhiValues(blockArgs);
         for (auto &blockAndArg : blockArgs)
            worklistInsert(blockAndArg.second);

         continue;
      }
      // Look through ObjC closures.
      auto fnType = V->getType().getAs<PILFunctionType>();
      if (fnType
          && fnType->getRepresentation() == PILFunctionTypeRepresentation::Block) {
         if (PILValue storedClosure = findClosureStoredIntoBlock(V))
            worklistInsert(storedClosure);

         continue;
      }
      if (auto *PAI = dyn_cast<PartialApplyInst>(V)) {
         PILValue thunkArg = isPartialApplyOfReabstractionThunk(PAI);
         if (thunkArg) {
            // Handle reabstraction thunks recursively. This may reabstract over
            // @convention(block).
            worklistInsert(thunkArg);
            continue;
         }
         results.push_back(PAI);
         continue;
      }
      // Ignore other unrecognized values that feed this applied argument.
   }
}

bool PolymorphicBuiltinSpecializedOverloadInfo::init(
   PILFunction *fn, BuiltinValueKind builtinKind,
   ArrayRef<PILType> oldOperandTypes, PILType oldResultType) {
   assert(!isInitialized && "Expected uninitialized info");
   POLAR_DEFER { isInitialized = true; };
   if (!isPolymorphicBuiltin(builtinKind))
      return false;

   // Ok, at this point we know that we have a true polymorphic builtin. See if
   // we have an overload for its current operand type.
   StringRef name = getBuiltinName(builtinKind);
   StringRef prefix = "generic_";
   assert(name.startswith(prefix) &&
          "Invalid polymorphic builtin name! Prefix should be Generic$OP?!");
   SmallString<32> staticOverloadName;
   staticOverloadName.append(name.drop_front(prefix.size()));

   // If our first argument is an address, we know we have an indirect @out
   // parameter by convention since all of these polymorphic builtins today never
   // take indirect parameters without an indirect out result parameter. We stash
   // this information and validate that if we have an out param, that our result
   // is equal to the empty tuple type.
   if (oldOperandTypes[0].isAddress()) {
      if (oldResultType != fn->getModule().Types.getEmptyTupleType())
         return false;

      hasOutParam = true;
      PILType firstType = oldOperandTypes.front();

      // We only handle polymorphic builtins with trivial types today.
      if (!firstType.is<BuiltinType>() || !firstType.isTrivial(*fn)) {
         return false;
      }

      resultType = firstType.getObjectType();
      oldOperandTypes = oldOperandTypes.drop_front();
   } else {
      resultType = oldResultType;
   }

   // Then go through all of our values and bail if any after substitution are
   // not concrete builtin types. Otherwise, stash each of them in the argTypes
   // array as objects. We will convert them as appropriate.
   for (PILType ty : oldOperandTypes) {
      // If after specialization, we do not have a trivial builtin type, bail.
      if (!ty.is<BuiltinType>() || !ty.isTrivial(*fn)) {
         return false;
      }

      // Otherwise, we have an object builtin type ready to go.
      argTypes.push_back(ty.getObjectType());
   }

   // Ok, we have all builtin types. Infer the underlying polymorphic builtin
   // name form our first argument.
   CanBuiltinType builtinType = argTypes.front().getAs<BuiltinType>();
   SmallString<32> builtinTypeNameStorage;
   StringRef typeName = builtinType->getTypeName(builtinTypeNameStorage, false);
   staticOverloadName.append("_");
   staticOverloadName.append(typeName);

   auto &ctx = fn->getAstContext();
   staticOverloadIdentifier = ctx.getIdentifier(staticOverloadName);

   // Ok, we have our overload identifier. Grab the builtin info from the
   // cache. If we did not actually found a valid builtin value kind for our
   // overload, then we do not have a static overload for the passed in types, so
   // return false.
   builtinInfo = &fn->getModule().getBuiltinInfo(staticOverloadIdentifier);
   return true;
}

bool PolymorphicBuiltinSpecializedOverloadInfo::init(BuiltinInst *bi) {
   assert(!isInitialized && "Can not init twice?!");
   POLAR_DEFER { isInitialized = true; };

   // First quickly make sure we have a /real/ BuiltinValueKind, not an intrinsic
   // or None.
   auto kind = bi->getBuiltinKind();
   if (!kind)
      return false;

   SmallVector<PILType, 8> oldOperandTypes;
   copy(bi->getOperandTypes(), std::back_inserter(oldOperandTypes));
   assert(bi->getNumResults() == 1 &&
          "We expect a tuple here instead of real args");
   PILType oldResultType = bi->getResult(0)->getType();
   return init(bi->getFunction(), *kind, oldOperandTypes, oldResultType);
}

PILValue
polar::getStaticOverloadForSpecializedPolymorphicBuiltin(BuiltinInst *bi) {

   PolymorphicBuiltinSpecializedOverloadInfo info;
   if (!info.init(bi))
      return PILValue();

   SmallVector<PILValue, 8> rawArgsData;
   copy(bi->getOperandValues(), std::back_inserter(rawArgsData));

   PILValue result = bi->getResult(0);
   MutableArrayRef<PILValue> rawArgs = rawArgsData;

   if (info.hasOutParam) {
      result = rawArgs.front();
      rawArgs = rawArgs.drop_front();
   }

   assert(bi->getNumResults() == 1 &&
          "We assume that builtins have a single result today. If/when this "
          "changes, this code needs to be updated");

   PILBuilderWithScope builder(bi);

   // Ok, now we know that we can convert this to our specialized
   // builtin. Prepare the arguments for the specialized value, loading the
   // values if needed and storing the result into an out parameter if needed.
   //
   // NOTE: We only support polymorphic builtins with trivial types today, so we
   // use load/store trivial as a result.
   SmallVector<PILValue, 8> newArgs;
   for (PILValue arg : rawArgs) {
      if (arg->getType().isObject()) {
         newArgs.push_back(arg);
         continue;
      }

      PILValue load = builder.emitLoadValueOperation(
         bi->getLoc(), arg, LoadOwnershipQualifier::Trivial);
      newArgs.push_back(load);
   }

   BuiltinInst *newBI =
      builder.createBuiltin(bi->getLoc(), info.staticOverloadIdentifier,
                            info.resultType, {}, newArgs);

   // If we have an out parameter initialize it now.
   if (info.hasOutParam) {
      builder.emitStoreValueOperation(newBI->getLoc(), newBI->getResult(0),
                                      result, StoreOwnershipQualifier::Trivial);
   }

   return newBI;
}
