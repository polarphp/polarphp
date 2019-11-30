//===--- AstContext.h - AST Context Object ----------------------*- C++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/30.
//===----------------------------------------------------------------------===//
// This file defines the AstContext interface.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ASTCONTEXT_H
#define POLARPHP_AST_ASTCONTEXT_H

#include "llvm/Support/DataTypes.h"
#include "polarphp/ast/ClangModuleLoader.h"
#include "polarphp/ast/Evaluator.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/SearchPathOptions.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/basic/Malloc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/Allocator.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace clang {
class Decl;
class MacroInfo;
class Module;
}

namespace polar::syntax {
class SyntaxArena;
}

namespace polar::basic {
class SourceLoc;
class UnifiedStatsReporter;
}

namespace polar::ast {

using polar::basic::SourceLoc;
using polar::basic::UnifiedStatsReporter;
using polar::kernel::LangOptions;
using polar::basic::aligned_alloc;

class AstContext;
enum class Associativity : unsigned char;
class AvailabilityContext;
class BoundGenericType;
class ClangNode;
class ConstructorDecl;
class Decl;
class DeclContext;
class DefaultArgumentInitializer;
class ExtensionDecl;
class ForeignRepresentationInfo;
class FuncDecl;
class GenericContext;
class InFlightDiagnostic;
class IterableDeclContext;
class LazyContextm_data;
class LazyGenericContextm_data;
class LazyIterableDeclContextm_data;
class LazyMemberLoader;
class LazyMemberParser;
class LazyResolver;
class PatternBindingDecl;
class PatternBindingInitializer;
class SourceFile;
class Type;
class TypeVariableType;
class TupleType;
class FunctionType;
class GenericSignatureBuilder;
class ArchetypeType;
class Identifier;
class InheritedNameSet;
class ModuleDecl;
class ModuleLoader;
class NominalTypeDecl;
class NormalProtocolConformance;
class OpaqueTypeDecl;
class InheritedProtocolConformance;
class m_selfProtocolConformance;
class SpecializedProtocolConformance;
enum class ProtocolConformanceState;
class Pattern;
enum PointerTypeKind : unsigned;
class PrecedenceGroupDecl;
class TupleTypeElt;
class EnumElementDecl;
class ProtocolDecl;
class SubstitutableType;
class SourceManager;
class ValueDecl;
class DiagnosticEngine;
class TypeCheckerDebugConsumer;
struct RawComment;
class DocComment;
class PILBoxType;
class TypeAliasDecl;
class VarDecl;

enum class KnownInterfaceKind : uint8_t;

/// The arena in which a particular AstContext allocation will go.
enum class AllocationArena
{
   /// The permanent arena, which is tied to the lifetime of
   /// the AstContext.
   ///
   /// All global declarations and types need to be allocated into this arena.
   /// At present, everything that is not a type involving a type variable is
   /// allocated in this arena.
   Permanent,
   /// The constraint solver's temporary arena, which is tied to the
   /// lifetime of a particular instance of the constraint solver.
   ///
   /// Any type involving a type variable is allocated in this arena.
   ConstraintSolver
};

/// Introduces a new constraint checker arena, whose lifetime is
/// tied to the lifetime of this RAII object.
class ConstraintCheckerArenaRAII
{
   AstContext &m_self;
   void *m_data;

public:
   /// Introduces a new constraint checker arena, supplanting any
   /// existing constraint checker arena.
   ///
   /// \param self The AstContext into which this constraint checker arena
   /// will be installed.
   ///
   /// \param allocator The allocator used for allocating any data that
   /// goes into the constraint checker arena.
   ConstraintCheckerArenaRAII(AstContext &self,
                              llvm::BumpPtrAllocator &allocator);

   ConstraintCheckerArenaRAII(const ConstraintCheckerArenaRAII &) = delete;
   ConstraintCheckerArenaRAII(ConstraintCheckerArenaRAII &&) = delete;

   ConstraintCheckerArenaRAII &
   operator=(const ConstraintCheckerArenaRAII &) = delete;

   ConstraintCheckerArenaRAII &
   operator=(ConstraintCheckerArenaRAII &&) = delete;

