//===--- AstContext.h - AST Context Object ----------------------*- C++ -*-===//
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
// This file defines the AstContext interface.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ASTCONTEXT_H
#define POLARPHP_AST_ASTCONTEXT_H

#include "polarphp/ast/Evaluator.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/SearchPathOptions.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/Types.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/LangOptions.h"
#include "polarphp/basic/Malloc.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/DataTypes.h"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace clang {
class Decl;
class MacroInfo;
class Module;
class ObjCInterfaceDecl;
}

namespace polar::syntax {
class SyntaxArena;
}

namespace polar {

using polar::syntax::SyntaxArena;

class SourceManager;
class UnifiedStatsReporter;
class SourceLoc;
class AbstractFunctionDecl;
class AstContext;
enum class Associativity : unsigned char;
class AvailabilityContext;
class BoundGenericType;
class ClangModuleLoader;
class ClangNode;
class ConcreteDeclRef;
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
class LazyContextData;
class LazyIterableDeclContextData;
class LazyMemberLoader;
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
class NormalInterfaceConformance;
class OpaqueTypeDecl;
class InheritedInterfaceConformance;
class SelfInterfaceConformance;
class SpecializedInterfaceConformance;
enum class InterfaceConformanceState;
class Pattern;
enum PointerTypeKind : unsigned;
class PrecedenceGroupDecl;
class TupleTypeElt;
class EnumElementDecl;
class InterfaceDecl;
class SubstitutableType;
class ValueDecl;
class DiagnosticEngine;
class TypeChecker;
class TypeCheckerDebugConsumer;
struct RawComment;
class DocComment;
class PILBoxType;
class TypeAliasDecl;
class VarDecl;
class IndexSubset;

enum class KnownInterfaceKind : uint8_t;

namespace namelookup {
class ImportCache;
}

/// The arena in which a particular AstContext allocation will go.
enum class AllocationArena {
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

/// Lists the set of "known" Foundation entities that are used in the
/// compiler.
///
/// While the names of Foundation types aren't likely to change in
/// Objective-C, their mapping into Swift can. Therefore, when
/// referring to names of Foundation entities in Swift, use this enum
/// and \c AstContext::getSwiftName or \c AstContext::getSwiftId.
//enum class KnownFoundationEntity {
//#define FOUNDATION_ENTITY(Name) Name,
//#include "polarphp/ast/KnownFoundationEntities.def"
//};

/// Retrieve the Foundation entity kind for the given Objective-C
/// entity name.
//Optional<KnownFoundationEntity> getKnownFoundationEntity(StringRef name);

/// Introduces a new constraint checker arena, whose lifetime is
/// tied to the lifetime of this RAII object.
class ConstraintCheckerArenaRAII {
   AstContext &Self;
   void *Data;

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

class AstContext final {
   AstContext(const AstContext&) = delete;
   void operator=(const AstContext&) = delete;

   AstContext(LangOptions &langOpts, TypeCheckerOptions &typeckOpts,
              SearchPathOptions &SearchPathOpts, SourceManager &SourceMgr,
              DiagnosticEngine &Diags);

public:
   // Members that should only be used by AstContext.cpp.
   struct Implementation;
   Implementation &getImpl() const;

   friend ConstraintCheckerArenaRAII;

   void operator delete(void *Data) throw();

   static AstContext *get(LangOptions &langOpts, TypeCheckerOptions &typeckOpts,
                          SearchPathOptions &SearchPathOpts,
                          SourceManager &SourceMgr, DiagnosticEngine &Diags);
   ~AstContext();

   /// Optional table of counters to report, nullptr when not collecting.
   ///
   /// This must be initialized early so that Allocate() doesn't try to access
   /// it before being set to null.
   UnifiedStatsReporter *Stats = nullptr;

   /// The language options used for translation.
   LangOptions &LangOpts;

   /// The type checker options.
   TypeCheckerOptions &TypeCheckerOpts;

