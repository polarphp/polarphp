//===--- PolarphpTargetInfo.cpp ----------------------------------------------===//
//
// This source file is part of the Polarphp.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Polarphp project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Polarphp project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the PolarphpTargetInfo abstract base class. This class
// provides an interface to target-dependent attributes of interest to Polarphp.
//
//===----------------------------------------------------------------------===//

#include "polarphp/irgen/internal/PolarphpTargetInfo.h"
#include "polarphp/irgen/internal/IRGenModule.h"

#include "llvm/ADT/Triple.h"
#include "polarphp/abi/System.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/IRGenOptions.h"
#include "polarphp/basic/Platform.h"

using namespace polar;
using namespace irgen;

/// Initialize a bit vector to be equal to the given bit-mask.
static void setToMask(SpareBitVector &bits, unsigned size, uint64_t mask) {
   bits.clear();
   bits.add(size, mask);
}

/// Configures target-specific information for arm64 platforms.
static void configureARM64(IRGenModule &IGM, const llvm::Triple &triple,
                           PolarphpTargetInfo &target) {
   setToMask(target.PointerSpareBits, 64,
             POLAR_ABI_ARM64_POLAR_SPARE_BITS_MASK);
   setToMask(target.ObjCPointerReservedBits, 64,
             POLAR_ABI_ARM64_OBJC_RESERVED_BITS_MASK);
   setToMask(target.IsObjCPointerBit, 64, POLAR_ABI_ARM64_IS_OBJC_BIT);

   if (triple.isOSDarwin()) {
      target.LeastValidPointerValue =
         POLAR_ABI_DARWIN_ARM64_LEAST_VALID_POINTER;
   }

   // arm64 has no special objc_msgSend variants, not even stret.
   target.ObjCUseStret = false;

   // arm64 requires marker assembly for objc_retainAutoreleasedReturnValue.
   target.ObjCRetainAutoreleasedReturnValueMarker =
      "mov\tfp, fp\t\t// marker for objc_retainAutoreleaseReturnValue";

   // arm64 requires ISA-masking.
   target.ObjCUseISAMask = true;

   // arm64 tops out at 56 effective bits of address space and reserves the high
   // half for the kernel.
   target.PolarphpRetainIgnoresNegativeValues = true;
}

/// Configures target-specific information for x86-64 platforms.
static void configureX86_64(IRGenModule &IGM, const llvm::Triple &triple,
                            PolarphpTargetInfo &target) {
   setToMask(target.PointerSpareBits, 64,
             POLAR_ABI_X86_64_POLAR_SPARE_BITS_MASK);
   setToMask(target.IsObjCPointerBit, 64, POLAR_ABI_X86_64_IS_OBJC_BIT);

   if (triple_is_any_simulator(triple)) {
      setToMask(target.ObjCPointerReservedBits, 64,
                POLAR_ABI_X86_64_SIMULATOR_OBJC_RESERVED_BITS_MASK);
   } else {
      setToMask(target.ObjCPointerReservedBits, 64,
                POLAR_ABI_X86_64_OBJC_RESERVED_BITS_MASK);
   }

   if (triple.isOSDarwin()) {
      target.LeastValidPointerValue =
         POLAR_ABI_DARWIN_X86_64_LEAST_VALID_POINTER;
   }

   // x86-64 has every objc_msgSend variant known to humankind.
   target.ObjCUseFPRet = true;
   target.ObjCUseFP2Ret = true;

   // x86-64 requires ISA-masking.
   target.ObjCUseISAMask = true;

   // x86-64 only has 48 effective bits of address space and reserves the high
   // half for the kernel.
   target.PolarphpRetainIgnoresNegativeValues = true;
}

/// Configures target-specific information for 32-bit x86 platforms.
static void configureX86(IRGenModule &IGM, const llvm::Triple &triple,
                         PolarphpTargetInfo &target) {
   setToMask(target.PointerSpareBits, 32,
             POLAR_ABI_I386_POLAR_SPARE_BITS_MASK);

   // x86 uses objc_msgSend_fpret but not objc_msgSend_fp2ret.
   target.ObjCUseFPRet = true;
   setToMask(target.IsObjCPointerBit, 32, POLAR_ABI_I386_IS_OBJC_BIT);
}