   ~ConstraintCheckerArenaRAII();
};

class PILLayout; // From PIL

/// AstContext - This object creates and owns the AST objects.
/// However, this class does more than just maintain context within an AST.
/// It is the closest thing to thread-local or compile-local storage in this
/// code base. Why? SourceKit uses this code with multiple threads per Unix
/// process. Each thread processes a different source file. Each thread has its
/// own instance of AstContext, and that instance persists for the duration of
/// the thread, throughout all phases of the compilation. (The name "AstContext"
/// is a bit of a misnomer here.) Why not use thread-local storage? This code
/// may use DispatchQueues and pthread-style TLS won't work with code that uses
/// DispatchQueues. Summary: if you think you need a global or static variable,
/// you probably need to put it here instead.

class AstContext final
{
   AstContext(const AstContext&) = delete;
   void operator=(const AstContext&) = delete;

   AstContext(LangOptions &langOpts, SearchPathOptions &searchPathOpts,
              SourceManager &sourceMgr, DiagnosticEngine &diags);

public:
   // Members that should only be used by AstContext.cpp.
   struct Implementation;
   Implementation &getImpl() const;

   friend ConstraintCheckerArenaRAII;

   void operator delete(void *data) noexcept;

   static AstContext *get(LangOptions &langOpts,
                          SearchPathOptions &searchPathOpts,
                          SourceManager &sourceMgr,
                          DiagnosticEngine &diags);
   ~AstContext();

   /// Optional table of counters to report, nullptr when not collecting.
   ///
   /// This must be initialized early so that allocate() doesn't try to access
   /// it before being set to null.
   UnifiedStatsReporter *stats = nullptr;

   /// The language options used for translation.
   LangOptions &langOpts;

   /// The search path options used by this AST context.
   SearchPathOptions &searchPathOpts;

   /// The source manager object.
   SourceManager &sourceMgr;

   /// diags - The diagnostics engine.
   DiagnosticEngine &diags;

   /// The request-evaluator that is used to process various requests.
   Evaluator evaluator;

   /// The set of top-level modules we have loaded.
   /// This map is used for iteration, therefore it's a MapVector and not a
   /// DenseMap.
   llvm::MapVector<Identifier, ModuleDecl*> loadedModules;

   /// The builtin module.
   ModuleDecl * const theBuiltinModule;

   /// The standard library module.
   mutable ModuleDecl *theStdlibModule = nullptr;

   /// The name of the standard library module "Polarphp".
   Identifier stdlibModuleName;

   /// The name of the SwiftShims module "SwiftShims".
   Identifier SwiftShimsModuleName;

   // Define the set of known identifiers.
   //#define IDENTIFIER_WITH_NAME(Name, IdStr) Identifier Id_##Name;
   //#include "polarphp/ast/KnownIdentifiersDef.h"

   /// The list of external definitions imported by this context.
   llvm::SetVector<Decl *> externalDefinitions;

   /// FIXME: HACK HACK HACK
   /// This state should be tracked somewhere else.
   unsigned lastCheckedExternalDefinition = 0;

   /// A consumer of type checker debug output.
   std::unique_ptr<TypeCheckerDebugConsumer> typeCheckerDebug;

   /// Cache for names of canonical GenericTypeParamTypes.
   mutable llvm::DenseMap<unsigned, Identifier>
   canonicalGenericTypeParamTypeNames;

   /// Cache of remapped types (useful for diagnostics).
   llvm::StringMap<Type> remappedTypes;

private:
   /// The current generation number, which reflects the number of
   /// times that external modules have been loaded.
   ///
   /// Various places in the ast, such as the set of extensions associated with
   /// a nominal type, keep track of the generation number they saw and will
   /// automatically update when they are out of date.
   unsigned m_currentGeneration = 0;

   friend class Pattern;

   /// Mapping from patterns that store interface types that will be lazily
   /// resolved to contextual types, to the declaration context in which the
   /// pattern resides.
   llvm::DenseMap<const Pattern *, DeclContext *>
   m_delayedPatternContexts;

   /// Cache of module names that fail the 'canImport' test in this context.
   llvm::SmallPtrSet<Identifier, 8> m_failedModuleImportNames;

