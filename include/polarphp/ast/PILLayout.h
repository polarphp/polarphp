//===--- PILLayout.h - Defines PIL-level aggregate layouts ------*- C++ -*-===//
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
// This file defines classes that describe the physical layout of nominal
// types in PIL, including structs, classes, and boxes. This is distinct from
// the AST-level layout for several reasons:
// - It avoids redundant work lowering the layout of aggregates from the AST.
// - It allows optimizations to manipulate the layout of aggregates without
//   requiring changes to the AST. For instance, optimizations can eliminate
//   dead fields from instances or turn invariant fields into global variables.
// - It allows for PIL-only aggregates to exist, such as boxes.
// - It improves the robustness of code in the face of resilience. A resilient
//   type can be modeled in PIL as not having a layout at all, preventing the
//   inappropriate use of fragile projection and injection operations on the
//   type.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PIL_LAYOUT_H
#define POLARPHP_AST_PIL_LAYOUT_H

#include "llvm/ADT/PointerIntPair.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/Type.h"

namespace polar {

class PILType;

/// A field of a PIL aggregate layout.
class PILField final {
  enum : unsigned {
    IsMutable = 0x1,
  };

  static constexpr const unsigned NumFlags = 1;

  llvm::PointerIntPair<CanType, NumFlags, unsigned> LoweredTypeAndFlags;

  static unsigned getFlagsValue(bool Mutable) {
    unsigned flags = 0;
    if (Mutable) flags |= IsMutable;

    assert(flags >> NumFlags == 0
           && "more flags than flag bits?!");
    return flags;
  }

public:
  PILField(CanType LoweredType, bool Mutable)
    : LoweredTypeAndFlags(LoweredType, getFlagsValue(Mutable))
  {}

  /// Get the lowered type of the field in the aggregate.
  ///
  /// This must be a lowered PIL type. If the containing aggregate is generic,
  /// then this type specifies the abstraction pattern at which values stored
  /// in this aggregate should be lowered.
  CanType getLoweredType() const { return LoweredTypeAndFlags.getPointer(); }

  PILType getAddressType() const; // In PILType.h
  PILType getObjectType() const; // In PILType.h

  /// True if this field is mutable inside its aggregate.
  ///
  /// This is only effectively a constraint on shared mutable reference types,
  /// such as classes and boxes. A value type or uniquely-owned immutable
  /// reference can always be mutated, since doing so is equivalent to
  /// destroying the old value and emplacing a new value with the modified
  /// field in the same place.
  bool isMutable() const { return LoweredTypeAndFlags.getInt() & IsMutable; }
};

/// A layout.
class PILLayout final : public llvm::FoldingSetNode,
                        private llvm::TrailingObjects<PILLayout, PILField>
{
  friend TrailingObjects;

  enum : unsigned {
    IsMutable = 0x1,
  };

  static constexpr const unsigned NumFlags = 1;

  static unsigned getFlagsValue(bool Mutable) {
    unsigned flags = 0;
    if (Mutable)
      flags |= IsMutable;

    assert(flags >> NumFlags == 0
           && "more flags than flag bits?!");
    return flags;
  }

  llvm::PointerIntPair<CanGenericSignature, NumFlags, unsigned>
    GenericSigAndFlags;

  unsigned NumFields;

  PILLayout(CanGenericSignature Signature,
            ArrayRef<PILField> Fields);

  PILLayout(const PILLayout &) = delete;
  PILLayout &operator=(const PILLayout &) = delete;
public:
  /// Get or create a layout.
  static PILLayout *get(AstContext &C,
                        CanGenericSignature Generics,
                        ArrayRef<PILField> Fields);

  /// Get the generic signature in which this layout exists.
  CanGenericSignature getGenericSignature() const {
    return GenericSigAndFlags.getPointer();
  }

  /// True if the layout contains any mutable fields.
  bool isMutable() const {
    return GenericSigAndFlags.getInt() & IsMutable;
  }

  /// Get the fields inside the layout.
  ArrayRef<PILField> getFields() const {
    return llvm::makeArrayRef(getTrailingObjects<PILField>(), NumFields);
  }

  /// Produce a profile of this layout, for use in a folding set.
  static void Profile(llvm::FoldingSetNodeID &id,
                      CanGenericSignature Generics,
                      ArrayRef<PILField> Fields);

  /// Produce a profile of this locator, for use in a folding set.
  void Profile(llvm::FoldingSetNodeID &id) {
    Profile(id, getGenericSignature(), getFields());
  }
};

} // end namespace polar

#endif // POLARPHP_AST_PIL_LAYOUT_H