   /// The search path options used by this AST context.
   SearchPathOptions &SearchPathOpts;

   /// The source manager object.
   SourceManager &SourceMgr;

   /// Diags - The diagnostics engine.
   DiagnosticEngine &Diags;

   /// The request-evaluator that is used to process various requests.
   Evaluator evaluator;

   /// The set of top-level modules we have loaded.
   /// This map is used for iteration, therefore it's a MapVector and not a
   /// DenseMap.
   llvm::MapVector<Identifier, ModuleDecl*> LoadedModules;

   /// The builtin module.
   ModuleDecl * const TheBuiltinModule;

   /// The standard library module.
   mutable ModuleDecl *TheStdlibModule = nullptr;

   /// The name of the standard library module "Swift".
   Identifier StdlibModuleName;

   /// The name of the SwiftShims module "SwiftShims".
   Identifier SwiftShimsModuleName;

   // Define the set of known identifiers.
#define IDENTIFIER_WITH_NAME(Name, IdStr) Identifier Id_##Name;
#include "polarphp/ast/KnownIdentifiersDef.h"

   /// A consumer of type checker debug output.
   std::unique_ptr<TypeCheckerDebugConsumer> TypeCheckerDebug;

   /// Cache for names of canonical GenericTypeParamTypes.
   mutable llvm::DenseMap<unsigned, Identifier>
      CanonicalGenericTypeParamTypeNames;

   /// Cache of remapped types (useful for diagnostics).
   llvm::StringMap<Type> RemappedTypes;

   /// The # of times we have performed typo correction.
   unsigned NumTypoCorrections = 0;

   /// The next auto-closure discriminator.  This needs to be preserved
   /// across invocations of both the parser and the type-checker.
   unsigned NextAutoClosureDiscriminator = 0;

private:
   /// The current generation number, which reflects the number of
   /// times that external modules have been loaded.
   ///
   /// Various places in the AST, such as the set of extensions associated with
   /// a nominal type, keep track of the generation number they saw and will
   /// automatically update when they are out of date.
   unsigned CurrentGeneration = 0;

   friend class Pattern;

   /// Mapping from patterns that store interface types that will be lazily
   /// resolved to contextual types, to the declaration context in which the
   /// pattern resides.
   llvm::DenseMap<const Pattern *, DeclContext *>
      DelayedPatternContexts;

   /// Cache of module names that fail the 'canImport' test in this context.
   llvm::SmallPtrSet<Identifier, 8> FailedModuleImportNames;

   /// Retrieve the allocator for the given arena.
   llvm::BumpPtrAllocator &
   getAllocator(AllocationArena arena = AllocationArena::Permanent) const;

public:
   /// Allocate - Allocate memory from the AstContext bump pointer.
   void *Allocate(unsigned long bytes, unsigned alignment,
                  AllocationArena arena = AllocationArena::Permanent) const {
      if (bytes == 0)
         return nullptr;

      if (LangOpts.UseMalloc)
         return aligned_alloc(bytes, alignment);

      if (arena == AllocationArena::Permanent && Stats)
         Stats->getFrontendCounters().NumAstBytesAllocated += bytes;
      return getAllocator(arena).Allocate(bytes, alignment);
   }

   template <typename T>
   T *Allocate(AllocationArena arena = AllocationArena::Permanent) const {
      T *res = (T *) Allocate(sizeof(T), alignof(T), arena);
      new (res) T();
      return res;
   }

   template <typename T>
   MutableArrayRef<T> AllocateUninitialized(unsigned NumElts,
                                            AllocationArena Arena = AllocationArena::Permanent) const {
      T *Data = (T *) Allocate(sizeof(T) * NumElts, alignof(T), Arena);
      return { Data, NumElts };
   }

   template <typename T>
   MutableArrayRef<T> Allocate(unsigned numElts,
                               AllocationArena arena = AllocationArena::Permanent) const {
      T *res = (T *) Allocate(sizeof(T) * numElts, alignof(T), arena);
      for (unsigned i = 0; i != numElts; ++i)
         new (res+i) T();
      return {res, numElts};
   }

