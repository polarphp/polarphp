//===--- MemAccessUtils.cpp - Utilities for PIL memory access. ------------===//
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

#define DEBUG_TYPE "pil-access-utils"

#include "polarphp/pil/lang/MemAccessUtils.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILGlobalVariable.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILUndef.h"

using namespace polar;

AccessedStorage::AccessedStorage(PILValue base, Kind kind) {
   assert(base && "invalid storage base");
   initKind(kind);

   switch (kind) {
      case Box:
         assert(isa<AllocBoxInst>(base));
         value = base;
         break;
      case Stack:
         assert(isa<AllocStackInst>(base));
         value = base;
         break;
      case Nested:
         assert(isa<BeginAccessInst>(base));
         value = base;
         break;
      case Yield:
         assert(isa<BeginApplyResult>(base));
         value = base;
         break;
      case Unidentified:
         value = base;
         break;
      case Argument:
         value = base;
         setElementIndex(cast<PILFunctionArgument>(base)->getIndex());
         break;
      case Global:
         if (auto *GAI = dyn_cast<GlobalAddrInst>(base))
            global = GAI->getReferencedGlobal();
         else {
            FullApplySite apply(cast<ApplyInst>(base));
            auto *funcRef = apply.getReferencedFunctionOrNull();
            assert(funcRef);
            global = getVariableOfGlobalInit(funcRef);
            assert(global);
            // Require a decl for all formally accessed globals defined in this
            // module. (Access of globals defined elsewhere has Unidentified storage).
            // AccessEnforcementWMO requires this.
            assert(global->getDecl());
         }
         break;
      case Class: {
         // Do a best-effort to find the identity of the object being projected
         // from. It is OK to be unsound here (i.e. miss when two ref_element_addrs
         // actually refer the same address) because these addresses will be
         // dynamically checked, and static analysis will be sufficiently
         // conservative given that classes are not "uniquely identified".
         auto *REA = cast<RefElementAddrInst>(base);
         value = stripBorrow(REA->getOperand());
         setElementIndex(REA->getFieldNo());
      }
   }
}

const ValueDecl *AccessedStorage::getDecl() const {
   switch (getKind()) {
      case Box:
         return cast<AllocBoxInst>(value)->getLoc().getAsAstNode<VarDecl>();

      case Stack:
         return cast<AllocStackInst>(value)->getDecl();

      case Global:
         return global->getDecl();

      case Class: {
         auto *decl = getObject()->getType().getNominalOrBoundGenericNominal();
         return decl->getStoredProperties()[getPropertyIndex()];
      }
      case Argument:
         return getArgument()->getDecl();

      case Yield:
         return nullptr;

      case Nested:
         return nullptr;

      case Unidentified:
         return nullptr;
   }
   llvm_unreachable("unhandled kind");
}

const char *AccessedStorage::getKindName(AccessedStorage::Kind k) {
   switch (k) {
      case Box:
         return "Box";
      case Stack:
         return "Stack";
      case Nested:
         return "Nested";
      case Unidentified:
         return "Unidentified";
      case Argument:
         return "Argument";
      case Yield:
         return "Yield";
      case Global:
         return "Global";
      case Class:
         return "Class";
   }
   llvm_unreachable("unhandled kind");
}

void AccessedStorage::print(raw_ostream &os) const {
   os << getKindName(getKind()) << " ";
   switch (getKind()) {
      case Box:
      case Stack:
      case Nested:
      case Yield:
      case Unidentified:
         os << value;
         break;
      case Argument:
         os << value;
         break;
      case Global:
         os << *global;
         break;
      case Class:
         os << getObject();
         os << "  Field: ";
         getDecl()->print(os);
         os << " Index: " << getPropertyIndex() << "\n";
         break;
   }
}

void AccessedStorage::dump() const { print(llvm::dbgs()); }

// Return true if the given apply invokes a global addressor defined in another
// module.
bool polar::isExternalGlobalAddressor(ApplyInst *AI) {
   FullApplySite apply(AI);
   auto *funcRef = apply.getReferencedFunctionOrNull();
   if (!funcRef)
      return false;

   return funcRef->isGlobalInit() && funcRef->isExternalDeclaration();
}

// Return true if the given StructExtractInst extracts the RawPointer from
// Unsafe[Mutable]Pointer.
bool polar::isUnsafePointerExtraction(StructExtractInst *SEI) {
   assert(isa<BuiltinRawPointerType>(SEI->getType().getAstType()));
   auto &C = SEI->getModule().getAstContext();
   auto *decl = SEI->getStructDecl();
   return decl == C.getUnsafeMutablePointerDecl()
          || decl == C.getUnsafePointerDecl();
}