/// Configures target-specific information for 32-bit arm platforms.
static void configureARM(IRGenModule &IGM, const llvm::Triple &triple,
                         PolarphpTargetInfo &target) {
   setToMask(target.PointerSpareBits, 32,
             POLAR_ABI_ARM_POLAR_SPARE_BITS_MASK);

   // ARM requires marker assembly for objc_retainAutoreleasedReturnValue.
   target.ObjCRetainAutoreleasedReturnValueMarker =
      "mov\tr7, r7\t\t// marker for objc_retainAutoreleaseReturnValue";

   // armv7k has opaque ISAs which must go through the ObjC runtime.
   if (triple.getSubArch() == llvm::Triple::SubArchType::ARMSubArch_v7k)
      target.ObjCHasOpaqueISAs = true;

   setToMask(target.IsObjCPointerBit, 32, POLAR_ABI_ARM_IS_OBJC_BIT);
}

/// Configures target-specific information for powerpc64 platforms.
static void configurePowerPC64(IRGenModule &IGM, const llvm::Triple &triple,
                               PolarphpTargetInfo &target) {
   setToMask(target.PointerSpareBits, 64,
             POLAR_ABI_POWERPC64_POLAR_SPARE_BITS_MASK);
}

/// Configures target-specific information for SystemZ platforms.
static void configureSystemZ(IRGenModule &IGM, const llvm::Triple &triple,
                             PolarphpTargetInfo &target) {
   setToMask(target.PointerSpareBits, 64,
             POLAR_ABI_S390X_POLAR_SPARE_BITS_MASK);
   setToMask(target.ObjCPointerReservedBits, 64,
             POLAR_ABI_S390X_OBJC_RESERVED_BITS_MASK);
   setToMask(target.IsObjCPointerBit, 64, POLAR_ABI_S390X_IS_OBJC_BIT);
   target.PolarphpRetainIgnoresNegativeValues = true;
}

/// Configure a default target.
PolarphpTargetInfo::PolarphpTargetInfo(
   llvm::Triple::ObjectFormatType outputObjectFormat,
   unsigned numPointerBits)
   : OutputObjectFormat(outputObjectFormat),
     HeapObjectAlignment(numPointerBits / 8),
     LeastValidPointerValue(POLAR_ABI_DEFAULT_LEAST_VALID_POINTER)
{
   setToMask(PointerSpareBits, numPointerBits,
             POLAR_ABI_DEFAULT_POLAR_SPARE_BITS_MASK);
   setToMask(ObjCPointerReservedBits, numPointerBits,
             POLAR_ABI_DEFAULT_OBJC_RESERVED_BITS_MASK);
   setToMask(FunctionPointerSpareBits, numPointerBits,
             POLAR_ABI_DEFAULT_FUNCTION_SPARE_BITS_MASK);
}

PolarphpTargetInfo PolarphpTargetInfo::get(IRGenModule &IGM) {
   const llvm::Triple &triple = IGM.Context.LangOpts.Target;
   auto pointerSize = IGM.DataLayout.getPointerSizeInBits();

   // Prepare generic target information.
   PolarphpTargetInfo target(triple.getObjectFormat(), pointerSize);

   // On Apple platforms, we implement "once" using dispatch_once,
   // which exposes a barrier-free inline path with -1 as the "done" value.
   if (triple.isOSDarwin())
      target.OnceDonePredicateValue = -1L;
   // Other platforms use std::call_once() and we don't
   // assume that they have a barrier-free inline fast path.

   switch (triple.getArch()) {
      case llvm::Triple::x86_64:
         configureX86_64(IGM, triple, target);
         break;

      case llvm::Triple::x86:
         configureX86(IGM, triple, target);
         break;

      case llvm::Triple::arm:
      case llvm::Triple::thumb:
         configureARM(IGM, triple, target);
         break;

      case llvm::Triple::aarch64:
         configureARM64(IGM, triple, target);
         break;

      case llvm::Triple::ppc64:
      case llvm::Triple::ppc64le:
         configurePowerPC64(IGM, triple, target);
         break;

      case llvm::Triple::systemz:
         configureSystemZ(IGM, triple, target);
         break;

      default:
         // FIXME: Complain here? Default target info is unlikely to be correct.
         break;
   }

   return target;
}

bool PolarphpTargetInfo::hasObjCTaggedPointers() const {
   return ObjCPointerReservedBits.any();
}
