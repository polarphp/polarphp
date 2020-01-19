//===--- UsePrespecialized.cpp - use pre-specialized functions ------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
/// An optimization which marks functions and types as inlinable or usable
/// from inline. This lets such functions be serialized (later in the pipeline),
/// which makes them available for other modules.
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "cross-module-serialization-setup"

#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "polarphp/ast/Module.h"

using namespace polar;

namespace {

/// Scans a whole module and marks functions and types as inlinable or usable
/// from inline.
class CrossModuleSerializationSetup {
   friend class InstructionVisitor;

   // The worklist of function which should be serialized.
   llvm::SmallVector<PILFunction *, 16> workList;
   llvm::SmallPtrSet<PILFunction *, 16> functionsHandled;

   llvm::SmallPtrSet<TypeBase *, 16> typesHandled;

   PILModule &M;

   void addToWorklistIfNotHandled(PILFunction *F) {
      if (functionsHandled.count(F) == 0) {
         workList.push_back(F);
         functionsHandled.insert(F);
      }
   }

   bool setUpForSerialization(PILFunction *F);

   void prepareInstructionForSerialization(PILInstruction *inst);

   void handleReferencedFunction(PILFunction *F);

   void handleReferencedMethod(PILDeclRef method);

   void makeTypeUsableFromInline(CanType type);

   void makeSubstUsableFromInline(const SubstitutionMap &substs);

public:
   CrossModuleSerializationSetup(PILModule &M) : M(M) { }

   void scanModule();
};

static bool canUseFromInline(PILFunction *F) {
   if (!F)
      return false;

   switch (F->getLinkage()) {
      case PILLinkage::PublicNonABI:
      case PILLinkage::Shared:
         return F->isSerialized() != IsNotSerialized;
      case PILLinkage::Public:
      case PILLinkage::Hidden:
      case PILLinkage::Private:
      case PILLinkage::PublicExternal:
      case PILLinkage::SharedExternal:
      case PILLinkage::PrivateExternal:
      case PILLinkage::HiddenExternal:
         break;
   }

   return true;
}

/// Visitor for making used types of an intruction inlinable.
///
/// We use the PILCloner for visiting types, though it sucks that we allocate
/// instructions just to delete them immediately. But it's better than to
/// reimplement the logic.
/// TODO: separate the type visiting logic in PILCloner from the instruction
/// creation.
class InstructionVisitor : public PILCloner<InstructionVisitor> {
   friend class PILCloner<InstructionVisitor>;
   friend class PILInstructionVisitor<InstructionVisitor>;

private:
   CrossModuleSerializationSetup &CMS;

public:
   InstructionVisitor(PILFunction *F, CrossModuleSerializationSetup &CMS) :
      PILCloner(*F), CMS(CMS) {}

   PILType remapType(PILType Ty) {
      CMS.makeTypeUsableFromInline(Ty.getAstType());
      return Ty;
   }

   CanType remapASTType(CanType Ty) {
      CMS.makeTypeUsableFromInline(Ty);
      return Ty;
   }

   SubstitutionMap remapSubstitutionMap(SubstitutionMap Subs) {
      CMS.makeSubstUsableFromInline(Subs);
      return Subs;
   }

   void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
      PILInstruction::destroy(Cloned);
      Orig->getFunction()->getModule().deallocateInst(Cloned);
   }

   PILValue getMappedValue(PILValue Value) { return Value; }

   PILBasicBlock *remapBasicBlock(PILBasicBlock *BB) { return BB; }

   static void visitInst(PILInstruction *I, CrossModuleSerializationSetup &CMS) {
      InstructionVisitor visitor(I->getFunction(), CMS);
      visitor.visit(I);
   }
};

/// Make a nominal type, including it's context, usable from inline.
static void makeDeclUsableFromInline(ValueDecl *decl, PILModule &M) {
   if (decl->getEffectiveAccess() >= AccessLevel::Public)
      return;

   if (!decl->isUsableFromInline()) {
      // Mark the nominal type as "usableFromInline".
      // TODO: find a way to do this without modifying the AST. The AST should be
      // immutable at this point.
      auto &ctx = decl->getAstContext();
      auto *attr = new (ctx) UsableFromInlineAttr(/*implicit=*/true);
      decl->getAttrs().add(attr);
   }
   if (auto *nominalCtx = dyn_cast<NominalTypeDecl>(decl->getDeclContext())) {
      makeDeclUsableFromInline(nominalCtx, M);
   } else if (auto *extCtx = dyn_cast<ExtensionDecl>(decl->getDeclContext())) {
      if (auto *extendedNominal = extCtx->getExtendedNominal()) {
         makeDeclUsableFromInline(extendedNominal, M);
      }
   } else if (decl->getDeclContext()->isLocalContext()) {
      // TODO
   }
}

/// Ensure that the \p type is usable from serialized functions.
void CrossModuleSerializationSetup::makeTypeUsableFromInline(CanType type) {
   if (!typesHandled.insert(type.getPointer()).second)
      return;

   if (NominalTypeDecl *NT = type->getNominalOrBoundGenericNominal()) {
      makeDeclUsableFromInline(NT, M);
   }

   // Also make all sub-types usable from inline.
   type.visit([this](Type rawSubType) {
      CanType subType = rawSubType->getCanonicalType();
      if (typesHandled.insert(subType.getPointer()).second) {
         if (NominalTypeDecl *subNT = subType->getNominalOrBoundGenericNominal()) {
            makeDeclUsableFromInline(subNT, M);
         }
      }
   });
}

