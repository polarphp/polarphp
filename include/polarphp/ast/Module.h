//===--- Module.h - Swift Language Module ASTs ------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//===----------------------------------------------------------------------===//
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
//===----------------------------------------------------------------------===//
//
// This file defines the Module class and its subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_MODULE_H
#define POLARPHP_AST_MODULE_H

#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/LookupKinds.h"
#include "polarphp/ast/ReferencedNameTracker.h"
#include "polarphp/ast/Type.h"
#include "polarphp/basic/OptionSet.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/basic/SourceLoc.h"
#include "polarphp/basic/InlineBitfield.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/MD5.h"

namespace clang {
class Module;
}

namespace polar::syntax {
class SourceFileSyntax;
}

namespace polar::ast {

using polar::basic::bitmax;
using llvm::StringRef;

enum class ArtificialMainKind : uint8_t;
class AstContext;
class AstScope;
class AstWalker;
class BraceStmt;
class Decl;
class DeclAttribute;
class TypeDecl;
enum class DeclKind : uint8_t;
class ExtensionDecl;
class DebuggerClient;
class DeclName;
class FileUnit;
class FuncDecl;
class InfixOperatorDecl;
class LazyResolver;
class LinkLibrary;
class LookupCache;
class ModuleLoader;
class NominalTypeDecl;
class EnumElementDecl;
class OperatorDecl;
class PostfixOperatorDecl;
class PrefixOperatorDecl;
class ProtocolConformance;
class InterfaceDecl;
struct PrintOptions;
class ReferencedNameTracker;
class Token;
class TupleType;
class Type;
class TypeRefinementContext;
class ValueDecl;
class VarDecl;
class VisibleDeclConsumer;
class SyntaxParsingCache;
class SourceFile;

/// Discriminator for file-units.
enum class FileUnitKind
{
   /// For a .swift source file.
   Source,
   /// For the compiler Builtin module.
   Builtin,
   /// A serialized Swift AST.
   SerializedAST,
   /// An imported Clang module.
   ClangModule,
   /// A Clang module imported from DWARF.
   DWARFModule
};


enum class SourceFileKind
{
   Library,  ///< A normal .polar file.
   Main,     ///< A .polar file that can have top-level code.
   REPL,     ///< A virtual file that holds the user's input in the REPL.
   PIL,      ///< Came from a .sil file.
   Interface ///< Came from a .polarinterface file, representing another module.
};

/// Discriminator for resilience strategy.
enum class ResilienceStrategy : unsigned
{
   /// Public nominal types: fragile
   /// Non-inlinable function bodies: resilient
   ///
   /// This is the default behavior without any flags.
   Default,

   /// Public nominal types: resilient
   /// Non-inlinable function bodies: resilient
   ///
   /// This is the behavior with -enable-library-evolution.
   Resilient
};

/// The minimum unit of compilation.
///
/// A module is made up of several file-units, which are all part of the same
/// output binary and logical module (such as a single library or executable).
///
/// \sa FileUnit
class ModuleDecl
{
protected:
   union {
      uint64_t opaqueBits;
      POLAR_INLINE_BITFIELD_BASE(
            ModuleDecl, 1+1+1+1+1+1+1+1,
            /// If the module was or is being compiled with `-enable-testing`.
            TestingEnabled : 1,

            /// If the module failed to load
            FailedToLoad : 1,

            /// Whether the module is resilient.
            ///
            /// \sa ResilienceStrategy
            RawResilienceStrategy : 1,

            /// Whether all imports have been resolved. Used to detect circular imports.
            HasResolvedImports : 1,

            /// If the module was or is being compiled with `-enable-private-imports`.
            PrivateImportsEnabled : 1,

            /// If the module is compiled with `-enable-implicit-dynamic`.
            ImplicitDynamicEnabled : 1,

            /// Whether the module is a system module.
            IsSystemModule : 1,

            /// Whether the module was imported from Clang (or, someday, maybe another
            /// language).
            IsNonPolarphpModule : 1
            );
   } m_bits;
public:
   typedef ArrayRef<std::pair<Identifier, SourceLoc>> AccessPathTy;
   typedef std::pair<ModuleDecl::AccessPathTy, ModuleDecl*> ImportedModule;