   /// Allocate a copy of the specified object.
   template <typename T>
   typename std::remove_reference<T>::type *AllocateObjectCopy(T &&t,
                                                               AllocationArena arena = AllocationArena::Permanent) const {
      // This function cannot be named AllocateCopy because it would always win
      // overload resolution over the AllocateCopy(ArrayRef<T>).
      using TNoRef = typename std::remove_reference<T>::type;
      TNoRef *res = (TNoRef *) Allocate(sizeof(TNoRef), alignof(TNoRef), arena);
      new (res) TNoRef(std::forward<T>(t));
      return res;
   }

   template <typename T, typename It>
   T *AllocateCopy(It start, It end,
                   AllocationArena arena = AllocationArena::Permanent) const {
      T *res = (T*)Allocate(sizeof(T)*(end-start), alignof(T), arena);
      for (unsigned i = 0; start != end; ++start, ++i)
         new (res+i) T(*start);
      return res;
   }

   template<typename T, size_t N>
   MutableArrayRef<T> AllocateCopy(T (&array)[N],
                                   AllocationArena arena = AllocationArena::Permanent) const {
      return MutableArrayRef<T>(AllocateCopy<T>(array, array+N, arena), N);
   }

   template<typename T>
   MutableArrayRef<T> AllocateCopy(ArrayRef<T> array,
                                   AllocationArena arena = AllocationArena::Permanent) const {
      return MutableArrayRef<T>(AllocateCopy<T>(array.begin(),array.end(), arena),
                                array.size());
   }


   template<typename T>
   ArrayRef<T> AllocateCopy(const SmallVectorImpl<T> &vec,
                            AllocationArena arena = AllocationArena::Permanent) const {
      return AllocateCopy(ArrayRef<T>(vec), arena);
   }

   template<typename T>
   MutableArrayRef<T>
   AllocateCopy(SmallVectorImpl<T> &vec,
                AllocationArena arena = AllocationArena::Permanent) const {
      return AllocateCopy(MutableArrayRef<T>(vec), arena);
   }

   StringRef AllocateCopy(StringRef Str,
                          AllocationArena arena = AllocationArena::Permanent) const {
      ArrayRef<char> Result =
         AllocateCopy(llvm::makeArrayRef(Str.data(), Str.size()), arena);
      return StringRef(Result.data(), Result.size());
   }

   template<typename T, typename Vector, typename Set>
   MutableArrayRef<T>
   AllocateCopy(llvm::SetVector<T, Vector, Set> setVector,
                AllocationArena arena = AllocationArena::Permanent) const {
      return MutableArrayRef<T>(AllocateCopy<T>(setVector.begin(),
                                   setVector.end(),
                                   arena),
                                setVector.size());
   }

   /// Retrive the syntax node memory manager for this context.
   llvm::IntrusiveRefCntPtr<SyntaxArena> getSyntaxArena() const;

   /// Set a new stats reporter.
   void setStatsReporter(UnifiedStatsReporter *stats);

private:
   friend class TypeChecker;

   void installGlobalTypeChecker(TypeChecker *TC);
public:
   /// Returns if semantic AST queries are enabled. This generally means module
   /// loading and name lookup can take place.
   bool areSemanticQueriesEnabled() const;

   /// Retrieve the global \c TypeChecker instance associated with this context.
   TypeChecker *getLegacyGlobalTypeChecker() const;

   /// getIdentifier - Return the uniqued and AST-Context-owned version of the
   /// specified string.
   Identifier getIdentifier(StringRef Str) const;

   /// Decide how to interpret two precedence groups.
   Associativity associateInfixOperators(PrecedenceGroupDecl *left,
                                         PrecedenceGroupDecl *right) const;

