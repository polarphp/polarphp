//===--- Linker.cpp -------------------------------------------------------===//
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
///
/// \file
///
/// The PIL linker walks the call graph beginning at a starting function,
/// deserializing functions, vtables and witness tables.
///
/// The behavior of the linker is controlled by a LinkMode value. The LinkMode
/// has three possible values:
///
/// - LinkNone: The linker does not deserialize anything. This is only used for
///   debugging and testing purposes, and never during normal operation.
///
/// - LinkNormal: The linker deserializes bodies for declarations that must be
///   emitted into the client because they do not have definitions available
///   externally. This includes:
///
///   - witness tables for imported conformances
///
///   - functions with shared linkage
///
/// - LinkAll: All reachable functions (including public functions) are
///   deserialized, including public functions.
///
/// The primary entry point into the linker is the PILModule::linkFunction()
/// function, which recursively walks the call graph starting from the given
/// function.
///
/// In the mandatory pipeline (-Onone), the linker is invoked from the mandatory
/// PIL linker pass, which pulls in just enough to allow us to emit code, using
/// LinkNormal mode.
///
/// In the performance pipeline, after guaranteed optimizations but before
/// performance optimizations, the 'performance PILLinker' pass links
/// transitively all reachable functions, to uncover optimization opportunities
/// that might be missed from deserializing late. The performance pipeline uses
/// LinkAll mode.
///
/// *NOTE*: In LinkAll mode, we deserialize all vtables and witness tables,
/// even those with public linkage. This is not strictly necessary, since the
/// devirtualizer deserializes vtables and witness tables as needed. However,
/// doing so early creates more opportunities for optimization.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-linker"

#include "polarphp/pil/lang/internal/Linker.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/Debug.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/clangimporter/ClangModule.h"
#include "polarphp/pil/lang/FormalLinkage.h"
#include <functional>

using namespace polar;
using namespace polar::lowering;

STATISTIC(NumFuncLinked, "Number of PIL functions linked");

//===----------------------------------------------------------------------===//
//                               Linker Helpers
//===----------------------------------------------------------------------===//

void PILLinkerVisitor::addFunctionToWorklist(PILFunction *F) {
   assert(F->isExternalDeclaration());

   LLVM_DEBUG(llvm::dbgs() << "Imported function: "
                           << F->getName() << "\n");
   if (Mod.loadFunction(F)) {
      if (F->isExternalDeclaration())
         return;

      F->setBare(IsBare);
      F->verify();
      Worklist.push_back(F);
      Changed = true;
      ++NumFuncLinked;
   }
}

/// Deserialize a function and add it to the worklist for processing.
void PILLinkerVisitor::maybeAddFunctionToWorklist(PILFunction *F) {
   // Don't need to do anything if the function already has a body.
   if (!F->isExternalDeclaration())
      return;

   // In the performance pipeline, we deserialize all reachable functions.
   if (isLinkAll())
      return addFunctionToWorklist(F);

   // Otherwise, make sure to deserialize shared functions; we need to
   // emit them into the client binary since they're not available
   // externally.
   if (hasSharedVisibility(F->getLinkage()))
      return addFunctionToWorklist(F);

   // Functions with PublicNonABI linkage are deserialized as having
   // HiddenExternal linkage when they are declarations, then they
   // become SharedExternal after the body has been deserialized.
   // So try deserializing HiddenExternal functions too.
   if (F->getLinkage() == PILLinkage::HiddenExternal)
      return addFunctionToWorklist(F);

   // Update the linkage of the function in case it's different in the serialized
   // PIL than derived from the AST. This can be the case with cross-module-
   // optimizations.
   Mod.updateFunctionLinkage(F);
}

/// Process F, recursively deserializing any thing F may reference.
bool PILLinkerVisitor::processFunction(PILFunction *F) {
   // If F is a declaration, first deserialize it.
   if (F->isExternalDeclaration()) {
      maybeAddFunctionToWorklist(F);
   } else {
      Worklist.push_back(F);
   }

   process();
   return Changed;
}

