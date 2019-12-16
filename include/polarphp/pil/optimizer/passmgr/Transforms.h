//===--- Transforms.h - Swift Transformations  ------------------*- C++ -*-===//
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
#ifndef POLARPHP_PIL_OPTIMIZER_PASSMANAGER_TRANSFORMS_H
#define POLARPHP_PIL_OPTIMIZER_PASSMANAGER_TRANSFORMS_H

#include "polarphp/pil/lang/Notifications.h"
#include "polarphp/pil/optimizer/passmgr/PassManager.h"

namespace polar {

class PILModule;
class PILFunction;
class PrettyStackTracePILFunctionTransform;

/// The base class for all PIL-level transformations.
class PILTransform : public DeleteNotificationHandler {
public:
   /// The kind of transformation passes we use.
   enum class TransformKind {
      Function,
      Module,
   };

   /// Stores the kind of derived class.
   const TransformKind Kind;

protected:
   // The pass manager that runs this pass.
   PILPassManager* PM;

   // The pass kind (used by the pass manager).
   PassKind passKind = PassKind::invalidPassKind;

public:
   /// C'tor. \p K indicates the kind of derived class.
   PILTransform(PILTransform::TransformKind K) : Kind(K), PM(0) {}

   /// D'tor.
   virtual ~PILTransform() = default;

   /// Returns the kind of derived class.
   TransformKind getKind() const { return Kind; }

   /// Returns the pass kind.
   PassKind getPassKind() const {
      assert(passKind != PassKind::invalidPassKind);
      return passKind;
   }

   /// Sets the pass kind. This should only be done in the add-functions of
   /// the pass manager.
   void setPassKind(PassKind newPassKind) {
      assert(passKind == PassKind::invalidPassKind);
      passKind = newPassKind;
   }

   /// Inject the pass manager running this pass.
   void injectPassManager(PILPassManager *PMM) { PM = PMM; }

   PILPassManager *getPassManager() const { return PM; }

   irgen::IRGenModule *getIRGenModule() {
      auto *Mod = PM->getIRGenModule();
      assert(Mod && "Expecting a valid module");
      return Mod;
   }

   /// Get the transform's (command-line) tag.
   llvm::StringRef getTag() { return PassKindTag(getPassKind()); }

   /// Get the transform's name as a C++ identifier.
   llvm::StringRef getID() { return PassKindID(getPassKind()); }

protected:
   /// Searches for an analysis of type T in the list of registered
   /// analysis. If the analysis is not found, the program terminates.
   template<typename T>
   T* getAnalysis() { return PM->getAnalysis<T>(); }

   const PILOptions &getOptions() { return PM->getOptions(); }
};

/// A transformation that operates on functions.
class PILFunctionTransform : public PILTransform {
   friend class PrettyStackTracePILFunctionTransform;
   PILFunction *F;

public:
   /// C'tor.
   PILFunctionTransform() : PILTransform(TransformKind::Function), F(0) {}

   /// The entry point to the transformation.
   virtual void run() = 0;

   static bool classof(const PILTransform *S) {
      return S->getKind() == TransformKind::Function;
   }

   void injectFunction(PILFunction *Func) { F = Func; }

   /// Notify the pass manager of a function \p F that needs to be
   /// processed by the function passes and the analyses.
   ///
   /// If not null, the function \p DerivedFrom is the function from which \p F
   /// is derived. This is used to limit the number of new functions which are
   /// derived from a common base function, e.g. due to specialization.
   /// The number should be small anyway, but bugs in optimizations could cause
   /// an infinite loop in the passmanager.
   void addFunctionToPassManagerWorklist(PILFunction *F,
                                         PILFunction *DerivedFrom) {
      PM->addFunctionToWorklist(F, DerivedFrom);
   }

   /// Reoptimize the current function by restarting the pass
   /// pipeline on it.
   void restartPassPipeline() { PM->restartWithCurrentFunction(this); }

   PILFunction *getFunction() { return F; }

   void invalidateAnalysis(PILAnalysis::InvalidationKind K) {
      PM->invalidateAnalysis(F, K);
   }
};

/// A transformation that operates on modules.
class PILModuleTransform : public PILTransform {
   PILModule *M;

public:
   /// C'tor.
   PILModuleTransform() : PILTransform(TransformKind::Module), M(0) {}

   /// The entry point to the transformation.
   virtual void run() = 0;

   static bool classof(const PILTransform *S) {
      return S->getKind() == TransformKind::Module;
   }

   void injectModule(PILModule *Mod) { M = Mod; }

   PILModule *getModule() { return M; }

   /// Invalidate all analysis data for the whole module.
   void invalidateAll() {
      PM->invalidateAllAnalysis();
   }

   /// Invalidate only the function \p F, using invalidation information \p K.
   void invalidateAnalysis(PILFunction *F, PILAnalysis::InvalidationKind K) {
      PM->invalidateAnalysis(F, K);
   }

   /// Invalidate the analysis data for witness- and vtables.
   void invalidateFunctionTables() {
      PM->invalidateFunctionTables();
   }

   /// Inform the pass manager that we are going to delete a function.
   void notifyWillDeleteFunction(PILFunction *F) {
      PM->notifyWillDeleteFunction(F);
   }
};
} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_PASSMANAGER_TRANSFORMS_H
