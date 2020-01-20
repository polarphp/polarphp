//===--- DeadFunctionElimination.cpp - Eliminate dead functions -----------===//
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

#define DEBUG_TYPE "pil-dead-function-elimination"

#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/pil/lang/InstructionUtils.h"
#include "polarphp/pil/lang/PatternMatch.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

using namespace polar;

STATISTIC(NumDeadFunc, "Number of dead functions eliminated");

namespace {

/// This is a base class for passes that are based on function liveness
/// computations like e.g. dead function elimination.
/// It provides a common logic for computing live (i.e. reachable) functions.
class FunctionLivenessComputation {
protected:

   /// Represents a function which is implementing a vtable or witness table
   /// method.
   struct FuncImpl {
      FuncImpl(PILFunction *F, ClassDecl *Cl) : F(F), Impl(Cl) {}
      FuncImpl(PILFunction *F, RootInterfaceConformance *C) : F(F), Impl(C) {}

      /// The implementing function.
      PILFunction *F;

      /// This is a class decl if we are tracking a class_method (i.e. a vtable
      /// method) and a protocol conformance if we are tracking a witness_method.
      PointerUnion<ClassDecl *, RootInterfaceConformance*> Impl;
   };

   /// Stores which functions implement a vtable or witness table method.
   struct MethodInfo {

      MethodInfo(bool isWitnessMethod) :
         methodIsCalled(false), isWitnessMethod(isWitnessMethod) {}

      /// All functions which implement the method. Together with the class for
      /// which the function implements the method. In case of a witness method,
      /// the class pointer is null.
      SmallVector<FuncImpl, 8> implementingFunctions;

      /// True, if the method is called, meaning that any of it's implementations
      /// may be called.
      bool methodIsCalled;

      /// True if this is a witness method, false if it's a vtable method.
      bool isWitnessMethod;

      /// Adds an implementation of the method in a specific class.
      void addClassMethodImpl(PILFunction *F, ClassDecl *C) {
         assert(!isWitnessMethod);
         implementingFunctions.push_back(FuncImpl(F, C));
      }

      /// Adds an implementation of the method in a specific conformance.
      void addWitnessFunction(PILFunction *F, RootInterfaceConformance *Conf) {
         assert(isWitnessMethod);
         implementingFunctions.push_back(FuncImpl(F, Conf));
      }
   };

   PILModule *Module;

   llvm::DenseMap<AbstractFunctionDecl *, MethodInfo *> MethodInfos;
   llvm::SpecificBumpPtrAllocator<MethodInfo> MethodInfoAllocator;

   llvm::SmallSetVector<PILFunction *, 16> Worklist;

   llvm::SmallPtrSet<void *, 32> AliveFunctionsAndTables;

   /// Checks is a function is alive, e.g. because it is visible externally.
   bool isAnchorFunction(PILFunction *F) {

      // Functions that may be used externally cannot be removed.
      if (F->isPossiblyUsedExternally())
         return true;

      if (F->getDynamicallyReplacedFunction())
         return true;

      if (F->isDynamicallyReplaceable())
         return true;

      // ObjC functions are called through the runtime and are therefore alive
      // even if not referenced inside PIL.
      if (F->getRepresentation() == PILFunctionTypeRepresentation::ObjCMethod)
         return true;

      // Global initializers are always emitted into the defining module and
      // their bodies are never PIL serialized.
      if (F->isGlobalInit())
         return true;

      return false;
   }

   /// Gets or creates the MethodInfo for a vtable or witness table method.
   /// \p decl The method declaration. In case of a vtable method this is always
   ///         the most overridden method.
   MethodInfo *getMethodInfo(AbstractFunctionDecl *decl, bool isWitnessMethod) {
      MethodInfo *&entry = MethodInfos[decl];
      if (entry == nullptr) {
         entry = new (MethodInfoAllocator.Allocate()) MethodInfo(isWitnessMethod);
      }
      assert(entry->isWitnessMethod == isWitnessMethod);
      return entry;
   }

   /// Returns true if a function is marked as alive.
   bool isAlive(PILFunction *F) {
      return AliveFunctionsAndTables.count(F) != 0;
   }

   /// Returns true if a witness table is marked as alive.
   bool isAlive(PILWitnessTable *WT) {
      return AliveFunctionsAndTables.count(WT) != 0;
   }

