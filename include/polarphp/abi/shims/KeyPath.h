//===--- KeyPath.h - ABI constants for key path objects ---------*- C++ -*-===//
//
// This source file is part of the swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of swift project authors
//
//===----------------------------------------------------------------------===//
//
//  Constants used in the layout of key path objects.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_ABI_SHIMS_KEYPATH_H
#define POLARPHP_ABI_SHIMS_KEYPATH_H

#include "polarphp/abi/shims/PhpStdint.h"

#ifdef __cplusplus
namespace polar {
extern "C" {
#endif

// Bitfields for the key path buffer header.

static const __polarphp_uint32_t _PolarphpKeyPathBufferHeader_SizeMask
  = 0x00FFFFFFU;
static const __polarphp_uint32_t _PolarphpKeyPathBufferHeader_TrivialFlag
  = 0x80000000U;
static const __polarphp_uint32_t _PolarphpKeyPathBufferHeader_HasReferencePrefixFlag
  = 0x40000000U;
static const __polarphp_uint32_t _PolarphpKeyPathBufferHeader_ReservedMask
  = 0x3F000000U;

// Bitfields for a key path component header.

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_PayloadMask
  = 0x00FFFFFFU;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_DiscriminatorMask
  = 0x7F000000U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_DiscriminatorShift
  = 24;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_StructTag
  = 1;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedTag
  = 2;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ClassTag
  = 3;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_OptionalTag
  = 4;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ExternalTag
  = 0;

static const __polarphp_uint32_t
_PolarphpKeyPathComponentHeader_TrivialPropertyDescriptorMarker = 0U;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_StoredOffsetPayloadMask
  = 0x007FFFFFU;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_MaximumOffsetPayload
  = 0x007FFFFCU;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_UnresolvedIndirectOffsetPayload
  = 0x007FFFFDU;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_UnresolvedFieldOffsetPayload
  = 0x007FFFFEU;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_OutOfLineOffsetPayload
  = 0x007FFFFFU;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_StoredMutableFlag
  = 0x00800000U;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_OptionalChainPayload
  = 0;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_OptionalWrapPayload
  = 1;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_OptionalForcePayload
  = 2;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_EndOfReferencePrefixFlag
  = 0x80000000U;

static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedMutatingFlag
  = 0x00800000U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedSettableFlag
  = 0x00400000U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedIDByStoredPropertyFlag
  = 0x00200000U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedIDByVTableOffsetFlag
  = 0x00100000U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedHasArgumentsFlag
  = 0x00080000U;
// Not ABI, used internally by key path runtime implementation
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedInstantiatedFromExternalWithArgumentsFlag
  = 0x00000010U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedIDResolutionMask
  = 0x0000000FU;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedIDResolved
  = 0x00000000U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedIDUnresolvedIndirectPointer
  = 0x00000002U;
static const __polarphp_uint32_t _PolarphpKeyPathComponentHeader_ComputedIDUnresolvedFunctionCall
  = 0x00000001U;

extern const void *_Nonnull (polarphp_keyPathGenericWitnessTable[]);

static inline const void *_Nonnull __polarphp_keyPathGenericWitnessTable_addr(void) {
  return polarphp_keyPathGenericWitnessTable;
}

#ifdef __cplusplus
} // extern "C"
} // namespace polar
#endif

#endif // POLARPHP_ABI_SHIMS_KEYPATH_H