   static bool matchesAccessPath(AccessPathTy accessPath, DeclName name)
   {
      assert(accessPath.size() <= 1 && "can only refer to top-level decls");

      return accessPath.empty()
            || DeclName(accessPath.front().first).matchesRef(name);
   }

   /// Arbitrarily orders ImportedModule records, for inclusion in sets and such.
   class OrderImportedModules
   {
   public:
      bool operator()(const ImportedModule &lhs,
                      const ImportedModule &rhs) const
      {
         if (lhs.second != rhs.second) {
            return std::less<const ModuleDecl *>()(lhs.second, rhs.second);
         }
         if (lhs.first.data() != rhs.first.data()) {
            return std::less<AccessPathTy::iterator>()(lhs.first.begin(),
                                                       rhs.first.begin());
         }
         return lhs.first.size() < rhs.first.size();
      }
   };

   /// Produces the components of a given module's full name in reverse order.
   ///
   /// For a Swift module, this will only ever have one component, but an
   /// imported Clang module might actually be a submodule.
   class ReverseFullNameIterator
   {
   public:
      // Make this look like a valid STL iterator.
      using difference_type = int;
      using value_type = StringRef;
      using pointer = StringRef *;
      using reference = StringRef;
      using iterator_category = std::forward_iterator_tag;

   private:
      PointerUnion<const ModuleDecl *, const /* clang::Module */ void *> current;
   public:
      ReverseFullNameIterator() = default;
      explicit ReverseFullNameIterator(const ModuleDecl *M);
      explicit ReverseFullNameIterator(const clang::Module *clangModule)
      {
         current = clangModule;
      }

      StringRef operator*() const;
      ReverseFullNameIterator &operator++();

      friend bool operator==(ReverseFullNameIterator left,
                             ReverseFullNameIterator right)
      {
         return left.current == right.current;
      }

      friend bool operator!=(ReverseFullNameIterator left,
                             ReverseFullNameIterator right)
      {
         return !(left == right);
      }

      /// This is a convenience function that writes the entire name, in forward
      /// order, to \p out.
      void printForward(raw_ostream &out) const;
   };

private:
   /// If non-NULL, a plug-in that should be used when performing external
   /// lookups.
   // FIXME: Do we really need to bloat all modules with this?
   DebuggerClient *DebugClient = nullptr;

   SmallVector<FileUnit *, 2> m_files;

   /// Tracks the file that will generate the module's entry point, either
   /// because it contains a class marked with \@UIApplicationMain
   /// or \@NSApplicationMain, or because it is a script file.
   class EntryPointInfoTy
   {
      enum class Flags
      {
         DiagnosedMultipleMainClasses = 1 << 0,
         DiagnosedMainClassWithScript = 1 << 1
      };
      llvm::PointerIntPair<FileUnit *, 2, OptionSet<Flags>> storage;
   public:
      EntryPointInfoTy() = default;

      FileUnit *getEntryPointFile() const
      {
         return storage.getPointer();
      }

      void setEntryPointFile(FileUnit *file)
      {
         assert(!storage.getPointer());
         storage.setPointer(file);
      }

      bool hasEntryPoint() const
      {
         return storage.getPointer();
      }

      bool markDiagnosedMultipleMainClasses()
      {
         bool res = storage.getInt().contains(Flags::DiagnosedMultipleMainClasses);
         storage.setInt(storage.getInt() | Flags::DiagnosedMultipleMainClasses);
         return !res;
      }

      bool markDiagnosedMainClassWithScript()
      {
         bool res = storage.getInt().contains(Flags::DiagnosedMainClassWithScript);
         storage.setInt(storage.getInt() | Flags::DiagnosedMainClassWithScript);
         return !res;
      }
   };

   /// Information about the file responsible for the module's entry point,
   /// if any.
   ///
   /// \see EntryPointInfoTy
   EntryPointInfoTy EntryPointInfo;

   ModuleDecl(Identifier name, AstContext &ctx);

public:
   ArrayRef<FileUnit *> getFiles()
   {
      return m_files;
   }

   ArrayRef<const FileUnit *> getFiles() const
   {
      return { m_files.begin(), m_files.size() };
   }

   bool isClangModule() const;
   void addFile(FileUnit &newFile);
   void removeFile(FileUnit &existingFile);

   /// Convenience accessor for clients that know what kind of file they're
   /// dealing with.
   SourceFile &getMainSourceFile(SourceFileKind expectedKind) const;

