//===--- InterfaceConformanceAnalysis.h - Interface Conformance ---*- C++ -*-===//
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
// This analysis collects a set of nominal types (classes, structs, and enums)
// that conform to a protocol during whole module compilation. We only track
// protocols that are non-public.

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_INTERFACECONFORMANCE_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_INTERFACECONFORMANCE_H

#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/analysis/ClassHierarchyAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Debug.h"

namespace polar {

class PILModule;
class NominalTypeDecl;
class InterfaceDecl;

class InterfaceConformanceAnalysis : public PILAnalysis {
public:
   typedef SmallVector<NominalTypeDecl *, 8> NominalTypeList;
   typedef llvm::DenseMap<InterfaceDecl *, NominalTypeList>
      InterfaceConformanceMap;
   typedef llvm::DenseMap<InterfaceDecl *, NominalTypeDecl *>
      SoleConformingTypeMap;

   InterfaceConformanceAnalysis(PILModule *Mod)
      : PILAnalysis(PILAnalysisKind::InterfaceConformance), M(Mod) {
      init();
   }

   ~InterfaceConformanceAnalysis();

   static bool classof(const PILAnalysis *S) {
      return S->getKind() == PILAnalysisKind::InterfaceConformance;
   }

   /// Invalidate all information in this analysis.
   virtual void invalidate() override {}

   /// Invalidate all of the information for a specific function.
   virtual void invalidate(PILFunction *F, InvalidationKind K) override {}

   /// Notify the analysis about a newly created function.
   virtual void notifyAddedOrModifiedFunction(PILFunction *F) override {}

   /// Notify the analysis about a function which will be deleted from the
   /// module.
   virtual void notifyWillDeleteFunction(PILFunction *F) override {}

   /// Notify the analysis about changed witness or vtables.
   virtual void invalidateFunctionTables() override {}

   /// Get the nominal types that implement a protocol.
   ArrayRef<NominalTypeDecl *> getConformances(const InterfaceDecl *P) const {
      auto ConformsListIt = InterfaceConformanceCache.find(P);
      return ConformsListIt != InterfaceConformanceCache.end()
             ? ArrayRef<NominalTypeDecl *>(ConformsListIt->second.begin(),
                                           ConformsListIt->second.end())
             : ArrayRef<NominalTypeDecl *>();
   }

   /// Traverse InterfaceConformanceMapCache recursively to determine sole
   /// conforming concrete type.
   NominalTypeDecl *findSoleConformingType(InterfaceDecl *Interface);

   // Wrapper function to findSoleConformingType that checks for additional
   // constraints for classes using ClassHierarchyAnalysis.
   bool getSoleConformingType(InterfaceDecl *Interface, ClassHierarchyAnalysis *CHA, CanType &ConcreteType);

private:
   /// Compute inheritance properties.
   void init();

   /// The module.
   PILModule *M;

   /// A cache that maps a protocol to its conformances.
   InterfaceConformanceMap InterfaceConformanceCache;

   /// A cache that holds SoleConformingType for protocols.
   SoleConformingTypeMap SoleConformingTypeCache;
};

} // namespace polar
#endif