   /// Retrieve the declaration of Swift.Error.
   InterfaceDecl *getErrorDecl() const;
   CanType getExceptionType() const;

#define KNOWN_STDLIB_TYPE_DECL(NAME, DECL_CLASS, NUM_GENERIC_PARAMS) \
  /** Retrieve the declaration of Swift.NAME. */ \
  DECL_CLASS *get##NAME##Decl() const;
#include "polarphp/ast/KnownStdlibTypesDef.h"

   /// Retrieve the declaration of Swift.Optional<T>.Some.
   EnumElementDecl *getOptionalSomeDecl() const;

   /// Retrieve the declaration of Swift.Optional<T>.None.
   EnumElementDecl *getOptionalNoneDecl() const;

   /// Retrieve the declaration of the "pointee" property of a pointer type.
   VarDecl *getPointerPointeePropertyDecl(PointerTypeKind ptrKind) const;

   /// Retrieve the type Swift.AnyObject.
   CanType getAnyObjectType() const;

   /// Retrieve the type Swift.Never.
   CanType getNeverType() const;

   // Declare accessors for the known declarations.
#define FUNC_DECL(Name, Id) \
  FuncDecl *get##Name() const;
#include "polarphp/ast/KnownDeclsDef.h"

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

   /// Retrieve the declaration of Swift.Bool.init(_builtinBooleanLiteral:)
   ConcreteDeclRef getBoolBuiltinInitDecl() const;

   /// Retrieve the witness for init(_builtinIntegerLiteral:).
   ConcreteDeclRef getIntBuiltinInitDecl(NominalTypeDecl *intDecl) const;

   /// Retrieve the witness for init(_builtinFloatLiteral:).
   ConcreteDeclRef getFloatBuiltinInitDecl(NominalTypeDecl *floatDecl) const;

   /// Retrieve the witness for (_builtinStringLiteral:utf8CodeUnitCount:isASCII:).
   ConcreteDeclRef getStringBuiltinInitDecl(NominalTypeDecl *stringDecl) const;

   ConcreteDeclRef getBuiltinInitDecl(NominalTypeDecl *decl,
                                      KnownInterfaceKind builtinInterface,
                                      llvm::function_ref<DeclName (AstContext &ctx)> initName) const;

   /// Retrieve the declaration of Swift.==(Int, Int) -> Bool.
   FuncDecl *getEqualIntDecl() const;

   /// Retrieve the declaration of Swift._hashValue<H>(for: H) -> Int.
   FuncDecl *getHashValueForDecl() const;

   /// Retrieve the declaration of Array.append(element:)
   FuncDecl *getArrayAppendElementDecl() const;

   /// Retrieve the declaration of
   /// Array.reserveCapacityForAppend(newElementsCount: Int)
   FuncDecl *getArrayReserveCapacityDecl() const;

   // Retrieve the declaration of Swift._stdlib_isOSVersionAtLeast.
   FuncDecl *getIsOSVersionAtLeastDecl() const;

   /// Look for the declaration with the given name within the
   /// Swift module.
   void lookupInPolarphpModule(StringRef name,
                            SmallVectorImpl<ValueDecl *> &results) const;

   /// Retrieve a specific, known protocol.
   InterfaceDecl *getInterface(KnownInterfaceKind kind) const;

   /// Get the Clang type corresponding to a Swift function type.
   ///
   /// \param params The function parameters.
   /// \param resultTy The Swift result type.
   /// \param incompleteExtInfo Used to convey escaping and throwing
   ///                          information, in case it is needed.
   /// \param trueRep The actual calling convention, which must be C-compatible.
   ///                The calling convention in \p incompleteExtInfo is ignored.
   const clang::Type *
   getClangFunctionType(ArrayRef<AnyFunctionType::Param> params, Type resultTy,
                        const FunctionType::ExtInfo incompleteExtInfo,
                        FunctionTypeRepresentation trueRep);

   /// Determine whether the given Swift type is representable in a
   /// given foreign language.
   ForeignRepresentationInfo
   getForeignRepresentationInfo(NominalTypeDecl *nominal,
                                ForeignLanguage language,
                                const DeclContext *dc);