   /// Convenience accessor for clients that know what kind of file they're
   /// dealing with.
   FileUnit &getMainFile(FileUnitKind expectedKind) const;

   DebuggerClient *getDebugClient() const { return DebugClient; }
   void setDebugClient(DebuggerClient *R) {
      assert(!DebugClient && "Debugger client already set");
      DebugClient = R;
   }

   /// Returns true if this module was or is being compiled for testing.
   bool isTestingEnabled() const
   {
      return m_bits.ModuleDecl.TestingEnabled;
   }

   void setTestingEnabled(bool enabled = true)
   {
      m_bits.ModuleDecl.TestingEnabled = enabled;
   }

   // Returns true if this module is compiled with implicit dynamic.
   bool isImplicitDynamicEnabled() const
   {
      return m_bits.ModuleDecl.ImplicitDynamicEnabled;
   }

   void setImplicitDynamicEnabled(bool enabled = true)
   {
      m_bits.ModuleDecl.ImplicitDynamicEnabled = enabled;
   }

   /// Returns true if this module was or is begin compile with
   /// `-enable-private-imports`.
   bool arePrivateImportsEnabled() const
   {
      return m_bits.ModuleDecl.PrivateImportsEnabled;
   }

   void setPrivateImportsEnabled(bool enabled = true)
   {
      m_bits.ModuleDecl.PrivateImportsEnabled = enabled;
   }

   /// Returns true if there was an error trying to load this module.
   bool failedToLoad() const
   {
      return m_bits.ModuleDecl.FailedToLoad;
   }

   void setFailedToLoad(bool failed = true)
   {
      m_bits.ModuleDecl.FailedToLoad = failed;
   }

   bool hasResolvedImports() const
   {
      return m_bits.ModuleDecl.HasResolvedImports;
   }

   void setHasResolvedImports()
   {
      m_bits.ModuleDecl.HasResolvedImports = true;
   }

   ResilienceStrategy getResilienceStrategy() const
   {
      return ResilienceStrategy(m_bits.ModuleDecl.RawResilienceStrategy);
   }

   void setResilienceStrategy(ResilienceStrategy strategy)
   {
      m_bits.ModuleDecl.RawResilienceStrategy = unsigned(strategy);
   }

   /// \returns true if this module is a system module; note that the StdLib is
   /// considered a system module.
   bool isSystemModule() const
   {
      return m_bits.ModuleDecl.IsSystemModule;
   }

   void setIsSystemModule(bool flag = true)
   {
      m_bits.ModuleDecl.IsSystemModule = flag;
   }

   /// Returns true if this module is a non-Swift module that was imported into
   /// Swift.
   ///
   /// Right now that's just Clang modules.
   bool isNonPolarphpModule() const
   {
      return m_bits.ModuleDecl.IsNonPolarphpModule;
   }

   /// \see #isNonPolarphpModule
   void setIsNonPolarphpModule(bool flag = true)
   {
      m_bits.ModuleDecl.IsNonPolarphpModule = flag;
   }

   bool isResilient() const
   {
      return getResilienceStrategy() != ResilienceStrategy::Default;
   }

   /// Look up a (possibly overloaded) value set at top-level scope
   /// (but with the specified access path, which may come from an import decl)
   /// within the current module.
   ///
   /// This does a simple local lookup, not recursively looking through imports.
   void lookupValue(AccessPathTy accessPath, DeclName name, NLKind lookupKind,
                    SmallVectorImpl<ValueDecl*> &Result) const;

   /// Look up a local type declaration by its mangled name.
   ///
   /// This does a simple local lookup, not recursively looking through imports.
   TypeDecl *lookupLocalType(StringRef mangledName) const;

   //   /// Look up an opaque return type by the mangled name of the declaration
   //   /// that defines it.
   //   OpaqueTypeDecl *lookupOpaqueResultType(StringRef mangledName,
   //                                          LazyResolver *resolver);

   /// Find ValueDecls in the module and pass them to the given consumer object.
   ///
   /// This does a simple local lookup, not recursively looking through imports.
   void lookupVisibleDecls(AccessPathTy accessPath,
                           VisibleDeclConsumer &consumer,
                           NLKind lookupKind) const;

   /// @{

