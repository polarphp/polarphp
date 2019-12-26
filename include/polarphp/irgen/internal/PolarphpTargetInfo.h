//===--- PolarphpTargetInfo.h --------------------------------------*- C++ -*-===//
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
// This file declares the PolarphpTargetInfo abstract base class. This class
// provides an interface to target-dependent attributes of interest to Swift.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_POLARPHP_TARGETINFO_H
#define POLARPHP_IRGEN_INTERNAL_POLARPHP_TARGETINFO_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/ClusteredBitVector.h"
#include "llvm/ADT/Triple.h"
#include "polarphp/irgen/internal/IRGen.h"

namespace polar::irgen {

class IRGenModule;

class PolarphpTargetInfo {
   explicit PolarphpTargetInfo(llvm::Triple::ObjectFormatType outputObjectFormat,
                               unsigned numPointerBits);

public:

   /// Produces a PolarphpTargetInfo object appropriate to the target.
   static PolarphpTargetInfo get(IRGenModule &IGM);

   /// True if the ObjC runtime for the chosen platform supports tagged pointers.
   bool hasObjCTaggedPointers() const;

   /// True if the ObjC runtime for the chosen platform requires ISA masking.
   bool hasISAMasking() const {
      return ObjCUseISAMask;
   }

   /// True if the ObjC runtime for the chosen platform has opaque ISAs.  This
   /// means that even masking the ISA may not return a pointer value.  The ObjC
   /// runtime should be used for all accesses to get the ISA from a value.
   bool hasOpaqueISAs() const {
      return ObjCHasOpaqueISAs;
   }

   /// The target's object format type.
   llvm::Triple::ObjectFormatType OutputObjectFormat;

   /// The spare bit mask for pointers. Bits set in this mask are unused by
   /// pointers of any alignment.
   SpareBitVector PointerSpareBits;

   /// The spare bit mask for (ordinary C) thin function pointers.
   SpareBitVector FunctionPointerSpareBits;

   /// The reserved bit mask for Objective-C pointers. Pointer values with
   /// bits from this mask set are reserved by the ObjC runtime and cannot be
   /// used for Swift value layout when a reference type may reference ObjC
   /// objects.
   SpareBitVector ObjCPointerReservedBits;

   /// These bits, if set, indicate that a Builtin.BridgeObject value is holding
   /// an Objective-C object.
   SpareBitVector IsObjCPointerBit;


   /// The alignment of heap objects.  By default, assume pointer alignment.
   Alignment HeapObjectAlignment;

   /// The least integer value that can theoretically form a valid pointer.
   /// By default, assume that there's an entire page free.
   ///
   /// This excludes addresses in the null page(s) guaranteed to be
   /// unmapped by the platform.
   ///
   /// Changes to this must be kept in sync with swift/Runtime/Metadata.h.
   uint64_t LeastValidPointerValue;

   /// The maximum number of scalars that we allow to be returned directly.
   unsigned MaxScalarsForDirectResult = 3;

   /// Inline assembly to mark a call to objc_retainAutoreleasedReturnValue.
   llvm::StringRef ObjCRetainAutoreleasedReturnValueMarker;

   /// Some architectures have specialized objc_msgSend variants.
   bool ObjCUseStret = true;
   bool ObjCUseFPRet = false;
   bool ObjCUseFP2Ret = false;
   bool ObjCUseISAMask = false;
   bool ObjCHasOpaqueISAs = false;

   /// The value stored in a Builtin.once predicate to indicate that an
   /// initialization has already happened, if known.
   Optional<int64_t> OnceDonePredicateValue = None;

   /// True if `polarphp_retain` and `polarphp_release` are no-ops when passed
   /// "negative" pointer values.
   bool PolarphpRetainIgnoresNegativeValues = false;
};

} // polar::irgen

#endif // POLARPHP_IRGEN_INTERNAL_POLARPHP_TARGETINFO_H