   /// Marks a function as alive.
   void makeAlive(PILFunction *F) {
      AliveFunctionsAndTables.insert(F);
      assert(F && "function does not exist");
      Worklist.insert(F);
   }

   /// Marks all contained functions and witness tables of a witness table as
   /// alive.
   void makeAlive(PILWitnessTable *WT) {
      LLVM_DEBUG(llvm::dbgs() << "    scan witness table " << WT->getName()
                              << '\n');

      AliveFunctionsAndTables.insert(WT);
      for (const PILWitnessTable::Entry &entry : WT->getEntries()) {
         switch (entry.getKind()) {
            case PILWitnessTable::Method: {

               auto methodWitness = entry.getMethodWitness();
               auto *fd = cast<AbstractFunctionDecl>(methodWitness.Requirement.
                  getDecl());
               assert(fd == getBaseMethod(fd) &&
                      "key in witness table is overridden");
               PILFunction *F = methodWitness.Witness;
               if (F) {
                  MethodInfo *MI = getMethodInfo(fd, /*isWitnessMethod*/ true);
                  if (MI->methodIsCalled || !F->isDefinition())
                     ensureAlive(F);
               }
            } break;

            case PILWitnessTable::AssociatedTypeInterface: {
               InterfaceConformanceRef CRef =
                  entry.getAssociatedTypeInterfaceWitness().Witness;
               if (CRef.isConcrete())
                  ensureAliveConformance(CRef.getConcrete());
               break;
            }
            case PILWitnessTable::BaseInterface:
               ensureAliveConformance(entry.getBaseInterfaceWitness().Witness);
               break;

            case PILWitnessTable::Invalid:
            case PILWitnessTable::AssociatedType:
               break;
         }
      }

      for (const auto &conf : WT->getConditionalConformances()) {
         if (conf.Conformance.isConcrete())
            ensureAliveConformance(conf.Conformance.getConcrete());
      }
   }

   /// Marks the declarations referenced by a key path pattern as alive if they
   /// aren't yet.
   void
   ensureKeyPathComponentIsAlive(const KeyPathPatternComponent &component) {
      component.visitReferencedFunctionsAndMethods(
         [this](PILFunction *F) {
            ensureAlive(F);
         },
         [this](PILDeclRef method) {
            if (method.isForeign) {
               // Nothing to do here: foreign functions aren't ours to be deleting.
               // (And even if they were, they're ObjC-dispatched and thus anchored
               // already: see isAnchorFunction)
            } else {
               auto decl = cast<AbstractFunctionDecl>(method.getDecl());
               if (auto clas = dyn_cast<ClassDecl>(decl->getDeclContext())) {
                  ensureAliveClassMethod(getMethodInfo(decl, /*witness*/ false),
                                         dyn_cast<FuncDecl>(decl),
                                         clas);
               } else if (isa<InterfaceDecl>(decl->getDeclContext())) {
                  ensureAliveInterfaceMethod(getMethodInfo(decl, /*witness*/ true));
               } else {
                  llvm_unreachable("key path keyed by a non-class, non-protocol method");
               }
            }
         }
      );
   }

   /// Marks a function as alive if it is not alive yet.
   void ensureAlive(PILFunction *F) {
      if (!isAlive(F))
         makeAlive(F);
   }

   /// Marks a witness table as alive if it is not alive yet.
   void ensureAliveConformance(const InterfaceConformance *C) {
      PILWitnessTable *WT = Module->lookUpWitnessTable(C,
         /*deserializeLazily*/ false);
      if (!WT || isAlive(WT))
         return;
      makeAlive(WT);
   }

   /// Returns true if the implementation of method \p FD in class \p ImplCl
   /// may be called when the type of the class_method's operand is \p MethodCl.
   /// Both, \p MethodCl and \p ImplCl, may by null if not known or if it's a
   /// protocol method.
   static bool canHaveSameImplementation(FuncDecl *FD, ClassDecl *MethodCl,
                                         ClassDecl *ImplCl) {
      if (!FD || !MethodCl || !ImplCl)
         return true;

      // All implementations of derived classes may be called.
      if (MethodCl->isSuperclassOf(ImplCl))
         return true;

      // Check if the method implementation is the same in a super class, i.e.
      // it is not overridden in the derived class.
      auto *Impl1 = MethodCl->findImplementingMethod(FD);
      assert(Impl1);
      auto *Impl2 = ImplCl->findImplementingMethod(FD);
      assert(Impl2);

      return Impl1 == Impl2;
   }