   /// Look up the given operator in this module.
   ///
   /// If the operator is not found, or if there is an ambiguity, returns null.
   InfixOperatorDecl *lookupInfixOperator(Identifier name,
                                          SourceLoc diagLoc = {});
   PrefixOperatorDecl *lookupPrefixOperator(Identifier name,
                                            SourceLoc diagLoc = {});
   PostfixOperatorDecl *lookupPostfixOperator(Identifier name,
                                              SourceLoc diagLoc = {});
   //   PrecedenceGroupDecl *lookupPrecedenceGroup(Identifier name,
   //                                              SourceLoc diagLoc = {});
   /// @}

   /// Finds all class members defined in this module.
   ///
   /// This does a simple local lookup, not recursively looking through imports.
   void lookupClassMembers(AccessPathTy accessPath,
                           VisibleDeclConsumer &consumer) const;

   /// Finds class members defined in this module with the given name.
   ///
   /// This does a simple local lookup, not recursively looking through imports.
   void lookupClassMember(AccessPathTy accessPath,
                          DeclName name,
                          SmallVectorImpl<ValueDecl*> &results) const;

   /// Look for the conformance of the given type to the given protocol.
   ///
   /// This routine determines whether the given \c type conforms to the given
   /// \c protocol.
   ///
   /// \param type The type for which we are computing conformance.
   ///
   /// \param protocol The protocol to which we are computing conformance.
   ///
   /// \returns The result of the conformance search, which will be
   /// None if the type does not conform to the protocol or contain a
   /// ProtocolConformanceRef if it does conform.
   Optional<ProtocolConformanceRef>
   lookupConformance(Type type, InterfaceDecl *protocol);

   /// Look for the conformance of the given existential type to the given
   /// protocol.
   Optional<ProtocolConformanceRef>
   lookupExistentialConformance(Type type, InterfaceDecl *protocol);

   /// Exposes TypeChecker functionality for querying protocol conformance.
   /// Returns a valid ProtocolConformanceRef only if all conditional
   /// requirements are successfully resolved.
   Optional<ProtocolConformanceRef>
   conformsToProtocol(Type sourceTy, InterfaceDecl *targetProtocol);

   /// Find a member named \p name in \p container that was declared in this
   /// module.
   ///
   /// \p container may be \c this for a top-level lookup.
   ///
   /// If \p privateDiscriminator is non-empty, only matching private decls are
   /// returned; otherwise, only non-private decls are returned.
   void lookupMember(SmallVectorImpl<ValueDecl*> &results,
                     DeclContext *container, DeclName name,
                     Identifier privateDiscriminator) const;


   /// \sa getImportedModules
   enum class ImportFilterKind
   {
      /// Include imports declared with `@_exported`.
      Public = 1 << 0,
      /// Include "regular" imports with no special annotation.
      Private = 1 << 1,
      /// Include imports declared with `@_implementationOnly`.
      ImplementationOnly = 1 << 2
   };
   /// \sa getImportedModules
   using ImportFilter = OptionSet<ImportFilterKind>;

   /// Looks up which modules are imported by this module.
   ///
   /// \p filter controls whether public, private, or any imports are included
   /// in this list.
   void getImportedModules(SmallVectorImpl<ImportedModule> &imports,
                           ImportFilter filter = ImportFilterKind::Public) const;

   /// Looks up which modules are imported by this module, ignoring any that
   /// won't contain top-level decls.
   ///
   /// This is a performance hack. Do not use for anything but name lookup.
   /// May go away in the future.
   void
   getImportedModulesForLookup(SmallVectorImpl<ImportedModule> &imports) const;

   /// Uniques the items in \p imports, ignoring the source locations of the
   /// access paths.
   ///
   /// The order of items in \p imports is \e not preserved.
   static void removeDuplicateImports(SmallVectorImpl<ImportedModule> &imports);

   /// Finds all top-level decls of this module.
   ///
   /// This does a simple local lookup, not recursively looking through imports.
   /// The order of the results is not guaranteed to be meaningful.
   void getTopLevelDecls(SmallVectorImpl<Decl*> &Results) const;

   /// Finds all local type decls of this module.
   ///
   /// This does a simple local lookup, not recursively looking through imports.
   /// The order of the results is not guaranteed to be meaningful.
   void getLocalTypeDecls(SmallVectorImpl<TypeDecl*> &Results) const;