/// Ensure that all replacement types of \p substs are usable from serialized
/// functions.
void CrossModuleSerializationSetup::
makeSubstUsableFromInline(const SubstitutionMap &substs) {
   for (Type replType : substs.getReplacementTypes()) {
      makeTypeUsableFromInline(replType->getCanonicalType());
   }
}

/// Decide whether to serialize a function.
static bool shouldSerialize(PILFunction *F) {
   // The basic heursitic: serialize all generic functions, because it makes a
   // huge difference if generic functions can be specialized or not.
   if (!F->getLoweredFunctionType()->isPolymorphic())
      return false;

   // Check if we already handled this function before.
   if (F->isSerialized() == IsSerialized)
      return false;

   if (F->hasSemanticsAttr("optimize.no.crossmodule"))
      return false;

   return true;
}

static void makeFunctionUsableFromInline(PILFunction *F) {
   if (!is_available_externally(F->getLinkage()))
      F->setLinkage(PILLinkage::Public);
}

/// Prepare \p inst for serialization and in case it's a function_ref, put the
/// referenced function onto the worklist.
void CrossModuleSerializationSetup::
prepareInstructionForSerialization(PILInstruction *inst) {
   // Make all types of the instruction usable from inline.
   InstructionVisitor::visitInst(inst, *this);

   // Put callees onto the worklist if they should be serialized as well.
   if (auto *FRI = dyn_cast<FunctionRefBaseInst>(inst)) {
      PILFunction *callee = FRI->getReferencedFunctionOrNull();
      assert(callee);
      handleReferencedFunction(callee);
      return;
   }
   if (auto *MI = dyn_cast<MethodInst>(inst)) {
      handleReferencedMethod(MI->getMember());
      return;
   }
   if (auto *KPI = dyn_cast<KeyPathInst>(inst)) {
      KPI->getPattern()->visitReferencedFunctionsAndMethods(
         [this](PILFunction *func) { handleReferencedFunction(func); },
         [this](PILDeclRef method) { handleReferencedMethod(method); });
      return;
   }
   if (auto *REAI = dyn_cast<RefElementAddrInst>(inst)) {
      makeDeclUsableFromInline(REAI->getField(), M);
   }
}

void CrossModuleSerializationSetup::handleReferencedFunction(PILFunction *func) {
   if (!func->isDefinition() || func->isAvailableExternally())
      return;
   if (shouldSerialize(func)) {
      addToWorklistIfNotHandled(func);
   } else {
      makeFunctionUsableFromInline(func);
   }
}

void CrossModuleSerializationSetup::handleReferencedMethod(PILDeclRef method) {
   if (method.isForeign)
      return;
   // Prevent the method from dead-method elimination.
   auto *methodDecl = cast<AbstractFunctionDecl>(method.getDecl());
   M.addExternallyVisibleDecl(getBaseMethod(methodDecl));
}

/// Setup the function \p param F for serialization and put callees onto the
/// worklist for further processing.
///
/// Returns false in case this is not possible for some reason.
bool CrossModuleSerializationSetup::setUpForSerialization(PILFunction *F) {
   // First step: check if serializing F is even possible.
   for (PILBasicBlock &block : *F) {
      for (PILInstruction &inst : block) {
         if (auto *FRI = dyn_cast<FunctionRefBaseInst>(&inst)) {
            PILFunction *callee = FRI->getReferencedFunctionOrNull();
            if (!canUseFromInline(callee))
               return false;
         } else if (auto *KPI = dyn_cast<KeyPathInst>(&inst)) {
            bool canUse = true;
            KPI->getPattern()->visitReferencedFunctionsAndMethods(
               [&](PILFunction *func) {
                  if (!canUseFromInline(func))
                     canUse = false;
               },
               [](PILDeclRef method) { });
            if (!canUse)
               return false;
         }
      }
   }

   // Second step: go through all instructions and prepare them for
   // for serialization.
   for (PILBasicBlock &block : *F) {
      for (PILInstruction &inst : block) {
         prepareInstructionForSerialization(&inst);
      }
   }
   return true;
}

/// Select functions in the module which should be serialized.
void CrossModuleSerializationSetup::scanModule() {

   // Start with public functions.
   for (PILFunction &F : M) {
      if (F.getLinkage() == PILLinkage::Public)
         addToWorklistIfNotHandled(&F);
   }

   // Continue with called functions.
   while (!workList.empty()) {
      PILFunction *F = workList.pop_back_val();
      // Decide whether we want to serialize the function.
      if (shouldSerialize(F)) {
         // Try to serialize.
         if (setUpForSerialization(F)) {
            F->setSerialized(IsSerialized);

            // As a code size optimization, make serialized functions
            // @alwaysEmitIntoClient.
            F->setLinkage(PILLinkage::PublicNonABI);
         } else {
            // If for some reason the function cannot be serialized, we mark it as
            // usable-from-inline.
            makeFunctionUsableFromInline(F);
         }
      }
   }
}

class CrossModuleSerializationSetupPass: public PILModuleTransform {
   void run() override {

      auto &M = *getModule();
      if (M.getTypePHPModule()->isResilient())
         return;
      if (!M.isWholeModule())
         return;
      if (!M.getOptions().CrossModuleOptimization)
         return;

      CrossModuleSerializationSetup CMSS(M);
      CMSS.scanModule();
   }
};

} // end anonymous namespace

PILTransform *polar::createCrossModuleSerializationSetup() {
   return new CrossModuleSerializationSetupPass();
}
