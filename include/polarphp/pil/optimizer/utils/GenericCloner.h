//===--- GenericCloner.h - Specializes generic functions  -------*- C++ -*-===//
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
//
// This contains the definition of a cloner class for creating specialized
// versions of generic functions by substituting concrete types.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_GENERICCLONER_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_GENERICCLONER_H

#include "polarphp/ast/Type.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/TypeSubstCloner.h"
#include "polarphp/pil/optimizer/utils/BasicBlockOptUtils.h"
#include "polarphp/pil/optimizer/utils/Generics.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include <functional>

namespace polar {

class GenericCloner
  : public TypeSubstCloner<GenericCloner, PILOptFunctionBuilder> {
  using SuperTy = TypeSubstCloner<GenericCloner, PILOptFunctionBuilder>;

  PILOptFunctionBuilder &FuncBuilder;
  IsSerialized_t Serialized;
  const ReabstractionInfo &ReInfo;
  CloneCollector::CallbackType Callback;
  llvm::SmallDenseMap<const PILDebugScope *, const PILDebugScope *, 8>
      RemappedScopeCache;

  llvm::SmallVector<AllocStackInst *, 8> AllocStacks;
  AllocStackInst *ReturnValueAddr = nullptr;

public:
  friend class PILCloner<GenericCloner>;

  GenericCloner(PILOptFunctionBuilder &FuncBuilder,
                PILFunction *F,
                const ReabstractionInfo &ReInfo,
                SubstitutionMap ParamSubs,
                StringRef NewName,
                CloneCollector::CallbackType Callback)
    : SuperTy(*initCloned(FuncBuilder, F, ReInfo, NewName), *F,
	      ParamSubs), FuncBuilder(FuncBuilder), ReInfo(ReInfo), Callback(Callback) {
    assert(F->getDebugScope()->Parent != getCloned()->getDebugScope()->Parent);
  }
  /// Clone and remap the types in \p F according to the substitution
  /// list in \p Subs. Parameters are re-abstracted (changed from indirect to
  /// direct) according to \p ReInfo.
  static PILFunction *
  cloneFunction(PILOptFunctionBuilder &FuncBuilder,
                PILFunction *F,
                const ReabstractionInfo &ReInfo,
                SubstitutionMap ParamSubs,
                StringRef NewName,
                CloneCollector::CallbackType Callback =nullptr) {
    // Clone and specialize the function.
    GenericCloner SC(FuncBuilder, F, ReInfo, ParamSubs,
                     NewName, Callback);
    SC.populateCloned();
    return SC.getCloned();
  }

  void fixUp(PILFunction *calleeFunction);

protected:
  void visitTerminator(PILBasicBlock *BB);

  // FIXME: We intentionally call PILClonerWithScopes here to ensure
  //        the debug scopes are set correctly for cloned
  //        functions. TypeSubstCloner, PILClonerWithScopes, and
  //        PILCloner desperately need refactoring and/or combining so
  //        that the obviously right things are happening for cloning
  //        vs. inlining.
  void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
    // Call client-supplied callback function.
    if (Callback)
      Callback(Orig, Cloned);

    PILClonerWithScopes<GenericCloner>::postProcess(Orig, Cloned);
  }

private:
  static PILFunction *initCloned(PILOptFunctionBuilder &FuncBuilder,
                                 PILFunction *Orig,
                                 const ReabstractionInfo &ReInfo,
                                 StringRef NewName);
  /// Clone the body of the function into the empty function that was created
  /// by initCloned.
  void populateCloned();
  PILFunction *getCloned() { return &getBuilder().getFunction(); }

  const PILDebugScope *remapScope(const PILDebugScope *DS);

};

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_UTILS_GENERICCLONER_H
