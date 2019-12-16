//===- TypeOrExtensionDecl.h - Swift Language Declaration ASTs -*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polar.org/LICENSE.txt for license information
// See https://polar.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file defines the TypeOrExtensionDecl struct, separately to Decl.h so
// that this can be included in files that Decl.h includes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_TYPE_OR_EXTENSION_DECL_H
#define POLARPHP_AST_TYPE_OR_EXTENSION_DECL_H

#include "polarphp/ast/TypeAlignments.h"
#include "llvm/ADT/PointerUnion.h"

namespace polar {

/// Describes either a nominal type declaration or an extension
/// declaration.
struct TypeOrExtensionDecl
{
  // (The definitions are in Decl.cpp.)
  llvm::PointerUnion<NominalTypeDecl *, ExtensionDecl *> Decl;

  TypeOrExtensionDecl() = default;

  TypeOrExtensionDecl(NominalTypeDecl *D);
  TypeOrExtensionDecl(ExtensionDecl *D);

  /// Return the contained *Decl as the Decl superclass.
  class Decl *getAsDecl() const;
  /// Return the contained *Decl as the DeclContext superclass.
  DeclContext *getAsDeclContext() const;
  /// Return the contained NominalTypeDecl or that of the extended type
  /// in the ExtensionDecl.
  NominalTypeDecl *getBaseNominal() const;

  /// Is the contained pointer null?
  bool isNull() const;
  explicit operator bool() const { return !isNull(); }

  bool operator==(TypeOrExtensionDecl rhs) { return Decl == rhs.Decl; }
  bool operator!=(TypeOrExtensionDecl rhs) { return Decl != rhs.Decl; }
  bool operator<(TypeOrExtensionDecl rhs) { return Decl < rhs.Decl; }
};

} // end namespace polar

#endif