   /// Add a cleanup function to be called when the AstContext is deallocated.
   void addCleanup(std::function<void(void)> cleanup);

   /// Add a cleanup to run the given object's destructor when the AstContext is
   /// deallocated.
   template<typename T>
   void addDestructorCleanup(T &object) {
      addCleanup([&object]{ object.~T(); });
   }

   /// Get the runtime availability of the opaque types language feature for the target platform.
   AvailabilityContext getOpaqueTypeAvailability();

   /// Get the runtime availability of features introduced in the Swift 5.1
   /// compiler for the target platform.
   AvailabilityContext getPolarphp51Availability();

   /// Get the runtime availability of
   /// swift_getTypeByMangledNameInContextInMetadataState.
   AvailabilityContext getTypesInAbstractMetadataStateAvailability();


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
   const CanType TheRawPointerType;        /// Builtin.RawPointer
   const CanType TheUnsafeValueBufferType; /// Builtin.UnsafeValueBuffer
   const CanType ThePILTokenType;          /// Builtin.PILToken
   const CanType TheIntegerLiteralType;    /// Builtin.IntegerLiteralType

   const CanType TheIEEE32Type;            /// 32-bit IEEE floating point
   const CanType TheIEEE64Type;            /// 64-bit IEEE floating point

   // Target specific types.
   const CanType TheIEEE16Type;            /// 16-bit IEEE floating point
   const CanType TheIEEE80Type;            /// 80-bit IEEE floating point
   const CanType TheIEEE128Type;           /// 128-bit IEEE floating point
   const CanType ThePPC128Type;            /// 128-bit PowerPC 2xDouble

   /// Adds a search path to SearchPathOpts, unless it is already present.
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
   /// \param isDWARF \c true if this module loader can load Clang modules
   ///                from DWARF.
   void addModuleLoader(std::unique_ptr<ModuleLoader> loader,
                        bool isClang = false, bool isDWARF = false);

   /// Load extensions to the given nominal type from the external
   /// module loaders.
   ///
   /// \param nominal The nominal type whose extensions should be loaded.
   ///
   /// \param previousGeneration The previous generation number. The AST already
   /// contains extensions loaded from any generation up to and including this
   /// one.
   void loadExtensions(NominalTypeDecl *nominal, unsigned previousGeneration);

   /// Load the methods within the given class that produce
   /// Objective-C class or instance methods with the given selector.
   ///
   /// \param classDecl The class in which we are searching for @objc methods.
   /// The search only considers this class and its extensions; not any
   /// superclasses.
   ///
   /// \param selector The selector to search for.
   ///
   /// \param isInstanceMethod Whether we are looking for an instance method
   /// (vs. a class method).
   ///
   /// \param previousGeneration The previous generation with which this
   /// callback was invoked. The list of methods will already contain all of
   /// the results from generations up and including \c previousGeneration.
   ///
   /// \param methods The list of @objc methods in this class that have this
   /// selector and are instance/class methods as requested. This list will be
   /// extended with any methods found in subsequent generations.
//   void loadObjCMethods(ClassDecl *classDecl,
//                        ObjCSelector selector,
//                        bool isInstanceMethod,
//                        unsigned previousGeneration,
//                        llvm::TinyPtrVector<AbstractFunctionDecl *> &methods);

   /// Retrieve the Clang module loader for this AstContext.
   ///
   /// If there is no Clang module loader, returns a null pointer.
   /// The loader is owned by the AST context.
   ClangModuleLoader *getClangModuleLoader() const;

   /// Retrieve the DWARF module loader for this AstContext.
   ///
   /// If there is no Clang module loader, returns a null pointer.
   /// The loader is owned by the AST context.
   ClangModuleLoader *getDWARFModuleLoader() const;

   namelookup::ImportCache &getImportCache() const;

   /// Asks every module loader to verify the ASTs it has loaded.
   ///
   /// Does nothing in non-asserts (NDEBUG) builds.
   void verifyAllLoadedModules() const;

