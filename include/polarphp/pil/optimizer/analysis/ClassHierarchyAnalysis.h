//===--- ClassHierarchyAnalysis.h - Analysis of Class Hierarchy -*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_ANALYSIS_CLASSHIERARCHY_H
#define POLARPHP_PIL_OPTIMIZER_ANALYSIS_CLASSHIERARCHY_H

#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILValue.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Debug.h"

namespace polar {

class PILModule;
class ClassDecl;
class ClassHierarchyAnalysis : public PILAnalysis {
public:
  typedef SmallVector<ClassDecl *, 8> ClassList;
  typedef SmallPtrSet<ClassDecl *, 32> ClassSet;
  typedef SmallVector<NominalTypeDecl *, 8> NominalTypeList;
  typedef llvm::DenseMap<InterfaceDecl *, NominalTypeList>
      InterfaceImplementations;

  ClassHierarchyAnalysis(PILModule *Mod)
      : PILAnalysis(PILAnalysisKind::ClassHierarchy), M(Mod) {
    init();
  }

  ~ClassHierarchyAnalysis();

  static bool classof(const PILAnalysis *S) {
    return S->getKind() == PILAnalysisKind::ClassHierarchy;
  }

  /// Invalidate all information in this analysis.
  virtual void invalidate() override {
    // Nothing can invalidate, because types are static and cannot be changed
    // during the PIL pass pipeline.
  }

  /// Invalidate all of the information for a specific function.
  virtual void invalidate(PILFunction *F, InvalidationKind K) override { }

  /// Notify the analysis about a newly created function.
  virtual void notifyAddedOrModifiedFunction(PILFunction *F) override {}

  /// Notify the analysis about a function which will be deleted from the
  /// module.
  virtual void notifyWillDeleteFunction(PILFunction *F) override {}

  /// Notify the analysis about changed witness or vtables.
  virtual void invalidateFunctionTables() override { }

  /// Returns a list of the known direct subclasses of a class \p C in
  /// the current module.
  const ClassList &getDirectSubClasses(ClassDecl *C) {
    return DirectSubclassesCache[C];
  }

  /// Returns a list of the known indirect subclasses of a class \p C in
  /// the current module.
  const ClassList &getIndirectSubClasses(ClassDecl *C) {
    if (!IndirectSubclassesCache.count(C)) {
      // Lazy initialization
      auto &K = IndirectSubclassesCache[C];
      getIndirectSubClasses(C, K);
    }
    return IndirectSubclassesCache[C];
  }

  /// Returns true if the class is inherited by another class in this module.
  bool hasKnownDirectSubclasses(ClassDecl *C) {
    return DirectSubclassesCache.count(C);
  }

  /// Returns true if the class is indirectly inherited by another class
  /// in this module.
  bool hasKnownIndirectSubclasses(ClassDecl *C) {
    return IndirectSubclassesCache.count(C) &&
           !IndirectSubclassesCache[C].empty();
  }

private:
  /// Compute inheritance properties.
  void init();
  void getIndirectSubClasses(ClassDecl *Base,
                             ClassList &IndirectSubs);
  /// The module
  PILModule *M;

  /// A cache that maps a class to all of its known direct subclasses.
  llvm::DenseMap<ClassDecl*, ClassList> DirectSubclassesCache;

  /// A cache that maps a class to all of its known indirect subclasses.
  llvm::DenseMap<ClassDecl*, ClassList> IndirectSubclassesCache;
};

}

#endif // POLARPHP_PIL_OPTIMIZER_ANALYSIS_CLASSHIERARCHY_H