/// Deserialize the given VTable all PIL the VTable transitively references.
void PILLinkerVisitor::linkInVTable(ClassDecl *D) {
   // Devirtualization already deserializes vtables as needed in both the
   // mandatory and performance pipelines, and we don't support specialized
   // vtables that might have shared linkage yet, so this is only needed in
   // the performance pipeline to deserialize more functions early, and expose
   // optimization opportunities.
   assert(isLinkAll());

   // Attempt to lookup the Vtbl from the PILModule.
   PILVTable *Vtbl = Mod.lookUpVTable(D);
   if (!Vtbl)
      return;

   // Ok we found our VTable. Visit each function referenced by the VTable. If
   // any of the functions are external declarations, add them to the worklist
   // for processing.
   for (auto P : Vtbl->getEntries()) {
      // Deserialize and recursively walk any vtable entries that do not have
      // bodies yet.
      maybeAddFunctionToWorklist(P.Implementation);
   }
}

//===----------------------------------------------------------------------===//
//                                  Visitors
//===----------------------------------------------------------------------===//

void PILLinkerVisitor::visitApplyInst(ApplyInst *AI) {
   visitApplySubstitutions(AI->getSubstitutionMap());
}

void PILLinkerVisitor::visitTryApplyInst(TryApplyInst *TAI) {
   visitApplySubstitutions(TAI->getSubstitutionMap());
}

void PILLinkerVisitor::visitPartialApplyInst(PartialApplyInst *PAI) {
   visitApplySubstitutions(PAI->getSubstitutionMap());
}

void PILLinkerVisitor::visitFunctionRefInst(FunctionRefInst *FRI) {
   maybeAddFunctionToWorklist(FRI->getInitiallyReferencedFunction());
}

void PILLinkerVisitor::visitDynamicFunctionRefInst(
   DynamicFunctionRefInst *FRI) {
   maybeAddFunctionToWorklist(FRI->getInitiallyReferencedFunction());
}

void PILLinkerVisitor::visitPreviousDynamicFunctionRefInst(
   PreviousDynamicFunctionRefInst *FRI) {
   maybeAddFunctionToWorklist(FRI->getInitiallyReferencedFunction());
}

// Eagerly visiting all used conformances leads to a large blowup
// in the amount of PIL we read in. For optimization purposes we can defer
// reading in most conformances until we need them for devirtualization.
// However, we *must* pull in shared clang-importer-derived conformances
// we potentially use, since we may not otherwise have a local definition.
static bool mustDeserializeInterfaceConformance(PILModule &M,
                                                InterfaceConformanceRef c) {
   if (!c.isConcrete())
      return false;
   auto conformance = c.getConcrete()->getRootConformance();
   return M.Types.interfaceRequiresWitnessTable(conformance->getInterface())
          && isa<ClangModuleUnit>(conformance->getDeclContext()
                                     ->getModuleScopeContext());
}

void PILLinkerVisitor::visitInterfaceConformance(
   InterfaceConformanceRef ref, const Optional<PILDeclRef> &Member) {
   // If an abstract interface conformance was passed in, do nothing.
   if (ref.isAbstract())
      return;

   bool mustDeserialize = mustDeserializeInterfaceConformance(Mod, ref);

   // Otherwise try and lookup a witness table for C.
   auto C = ref.getConcrete();

   if (!VisitedConformances.insert(C).second)
      return;

   auto *WT = Mod.lookUpWitnessTable(C, mustDeserialize);

   // If the looked up witness table is a declaration, there is nothing we can
   // do here.
   if (WT == nullptr || WT->isDeclaration()) {
#ifndef NDEBUG
      if (mustDeserialize) {
         llvm::errs() << "PILGen failed to emit required conformance:\n";
         ref.dump(llvm::errs());
         llvm::errs() << "\n";
         abort();
      }
#endif
      return;
   }

   auto maybeVisitRelatedConformance = [&](InterfaceConformanceRef c) {
      // Formally all conformances referenced by a used conformance are used.
      // However, eagerly visiting them all at this point leads to a large blowup
      // in the amount of PIL we read in. For optimization purposes we can defer
      // reading in most conformances until we need them for devirtualization.
      // However, we *must* pull in shared clang-importer-derived conformances
      // we potentially use, since we may not otherwise have a local definition.
      if (mustDeserializeInterfaceConformance(Mod, c))
         visitInterfaceConformance(c, None);
   };

   // For each entry in the witness table...
   for (auto &E : WT->getEntries()) {
      switch (E.getKind()) {
         // If the entry is a witness method...
         case PILWitnessTable::WitnessKind::Method: {
            // And we are only interested in deserializing a specific requirement
            // and don't have that requirement, don't deserialize this method.
            if (Member.hasValue() && E.getMethodWitness().Requirement != *Member)
               continue;

            // The witness could be removed by dead function elimination.
            if (!E.getMethodWitness().Witness)
               continue;

            // Otherwise, deserialize the witness if it has shared linkage, or if
            // we were asked to deserialize everything.
            maybeAddFunctionToWorklist(E.getMethodWitness().Witness);
            break;
         }

            // If the entry is a related witness table, see whether we need to
            // eagerly deserialize it.
         case PILWitnessTable::WitnessKind::BaseInterface: {
            auto baseConformance = E.getBaseInterfaceWitness().Witness;
            maybeVisitRelatedConformance(InterfaceConformanceRef(baseConformance));
            break;
         }
         case PILWitnessTable::WitnessKind::AssociatedTypeInterface: {
            auto assocConformance = E.getAssociatedTypeInterfaceWitness().Witness;
            maybeVisitRelatedConformance(assocConformance);
            break;
         }

         case PILWitnessTable::WitnessKind::AssociatedType:
         case PILWitnessTable::WitnessKind::Invalid:
            break;
      }
   }
}