// Given a block argument address base, check if it is actually a box projected
// from a switch_enum. This is a valid pattern at any PIL stage resulting in a
// block-type phi. In later PIL stages, the optimizer may form address-type
// phis, causing this assert if called on those cases.
void polar::checkSwitchEnumBlockArg(PILPhiArgument *arg) {
   assert(!arg->getType().isAddress());
   PILBasicBlock *Pred = arg->getParent()->getSinglePredecessorBlock();
   if (!Pred || !isa<SwitchEnumInst>(Pred->getTerminator())) {
      arg->dump();
      llvm_unreachable("unexpected box source.");
   }
}

/// Return true if the given address value is produced by a special address
/// producer that is only used for local initialization, not formal access.
bool polar::isAddressForLocalInitOnly(PILValue sourceAddr) {
   switch (sourceAddr->getKind()) {
      default:
         return false;

         // Value to address conversions: the operand is the non-address source
         // value. These allow local mutation of the value but should never be used
         // for formal access of an lvalue.
      case ValueKind::OpenExistentialBoxInst:
      case ValueKind::ProjectExistentialBoxInst:
         return true;

         // Self-evident local initialization.
      case ValueKind::InitEnumDataAddrInst:
      case ValueKind::InitExistentialAddrInst:
      case ValueKind::AllocExistentialBoxInst:
      case ValueKind::AllocValueBufferInst:
      case ValueKind::ProjectValueBufferInst:
         return true;
   }
}

namespace {
// The result of an accessed storage query. A complete result produces an
// AccessedStorage object, which may or may not be valid. An incomplete result
// produces a PILValue representing the source address for use with deeper
// queries. The source address is not necessarily a PIL address type because
// the query can extend past pointer-to-address casts.
class AccessedStorageResult {
   AccessedStorage storage;
   PILValue address;
   bool complete;

   explicit AccessedStorageResult(PILValue address)
      : address(address), complete(false) {}

public:
   AccessedStorageResult(const AccessedStorage &storage)
      : storage(storage), complete(true) {}

   static AccessedStorageResult incomplete(PILValue address) {
      return AccessedStorageResult(address);
   }

   bool isComplete() const { return complete; }

   AccessedStorage getStorage() const { assert(complete); return storage; }

   PILValue getAddress() const { assert(!complete); return address; }
};

struct FindAccessedStorageVisitor
   : public AccessUseDefChainVisitor<FindAccessedStorageVisitor>
{
   SmallVector<PILValue, 8> addressWorklist;
   SmallPtrSet<PILPhiArgument *, 4> visitedPhis;
   Optional<AccessedStorage> storage;

public:
   FindAccessedStorageVisitor(PILValue firstSourceAddr)
      : addressWorklist({firstSourceAddr})
   {}

   AccessedStorage doIt() {
      while (!addressWorklist.empty()) {
         visit(addressWorklist.pop_back_val());
      }

      return storage.getValueOr(AccessedStorage());
   }

   void setStorage(AccessedStorage foundStorage) {
      if (!storage) {
         storage = foundStorage;
      } else {
         // `storage` may still be invalid. If both `storage` and `foundStorage`
         // are invalid, this check passes, but we return an invalid storage
         // below.
         if (!storage->hasIdenticalBase(foundStorage)) {
            storage = AccessedStorage();
            addressWorklist.clear();
         }
      }
   }

   void visitBase(PILValue base, AccessedStorage::Kind kind) {
      setStorage(AccessedStorage(base, kind));
   }

   void visitNonAccess(PILValue value)  {
      setStorage(AccessedStorage());
   }

   void visitPhi(PILPhiArgument *phiArg) {
      if (visitedPhis.insert(phiArg).second) {
         phiArg->getIncomingPhiValues(addressWorklist);
      }
   }

   void visitIncomplete(PILValue projectedAddr, PILValue parentAddr) {
      addressWorklist.push_back(parentAddr);
   }
};
} // namespace

AccessedStorage polar::findAccessedStorage(PILValue sourceAddr) {
   return FindAccessedStorageVisitor(sourceAddr).doIt();
}

AccessedStorage polar::findAccessedStorageNonNested(PILValue sourceAddr) {
   while (true) {
      const AccessedStorage &storage = findAccessedStorage(sourceAddr);
      if (!storage || storage.getKind() != AccessedStorage::Nested)
         return storage;

      sourceAddr = cast<BeginAccessInst>(storage.getValue())->getSource();
   }
}