   /// Retrieve the allocator for the given arena.
   llvm::BumpPtrAllocator &getAllocator(AllocationArena arena = AllocationArena::Permanent) const;

public:
   /// allocate - allocate memory from the AstContext bump pointer.
   void *allocate(unsigned long bytes, unsigned alignment,
                  AllocationArena arena = AllocationArena::Permanent) const
   {
      if (bytes == 0)
         return nullptr;

      if (langOpts.useMalloc) {
         return aligned_alloc(bytes, alignment);
      }

      if (arena == AllocationArena::Permanent && stats) {
         stats->getFrontendCounters().NumASTBytesAllocated += bytes;
      }
      return getAllocator(arena).Allocate(bytes, alignment);
   }

   template <typename T>
   T *allocate(AllocationArena arena = AllocationArena::Permanent) const
   {
      T *res = reinterpret_cast<T *>(allocate(sizeof(T), alignof(T), arena));
      new (res) T();
      return res;
   }

   template <typename T>
   MutableArrayRef<T> allocateUninitialized(unsigned numElts,
                                            AllocationArena Arena = AllocationArena::Permanent) const
   {
      T *data = reinterpret_cast<T *>(allocate(sizeof(T) * numElts, alignof(T), Arena));
      return { data, numElts };
   }

   template <typename T>
   MutableArrayRef<T> allocate(unsigned numElts,
                               AllocationArena arena = AllocationArena::Permanent) const
   {
      T *res = reinterpret_cast<T *>(allocate(sizeof(T) * numElts, alignof(T), arena));
      for (unsigned i = 0; i != numElts; ++i) {
         new (res+i) T();
      }
      return {res, numElts};
   }

   /// allocate a copy of the specified object.
   template <typename T>
   typename std::remove_reference<T>::type *allocateObjectCopy(T &&t,
                                                               AllocationArena arena = AllocationArena::Permanent) const
   {
      // This function cannot be named allocateCopy because it would always win
      // overload resolution over the allocateCopy(ArrayRef<T>).
      using TNoRef = typename std::remove_reference<T>::type;
      TNoRef *res = reinterpret_cast<TNoRef *>(allocate(sizeof(TNoRef), alignof(TNoRef), arena));
      new (res) TNoRef(std::forward<T>(t));
      return res;
   }

   template <typename T, typename It>
   T *allocateCopy(It start, It end,
                   AllocationArena arena = AllocationArena::Permanent) const
   {
      T *res = reinterpret_cast<T*>(allocate(sizeof(T)*(end-start), alignof(T), arena));
      for (unsigned i = 0; start != end; ++start, ++i) {
         new (res+i) T(*start);
      }
      return res;
   }

   template<typename T, size_t N>
   MutableArrayRef<T> allocateCopy(T (&array)[N],
                                   AllocationArena arena = AllocationArena::Permanent) const
   {
      return MutableArrayRef<T>(allocateCopy<T>(array, array+N, arena), N);
   }

   template<typename T>
   MutableArrayRef<T> allocateCopy(ArrayRef<T> array,
                                   AllocationArena arena = AllocationArena::Permanent) const
   {
      return MutableArrayRef<T>(allocateCopy<T>(array.begin(),array.end(), arena),
                                array.size());
   }


   template<typename T>
   ArrayRef<T> allocateCopy(const SmallVectorImpl<T> &vec,
                            AllocationArena arena = AllocationArena::Permanent) const
   {
      return allocateCopy(ArrayRef<T>(vec), arena);
   }

   template<typename T>
   MutableArrayRef<T>
   allocateCopy(SmallVectorImpl<T> &vec,
                AllocationArena arena = AllocationArena::Permanent) const
   {
      return allocateCopy(MutableArrayRef<T>(vec), arena);
   }

   StringRef allocateCopy(StringRef Str,
                          AllocationArena arena = AllocationArena::Permanent) const
   {
      ArrayRef<char> Result =
            allocateCopy(llvm::makeArrayRef(Str.data(), Str.size()), arena);
      return StringRef(Result.data(), Result.size());
   }

   template<typename T, typename Vector, typename Set>
   MutableArrayRef<T>
   allocateCopy(llvm::SetVector<T, Vector, Set> setVector,
                AllocationArena arena = AllocationArena::Permanent) const
   {
      return MutableArrayRef<T>(allocateCopy<T>(setVector.begin(),
                                                setVector.end(),
                                                arena),
                                setVector.size());
   }

