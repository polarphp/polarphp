//===--- USRGeneration.h - Routines for USR generation ----------*- C++ -*-===//
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
// Unique Symbol References (USRs) provide a textual encoding for
// declarations. These are used for indexing, analogous to how mangled names
// are used in object files.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_USRGENERATION_H
#define POLARPHP_AST_USRGENERATION_H

#include "polarphp/basic/LLVM.h"

namespace polar {

class Decl;
class AbstractStorageDecl;
class ValueDecl;
class ExtensionDecl;
class ModuleEntity;
enum class AccessorKind;
class Type;

namespace ide {

/// Prints out the USR for the Type.
/// \returns true if it failed, false on success.
bool printTypeUSR(Type Ty, raw_ostream &ostream);

/// Prints out the USR for the Type of the given decl.
/// \returns true if it failed, false on success.
bool printDeclTypeUSR(const ValueDecl *D, raw_ostream &ostream);

/// Prints out the USR for the given ValueDecl.
/// \returns true if it failed, false on success.
bool printValueDeclUSR(const ValueDecl *D, raw_ostream &ostream);

/// Prints out the USR for the given ModuleEntity.
/// \returns true if it failed, false on success.
bool printModuleUSR(ModuleEntity Mod, raw_ostream &ostream);

/// Prints out the accessor USR for the given storage Decl.
/// \returns true if it failed, false on success.
bool printAccessorUSR(const AbstractStorageDecl *D, AccessorKind AccKind,
                      llvm::raw_ostream &ostream);

/// Prints out the extension USR for the given extension Decl.
/// \returns true if it failed, false on success.
bool printExtensionUSR(const ExtensionDecl *ED, raw_ostream &ostream);

/// Prints out the USR for the given Decl.
/// \returns true if it failed, false on success.
bool printDeclUSR(const Decl *D, raw_ostream &ostream);

} // namespace ide
} // namespace polar

#endif // POLARPHP_AST_USRGENERATION_H

