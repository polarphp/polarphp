//===--- ConcreteDeclRef.h - Reference to a concrete decl -------*- C++ -*-===//
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
// This file defines the ConcreteDeclRef class, which provides a reference to
// a declaration that is potentially specialized.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_CONCRETEDECLREF_H
#define POLARPHP_AST_CONCRETEDECLREF_H

#include "polarphp/basic/Debug.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/ast/TypeAlignments.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/Compiler.h"
#include <cstring>

namespace polar{
class SourceManager;
}

namespace polar {

class AstContext;
class ValueDecl;

using polar::SourceManager;

/// A reference to a concrete representation of a particular declaration,
/// providing substitutions for all type parameters of the original,
/// underlying declaration.
class ConcreteDeclRef
{
  /// The declaration.
  ValueDecl *decl = nullptr;

  /// The substitutions.
  SubstitutionMap substitutions;

public:
  /// Create an empty declaration reference.
  ConcreteDeclRef() { }

  /// Construct a reference to the given value.
  ConcreteDeclRef(ValueDecl *decl) : decl(decl) { }

  /// Construct a reference to the given value, specialized with the given
  /// substitutions.
  ///
  /// \param decl The declaration to which this reference refers, which will
  /// be specialized by applying the given substitutions.
  ///
  /// \param substitutions The complete set of substitutions to apply to the
  /// given declaration.
  ConcreteDeclRef(ValueDecl *decl, SubstitutionMap substitutions)
    : decl(decl), substitutions(substitutions) { }

  /// Determine whether this declaration reference refers to anything.
  explicit operator bool() const { return decl != nullptr; }

  /// Retrieve the declarations to which this reference refers.
  ValueDecl *getDecl() const { return decl; }

  /// Retrieve a reference to the declaration this one overrides.
  ConcreteDeclRef getOverriddenDecl() const;

  /// Determine whether this reference specializes the declaration to which
  /// it refers.
  bool isSpecialized() const { return !substitutions.empty(); }

  /// For a specialized reference, return the set of substitutions applied to
  /// the declaration reference.
  SubstitutionMap getSubstitutions() const { return substitutions; }

  friend bool operator==(ConcreteDeclRef lhs, ConcreteDeclRef rhs) {
    return lhs.decl == rhs.decl && lhs.substitutions == rhs.substitutions;
  }

  /// Dump a debug representation of this reference.
  void dump(raw_ostream &os) const;
  POLAR_DEBUG_DUMP;
};

} // end namespace polar

#endif // POLARPHP_AST_CONCRETEDECLREF_H
