//===--- PILGlobalVariable.cpp - Defines PILGlobalVariable structure ------===//
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

#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILGlobalVariable.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILLinkage.h"
#include "polarphp/pil/lang/PILModule.h"

using namespace polar;

PILGlobalVariable *PILGlobalVariable::create(PILModule &M, PILLinkage linkage,
                                             IsSerialized_t isSerialized,
                                             StringRef name,
                                             PILType loweredType,
                                             Optional<PILLocation> loc,
                                             VarDecl *Decl) {
   // Get a StringMapEntry for the variable.  As a sop to error cases,
   // allow the name to have an empty string.
   llvm::StringMapEntry<PILGlobalVariable*> *entry = nullptr;
   if (!name.empty()) {
      entry = &*M.GlobalVariableMap.insert(std::make_pair(name, nullptr)).first;
      assert(!entry->getValue() && "global variable already exists");
      name = entry->getKey();
   }

   auto var = new (M) PILGlobalVariable(M, linkage, isSerialized, name,
                                        loweredType, loc, Decl);

   if (entry) entry->setValue(var);
   return var;
}


PILGlobalVariable::PILGlobalVariable(PILModule &Module, PILLinkage Linkage,
                                     IsSerialized_t isSerialized,
                                     StringRef Name, PILType LoweredType,
                                     Optional<PILLocation> Loc, VarDecl *Decl)
   : Module(Module),
     Name(Name),
     LoweredType(LoweredType),
     Location(Loc),
     Linkage(unsigned(Linkage)),
     VDecl(Decl) {
   setSerialized(isSerialized);
   IsDeclaration = is_available_externally(Linkage);
   setLet(Decl ? Decl->isLet() : false);
   Module.silGlobals.push_back(this);
}

PILGlobalVariable::~PILGlobalVariable() {
   getModule().GlobalVariableMap.erase(Name);
}

/// Get this global variable's fragile attribute.
IsSerialized_t PILGlobalVariable::isSerialized() const {
   return Serialized ? IsSerialized : IsNotSerialized;
}
void PILGlobalVariable::setSerialized(IsSerialized_t isSerialized) {
   assert(isSerialized != IsSerializable);
   Serialized = isSerialized ? 1 : 0;
}

/// Return the value that is written into the global variable.
PILInstruction *PILGlobalVariable::getStaticInitializerValue() {
   if (StaticInitializerBlock.empty())
      return nullptr;

   return &StaticInitializerBlock.back();
}

BuiltinInst *PILGlobalVariable::getOffsetSubtract(const TupleExtractInst *TE,
                                                  PILModule &M) {

   // Match the pattern:
   // tuple_extract(usub_with_overflow(x, integer_literal, integer_literal 0), 0)

   if (TE->getFieldNo() != 0)
      return nullptr;

   auto *BI = dyn_cast<BuiltinInst>(TE->getOperand());
   if (!BI)
      return nullptr;
   if (M.getBuiltinInfo(BI->getName()).ID != BuiltinValueKind::USubOver)
      return nullptr;

   if (!isa<IntegerLiteralInst>(BI->getArguments()[1]))
      return nullptr;

   auto *overflowFlag = dyn_cast<IntegerLiteralInst>(BI->getArguments()[2]);
   if (!overflowFlag || !overflowFlag->getValue().isNullValue())
      return nullptr;

   return BI;
}