void PILLinkerVisitor::visitApplySubstitutions(SubstitutionMap subs) {
   for (auto conformance : subs.getConformances()) {
      // Formally all conformances referenced in a function application are
      // used. However, eagerly visiting them all at this point leads to a
      // large blowup in the amount of PIL we read in, and we aren't very
      // systematic about laziness. For optimization purposes we can defer
      // reading in most conformances until we need them for devirtualization.
      // However, we *must* pull in shared clang-importer-derived conformances
      // we potentially use, since we may not otherwise have a local definition.
      if (mustDeserializeInterfaceConformance(Mod, conformance)) {
         visitInterfaceConformance(conformance, None);
      }
   }
}

void PILLinkerVisitor::visitInitExistentialAddrInst(
   InitExistentialAddrInst *IEI) {
   // Link in all interface conformances that this touches.
   //
   // TODO: There might be a two step solution where the init_existential_inst
   // causes the witness table to be brought in as a declaration and then the
   // interface method inst causes the actual deserialization. For now we are
   // not going to be smart about this to enable avoiding any issues with
   // visiting the open_existential_addr/witness_method before the
   // init_existential_inst.
   for (InterfaceConformanceRef C : IEI->getConformances()) {
      visitInterfaceConformance(C, Optional<PILDeclRef>());
   }
}

void PILLinkerVisitor::visitInitExistentialRefInst(
   InitExistentialRefInst *IERI) {
   // Link in all interface conformances that this touches.
   //
   // TODO: There might be a two step solution where the init_existential_inst
   // causes the witness table to be brought in as a declaration and then the
   // interface method inst causes the actual deserialization. For now we are
   // not going to be smart about this to enable avoiding any issues with
   // visiting the interface_method before the init_existential_inst.
   for (InterfaceConformanceRef C : IERI->getConformances()) {
      visitInterfaceConformance(C, Optional<PILDeclRef>());
   }
}

void PILLinkerVisitor::visitAllocRefInst(AllocRefInst *ARI) {
   if (!isLinkAll())
      return;

   // Grab the class decl from the alloc ref inst.
   ClassDecl *D = ARI->getType().getClassOrBoundGenericClass();
   if (!D)
      return;

   linkInVTable(D);
}

void PILLinkerVisitor::visitMetatypeInst(MetatypeInst *MI) {
   if (!isLinkAll())
      return;

   CanType instTy = MI->getType().castTo<MetatypeType>().getInstanceType();
   ClassDecl *C = instTy.getClassOrBoundGenericClass();
   if (!C)
      return;

   linkInVTable(C);
}

//===----------------------------------------------------------------------===//
//                             Top Level Routine
//===----------------------------------------------------------------------===//

// Main loop of the visitor. Called by one of the other *visit* methods.
void PILLinkerVisitor::process() {
   // Process everything transitively referenced by one of the functions in the
   // worklist.
   while (!Worklist.empty()) {
      auto *Fn = Worklist.pop_back_val();

      if (Fn->getModule().isSerialized()) {
         // If the containing module has been serialized,
         // Remove The Serialized state (if any)
         //  This allows for more optimizations
         Fn->setSerialized(IsSerialized_t::IsNotSerialized);
      }

      LLVM_DEBUG(llvm::dbgs() << "Process imports in function: "
                              << Fn->getName() << "\n");

      for (auto &BB : *Fn) {
         for (auto &I : BB) {
            visit(&I);
         }
      }
   }
}