   /// Check whether the module with a given name can be imported without
   /// importing it.
   ///
   /// Note that even if this check succeeds, errors may still occur if the
   /// module is loaded in full.
   bool canImportModule(std::pair<Identifier, SourceLoc> ModulePath);

   /// \returns a module with a given name that was already loaded.  If the
   /// module was not loaded, returns nullptr.
   ModuleDecl *getLoadedModule(
      ArrayRef<std::pair<Identifier, SourceLoc>> ModulePath) const;

   ModuleDecl *getLoadedModule(Identifier ModuleName) const;

   /// Attempts to load a module into this AstContext.
   ///
   /// If a module by this name has already been loaded, the existing module will
   /// be returned.
   ///
   /// \returns The requested module, or NULL if the module cannot be found.
   ModuleDecl *getModule(ArrayRef<std::pair<Identifier, SourceLoc>> ModulePath);

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
   unsigned getCurrentGeneration() const { return CurrentGeneration; }

   /// Increase the generation number, implying that various lookup
   /// tables have been significantly altered by the introduction of a new
   /// module import.
   ///
   /// \returns the previous generation number.
   unsigned bumpGeneration() { return CurrentGeneration++; }

   /// Produce a "normal" conformance for a nominal type.
   NormalInterfaceConformance *
   getConformance(Type conformingType,
                  InterfaceDecl *protocol,
                  SourceLoc loc,
                  DeclContext *dc,
                  InterfaceConformanceState state);

   /// Produce a self-conformance for the given protocol.
   SelfInterfaceConformance *
   getSelfConformance(InterfaceDecl *protocol);

   /// A callback used to produce a diagnostic for an ill-formed protocol
   /// conformance that was type-checked before we're actually walking the
   /// conformance itself, along with a bit indicating whether this diagnostic
   /// produces an error.
   struct DelayedConformanceDiag {
      ValueDecl *Requirement;
      std::function<void()> Callback;
      bool IsError;
   };

   /// Check whether current context has any errors associated with
   /// ill-formed protocol conformances which haven't been produced yet.
   bool hasDelayedConformanceErrors() const;

   /// Add a delayed diagnostic produced while type-checking a
   /// particular protocol conformance.
   void addDelayedConformanceDiag(NormalInterfaceConformance *conformance,
                                  DelayedConformanceDiag fn);

   /// Retrieve the delayed-conformance diagnostic callbacks for the
   /// given normal protocol conformance.
   std::vector<DelayedConformanceDiag>
   takeDelayedConformanceDiags(NormalInterfaceConformance *conformance);

   /// Add delayed missing witnesses for the given normal protocol conformance.
   void addDelayedMissingWitnesses(NormalInterfaceConformance *conformance,
                                   ArrayRef<ValueDecl*> witnesses);

   /// Retrieve the delayed missing witnesses for the given normal protocol
   /// conformance.
   std::vector<ValueDecl*>
   takeDelayedMissingWitnesses(NormalInterfaceConformance *conformance);

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
   InterfaceConformance *
   getSpecializedConformance(Type type,
                             InterfaceConformance *generic,
                             SubstitutionMap substitutions);

   /// Produce an inherited conformance, for subclasses of a type
   /// that already conforms to a protocol.
   ///
   /// \param type The type for which we are retrieving the conformance.
   ///
   /// \param inherited The inherited conformance.
   InheritedInterfaceConformance *
   getInheritedConformance(Type type, InterfaceConformance *inherited);

   /// Get the lazy data for the given declaration.
   ///
   /// \param lazyLoader If non-null, the lazy loader to use when creating the
   /// lazy data. The pointer must either be null or be consistent
   /// across all calls for the same \p func.
   LazyContextData *getOrCreateLazyContextData(const DeclContext *decl,
                                               LazyMemberLoader *lazyLoader);