   /// Marks the implementing functions of the method \p FD as alive. If it is a
   /// class method, \p MethodCl is the type of the class_method instruction's
   /// operand.
   void ensureAliveClassMethod(MethodInfo *mi, FuncDecl *FD, ClassDecl *MethodCl) {
      if (mi->methodIsCalled)
         return;
      bool allImplsAreCalled = true;

      for (FuncImpl &FImpl : mi->implementingFunctions) {
         if (!isAlive(FImpl.F) &&
             canHaveSameImplementation(FD, MethodCl,
                                       FImpl.Impl.get<ClassDecl *>())) {
            makeAlive(FImpl.F);
         } else {
            allImplsAreCalled = false;
         }
      }
      if (allImplsAreCalled)
         mi->methodIsCalled = true;
   }

   /// Marks the implementing functions of the protocol method \p mi as alive.
   void ensureAliveInterfaceMethod(MethodInfo *mi) {
      assert(mi->isWitnessMethod);
      if (mi->methodIsCalled)
         return;
      mi->methodIsCalled = true;
      for (FuncImpl &FImpl : mi->implementingFunctions) {
         if (auto Conf = FImpl.Impl.dyn_cast<RootInterfaceConformance *>()) {
            PILWitnessTable *WT =
               Module->lookUpWitnessTable(Conf,
                  /*deserializeLazily*/ false);
            if (!WT || isAlive(WT))
               makeAlive(FImpl.F);
         } else {
            makeAlive(FImpl.F);
         }
      }
   }

   /// Scans all references inside a function.
   void scanFunction(PILFunction *F) {

      LLVM_DEBUG(llvm::dbgs() << "    scan function " << F->getName() << '\n');

      // First scan all instructions of the function.
      for (PILBasicBlock &BB : *F) {
         for (PILInstruction &I : BB) {
            if (auto *WMI = dyn_cast<WitnessMethodInst>(&I)) {
               auto *funcDecl = getBaseMethod(
                  cast<AbstractFunctionDecl>(WMI->getMember().getDecl()));
               MethodInfo *mi = getMethodInfo(funcDecl, /*isWitnessTable*/ true);
               ensureAliveInterfaceMethod(mi);
            } else if (auto *MI = dyn_cast<MethodInst>(&I)) {
               auto *funcDecl = getBaseMethod(
                  cast<AbstractFunctionDecl>(MI->getMember().getDecl()));
               assert(MI->getNumOperands() - MI->getNumTypeDependentOperands() == 1
                      && "method insts except witness_method must have 1 operand");
               ClassDecl *MethodCl = MI->getOperand(0)->getType().
                  getClassOrBoundGenericClass();
               MethodInfo *mi = getMethodInfo(funcDecl, /*isWitnessTable*/ false);
               ensureAliveClassMethod(mi, dyn_cast<FuncDecl>(funcDecl), MethodCl);
            } else if (auto *FRI = dyn_cast<FunctionRefInst>(&I)) {
               ensureAlive(FRI->getInitiallyReferencedFunction());
            } else if (auto *FRI = dyn_cast<DynamicFunctionRefInst>(&I)) {
               ensureAlive(FRI->getInitiallyReferencedFunction());
            } else if (auto *FRI = dyn_cast<PreviousDynamicFunctionRefInst>(&I)) {
               ensureAlive(FRI->getInitiallyReferencedFunction());
            } else if (auto *KPI = dyn_cast<KeyPathInst>(&I)) {
               for (auto &component : KPI->getPattern()->getComponents())
                  ensureKeyPathComponentIsAlive(component);
            }
         }
      }
   }