   //   /// Finds all precedence group decls of this module.
   //   ///
   //   /// This does a simple local lookup, not recursively looking through imports.
   //   /// The order of the results is not guaranteed to be meaningful.
   //   void getPrecedenceGroups(SmallVectorImpl<PrecedenceGroupDecl*> &Results) const;

   /// Finds all top-level decls that should be displayed to a client of this
   /// module.
   ///
   /// This includes types, variables, functions, and extensions.
   /// This does a simple local lookup, not recursively looking through imports.
   /// The order of the results is not guaranteed to be meaningful.
   ///
   /// This can differ from \c getTopLevelDecls, e.g. it returns decls from a
   /// shadowed clang module.
   void getDisplayDecls(SmallVectorImpl<Decl*> &results) const;

   /// @{

   /// Perform an action for every module visible from this module.
   ///
   /// This only includes modules with at least one declaration visible: if two
   /// import access paths are incompatible, the indirect module will be skipped.
   /// Modules that can't be used for lookup (including Clang submodules at the
   /// time this comment was written) are also skipped under certain
   /// circumstances.
   ///
   /// \param topLevelAccessPath If present, include the top-level module in the
   ///        results, with the given access path.
   /// \param fn A callback of type bool(ImportedModule) or void(ImportedModule).
   ///        Return \c false to abort iteration.
   ///
   /// \return True if the traversal ran to completion, false if it ended early
   ///         due to the callback.
   bool forAllVisibleModules(AccessPathTy topLevelAccessPath,
                             llvm::function_ref<bool(ImportedModule)> fn);

   bool forAllVisibleModules(AccessPathTy topLevelAccessPath,
                             llvm::function_ref<void(ImportedModule)> fn)
   {
      return forAllVisibleModules(topLevelAccessPath,
                                  [=](const ImportedModule &import) -> bool {
         fn(import);
         return true;
      });
   }

   template <typename Fn>
   bool forAllVisibleModules(AccessPathTy topLevelAccessPath,
                             Fn &&fn)
   {
      using RetTy = typename std::result_of<Fn(ImportedModule)>::type;
      llvm::function_ref<RetTy(ImportedModule)> wrapped{std::forward<Fn>(fn)};
      return forAllVisibleModules(topLevelAccessPath, wrapped);
   }

   /// @}

   using LinkLibraryCallback = llvm::function_ref<void(LinkLibrary)>;

   /// Generate the list of libraries needed to link this module, based on its
   /// imports.
   void collectLinkLibraries(LinkLibraryCallback callback);

   /// Returns true if the two access paths contain the same chain of
   /// identifiers.
   ///
   /// Source locations are ignored here.
   static bool isSameAccessPath(AccessPathTy lhs, AccessPathTy rhs);

   /// Get the path for the file that this module came from, or an empty
   /// string if this is not applicable.
   StringRef getModuleFilename() const;

   /// \returns true if this module is the "swift" standard library module.
   bool isStdlibModule() const;

   /// \returns true if this module is the "SwiftShims" module;
   bool isSwiftShimsModule() const;

   /// \returns true if this module is the "builtin" module.
   bool isBuiltinModule() const;

   /// \returns true if this module is the "SwiftOnoneSupport" module;
   bool isOnoneSupportModule() const;

   /// \returns true if traversal was aborted, false otherwise.
   bool walk(AstWalker &walker);

   /// Register the file responsible for generating this module's entry point.
   ///
   /// \returns true if there was a problem adding this file.
   bool registerEntryPointFile(FileUnit *file, SourceLoc diagLoc,
                               Optional<ArtificialMainKind> kind);

   /// \returns true if this module has a main entry point.
   bool hasEntryPoint() const
   {
      return EntryPointInfo.hasEntryPoint();
   }

   /// Returns the associated clang module if one exists.
   const clang::Module *findUnderlyingClangModule() const;

   /// Returns a generator with the components of this module's full,
   /// hierarchical name.
   ///
   /// For a Swift module, this will only ever have one component, but an
   /// imported Clang module might actually be a submodule.
   ReverseFullNameIterator getReverseFullModuleName() const
   {
      return ReverseFullNameIterator(this);
   }

   SourceRange getSourceRange() const
   {
      return SourceRange();
   }
};

} // polar::ast

#endif // POLARPHP_AST_MODULE_H
