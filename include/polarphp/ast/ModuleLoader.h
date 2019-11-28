//===--- ModuleLoader.h - Module Loader Interface ---------------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.
//
//===----------------------------------------------------------------------===//
//
// This file implements an abstract interface for loading modules.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_MODULE_LOADER_H
#define POLARPHP_AST_MODULE_LOADER_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/SourceLoc.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/TinyPtrVector.h"

namespace clang {
class DependencyCollector;
} // clang

namespace polar::syntax {
class TokenSyntax;
} // polar::syntax

namespace polar::ast {

class AbstractFunctionDecl;
class ClangImporterOptions;
class ClassDecl;
class ModuleDecl;
class NominalTypeDecl;

using polar::basic::SourceLoc;
using polar::syntax::TokenSyntax;

enum class KnownProtocolKind : uint8_t;

enum class Bridgeability : unsigned
{
   /// This context does not permit bridging at all.  For example, the
   /// target of a C pointer.
   None,

   /// This context permits all kinds of bridging.  For example, the
   /// imported result of a method declaration.
   Full
};

/// Records dependencies on files outside of the current module;
/// implemented in terms of a wrapped clang::DependencyCollector.
class DependencyTracker
{
   std::shared_ptr<clang::DependencyCollector> clangCollector;
public:
   explicit DependencyTracker(bool TrackSystemDeps);

   /// Adds a file as a dependency.
   ///
   /// The contents of \p file are taken literally, and should be appropriate
   /// for appearing in a list of dependencies suitable for tooling like Make.
   /// No path canonicalization is done.
   void addDependency(StringRef file, bool isSystem);

   /// Fetches the list of dependencies.
   ArrayRef<std::string> getDependencies() const;

   /// Return the underlying clang::DependencyCollector that this
   /// class wraps.
   std::shared_ptr<clang::DependencyCollector> getClangCollector();
};

/// Abstract interface that loads named modules into the AST.
class ModuleLoader
{
   virtual void anchor();

protected:
   DependencyTracker * const dependencyTracker;
   ModuleLoader(DependencyTracker *tracker)
      : dependencyTracker(tracker)
   {}

public:
   virtual ~ModuleLoader() = default;

   /// Collect visible module names.
   ///
   /// Append visible module names to \p names. Note that names are possibly
   /// duplicated, and not guaranteed to be ordered in any way.
   virtual void collectVisibleTopLevelModuleNames(
         SmallVectorImpl<TokenSyntax> &names) const = 0;

   /// Check whether the module with a given name can be imported without
   /// importing it.
   ///
   /// Note that even if this check succeeds, errors may still occur if the
   /// module is loaded in full.
   virtual bool canImportModule(std::pair<TokenSyntax, SourceLoc> named) = 0;

   /// Import a module with the given module path.
   ///
   /// \param importLoc The location of the 'import' keyword.
   ///
   /// \param path A sequence of (identifier, location) pairs that denote
   /// the dotted module name to load, e.g., AppKit.NSWindow.
   ///
   /// \returns the module referenced, if it could be loaded. Otherwise,
   /// emits a diagnostic and returns NULL.
   virtual
   ModuleDecl *loadModule(SourceLoc importLoc,
                          ArrayRef<std::pair<TokenSyntax, SourceLoc>> path) = 0;

   /// Load extensions to the given nominal type.
   ///
   /// \param nominal The nominal type whose extensions should be loaded.
   ///
   /// \param previousGeneration The previous generation number. The AST already
   /// contains extensions loaded from any generation up to and including this
   /// one.
   virtual void loadExtensions(NominalTypeDecl *nominal, unsigned previousGeneration) {}

   /// Verify all modules loaded by this loader.
   virtual void verifyAllModules() {}
};

} // polar::ast

#endif // POLARPHP_AST_MODULE_LOADER_H
