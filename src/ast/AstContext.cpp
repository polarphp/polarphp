//===--- AstContext.cpp - AstContext Implementation -----------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This file implements the AstContext class.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/internal/ClangTypeConverter.h"
#include "polarphp/ast/internal/ForeignRepresentationInfo.h"
#include "polarphp/ast/internal/SubstitutionMapStorage.h"
#include "polarphp/ast/ClangModuleLoader.h"
#include "polarphp/ast/ConcreteDeclRef.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsSema.h"
#include "polarphp/ast/ExistentialLayout.h"
#include "polarphp/ast/FileUnit.h"
#include "polarphp/ast/ForeignErrorConvention.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/GenericSignatureBuilder.h"
#include "polarphp/ast/ImportCache.h"
#include "polarphp/ast/IndexSubset.h"
#include "polarphp/ast/KnownInterfaces.h"
#include "polarphp/ast/LazyResolver.h"
#include "polarphp/ast/ModuleLoader.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/PrettyStackTrace.h"
#include "polarphp/ast/PropertyWrappers.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/RawComment.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/SubstitutionMap.h"
#include "polarphp/ast/PILLayout.h"
#include "polarphp/ast/TypeCheckRequests.h"
#include "polarphp/ast/TypeCheckerDebugConsumer.h"
#include "polarphp/basic/SourceMgr.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/StringExtras.h"
#include "polarphp/syntax/References.h"
#include "polarphp/syntax/SyntaxArena.h"
#include "polarphp/global/NameStrings.h"
#include "polarphp/global/Subsystems.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/Compiler.h"
#include <algorithm>
#include <memory>

namespace polar {

using namespace llvm;
using polar::syntax::SyntaxArena;
using polar::syntax::RefCountPtr;

#define DEBUG_TYPE "AstContext"
STATISTIC(NumRegisteredGenericSignatureBuilders,
          "# of generic signature builders successfully registered");
STATISTIC(NumRegisteredGenericSignatureBuildersAlready,
          "# of generic signature builders already registered");
STATISTIC(NumCollapsedSpecializedInterfaceConformances,
          "# of specialized protocol conformances collapsed");

/// Define this to 1 to enable expensive assertions of the
/// GenericSignatureBuilder.
#define POLAR_GSB_EXPENSIVE_ASSERTIONS 0

void ModuleLoader::anchor() {}

void ClangModuleLoader::anchor() {}

llvm::StringRef get_interface_name(KnownInterfaceKind kind) {
   switch (kind) {
#define INTERFACE_WITH_NAME(Id, Name) \
  case KnownInterfaceKind::Id: \
    return Name;

#include "polarphp/ast/KnownInterfacesDef.h"
   }
   llvm_unreachable("bad KnownInterfaceKind");
}

namespace {
enum class SearchPathKind : uint8_t {
   Import = 1 << 0,
   Framework = 1 << 1
};
} // end anonymous namespace

using AssociativityCacheType =
llvm::DenseMap<std::pair<PrecedenceGroupDecl *, PrecedenceGroupDecl *>,
   Associativity>;

struct OverrideSignatureKey {
   GenericSignature baseMethodSig;
   GenericSignature derivedClassSig;
   Type superclassTy;

   OverrideSignatureKey(GenericSignature baseMethodSignature,
                        GenericSignature derivedClassSignature,
                        Type superclassType)
      : baseMethodSig(baseMethodSignature),
        derivedClassSig(derivedClassSignature), superclassTy(superclassType) {}
};

} // polar

namespace llvm {

using polar::OverrideSignatureKey;

template<>
struct DenseMapInfo<OverrideSignatureKey> {
   using Type = polar::Type;
   using GenericSignature = polar::GenericSignature;

   static bool isEqual(const OverrideSignatureKey lhs,
                       const OverrideSignatureKey rhs) {
      return lhs.baseMethodSig.getPointer() == rhs.baseMethodSig.getPointer() &&
             lhs.derivedClassSig.getPointer() == rhs.derivedClassSig.getPointer() &&
             lhs.superclassTy.getPointer() == rhs.superclassTy.getPointer();
   }

   static inline OverrideSignatureKey getEmptyKey() {
      return OverrideSignatureKey(DenseMapInfo<GenericSignature>::getEmptyKey(),
                                  DenseMapInfo<GenericSignature>::getEmptyKey(),
                                  DenseMapInfo<Type>::getEmptyKey());
   }

   static inline OverrideSignatureKey getTombstoneKey() {
      return OverrideSignatureKey(
         DenseMapInfo<GenericSignature>::getTombstoneKey(),
         DenseMapInfo<GenericSignature>::getTombstoneKey(),
         DenseMapInfo<Type>::getTombstoneKey());
   }

   static unsigned getHashValue(const OverrideSignatureKey &Val) {
      return hash_combine(
         DenseMapInfo<GenericSignature>::getHashValue(Val.baseMethodSig),
         DenseMapInfo<GenericSignature>::getHashValue(Val.derivedClassSig),
         DenseMapInfo<Type>::getHashValue(Val.superclassTy));
   }
};
} // namespace llvm

namespace polar {

struct AstContext::Implementation {
   Implementation();

   ~Implementation();

   llvm::BumpPtrAllocator Allocator; // used in later initializations

   /// The set of cleanups to be called when the AstContext is destroyed.
   std::vector<std::function<void(void)>> Cleanups;

   /// A global type checker instance..
   TypeChecker *Checker = nullptr;

   // FIXME: This is a StringMap rather than a StringSet because StringSet
   // doesn't allow passing in a pre-existing allocator.
   llvm::StringMap<Identifier::Aligner, llvm::BumpPtrAllocator &>
      IdentifierTable;

   /// The declaration of Swift.AssignmentPrecedence.
   PrecedenceGroupDecl *AssignmentPrecedence = nullptr;

   /// The declaration of Swift.CastingPrecedence.
   PrecedenceGroupDecl *CastingPrecedence = nullptr;

   /// The declaration of Swift.FunctionArrowPrecedence.
   PrecedenceGroupDecl *FunctionArrowPrecedence = nullptr;

   /// The declaration of Swift.TernaryPrecedence.
   PrecedenceGroupDecl *TernaryPrecedence = nullptr;

   /// The declaration of Swift.DefaultPrecedence.
   PrecedenceGroupDecl *DefaultPrecedence = nullptr;

   /// The AnyObject type.
   CanType AnyObjectType;

#define KNOWN_STDLIB_TYPE_DECL(NAME, DECL_CLASS, NUM_GENERIC_PARAMS) \
  /** The declaration of Swift.NAME. */ \
  DECL_CLASS *NAME##Decl = nullptr;

#include "polarphp/ast/KnownStdlibTypesDef.h"

   /// The declaration of '+' function for two RangeReplaceableCollection.
   FuncDecl *PlusFunctionOnRangeReplaceableCollection = nullptr;

   /// The declaration of '+' function for two String.
   FuncDecl *PlusFunctionOnString = nullptr;

   /// The declaration of Swift.Optional<T>.Some.
   EnumElementDecl *OptionalSomeDecl = nullptr;

   /// The declaration of Swift.Optional<T>.None.
   EnumElementDecl *OptionalNoneDecl = nullptr;

   /// The declaration of Swift.UnsafeMutableRawPointer.memory.
   VarDecl *UnsafeMutableRawPointerMemoryDecl = nullptr;

   /// The declaration of Swift.UnsafeRawPointer.memory.
   VarDecl *UnsafeRawPointerMemoryDecl = nullptr;

   /// The declaration of Swift.UnsafeMutablePointer<T>.memory.
   VarDecl *UnsafeMutablePointerMemoryDecl = nullptr;

   /// The declaration of Swift.UnsafePointer<T>.memory.
   VarDecl *UnsafePointerMemoryDecl = nullptr;

   /// The declaration of Swift.AutoreleasingUnsafeMutablePointer<T>.memory.
   VarDecl *AutoreleasingUnsafeMutablePointerMemoryDecl = nullptr;

   // Declare cached declarations for each of the known declarations.
#define FUNC_DECL(Name, Id) FuncDecl *Get##Name = nullptr;

#include "polarphp/ast/KnownDeclsDef.h"

   /// func ==(Int, Int) -> Bool
   FuncDecl *EqualIntDecl = nullptr;

   /// func _hashValue<H: Hashable>(for: H) -> Int
   FuncDecl *HashValueForDecl = nullptr;

   /// func append(Element) -> void
   FuncDecl *ArrayAppendElementDecl = nullptr;

   /// func reserveCapacityForAppend(newElementsCount: Int)
   FuncDecl *ArrayReserveCapacityDecl = nullptr;

   /// func _stdlib_isOSVersionAtLeast(Builtin.Word,Builtin.Word, Builtin.word)
   //    -> Builtin.Int1
   FuncDecl *IsOSVersionAtLeastDecl = nullptr;

   /// The set of known protocols, lazily populated as needed.
   InterfaceDecl *KnownInterfaces[NumKnownInterfaces] = {};

   /// The various module loaders that import external modules into this
   /// AstContext.
   SmallVector<std::unique_ptr<ModuleLoader>, 4> ModuleLoaders;

   /// Singleton used to cache the import graph.
   namelookup::ImportCache TheImportCache;

   /// The module loader used to load Clang modules.
   ClangModuleLoader *TheClangModuleLoader = nullptr;

   /// The module loader used to load Clang modules from DWARF.
   ClangModuleLoader *TheDWARFModuleLoader = nullptr;

   /// Map from Swift declarations to raw comments.
   llvm::DenseMap<const Decl *, RawComment> RawComments;

   /// Map from Swift declarations to brief comments.
   llvm::DenseMap<const Decl *, StringRef> BriefComments;

   /// Map from declarations to foreign error conventions.
   /// This applies to both actual imported functions and to @objc functions.
   llvm::DenseMap<const AbstractFunctionDecl *,
      ForeignErrorConvention> ForeignErrorConventions;

   /// Cache of previously looked-up precedence queries.
   AssociativityCacheType AssociativityCache;

   /// Map from normal protocol conformances to diagnostics that have
   /// been delayed until the conformance is fully checked.
   llvm::DenseMap<NormalInterfaceConformance *,
      std::vector<AstContext::DelayedConformanceDiag>>
      DelayedConformanceDiags;

   /// Map from normal protocol conformances to missing witnesses that have
   /// been delayed until the conformance is fully checked, so that we can
   /// issue a fixit that fills the entire protocol stub.
   llvm::DenseMap<NormalInterfaceConformance *, std::vector<ValueDecl *>>
      DelayedMissingWitnesses;

   /// Stores information about lazy deserialization of various declarations.
   llvm::DenseMap<const DeclContext *, LazyContextData *> LazyContexts;

   /// The single-parameter generic signature with no constraints, <T>.
   CanGenericSignature SingleGenericParameterSignature;

   /// The existential signature <T : P> for each P.
   llvm::DenseMap<CanType, CanGenericSignature> ExistentialSignatures;

   /// Overridden declarations.
   llvm::DenseMap<const ValueDecl *, ArrayRef<ValueDecl *>> Overrides;

   /// Default witnesses.
   llvm::DenseMap<std::pair<const InterfaceDecl *, ValueDecl *>, Witness>
      DefaultWitnesses;

   /// Default type witnesses for protocols.
   llvm::DenseMap<std::pair<const InterfaceDecl *, AssociatedTypeDecl *>, Type>
      DefaultTypeWitnesses;

   /// Default associated conformance witnesses for protocols.
   llvm::DenseMap<std::tuple<const InterfaceDecl *, CanType, InterfaceDecl *>,
      InterfaceConformanceRef>
      DefaultAssociatedConformanceWitnesses;

   /// Caches of default types for DefaultTypeRequest.
   /// Used to be instance variables in the TypeChecker.
   /// There is a logically separate cache for each SourceFile and
   /// KnownInterfaceKind.
   llvm::DenseMap<SourceFile *, std::array<Type, NumKnownInterfaces>>
      DefaultTypeRequestCaches;

   /// Mapping from property declarations to the backing variable types.
   llvm::DenseMap<const VarDecl *, Type> PropertyWrapperBackingVarTypes;

   /// A mapping from the backing storage of a property that has a wrapper
   /// to the original property with the wrapper.
   llvm::DenseMap<const VarDecl *, VarDecl *> OriginalWrappedProperties;

   /// The builtin initializer witness for a literal. Used when building
   /// LiteralExprs in fully-checked AST.
   llvm::DenseMap<const NominalTypeDecl *, ConcreteDeclRef> BuiltinInitWitness;

   /// Structure that captures data that is segregated into different
   /// arenas.
   struct Arena {
      static_assert(alignof(TypeBase) >= 8, "TypeBase not 8-byte aligned?");
      static_assert(alignof(TypeBase) > static_cast<unsigned>(
                                           MetatypeRepresentation::Last_MetatypeRepresentation) + 1,
                    "Use std::pair for MetatypeTypes and ExistentialMetatypeTypes.");

      llvm::DenseMap<Type, ErrorType *> ErrorTypesWithOriginal;
      llvm::FoldingSet<TypeAliasType> TypeAliasTypes;
      llvm::FoldingSet<TupleType> TupleTypes;
      llvm::DenseMap<llvm::PointerIntPair<TypeBase *, 3, unsigned>,
         MetatypeType *> MetatypeTypes;
      llvm::DenseMap<llvm::PointerIntPair<TypeBase *, 3, unsigned>,
         ExistentialMetatypeType *> ExistentialMetatypeTypes;
      llvm::DenseMap<Type, ArraySliceType *> ArraySliceTypes;
      llvm::DenseMap<std::pair<Type, Type>, DictionaryType *> DictionaryTypes;
      llvm::DenseMap<Type, OptionalType *> OptionalTypes;
      llvm::DenseMap<Type, ParenType *> SimpleParenTypes; // Most are simple
      llvm::DenseMap<std::pair<Type, unsigned>, ParenType *> ParenTypes;
      llvm::DenseMap<uintptr_t, ReferenceStorageType *> ReferenceStorageTypes;
      llvm::DenseMap<Type, LValueType *> LValueTypes;
      llvm::DenseMap<Type, InOutType *> InOutTypes;
      llvm::DenseMap<std::pair<Type, void *>, DependentMemberType *>
         DependentMemberTypes;
      llvm::DenseMap<Type, DynamicSelfType *> DynamicSelfTypes;
      llvm::DenseMap<std::pair<EnumDecl *, Type>, EnumType *> EnumTypes;
      llvm::DenseMap<std::pair<StructDecl *, Type>, StructType *> StructTypes;
      llvm::DenseMap<std::pair<ClassDecl *, Type>, ClassType *> ClassTypes;
      llvm::DenseMap<std::pair<InterfaceDecl *, Type>, InterfaceType *> InterfaceTypes;
      llvm::FoldingSet<UnboundGenericType> UnboundGenericTypes;
      llvm::FoldingSet<BoundGenericType> BoundGenericTypes;
      llvm::FoldingSet<InterfaceCompositionType> InterfaceCompositionTypes;
      llvm::FoldingSet<LayoutConstraintInfo> LayoutConstraints;
      llvm::FoldingSet<OpaqueTypeArchetypeType> OpaqueArchetypes;

      llvm::FoldingSet<GenericSignatureImpl> GenericSignatures;

      /// Stored generic signature builders for canonical generic signatures.
      llvm::DenseMap<GenericSignature, std::unique_ptr<GenericSignatureBuilder>>
         GenericSignatureBuilders;

      /// The set of function types.
      llvm::FoldingSet<FunctionType> FunctionTypes;

      /// The set of normal protocol conformances.
      llvm::FoldingSet<NormalInterfaceConformance> NormalConformances;

      // The set of self protocol conformances.
      llvm::DenseMap<InterfaceDecl *, SelfInterfaceConformance *> SelfConformances;

      /// The set of specialized protocol conformances.
      llvm::FoldingSet<SpecializedInterfaceConformance> SpecializedConformances;

      /// The set of inherited protocol conformances.
      llvm::FoldingSet<InheritedInterfaceConformance> InheritedConformances;

      /// The set of substitution maps (uniqued by their storage).
      llvm::FoldingSet<SubstitutionMap::Storage> SubstitutionMaps;

      ~Arena() {
         for (auto &conformance : SpecializedConformances)
            conformance.~SpecializedInterfaceConformance();
         // Work around MSVC warning: local variable is initialized but
         // not referenced.
#ifdef POLAR_CC_MSVC
#pragma warning (disable: 4189)
#endif
         for (auto &conformance : InheritedConformances)
            conformance.~InheritedInterfaceConformance();
#ifdef POLAR_CC_MSVC
#pragma warning (default: 4189)
#endif

         // Call the normal conformance destructors last since they could be
         // referenced by the other conformance types.
         for (auto &conformance : NormalConformances)
            conformance.~NormalInterfaceConformance();
      }

      size_t getTotalMemory() const;
   };

   llvm::DenseMap<ModuleDecl *, ModuleType *> ModuleTypes;
   llvm::DenseMap<std::pair<unsigned, unsigned>, GenericTypeParamType *>
      GenericParamTypes;
   llvm::FoldingSet<GenericFunctionType> GenericFunctionTypes;
   llvm::FoldingSet<PILFunctionType> PILFunctionTypes;
   llvm::DenseMap<CanType, PILBlockStorageType *> PILBlockStorageTypes;
   llvm::FoldingSet<PILBoxType> PILBoxTypes;
   llvm::DenseMap<BuiltinIntegerWidth, BuiltinIntegerType *> IntegerTypes;
   llvm::FoldingSet<BuiltinVectorType> BuiltinVectorTypes;
   llvm::FoldingSet<DeclName::CompoundDeclName> CompoundNames;
   llvm::DenseMap<UUID, OpenedArchetypeType *> OpenedExistentialArchetypes;

   /// For uniquifying `IndexSubset` allocations.
   llvm::FoldingSet<IndexSubset> IndexSubsets;

   /// A cache of information about whether particular nominal types
   /// are representable in a foreign language.
   llvm::DenseMap<NominalTypeDecl *, ForeignRepresentationInfo>
      ForeignRepresentableCache;

   llvm::StringMap<OptionSet<SearchPathKind>> SearchPathsSet;

   /// The permanent arena.
   Arena Permanent;

   /// Temporary arena used for a constraint solver.
   struct ConstraintSolverArena : public Arena {
      /// The allocator used for all allocations within this arena.
      llvm::BumpPtrAllocator &Allocator;

      ConstraintSolverArena(llvm::BumpPtrAllocator &allocator)
         : Allocator(allocator) {}

      ConstraintSolverArena(const ConstraintSolverArena &) = delete;

      ConstraintSolverArena(ConstraintSolverArena &&) = delete;

      ConstraintSolverArena &operator=(const ConstraintSolverArena &) = delete;

      ConstraintSolverArena &operator=(ConstraintSolverArena &&) = delete;
   };

   /// The current constraint solver arena, if any.
   std::unique_ptr<ConstraintSolverArena> CurrentConstraintSolverArena;

   Arena &getArena(AllocationArena arena) {
      switch (arena) {
         case AllocationArena::Permanent:
            return Permanent;

         case AllocationArena::ConstraintSolver:
            assert(CurrentConstraintSolverArena && "No constraint solver active?");
            return *CurrentConstraintSolverArena;
      }
      llvm_unreachable("bad AllocationArena");
   }

   llvm::FoldingSet<PILLayout> PILLayouts;

   RefCountPtr <SyntaxArena> TheSyntaxArena;

   llvm::DenseMap<OverrideSignatureKey, GenericSignature> overrideSigCache;

