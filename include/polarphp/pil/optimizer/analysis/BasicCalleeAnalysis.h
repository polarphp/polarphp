//===- BasicCalleeAnalysis.h - Determine callees per call site --*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_BASICCALLEEANALYSIS_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_BASICCALLEEANALYSIS_H

#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILModule.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/Allocator.h"

namespace polar {

class ClassDecl;
class PILFunction;
class PILModule;
class PILWitnessTable;

/// CalleeList is a data structure representing the list of potential
/// callees at a particular apply site. It also has a query that
/// allows a client to determine whether the list is incomplete in the
/// sense that there may be unrepresented callees.
class CalleeList {
   llvm::TinyPtrVector<PILFunction *> CalleeFunctions;
   bool IsIncomplete;

public:
   /// Constructor for when we know nothing about the callees and must
   /// assume the worst.
   CalleeList() : IsIncomplete(true) {}

   /// Constructor for the case where we know an apply can target only
   /// a single function.
   CalleeList(PILFunction *F) : CalleeFunctions(F), IsIncomplete(false) {}

   /// Constructor for arbitrary lists of callees.
   CalleeList(llvm::SmallVectorImpl<PILFunction *> &List, bool IsIncomplete)
      : CalleeFunctions(llvm::makeArrayRef(List.begin(), List.end())),
        IsIncomplete(IsIncomplete) {}

   POLAR_DEBUG_DUMP;

   void print(llvm::raw_ostream &os) const;

   /// Return an iterator for the beginning of the list.
   ArrayRef<PILFunction *>::iterator begin() const {
      return CalleeFunctions.begin();
   }

   /// Return an iterator for the end of the list.
   ArrayRef<PILFunction *>::iterator end() const {
      return CalleeFunctions.end();
   }

   bool isIncomplete() const { return IsIncomplete; }

   /// Returns true if all callees are known and not external.
   bool allCalleesVisible() const;
};

/// CalleeCache is a helper class that builds lists of potential
/// callees for class and witness method applications, and provides an
/// interface for retrieving a (possibly incomplete) CalleeList for
/// any function application site (including those that are simple
/// function_ref, thin_to_thick, or partial_apply callees).
class CalleeCache {
   using Callees = llvm::SmallVector<PILFunction *, 16>;
   using CalleesAndCanCallUnknown = llvm::PointerIntPair<Callees *, 1>;
   using CacheType = llvm::DenseMap<PILDeclRef, CalleesAndCanCallUnknown>;

   PILModule &M;

   // Allocator for the SmallVectors that we will be allocating.
   llvm::SpecificBumpPtrAllocator<Callees> Allocator;

   // The cache of precomputed callee lists for function decls appearing
   // in class virtual dispatch tables and witness tables.
   CacheType TheCache;

public:
   CalleeCache(PILModule &M) : M(M) {
      computeMethodCallees();
      sortAndUniqueCallees();
   }

   ~CalleeCache() {
      Allocator.DestroyAll();
   }

   /// Return the list of callees that can potentially be called at the
   /// given apply site.
   CalleeList getCalleeList(FullApplySite FAS) const;
   /// Return the list of callees that can potentially be called at the
   /// given instruction. E.g. it could be destructors.
   CalleeList getCalleeList(PILInstruction *I) const;

   CalleeList getCalleeList(PILDeclRef Decl) const;

private:
   void enumerateFunctionsInModule();
   void sortAndUniqueCallees();
   CalleesAndCanCallUnknown &getOrCreateCalleesForMethod(PILDeclRef Decl);
   void computeClassMethodCallees();
   void computeWitnessMethodCalleesForWitnessTable(PILWitnessTable &WT);
   void computeMethodCallees();
   PILFunction *getSingleCalleeForWitnessMethod(WitnessMethodInst *WMI) const;
   CalleeList getCalleeList(WitnessMethodInst *WMI) const;
   CalleeList getCalleeList(ClassMethodInst *CMI) const;
   CalleeList getCalleeListForCalleeKind(PILValue Callee) const;
};

class BasicCalleeAnalysis : public PILAnalysis {
   PILModule &M;
   std::unique_ptr<CalleeCache> Cache;

public:
   BasicCalleeAnalysis(PILModule *M)
      : PILAnalysis(PILAnalysisKind::BasicCallee), M(*M), Cache(nullptr) {}

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::BasicCallee;
   }

   /// Invalidate all information in this analysis.
   virtual void invalidate() override {
      Cache.reset();
   }

   /// Invalidate all of the information for a specific function.
   virtual void invalidate(PILFunction *F, InvalidationKind K) override {
      // No invalidation needed because the analysis does not cache anything
      // per call-site in functions.
   }

   /// Notify the analysis about a newly created function.
   virtual void notifyAddedOrModifiedFunction(PILFunction *F) override {
      // Nothing to be done because the analysis does not cache anything
      // per call-site in functions.
   }

   /// Notify the analysis about a function which will be deleted from the
   /// module.
   virtual void notifyWillDeleteFunction(PILFunction *F) override {
      // No invalidation needed because the analysis does not cache anything per
      // call-site in functions.
   }

   /// Notify the analysis about changed witness or vtables.
   virtual void invalidateFunctionTables() override {
      Cache.reset();
   }

   POLAR_DEBUG_DUMP;

   void print(llvm::raw_ostream &os) const;

   void updateCache() {
      if (!Cache)
         Cache = std::make_unique<CalleeCache>(M);
   }

   CalleeList getCalleeList(FullApplySite FAS) {
      updateCache();
      return Cache->getCalleeList(FAS);
   }

   CalleeList getCalleeList(PILInstruction *I) {
      updateCache();
      return Cache->getCalleeList(I);
   }
};

} // end namespace polar

#endif