bool PILGlobalVariable::isValidStaticInitializerInst(const PILInstruction *I,
                                                     PILModule &M) {
   switch (I->getKind()) {
      case PILInstructionKind::BuiltinInst: {
         auto *bi = cast<BuiltinInst>(I);
         switch (M.getBuiltinInfo(bi->getName()).ID) {
            case BuiltinValueKind::PtrToInt:
               if (isa<LiteralInst>(bi->getArguments()[0]))
                  return true;
               break;
            case BuiltinValueKind::StringObjectOr:
               // The first operand can be a string literal (i.e. a pointer), but the
               // second operand must be a constant. This enables creating a
               // a pointer+offset relocation.
               // Note that StringObjectOr requires the or'd bits in the first
               // operand to be 0, so the operation is equivalent to an addition.
               if (isa<IntegerLiteralInst>(bi->getArguments()[1]))
                  return true;
               break;
            case BuiltinValueKind::ZExtOrBitCast:
               return true;
            case BuiltinValueKind::USubOver: {
               // Handle StringObjectOr(tuple_extract(usub_with_overflow(x, offset)), bits)
               // This pattern appears in UTF8 String literal construction.
               auto *TE = bi->getSingleUserOfType<TupleExtractInst>();
               return TE && getOffsetSubtract(TE, M);
            }
            default:
               break;
         }
         return false;
      }
      case PILInstructionKind::TupleExtractInst: {
         // Handle StringObjectOr(tuple_extract(usub_with_overflow(x, offset)), bits)
         // This pattern appears in UTF8 String literal construction.
         auto *TE = cast<TupleExtractInst>(I);
         if (!getOffsetSubtract(TE, M))
            return false;
         auto *BI = TE->getSingleUserOfType<BuiltinInst>();
         return BI &&
                M.getBuiltinInfo(BI->getName()).ID == BuiltinValueKind::StringObjectOr;
      }
      case PILInstructionKind::StringLiteralInst:
         switch (cast<StringLiteralInst>(I)->getEncoding()) {
            case StringLiteralInst::Encoding::Bytes:
            case StringLiteralInst::Encoding::UTF8:
            case StringLiteralInst::Encoding::UTF16:
               return true;
            case StringLiteralInst::Encoding::ObjCSelector:
               // Objective-C selector string literals cannot be used in static
               // initializers.
               return false;
         }
         return false;
      case PILInstructionKind::StructInst:
      case PILInstructionKind::TupleInst:
      case PILInstructionKind::IntegerLiteralInst:
      case PILInstructionKind::FloatLiteralInst:
      case PILInstructionKind::ObjectInst:
      case PILInstructionKind::ValueToBridgeObjectInst:
         return true;
      default:
         return false;
   }
}

/// Return whether this variable corresponds to a Clang node.
bool PILGlobalVariable::hasClangNode() const {
   return (VDecl ? VDecl->hasClangNode() : false);
}

/// Return the Clang node associated with this variable if it has one.
ClangNode PILGlobalVariable::getClangNode() const {
   return (VDecl ? VDecl->getClangNode() : ClangNode());
}
const clang::Decl *PILGlobalVariable::getClangDecl() const {
   return (VDecl ? VDecl->getClangDecl() : nullptr);
}

//===----------------------------------------------------------------------===//
// Utilities for verification and optimization.
//===----------------------------------------------------------------------===//

static PILGlobalVariable *getStaticallyInitializedVariable(PILFunction *AddrF) {
   if (AddrF->isExternalDeclaration())
      return nullptr;

   auto *RetInst = cast<ReturnInst>(AddrF->findReturnBB()->getTerminator());
   auto *API = dyn_cast<AddressToPointerInst>(RetInst->getOperand());
   if (!API)
      return nullptr;
   auto *GAI = dyn_cast<GlobalAddrInst>(API->getOperand());
   if (!GAI)
      return nullptr;

   return GAI->getReferencedGlobal();
}

PILGlobalVariable *polar::getVariableOfGlobalInit(PILFunction *AddrF) {
   if (!AddrF->isGlobalInit())
      return nullptr;

   if (auto *PILG = getStaticallyInitializedVariable(AddrF))
      return PILG;

   // If the addressor contains a single "once" call, it calls globalinit_func,
   // and the globalinit_func is called by "once" from a single location,
   // continue; otherwise bail.
   BuiltinInst *CallToOnce;
   auto *InitF = findInitializer(&AddrF->getModule(), AddrF, CallToOnce);

   if (!InitF || !InitF->getName().startswith("globalinit_"))
      return nullptr;

   // If the globalinit_func is trivial, continue; otherwise bail.
   SingleValueInstruction *dummyInitVal;
   auto *PILG = getVariableOfStaticInitializer(InitF, dummyInitVal);

   return PILG;
}