// Return true if the given access is on a 'let' lvalue.
bool AccessedStorage::isLetAccess(PILFunction *F) const {
   if (auto *decl = dyn_cast_or_null<VarDecl>(getDecl()))
      return decl->isLet();

   // It's unclear whether a global will ever be missing it's varDecl, but
   // technically we only preserve it for debug info. So if we don't have a decl,
   // check the flag on PILGlobalVariable, which is guaranteed valid,
   if (getKind() == AccessedStorage::Global)
      return getGlobal()->isLet();

   return false;
}

static bool isScratchBuffer(PILValue value) {
   // Special case unsafe value buffer access.
   return value->getType().is<BuiltinUnsafeValueBufferType>();
}

bool polar::memInstMustInitialize(Operand *memOper) {
   PILValue address = memOper->get();
   PILInstruction *memInst = memOper->getUser();

   switch (memInst->getKind()) {
      default:
         return false;

      case PILInstructionKind::CopyAddrInst: {
         auto *CAI = cast<CopyAddrInst>(memInst);
         return CAI->getDest() == address && CAI->isInitializationOfDest();
      }
      case PILInstructionKind::InitExistentialAddrInst:
      case PILInstructionKind::InitEnumDataAddrInst:
      case PILInstructionKind::InjectEnumAddrInst:
         return true;

      case PILInstructionKind::StoreInst:
         return cast<StoreInst>(memInst)->getOwnershipQualifier()
                == StoreOwnershipQualifier::Init;

#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  case PILInstructionKind::Store##Name##Inst: \
    return cast<Store##Name##Inst>(memInst)->isInitializationOfDest();
#include "polarphp/ast/ReferenceStorageDef.h"
   }
}

bool polar::isPossibleFormalAccessBase(const AccessedStorage &storage,
                                       PILFunction *F) {
   switch (storage.getKind()) {
      case AccessedStorage::Box:
      case AccessedStorage::Stack:
         if (isScratchBuffer(storage.getValue()))
            return false;
         break;
      case AccessedStorage::Global:
         break;
      case AccessedStorage::Class:
         break;
      case AccessedStorage::Yield:
         // Yields are accessed by the caller.
         return false;
      case AccessedStorage::Argument:
         // Function arguments are accessed by the caller.
         return false;
      case AccessedStorage::Nested: {
         // A begin_access is considered a separate base for the purpose of conflict
         // checking. However, for the purpose of inserting unenforced markers and
         // performaing verification, it needs to be ignored.
         auto *BAI = cast<BeginAccessInst>(storage.getValue());
         const AccessedStorage &nestedStorage =
            findAccessedStorage(BAI->getSource());
         if (!nestedStorage)
            return false;

         return isPossibleFormalAccessBase(nestedStorage, F);
      }
      case AccessedStorage::Unidentified:
         if (isAddressForLocalInitOnly(storage.getValue()))
            return false;

         if (isa<PILPhiArgument>(storage.getValue())) {
            checkSwitchEnumBlockArg(cast<PILPhiArgument>(storage.getValue()));
            return false;
         }
         // Pointer-to-address exclusivity cannot be enforced. `baseAddress` may be
         // pointing anywhere within an object.
         if (isa<PointerToAddressInst>(storage.getValue()))
            return false;

         if (isa<PILUndef>(storage.getValue()))
            return false;

         if (isScratchBuffer(storage.getValue()))
            return false;
   }
   // Additional checks that apply to anything that may fall through.

   // Immutable values are only accessed for initialization.
   if (storage.isLetAccess(F))
      return false;

   return true;
}

/// Helper for visitApplyAccesses that visits address-type call arguments,
/// including arguments to @noescape functions that are passed as closures to
/// the current call.
static void visitApplyAccesses(ApplySite apply,
                               llvm::function_ref<void(Operand *)> visitor) {
   for (Operand &oper : apply.getArgumentOperands()) {
      // Consider any address-type operand an access. Whether it is read or modify
      // depends on the argument convention.
      if (oper.get()->getType().isAddress()) {
         visitor(&oper);
         continue;
      }
      auto fnType = oper.get()->getType().getAs<PILFunctionType>();
      if (!fnType || !fnType->isNoEscape())
         continue;

      // When @noescape function closures are passed as arguments, their
      // arguments are considered accessed at the call site.
      TinyPtrVector<PartialApplyInst *> partialApplies;
      findClosuresForFunctionValue(oper.get(), partialApplies);
      // Recursively visit @noescape function closure arguments.
      for (auto *PAI : partialApplies)
         visitApplyAccesses(PAI, visitor);
   }
}

