//===--- KeyPath.h - ABI constants for key path objects ---------*- C++ -*-===//
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
//  Constants used in the layout of key path objects.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_ABI_KEYPATH_H
#define POLARPHP_ABI_KEYPATH_H

// We include the basic constants in a shim header so that it can be shared with
// the Swift implementation in the standard library.

#include <cstdint>
#include <cassert>
#include "polarphp/abi/shims/KeyPath.h"

namespace polar {

/// Header layout for a key path's data buffer header.
class KeyPathBufferHeader {
   uint32_t Data;

   constexpr KeyPathBufferHeader(unsigned Data) : Data(Data) {}

   static constexpr uint32_t validateSize(uint32_t size) {
      return assert(size <= _PolarphpKeyPathBufferHeader_SizeMask
                    && "size too big!"),
         size;
   }
public:
   constexpr KeyPathBufferHeader(unsigned size,
                                 bool trivialOrInstantiableInPlace,
                                 bool hasReferencePrefix)
      : Data((validateSize(size) & _PolarphpKeyPathBufferHeader_SizeMask)
             | (trivialOrInstantiableInPlace ? _PolarphpKeyPathBufferHeader_TrivialFlag : 0)
             | (hasReferencePrefix ? _PolarphpKeyPathBufferHeader_HasReferencePrefixFlag : 0))
   {
   }

   constexpr KeyPathBufferHeader withSize(unsigned size) const {
      return (Data & ~_PolarphpKeyPathBufferHeader_SizeMask) | validateSize(size);
   }

   constexpr KeyPathBufferHeader withIsTrivial(bool isTrivial) const {
      return (Data & ~_PolarphpKeyPathBufferHeader_TrivialFlag)
             | (isTrivial ? _PolarphpKeyPathBufferHeader_TrivialFlag : 0);
   }
   constexpr KeyPathBufferHeader withIsInstantiableInPlace(bool isTrivial) const {
      return (Data & ~_PolarphpKeyPathBufferHeader_TrivialFlag)
             | (isTrivial ? _PolarphpKeyPathBufferHeader_TrivialFlag : 0);
   }

   constexpr KeyPathBufferHeader withHasReferencePrefix(bool hasPrefix) const {
      return (Data & ~_PolarphpKeyPathBufferHeader_HasReferencePrefixFlag)
             | (hasPrefix ? _PolarphpKeyPathBufferHeader_HasReferencePrefixFlag : 0);
   }

   constexpr uint32_t getData() const {
      return Data;
   }
};

/// Header layout for a key path component's header.
class KeyPathComponentHeader {
   uint32_t Data;

   constexpr KeyPathComponentHeader(unsigned Data) : Data(Data) {}

   static constexpr uint32_t validateInlineOffset(uint32_t offset) {
      return assert(offsetCanBeInline(offset)
                    && "offset too big!"),
         offset;
   }

   static constexpr uint32_t isLetBit(bool isLet) {
      return isLet ? 0 : _PolarphpKeyPathComponentHeader_StoredMutableFlag;
   }

public:
   static constexpr bool offsetCanBeInline(unsigned offset) {
      return offset <= _PolarphpKeyPathComponentHeader_MaximumOffsetPayload;
   }

   constexpr static KeyPathComponentHeader
   forStructComponentWithInlineOffset(bool isLet,
                                      unsigned offset) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_StructTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | validateInlineOffset(offset)
         | isLetBit(isLet));
   }

   constexpr static KeyPathComponentHeader
   forStructComponentWithOutOfLineOffset(bool isLet) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_StructTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_OutOfLineOffsetPayload
         | isLetBit(isLet));
   }

   constexpr static KeyPathComponentHeader
   forStructComponentWithUnresolvedFieldOffset(bool isLet) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_StructTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_UnresolvedFieldOffsetPayload
         | isLetBit(isLet));
   }

   constexpr static KeyPathComponentHeader
   forClassComponentWithInlineOffset(bool isLet,
                                     unsigned offset) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_ClassTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | validateInlineOffset(offset)
         | isLetBit(isLet));
   }

   constexpr static KeyPathComponentHeader
   forClassComponentWithOutOfLineOffset(bool isLet) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_ClassTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_OutOfLineOffsetPayload
         | isLetBit(isLet));
   }

   constexpr static KeyPathComponentHeader
   forClassComponentWithUnresolvedFieldOffset(bool isLet) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_ClassTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_UnresolvedFieldOffsetPayload
         | isLetBit(isLet));
   }

   constexpr static KeyPathComponentHeader
   forClassComponentWithUnresolvedIndirectOffset(bool isLet) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_ClassTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_UnresolvedIndirectOffsetPayload
         | isLetBit(isLet));
   }

   constexpr static KeyPathComponentHeader
   forOptionalChain() {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_OptionalTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_OptionalChainPayload);
   }
   constexpr static KeyPathComponentHeader
   forOptionalWrap() {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_OptionalTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_OptionalWrapPayload);
   }
   constexpr static KeyPathComponentHeader
   forOptionalForce() {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_OptionalTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | _PolarphpKeyPathComponentHeader_OptionalForcePayload);
   }

   enum ComputedPropertyKind {
      GetOnly,
      SettableNonmutating,
      SettableMutating,
   };

   enum ComputedPropertyIDKind {
      Pointer,
      StoredPropertyIndex,
      VTableOffset,
   };

   enum ComputedPropertyIDResolution {
      Resolved,
      IndirectPointer,
      FunctionCall,
   };

   constexpr static KeyPathComponentHeader
   forComputedProperty(ComputedPropertyKind kind,
                       ComputedPropertyIDKind idKind,
                       bool hasArguments,
                       ComputedPropertyIDResolution resolution) {
      return KeyPathComponentHeader(
         (_PolarphpKeyPathComponentHeader_ComputedTag
            << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
         | (kind != GetOnly
            ? _PolarphpKeyPathComponentHeader_ComputedSettableFlag : 0)
         | (kind == SettableMutating
            ? _PolarphpKeyPathComponentHeader_ComputedMutatingFlag : 0)
         | (idKind == StoredPropertyIndex
            ? _PolarphpKeyPathComponentHeader_ComputedIDByStoredPropertyFlag : 0)
         | (idKind == VTableOffset
            ? _PolarphpKeyPathComponentHeader_ComputedIDByVTableOffsetFlag : 0)
         | (hasArguments ? _PolarphpKeyPathComponentHeader_ComputedHasArgumentsFlag : 0)
         | (resolution == Resolved ? _PolarphpKeyPathComponentHeader_ComputedIDResolved
                                   : resolution == IndirectPointer ? _PolarphpKeyPathComponentHeader_ComputedIDUnresolvedIndirectPointer
                                                                   : resolution == FunctionCall ? _PolarphpKeyPathComponentHeader_ComputedIDUnresolvedFunctionCall
                                                                                                : (assert(false && "invalid resolution"), 0)));
   }

   constexpr static KeyPathComponentHeader
   forExternalComponent(unsigned numSubstitutions) {
      return assert(numSubstitutions <
                    (1u << _PolarphpKeyPathComponentHeader_DiscriminatorShift) - 1u
                    && "too many substitutions"),
         KeyPathComponentHeader(
            (_PolarphpKeyPathComponentHeader_ExternalTag
               << _PolarphpKeyPathComponentHeader_DiscriminatorShift)
            | numSubstitutions);
   }

   constexpr uint32_t getData() const { return Data; }
};

} // polar

#endif // POLARPHP_ABI_KEYPATH_H
