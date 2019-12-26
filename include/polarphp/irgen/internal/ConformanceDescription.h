//===--- ConformanceDescription.h - Conformance record ----------*- C++ -*-===//
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
// This file defines the ConformanceDescription type, which records a
// conformance which needs to be emitted.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_CONFORMANCEDESCRIPTION_H
#define POLARPHP_IRGEN_INTERNAL_CONFORMANCEDESCRIPTION_H

namespace llvm {
class Constant;
}

namespace polar {

class PILWitnessTable;

namespace irgen {

/// The description of a protocol conformance, including its witness table
/// and any additional information needed to produce the protocol conformance
/// descriptor.
class ConformanceDescription {
public:
   /// The conformance itself.
   RootInterfaceConformance *conformance;

   /// The witness table.
   PILWitnessTable *wtable;

   /// The witness table pattern, which is also a complete witness table
   /// when \c requiresSpecialization is \c false.
   llvm::Constant *pattern;

   /// The size of the witness table.
   const uint16_t witnessTableSize;

   /// The private size of the witness table, allocated
   const uint16_t witnessTablePrivateSize;

   /// Whether this witness table requires runtime specialization.
   const unsigned requiresSpecialization : 1;

   /// The instantiation function, to be run at the end of witness table
   /// instantiation.
   llvm::Constant *instantiationFn = nullptr;

   /// The resilient witnesses, if any.
   SmallVector<llvm::Constant *, 4> resilientWitnesses;

   ConformanceDescription(RootInterfaceConformance *conformance,
                          PILWitnessTable *wtable,
                          llvm::Constant *pattern,
                          uint16_t witnessTableSize,
                          uint16_t witnessTablePrivateSize,
                          bool requiresSpecialization)
      : conformance(conformance), wtable(wtable), pattern(pattern),
        witnessTableSize(witnessTableSize),
        witnessTablePrivateSize(witnessTablePrivateSize),
        requiresSpecialization(requiresSpecialization)
   {
   }
};

} // end namespace irgen
} // end namespace polar

#endif // POLARPHP_IRGEN_INTERNAL_CONFORMANCEDESCRIPTION_H