PILFunction *polar::getCalleeOfOnceCall(BuiltinInst *BI) {
   assert(BI->getNumOperands() == 2 && "once call should have 2 operands.");

   auto Callee = BI->getOperand(1);
   assert(Callee->getType().castTo<PILFunctionType>()->getRepresentation()
          == PILFunctionTypeRepresentation::CFunctionPointer &&
          "Expected C function representation!");

   if (auto *FR = dyn_cast<FunctionRefInst>(Callee))
      return FR->getReferencedFunctionOrNull();

   return nullptr;
}

// Find the globalinit_func by analyzing the body of the addressor.
PILFunction *polar::findInitializer(PILModule *Module, PILFunction *AddrF,
                                    BuiltinInst *&CallToOnce) {
   // We only handle a single PILBasicBlock for now.
   if (AddrF->size() != 1)
      return nullptr;

   CallToOnce = nullptr;
   PILBasicBlock *BB = &AddrF->front();
   for (auto &I : *BB) {
      // Find the builtin "once" call.
      if (auto *BI = dyn_cast<BuiltinInst>(&I)) {
         const BuiltinInfo &Builtin =
            BI->getModule().getBuiltinInfo(BI->getName());
         if (Builtin.ID != BuiltinValueKind::Once)
            continue;

         // Bail if we have multiple "once" calls in the addressor.
         if (CallToOnce)
            return nullptr;

         CallToOnce = BI;
      }
   }
   if (!CallToOnce)
      return nullptr;
   return getCalleeOfOnceCall(CallToOnce);
}

PILGlobalVariable *
polar::getVariableOfStaticInitializer(PILFunction *InitFunc,
                                      SingleValueInstruction *&InitVal) {
   InitVal = nullptr;
   PILGlobalVariable *GVar = nullptr;
   // We only handle a single PILBasicBlock for now.
   if (InitFunc->size() != 1)
      return nullptr;

   PILBasicBlock *BB = &InitFunc->front();
   GlobalAddrInst *SGA = nullptr;
   bool HasStore = false;
   for (auto &I : *BB) {
      // Make sure we have a single GlobalAddrInst and a single StoreInst.
      // And the StoreInst writes to the GlobalAddrInst.
      if (isa<AllocGlobalInst>(&I) || isa<ReturnInst>(&I)
          || isa<DebugValueInst>(&I)) {
         continue;
      } else if (auto *sga = dyn_cast<GlobalAddrInst>(&I)) {
         if (SGA)
            return nullptr;
         SGA = sga;
         GVar = SGA->getReferencedGlobal();
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
         if (HasStore || SI->getDest() != SGA)
            return nullptr;
         HasStore = true;
         PILValue value = SI->getSrc();

         // We only handle StructInst and TupleInst being stored to a
         // global variable for now.
         if (!isa<StructInst>(value) && !isa<TupleInst>(value))
            return nullptr;
         InitVal = cast<SingleValueInstruction>(value);
      } else if (!PILGlobalVariable::isValidStaticInitializerInst(&I,
                                                                  I.getModule())) {
         return nullptr;
      }
   }
   if (!InitVal)
      return nullptr;
   return GVar;
}

PILType
PILGlobalVariable::getLoweredTypeInContext(TypeExpansionContext context) const {
   auto ty = getLoweredType();
   if (!ty.getAstType()->hasOpaqueArchetype() ||
       !context.shouldLookThroughOpaqueTypeArchetypes())
      return ty;
   auto resultTy =
      getModule().Types.getTypeLowering(ty, context).getLoweredType();
   return resultTy.getCategoryType(ty.getCategory());
}