   /// Retrieve the visibility information from the AST.
   ///
   /// This differs from PILModule::isVisibleExternally(VarDecl *) because of
   /// it's handling of class methods. It returns true for methods whose
   /// declarations are not directly visible externally, but have been imported
   /// from another module. This ensures that entries aren't deleted from vtables
   /// imported from the stdlib.
   /// FIXME: Passes should not embed special logic for handling linkage.
   bool isVisibleExternally(const ValueDecl *decl) {
      AccessLevel access = decl->getEffectiveAccess();
      PILLinkage linkage;
      switch (access) {
         case AccessLevel::Private:
         case AccessLevel::FilePrivate:
            linkage = PILLinkage::Private;
            break;
         case AccessLevel::Internal:
            linkage = PILLinkage::Hidden;
            break;
         case AccessLevel::Public:
         case AccessLevel::Open:
            linkage = PILLinkage::Public;
            break;
      }
      if (isPossiblyUsedExternally(linkage, Module->isWholeModule()))
         return true;

      // Special case for vtable visibility.
      if (decl->getDeclContext()->getParentModule() != Module->getTypePHPModule())
         return true;

      return false;
   }

   /// Find anchors in vtables and witness tables, if required.
   virtual void findAnchorsInTables() = 0;

   /// Find all functions which are alive from the beginning.
   /// For example, functions which may be referenced externally.
   void findAnchors() {

      findAnchorsInTables();

      for (PILFunction &F : *Module) {
         if (isAnchorFunction(&F)) {
            LLVM_DEBUG(llvm::dbgs() << "  anchor function: " << F.getName() <<"\n");
            ensureAlive(&F);
         }

         if (!F.shouldOptimize()) {
            LLVM_DEBUG(llvm::dbgs() << "  anchor a no optimization function: "
                                    << F.getName() << "\n");
            ensureAlive(&F);
         }
      }
   }

public:
   FunctionLivenessComputation(PILModule *module) :
      Module(module) {}

   /// The main entry point of the optimization.
   bool findAliveFunctions() {

      LLVM_DEBUG(llvm::dbgs() << "running function elimination\n");

      // Find everything which may not be eliminated, e.g. because it is accessed
      // externally.
      findAnchors();

      // The core of the algorithm: Mark functions as alive which can be reached
      // from the anchors.
      while (!Worklist.empty()) {
         PILFunction *F = Worklist.back();
         Worklist.pop_back();
         scanFunction(F);
      }

      return false;
   }

   virtual ~FunctionLivenessComputation() {}
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                             DeadFunctionElimination
//===----------------------------------------------------------------------===//

namespace {

class DeadFunctionElimination : FunctionLivenessComputation {

   void collectMethodImplementations() {
      // Collect vtable method implementations.
      for (PILVTable &vTable : Module->getVTableList()) {
         for (const PILVTable::Entry &entry : vTable.getEntries()) {
            // We don't need to collect destructors because we mark them as alive
            // anyway.
            if (entry.Method.kind == PILDeclRef::Kind::Deallocator ||
                entry.Method.kind == PILDeclRef::Kind::IVarDestroyer) {
               continue;
            }
            PILFunction *F = entry.Implementation;
            auto *fd = getBaseMethod(cast<AbstractFunctionDecl>(
               entry.Method.getDecl()));
            MethodInfo *mi = getMethodInfo(fd, /*isWitnessTable*/ false);
            mi->addClassMethodImpl(F, vTable.getClass());
         }
      }

      // Collect witness method implementations.
      for (PILWitnessTable &WT : Module->getWitnessTableList()) {
         auto Conf = WT.getConformance();
         for (const PILWitnessTable::Entry &entry : WT.getEntries()) {
            if (entry.getKind() != PILWitnessTable::Method)
               continue;

            auto methodWitness = entry.getMethodWitness();
            auto *fd = cast<AbstractFunctionDecl>(methodWitness.Requirement.
               getDecl());
            assert(fd == getBaseMethod(fd) &&
                   "key in witness table is overridden");
            PILFunction *F = methodWitness.Witness;
            if (!F)
               continue;

            MethodInfo *mi = getMethodInfo(fd, /*isWitnessTable*/ true);
            mi->addWitnessFunction(F, Conf);
         }
      }

      // Collect default witness method implementations.
      for (PILDefaultWitnessTable &WT : Module->getDefaultWitnessTableList()) {
         for (const PILDefaultWitnessTable::Entry &entry : WT.getEntries()) {
            if (!entry.isValid() || entry.getKind() != PILWitnessTable::Method)
               continue;

            PILFunction *F = entry. getMethodWitness().Witness;
            auto *fd = cast<AbstractFunctionDecl>(
               entry.getMethodWitness().Requirement.getDecl());
            MethodInfo *mi = getMethodInfo(fd, /*isWitnessTable*/ true);
            mi->addWitnessFunction(F, nullptr);
         }
      }
   }