static void visitBuiltinAddress(BuiltinInst *builtin,
                                llvm::function_ref<void(Operand *)> visitor) {
   if (auto kind = builtin->getBuiltinKind()) {
      switch (kind.getValue()) {
         default:
            builtin->dump();
            llvm_unreachable("unexpected builtin memory access.");

            // WillThrow exists for the debugger, does nothing.
         case BuiltinValueKind::WillThrow:
            return;

            // Buitins that affect memory but can't be formal accesses.
         case BuiltinValueKind::UnexpectedError:
         case BuiltinValueKind::ErrorInMain:
         case BuiltinValueKind::IsOptionalType:
         case BuiltinValueKind::AllocRaw:
         case BuiltinValueKind::DeallocRaw:
         case BuiltinValueKind::Fence:
         case BuiltinValueKind::StaticReport:
         case BuiltinValueKind::Once:
         case BuiltinValueKind::OnceWithContext:
         case BuiltinValueKind::Unreachable:
         case BuiltinValueKind::CondUnreachable:
         case BuiltinValueKind::DestroyArray:
         case BuiltinValueKind::UnsafeGuaranteed:
         case BuiltinValueKind::UnsafeGuaranteedEnd:
         case BuiltinValueKind::Swift3ImplicitObjCEntrypoint:
         case BuiltinValueKind::TSanInoutAccess:
            return;

            // General memory access to a pointer in first operand position.
         case BuiltinValueKind::CmpXChg:
         case BuiltinValueKind::AtomicLoad:
         case BuiltinValueKind::AtomicStore:
         case BuiltinValueKind::AtomicRMW:
            // Currently ignored because the access is on a RawPointer, not a
            // PIL address.
            // visitor(&builtin->getAllOperands()[0]);
            return;

            // Arrays: (T.Type, Builtin.RawPointer, Builtin.RawPointer,
            // Builtin.Word)
         case BuiltinValueKind::CopyArray:
         case BuiltinValueKind::TakeArrayNoAlias:
         case BuiltinValueKind::TakeArrayFrontToBack:
         case BuiltinValueKind::TakeArrayBackToFront:
         case BuiltinValueKind::AssignCopyArrayNoAlias:
         case BuiltinValueKind::AssignCopyArrayFrontToBack:
         case BuiltinValueKind::AssignCopyArrayBackToFront:
         case BuiltinValueKind::AssignTakeArray:
            // Currently ignored because the access is on a RawPointer.
            // visitor(&builtin->getAllOperands()[1]);
            // visitor(&builtin->getAllOperands()[2]);
            return;
      }
   }
   if (auto ID = builtin->getIntrinsicID()) {
      switch (ID.getValue()) {
         // Exhaustively verifying all LLVM instrinsics that access memory is
         // impractical. Instead, we call out the few common cases and return in
         // the default case.
         default:
            return;
         case llvm::Intrinsic::memcpy:
         case llvm::Intrinsic::memmove:
            // Currently ignored because the access is on a RawPointer.
            // visitor(&builtin->getAllOperands()[0]);
            // visitor(&builtin->getAllOperands()[1]);
            return;
         case llvm::Intrinsic::memset:
            // Currently ignored because the access is on a RawPointer.
            // visitor(&builtin->getAllOperands()[0]);
            return;
      }
   }
   llvm_unreachable("Must be either a builtin or intrinsic.");
}