   /// Get the lazy iterable context for the given iterable declaration context.
   ///
   /// \param lazyLoader If non-null, the lazy loader to use when creating the
   /// iterable context data. The pointer must either be null or be consistent
   /// across all calls for the same \p idc.
   LazyIterableDeclContextData *getOrCreateLazyIterableContextData(
      const IterableDeclContext *idc,
      LazyMemberLoader *lazyLoader);

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

   // @todo
//   /// Retrieve the Swift name for the given Foundation entity, where
//    /// "NS" prefix stripping will apply under omit-needless-words.
//    StringRef getSwiftName(KnownFoundationEntity kind);
//
//    /// Retrieve the Swift identifier for the given Foundation entity, where
//    /// "NS" prefix stripping will apply under omit-needless-words.
//    Identifier getSwiftId(KnownFoundationEntity kind) {
//      return getIdentifier(getSwiftName(kind));
//    }

   /// Populate \p names with visible top level module names.
   /// This guarantees that resulted \p names doesn't have duplicated names.
   void getVisibleTopLevelModuleNames(SmallVectorImpl<Identifier> &names) const;

   /// Whether to perform typo correction given the pre-configured correction limit.
   /// Increments \c NumTypoCorrections then checks this against the limit in
   /// the language options.
   bool shouldPerformTypoCorrection();

private:
   /// Register the given generic signature builder to be used as the canonical
   /// generic signature builder for the given signature, if we don't already
   /// have one.
   void registerGenericSignatureBuilder(GenericSignature sig,
                                        GenericSignatureBuilder &&builder);
   friend class GenericSignatureBuilder;

public:
   /// Retrieve or create the stored generic signature builder for the given
   /// canonical generic signature and module.
   GenericSignatureBuilder *getOrCreateGenericSignatureBuilder(
      CanGenericSignature sig);

   /// Retrieve a generic signature with a single unconstrained type parameter,
   /// like `<T>`.
   CanGenericSignature getSingleGenericParameterSignature() const;

   /// Retrieve a generic signature with a single type parameter conforming
   /// to the given opened archetype.
   CanGenericSignature getOpenedArchetypeSignature(CanType existential,
                                                   ModuleDecl *mod);

   GenericSignature getOverrideGenericSignature(const ValueDecl *base,
                                                const ValueDecl *derived);

   enum class OverrideGenericSignatureReqCheck {
      /// Base method's generic requirements are satisifed by derived method
         BaseReqSatisfiedByDerived,

      /// Derived method's generic requirements are satisifed by base method
         DerivedReqSatisfiedByBase
   };

   bool overrideGenericSignatureReqsSatisfied(
      const ValueDecl *base, const ValueDecl *derived,
      const OverrideGenericSignatureReqCheck direction);

   /// Whether our effective Swift version is at least 'major'.
   ///
   /// This is usually the check you want; for example, when introducing
   /// a new language feature which is only visible in Swift 5, you would
   /// check for isPolarVersionAtLeast(5).
   bool isPolarphpVersionAtLeast(unsigned major, unsigned minor = 0) const {
      return LangOpts.isPolarphpVersionAtLeast(major, minor);
   }

   /// Check whether it's important to respect access control restrictions
   /// in current context.
   bool isAccessControlDisabled() const {
      return !LangOpts.EnableAccessControl;
   }

   /// Each kind and SourceFile has its own cache for a Type.
   Type &getDefaultTypeRequestCache(SourceFile *, KnownInterfaceKind);

private:
   friend Decl;
   Optional<RawComment> getRawComment(const Decl *D);
   void setRawComment(const Decl *D, RawComment RC);

   Optional<StringRef> getBriefComment(const Decl *D);
   void setBriefComment(const Decl *D, StringRef Comment);

   friend TypeBase;
   friend ArchetypeType;
   friend OpaqueTypeDecl;

   /// Provide context-level uniquing for PIL lowered type layouts and boxes.
   friend PILLayout;
   friend PILBoxType;
};

} // end namespace polar

#endif