   /// DeadFunctionElimination pass takes functions
   /// reachable via vtables and witness_tables into account
   /// when computing a function liveness information.
   void findAnchorsInTables() override {

      collectMethodImplementations();

      // Check vtable methods.
      for (PILVTable &vTable : Module->getVTableList()) {
         for (const PILVTable::Entry &entry : vTable.getEntries()) {
            if (entry.Method.kind == PILDeclRef::Kind::Deallocator ||
                entry.Method.kind == PILDeclRef::Kind::IVarDestroyer) {
               // Destructors are alive because they are called from swift_release
               ensureAlive(entry.Implementation);
               continue;
            }

            PILFunction *F = entry.Implementation;
            auto *fd = getBaseMethod(cast<AbstractFunctionDecl>(
               entry.Method.getDecl()));

            if (// We also have to check the method declaration's access level.
               // Needed if it's a public base method declared in another
               // compilation unit (for this we have no PILFunction).
               isVisibleExternally(fd)
               || Module->isExternallyVisibleDecl(fd)
               // Declarations are always accessible externally, so they are alive.
               || !F->isDefinition()) {
               MethodInfo *mi = getMethodInfo(fd, /*isWitnessTable*/ false);
               ensureAliveClassMethod(mi, nullptr, nullptr);
            }
         }
      }

      // Check witness table methods.
      for (PILWitnessTable &WT : Module->getWitnessTableList()) {
         InterfaceConformance *Conf = WT.getConformance();
         bool tableExternallyVisible = isVisibleExternally(Conf->getInterface());
         // The witness table is visible from "outside". Therefore all methods
         // might be called and we mark all methods as alive.
         for (const PILWitnessTable::Entry &entry : WT.getEntries()) {
            if (entry.getKind() != PILWitnessTable::Method)
               continue;

            auto methodWitness = entry.getMethodWitness();
            auto *fd = cast<AbstractFunctionDecl>(methodWitness.Requirement.
               getDecl());
            assert(fd == getBaseMethod(fd) &&
                   "key in witness table is overridden");
            PILFunction *F = methodWitness.Witness;
            if (!F)
               continue;

            if (!tableExternallyVisible && !Module->isExternallyVisibleDecl(fd))
               continue;

            MethodInfo *mi = getMethodInfo(fd, /*isWitnessTable*/ true);
            ensureAliveInterfaceMethod(mi);
         }

         // We don't do dead witness table elimination right now. So we assume
         // that all witness tables are alive. Dead witness table elimination is
         // done in IRGen by lazily emitting witness tables.
         makeAlive(&WT);
      }

      // Check default witness methods.
      for (PILDefaultWitnessTable &WT : Module->getDefaultWitnessTableList()) {
         if (isVisibleExternally(WT.getInterface())) {
            // The default witness table is visible from "outside". Therefore all
            // methods might be called and we mark all methods as alive.
            for (const PILDefaultWitnessTable::Entry &entry : WT.getEntries()) {
               if (!entry.isValid() || entry.getKind() != PILWitnessTable::Method)
                  continue;

               auto *fd =
                  cast<AbstractFunctionDecl>(
                     entry.getMethodWitness().Requirement.getDecl());
               assert(fd == getBaseMethod(fd) &&
                      "key in default witness table is overridden");
               PILFunction *F = entry.getMethodWitness().Witness;
               if (!F)
                  continue;

               MethodInfo *mi = getMethodInfo(fd, /*isWitnessTable*/ true);
               ensureAliveInterfaceMethod(mi);
            }
         }
      }
      // Check property descriptor implementations.
      for (PILProperty &P : Module->getPropertyList()) {
         if (auto component = P.getComponent()) {
            ensureKeyPathComponentIsAlive(*component);
         }
      }

   }