   /// Retrive the syntax node memory manager for this context.
   llvm::IntrusiveRefCntPtr<syntax::SyntaxArena> getSyntaxArena() const;

   /// Set a new stats reporter.
   void setStatsReporter(UnifiedStatsReporter *stats);

   /// Creates a new lazy resolver by passing the AstContext and the other
   /// given arguments to a newly-allocated instance of \c ResolverType.
   ///
   /// \returns true if a new lazy resolver was created, false if there was
   /// already a lazy resolver registered.
   template<typename ResolverType, typename ... Args>
   bool createLazyResolverIfMissing(Args && ...args)
   {
      if (getLazyResolver()) {
         return false;
      }
      setLazyResolver(new ResolverType(*this, std::forward<Args>(args)...));
      return true;
   }

   /// Remove the lazy resolver, if there is one.
   ///
   /// FIXME: We probably don't ever want to do this.
   void removeLazyResolver()
   {
      setLazyResolver(nullptr);
   }

   /// Retrieve the lazy resolver for this context.
   LazyResolver *getLazyResolver() const;

private:
   /// Set the lazy resolver for this context.
   void setLazyResolver(LazyResolver *resolver);

public:
   /// Add a lazy parser for resolving members later.
   void addLazyParser(LazyMemberParser *parser);

   /// Remove a lazy parser.
   void removeLazyParser(LazyMemberParser *parser);

   /// getIdentifier - Return the uniqued and AST-Context-owned version of the
   /// specified string.
   Identifier getIdentifier(StringRef Str) const;

   /// Decide how to interpret two precedence groups.
   Associativity associateInfixOperators(PrecedenceGroupDecl *left,
                                         PrecedenceGroupDecl *right) const;

   /// Retrieve the declaration of Polarphp.Error.
   ProtocolDecl *getErrorDecl() const;
   CanType getExceptionType() const;

#define KNOWN_STDLIB_TYPE_DECL(NAME, DECL_CLASS, NUM_GENERIC_PARAMS) \
   /** Retrieve the declaration of Polarphp.NAME. */ \
   DECL_CLASS *get##NAME##Decl() const;
#include "polarphp/ast/KnownStdlibTypesDef.h"

   /// Retrieve the declaration of Polarphp.Optional<T>.Some.
   EnumElementDecl *getOptionalSomeDecl() const;

   /// Retrieve the declaration of Polarphp.Optional<T>.None.
   EnumElementDecl *getOptionalNoneDecl() const;

   /// Retrieve the declaration of the "pointee" property of a pointer type.
   VarDecl *getPointerPointeePropertyDecl(PointerTypeKind ptrKind) const;

   /// Retrieve the type Polarphp.AnyObject.
   CanType getAnyObjectType() const;

   /// Retrieve the type Polarphp.Never.
   CanType getNeverType() const;

   /// Retrieve the declaration of Polarphp.Void.
   TypeAliasDecl *getVoidDecl() const;

   /// Get the '+' function on two RangeReplaceableCollection.
   FuncDecl *getPlusFunctionOnRangeReplaceableCollection() const;

   /// Get the '+' function on two String.
   FuncDecl *getPlusFunctionOnString() const;

   /// Check whether the standard library provides all the correct
   /// intrinsic support for Optional<T>.
   ///
   /// If this is true, the four methods above all promise to return
   /// non-null.
   bool hasOptionalIntrinsics() const;

   /// Check whether the standard library provides all the correct
   /// intrinsic support for UnsafeMutablePointer<T> function arguments.
   ///
   /// If this is true, the methods getConvert*ToPointerArgument
   /// all promise to return non-null.
   bool hasPointerArgumentIntrinsics() const;

   /// Check whether the standard library provides all the correct
   /// intrinsic support for array literals.
   ///
   /// If this is true, the method getAllocateUninitializedArray
   /// promises to return non-null.
   bool hasArrayLiteralIntrinsics() const;

   /// Retrieve the declaration of Polarphp.Bool.init(_builtinBooleanLiteral:)
   ConstructorDecl *getBoolBuiltinInitDecl() const;

   /// Retrieve the declaration of Polarphp.==(Int, Int) -> Bool.
   FuncDecl *getEqualIntDecl() const;

