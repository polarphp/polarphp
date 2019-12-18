//===---- ExistentialSpecializerTransform.h - Existential Specializer -----===//
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
// This contains utilities for transforming existential args to generics.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_OPTIMIZER_INTERNAL_FUNCSIGNATURE_TRANSFORMS_EXISTENTIALTRANSFORM_H
#define POLARPHP_PIL_OPTIMIZER_INTERNAL_FUNCSIGNATURE_TRANSFORMS_EXISTENTIALTRANSFORM_H

#include "polarphp/pil/optimizer/internal/funcsignaturetransforms/FunctionSignatureOpts.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/optimizer/utils/Existential.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "polarphp/pil/optimizer/utils/PILOptFunctionBuilder.h"
#include "polarphp/pil/optimizer/utils/SpecializationMangler.h"
#include "llvm/ADT/SmallBitVector.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

namespace polar {

/// A descriptor to carry information from ExistentialTransform analysis
/// to transformation.
struct ExistentialTransformArgumentDescriptor {
   OpenedExistentialAccess AccessType;
   bool isConsumed;
};

/// ExistentialTransform creates a protocol constrained generic and a thunk.
class ExistentialTransform {
   /// Function Builder to create a new thunk.
   PILOptFunctionBuilder &FunctionBuilder;

   /// The original function to analyze and transform.
   PILFunction *F;

   /// The newly created inner function.
   PILFunction *NewF;

   /// The function signature mangler we are using.
   mangle::FunctionSignatureSpecializationMangler &Mangler;

   /// List of arguments and their descriptors to specialize
   llvm::SmallDenseMap<int, ExistentialTransformArgumentDescriptor>
      &ExistentialArgDescriptor;

   /// Argument to Generic Type Map for NewF.
   llvm::SmallDenseMap<int, GenericTypeParamType *> ArgToGenericTypeMap;

   /// Allocate the argument descriptors.
   llvm::SmallVector<ArgumentDescriptor, 4> &ArgumentDescList;

   /// Create the Devirtualized Inner Function.
   void createExistentialSpecializedFunction();

   /// Create new generic arguments from existential arguments.
   void
   convertExistentialArgTypesToGenericArgTypes(
      SmallVectorImpl<GenericTypeParamType *> &genericParams,
      SmallVectorImpl<Requirement> &requirements);

   /// Create a name for the inner function.
   std::string createExistentialSpecializedFunctionName();

   /// Create the new devirtualized protocol function signature.
   CanPILFunctionType createExistentialSpecializedFunctionType();

   /// Create the thunk.
   void populateThunkBody();

public:
   /// Constructor.
   ExistentialTransform(
      PILOptFunctionBuilder &FunctionBuilder, PILFunction *F,
      mangle::FunctionSignatureSpecializationMangler &Mangler,
      llvm::SmallVector<ArgumentDescriptor, 4> &ADL,
      llvm::SmallDenseMap<int, ExistentialTransformArgumentDescriptor>
      &ExistentialArgDescriptor)
      : FunctionBuilder(FunctionBuilder), F(F), NewF(nullptr), Mangler(Mangler),
        ExistentialArgDescriptor(ExistentialArgDescriptor),
        ArgumentDescList(ADL) {}

   /// Return the optimized iner function.
   PILFunction *getExistentialSpecializedFunction() { return NewF; }

   /// External function for the optimization.
   bool run() {
      createExistentialSpecializedFunction();
      return true;
   }
};

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_INTERNAL_FUNCSIGNATURE_TRANSFORMS_EXISTENTIALTRANSFORM_H