   /// Removes all dead methods from vtables and witness tables.
   bool removeDeadEntriesFromTables() {
      bool changedTable = false;
      for (PILVTable &vTable : Module->getVTableList()) {
         vTable.removeEntries_if([this, &changedTable]
                                    (PILVTable::Entry &entry) -> bool {
            if (!isAlive(entry.Implementation)) {
               LLVM_DEBUG(llvm::dbgs() << "  erase dead vtable method "
                                       << entry.Implementation->getName() << "\n");
               changedTable = true;
               return true;
            }
            return false;
         });
      }

      auto &WitnessTables = Module->getWitnessTableList();
      for (auto WI = WitnessTables.begin(), EI = WitnessTables.end(); WI != EI;) {
         PILWitnessTable *WT = &*WI;
         WI++;
         WT->clearMethods_if([this, &changedTable]
                                (const PILWitnessTable::MethodWitness &MW) -> bool {
            if (!isAlive(MW.Witness)) {
               LLVM_DEBUG(llvm::dbgs() << "  erase dead witness method "
                                       << MW.Witness->getName() << "\n");
               changedTable = true;
               return true;
            }
            return false;
         });
      }

      auto DefaultWitnessTables = Module->getDefaultWitnessTables();
      for (auto WI = DefaultWitnessTables.begin(),
              EI = DefaultWitnessTables.end();
           WI != EI;) {
         PILDefaultWitnessTable *WT = &*WI;
         WI++;
         WT->clearMethods_if([this, &changedTable](PILFunction *MW) -> bool {
            if (!MW)
               return false;
            if (!isAlive(MW)) {
               LLVM_DEBUG(llvm::dbgs() << "  erase dead default witness method "
                                       << MW->getName() << "\n");
               changedTable = true;
               return true;
            }
            return false;
         });
      }
      return changedTable;
   }

public:
   DeadFunctionElimination(PILModule *module)
      : FunctionLivenessComputation(module) {}

   /// The main entry point of the optimization.
   void eliminateFunctions(PILModuleTransform *DFEPass) {

      LLVM_DEBUG(llvm::dbgs() << "running dead function elimination\n");
      findAliveFunctions();

      bool changedTables = removeDeadEntriesFromTables();

      // First drop all references so that we don't get problems with non-zero
      // reference counts of dead functions.
      std::vector<PILFunction *> DeadFunctions;
      for (PILFunction &F : *Module) {
         if (!isAlive(&F)) {
            F.dropAllReferences();
            DeadFunctions.push_back(&F);
         }
      }

      // Next step: delete dead witness tables.
      PILModule::WitnessTableListType &WTables = Module->getWitnessTableList();
      for (auto Iter = WTables.begin(), End = WTables.end(); Iter != End;) {
         PILWitnessTable *Wt = &*Iter;
         Iter++;
         if (!isAlive(Wt)) {
            LLVM_DEBUG(llvm::dbgs() << "  erase dead witness table "
                                    << Wt->getName() << '\n');
            Module->deleteWitnessTable(Wt);
         }
      }

      // Last step: delete all dead functions.
      while (!DeadFunctions.empty()) {
         PILFunction *F = DeadFunctions.back();
         DeadFunctions.pop_back();

         LLVM_DEBUG(llvm::dbgs() << "  erase dead function " << F->getName()
                                 << "\n");
         NumDeadFunc++;
         DFEPass->notifyWillDeleteFunction(F);
         Module->eraseFunction(F);
      }
      if (changedTables)
         DFEPass->invalidateFunctionTables();
   }
};

} // end anonymous namespace

//===----------------------------------------------------------------------===//
//                      Pass Definition and Entry Points
//===----------------------------------------------------------------------===//

namespace {

class PILDeadFuncElimination : public PILModuleTransform {
   void run() override {
      LLVM_DEBUG(llvm::dbgs() << "Running DeadFuncElimination\n");

      // The deserializer caches functions that it deserializes so that if it is
      // asked to deserialize that function again, it does not do extra work. This
      // causes the function's reference count to be incremented causing it to be
      // alive unnecessarily. We invalidate the PILLoaderCaches here so that we
      // can eliminate such functions.
      getModule()->invalidatePILLoaderCaches();

      DeadFunctionElimination deadFunctionElimination(getModule());
      deadFunctionElimination.eliminateFunctions(this);
   }
};

} // end anonymous namespace

PILTransform *polar::createDeadFunctionElimination() {
   return new PILDeadFuncElimination();
}

void polar::performPILDeadFunctionElimination(PILModule *M) {
   PILPassManager PM(M);
   llvm::SmallVector<PassKind, 1> Pass = {PassKind::DeadFunctionElimination};
   PM.executePassPipelinePlan(
      PILPassPipelinePlan::getPassPipelineForKinds(M->getOptions(), Pass));
}