   /// Retrieve the declaration of Polarphp._hashValue<H>(for: H) -> Int.
   FuncDecl *getHashValueForDecl() const;

   /// Retrieve the declaration of Array.append(element:)
   FuncDecl *getArrayAppendElementDecl() const;

   /// Retrieve the declaration of
   /// Array.reserveCapacityForAppend(newElementsCount: Int)
   FuncDecl *getArrayReserveCapacityDecl() const;

   /// Retrieve the declaration of Polarphp._unimplementedInitializer.
   FuncDecl *getUnimplementedInitializerDecl() const;

   /// Retrieve the declaration of Polarphp._undefined.
   FuncDecl *getUndefinedDecl() const;

   // Retrieve the declaration of Polarphp._stdlib_isOSVersionAtLeast.
   FuncDecl *getIsOSVersionAtLeastDecl() const;

   /// Look for the declaration with the given name within the
   /// Polarphp module.
   void lookupInSwiftModule(StringRef name,
                            SmallVectorImpl<ValueDecl *> &results) const;

   /// Retrieve a specific, known protocol.
   ProtocolDecl *getProtocol(KnownProtocolKind kind) const;

   /// Determine whether the given nominal type is one of the standard
   /// library or Cocoa framework types that is known to be bridged by another
   /// module's overlay, for layering or implementation detail reasons.
   bool isTypeBridgedInExternalModule(NominalTypeDecl *nominal) const;

   /// Determine whether the given Polarphp type is representable in a
   /// given foreign language.
   ForeignRepresentationInfo
   getForeignRepresentationInfo(NominalTypeDecl *nominal,
                                ForeignLanguage language,
                                const DeclContext *dc);

   /// Add a declaration to a list of declarations that need to be emitted
   /// as part of the current module or source file, but are otherwise not
   /// nested within it.
   void addExternalDecl(Decl *decl);

   /// Add a declaration that was synthesized to a per-source file list if
   /// if is part of a source file, or the external declarations list if
   /// it is part of an imported type context.
   void addSynthesizedDecl(Decl *decl);

   /// Add a cleanup function to be called when the AstContext is deallocated.
   void addCleanup(std::function<void(void)> cleanup);

   /// Add a cleanup to run the given object's destructor when the AstContext is
   /// deallocated.
   template<typename T>
   void addDestructorCleanup(T &object)
   {
      addCleanup([&object]{ object.~T(); });
   }

   /// Get the runtime availability of the opaque types language feature for the target platform.
   AvailabilityContext getOpaqueTypeAvailability();

   /// Get the runtime availability of features introduced in the Polarphp 5.1
   /// compiler for the target platform.
   AvailabilityContext getSwift51Availability();

   //===--------------------------------------------------------------------===//
   // Diagnostics Helper functions
   //===--------------------------------------------------------------------===//

   bool hadError() const;

   //===--------------------------------------------------------------------===//
   // Type manipulation routines.
   //===--------------------------------------------------------------------===//

   // Builtin type and simple types that are used frequently.
   const CanType TheErrorType;             /// This is the ErrorType singleton.
   const CanType TheUnresolvedType;        /// This is the UnresolvedType singleton.
   const CanType TheEmptyTupleType;        /// This is '()', aka Void
   const CanType TheAnyType;               /// This is 'Any', the empty protocol composition
   const CanType TheNativeObjectType;      /// Builtin.NativeObject
   const CanType TheBridgeObjectType;      /// Builtin.BridgeObject
   const CanType TheUnknownObjectType;     /// Builtin.UnknownObject
   const CanType TheRawPointerType;        /// Builtin.RawPointer
   const CanType TheUnsafeValueBufferType; /// Builtin.UnsafeValueBuffer
   const CanType TheSILTokenType;          /// Builtin.SILToken
   const CanType TheIntegerLiteralType;    /// Builtin.IntegerLiteralType

   const CanType TheIEEE32Type;            /// 32-bit IEEE floating point
   const CanType TheIEEE64Type;            /// 64-bit IEEE floating point

   // Target specific types.
   const CanType TheIEEE16Type;            /// 16-bit IEEE floating point
   const CanType TheIEEE80Type;            /// 80-bit IEEE floating point
   const CanType TheIEEE128Type;           /// 128-bit IEEE floating point
   const CanType ThePPC128Type;            /// 128-bit PowerPC 2xDouble