void polar::visitAccessedAddress(PILInstruction *I,
                                 llvm::function_ref<void(Operand *)> visitor) {
   assert(I->mayReadOrWriteMemory());

   // Reference counting instructions do not access user visible memory.
   if (isa<RefCountingInst>(I))
      return;

   if (isa<DeallocationInst>(I))
      return;

   if (auto apply = FullApplySite::isa(I)) {
      visitApplyAccesses(apply, visitor);
      return;
   }

   if (auto builtin = dyn_cast<BuiltinInst>(I)) {
      visitBuiltinAddress(builtin, visitor);
      return;
   }

   switch (I->getKind()) {
      default:
         I->dump();
         llvm_unreachable("unexpected memory access.");

      case PILInstructionKind::AssignInst:
      case PILInstructionKind::AssignByWrapperInst:
         visitor(&I->getAllOperands()[AssignInst::Dest]);
         return;

      case PILInstructionKind::CheckedCastAddrBranchInst:
         visitor(&I->getAllOperands()[CheckedCastAddrBranchInst::Src]);
         visitor(&I->getAllOperands()[CheckedCastAddrBranchInst::Dest]);
         return;

      case PILInstructionKind::CopyAddrInst:
         visitor(&I->getAllOperands()[CopyAddrInst::Src]);
         visitor(&I->getAllOperands()[CopyAddrInst::Dest]);
         return;

#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  case PILInstructionKind::Store##Name##Inst:
#include "polarphp/ast/ReferenceStorageDef.h"
      case PILInstructionKind::StoreInst:
      case PILInstructionKind::StoreBorrowInst:
         visitor(&I->getAllOperands()[StoreInst::Dest]);
         return;

      case PILInstructionKind::SelectEnumAddrInst:
         visitor(&I->getAllOperands()[0]);
         return;

#define NEVER_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  case PILInstructionKind::Load##Name##Inst:
#include "polarphp/ast/ReferenceStorageDef.h"
      case PILInstructionKind::InitExistentialAddrInst:
      case PILInstructionKind::InjectEnumAddrInst:
      case PILInstructionKind::LoadInst:
      case PILInstructionKind::LoadBorrowInst:
      case PILInstructionKind::OpenExistentialAddrInst:
      case PILInstructionKind::SwitchEnumAddrInst:
      case PILInstructionKind::UncheckedTakeEnumDataAddrInst:
      case PILInstructionKind::UnconditionalCheckedCastInst: {
         // Assuming all the above have only a single address operand.
         assert(I->getNumOperands() - I->getNumTypeDependentOperands() == 1);
         Operand *singleOperand = &I->getAllOperands()[0];
         // Check the operand type because UnconditionalCheckedCastInst may operate
         // on a non-address.
         if (singleOperand->get()->getType().isAddress())
            visitor(singleOperand);
         return;
      }
         // Non-access cases: these are marked with memory side effects, but, by
         // themselves, do not access formal memory.
#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#define ALWAYS_OR_SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...)            \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#include "polarphp/ast/ReferenceStorageDef.h"
      case PILInstructionKind::AbortApplyInst:
      case PILInstructionKind::AllocBoxInst:
      case PILInstructionKind::AllocExistentialBoxInst:
      case PILInstructionKind::AllocGlobalInst:
      case PILInstructionKind::BeginAccessInst:
      case PILInstructionKind::BeginApplyInst:
      case PILInstructionKind::BeginBorrowInst:
      case PILInstructionKind::BeginUnpairedAccessInst:
      case PILInstructionKind::BindMemoryInst:
      case PILInstructionKind::CheckedCastValueBranchInst:
      case PILInstructionKind::CondFailInst:
      case PILInstructionKind::CopyBlockInst:
      case PILInstructionKind::CopyBlockWithoutEscapingInst:
      case PILInstructionKind::CopyValueInst:
      case PILInstructionKind::DeinitExistentialAddrInst:
      case PILInstructionKind::DeinitExistentialValueInst:
      case PILInstructionKind::DestroyAddrInst:
      case PILInstructionKind::DestroyValueInst:
      case PILInstructionKind::EndAccessInst:
      case PILInstructionKind::EndApplyInst:
      case PILInstructionKind::EndBorrowInst:
      case PILInstructionKind::EndUnpairedAccessInst:
      case PILInstructionKind::EndLifetimeInst:
      case PILInstructionKind::ExistentialMetatypeInst:
      case PILInstructionKind::FixLifetimeInst:
      case PILInstructionKind::InitExistentialValueInst:
      case PILInstructionKind::IsUniqueInst:
      case PILInstructionKind::IsEscapingClosureInst:
      case PILInstructionKind::KeyPathInst:
      case PILInstructionKind::OpenExistentialBoxInst:
      case PILInstructionKind::OpenExistentialBoxValueInst:
      case PILInstructionKind::OpenExistentialValueInst:
      case PILInstructionKind::PartialApplyInst:
      case PILInstructionKind::ProjectValueBufferInst:
      case PILInstructionKind::YieldInst:
      case PILInstructionKind::UnwindInst:
      case PILInstructionKind::UncheckedOwnershipConversionInst:
      case PILInstructionKind::UncheckedRefCastAddrInst:
      case PILInstructionKind::UnconditionalCheckedCastAddrInst:
      case PILInstructionKind::UnconditionalCheckedCastValueInst:
      case PILInstructionKind::ValueMetatypeInst:
         return;
   }
}

PILBasicBlock::iterator polar::removeBeginAccess(BeginAccessInst *beginAccess) {
   while (!beginAccess->use_empty()) {
      Operand *op = *beginAccess->use_begin();

      // Delete any associated end_access instructions.
      if (auto endAccess = dyn_cast<EndAccessInst>(op->getUser())) {
         endAccess->eraseFromParent();

         // Forward all other uses to the original address.
      } else {
         op->set(beginAccess->getSource());
      }
   }
   return beginAccess->getParent()->erase(beginAccess);
}