   Optional<ClangTypeConverter> Converter;
};

AstContext::Implementation::Implementation()
   : IdentifierTable(Allocator),
     TheSyntaxArena(new SyntaxArena()) {}

AstContext::Implementation::~Implementation() {
   for (auto &cleanup : Cleanups)
      cleanup();
}

ConstraintCheckerArenaRAII::
ConstraintCheckerArenaRAII(AstContext &self, llvm::BumpPtrAllocator &allocator)
   : Self(self), Data(self.getImpl().CurrentConstraintSolverArena.release()) {
   Self.getImpl().CurrentConstraintSolverArena.reset(
      new AstContext::Implementation::ConstraintSolverArena(allocator));
}

ConstraintCheckerArenaRAII::~ConstraintCheckerArenaRAII() {
   Self.getImpl().CurrentConstraintSolverArena.reset(
      (AstContext::Implementation::ConstraintSolverArena *) Data);
}

static ModuleDecl *createBuiltinModule(AstContext &ctx) {
   auto M = ModuleDecl::create(ctx.getIdentifier(BUILTIN_NAME), ctx);
   M->addFile(*new(ctx) BuiltinUnit(*M));
   M->setHasResolvedImports();
   return M;
}

inline AstContext::Implementation &AstContext::getImpl() const {
   auto pointer = reinterpret_cast<char *>(const_cast<AstContext *>(this));
   auto offset = llvm::alignAddr((void *) sizeof(*this), llvm::Align(alignof(Implementation)));
   return *reinterpret_cast<Implementation *>(pointer + offset);
}

void AstContext::operator delete(void *Data) throw() {
   aligned_free(Data);
}

AstContext *AstContext::get(LangOptions &langOpts,
                            TypeCheckerOptions &typeckOpts,
                            SearchPathOptions &SearchPathOpts,
                            SourceManager &SourceMgr,
                            DiagnosticEngine &Diags) {
   // If more than two data structures are concatentated, then the aggregate
   // size math needs to become more complicated due to per-struct alignment
   // constraints.
   auto align = std::max(alignof(AstContext), alignof(Implementation));
   auto size = llvm::alignTo(sizeof(AstContext) + sizeof(Implementation), align);
   auto mem = aligned_alloc(size, align);
   auto impl = reinterpret_cast<void *>((char *) mem + sizeof(AstContext));
   impl = reinterpret_cast<void *>(llvm::alignAddr(impl, llvm::Align(alignof(Implementation))));
   new(impl) Implementation();
   return new(mem)
      AstContext(langOpts, typeckOpts, SearchPathOpts, SourceMgr, Diags);
}

AstContext::AstContext(LangOptions &langOpts, TypeCheckerOptions &typeckOpts,
                       SearchPathOptions &SearchPathOpts,
                       SourceManager &SourceMgr, DiagnosticEngine &Diags)
   : LangOpts(langOpts),
     TypeCheckerOpts(typeckOpts),
     SearchPathOpts(SearchPathOpts), SourceMgr(SourceMgr), Diags(Diags),
     evaluator(Diags, langOpts.DebugDumpCycles),
     TheBuiltinModule(createBuiltinModule(*this)),
     StdlibModuleName(getIdentifier(STDLIB_NAME)),
     SwiftShimsModuleName(getIdentifier(POLAR_SHIMS_NAME)),
     TypeCheckerDebug(new StderrTypeCheckerDebugConsumer()),
     TheErrorType(
        new(*this, AllocationArena::Permanent)
           ErrorType(*this, Type(), RecursiveTypeProperties::HasError)),
     TheUnresolvedType(new(*this, AllocationArena::Permanent)
                          UnresolvedType(*this)),
     TheEmptyTupleType(TupleType::get(ArrayRef<TupleTypeElt>(), *this)),
     TheAnyType(InterfaceCompositionType::get(*this, ArrayRef<Type>(),
        /*HasExplicitAnyObject=*/false)),
     TheNativeObjectType(new(*this, AllocationArena::Permanent)
                            BuiltinNativeObjectType(*this)),
     TheBridgeObjectType(new(*this, AllocationArena::Permanent)
                            BuiltinBridgeObjectType(*this)),
     TheRawPointerType(new(*this, AllocationArena::Permanent)
                          BuiltinRawPointerType(*this)),
     TheUnsafeValueBufferType(new(*this, AllocationArena::Permanent)
                                 BuiltinUnsafeValueBufferType(*this)),
     ThePILTokenType(new(*this, AllocationArena::Permanent)
                        PILTokenType(*this)),
     TheIntegerLiteralType(new(*this, AllocationArena::Permanent)
                              BuiltinIntegerLiteralType(*this)),
     TheIEEE32Type(new(*this, AllocationArena::Permanent)
                      BuiltinFloatType(BuiltinFloatType::IEEE32, *this)),
     TheIEEE64Type(new(*this, AllocationArena::Permanent)
                      BuiltinFloatType(BuiltinFloatType::IEEE64, *this)),
     TheIEEE16Type(new(*this, AllocationArena::Permanent)
                      BuiltinFloatType(BuiltinFloatType::IEEE16, *this)),
     TheIEEE80Type(new(*this, AllocationArena::Permanent)
                      BuiltinFloatType(BuiltinFloatType::IEEE80, *this)),
     TheIEEE128Type(new(*this, AllocationArena::Permanent)
                       BuiltinFloatType(BuiltinFloatType::IEEE128, *this)),
     ThePPC128Type(new(*this, AllocationArena::Permanent)
                      BuiltinFloatType(BuiltinFloatType::PPC128, *this)) {

   // Initialize all of the known identifiers.
#define IDENTIFIER_WITH_NAME(Name, IdStr) Id_##Name = getIdentifier(IdStr);

#include "polarphp/ast/KnownIdentifiersDef.h"

   // Record the initial set of search paths.
   for (StringRef path : SearchPathOpts.ImportSearchPaths)
      getImpl().SearchPathsSet[path] |= SearchPathKind::Import;
   for (const auto &framepath : SearchPathOpts.FrameworkSearchPaths)
      getImpl().SearchPathsSet[framepath.Path] |= SearchPathKind::Framework;

   // Register any request-evaluator functions available at the AST layer.
   registerAccessRequestFunctions(evaluator);
   registerNameLookupRequestFunctions(evaluator);
}

AstContext::~AstContext() {
   // Emit evaluator dependency graph if requested.
   auto graphPath = LangOpts.RequestEvaluatorGraphVizPath;
   if (!graphPath.empty()) {
      evaluator.emitRequestEvaluatorGraphViz(graphPath);
   }
   getImpl().~Implementation();
}

llvm::BumpPtrAllocator &AstContext::getAllocator(AllocationArena arena) const {
   switch (arena) {
      case AllocationArena::Permanent:
         return getImpl().Allocator;

      case AllocationArena::ConstraintSolver:
         assert(getImpl().CurrentConstraintSolverArena != nullptr);
         return getImpl().CurrentConstraintSolverArena->Allocator;
   }
   llvm_unreachable("bad AllocationArena");
}

/// Set a new stats reporter.
void AstContext::setStatsReporter(UnifiedStatsReporter *stats) {
   Stats = stats;
   evaluator.setStatsReporter(stats);

   if (stats) {
      stats->getFrontendCounters().NumAstBytesAllocated =
         getAllocator().getBytesAllocated();
   }
}

RefCountPtr <SyntaxArena> AstContext::getSyntaxArena() const {
   return getImpl().TheSyntaxArena;
}

bool AstContext::areSemanticQueriesEnabled() const {
   return getLegacyGlobalTypeChecker() != nullptr;
}

TypeChecker *AstContext::getLegacyGlobalTypeChecker() const {
   return getImpl().Checker;
}

void AstContext::installGlobalTypeChecker(TypeChecker *TC) {
   assert(!getImpl().Checker);
   getImpl().Checker = TC;
}

/// getIdentifier - Return the uniqued and AST-Context-owned version of the
/// specified string.
Identifier AstContext::getIdentifier(StringRef Str) const {
   // Make sure null pointers stay null.
   if (Str.data() == nullptr)
      return Identifier(nullptr);

   auto pair = std::make_pair(Str, Identifier::Aligner());
   auto I = getImpl().IdentifierTable.insert(pair).first;
   return Identifier(I->getKeyData());
}

void AstContext::lookupInPolarphpModule(
   StringRef name,
   SmallVectorImpl<ValueDecl *> &results) const {
   ModuleDecl *M = getStdlibModule();
   if (!M)
      return;

   // Find all of the declarations with this name in the Swift module.
   auto identifier = getIdentifier(name);
   M->lookupValue(identifier, NLKind::UnqualifiedLookup, results);
}

FuncDecl *AstContext::getPlusFunctionOnRangeReplaceableCollection() const {
   if (getImpl().PlusFunctionOnRangeReplaceableCollection) {
      return getImpl().PlusFunctionOnRangeReplaceableCollection;
   }
   // Find all of the declarations with this name in the Swift module.
   SmallVector<ValueDecl *, 1> Results;
   lookupInPolarphpModule("+", Results);
   for (auto Result : Results) {
      if (auto *FD = dyn_cast<FuncDecl>(Result)) {
         if (!FD->getOperatorDecl())
            continue;
         for (auto Req: FD->getGenericRequirements()) {
            if (Req.getKind() == RequirementKind::Conformance &&
                Req.getSecondType()->getNominalOrBoundGenericNominal() ==
                getRangeReplaceableCollectionDecl()) {
               getImpl().PlusFunctionOnRangeReplaceableCollection = FD;
            }
         }
      }
   }
   return getImpl().PlusFunctionOnRangeReplaceableCollection;
}

FuncDecl *AstContext::getPlusFunctionOnString() const {
   if (getImpl().PlusFunctionOnString) {
      return getImpl().PlusFunctionOnString;
   }
   // Find all of the declarations with this name in the Swift module.
   SmallVector<ValueDecl *, 1> Results;
   lookupInPolarphpModule("+", Results);
   for (auto Result : Results) {
      if (auto *FD = dyn_cast<FuncDecl>(Result)) {
         if (!FD->getOperatorDecl())
            continue;
         auto ResultType = FD->getResultInterfaceType();
         if (ResultType->getNominalOrBoundGenericNominal() != getStringDecl())
            continue;
         auto ParamList = FD->getParameters();
         if (ParamList->size() != 2)
            continue;
         auto CheckIfStringParam = [this](ParamDecl *Param) {
            auto Type = Param->getInterfaceType()->getNominalOrBoundGenericNominal();
            return Type == getStringDecl();
         };
         if (CheckIfStringParam(ParamList->get(0)) &&
             CheckIfStringParam(ParamList->get(1))) {
            getImpl().PlusFunctionOnString = FD;
            break;
         }
      }
   }
   return getImpl().PlusFunctionOnString;
}

#define KNOWN_STDLIB_TYPE_DECL(NAME, DECL_CLASS, NUM_GENERIC_PARAMS) \
  DECL_CLASS *AstContext::get##NAME##Decl() const { \
    if (getImpl().NAME##Decl) \
      return getImpl().NAME##Decl; \
    SmallVector<ValueDecl *, 1> results; \
    lookupInPolarphpModule(#NAME, results); \
    for (auto result : results) { \
      if (auto type = dyn_cast<DECL_CLASS>(result)) { \
        auto params = type->getGenericParams(); \
        if (NUM_GENERIC_PARAMS == (params == nullptr ? 0 : params->size())) { \
          getImpl().NAME##Decl = type; \
          return type; \
        } \
      } \
    } \
    return nullptr; \
  }

#include "polarphp/ast/KnownStdlibTypesDef.h"

CanType AstContext::getExceptionType() const {
   if (auto exn = getErrorDecl()) {
      return exn->getDeclaredType()->getCanonicalType();
   } else {
      // Use Builtin.NativeObject just as a stand-in.
      return TheNativeObjectType;
   }
}

InterfaceDecl *AstContext::getErrorDecl() const {
   return getInterface(KnownInterfaceKind::Error);
}

EnumElementDecl *AstContext::getOptionalSomeDecl() const {
   if (!getImpl().OptionalSomeDecl)
      getImpl().OptionalSomeDecl = getOptionalDecl()->getUniqueElement(/*hasVal*/true);
   return getImpl().OptionalSomeDecl;
}

EnumElementDecl *AstContext::getOptionalNoneDecl() const {
   if (!getImpl().OptionalNoneDecl)
      getImpl().OptionalNoneDecl = getOptionalDecl()->getUniqueElement(/*hasVal*/false);
   return getImpl().OptionalNoneDecl;
}

static VarDecl *getPointeeProperty(VarDecl *&cache,
                                   NominalTypeDecl *(AstContext::*getNominal)() const,
                                   const AstContext &ctx) {
   if (cache) return cache;

   // There must be a generic type with one argument.
   NominalTypeDecl *nominal = (ctx.*getNominal)();
   if (!nominal) return nullptr;
   auto sig = nominal->getGenericSignature();
   if (!sig) return nullptr;
   if (sig->getGenericParams().size() != 1) return nullptr;

   // There must be a property named "pointee".
   auto identifier = ctx.getIdentifier("pointee");
   auto results = nominal->lookupDirect(identifier);
   if (results.size() != 1) return nullptr;

   // The property must have type T.
   auto *property = dyn_cast<VarDecl>(results[0]);
   if (!property) return nullptr;
   if (!property->getInterfaceType()->isEqual(sig->getGenericParams()[0]))
      return nullptr;

   cache = property;
   return property;
}

VarDecl *
AstContext::getPointerPointeePropertyDecl(PointerTypeKind ptrKind) const {
   switch (ptrKind) {
      case PTK_UnsafeMutableRawPointer:
         return getPointeeProperty(getImpl().UnsafeMutableRawPointerMemoryDecl,
                                   &AstContext::getUnsafeMutableRawPointerDecl,
                                   *this);
      case PTK_UnsafeRawPointer:
         return getPointeeProperty(getImpl().UnsafeRawPointerMemoryDecl,
                                   &AstContext::getUnsafeRawPointerDecl,
                                   *this);
      case PTK_UnsafeMutablePointer:
         return getPointeeProperty(getImpl().UnsafeMutablePointerMemoryDecl,
                                   &AstContext::getUnsafeMutablePointerDecl,
                                   *this);
      case PTK_UnsafePointer:
         return getPointeeProperty(getImpl().UnsafePointerMemoryDecl,
                                   &AstContext::getUnsafePointerDecl,
                                   *this);
      case PTK_AutoreleasingUnsafeMutablePointer:
         return getPointeeProperty(getImpl().AutoreleasingUnsafeMutablePointerMemoryDecl,
                                   &AstContext::getAutoreleasingUnsafeMutablePointerDecl,
                                   *this);
   }
   llvm_unreachable("bad pointer kind");
}

CanType AstContext::getAnyObjectType() const {
   if (getImpl().AnyObjectType) {
      return getImpl().AnyObjectType;
   }

   getImpl().AnyObjectType = CanType(
      InterfaceCompositionType::get(
         *this, {}, /*HasExplicitAnyObject=*/true));
   return getImpl().AnyObjectType;
}

CanType AstContext::getNeverType() const {
   auto neverDecl = getNeverDecl();
   if (!neverDecl)
      return CanType();
   return neverDecl->getDeclaredType()->getCanonicalType();
}

InterfaceDecl *AstContext::getInterface(KnownInterfaceKind kind) const {
   // Check whether we've already looked for and cached this protocol.
   unsigned index = (unsigned) kind;
   assert(index < NumKnownInterfaces && "Number of known protocols is wrong");
   if (getImpl().KnownInterfaces[index])
      return getImpl().KnownInterfaces[index];

   // Find all of the declarations with this name in the appropriate module.
   SmallVector<ValueDecl *, 1> results;

   const ModuleDecl *M;
   switch (kind) {
      case KnownInterfaceKind::BridgedNSError:
      case KnownInterfaceKind::BridgedStoredNSError:
      case KnownInterfaceKind::ErrorCodeInterface:
         M = getLoadedModule(Id_Foundation);
         break;
      case KnownInterfaceKind::CFObject:
         M = getLoadedModule(Id_CoreFoundation);
         break;
      case KnownInterfaceKind::Differentiable:
         M = getLoadedModule(Id_Differentiation);
         break;
      default:
         M = getStdlibModule();
         break;
   }

   if (!M)
      return nullptr;
   M->lookupValue(getIdentifier(get_interface_name(kind)),
                  NLKind::UnqualifiedLookup, results);

   for (auto result : results) {
      if (auto protocol = dyn_cast<InterfaceDecl>(result)) {
         getImpl().KnownInterfaces[index] = protocol;
         return protocol;
      }
   }

   return nullptr;
}

/// Find the implementation for the given "intrinsic" library function.
static FuncDecl *findLibraryIntrinsic(const AstContext &ctx,
                                      StringRef name) {
   SmallVector<ValueDecl *, 1> results;
   ctx.lookupInPolarphpModule(name, results);
   if (results.size() == 1)
      return dyn_cast_or_null<FuncDecl>(results.front());
   return nullptr;
}

/// Returns the type of an intrinsic function if it is not generic, otherwise
/// returns nullptr.
static FunctionType *
getIntrinsicCandidateType(FuncDecl *fn, bool allowTypeMembers) {
   auto type = fn->getInterfaceType();
   if (allowTypeMembers && fn->getDeclContext()->isTypeContext()) {
      auto fnType = type->getAs<FunctionType>();
      if (!fnType) return nullptr;

      type = fnType->getResult();
   }
   return type->getAs<FunctionType>();
}

/// Check whether the given type is Builtin.Int1.
static bool isBuiltinInt1Type(Type type) {
   if (auto intType = type->getAs<BuiltinIntegerType>())
      return intType->isFixedWidth() && intType->getFixedWidth() == 1;
   return false;
}

/// Check whether the given type is Builtin.Word.
static bool isBuiltinWordType(Type type) {
   if (auto intType = type->getAs<BuiltinIntegerType>())
      return intType->getWidth().isPointerWidth();
   return false;
}

/// Looks up all implementations of an operator (globally and declared in types)
/// and passes potential matches to the given callback. The search stops when
/// the predicate returns true (in which case the matching function declaration
/// is returned); otherwise, nullptr is returned if there are no matches.
/// \p C The AST context.
/// \p oper The name of the operator.
/// \p contextType If the operator is declared on a type, then only operators
///     defined on this type should be considered.
/// \p pred A callback predicate that takes as its argument the type of a
///     candidate function declaration and returns true if the function matches
///     the desired criteria.
/// \return The matching function declaration, or nullptr if there was no match.
static FuncDecl *
lookupOperatorFunc(const AstContext &ctx, StringRef oper, Type contextType,
                   llvm::function_ref<bool(FunctionType *)> pred) {
   SmallVector<ValueDecl *, 32> candidates;
   ctx.lookupInPolarphpModule(oper, candidates);

   for (auto candidate : candidates) {
      // All operator declarations should be functions, but make sure.
      auto *fnDecl = dyn_cast<FuncDecl>(candidate);
      if (!fnDecl)
         continue;

      if (fnDecl->getDeclContext()->isTypeContext()) {
         auto contextTy = fnDecl->getDeclContext()->getDeclaredInterfaceType();
         if (!contextTy->isEqual(contextType)) continue;
      }

      auto *funcTy = getIntrinsicCandidateType(fnDecl, /*allowTypeMembers=*/true);
      if (!funcTy)
         continue;

      if (pred(funcTy))
         return fnDecl;
   }

   return nullptr;
}

ConcreteDeclRef AstContext::getBoolBuiltinInitDecl() const {
   auto fn = [&](AstContext &ctx) {
      return DeclName(ctx, DeclBaseName::createConstructor(),
                      {Id_builtinBooleanLiteral});
   };
   auto builtinInterfaceKind =
      KnownInterfaceKind::ExpressibleByBuiltinBooleanLiteral;
   return getBuiltinInitDecl(getBoolDecl(), builtinInterfaceKind, fn);
}

ConcreteDeclRef
AstContext::getIntBuiltinInitDecl(NominalTypeDecl *intDecl) const {
   auto fn = [&](AstContext &ctx) {
      return DeclName(ctx, DeclBaseName::createConstructor(),
                      {Id_builtinIntegerLiteral});
   };
   auto builtinInterfaceKind =
      KnownInterfaceKind::ExpressibleByBuiltinIntegerLiteral;
   return getBuiltinInitDecl(intDecl, builtinInterfaceKind, fn);
}

ConcreteDeclRef
AstContext::getFloatBuiltinInitDecl(NominalTypeDecl *floatDecl) const {
   auto fn = [&](AstContext &ctx) {
      return DeclName(ctx, DeclBaseName::createConstructor(),
                      {Id_builtinFloatLiteral});
   };

   auto builtinInterfaceKind =
      KnownInterfaceKind::ExpressibleByBuiltinFloatLiteral;
   return getBuiltinInitDecl(floatDecl, builtinInterfaceKind, fn);
}

ConcreteDeclRef
AstContext::getStringBuiltinInitDecl(NominalTypeDecl *stringDecl) const {
   auto fn = [&](AstContext &ctx) {
      return DeclName(ctx, DeclBaseName::createConstructor(),
                      {Id_builtinStringLiteral,
                       getIdentifier("utf8CodeUnitCount"),
                       getIdentifier("isASCII")});
   };

   auto builtinInterfaceKind =
      KnownInterfaceKind::ExpressibleByBuiltinStringLiteral;
   return getBuiltinInitDecl(stringDecl, builtinInterfaceKind, fn);
}

ConcreteDeclRef
AstContext::getBuiltinInitDecl(NominalTypeDecl *decl,
                               KnownInterfaceKind builtinInterfaceKind,
                               llvm::function_ref<DeclName(AstContext &ctx)> initName) const {
   auto &witness = getImpl().BuiltinInitWitness[decl];
   if (witness)
      return witness;

   auto type = decl->getDeclaredType();
   auto builtinInterface = getInterface(builtinInterfaceKind);
   auto builtinConformance = getStdlibModule()->lookupConformance(
      type, builtinInterface);
   if (builtinConformance.isInvalid()) {
      assert(false && "Missing required conformance");
      witness = ConcreteDeclRef();
      return witness;
   }

   auto *ctx = const_cast<AstContext *>(this);
   witness = builtinConformance.getWitnessByName(type, initName(*ctx));
   if (!witness) {
      assert(false && "Missing required witness");
      witness = ConcreteDeclRef();
      return witness;
   }

   return witness;
}

FuncDecl *AstContext::getEqualIntDecl() const {
   if (getImpl().EqualIntDecl)
      return getImpl().EqualIntDecl;

   if (!getIntDecl() || !getBoolDecl())
      return nullptr;

   auto intType = getIntDecl()->getDeclaredType();
   auto isIntParam = [&](AnyFunctionType::Param param) {
      return (!param.isVariadic() && !param.isInOut() &&
              param.getPlainType()->isEqual(intType));
   };
   auto boolType = getBoolDecl()->getDeclaredType();
   auto decl = lookupOperatorFunc(*this, "==",
                                  intType, [=](FunctionType *type) {
         // Check for the signature: (Int, Int) -> Bool
         if (type->getParams().size() != 2) return false;
         if (!isIntParam(type->getParams()[0]) ||
             !isIntParam(type->getParams()[1]))
            return false;
         return type->getResult()->isEqual(boolType);
      });
   getImpl().EqualIntDecl = decl;
   return decl;
}

FuncDecl *AstContext::getHashValueForDecl() const {
   if (getImpl().HashValueForDecl)
      return getImpl().HashValueForDecl;

   SmallVector<ValueDecl *, 1> results;
   lookupInPolarphpModule("_hashValue", results);
   for (auto result : results) {
      auto *fd = dyn_cast<FuncDecl>(result);
      if (!fd)
         continue;
      auto paramList = fd->getParameters();
      if (paramList->size() != 1)
         continue;
      auto paramDecl = paramList->get(0);
      if (paramDecl->getArgumentName() != Id_for)
         continue;
      auto genericParams = fd->getGenericParams();
      if (!genericParams || genericParams->size() != 1)
         continue;
      getImpl().HashValueForDecl = fd;
      return fd;
   }
   return nullptr;
}

FuncDecl *AstContext::getArrayAppendElementDecl() const {
   if (getImpl().ArrayAppendElementDecl)
      return getImpl().ArrayAppendElementDecl;

   auto AppendFunctions = getArrayDecl()->lookupDirect(getIdentifier("append"));

   for (auto CandidateFn : AppendFunctions) {
      auto FnDecl = dyn_cast<FuncDecl>(CandidateFn);
      auto Attrs = FnDecl->getAttrs();
      for (auto *A : Attrs.getAttributes<SemanticsAttr, false>()) {
         if (A->Value != "array.append_element")
            continue;

         auto SelfDecl = FnDecl->getImplicitSelfDecl();
         if (!SelfDecl->isInOut())
            return nullptr;

         auto SelfInOutTy = SelfDecl->getInterfaceType();
         BoundGenericStructType *SelfGenericStructTy =
            SelfInOutTy->getAs<BoundGenericStructType>();
         if (!SelfGenericStructTy)
            return nullptr;
         if (SelfGenericStructTy->getDecl() != getArrayDecl())
            return nullptr;

         auto ParamList = FnDecl->getParameters();
         if (ParamList->size() != 1)
            return nullptr;

         GenericTypeParamType *ElementType = ParamList->get(0)->
            getInterfaceType()->getAs<GenericTypeParamType>();
         if (!ElementType)
            return nullptr;
         if (ElementType->getName() != getIdentifier("Element"))
            return nullptr;

         if (!FnDecl->getResultInterfaceType()->isVoid())
            return nullptr;

         getImpl().ArrayAppendElementDecl = FnDecl;
         return FnDecl;
      }
   }
   return nullptr;
}

FuncDecl *AstContext::getArrayReserveCapacityDecl() const {
   if (getImpl().ArrayReserveCapacityDecl)
      return getImpl().ArrayReserveCapacityDecl;

   auto ReserveFunctions = getArrayDecl()->lookupDirect(
      getIdentifier("reserveCapacityForAppend"));

   for (auto CandidateFn : ReserveFunctions) {
      auto FnDecl = dyn_cast<FuncDecl>(CandidateFn);
      auto Attrs = FnDecl->getAttrs();
      for (auto *A : Attrs.getAttributes<SemanticsAttr, false>()) {
         if (A->Value != "array.reserve_capacity_for_append")
            continue;

         auto SelfDecl = FnDecl->getImplicitSelfDecl();
         if (!SelfDecl->isInOut())
            return nullptr;

         auto SelfInOutTy = SelfDecl->getInterfaceType();
         BoundGenericStructType *SelfGenericStructTy =
            SelfInOutTy->getAs<BoundGenericStructType>();
         if (!SelfGenericStructTy)
            return nullptr;
         if (SelfGenericStructTy->getDecl() != getArrayDecl())
            return nullptr;

         auto ParamList = FnDecl->getParameters();
         if (ParamList->size() != 1)
            return nullptr;
         StructType *IntType =
            ParamList->get(0)->getInterfaceType()->getAs<StructType>();
         if (!IntType)
            return nullptr;

         StructDecl *IntDecl = IntType->getDecl();
         auto StoredProperties = IntDecl->getStoredProperties();
         if (StoredProperties.size() != 1)
            return nullptr;
         VarDecl *field = StoredProperties[0];
         if (field->hasClangNode())
            return nullptr;
         if (!field->getInterfaceType()->is<BuiltinIntegerType>())
            return nullptr;

         if (!FnDecl->getResultInterfaceType()->isVoid())
            return nullptr;

         getImpl().ArrayReserveCapacityDecl = FnDecl;
         return FnDecl;
      }
   }
   return nullptr;
}

FuncDecl *AstContext::getIsOSVersionAtLeastDecl() const {
   if (getImpl().IsOSVersionAtLeastDecl)
      return getImpl().IsOSVersionAtLeastDecl;

   // Look for the function.
   auto decl =
      findLibraryIntrinsic(*this, "_stdlib_isOSVersionAtLeast");
   if (!decl)
      return nullptr;

   auto *fnType = getIntrinsicCandidateType(decl, /*allowTypeMembers=*/false);
   if (!fnType)
      return nullptr;

   // Input must be (Builtin.Word, Builtin.Word, Builtin.Word)
   auto intrinsicsParams = fnType->getParams();
   if (intrinsicsParams.size() != 3)
      return nullptr;

   if (llvm::any_of(intrinsicsParams, [](AnyFunctionType::Param param) {
      return (param.isVariadic() || param.isInOut() ||
              !isBuiltinWordType(param.getPlainType()));
   })) {
      return nullptr;
   }

   // Output must be Builtin.Int1
   if (!isBuiltinInt1Type(fnType->getResult()))
      return nullptr;

   getImpl().IsOSVersionAtLeastDecl = decl;
   return decl;
}

static bool isHigherPrecedenceThan(PrecedenceGroupDecl *a,
                                   PrecedenceGroupDecl *b) {
   assert(a != b && "exact match should already have been filtered");

   SmallVector<PrecedenceGroupDecl *, 4> stack;

   // Compute the transitive set of precedence groups that are
   // explicitly lower than 'b', including 'b' itself.  This is expected
   // to be very small, since it's only legal in downstream modules.
   SmallPtrSet<PrecedenceGroupDecl *, 4> targets;
   targets.insert(b);
   stack.push_back(b);
   do {
      auto cur = stack.pop_back_val();
      for (auto &rel : cur->getLowerThan()) {
         auto group = rel.Group;

         // If we ever see 'a', we're done.
         if (group == a) return true;

         // Protect against invalid ASTs where the group isn't actually set.
         if (!group) continue;

         // If we've already inserted this, don't add it to the queue.
         if (!targets.insert(group).second) continue;

         stack.push_back(group);
      }
   } while (!stack.empty());

   // Walk down the higherThan relationships from 'a' and look for
   // anything in the set we just built.
   stack.push_back(a);
   do {
      auto cur = stack.pop_back_val();
      assert(!targets.count(cur));

      for (auto &rel : cur->getHigherThan()) {
         auto group = rel.Group;

         if (!group) continue;

         // If we ever see a group that's in the targets set, we're done.
         if (targets.count(group)) return true;

         stack.push_back(group);
      }
   } while (!stack.empty());

   return false;
}

static Associativity computeAssociativity(AssociativityCacheType &cache,
                                          PrecedenceGroupDecl *left,
                                          PrecedenceGroupDecl *right) {
   auto it = cache.find({left, right});
   if (it != cache.end()) return it->second;

   auto result = Associativity::None;
   if (isHigherPrecedenceThan(left, right))
      result = Associativity::Left;
   else if (isHigherPrecedenceThan(right, left))
      result = Associativity::Right;
   cache.insert({{left, right}, result});
   return result;
}

Associativity
AstContext::associateInfixOperators(PrecedenceGroupDecl *left,
                                    PrecedenceGroupDecl *right) const {
   // If the operators are in the same precedence group, use the group's
   // associativity.
   if (left == right) {
      return left->getAssociativity();
   }

   // This relationship is antisymmetric, so we can canonicalize to avoid
   // computing it twice.  Arbitrarily, if the pointer value of 'left'
   // is greater than the pointer value of 'right', we flip them and
   // then flip the result.

   if (uintptr_t(left) < uintptr_t(right)) {
      return computeAssociativity(getImpl().AssociativityCache, left, right);
   }

   switch (computeAssociativity(getImpl().AssociativityCache, right, left)) {
      case Associativity::Left:
         return Associativity::Right;
      case Associativity::Right:
         return Associativity::Left;
      case Associativity::None:
         return Associativity::None;
   }
   llvm_unreachable("bad associativity");
}

// Find library intrinsic function.
static FuncDecl *findLibraryFunction(const AstContext &ctx, FuncDecl *&cache,
                                     StringRef name) {
   if (cache) return cache;

   // Look for a generic function.
   cache = findLibraryIntrinsic(ctx, name);
   return cache;
}

#define FUNC_DECL(Name, Id)                                         \
FuncDecl *AstContext::get##Name() const {     \
  return findLibraryFunction(*this, getImpl().Get##Name, Id);  \
}
#include "polarphp/ast/KnownDeclsDef.h"

bool AstContext::hasOptionalIntrinsics() const {
   return getOptionalDecl() &&
          getOptionalSomeDecl() &&
          getOptionalNoneDecl() &&
          getDiagnoseUnexpectedNilOptional();
}

bool AstContext::hasPointerArgumentIntrinsics() const {
   return getUnsafeMutableRawPointerDecl()
          && getUnsafeRawPointerDecl()
          && getUnsafeMutablePointerDecl()
          && getUnsafePointerDecl()
          && (!LangOpts.EnableObjCInterop || getAutoreleasingUnsafeMutablePointerDecl())
          && getUnsafeBufferPointerDecl()
          && getUnsafeMutableBufferPointerDecl()
          && getUnsafeRawBufferPointerDecl()
          && getUnsafeMutableRawBufferPointerDecl()
          && getConvertPointerToPointerArgument()
          && getConvertMutableArrayToPointerArgument()
          && getConvertConstArrayToPointerArgument()
          && getConvertConstStringToUTF8PointerArgument()
          && getConvertInOutToPointerArgument();
}

bool AstContext::hasArrayLiteralIntrinsics() const {
   return getArrayDecl()
          && getAllocateUninitializedArray()
          && getDeallocateUninitializedArray();
}

void AstContext::addCleanup(std::function<void(void)> cleanup) {
   getImpl().Cleanups.push_back(std::move(cleanup));
}

bool AstContext::hadError() const {
   return Diags.hadAnyError();
}

/// Retrieve the arena from which we should allocate storage for a type.
static AllocationArena getArena(RecursiveTypeProperties properties) {
   bool hasTypeVariable = properties.hasTypeVariable();
   return hasTypeVariable ? AllocationArena::ConstraintSolver
                          : AllocationArena::Permanent;
}

void AstContext::addSearchPath(StringRef searchPath, bool isFramework,
                               bool isSystem) {
   OptionSet<SearchPathKind> &loaded = getImpl().SearchPathsSet[searchPath];
   auto kind = isFramework ? SearchPathKind::Framework : SearchPathKind::Import;
   if (loaded.contains(kind))
      return;
   loaded |= kind;

   if (isFramework)
      SearchPathOpts.FrameworkSearchPaths.push_back({searchPath, isSystem});
   else
      SearchPathOpts.ImportSearchPaths.push_back(searchPath);

   if (auto *clangLoader = getClangModuleLoader())
      clangLoader->addSearchPath(searchPath, isFramework, isSystem);
}

void AstContext::addModuleLoader(std::unique_ptr<ModuleLoader> loader,
                                 bool IsClang, bool IsDwarf) {
   if (IsClang && !IsDwarf && !getImpl().TheClangModuleLoader)
      getImpl().TheClangModuleLoader =
         static_cast<ClangModuleLoader *>(loader.get());
   if (IsClang && IsDwarf && !getImpl().TheDWARFModuleLoader)
      getImpl().TheDWARFModuleLoader =
         static_cast<ClangModuleLoader *>(loader.get());

   getImpl().ModuleLoaders.push_back(std::move(loader));
}

void AstContext::loadExtensions(NominalTypeDecl *nominal,
                                unsigned previousGeneration) {
   PrettyStackTraceDecl stackTrace("loading extensions for", nominal);
   for (auto &loader : getImpl().ModuleLoaders) {
      loader->loadExtensions(nominal, previousGeneration);
   }
}

void AstContext::verifyAllLoadedModules() const {
#ifndef NDEBUG
   FrontendStatsTracer tracer(Stats, "verify-all-loaded-modules");
   for (auto &loader : getImpl().ModuleLoaders)
      loader->verifyAllModules();

   for (auto &topLevelModulePair : LoadedModules) {
      ModuleDecl *M = topLevelModulePair.second;
      assert(!M->getFiles().empty() || M->failedToLoad());
   }
#endif
}

namelookup::ImportCache &AstContext::getImportCache() const {
   return getImpl().TheImportCache;
}

ClangModuleLoader *AstContext::getClangModuleLoader() const {
   return getImpl().TheClangModuleLoader;
}

ClangModuleLoader *AstContext::getDWARFModuleLoader() const {
   return getImpl().TheDWARFModuleLoader;
}

ModuleDecl *AstContext::getLoadedModule(
   ArrayRef<std::pair<Identifier, SourceLoc>> ModulePath) const {
   assert(!ModulePath.empty());

   // TODO: Swift submodules.
   if (ModulePath.size() == 1) {
      return getLoadedModule(ModulePath[0].first);
   }
   return nullptr;
}

ModuleDecl *AstContext::getLoadedModule(Identifier ModuleName) const {
   return LoadedModules.lookup(ModuleName);
}

static AllocationArena getArena(GenericSignature genericSig) {
   if (!genericSig)
      return AllocationArena::Permanent;

   if (genericSig->hasTypeVariable())
      return AllocationArena::ConstraintSolver;

   return AllocationArena::Permanent;
}

void AstContext::registerGenericSignatureBuilder(
   GenericSignature sig,
   GenericSignatureBuilder &&builder) {
   auto canSig = sig->getCanonicalSignature();
   auto arena = getArena(sig);
   auto &genericSignatureBuilders =
      getImpl().getArena(arena).GenericSignatureBuilders;
   auto known = genericSignatureBuilders.find(canSig);
   if (known != genericSignatureBuilders.end()) {
      ++NumRegisteredGenericSignatureBuildersAlready;
      return;
   }

   ++NumRegisteredGenericSignatureBuilders;
   genericSignatureBuilders[canSig] =
      std::make_unique<GenericSignatureBuilder>(std::move(builder));
}

GenericSignatureBuilder *AstContext::getOrCreateGenericSignatureBuilder(
   CanGenericSignature sig) {
   // Check whether we already have a generic signature builder for this
   // signature and module.
   auto arena = getArena(sig);
   auto &genericSignatureBuilders =
      getImpl().getArena(arena).GenericSignatureBuilders;
   auto known = genericSignatureBuilders.find(sig);
   if (known != genericSignatureBuilders.end())
      return known->second.get();

   // Create a new generic signature builder with the given signature.
   auto builder = new GenericSignatureBuilder(*this);

   // Store this generic signature builder (no generic environment yet).
   genericSignatureBuilders[sig] =
      std::unique_ptr<GenericSignatureBuilder>(builder);

   builder->addGenericSignature(sig);

#if POLAR_GSB_EXPENSIVE_ASSERTIONS
                                                                                                                           auto builderSig =
    builder->computeGenericSignature(SourceLoc(),
                                     /*allowConcreteGenericParams=*/true);
  if (builderSig->getCanonicalSignature() != sig) {
    llvm::errs() << "ERROR: generic signature builder is not idempotent.\n";
    llvm::errs() << "Original generic signature   : ";
    sig->print(llvm::errs());
    llvm::errs() << "\nReprocessed generic signature: ";
    auto reprocessedSig = builderSig->getCanonicalSignature();

    reprocessedSig->print(llvm::errs());
    llvm::errs() << "\n";

    if (sig->getGenericParams().size() ==
          reprocessedSig->getGenericParams().size() &&
        sig->getRequirements().size() ==
          reprocessedSig->getRequirements().size()) {
      for (unsigned i : indices(sig->getRequirements())) {
        auto sigReq = sig->getRequirements()[i];
        auto reprocessedReq = reprocessedSig->getRequirements()[i];
        if (sigReq.getKind() != reprocessedReq.getKind()) {
          llvm::errs() << "Requirement mismatch:\n";
          llvm::errs() << "  Original: ";
          sigReq.print(llvm::errs(), PrintOptions());
          llvm::errs() << "\n  Reprocessed: ";
          reprocessedReq.print(llvm::errs(), PrintOptions());
          llvm::errs() << "\n";
          break;
        }

        if (!sigReq.getFirstType()->isEqual(reprocessedReq.getFirstType())) {
          llvm::errs() << "First type mismatch, original is:\n";
          sigReq.getFirstType().dump(llvm::errs());
          llvm::errs() << "Reprocessed:\n";
          reprocessedReq.getFirstType().dump(llvm::errs());
          llvm::errs() << "\n";
          break;
        }

        if (sigReq.getKind() == RequirementKind::SameType &&
            !sigReq.getSecondType()->isEqual(reprocessedReq.getSecondType())) {
          llvm::errs() << "Second type mismatch, original is:\n";
          sigReq.getSecondType().dump(llvm::errs());
          llvm::errs() << "Reprocessed:\n";
          reprocessedReq.getSecondType().dump(llvm::errs());
          llvm::errs() << "\n";
          break;
        }
      }
    }

    llvm_unreachable("idempotency problem with a generic signature");
  }
#else
   // FIXME: This should be handled lazily in the future, and therefore not
   // required.
   builder->processDelayedRequirements();
#endif

   return builder;
}

Optional<llvm::TinyPtrVector<ValueDecl *>>
OverriddenDeclsRequest::getCachedResult() const {
   auto decl = std::get<0>(getStorage());
   if (!decl->LazySemanticInfo.hasOverriddenComputed)
      return None;

   // If there are no overridden declarations (the common case), return.
   llvm::TinyPtrVector<ValueDecl *> overridden;
   if (!decl->LazySemanticInfo.hasOverridden) return overridden;

   // Retrieve the set of overrides from the AstContext.
   AstContext &ctx = decl->getAstContext();
   auto known = ctx.getImpl().Overrides.find(decl);
   assert(known != ctx.getImpl().Overrides.end());
   overridden.insert(overridden.end(),
                     known->second.begin(), known->second.end());
   return overridden;
}

void OverriddenDeclsRequest::cacheResult(
   llvm::TinyPtrVector<ValueDecl *> value) const {
   auto decl = std::get<0>(getStorage());
   decl->LazySemanticInfo.hasOverriddenComputed = true;
   decl->LazySemanticInfo.hasOverridden = !value.empty();

   if (value.empty())
      return;

   // Sanity-check the declarations we were given.
   for (auto overriddenDecl : value) {
      assert(overriddenDecl->getKind() == decl->getKind() &&
             "Overridden decl kind mismatch");
      if (auto func = dyn_cast<AbstractFunctionDecl>(overriddenDecl))
         func->setIsOverridden();
   }

   // Record the overrides in the context.
   auto &ctx = decl->getAstContext();
   auto overriddenCopy =
      ctx.AllocateCopy(value.operator ArrayRef<ValueDecl *>());
   (void) ctx.getImpl().Overrides.insert({decl, overriddenCopy});
}

/// Returns the default witness for a requirement, or nullptr if there is
/// no default.
Witness InterfaceDecl::getDefaultWitness(ValueDecl *requirement) const {
   loadAllMembers();

   AstContext &ctx = getAstContext();
   auto found = ctx.getImpl().DefaultWitnesses.find({this, requirement});
   if (found == ctx.getImpl().DefaultWitnesses.end())
      return Witness();
   return found->second;
}

/// Record the default witness for a requirement.
void InterfaceDecl::setDefaultWitness(ValueDecl *requirement, Witness witness) {
   assert(witness);
   AstContext &ctx = getAstContext();
   auto pair = ctx.getImpl().DefaultWitnesses.insert(
      std::make_pair(std::make_pair(this, requirement), witness));
   assert(pair.second && "Already have a default witness!");
   (void) pair;
}

/// Returns the default type witness for an associated type, or a null
/// type if there is no default.
Type InterfaceDecl::getDefaultTypeWitness(AssociatedTypeDecl *assocType) const {
   auto &ctx = getAstContext();
   auto found = ctx.getImpl().DefaultTypeWitnesses.find({this, assocType});
   if (found == ctx.getImpl().DefaultTypeWitnesses.end())
      return Type();

   return found->second;
}

/// Set the default type witness for an associated type.
void InterfaceDecl::setDefaultTypeWitness(AssociatedTypeDecl *assocType,
                                          Type witness) {
   assert(witness);
   assert(!witness->hasArchetype() && "Only record interface types");
   AstContext &ctx = getAstContext();
   auto pair = ctx.getImpl().DefaultTypeWitnesses.insert(
      std::make_pair(std::make_pair(this, assocType), witness));
   assert(pair.second && "Already have a default witness");
   (void) pair;
}

InterfaceConformanceRef InterfaceDecl::getDefaultAssociatedConformanceWitness(
   CanType association, InterfaceDecl *requirement) const {
   auto &ctx = getAstContext();
   auto found =
      ctx.getImpl().DefaultAssociatedConformanceWitnesses.find(
         std::make_tuple(this, association, requirement));
   if (found == ctx.getImpl().DefaultAssociatedConformanceWitnesses.end())
      return InterfaceConformanceRef::forInvalid();

   return found->second;
}

void InterfaceDecl::setDefaultAssociatedConformanceWitness(
   CanType association,
   InterfaceDecl *requirement,
   InterfaceConformanceRef conformance) {
   auto &ctx = getAstContext();
   auto pair = ctx.getImpl().DefaultAssociatedConformanceWitnesses.insert(
      std::make_pair(std::make_tuple(this, association, requirement),
                     conformance));
   assert(pair.second && "Already have a default associated conformance");
   (void) pair;
}

void AstContext::getVisibleTopLevelModuleNames(
   SmallVectorImpl<Identifier> &names) const {
   names.clear();
   for (auto &importer : getImpl().ModuleLoaders)
      importer->collectVisibleTopLevelModuleNames(names);

   // Sort and unique.
   std::sort(names.begin(), names.end(), [](Identifier LHS, Identifier RHS) {
      return LHS.str().compare_lower(RHS.str()) < 0;
   });
   names.erase(std::unique(names.begin(), names.end()), names.end());
}

bool AstContext::shouldPerformTypoCorrection() {
   NumTypoCorrections += 1;
   return NumTypoCorrections <= LangOpts.TypoCorrectionLimit;
}

bool AstContext::canImportModule(std::pair<Identifier, SourceLoc> ModulePath) {
   // If this module has already been successfully imported, it is importable.
   if (getLoadedModule(ModulePath) != nullptr)
      return true;

   // If we've failed loading this module before, don't look for it again.
   if (FailedModuleImportNames.count(ModulePath.first))
      return false;

   // Otherwise, ask the module loaders.
   for (auto &importer : getImpl().ModuleLoaders) {
      if (importer->canImportModule(ModulePath)) {
         return true;
      }
   }

   FailedModuleImportNames.insert(ModulePath.first);
   return false;
}

ModuleDecl *
AstContext::getModule(ArrayRef<std::pair<Identifier, SourceLoc>> ModulePath) {
   assert(!ModulePath.empty());

   if (auto *M = getLoadedModule(ModulePath))
      return M;

   auto moduleID = ModulePath[0];
   for (auto &importer : getImpl().ModuleLoaders) {
      if (ModuleDecl *M = importer->loadModule(moduleID.second, ModulePath)) {
         return M;
      }
   }

   return nullptr;
}

ModuleDecl *AstContext::getModuleByName(StringRef ModuleName) {
   SmallVector<std::pair<Identifier, SourceLoc>, 4>
      AccessPath;
   while (!ModuleName.empty()) {
      StringRef SubModuleName;
      std::tie(SubModuleName, ModuleName) = ModuleName.split('.');
      AccessPath.push_back({getIdentifier(SubModuleName), SourceLoc()});
   }
   return getModule(AccessPath);
}

ModuleDecl *AstContext::getStdlibModule(bool loadIfAbsent) {
   if (TheStdlibModule)
      return TheStdlibModule;

   if (loadIfAbsent) {
      auto mutableThis = const_cast<AstContext *>(this);
      TheStdlibModule =
         mutableThis->getModule({std::make_pair(StdlibModuleName, SourceLoc())});
   } else {
      TheStdlibModule = getLoadedModule(StdlibModuleName);
   }
   return TheStdlibModule;
}

Optional<RawComment> AstContext::getRawComment(const Decl *D) {
   auto Known = getImpl().RawComments.find(D);
   if (Known == getImpl().RawComments.end())
      return None;

   return Known->second;
}

void AstContext::setRawComment(const Decl *D, RawComment RefCountPtr) {
   getImpl().RawComments[D] = RefCountPtr;
}

Optional<StringRef> AstContext::getBriefComment(const Decl *D) {
   auto Known = getImpl().BriefComments.find(D);
   if (Known == getImpl().BriefComments.end())
      return None;

   return Known->second;
}

void AstContext::setBriefComment(const Decl *D, StringRef Comment) {
   getImpl().BriefComments[D] = Comment;
}

NormalInterfaceConformance *
AstContext::getConformance(Type conformingType,
                           InterfaceDecl *protocol,
                           SourceLoc loc,
                           DeclContext *dc,
                           InterfaceConformanceState state) {
   assert(dc->isTypeContext());

   llvm::FoldingSetNodeID id;
   NormalInterfaceConformance::Profile(id, protocol, dc);

   // Did we already record the normal conformance?
   void *insertPos;
   auto &normalConformances =
      getImpl().getArena(AllocationArena::Permanent).NormalConformances;
   if (auto result = normalConformances.FindNodeOrInsertPos(id, insertPos))
      return result;

   // Build a new normal protocol conformance.
   auto result
      = new(*this, AllocationArena::Permanent)
         NormalInterfaceConformance(conformingType, protocol, loc, dc, state);
   normalConformances.InsertNode(result, insertPos);

   return result;
}

/// Produce a self-conformance for the given protocol.
SelfInterfaceConformance *
AstContext::getSelfConformance(InterfaceDecl *protocol) {
   auto &selfConformances =
      getImpl().getArena(AllocationArena::Permanent).SelfConformances;
   auto &entry = selfConformances[protocol];
   if (!entry) {
      entry = new(*this, AllocationArena::Permanent)
         SelfInterfaceConformance(protocol->getDeclaredInterfaceType());
   }
   return entry;
}

/// If one of the ancestor conformances already has a matching type, use
/// that instead.
static InterfaceConformance *collapseSpecializedConformance(
   Type type,
   InterfaceConformance *conformance,
   SubstitutionMap substitutions) {
   while (true) {
      switch (conformance->getKind()) {
         case InterfaceConformanceKind::Specialized:
            conformance = cast<SpecializedInterfaceConformance>(conformance)
               ->getGenericConformance();
            break;

         case InterfaceConformanceKind::Normal:
         case InterfaceConformanceKind::Inherited:
         case InterfaceConformanceKind::Self:
            // If the conformance matches, return it.
            if (conformance->getType()->isEqual(type)) {
               for (auto subConformance : substitutions.getConformances())
                  if (!subConformance.isAbstract())
                     return nullptr;

               return conformance;
            }

            return nullptr;
      }
   }
}

InterfaceConformance *
AstContext::getSpecializedConformance(Type type,
                                      InterfaceConformance *generic,
                                      SubstitutionMap substitutions) {
   // If we are performing a substitution that would get us back to the
   // a prior conformance (e.g., mapping into and then out of a conformance),
   // return the existing conformance.
   if (auto existing = collapseSpecializedConformance(type, generic,
                                                      substitutions)) {
      ++NumCollapsedSpecializedInterfaceConformances;
      return existing;
   }

   llvm::FoldingSetNodeID id;
   SpecializedInterfaceConformance::Profile(id, type, generic, substitutions);

   // Figure out which arena this conformance should go into.
   AllocationArena arena = getArena(type->getRecursiveProperties());

   // Did we already record the specialized conformance?
   void *insertPos;
   auto &specializedConformances = getImpl().getArena(arena).SpecializedConformances;
   if (auto result = specializedConformances.FindNodeOrInsertPos(id, insertPos))
      return result;

   // Build a new specialized conformance.
   auto result
      = new(*this, arena) SpecializedInterfaceConformance(type, generic,
                                                          substitutions);
   auto node = specializedConformances.FindNodeOrInsertPos(id, insertPos);
   (void) node;
   assert(!node);
   specializedConformances.InsertNode(result, insertPos);
   return result;
}

InheritedInterfaceConformance *
AstContext::getInheritedConformance(Type type, InterfaceConformance *inherited) {
   llvm::FoldingSetNodeID id;
   InheritedInterfaceConformance::Profile(id, type, inherited);

   // Figure out which arena this conformance should go into.
   AllocationArena arena = getArena(type->getRecursiveProperties());

   // Did we already record the normal protocol conformance?
   void *insertPos;
   auto &inheritedConformances = getImpl().getArena(arena).InheritedConformances;
   if (auto result
      = inheritedConformances.FindNodeOrInsertPos(id, insertPos))
      return result;

   // Build a new normal protocol conformance.
   auto result = new(*this, arena) InheritedInterfaceConformance(type, inherited);
   inheritedConformances.InsertNode(result, insertPos);
   return result;
}

LazyContextData *AstContext::getOrCreateLazyContextData(
   const DeclContext *dc,
   LazyMemberLoader *lazyLoader) {
   LazyContextData *&entry = getImpl().LazyContexts[dc];
   if (entry) {
      // Make sure we didn't provide an incompatible lazy loader.
      assert(!lazyLoader || lazyLoader == entry->loader);
      return entry;
   }

   // Create new lazy context data with the given loader.
   assert(lazyLoader && "Queried lazy data for non-lazy iterable context");
   if (isa<InterfaceDecl>(dc))
      entry = Allocate<LazyInterfaceData>();
   else {
      assert(isa<NominalTypeDecl>(dc) || isa<ExtensionDecl>(dc));
      entry = Allocate<LazyIterableDeclContextData>();
   }

   entry->loader = lazyLoader;
   return entry;
}

LazyIterableDeclContextData *AstContext::getOrCreateLazyIterableContextData(
   const IterableDeclContext *idc,
   LazyMemberLoader *lazyLoader) {
   if (auto ext = dyn_cast<ExtensionDecl>(idc)) {
      return (LazyIterableDeclContextData *) getOrCreateLazyContextData(
         ext, lazyLoader);
   }

   auto nominal = cast<NominalTypeDecl>(idc);
   return (LazyIterableDeclContextData *) getOrCreateLazyContextData(nominal,
                                                                     lazyLoader);
}

bool AstContext::hasDelayedConformanceErrors() const {
   for (const auto &entry : getImpl().DelayedConformanceDiags) {
      auto &diagnostics = entry.getSecond();
      if (std::any_of(diagnostics.begin(), diagnostics.end(),
                      [](const AstContext::DelayedConformanceDiag &diag) {
                         return diag.IsError;
                      }))
         return true;
   }

   return false;
}

void AstContext::addDelayedConformanceDiag(
   NormalInterfaceConformance *conformance,
   DelayedConformanceDiag fn) {
   getImpl().DelayedConformanceDiags[conformance].push_back(std::move(fn));
}

void AstContext::
addDelayedMissingWitnesses(NormalInterfaceConformance *conformance,
                           ArrayRef<ValueDecl *> witnesses) {
   auto &bucket = getImpl().DelayedMissingWitnesses[conformance];
   bucket.insert(bucket.end(), witnesses.begin(), witnesses.end());
}

std::vector<ValueDecl *> AstContext::
takeDelayedMissingWitnesses(NormalInterfaceConformance *conformance) {
   std::vector<ValueDecl *> result;
   auto known = getImpl().DelayedMissingWitnesses.find(conformance);
   if (known != getImpl().DelayedMissingWitnesses.end()) {
      result = std::move(known->second);
      getImpl().DelayedMissingWitnesses.erase(known);
   }
   return result;
}

std::vector<AstContext::DelayedConformanceDiag>
AstContext::takeDelayedConformanceDiags(NormalInterfaceConformance *conformance) {
   std::vector<AstContext::DelayedConformanceDiag> result;
   auto known = getImpl().DelayedConformanceDiags.find(conformance);
   if (known != getImpl().DelayedConformanceDiags.end()) {
      result = std::move(known->second);
      getImpl().DelayedConformanceDiags.erase(known);
   }
   return result;
}

size_t AstContext::getTotalMemory() const {
   size_t Size = sizeof(*this) +
                 // LoadedModules ?
                 llvm::capacity_in_bytes(CanonicalGenericTypeParamTypeNames) +
                 // RemappedTypes ?
                 sizeof(getImpl()) +
                 getImpl().Allocator.getTotalMemory() +
                 getImpl().Cleanups.capacity() +
                 llvm::capacity_in_bytes(getImpl().ModuleLoaders) +
                 llvm::capacity_in_bytes(getImpl().RawComments) +
                 llvm::capacity_in_bytes(getImpl().BriefComments) +
                 llvm::capacity_in_bytes(getImpl().ModuleTypes) +
                 llvm::capacity_in_bytes(getImpl().GenericParamTypes) +
                 // getImpl().GenericFunctionTypes ?
                 // getImpl().PILFunctionTypes ?
                 llvm::capacity_in_bytes(getImpl().PILBlockStorageTypes) +
                 llvm::capacity_in_bytes(getImpl().IntegerTypes) +
                 // getImpl().InterfaceCompositionTypes ?
                 // getImpl().BuiltinVectorTypes ?
                 // getImpl().GenericSignatures ?
                 // getImpl().CompoundNames ?
                 getImpl().OpenedExistentialArchetypes.getMemorySize() +
                 getImpl().Permanent.getTotalMemory();

   Size += getSolverMemory();

   return Size;
}

size_t AstContext::getSolverMemory() const {
   size_t Size = 0;

   if (getImpl().CurrentConstraintSolverArena) {
      Size += getImpl().CurrentConstraintSolverArena->getTotalMemory();
      Size += getImpl().CurrentConstraintSolverArena->Allocator.getBytesAllocated();
   }

   return Size;
}

size_t AstContext::Implementation::Arena::getTotalMemory() const {
   return sizeof(*this) +
          // TupleTypes ?
          llvm::capacity_in_bytes(MetatypeTypes) +
          llvm::capacity_in_bytes(ExistentialMetatypeTypes) +
          llvm::capacity_in_bytes(ArraySliceTypes) +
          llvm::capacity_in_bytes(DictionaryTypes) +
          llvm::capacity_in_bytes(OptionalTypes) +
          llvm::capacity_in_bytes(SimpleParenTypes) +
          llvm::capacity_in_bytes(ParenTypes) +
          llvm::capacity_in_bytes(ReferenceStorageTypes) +
          llvm::capacity_in_bytes(LValueTypes) +
          llvm::capacity_in_bytes(InOutTypes) +
          llvm::capacity_in_bytes(DependentMemberTypes) +
          llvm::capacity_in_bytes(EnumTypes) +
          llvm::capacity_in_bytes(StructTypes) +
          llvm::capacity_in_bytes(ClassTypes) +
          llvm::capacity_in_bytes(InterfaceTypes) +
          llvm::capacity_in_bytes(DynamicSelfTypes);
   // FunctionTypes ?
   // UnboundGenericTypes ?
   // BoundGenericTypes ?
   // NormalConformances ?
   // SpecializedConformances ?
   // InheritedConformances ?
}

void AbstractFunctionDecl::setForeignErrorConvention(
   const ForeignErrorConvention &conv) {
   assert(hasThrows() && "setting error convention on non-throwing decl");
   auto &conventionsMap = getAstContext().getImpl().ForeignErrorConventions;
   assert(!conventionsMap.count(this) && "error convention already set");
   conventionsMap.insert({this, conv});
}

Optional<ForeignErrorConvention>
AbstractFunctionDecl::getForeignErrorConvention() const {
   if (!hasThrows())
      return None;
   auto &conventionsMap = getAstContext().getImpl().ForeignErrorConventions;
   auto it = conventionsMap.find(this);
   if (it == conventionsMap.end()) return None;
   return it->second;
}

//===----------------------------------------------------------------------===//
// Type manipulation routines.
//===----------------------------------------------------------------------===//

TypeAliasType::TypeAliasType(TypeAliasDecl *typealias, Type parent,
                             SubstitutionMap substitutions,
                             Type underlying,
                             RecursiveTypeProperties properties)
   : SugarType(TypeKind::TypeAlias, underlying, properties),
     typealias(typealias) {
   // Record the parent (or absence of a parent).
   if (parent) {
      Bits.TypeAliasType.HasParent = true;
      *getTrailingObjects<Type>() = parent;
   } else {
      Bits.TypeAliasType.HasParent = false;
   }

   // Record the substitutions.
   if (substitutions) {
      Bits.TypeAliasType.HasSubstitutionMap = true;
      *getTrailingObjects<SubstitutionMap>() = substitutions;
   } else {
      Bits.TypeAliasType.HasSubstitutionMap = false;
   }
}

TypeAliasType *TypeAliasType::get(TypeAliasDecl *typealias, Type parent,
                                  SubstitutionMap substitutions,
                                  Type underlying) {
   // Compute the recursive properties.
   //
   auto properties = underlying->getRecursiveProperties();
   if (parent)
      properties |= parent->getRecursiveProperties();

   for (auto substGP : substitutions.getReplacementTypes())
      properties |= substGP->getRecursiveProperties();

   // Figure out which arena this type will go into.
   auto &ctx = underlying->getAstContext();
   auto arena = getArena(properties);

   // Profile the type.
   llvm::FoldingSetNodeID id;
   TypeAliasType::Profile(id, typealias, parent, substitutions, underlying);

   // Did we already record this type?
   void *insertPos;
   auto &types = ctx.getImpl().getArena(arena).TypeAliasTypes;
   if (auto result = types.FindNodeOrInsertPos(id, insertPos))
      return result;

   // Build a new type.
   auto genericSig = substitutions.getGenericSignature();
   auto size = totalSizeToAlloc<Type, SubstitutionMap>(parent ? 1 : 0,
                                                       genericSig ? 1 : 0);
   auto mem = ctx.Allocate(size, alignof(TypeAliasType), arena);
   auto result = new(mem) TypeAliasType(typealias, parent, substitutions,
                                        underlying, properties);
   types.InsertNode(result, insertPos);
   return result;
}

void TypeAliasType::Profile(llvm::FoldingSetNodeID &id) const {
   Profile(id, getDecl(), getParent(), getSubstitutionMap(),
           Type(getSinglyDesugaredType()));
}

void TypeAliasType::Profile(
   llvm::FoldingSetNodeID &id,
   TypeAliasDecl *typealias,
   Type parent, SubstitutionMap substitutions,
   Type underlying) {
   id.AddPointer(typealias);
   id.AddPointer(parent.getPointer());
   substitutions.profile(id);
   id.AddPointer(underlying.getPointer());
}

// Simple accessors.
Type ErrorType::get(const AstContext &C) { return C.TheErrorType; }

Type ErrorType::get(Type originalType) {
   assert(originalType);

   auto originalProperties = originalType->getRecursiveProperties();
   auto arena = getArena(originalProperties);

   auto &ctx = originalType->getAstContext();
   auto &entry = ctx.getImpl().getArena(arena).ErrorTypesWithOriginal[originalType];
   if (entry) return entry;

   void *mem = ctx.Allocate(sizeof(ErrorType) + sizeof(Type),
                            alignof(ErrorType), arena);
   RecursiveTypeProperties properties = RecursiveTypeProperties::HasError;
   if (originalProperties.hasTypeVariable())
      properties |= RecursiveTypeProperties::HasTypeVariable;
   return entry = new(mem) ErrorType(ctx, originalType, properties);
}

BuiltinIntegerType *BuiltinIntegerType::get(BuiltinIntegerWidth BitWidth,
                                            const AstContext &C) {
   assert(!BitWidth.isArbitraryWidth());
   BuiltinIntegerType *&Result = C.getImpl().IntegerTypes[BitWidth];
   if (Result == nullptr)
      Result = new(C, AllocationArena::Permanent) BuiltinIntegerType(BitWidth, C);
   return Result;
}

BuiltinVectorType *BuiltinVectorType::get(const AstContext &context,
                                          Type elementType,
                                          unsigned numElements) {
   llvm::FoldingSetNodeID id;
   BuiltinVectorType::Profile(id, elementType, numElements);

   void *insertPos;
   if (BuiltinVectorType *vecType
      = context.getImpl().BuiltinVectorTypes.FindNodeOrInsertPos(id, insertPos))
      return vecType;

   assert(elementType->isCanonical() && "Non-canonical builtin vector?");
   BuiltinVectorType *vecTy
      = new(context, AllocationArena::Permanent)
         BuiltinVectorType(context, elementType, numElements);
   context.getImpl().BuiltinVectorTypes.InsertNode(vecTy, insertPos);
   return vecTy;
}

ParenType *ParenType::get(const AstContext &C, Type underlying,
                          ParameterTypeFlags fl) {
   if (fl.isInOut())
      assert(!underlying->is<InOutType>() && "caller did not pass a base type");
   if (underlying->is<InOutType>())
      assert(fl.isInOut() && "caller did not set flags correctly");

   auto properties = underlying->getRecursiveProperties();
   auto arena = getArena(properties);
   auto flags = fl.toRaw();
   ParenType *&Result = flags == 0
                        ? C.getImpl().getArena(arena).SimpleParenTypes[underlying]
                        : C.getImpl().getArena(arena).ParenTypes[{underlying, flags}];
   if (Result == nullptr) {
      Result = new(C, arena) ParenType(underlying,
                                       properties, fl);
   }
   return Result;
}

CanTupleType TupleType::getEmpty(const AstContext &C) {
   return cast<TupleType>(CanType(C.TheEmptyTupleType));
}

void TupleType::Profile(llvm::FoldingSetNodeID &ID,
                        ArrayRef<TupleTypeElt> Fields) {
   ID.AddInteger(Fields.size());
   for (const TupleTypeElt &Elt : Fields) {
      ID.AddPointer(Elt.Name.get());
      ID.AddPointer(Elt.getType().getPointer());
      ID.AddInteger(Elt.Flags.toRaw());
   }
}

/// getTupleType - Return the uniqued tuple type with the specified elements.
Type TupleType::get(ArrayRef<TupleTypeElt> Fields, const AstContext &C) {
   if (Fields.size() == 1 && !Fields[0].isVararg() && !Fields[0].hasName())
      return ParenType::get(C, Fields[0].getRawType(),
                            Fields[0].getParameterFlags());

   RecursiveTypeProperties properties;
   bool hasElementWithOwnership = false;
   for (const TupleTypeElt &Elt : Fields) {
      auto eltTy = Elt.getType();
      if (!eltTy) continue;

      properties |= eltTy->getRecursiveProperties();
      // Recur into paren types and canonicalized paren types.  'inout' in nested
      // non-paren tuples are malformed and will be diagnosed later.
      if (auto *TTy = Elt.getType()->getAs<TupleType>()) {
         if (TTy->getNumElements() == 1)
            hasElementWithOwnership |= TTy->hasElementWithOwnership();
      } else if (auto *Pty = dyn_cast<ParenType>(Elt.getType().getPointer())) {
         hasElementWithOwnership |= (Pty->getParameterFlags().getValueOwnership() !=
                                     ValueOwnership::Default);
      } else {
         hasElementWithOwnership |= (Elt.getParameterFlags().getValueOwnership() !=
                                     ValueOwnership::Default);
      }
   }

   auto arena = getArena(properties);

   void *InsertPos = nullptr;
   // Check to see if we've already seen this tuple before.
   llvm::FoldingSetNodeID ID;
   TupleType::Profile(ID, Fields);

   if (TupleType *TT
      = C.getImpl().getArena(arena).TupleTypes.FindNodeOrInsertPos(ID, InsertPos))
      return TT;

   bool IsCanonical = true;   // All canonical elts means this is canonical.
   for (const TupleTypeElt &Elt : Fields) {
      if (Elt.getType().isNull() || !Elt.getType()->isCanonical()) {
         IsCanonical = false;
         break;
      }
   }

   // TupleType will copy the fields list into AstContext owned memory.
   void *mem = C.Allocate(sizeof(TupleType) +
                          sizeof(TupleTypeElt) * Fields.size(),
                          alignof(TupleType), arena);
   auto New = new(mem) TupleType(Fields, IsCanonical ? &C : nullptr, properties,
                                 hasElementWithOwnership);
   C.getImpl().getArena(arena).TupleTypes.InsertNode(New, InsertPos);
   return New;
}

TupleTypeElt::TupleTypeElt(Type ty, Identifier name,
                           ParameterTypeFlags fl)
   : Name(name), ElementType(ty), Flags(fl) {
   if (fl.isInOut())
      assert(!ty->is<InOutType>() && "caller did not pass a base type");
   if (ty->is<InOutType>())
      assert(fl.isInOut() && "caller did not set flags correctly");
}

Type TupleTypeElt::getType() const {
   if (Flags.isInOut()) return InOutType::get(ElementType);
   return ElementType;
}

Type AnyFunctionType::Param::getOldType() const {
   if (Flags.isInOut()) return InOutType::get(Ty);
   return Ty;
}

AnyFunctionType::Param computeSelfParam(AbstractFunctionDecl *AFD,
                                               bool isInitializingCtor,
                                               bool wantDynamicSelf) {
   auto *dc = AFD->getDeclContext();
   auto &Ctx = dc->getAstContext();

   // Determine the type of the container.
   auto containerTy = dc->getDeclaredInterfaceType();
   if (!containerTy || containerTy->hasError())
      return AnyFunctionType::Param(ErrorType::get(Ctx));

   // Determine the type of 'self' inside the container.
   auto selfTy = dc->getSelfInterfaceType();
   if (!selfTy || selfTy->hasError())
      return AnyFunctionType::Param(ErrorType::get(Ctx));

   bool isStatic = false;
   SelfAccessKind selfAccess = SelfAccessKind::NonMutating;
   bool isDynamicSelf = false;

   if (auto *FD = dyn_cast<FuncDecl>(AFD)) {
      isStatic = FD->isStatic();
      selfAccess = FD->getSelfAccessKind();

      // `self`s type for subscripts and properties
      if (auto *AD = dyn_cast<AccessorDecl>(AFD)) {
         if (wantDynamicSelf && AD->getStorage()
            ->getValueInterfaceType()->hasDynamicSelfType())
            isDynamicSelf = true;
      }
         // Methods returning 'Self' have a dynamic 'self'.
         //
         // FIXME: All methods of non-final classes should have this.
      else if (wantDynamicSelf && FD->hasDynamicSelfResult())
         isDynamicSelf = true;
   } else if (auto *CD = dyn_cast<ConstructorDecl>(AFD)) {
      if (isInitializingCtor) {
         // initializing constructors of value types always have an implicitly
         // inout self.
         selfAccess = SelfAccessKind::Mutating;
      } else {
         // allocating constructors have metatype 'self'.
         isStatic = true;
      }

      // Convenience initializers have a dynamic 'self' in '-swift-version 5'.
      if (Ctx.isPolarphpVersionAtLeast(5)) {
         if (wantDynamicSelf && CD->isConvenienceInit())
            if (auto *classDecl = selfTy->getClassOrBoundGenericClass())
               if (!classDecl->isFinal())
                  isDynamicSelf = true;
      }
   } else if (isa<DestructorDecl>(AFD)) {
      // Destructors only correctly appear on classes today. (If move-only types
      // have destructors, they probably would want to consume self.)
      // Note that we can't assert(containerTy->hasReferenceSemantics()) here
      // since incorrect or incomplete code could have deinit decls in invalid
      // contexts, and we need to recover gracefully in those cases.
   }

   if (isDynamicSelf)
      selfTy = DynamicSelfType::get(selfTy, Ctx);

   // 'static' functions have 'self' of type metatype<T>.
   if (isStatic)
      return AnyFunctionType::Param(MetatypeType::get(selfTy, Ctx));

   // Reference types have 'self' of type T.
   if (containerTy->hasReferenceSemantics())
      return AnyFunctionType::Param(selfTy);

   auto flags = ParameterTypeFlags();
   switch (selfAccess) {
      case SelfAccessKind::Consuming:
         flags = flags.withOwned(true);
         break;
      case SelfAccessKind::Mutating:
         flags = flags.withInOut(true);
         break;
      case SelfAccessKind::NonMutating:
         // The default flagless state.
         break;
   }

   return AnyFunctionType::Param(selfTy, Identifier(), flags);
}

void UnboundGenericType::Profile(llvm::FoldingSetNodeID &ID,
                                 GenericTypeDecl *TheDecl, Type Parent) {
   ID.AddPointer(TheDecl);
   ID.AddPointer(Parent.getPointer());
}

UnboundGenericType *UnboundGenericType::
get(GenericTypeDecl *TheDecl, Type Parent, const AstContext &C) {
   llvm::FoldingSetNodeID ID;
   UnboundGenericType::Profile(ID, TheDecl, Parent);
   void *InsertPos = nullptr;
   RecursiveTypeProperties properties;
   if (Parent) properties |= Parent->getRecursiveProperties();
   auto arena = getArena(properties);

   if (auto unbound = C.getImpl().getArena(arena).UnboundGenericTypes
      .FindNodeOrInsertPos(ID, InsertPos))
      return unbound;

   auto result = new(C, arena) UnboundGenericType(TheDecl, Parent, C,
                                                  properties);
   C.getImpl().getArena(arena).UnboundGenericTypes.InsertNode(result, InsertPos);
   return result;
}

void BoundGenericType::Profile(llvm::FoldingSetNodeID &ID,
                               NominalTypeDecl *TheDecl, Type Parent,
                               ArrayRef<Type> GenericArgs) {
   ID.AddPointer(TheDecl);
   ID.AddPointer(Parent.getPointer());
   ID.AddInteger(GenericArgs.size());
   for (Type Arg : GenericArgs) {
      ID.AddPointer(Arg.getPointer());
   }
}

BoundGenericType::BoundGenericType(TypeKind theKind,
                                   NominalTypeDecl *theDecl,
                                   Type parent,
                                   ArrayRef<Type> genericArgs,
                                   const AstContext *context,
                                   RecursiveTypeProperties properties)
   : NominalOrBoundGenericNominalType(theDecl, parent, theKind, context,
                                      properties) {
   Bits.BoundGenericType.GenericArgCount = genericArgs.size();
   // Subtypes are required to provide storage for the generic arguments
   std::uninitialized_copy(genericArgs.begin(), genericArgs.end(),
                           getTrailingObjectsPointer());
}

BoundGenericType *BoundGenericType::get(NominalTypeDecl *TheDecl,
                                        Type Parent,
                                        ArrayRef<Type> GenericArgs) {
   assert(TheDecl->getGenericParams() && "must be a generic type decl");
   assert((!Parent || Parent->is<NominalType>() ||
           Parent->is<BoundGenericType>() ||
           Parent->is<UnboundGenericType>()) &&
          "parent must be a nominal type");

   AstContext &C = TheDecl->getDeclContext()->getAstContext();
   llvm::FoldingSetNodeID ID;
   BoundGenericType::Profile(ID, TheDecl, Parent, GenericArgs);
   RecursiveTypeProperties properties;
   if (Parent) properties |= Parent->getRecursiveProperties();
   for (Type Arg : GenericArgs) {
      properties |= Arg->getRecursiveProperties();
   }

   auto arena = getArena(properties);

   void *InsertPos = nullptr;
   if (BoundGenericType *BGT =
      C.getImpl().getArena(arena).BoundGenericTypes.FindNodeOrInsertPos(ID,
                                                                        InsertPos))
      return BGT;

   bool IsCanonical = !Parent || Parent->isCanonical();
   if (IsCanonical) {
      for (Type Arg : GenericArgs) {
         if (!Arg->isCanonical()) {
            IsCanonical = false;
            break;
         }
      }
   }

   BoundGenericType *newType;
   if (auto theClass = dyn_cast<ClassDecl>(TheDecl)) {
      auto sz = BoundGenericClassType::totalSizeToAlloc<Type>(GenericArgs.size());
      auto mem = C.Allocate(sz, alignof(BoundGenericClassType), arena);
      newType = new(mem) BoundGenericClassType(
         theClass, Parent, GenericArgs, IsCanonical ? &C : nullptr, properties);
   } else if (auto theStruct = dyn_cast<StructDecl>(TheDecl)) {
      auto sz = BoundGenericStructType::totalSizeToAlloc<Type>(GenericArgs.size());
      auto mem = C.Allocate(sz, alignof(BoundGenericStructType), arena);
      newType = new(mem) BoundGenericStructType(
         theStruct, Parent, GenericArgs, IsCanonical ? &C : nullptr, properties);
   } else if (auto theEnum = dyn_cast<EnumDecl>(TheDecl)) {
      auto sz = BoundGenericEnumType::totalSizeToAlloc<Type>(GenericArgs.size());
      auto mem = C.Allocate(sz, alignof(BoundGenericEnumType), arena);
      newType = new(mem) BoundGenericEnumType(
         theEnum, Parent, GenericArgs, IsCanonical ? &C : nullptr, properties);
   } else {
      llvm_unreachable("Unhandled NominalTypeDecl");
   }
   C.getImpl().getArena(arena).BoundGenericTypes.InsertNode(newType, InsertPos);

   return newType;
}

NominalType *NominalType::get(NominalTypeDecl *D, Type Parent, const AstContext &C) {
   assert((isa<InterfaceDecl>(D) || !D->getGenericParams()) &&
          "must be a non-generic type decl");
   assert((!Parent || Parent->is<NominalType>() ||
           Parent->is<BoundGenericType>() ||
           Parent->is<UnboundGenericType>()) &&
          "parent must be a nominal type");

   switch (D->getKind()) {
      case DeclKind::Enum:
         return EnumType::get(cast<EnumDecl>(D), Parent, C);
      case DeclKind::Struct:
         return StructType::get(cast<StructDecl>(D), Parent, C);
      case DeclKind::Class:
         return ClassType::get(cast<ClassDecl>(D), Parent, C);
      case DeclKind::Interface: {
         return InterfaceType::get(cast<InterfaceDecl>(D), Parent, C);
      }

      default:
         llvm_unreachable("Not a nominal declaration!");
   }
}

EnumType::EnumType(EnumDecl *TheDecl, Type Parent, const AstContext &C,
                   RecursiveTypeProperties properties)
   : NominalType(TypeKind::Enum, &C, TheDecl, Parent, properties) {}

EnumType *EnumType::get(EnumDecl *D, Type Parent, const AstContext &C) {
   RecursiveTypeProperties properties;
   if (Parent) properties |= Parent->getRecursiveProperties();
   auto arena = getArena(properties);

   auto *&known = C.getImpl().getArena(arena).EnumTypes[{D, Parent}];
   if (!known) {
      known = new(C, arena) EnumType(D, Parent, C, properties);
   }
   return known;
}

StructType::StructType(StructDecl *TheDecl, Type Parent, const AstContext &C,
                       RecursiveTypeProperties properties)
   : NominalType(TypeKind::Struct, &C, TheDecl, Parent, properties) {}

StructType *StructType::get(StructDecl *D, Type Parent, const AstContext &C) {
   RecursiveTypeProperties properties;
   if (Parent) properties |= Parent->getRecursiveProperties();
   auto arena = getArena(properties);

   auto *&known = C.getImpl().getArena(arena).StructTypes[{D, Parent}];
   if (!known) {
      known = new(C, arena) StructType(D, Parent, C, properties);
   }
   return known;
}

ClassType::ClassType(ClassDecl *TheDecl, Type Parent, const AstContext &C,
                     RecursiveTypeProperties properties)
   : NominalType(TypeKind::Class, &C, TheDecl, Parent, properties) {}

ClassType *ClassType::get(ClassDecl *D, Type Parent, const AstContext &C) {
   RecursiveTypeProperties properties;
   if (Parent) properties |= Parent->getRecursiveProperties();
   auto arena = getArena(properties);

   auto *&known = C.getImpl().getArena(arena).ClassTypes[{D, Parent}];
   if (!known) {
      known = new(C, arena) ClassType(D, Parent, C, properties);
   }
   return known;
}

InterfaceCompositionType *
InterfaceCompositionType::build(const AstContext &C, ArrayRef<Type> Members,
                                bool HasExplicitAnyObject) {
   // Check to see if we've already seen this protocol composition before.
   void *InsertPos = nullptr;
   llvm::FoldingSetNodeID ID;
   InterfaceCompositionType::Profile(ID, Members, HasExplicitAnyObject);

   bool isCanonical = true;
   RecursiveTypeProperties properties;
   for (Type t : Members) {
      if (!t->isCanonical())
         isCanonical = false;
      properties |= t->getRecursiveProperties();
   }

   // Create a new protocol composition type.
   auto arena = getArena(properties);

   if (auto compTy
      = C.getImpl().getArena(arena).InterfaceCompositionTypes
         .FindNodeOrInsertPos(ID, InsertPos))
      return compTy;

   // Use trailing objects for member type storage
   auto size = totalSizeToAlloc<Type>(Members.size());
   auto mem = C.Allocate(size, alignof(InterfaceCompositionType), arena);
   auto compTy = new(mem) InterfaceCompositionType(isCanonical ? &C : nullptr,
                                                   Members,
                                                   HasExplicitAnyObject,
                                                   properties);
   C.getImpl().getArena(arena).InterfaceCompositionTypes.InsertNode(compTy, InsertPos);
   return compTy;
}

ReferenceStorageType *ReferenceStorageType::get(Type T,
                                                ReferenceOwnership ownership,
                                                const AstContext &C) {
   assert(!T->hasTypeVariable()); // not meaningful in type-checker
   switch (optionalityOf(ownership)) {
      case ReferenceOwnershipOptionality::Disallowed:
         assert(!T->getOptionalObjectType() && "optional type is disallowed");
         break;
      case ReferenceOwnershipOptionality::Allowed:
         break;
      case ReferenceOwnershipOptionality::Required:
         assert(T->getOptionalObjectType() && "optional type is required");
         break;
   }

   auto properties = T->getRecursiveProperties();
   auto arena = getArena(properties);

   auto key = uintptr_t(T.getPointer()) | unsigned(ownership);
   auto &entry = C.getImpl().getArena(arena).ReferenceStorageTypes[key];
   if (entry) return entry;

   switch (ownership) {
      case ReferenceOwnership::Strong:
         llvm_unreachable("strong ownership does not use ReferenceStorageType");
#define REF_STORAGE(Name, ...) \
  case ReferenceOwnership::Name: \
    return entry = new (C, arena) \
      Name##StorageType(T, T->isCanonical() ? &C : nullptr, properties);

#include "polarphp/ast/ReferenceStorageDef.h"
   }
   llvm_unreachable("bad ownership");
}

AnyMetatypeType::AnyMetatypeType(TypeKind kind, const AstContext *C,
                                 RecursiveTypeProperties properties,
                                 Type instanceType,
                                 Optional<MetatypeRepresentation> repr)
   : TypeBase(kind, C, properties), InstanceType(instanceType) {
   if (repr) {
      Bits.AnyMetatypeType.Representation = static_cast<char>(*repr) + 1;
   } else {
      Bits.AnyMetatypeType.Representation = 0;
   }
}

MetatypeType *MetatypeType::get(Type T, Optional<MetatypeRepresentation> Repr,
                                const AstContext &Ctx) {
   auto properties = T->getRecursiveProperties();
   auto arena = getArena(properties);

   unsigned reprKey;
   if (Repr.hasValue())
      reprKey = static_cast<unsigned>(*Repr) + 1;
   else
      reprKey = 0;

   auto pair = llvm::PointerIntPair<TypeBase *, 3, unsigned>(T.getPointer(),
                                                             reprKey);

   MetatypeType *&Entry = Ctx.getImpl().getArena(arena).MetatypeTypes[pair];
   if (Entry) return Entry;

   return Entry = new(Ctx, arena) MetatypeType(
      T, T->isCanonical() ? &Ctx : nullptr, properties, Repr);
}

MetatypeType::MetatypeType(Type T, const AstContext *C,
                           RecursiveTypeProperties properties,
                           Optional<MetatypeRepresentation> repr)
   : AnyMetatypeType(TypeKind::Metatype, C, properties, T, repr) {
}

ExistentialMetatypeType *
ExistentialMetatypeType::get(Type T, Optional<MetatypeRepresentation> repr,
                             const AstContext &ctx) {
   auto properties = T->getRecursiveProperties();
   auto arena = getArena(properties);

   unsigned reprKey;
   if (repr.hasValue())
      reprKey = static_cast<unsigned>(*repr) + 1;
   else
      reprKey = 0;

   auto pair = llvm::PointerIntPair<TypeBase *, 3, unsigned>(T.getPointer(),
                                                             reprKey);

   auto &entry = ctx.getImpl().getArena(arena).ExistentialMetatypeTypes[pair];
   if (entry) return entry;

   return entry = new(ctx, arena) ExistentialMetatypeType(
      T, T->isCanonical() ? &ctx : nullptr, properties, repr);
}

ExistentialMetatypeType::ExistentialMetatypeType(Type T,
                                                 const AstContext *C,
                                                 RecursiveTypeProperties properties,
                                                 Optional<MetatypeRepresentation> repr)
   : AnyMetatypeType(TypeKind::ExistentialMetatype, C, properties, T, repr) {
   if (repr) {
      assert(*repr != MetatypeRepresentation::Thin &&
             "creating a thin existential metatype?");
      assert(getAstContext().LangOpts.EnableObjCInterop ||
             *repr != MetatypeRepresentation::ObjC);
   }
}

ModuleType *ModuleType::get(ModuleDecl *M) {
   AstContext &C = M->getAstContext();

   ModuleType *&Entry = C.getImpl().ModuleTypes[M];
   if (Entry) return Entry;

   return Entry = new(C, AllocationArena::Permanent) ModuleType(M, C);
}

DynamicSelfType *DynamicSelfType::get(Type selfType, const AstContext &ctx) {
   assert(selfType->isMaterializable()
          && "non-materializable dynamic self?");

   auto properties = selfType->getRecursiveProperties();
   auto arena = getArena(properties);

   auto &dynamicSelfTypes = ctx.getImpl().getArena(arena).DynamicSelfTypes;
   auto known = dynamicSelfTypes.find(selfType);
   if (known != dynamicSelfTypes.end())
      return known->second;

   auto result = new(ctx, arena) DynamicSelfType(selfType, ctx, properties);
   dynamicSelfTypes.insert({selfType, result});
   return result;
}

static RecursiveTypeProperties
getFunctionRecursiveProperties(ArrayRef<AnyFunctionType::Param> params,
                               Type result) {
   RecursiveTypeProperties properties;
   for (auto param : params)
      properties |= param.getPlainType()->getRecursiveProperties();
   properties |= result->getRecursiveProperties();
   properties &= ~RecursiveTypeProperties::IsLValue;
   return properties;
}

static bool
isFunctionTypeCanonical(ArrayRef<AnyFunctionType::Param> params,
                        Type result) {
   for (auto param : params) {
      if (!param.getPlainType()->isCanonical())
         return false;
   }

   return result->isCanonical();
}

// For now, generic function types cannot be dependent (in fact,
// they erase dependence) or contain type variables, and they're
// always materializable.
static RecursiveTypeProperties
getGenericFunctionRecursiveProperties(ArrayRef<AnyFunctionType::Param> params,
                                      Type result) {
   static_assert(RecursiveTypeProperties::BitWidth == 11,
                 "revisit this if you add new recursive type properties");
   RecursiveTypeProperties properties;

   for (auto param : params) {
      if (param.getPlainType()->getRecursiveProperties().hasError())
         properties |= RecursiveTypeProperties::HasError;
   }

   if (result->getRecursiveProperties().hasDynamicSelf())
      properties |= RecursiveTypeProperties::HasDynamicSelf;
   if (result->getRecursiveProperties().hasError())
      properties |= RecursiveTypeProperties::HasError;

   return properties;
}

static bool
isGenericFunctionTypeCanonical(GenericSignature sig,
                               ArrayRef<AnyFunctionType::Param> params,
                               Type result) {
   if (!sig->isCanonical())
      return false;

   for (auto param : params) {
      if (!sig->isCanonicalTypeInContext(param.getPlainType()))
         return false;
   }

   return sig->isCanonicalTypeInContext(result);
}

AnyFunctionType *AnyFunctionType::withExtInfo(ExtInfo info) const {
   if (isa<FunctionType>(this))
      return FunctionType::get(getParams(), getResult(), info);

   auto *genFnTy = cast<GenericFunctionType>(this);
   return GenericFunctionType::get(genFnTy->getGenericSignature(),
                                   getParams(), getResult(), info);
}

void AnyFunctionType::decomposeInput(
   Type type, SmallVectorImpl<AnyFunctionType::Param> &result) {
   switch (type->getKind()) {
      case TypeKind::Tuple: {
         auto tupleTy = cast<TupleType>(type.getPointer());
         for (auto &elt : tupleTy->getElements()) {
            result.emplace_back((elt.isVararg()
                                 ? elt.getVarargBaseTy()
                                 : elt.getRawType()),
                                elt.getName(),
                                elt.getParameterFlags());
         }
         return;
      }

      case TypeKind::Paren: {
         auto pty = cast<ParenType>(type.getPointer());
         result.emplace_back(pty->getUnderlyingType()->getInOutObjectType(),
                             Identifier(),
                             pty->getParameterFlags());
         return;
      }

      default:
         result.emplace_back(type->getInOutObjectType(), Identifier(),
                             ParameterTypeFlags::fromParameterType(
                                type, false, false, false, ValueOwnership::Default));
         return;
   }
}

Type AnyFunctionType::Param::getParameterType(bool forCanonical,
                                              AstContext *ctx) const {
   Type type = getPlainType();
   if (isVariadic()) {
      if (!ctx) ctx = &type->getAstContext();
      auto arrayDecl = ctx->getArrayDecl();
      if (!arrayDecl)
         type = ErrorType::get(*ctx);
      else if (forCanonical)
         type = BoundGenericType::get(arrayDecl, Type(), {type});
      else
         type = ArraySliceType::get(type);
   }
   return type;
}

Type AnyFunctionType::composeInput(AstContext &ctx, ArrayRef<Param> params,
                                   bool canonicalVararg) {
   SmallVector<TupleTypeElt, 4> elements;
   for (const auto &param : params) {
      Type eltType = param.getParameterType(canonicalVararg, &ctx);
      elements.emplace_back(eltType, param.getLabel(),
                            param.getParameterFlags());
   }
   return TupleType::get(elements, ctx);
}

bool AnyFunctionType::equalParams(ArrayRef<AnyFunctionType::Param> a,
                                  ArrayRef<AnyFunctionType::Param> b) {
   if (a.size() != b.size())
      return false;

   for (unsigned i = 0, n = a.size(); i != n; ++i) {
      if (a[i] != b[i])
         return false;
   }

   return true;
}

bool AnyFunctionType::equalParams(CanParamArrayRef a, CanParamArrayRef b) {
   if (a.size() != b.size())
      return false;

   for (unsigned i = 0, n = a.size(); i != n; ++i) {
      if (a[i] != b[i])
         return false;
   }

   return true;
}

void AnyFunctionType::relabelParams(MutableArrayRef<Param> params,
                                    ArrayRef<Identifier> labels) {
   assert(params.size() == labels.size());
   for (auto i : indices(params)) {
      auto &param = params[i];
      param = AnyFunctionType::Param(param.getPlainType(),
                                     labels[i],
                                     param.getParameterFlags());
   }
}

static void profileParams(llvm::FoldingSetNodeID &ID,
                          ArrayRef<AnyFunctionType::Param> params) {
   ID.AddInteger(params.size());
   for (auto param : params) {
      ID.AddPointer(param.getLabel().get());
      ID.AddPointer(param.getPlainType().getPointer());
      ID.AddInteger(param.getParameterFlags().toRaw());
   }
}

void FunctionType::Profile(llvm::FoldingSetNodeID &ID,
                           ArrayRef<AnyFunctionType::Param> params,
                           Type result,
                           ExtInfo info) {
   profileParams(ID, params);
   ID.AddPointer(result.getPointer());
   auto infoKey = info.getFuncAttrKey();
   ID.AddInteger(infoKey.first);
   ID.AddPointer(infoKey.second);
}

FunctionType *FunctionType::get(ArrayRef<AnyFunctionType::Param> params,
                                Type result, ExtInfo info) {
   auto properties = getFunctionRecursiveProperties(params, result);
   auto arena = getArena(properties);

   llvm::FoldingSetNodeID id;
   FunctionType::Profile(id, params, result, info);

   const AstContext &ctx = result->getAstContext();

   // Do we already have this generic function type?
   void *insertPos;
   if (auto funcTy =
      ctx.getImpl().getArena(arena).FunctionTypes.FindNodeOrInsertPos(id, insertPos)) {
      return funcTy;
   }

   Optional<ExtInfo::Uncommon> uncommon = info.getUncommonInfo();

   size_t allocSize =
      totalSizeToAlloc<AnyFunctionType::Param, ExtInfo::Uncommon>(
         params.size(), uncommon.hasValue() ? 1 : 0);
   void *mem = ctx.Allocate(allocSize, alignof(FunctionType), arena);

   bool isCanonical = isFunctionTypeCanonical(params, result);
   if (uncommon.hasValue())
      isCanonical &= uncommon->ClangFunctionType->isCanonicalUnqualified();

   auto funcTy = new(mem) FunctionType(params, result, info,
                                       isCanonical ? &ctx : nullptr,
                                       properties);
   ctx.getImpl().getArena(arena).FunctionTypes.InsertNode(funcTy, insertPos);
   return funcTy;
}

// If the input and result types are canonical, then so is the result.
FunctionType::FunctionType(ArrayRef<AnyFunctionType::Param> params,
                           Type output, ExtInfo info,
                           const AstContext *ctx,
                           RecursiveTypeProperties properties)
   : AnyFunctionType(TypeKind::Function, ctx,
                     output, properties, params.size(), info) {
   std::uninitialized_copy(params.begin(), params.end(),
                           getTrailingObjects<AnyFunctionType::Param>());
   Optional<ExtInfo::Uncommon> uncommon = info.getUncommonInfo();
   if (uncommon.hasValue())
      *getTrailingObjects<ExtInfo::Uncommon>() = uncommon.getValue();
}

void GenericFunctionType::Profile(llvm::FoldingSetNodeID &ID,
                                  GenericSignature sig,
                                  ArrayRef<AnyFunctionType::Param> params,
                                  Type result,
                                  ExtInfo info) {
   ID.AddPointer(sig.getPointer());
   profileParams(ID, params);
   ID.AddPointer(result.getPointer());
   auto infoKey = info.getFuncAttrKey();
   ID.AddInteger(infoKey.first);
   ID.AddPointer(infoKey.second);
}

GenericFunctionType *GenericFunctionType::get(GenericSignature sig,
                                              ArrayRef<Param> params,
                                              Type result,
                                              ExtInfo info) {
   assert(sig && "no generic signature for generic function type?!");
   assert(!result->hasTypeVariable());

   llvm::FoldingSetNodeID id;
   GenericFunctionType::Profile(id, sig, params, result, info);

   const AstContext &ctx = result->getAstContext();

   // Do we already have this generic function type?
   void *insertPos;
   if (auto result
      = ctx.getImpl().GenericFunctionTypes.FindNodeOrInsertPos(id, insertPos)) {
      return result;
   }

   // We have to construct this generic function type. Determine whether
   // it's canonical.  Unfortunately, isCanonicalTypeInContext can cause
   // new GenericFunctionTypes to be created and thus invalidate our insertion
   // point.
   bool isCanonical = isGenericFunctionTypeCanonical(sig, params, result);

   if (auto funcTy
      = ctx.getImpl().GenericFunctionTypes.FindNodeOrInsertPos(id, insertPos)) {
      return funcTy;
   }

   size_t allocSize = totalSizeToAlloc<AnyFunctionType::Param>(params.size());
   void *mem = ctx.Allocate(allocSize, alignof(GenericFunctionType));

   auto properties = getGenericFunctionRecursiveProperties(params, result);
   auto funcTy = new(mem) GenericFunctionType(sig, params, result, info,
                                              isCanonical ? &ctx : nullptr,
                                              properties);

   ctx.getImpl().GenericFunctionTypes.InsertNode(funcTy, insertPos);
   return funcTy;
}

GenericFunctionType::GenericFunctionType(
   GenericSignature sig,
   ArrayRef<AnyFunctionType::Param> params,
   Type result,
   ExtInfo info,
   const AstContext *ctx,
   RecursiveTypeProperties properties)
   : AnyFunctionType(TypeKind::GenericFunction, ctx, result,
                     properties, params.size(), info), Signature(sig) {
   std::uninitialized_copy(params.begin(), params.end(),
                           getTrailingObjects<AnyFunctionType::Param>());
}

GenericTypeParamType *GenericTypeParamType::get(unsigned depth, unsigned index,
                                                const AstContext &ctx) {
   auto known = ctx.getImpl().GenericParamTypes.find({depth, index});
   if (known != ctx.getImpl().GenericParamTypes.end())
      return known->second;

   auto result = new(ctx, AllocationArena::Permanent)
      GenericTypeParamType(depth, index, ctx);
   ctx.getImpl().GenericParamTypes[{depth, index}] = result;
   return result;
}

TypeArrayView<GenericTypeParamType>
GenericFunctionType::getGenericParams() const {
   return Signature->getGenericParams();
}

/// Retrieve the requirements of this polymorphic function type.
ArrayRef<Requirement> GenericFunctionType::getRequirements() const {
   return Signature->getRequirements();
}

void PILFunctionType::Profile(
   llvm::FoldingSetNodeID &id,
   GenericSignature genericParams,
   ExtInfo info,
   PILCoroutineKind coroutineKind,
   ParameterConvention calleeConvention,
   ArrayRef<PILParameterInfo> params,
   ArrayRef<PILYieldInfo> yields,
   ArrayRef<PILResultInfo> results,
   Optional<PILResultInfo> errorResult,
   InterfaceConformanceRef conformance,
   bool isGenericSignatureImplied,
   SubstitutionMap substitutions) {
   id.AddPointer(genericParams.getPointer());
   auto infoKey = info.getFuncAttrKey();
   id.AddInteger(infoKey.first);
   id.AddPointer(infoKey.second);
   id.AddInteger(unsigned(coroutineKind));
   id.AddInteger(unsigned(calleeConvention));
   id.AddInteger(params.size());
   for (auto param : params)
      param.profile(id);
   id.AddInteger(yields.size());
   for (auto yield : yields)
      yield.profile(id);
   id.AddInteger(results.size());
   for (auto result : results)
      result.profile(id);

   // Just allow the profile length to implicitly distinguish the
   // presence of an error result.
   if (errorResult) errorResult->profile(id);
   id.AddBoolean(isGenericSignatureImplied);
   substitutions.profile(id);
   id.AddBoolean((bool) conformance);
   if (conformance)
      id.AddPointer(conformance.getRequirement());
}

PILFunctionType::PILFunctionType(
   GenericSignature genericSig,
   ExtInfo ext,
   PILCoroutineKind coroutineKind,
   ParameterConvention calleeConvention,
   ArrayRef<PILParameterInfo> params,
   ArrayRef<PILYieldInfo> yields,
   ArrayRef<PILResultInfo> normalResults,
   Optional<PILResultInfo> errorResult,
   SubstitutionMap substitutions,
   bool genericSigIsImplied,
   const AstContext &ctx,
   RecursiveTypeProperties properties,
   InterfaceConformanceRef witnessMethodConformance)
   : TypeBase(TypeKind::PILFunction, &ctx, properties),
     GenericSigAndIsImplied(CanGenericSignature(genericSig),
                            genericSigIsImplied),
     WitnessMethodConformance(witnessMethodConformance),
     Substitutions(substitutions) {

   Bits.PILFunctionType.HasErrorResult = errorResult.hasValue();
   Bits.PILFunctionType.ExtInfoBits = ext.Bits;
   Bits.PILFunctionType.HasUncommonInfo = false;
   // The use of both assert() and static_assert() below is intentional.
   assert(Bits.PILFunctionType.ExtInfoBits == ext.Bits && "Bits were dropped!");
   static_assert(ExtInfo::NumMaskBits == NumPILExtInfoBits,
                 "ExtInfo and PILFunctionTypeBitfields must agree on bit size");
   Bits.PILFunctionType.CoroutineKind = unsigned(coroutineKind);
   NumParameters = params.size();
   if (coroutineKind == PILCoroutineKind::None) {
      assert(yields.empty());
      NumAnyResults = normalResults.size();
      NumAnyIndirectFormalResults =
         std::count_if(normalResults.begin(), normalResults.end(),
                       [](const PILResultInfo &resultInfo) {
                          return resultInfo.isFormalIndirect();
                       });
      memcpy(getMutableResults().data(), normalResults.data(),
             normalResults.size() * sizeof(PILResultInfo));
   } else {
      assert(normalResults.empty());
      NumAnyResults = yields.size();
      NumAnyIndirectFormalResults = 0; // unused
      memcpy(getMutableYields().data(), yields.data(),
             yields.size() * sizeof(PILYieldInfo));
   }

   assert(!isIndirectFormalParameter(calleeConvention));
   Bits.PILFunctionType.CalleeConvention = unsigned(calleeConvention);

   memcpy(getMutableParameters().data(), params.data(),
          params.size() * sizeof(PILParameterInfo));
   if (errorResult)
      getMutableErrorResult() = *errorResult;

   if (hasResultCache()) {
      getMutableFormalResultsCache() = CanType();
      getMutableAllResultsCache() = CanType();
   }
#ifndef NDEBUG
   if (ext.getRepresentation() == Representation::WitnessMethod)
      assert(!WitnessMethodConformance.isInvalid() &&
             "witness_method PIL function without a conformance");
   else
      assert(WitnessMethodConformance.isInvalid() &&
             "non-witness_method PIL function with a conformance");

   // Make sure the type follows invariants.
   assert((!substitutions || genericSig)
          && "can only have substitutions with a generic signature");
   assert((!genericSigIsImplied || substitutions)
          && "genericSigIsImplied should only be set for a type with generic "
             "types and substitutions");

   if (substitutions) {
      assert(substitutions.getGenericSignature()->getCanonicalSignature()
             == genericSig->getCanonicalSignature()
             && "substitutions must match generic signature");
   }

   if (genericSig) {
      assert(!genericSig->areAllParamsConcrete() &&
             "If all generic parameters are concrete, PILFunctionType should "
             "not have a generic signature at all");

      for (auto gparam : genericSig->getGenericParams()) {
         (void) gparam;
         assert(gparam->isCanonical() && "generic signature is not canonicalized");
      }

      for (auto param : getParameters()) {
         (void) param;
         assert(!param.getInterfaceType()->hasError()
                && "interface type of parameter should not contain error types");
         assert(!param.getInterfaceType()->hasArchetype()
                && "interface type of parameter should not contain context archetypes");
      }
      for (auto result : getResults()) {
         (void) result;
         assert(!result.getInterfaceType()->hasError()
                && "interface type of result should not contain error types");
         assert(!result.getInterfaceType()->hasArchetype()
                && "interface type of result should not contain context archetypes");
      }
      for (auto yield : getYields()) {
         (void) yield;
         assert(!yield.getInterfaceType()->hasError()
                && "interface type of yield should not contain error types");
         assert(!yield.getInterfaceType()->hasArchetype()
                && "interface type of yield should not contain context archetypes");
      }
      if (hasErrorResult()) {
         assert(!getErrorResult().getInterfaceType()->hasError()
                && "interface type of result should not contain error types");
         assert(!getErrorResult().getInterfaceType()->hasArchetype()
                && "interface type of result should not contain context archetypes");
      }
   }
   for (auto result : getResults()) {
      (void) result;
      if (auto *FnType = result.getInterfaceType()->getAs<PILFunctionType>()) {
         assert(!FnType->isNoEscape() &&
                "Cannot return an @noescape function type");
      }
   }
#endif
}

CanPILBlockStorageType PILBlockStorageType::get(CanType captureType) {
   AstContext &ctx = captureType->getAstContext();
   auto found = ctx.getImpl().PILBlockStorageTypes.find(captureType);
   if (found != ctx.getImpl().PILBlockStorageTypes.end())
      return CanPILBlockStorageType(found->second);

   void *mem = ctx.Allocate(sizeof(PILBlockStorageType),
                            alignof(PILBlockStorageType));

   PILBlockStorageType *storageTy = new(mem) PILBlockStorageType(captureType);
   ctx.getImpl().PILBlockStorageTypes.insert({captureType, storageTy});
   return CanPILBlockStorageType(storageTy);
}

CanPILFunctionType PILFunctionType::get(
   GenericSignature genericSig,
   ExtInfo ext, PILCoroutineKind coroutineKind,
   ParameterConvention callee,
   ArrayRef<PILParameterInfo> params,
   ArrayRef<PILYieldInfo> yields,
   ArrayRef<PILResultInfo> normalResults,
   Optional<PILResultInfo> errorResult,
   SubstitutionMap substitutions,
   bool genericSigIsImplied,
   const AstContext &ctx,
   InterfaceConformanceRef witnessMethodConformance) {
   assert(coroutineKind == PILCoroutineKind::None || normalResults.empty());
   assert(coroutineKind != PILCoroutineKind::None || yields.empty());
   assert(!ext.isPseudogeneric() || genericSig);

   llvm::FoldingSetNodeID id;
   PILFunctionType::Profile(id, genericSig, ext, coroutineKind, callee, params,
                            yields, normalResults, errorResult,
                            witnessMethodConformance, genericSigIsImplied,
                            substitutions);

   // Do we already have this generic function type?
   void *insertPos;
   if (auto result
      = ctx.getImpl().PILFunctionTypes.FindNodeOrInsertPos(id, insertPos))
      return CanPILFunctionType(result);

   // All PILFunctionTypes are canonical.

   // Allocate storage for the object.
   size_t bytes = sizeof(PILFunctionType)
                  + sizeof(PILParameterInfo) * params.size()
                  + sizeof(PILYieldInfo) * yields.size()
                  + sizeof(PILResultInfo) * normalResults.size()
                  + (errorResult ? sizeof(PILResultInfo) : 0)
                  + (normalResults.size() > 1 ? sizeof(CanType) * 2 : 0);
   void *mem = ctx.Allocate(bytes, alignof(PILFunctionType));

   RecursiveTypeProperties properties;
   static_assert(RecursiveTypeProperties::BitWidth == 11,
                 "revisit this if you add new recursive type properties");
   for (auto &param : params)
      properties |= param.getInterfaceType()->getRecursiveProperties();
   for (auto &yield : yields)
      properties |= yield.getInterfaceType()->getRecursiveProperties();
   for (auto &result : normalResults)
      properties |= result.getInterfaceType()->getRecursiveProperties();
   if (errorResult)
      properties |= errorResult->getInterfaceType()->getRecursiveProperties();

   // FIXME: If we ever have first-class polymorphic values, we'll need to
   // revisit this.
   if (genericSig) {
      properties.removeHasTypeParameter();
      properties.removeHasDependentMember();
   }

   for (auto replacement : substitutions.getReplacementTypes()) {
      properties |= replacement->getRecursiveProperties();
   }

   auto fnType =
      new(mem) PILFunctionType(genericSig, ext, coroutineKind, callee,
                               params, yields, normalResults, errorResult,
                               substitutions, genericSigIsImplied,
                               ctx, properties, witnessMethodConformance);
   ctx.getImpl().PILFunctionTypes.InsertNode(fnType, insertPos);
   return CanPILFunctionType(fnType);
}

ArraySliceType *ArraySliceType::get(Type base) {
   auto properties = base->getRecursiveProperties();
   auto arena = getArena(properties);

   const AstContext &C = base->getAstContext();

   ArraySliceType *&entry = C.getImpl().getArena(arena).ArraySliceTypes[base];
   if (entry) return entry;

   return entry = new(C, arena) ArraySliceType(C, base, properties);
}

DictionaryType *DictionaryType::get(Type keyType, Type valueType) {
   auto properties = keyType->getRecursiveProperties()
                     | valueType->getRecursiveProperties();
   auto arena = getArena(properties);

   const AstContext &C = keyType->getAstContext();

   DictionaryType *&entry
      = C.getImpl().getArena(arena).DictionaryTypes[{keyType, valueType}];
   if (entry) return entry;

   return entry = new(C, arena) DictionaryType(C, keyType, valueType,
                                               properties);
}

OptionalType *OptionalType::get(Type base) {
   auto properties = base->getRecursiveProperties();
   auto arena = getArena(properties);

   const AstContext &C = base->getAstContext();

   OptionalType *&entry = C.getImpl().getArena(arena).OptionalTypes[base];
   if (entry) return entry;

   return entry = new(C, arena) OptionalType(C, base, properties);
}

InterfaceType *InterfaceType::get(InterfaceDecl *D, Type Parent,
                                  const AstContext &C) {
   RecursiveTypeProperties properties;
   if (Parent) properties |= Parent->getRecursiveProperties();
   auto arena = getArena(properties);

   auto *&known = C.getImpl().getArena(arena).InterfaceTypes[{D, Parent}];
   if (!known) {
      known = new(C, arena) InterfaceType(D, Parent, C, properties);
   }
   return known;
}

InterfaceType::InterfaceType(InterfaceDecl *TheDecl, Type Parent,
                             const AstContext &Ctx,
                             RecursiveTypeProperties properties)
   : NominalType(TypeKind::Interface, &Ctx, TheDecl, Parent, properties) {}

LValueType *LValueType::get(Type objectTy) {
   assert(!objectTy->is<LValueType>() && !objectTy->is<InOutType>() &&
          "cannot have 'inout' or @lvalue wrapped inside an @lvalue");

   auto properties = objectTy->getRecursiveProperties()
                     | RecursiveTypeProperties::IsLValue;
   auto arena = getArena(properties);

   auto &C = objectTy->getAstContext();
   auto &entry = C.getImpl().getArena(arena).LValueTypes[objectTy];
   if (entry)
      return entry;

   const AstContext *canonicalContext = objectTy->isCanonical() ? &C : nullptr;
   return entry = new(C, arena) LValueType(objectTy, canonicalContext,
                                           properties);
}

InOutType *InOutType::get(Type objectTy) {
   assert(!objectTy->is<LValueType>() && !objectTy->is<InOutType>() &&
          "cannot have 'inout' or @lvalue wrapped inside an 'inout'");

   auto properties = objectTy->getRecursiveProperties();

   properties &= ~RecursiveTypeProperties::IsLValue;
   auto arena = getArena(properties);

   auto &C = objectTy->getAstContext();
   auto &entry = C.getImpl().getArena(arena).InOutTypes[objectTy];
   if (entry)
      return entry;

   const AstContext *canonicalContext = objectTy->isCanonical() ? &C : nullptr;
   return entry = new(C, arena) InOutType(objectTy, canonicalContext,
                                          properties);
}

DependentMemberType *DependentMemberType::get(Type base, Identifier name) {
   auto properties = base->getRecursiveProperties();
   properties |= RecursiveTypeProperties::HasDependentMember;
   auto arena = getArena(properties);

   llvm::PointerUnion<Identifier, AssociatedTypeDecl *> stored(name);
   const AstContext &ctx = base->getAstContext();
   auto *&known = ctx.getImpl().getArena(arena).DependentMemberTypes[
      {base, stored.getOpaqueValue()}];
   if (!known) {
      const AstContext *canonicalCtx = base->isCanonical() ? &ctx : nullptr;
      known = new(ctx, arena) DependentMemberType(base, name, canonicalCtx,
                                                  properties);
   }
   return known;
}

DependentMemberType *DependentMemberType::get(Type base,
                                              AssociatedTypeDecl *assocType) {
   assert(assocType && "Missing associated type");
   auto properties = base->getRecursiveProperties();
   properties |= RecursiveTypeProperties::HasDependentMember;
   auto arena = getArena(properties);

   llvm::PointerUnion<Identifier, AssociatedTypeDecl *> stored(assocType);
   const AstContext &ctx = base->getAstContext();
   auto *&known = ctx.getImpl().getArena(arena).DependentMemberTypes[
      {base, stored.getOpaqueValue()}];
   if (!known) {
      const AstContext *canonicalCtx = base->isCanonical() ? &ctx : nullptr;
      known = new(ctx, arena) DependentMemberType(base, assocType, canonicalCtx,
                                                  properties);
   }
   return known;
}

OpaqueTypeArchetypeType *
OpaqueTypeArchetypeType::get(OpaqueTypeDecl *Decl,
                             SubstitutionMap Substitutions) {
   // TODO: We could attempt to preserve type sugar in the substitution map.
   // Currently archetypes are assumed to be always canonical in many places,
   // though, so doing so would require fixing those places.
   Substitutions = Substitutions.getCanonical();

   llvm::FoldingSetNodeID id;
   Profile(id, Decl, Substitutions);

   auto &ctx = Decl->getAstContext();

   // An opaque type isn't contextually dependent like other archetypes, so
   // by itself, it doesn't impose the "Has Archetype" recursive property,
   // but the substituted types might. A disjoint "Has Opaque Archetype" tracks
   // the presence of opaque archetypes.
   RecursiveTypeProperties properties =
      RecursiveTypeProperties::HasOpaqueArchetype;
   for (auto type : Substitutions.getReplacementTypes()) {
      properties |= type->getRecursiveProperties();
   }

   auto arena = getArena(properties);

   llvm::FoldingSet<OpaqueTypeArchetypeType> &set
      = ctx.getImpl().getArena(arena).OpaqueArchetypes;

   {
      void *insertPos; // Discarded because the work below may invalidate the
      // insertion point inside the folding set
      if (auto existing = set.FindNodeOrInsertPos(id, insertPos)) {
         return existing;
      }
   }

   // Create a new opaque archetype.
   // It lives in an environment in which the interface generic arguments of the
   // decl have all been same-type-bound to the arguments from our substitution
   // map.
   SmallVector<Requirement, 2> newRequirements;

   // TODO: The proper thing to do to build the environment in which the opaque
   // type's archetype exists would be to take the generic signature of the
   // decl, feed it into a GenericSignatureBuilder, then add same-type
   // constraints into the builder to bind the outer generic parameters
   // to their substituted types provided by \c Substitutions. However,
   // this is problematic for interface types. In a situation like this:
   //
   // __opaque_type Foo<t_0_0: P>: Q // internal signature <t_0_0: P, t_1_0: Q>
   //
   // func bar<t_0_0, t_0_1, t_0_2: P>() -> Foo<t_0_2>
   //
   // we'd want to feed the GSB constraints to form:
   //
   // <t_0_0: P, t_1_0: Q where t_0_0 == t_0_2>
   //
   // even though t_0_2 isn't *in* the generic signature being built; it
   // represents a type
   // bound elsewhere from some other generic context. If we knew the generic
   // environment `t_0_2` came from, then maybe we could map it into that context,
   // but currently we have no way to know that with certainty.
   //
   // Because opaque types are currently limited so that they only have immediate
   // protocol constraints, and therefore don't interact with the outer generic
   // parameters at all, we can get away without adding these constraints for now.
   // Adding where clauses would break this hack.
#if DO_IT_CORRECTLY
                                                                                                                           // Same-type-constrain the arguments in the outer signature to their
  // replacements in the substitution map.
  if (auto outerSig = Decl->getGenericSignature()) {
    for (auto outerParam : outerSig->getGenericParams()) {
      auto boundType = Type(outerParam).subst(Substitutions);
      newRequirements.push_back(
          Requirement(RequirementKind::SameType, Type(outerParam), boundType));
    }
  }
#else
   // Assert that there are no same type constraints on the underlying type
   // or its associated types.
   //
   // This should not be possible until we add where clause support, with the
   // exception of generic base class constraints (handled below).
   (void) newRequirements;
# ifndef NDEBUG
   for (auto reqt :
      Decl->getOpaqueInterfaceGenericSignature()->getRequirements()) {
      auto reqtBase = reqt.getFirstType()->getRootGenericParam();
      if (reqtBase->isEqual(Decl->getUnderlyingInterfaceType())) {
         assert(reqt.getKind() != RequirementKind::SameType
                && "supporting where clauses on opaque types requires correctly "
                   "setting up the generic environment for "
                   "OpaqueTypeArchetypeTypes; see comment above");
      }
   }
# endif
#endif
   auto signature = evaluateOrDefault(
      ctx.evaluator,
      AbstractGenericSignatureRequest{
         Decl->getOpaqueInterfaceGenericSignature().getPointer(),
         /*genericParams=*/{},
         std::move(newRequirements)},
      nullptr);

   auto opaqueInterfaceTy = Decl->getUnderlyingInterfaceType();
   auto layout = signature->getLayoutConstraint(opaqueInterfaceTy);
   auto superclass = signature->getSuperclassBound(opaqueInterfaceTy);
   #if !DO_IT_CORRECTLY
   // Ad-hoc substitute the generic parameters of the superclass.
   // If we correctly applied the substitutions to the generic signature
   // constraints above, this would be unnecessary.
   if (superclass && superclass->hasTypeParameter()) {
      superclass = superclass.subst(Substitutions);
   }
   #endif
   SmallVector<InterfaceDecl *, 4> protos;
   for (auto proto : signature->getConformsTo(opaqueInterfaceTy)) {
      protos.push_back(proto);
   }

   auto mem = ctx.Allocate(
      OpaqueTypeArchetypeType::totalSizeToAlloc<InterfaceDecl *, Type, LayoutConstraint>(
         protos.size(), superclass ? 1 : 0, layout ? 1 : 0),
      alignof(OpaqueTypeArchetypeType),
      arena);

   auto newOpaque = ::new(mem) OpaqueTypeArchetypeType(Decl, Substitutions,
                                                       properties,
                                                       opaqueInterfaceTy,
                                                       protos, superclass, layout);

   // Create a generic environment and bind the opaque archetype to the
   // opaque interface type from the decl's signature.
   auto *builder = signature->getGenericSignatureBuilder();
   auto *env = GenericEnvironment::getIncomplete(signature, builder);
   env->addMapping(GenericParamKey(opaqueInterfaceTy), newOpaque);
   newOpaque->Environment = env;

   // Look up the insertion point in the folding set again in case something
   // invalidated it above.
   {
      void *insertPos;
      auto existing = set.FindNodeOrInsertPos(id, insertPos);
      (void) existing;
      assert(!existing && "race to create opaque archetype?!");
      set.InsertNode(newOpaque, insertPos);
   }

   return newOpaque;
}

CanOpenedArchetypeType OpenedArchetypeType::get(Type existential,
                                                Optional<UUID> knownID) {
   auto &ctx = existential->getAstContext();
   auto &openedExistentialArchetypes = ctx.getImpl().OpenedExistentialArchetypes;
   // If we know the ID already...
   if (knownID) {
      // ... and we already have an archetype for that ID, return it.
      auto found = openedExistentialArchetypes.find(*knownID);

      if (found != openedExistentialArchetypes.end()) {
         auto result = found->second;
         assert(result->getOpenedExistentialType()->isEqual(existential) &&
                "Retrieved the wrong opened existential type?");
         return CanOpenedArchetypeType(result);
      }
   } else {
      // Create a new ID.
      knownID = UUID::fromTime();
   }

   auto layout = existential->getExistentialLayout();

   SmallVector<InterfaceDecl *, 2> protos;
   for (auto proto : layout.getInterfaces())
      protos.push_back(proto->getDecl());

   auto layoutConstraint = layout.getLayoutConstraint();
   auto layoutSuperclass = layout.getSuperclass();

   auto arena = AllocationArena::Permanent;
   void *mem = ctx.Allocate(
      OpenedArchetypeType::totalSizeToAlloc<InterfaceDecl *, Type, LayoutConstraint>(
         protos.size(),
         layoutSuperclass ? 1 : 0,
         layoutConstraint ? 1 : 0),
      alignof(OpenedArchetypeType), arena);

   auto result =
      ::new(mem) OpenedArchetypeType(ctx, existential,
                                     protos, layoutSuperclass,
                                     layoutConstraint, *knownID);
   result->InterfaceType = GenericTypeParamType::get(0, 0, ctx);

   openedExistentialArchetypes[*knownID] = result;
   return CanOpenedArchetypeType(result);
}

GenericEnvironment *OpenedArchetypeType::getGenericEnvironment() const {
   if (Environment)
      return Environment;

   auto thisType = Type(const_cast<OpenedArchetypeType *>(this));
   auto &ctx = thisType->getAstContext();
   // Create a generic environment to represent the opened type.
   auto signature =
      ctx.getOpenedArchetypeSignature(Opened->getCanonicalType(), nullptr);
   auto *builder = signature->getGenericSignatureBuilder();
   auto *env = GenericEnvironment::getIncomplete(signature, builder);
   env->addMapping(signature->getGenericParams()[0], thisType);
   Environment = env;

   return env;
}

CanType OpenedArchetypeType::getAny(Type existential) {
   if (auto metatypeTy = existential->getAs<ExistentialMetatypeType>()) {
      auto instanceTy = metatypeTy->getInstanceType();
      return CanMetatypeType::get(OpenedArchetypeType::getAny(instanceTy));
   }
   assert(existential->isExistentialType());
   return OpenedArchetypeType::get(existential);
}

void TypeLoc::setInvalidType(AstContext &C) {
   Ty = ErrorType::get(C);
}

namespace {
class raw_capturing_ostream : public raw_ostream {
   std::string Message;
   uint64_t Pos;
   CapturingTypeCheckerDebugConsumer &Listener;

public:
   raw_capturing_ostream(CapturingTypeCheckerDebugConsumer &Listener)
      : Listener(Listener) {}

   ~raw_capturing_ostream() override {
      flush();
   }

   void write_impl(const char *Ptr, size_t Size) override {
      Message.append(Ptr, Size);
      Pos += Size;

      // Check if we have at least one complete line.
      size_t LastNewline = StringRef(Message).rfind('\n');
      if (LastNewline == StringRef::npos)
         return;
      Listener.handleMessage(StringRef(Message.data(), LastNewline + 1));
      Message.erase(0, LastNewline + 1);
   }

   uint64_t current_pos() const override {
      return Pos;
   }
};
} // unnamed namespace

TypeCheckerDebugConsumer::~TypeCheckerDebugConsumer() {}

CapturingTypeCheckerDebugConsumer::CapturingTypeCheckerDebugConsumer()
   : Log(new raw_capturing_ostream(*this)) {
   Log->SetUnbuffered();
}

void SubstitutionMap::Storage::Profile(
   llvm::FoldingSetNodeID &id,
   GenericSignature genericSig,
   ArrayRef<Type> replacementTypes,
   ArrayRef<InterfaceConformanceRef> conformances) {
   id.AddPointer(genericSig.getPointer());
   if (!genericSig) return;

   // Profile those replacement types that corresponding to canonical generic
   // parameters within the generic signature.
   id.AddInteger(replacementTypes.size());

   unsigned i = 0;
   genericSig->forEachParam([&](GenericTypeParamType *gp, bool canonical) {
      if (canonical)
         id.AddPointer(replacementTypes[i].getPointer());
      else
         id.AddPointer(nullptr);
      i++;
   });

   // Conformances.
   id.AddInteger(conformances.size());
   for (auto conformance : conformances)
      id.AddPointer(conformance.getOpaqueValue());
}

SubstitutionMap::Storage *SubstitutionMap::Storage::get(
   GenericSignature genericSig,
   ArrayRef<Type> replacementTypes,
   ArrayRef<InterfaceConformanceRef> conformances) {
   // If there is no generic signature, we need no storage.
   if (!genericSig) {
      assert(replacementTypes.empty());
      assert(conformances.empty());
      return nullptr;
   }

   // Figure out which arena this should go in.
   RecursiveTypeProperties properties;
   for (auto type : replacementTypes) {
      if (type)
         properties |= type->getRecursiveProperties();
   }

   // Profile the substitution map.
   llvm::FoldingSetNodeID id;
   SubstitutionMap::Storage::Profile(id, genericSig, replacementTypes,
                                     conformances);

   auto arena = getArena(properties);

   // Did we already record this substitution map?
   auto &ctx = genericSig->getAstContext();
   void *insertPos;
   auto &substitutionMaps = ctx.getImpl().getArena(arena).SubstitutionMaps;
   if (auto result = substitutionMaps.FindNodeOrInsertPos(id, insertPos))
      return result;

   // Allocate the appropriate amount of storage for the signature and its
   // replacement types and conformances.
   auto size = Storage::totalSizeToAlloc<Type, InterfaceConformanceRef>(
      replacementTypes.size(),
      conformances.size());
   auto mem = ctx.Allocate(size, alignof(Storage), arena);

   auto result = new(mem) Storage(genericSig, replacementTypes, conformances);
   substitutionMaps.InsertNode(result, insertPos);
   return result;
}

void GenericSignatureImpl::Profile(llvm::FoldingSetNodeID &ID,
                                   TypeArrayView<GenericTypeParamType> genericParams,
                                   ArrayRef<Requirement> requirements) {
   for (auto p : genericParams)
      ID.AddPointer(p);

   for (auto &reqt : requirements) {
      ID.AddPointer(reqt.getFirstType().getPointer());
      if (reqt.getKind() != RequirementKind::Layout)
         ID.AddPointer(reqt.getSecondType().getPointer());
      else
         ID.AddPointer(reqt.getLayoutConstraint().getPointer());
      ID.AddInteger(unsigned(reqt.getKind()));
   }
}

GenericSignature
GenericSignature::get(ArrayRef<GenericTypeParamType *> params,
                      ArrayRef<Requirement> requirements,
                      bool isKnownCanonical) {
   SmallVector<Type, 4> paramTypes;
   for (auto param : params)
      paramTypes.push_back(param);
   auto paramsView = TypeArrayView<GenericTypeParamType>(paramTypes);
   return get(paramsView, requirements, isKnownCanonical);
}

GenericSignature
GenericSignature::get(TypeArrayView<GenericTypeParamType> params,
                      ArrayRef<Requirement> requirements,
                      bool isKnownCanonical) {
   assert(!params.empty());

#ifndef NDEBUG
   for (auto req : requirements)
      assert(req.getFirstType()->isTypeParameter());
#endif

   // Check for an existing generic signature.
   llvm::FoldingSetNodeID ID;
   GenericSignatureImpl::Profile(ID, params, requirements);

   auto arena = GenericSignature::hasTypeVariable(requirements)
                ? AllocationArena::ConstraintSolver
                : AllocationArena::Permanent;

   auto &ctx = getAstContext(params, requirements);
   void *insertPos;
   auto &sigs = ctx.getImpl().getArena(arena).GenericSignatures;
   if (auto *sig = sigs.FindNodeOrInsertPos(ID, insertPos)) {
      if (isKnownCanonical)
         sig->CanonicalSignatureOrAstContext = &ctx;

      return sig;
   }

   // Allocate and construct the new signature.
   size_t bytes =
      GenericSignatureImpl::template totalSizeToAlloc<Type, Requirement>(
         params.size(), requirements.size());
   void *mem = ctx.Allocate(bytes, alignof(GenericSignatureImpl));
   auto *newSig =
      new(mem) GenericSignatureImpl(params, requirements, isKnownCanonical);
   ctx.getImpl().getArena(arena).GenericSignatures.InsertNode(newSig, insertPos);
   return newSig;
}

GenericEnvironment *GenericEnvironment::getIncomplete(
   GenericSignature signature,
   GenericSignatureBuilder *builder) {
   auto &ctx = signature->getAstContext();

   // Allocate and construct the new environment.
   unsigned numGenericParams = signature->getGenericParams().size();
   size_t bytes = totalSizeToAlloc<Type>(numGenericParams);
   void *mem = ctx.Allocate(bytes, alignof(GenericEnvironment));
   return new(mem) GenericEnvironment(signature, builder);
}

void DeclName::CompoundDeclName::Profile(llvm::FoldingSetNodeID &id,
                                         DeclBaseName baseName,
                                         ArrayRef<Identifier> argumentNames) {
   id.AddPointer(baseName.getAsOpaquePointer());
   id.AddInteger(argumentNames.size());
   for (auto arg : argumentNames)
      id.AddPointer(arg.get());
}

void DeclName::initialize(AstContext &C, DeclBaseName baseName,
                          ArrayRef<Identifier> argumentNames) {
   if (argumentNames.empty()) {
      SimpleOrCompound = BaseNameAndCompound(baseName, true);
      return;
   }

   llvm::FoldingSetNodeID id;
   CompoundDeclName::Profile(id, baseName, argumentNames);

   void *insert = nullptr;
   if (CompoundDeclName * compoundName
          = C.getImpl().CompoundNames.FindNodeOrInsertPos(id, insert)) {
      SimpleOrCompound = compoundName;
      return;
   }

   size_t size =
      CompoundDeclName::totalSizeToAlloc<Identifier>(argumentNames.size());
   auto buf = C.Allocate(size, alignof(CompoundDeclName));
   auto compoundName = new(buf) CompoundDeclName(baseName, argumentNames.size());
   std::uninitialized_copy(argumentNames.begin(), argumentNames.end(),
                           compoundName->getArgumentNames().begin());
   SimpleOrCompound = compoundName;
   C.getImpl().CompoundNames.InsertNode(compoundName, insert);
}

/// Build a compound value name given a base name and a set of argument names
/// extracted from a parameter list.
DeclName::DeclName(AstContext &C, DeclBaseName baseName,
                   ParameterList *paramList) {
   SmallVector<Identifier, 4> names;

   for (auto P : *paramList)
      names.push_back(P->getArgumentName());
   initialize(C, baseName, names);
}

/// Find the implementation of the named type in the given module.
static NominalTypeDecl *findUnderlyingTypeInModule(AstContext &ctx,
                                                   Identifier name,
                                                   ModuleDecl *module) {
   // Find all of the declarations with this name in the Swift module.
   SmallVector<ValueDecl *, 1> results;
   module->lookupValue(name, NLKind::UnqualifiedLookup, results);
   for (auto result : results) {
      if (auto nominal = dyn_cast<NominalTypeDecl>(result))
         return nominal;

      // Look through typealiases.
      if (auto typealias = dyn_cast<TypeAliasDecl>(result)) {
         return typealias->getDeclaredInterfaceType()->getAnyNominal();
      }
   }

   return nullptr;
}

bool ForeignRepresentationInfo::isRepresentableAsOptional() const {
   switch (getKind()) {
      case ForeignRepresentableKind::None:
         llvm_unreachable("this type is not representable");

      case ForeignRepresentableKind::Trivial:
         return Storage.getPointer() != 0;

      case ForeignRepresentableKind::Bridged: {
         auto KPK_ObjectiveCBridgeable = KnownInterfaceKind::ObjectiveCBridgeable;
         InterfaceDecl *proto = getConformance()->getInterface();
         assert(proto->isSpecificInterface(KPK_ObjectiveCBridgeable) &&
                "unknown protocol; does it support optional?");
         (void) proto;
         (void) KPK_ObjectiveCBridgeable;

         return true;
      }

      case ForeignRepresentableKind::BridgedError:
         return true;

      case ForeignRepresentableKind::Object:
      case ForeignRepresentableKind::StaticBridged:
         llvm_unreachable("unexpected kind in ForeignRepresentableCacheEntry");
   }

   llvm_unreachable("Unhandled ForeignRepresentableKind in switch.");
}

ForeignRepresentationInfo
AstContext::getForeignRepresentationInfo(NominalTypeDecl *nominal,
                                         ForeignLanguage language,
                                         const DeclContext *dc) {
   // Local function to add a type with the given name and module as
   // trivially-representable.
   auto addTrivial = [&](Identifier name, ModuleDecl *module,
                         bool allowOptional = false) {
      if (auto type = findUnderlyingTypeInModule(*this, name, module)) {
         auto info = ForeignRepresentationInfo::forTrivial();
         if (allowOptional)
            info = ForeignRepresentationInfo::forTrivialWithOptional();
         getImpl().ForeignRepresentableCache.insert({type, info});
      }
   };

   if (getImpl().ForeignRepresentableCache.empty()) {
      // Pre-populate the foreign-representable cache with known types.
      if (auto stdlib = getStdlibModule()) {
         addTrivial(getIdentifier("OpaquePointer"), stdlib, true);

         // Builtin types
         // FIXME: Layering violation to use the ClangImporter's define.
#define MAP_BUILTIN_TYPE(CLANG_BUILTIN_KIND, POLAR_TYPE_NAME) \
      addTrivial(getIdentifier(#POLAR_TYPE_NAME), stdlib);

#include "polarphp/clangimporter/BuiltinMappedTypesDef.h"

         // Even though we may never import types directly as Int or UInt
         // (e.g. on 64-bit Windows, where CLong maps to Int32 and
         // CLongLong to Int64), it's always possible to convert an Int
         // or UInt to a C type.
         addTrivial(getIdentifier("Int"), stdlib);
         addTrivial(getIdentifier("UInt"), stdlib);
      }

      if (auto darwin = getLoadedModule(Id_Darwin)) {
         // Note: DarwinBoolean is odd because it's bridged to Bool in APIs,
         // but can also be trivially bridged.
         addTrivial(getIdentifier("DarwinBoolean"), darwin);
      }

      if (auto winsdk = getLoadedModule(Id_WinSDK)) {
         // NOTE: WindowsBool is odd because it is bridged to Bool in APIs, but can
         // also be trivially bridged.
         addTrivial(getIdentifier("WindowsBool"), winsdk);
      }

//      if (auto objectiveC = getLoadedModule(Id_ObjectiveC)) {
//         addTrivial(Id_Selector, objectiveC, true);
//
//         // Note: ObjCBool is odd because it's bridged to Bool in APIs,
//         // but can also be trivially bridged.
//         addTrivial(getIdentifier("ObjCBool"), objectiveC);
//
//         addTrivial(getSwiftId(KnownFoundationEntity::NSZone), objectiveC, true);
//      }

      if (auto coreGraphics = getLoadedModule(getIdentifier("CoreGraphics"))) {
         addTrivial(Id_CGFloat, coreGraphics);
      }

      // Pull SIMD types of size 2...4 from the SIMD module, if it exists.
      // FIXME: Layering violation to use the ClangImporter's define.
      const unsigned POLAR_MAX_IMPORTED_SIMD_ELEMENTS = 4;
      if (auto simd = getLoadedModule(Id_simd)) {
#define MAP_SIMD_TYPE(BASENAME, _, __)                                  \
      {                                                                 \
        char name[] = #BASENAME "0";                                    \
        for (unsigned i = 2; i <= POLAR_MAX_IMPORTED_SIMD_ELEMENTS; ++i) { \
          *(std::end(name) - 2) = '0' + i;                              \
          addTrivial(getIdentifier(name), simd);                        \
        }                                                               \
      }

#include "polarphp/clangimporter/SIMDMappedTypesDef.h"
      }
   }

   // Determine whether we know anything about this nominal type
   // yet. If we've never seen this nominal type before, or if we have
   // an out-of-date negative cached value, we'll have to go looking.
   auto known = getImpl().ForeignRepresentableCache.find(nominal);
   bool wasNotFoundInCache = known == getImpl().ForeignRepresentableCache.end();

   // For the REPL. We might have initialized the cache above before CoreGraphics
   // was loaded.
   //   let s = "" // Here we initialize the ForeignRepresentableCache.
   //   import Foundation
   //   let pt = CGPoint(x: 1.0, y: 2.0) // Here we query for CGFloat.
   // Add CGFloat as trivial if we encounter it later.
   // If the type was not found check if it would be found after having recently
   // loaded the module.
   // Similar for types for other non stdlib modules.
   auto conditionallyAddTrivial = [&](NominalTypeDecl *nominalDecl,
                                      Identifier typeName, Identifier moduleName,
                                      bool allowOptional = false) {
      if (nominal->getName() == typeName && wasNotFoundInCache) {
         if (auto module = getLoadedModule(moduleName)) {
            addTrivial(typeName, module, allowOptional);
            known = getImpl().ForeignRepresentableCache.find(nominal);
            wasNotFoundInCache = known == getImpl().ForeignRepresentableCache.end();
         }
      }
   };
   conditionallyAddTrivial(nominal, getIdentifier("DarwinBoolean"), Id_Darwin);
   conditionallyAddTrivial(nominal, getIdentifier("WindowsBool"), Id_WinSDK);
   conditionallyAddTrivial(nominal, getIdentifier("ObjCBool"), Id_ObjectiveC);
   const unsigned POLAR_MAX_IMPORTED_SIMD_ELEMENTS = 4;
#define MAP_SIMD_TYPE(BASENAME, _, __)                                         \
  {                                                                            \
    char name[] = #BASENAME "0";                                               \
    for (unsigned i = 2; i <= POLAR_MAX_IMPORTED_SIMD_ELEMENTS; ++i) {         \
      *(std::end(name) - 2) = '0' + i;                                         \
      conditionallyAddTrivial(nominal, getIdentifier(name), Id_simd);          \
    }                                                                          \
  }

#include "polarphp/clangimporter/SIMDMappedTypesDef.h"

   if (wasNotFoundInCache ||
       (known->second.getKind() == ForeignRepresentableKind::None &&
        known->second.getGeneration() < CurrentGeneration)) {
      Optional<ForeignRepresentationInfo> result;

      // Look for a conformance to _ObjectiveCBridgeable (other than Optional's--
      // we don't want to allow exposing APIs with double-optional types like
      // NSObject??, even though Optional is bridged to its underlying type).
      //
      // FIXME: We're implicitly depending on the fact that lookupConformance
      // is global, ignoring the module we provide for it.
      if (nominal != dc->getAstContext().getOptionalDecl()) {
         if (auto objcBridgeable
            = getInterface(KnownInterfaceKind::ObjectiveCBridgeable)) {
            auto conformance = dc->getParentModule()->lookupConformance(
               nominal->getDeclaredType(), objcBridgeable);
            if (conformance) {
               result =
                  ForeignRepresentationInfo::forBridged(conformance.getConcrete());
            }
         }
      }


      // If we didn't find anything, mark the result as "None".
      if (!result)
         result = ForeignRepresentationInfo::forNone(CurrentGeneration);

      // Cache the result.
      known = getImpl().ForeignRepresentableCache.insert({nominal, *result}).first;
   }

   // Map a cache entry to a result for this specific
   auto entry = known->second;
   if (entry.getKind() == ForeignRepresentableKind::None)
      return entry;

   // Extract the protocol conformance.
   auto conformance = entry.getConformance();

   // If the conformance is not visible, fail.
   if (conformance && !conformance->isVisibleFrom(dc))
      return ForeignRepresentationInfo::forNone();

   // Language-specific filtering.
   switch (language) {
      case ForeignLanguage::C:
         // Ignore _ObjectiveCBridgeable conformances in C.
         if (conformance &&
             conformance->getInterface()->isSpecificInterface(
                KnownInterfaceKind::ObjectiveCBridgeable))
            return ForeignRepresentationInfo::forNone();

         // Ignore error bridging in C.
         if (entry.getKind() == ForeignRepresentableKind::BridgedError)
            return ForeignRepresentationInfo::forNone();

         LLVM_FALLTHROUGH;

      case ForeignLanguage::ObjectiveC:
         return entry;
   }

   llvm_unreachable("Unhandled ForeignLanguage in switch.");
}

//bool AstContext::isTypeBridgedInExternalModule(
//   NominalTypeDecl *nominal) const {
//   return (nominal == getBoolDecl() ||
//           nominal == getIntDecl() ||
//           nominal == getInt64Decl() ||
//           nominal == getInt32Decl() ||
//           nominal == getInt16Decl() ||
//           nominal == getInt8Decl() ||
//           nominal == getUIntDecl() ||
//           nominal == getUInt64Decl() ||
//           nominal == getUInt32Decl() ||
//           nominal == getUInt16Decl() ||
//           nominal == getUInt8Decl() ||
//           nominal == getFloatDecl() ||
//           nominal == getDoubleDecl() ||
//           nominal == getArrayDecl() ||
//           nominal == getCollectionDifferenceDecl() ||
//           (nominal->getDeclContext()->getAsDecl() ==
//            getCollectionDifferenceDecl() &&
//            nominal->getBaseName() == Id_Change) ||
//           nominal == getDictionaryDecl() ||
//           nominal == getSetDecl() ||
//           nominal == getStringDecl() ||
//           nominal == getSubstringDecl() ||
//           nominal == getErrorDecl() ||
//           nominal == getAnyHashableDecl() ||
//           // Foundation's overlay depends on the CoreGraphics overlay, but
//           // CoreGraphics value types bridge to Foundation objects such as
//           // NSValue and NSNumber, so to avoid circular dependencies, the
//           // bridging implementations of CG types appear in the Foundation
//           // module.
//           nominal->getParentModule()->getName() == Id_CoreGraphics ||
//           // CoreMedia is a dependency of AVFoundation, but the bridged
//           // NSValue implementations for CMTime, CMTimeRange, and
//           // CMTimeMapping are provided by AVFoundation, and AVFoundation
//           // gets upset if you don't use the NSValue subclasses its factory
//           // methods instantiate.
//           nominal->getParentModule()->getName() == Id_CoreMedia);
//}

const clang::Type *
AstContext::getClangFunctionType(ArrayRef<AnyFunctionType::Param> params,
                                 Type resultTy,
                                 FunctionType::ExtInfo incompleteExtInfo,
                                 FunctionTypeRepresentation trueRep) {
   auto &impl = getImpl();
   if (!impl.Converter) {
      auto *cml = getClangModuleLoader();
      impl.Converter.emplace(*this, cml->getClangAstContext(), LangOpts.Target);
   }
   return impl.Converter.getValue().getFunctionType(params, resultTy, trueRep);
}

CanGenericSignature AstContext::getSingleGenericParameterSignature() const {
   if (auto theSig = getImpl().SingleGenericParameterSignature)
      return theSig;

   auto param = GenericTypeParamType::get(0, 0, *this);
   auto sig = GenericSignature::get(param, {});
   auto canonicalSig = CanGenericSignature(sig);
   getImpl().SingleGenericParameterSignature = canonicalSig;
   return canonicalSig;
}

// Return the signature for an opened existential. The opened archetype may have
// a different set of conformances from the corresponding existential. The
// opened archetype conformances are dictated by the ABI for generic arguments,
// while the existential value conformances are dictated by their layout (see
// Type::getExistentialLayout()). In particular, the opened archetype signature
// does not have requirements for conformances inherited from superclass
// constraints while existential values do.
CanGenericSignature AstContext::getOpenedArchetypeSignature(CanType existential,
                                                            ModuleDecl *mod) {
   auto found = getImpl().ExistentialSignatures.find(existential);
   if (found != getImpl().ExistentialSignatures.end())
      return found->second;

   assert(existential.isExistentialType());

   auto genericParam = GenericTypeParamType::get(0, 0, *this);
   Requirement requirement(RequirementKind::Conformance, genericParam,
                           existential);
   auto genericSig = evaluateOrDefault(
      evaluator,
      AbstractGenericSignatureRequest{nullptr, {genericParam}, {requirement}},
      GenericSignature());

   CanGenericSignature canGenericSig(genericSig);

   auto result = getImpl().ExistentialSignatures.insert(
      std::make_pair(existential, canGenericSig));
   assert(result.second);
   (void) result;

   return canGenericSig;
}

GenericSignature
AstContext::getOverrideGenericSignature(const ValueDecl *base,
                                        const ValueDecl *derived) {
   auto baseGenericCtx = base->getAsGenericContext();
   auto derivedGenericCtx = derived->getAsGenericContext();
   auto &ctx = base->getAstContext();

   if (!baseGenericCtx || !derivedGenericCtx)
      return nullptr;

   auto baseClass = base->getDeclContext()->getSelfClassDecl();
   if (!baseClass)
      return nullptr;

   auto derivedClass = derived->getDeclContext()->getSelfClassDecl();
   if (!derivedClass)
      return nullptr;

   if (derivedClass->getSuperclass().isNull())
      return nullptr;

   if (baseGenericCtx->getGenericSignature().isNull() ||
       derivedGenericCtx->getGenericSignature().isNull())
      return nullptr;

   auto baseClassSig = baseClass->getGenericSignature();
   auto subMap = derivedClass->getSuperclass()->getContextSubstitutionMap(
      derivedClass->getModuleContext(), baseClass);

   unsigned derivedDepth = 0;

   auto key = OverrideSignatureKey(baseGenericCtx->getGenericSignature(),
                                   derivedClass->getGenericSignature(),
                                   derivedClass->getSuperclass());

   if (getImpl().overrideSigCache.find(key) !=
       getImpl().overrideSigCache.end()) {
      return getImpl().overrideSigCache.lookup(key);
   }

   if (auto derivedSig = derivedClass->getGenericSignature())
      derivedDepth = derivedSig->getGenericParams().back()->getDepth() + 1;

   SmallVector<GenericTypeParamType *, 2> addedGenericParams;
   if (auto *gpList = derivedGenericCtx->getGenericParams()) {
      for (auto gp : *gpList) {
         addedGenericParams.push_back(
            gp->getDeclaredInterfaceType()->castTo<GenericTypeParamType>());
      }
   }

   unsigned baseDepth = 0;

   if (baseClassSig) {
      baseDepth = baseClassSig->getGenericParams().back()->getDepth() + 1;
   }

   auto substFn = [&](SubstitutableType *type) -> Type {
      auto *gp = cast<GenericTypeParamType>(type);

      if (gp->getDepth() < baseDepth) {
         return Type(gp).subst(subMap);
      }

      return CanGenericTypeParamType::get(
         gp->getDepth() - baseDepth + derivedDepth, gp->getIndex(), ctx);
   };

   auto lookupConformanceFn =
      [&](CanType depTy, Type substTy,
          InterfaceDecl *proto) -> InterfaceConformanceRef {
         if (auto conf = subMap.lookupConformance(depTy, proto))
            return conf;

         return InterfaceConformanceRef(proto);
      };

   SmallVector<Requirement, 2> addedRequirements;
   for (auto reqt : baseGenericCtx->getGenericSignature()->getRequirements()) {
      if (auto substReqt = reqt.subst(substFn, lookupConformanceFn)) {
         addedRequirements.push_back(*substReqt);
      }
   }

   auto genericSig = evaluateOrDefault(
      ctx.evaluator,
      AbstractGenericSignatureRequest{
         derivedClass->getGenericSignature().getPointer(),
         std::move(addedGenericParams),
         std::move(addedRequirements)},
      GenericSignature());
   getImpl().overrideSigCache.insert(std::make_pair(key, genericSig));
   return genericSig;
}

bool AstContext::overrideGenericSignatureReqsSatisfied(
   const ValueDecl *base, const ValueDecl *derived,
   const OverrideGenericSignatureReqCheck direction) {
   auto sig = getOverrideGenericSignature(base, derived);
   if (!sig)
      return true;

   auto derivedSig = derived->getAsGenericContext()->getGenericSignature();

   switch (direction) {
      case OverrideGenericSignatureReqCheck::BaseReqSatisfiedByDerived:
         return sig->requirementsNotSatisfiedBy(derivedSig).empty();
      case OverrideGenericSignatureReqCheck::DerivedReqSatisfiedByBase:
         return derivedSig->requirementsNotSatisfiedBy(sig).empty();
   }
   llvm_unreachable("Unhandled OverrideGenericSignatureReqCheck in switch");
}

PILLayout *PILLayout::get(AstContext &C,
                          CanGenericSignature Generics,
                          ArrayRef<PILField> Fields) {
   // Profile the layout parameters.
   llvm::FoldingSetNodeID id;
   Profile(id, Generics, Fields);

   // Return an existing layout if there is one.
   void *insertPos;
   auto &Layouts = C.getImpl().PILLayouts;

   if (auto existing = Layouts.FindNodeOrInsertPos(id, insertPos))
      return existing;

   // Allocate a new layout.
   void *memory = C.Allocate(totalSizeToAlloc<PILField>(Fields.size()),
                             alignof(PILLayout));

   auto newLayout = ::new(memory) PILLayout(Generics, Fields);
   Layouts.InsertNode(newLayout, insertPos);
   return newLayout;
}

CanPILBoxType PILBoxType::get(AstContext &C,
                              PILLayout *Layout,
                              SubstitutionMap Substitutions) {
   // Canonicalize substitutions.
   Substitutions = Substitutions.getCanonical();

   // Return an existing layout if there is one.
   void *insertPos;
   auto &PILBoxTypes = C.getImpl().PILBoxTypes;
   llvm::FoldingSetNodeID id;
   Profile(id, Layout, Substitutions);
   if (auto existing = PILBoxTypes.FindNodeOrInsertPos(id, insertPos))
      return CanPILBoxType(existing);

   auto newBox = new(C, AllocationArena::Permanent) PILBoxType(C, Layout,
                                                               Substitutions);
   PILBoxTypes.InsertNode(newBox, insertPos);
   return CanPILBoxType(newBox);
}

/// TODO: Transitional factory to present the single-type PILBoxType::get
/// interface.
CanPILBoxType PILBoxType::get(CanType boxedType) {
   auto &ctx = boxedType->getAstContext();
   auto singleGenericParamSignature = ctx.getSingleGenericParameterSignature();
   auto genericParam = singleGenericParamSignature->getGenericParams()[0];
   auto layout = PILLayout::get(ctx, singleGenericParamSignature,
                                PILField(CanType(genericParam),
                                   /*mutable*/ true));

   auto subMap =
      SubstitutionMap::get(
         singleGenericParamSignature,
         [&](SubstitutableType *type) -> Type {
            if (type->isEqual(genericParam)) return boxedType;

            return nullptr;
         },
         MakeAbstractConformanceForGenericType());
   return get(boxedType->getAstContext(), layout, subMap);
}

LayoutConstraint
LayoutConstraint::getLayoutConstraint(LayoutConstraintKind Kind,
                                      AstContext &C) {
   return getLayoutConstraint(Kind, 0, 0, C);
}

LayoutConstraint LayoutConstraint::getLayoutConstraint(LayoutConstraintKind Kind,
                                                       unsigned SizeInBits,
                                                       unsigned Alignment,
                                                       AstContext &C) {
   if (!LayoutConstraintInfo::isKnownSizeTrivial(Kind)) {
      assert(SizeInBits == 0);
      assert(Alignment == 0);
      return getLayoutConstraint(Kind);
   }

   // Check to see if we've already seen this tuple before.
   llvm::FoldingSetNodeID ID;
   LayoutConstraintInfo::Profile(ID, Kind, SizeInBits, Alignment);

   void *InsertPos = nullptr;
   if (LayoutConstraintInfo *Layout =
      C.getImpl().getArena(AllocationArena::Permanent)
         .LayoutConstraints.FindNodeOrInsertPos(ID, InsertPos))
      return LayoutConstraint(Layout);

   LayoutConstraintInfo *New =
      LayoutConstraintInfo::isTrivial(Kind)
      ? new(C, AllocationArena::Permanent)
         LayoutConstraintInfo(Kind, SizeInBits, Alignment)
      : new(C, AllocationArena::Permanent) LayoutConstraintInfo(Kind);
   C.getImpl().getArena(AllocationArena::Permanent)
      .LayoutConstraints.InsertNode(New, InsertPos);
   return LayoutConstraint(New);
}

Type &AstContext::getDefaultTypeRequestCache(SourceFile *SF,
                                             KnownInterfaceKind kind) {
   return getImpl().DefaultTypeRequestCaches[SF][size_t(kind)];
}

Type AstContext::getSideCachedPropertyWrapperBackingPropertyType(
   VarDecl *var) const {
   return getImpl().PropertyWrapperBackingVarTypes[var];
}

void AstContext::setSideCachedPropertyWrapperBackingPropertyType(
   VarDecl *var, Type type) {
   assert(!getImpl().PropertyWrapperBackingVarTypes[var] ||
          getImpl().PropertyWrapperBackingVarTypes[var]->isEqual(type));
   getImpl().PropertyWrapperBackingVarTypes[var] = type;
}

VarDecl *VarDecl::getOriginalWrappedProperty(
   Optional<PropertyWrapperSynthesizedPropertyKind> kind) const {
   if (!Bits.VarDecl.IsPropertyWrapperBackingProperty)
      return nullptr;

   AstContext &ctx = getAstContext();
   assert(ctx.getImpl().OriginalWrappedProperties.count(this) > 0);
   auto original = ctx.getImpl().OriginalWrappedProperties[this];
   if (!kind)
      return original;

   auto wrapperInfo = original->getPropertyWrapperBackingPropertyInfo();
   switch (*kind) {
      case PropertyWrapperSynthesizedPropertyKind::Backing:
         return this == wrapperInfo.backingVar ? original : nullptr;

      case PropertyWrapperSynthesizedPropertyKind::StorageWrapper:
         return this == wrapperInfo.storageWrapperVar ? original : nullptr;
   }
   llvm_unreachable("covered switch");
}

void VarDecl::setOriginalWrappedProperty(VarDecl *originalProperty) {
   Bits.VarDecl.IsPropertyWrapperBackingProperty = true;
   AstContext &ctx = getAstContext();
   assert(ctx.getImpl().OriginalWrappedProperties.count(this) == 0);
   ctx.getImpl().OriginalWrappedProperties[this] = originalProperty;
}

IndexSubset *
IndexSubset::get(AstContext &ctx, const SmallBitVector &indices) {
   auto &foldingSet = ctx.getImpl().IndexSubsets;
   llvm::FoldingSetNodeID id;
   unsigned capacity = indices.size();
   id.AddInteger(capacity);
   for (unsigned index : indices.set_bits())
      id.AddInteger(index);
   void *insertPos = nullptr;
   auto *existing = foldingSet.FindNodeOrInsertPos(id, insertPos);
   if (existing)
      return existing;
   auto sizeToAlloc = sizeof(IndexSubset) +
                      getNumBytesNeededForCapacity(capacity);
   auto *buf = reinterpret_cast<IndexSubset *>(
      ctx.Allocate(sizeToAlloc, alignof(IndexSubset)));
   auto *newNode = new(buf) IndexSubset(indices);
   foldingSet.InsertNode(newNode, insertPos);
   return newNode;
}

} // polar