   /// Adds a search path to searchPathOpts, unless it is already present.
   ///
   /// Does any proper bookkeeping to keep all module loaders up to date as well.
   void addSearchPath(StringRef searchPath, bool isFramework, bool isSystem);

   /// Adds a module loader to this AST context.
   ///
   /// \param loader The new module loader, which will be added after any
   ///               existing module loaders.
   /// \param isClang \c true if this module loader is responsible for loading
   ///                Clang modules, which are special-cased in some parts of the
   ///                compiler.
   void addModuleLoader(std::unique_ptr<ModuleLoader> loader,
                        bool isClang = false);

   /// Load extensions to the given nominal type from the external
   /// module loaders.
   ///
   /// \param nominal The nominal type whose extensions should be loaded.
   ///
   /// \param previousGeneration The previous generation number. The AST already
   /// contains extensions loaded from any generation up to and including this
   /// one.
   void loadExtensions(NominalTypeDecl *nominal, unsigned previousGeneration);


   /// Retrieve the Clang module loader for this AstContext.
   ///
   /// If there is no Clang module loader, returns a null pointer.
   /// The loader is owned by the AST context.
   ClangModuleLoader *getClangModuleLoader() const;

   /// Asks every module loader to verify the ASTs it has loaded.
   ///
   /// Does nothing in non-asserts (NDEBUG) builds.
   void verifyAllLoadedModules() const;

   /// Check whether the module with a given name can be imported without
   /// importing it.
   ///
   /// Note that even if this check succeeds, errors may still occur if the
   /// module is loaded in full.
   bool canImportModule(std::pair<Identifier, SourceLoc> modulePath);

   /// \returns a module with a given name that was already loaded.  If the
   /// module was not loaded, returns nullptr.
   ModuleDecl *getLoadedModule(
         ArrayRef<std::pair<Identifier, SourceLoc>> modulePath) const;

   ModuleDecl *getLoadedModule(Identifier ModuleName) const;

   /// Attempts to load a module into this AstContext.
   ///
   /// If a module by this name has already been loaded, the existing module will
   /// be returned.
   ///
   /// \returns The requested module, or NULL if the module cannot be found.
   ModuleDecl *getModule(ArrayRef<std::pair<Identifier, SourceLoc>> modulePath);

   ModuleDecl *getModuleByName(StringRef ModuleName);

   /// Returns the standard library module, or null if the library isn't present.
   ///
   /// If \p loadIfAbsent is true, the AstContext will attempt to load the module
   /// if it hasn't been set yet.
   ModuleDecl *getStdlibModule(bool loadIfAbsent = false);

   ModuleDecl *getStdlibModule() const {
      return const_cast<AstContext *>(this)->getStdlibModule(false);
   }

   /// Retrieve the current generation number, which reflects the
   /// number of times a module import has caused mass invalidation of
   /// lookup tables.
   ///
   /// Various places in the AST keep track of the generation numbers at which
   /// their own information is valid, such as the list of extensions associated
   /// with a nominal type.
   unsigned getCurrentGeneration() const { return m_currentGeneration; }

   /// Increase the generation number, implying that various lookup
   /// tables have been significantly altered by the introduction of a new
   /// module import.
   ///
   /// \returns the previous generation number.
   unsigned bumpGeneration() { return m_currentGeneration++; }

   /// Produce a "normal" conformance for a nominal type.
   NormalProtocolConformance *
   getConformance(Type conformingType,
                  ProtocolDecl *protocol,
                  SourceLoc loc,
                  DeclContext *dc,
                  ProtocolConformanceState state);

   /// A callback used to produce a diagnostic for an ill-formed protocol
   /// conformance that was type-checked before we're actually walking the
   /// conformance itself, along with a bit indicating whether this diagnostic
   /// produces an error.
   struct DelayedConformanceDiag
   {
      ValueDecl *Requirement;
      std::function<void()> Callback;
      bool IsError;
   };

   /// Check whether current context has any errors associated with
   /// ill-formed protocol conformances which haven't been produced yet.
   bool hasDelayedConformanceErrors() const;

   /// Add a delayed diagnostic produced while type-checking a
   /// particular protocol conformance.
   void addDelayedConformanceDiag(NormalProtocolConformance *conformance,
                                  DelayedConformanceDiag fn);

   /// Retrieve the delayed-conformance diagnostic callbacks for the
   /// given normal protocol conformance.
   std::vector<DelayedConformanceDiag>
   takeDelayedConformancediags(NormalProtocolConformance *conformance);

   /// Add delayed missing witnesses for the given normal protocol conformance.
   void addDelayedMissingWitnesses(NormalProtocolConformance *conformance,
                                   ArrayRef<ValueDecl*> witnesses);

   /// Retrieve the delayed missing witnesses for the given normal protocol
   /// conformance.
   std::vector<ValueDecl*>
   takeDelayedMissingWitnesses(NormalProtocolConformance *conformance);

   /// Produce a specialized conformance, which takes a generic
   /// conformance and substitutions written in terms of the generic
   /// conformance's signature.
   ///
   /// \param type The type for which we are retrieving the conformance.
   ///
   /// \param generic The generic conformance.
   ///
   /// \param substitutions The set of substitutions required to produce the
   /// specialized conformance from the generic conformance.
   ProtocolConformance *
   getSpecializedConformance(Type type,
                             ProtocolConformance *generic,
                             SubstitutionMap substitutions);

   /// Produce an inherited conformance, for subclasses of a type
   /// that already conforms to a protocol.
   ///
   /// \param type The type for which we are retrieving the conformance.
   ///
   /// \param inherited The inherited conformance.
   InheritedProtocolConformance *
   getInheritedConformance(Type type, ProtocolConformance *inherited);

   //   /// Get the lazy data for the given declaration.
   //   ///
   //   /// \param lazyLoader If non-null, the lazy loader to use when creating the
   //   /// lazy data. The pointer must either be null or be consistent
   //   /// across all calls for the same \p func.
   //   LazyContextdata *getOrCreateLazyContextdata(const DeclContext *decl,
   //                                               LazyMemberLoader *lazyLoader);

   /// Use the lazy parsers associated with the context to populate the members
   /// of the given decl context.
   ///
   /// \param IDC The context whose member decls should be lazily parsed.
   void parseMembers(IterableDeclContext *IDC);

   /// Use the lazy parsers associated with the context to check whether the decl
   /// context has been parsed.
   bool hasUnparsedMembers(const IterableDeclContext *IDC) const;

   /// Access the side cache for property wrapper backing property types,
   /// used because TypeChecker::typeCheckBinding() needs somewhere to stash
   /// the backing property type.
   Type getSideCachedPropertyWrapperBackingPropertyType(VarDecl *var) const;
   void setSideCachedPropertyWrapperBackingPropertyType(VarDecl *var,
                                                        Type type);

   /// Returns memory usage of this AstContext.
   size_t getTotalMemory() const;

   /// Returns memory used exclusively by constraint solver.
   size_t getSolverMemory() const;

   /// Populate \p names with visible top level module names.
   /// This guarantees that resulted \p names doesn't have duplicated names.
   void getVisibleTopLevelModuleNames(SmallVectorImpl<Identifier> &names) const;

public:

   //   /// Whether our effective Polarphp version is at least 'major'.
   //   ///
   //   /// This is usually the check you want; for example, when introducing
   //   /// a new language feature which is only visible in Polarphp 5, you would
   //   /// check for isSwiftVersionAtLeast(5).
   //   bool isPolarphpVersionAtLeast(unsigned major, unsigned minor = 0) const {
   //      return langOpts.isPolarphpVersionAtLeast(major, minor);
   //   }

   /// Each kind and SourceFile has its own cache for a Type.
   Type &getDefaultTypeRequestCache(SourceFile *, KnownProtocolKind);

private:
   friend Decl;
   friend TypeBase;
   friend ArchetypeType;
   friend OpaqueTypeDecl;

   /// Provide context-level uniquing for SIL lowered type layouts and boxes.
   friend PILLayout;
   friend PILBoxType;
};


/// Attach Fix-Its to the given diagnostic that updates the name of the
/// given declaration to the desired target name.
///
/// \returns false if the name could not be fixed.
bool fix_declaration_name(InFlightDiagnostic &diag, ValueDecl *decl,
                          DeclName targetName);


} // polar::ast

#endif // POLARPHP_AST_ASTCONTEXT_H


