//===--- PILFunction.h - Defines the PILFunction class ----------*- C++ -*-===//
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
// This file defines the PILFunction class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILFUNCTION_H
#define POLARPHP_PIL_PILFUNCTION_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/AstNode.h"
#include "polarphp/ast/Availability.h"
#include "polarphp/ast/ResilienceExpansion.h"
#include "polarphp/basic/ProfileCounter.h"
#include "polarphp/pil/lang/PILFunctionConventions.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILLinkage.h"
#include "polarphp/pil/lang/PILPrintContext.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/ADT/ilist.h"
#include "llvm/ADT/ilist_node.h"

/// The symbol name used for the program entry point function.
#define POLAR_ENTRY_POINT_FUNCTION "main"

/// @todo cycle dependency

namespace polar {

class AstContext;
class GenericSignature;
class GenericEnvironment;
class GenericSpecializationInformation;
class Identifier;
class AvailabilityContext;
class PILInstruction;
class PILModule;
class PILFunctionBuilder;
class PILProfiler;
class PILArgument;
class AstNode;
enum class ResilienceExpansion : unsigned;
class NumOptimizationModeBits;
class NumEffectsKindBits;
enum class OptimizationMode : uint8_t;

namespace lowering {
class TypeLowering;
class AbstractionPattern;
}

enum IsBare_t { IsNotBare, IsBare };
enum IsTransparent_t { IsNotTransparent, IsTransparent };
enum Inline_t { InlineDefault, NoInline, AlwaysInline };
enum IsThunk_t {
   IsNotThunk,
   IsThunk,
   IsReabstractionThunk,
   IsSignatureOptimizedThunk
};

enum IsDynamicallyReplaceable_t {
   IsNotDynamic,
   IsDynamic
};

enum IsExactSelfClass_t {
   IsNotExactSelfClass,
   IsExactSelfClass,
};

class PILSpecializeAttr final {
   friend PILFunction;
public:
   enum class SpecializationKind {
      Full,
      Partial
   };

   static PILSpecializeAttr *create(PILModule &M,
                                    GenericSignature specializedSignature,
                                    bool exported, SpecializationKind kind);

   bool isExported() const {
      return exported;
   }

   bool isFullSpecialization() const {
      return kind == SpecializationKind::Full;
   }

   bool isPartialSpecialization() const {
      return kind == SpecializationKind::Partial;
   }

   SpecializationKind getSpecializationKind() const {
      return kind;
   }

   GenericSignature getSpecializedSignature() const {
      return specializedSignature;
   }

   PILFunction *getFunction() const {
      return F;
   }

   void print(llvm::raw_ostream &OS) const;

private:
   SpecializationKind kind;
   bool exported;
   GenericSignature specializedSignature;
   PILFunction *F = nullptr;

   PILSpecializeAttr(bool exported, SpecializationKind kind,
                     GenericSignature specializedSignature);
};

/// PILFunction - A function body that has been lowered to PIL. This consists of
/// zero or more PIL PILBasicBlock objects that contain the PILInstruction
/// objects making up the function.
class PILFunction
   : public llvm::ilist_node<PILFunction>, public PILAllocated<PILFunction> {
public:
   using BlockListType = llvm::iplist<PILBasicBlock>;

private:
   friend class PILBasicBlock;
   friend class PILModule;
   friend class PILFunctionBuilder;

   /// Module - The PIL module that the function belongs to.
   PILModule &Module;

   /// The mangled name of the PIL function, which will be propagated
   /// to the binary.  A pointer into the module's lookup table.
   StringRef Name;

   /// The lowered type of the function.
   CanPILFunctionType LoweredType;

   /// The context archetypes of the function.
   GenericEnvironment *GenericEnv;

   /// The information about specialization.
   /// Only set if this function is a specialization of another function.
   const GenericSpecializationInformation *SpecializationInfo;

   /// The forwarding substitution map, lazily computed.
   SubstitutionMap ForwardingSubMap;

   /// The collection of all BasicBlocks in the PILFunction. Empty for external
   /// function references.
   BlockListType BlockList;

   /// The owning declaration of this function's clang node, if applicable.
   ValueDecl *ClangNodeOwner = nullptr;

   /// The source location and scope of the function.
   const PILDebugScope *DebugScope;

   /// The AST decl context of the function.
   DeclContext *DeclCtxt;

   /// The profiler for instrumentation based profiling, or null if profiling is
   /// disabled.
   PILProfiler *Profiler = nullptr;

   /// The function this function is meant to replace. Null if this is not a
   /// @_dynamicReplacement(for:) function.
   PILFunction *ReplacedFunction = nullptr;

   Identifier ObjCReplacementFor;

   /// The function's set of semantics attributes.
   ///
   /// TODO: Why is this using a std::string? Why don't we use uniqued
   /// StringRefs?
   std::vector<std::string> SemanticsAttrSet;

   /// The function's remaining set of specialize attributes.
   std::vector<PILSpecializeAttr*> SpecializeAttrSet;

   /// Has value if there's a profile for this function
   /// Contains Function Entry Count
   ProfileCounter EntryCount;

   /// The availability used to determine if declarations of this function
   /// should use weak linking.
   AvailabilityContext Availability;

   /// This is the number of uses of this PILFunction inside the PIL.
   /// It does not include references from debug scopes.
   unsigned RefCount = 0;

   /// The function's bare attribute. Bare means that the function is PIL-only
   /// and does not require debug info.
   unsigned Bare : 1;

   /// The function's transparent attribute.
   unsigned Transparent : 1;

   /// The function's serialized attribute.
   unsigned Serialized : 2;

   /// Specifies if this function is a thunk or a reabstraction thunk.
   ///
   /// The inliner uses this information to avoid inlining (non-trivial)
   /// functions into the thunk.
   unsigned Thunk : 2;

   /// The scope in which the parent class can be subclassed, if this is a method
   /// which is contained in the vtable of that class.
   unsigned ClassSubclassScope : 2;

   /// The function's global_init attribute.
   unsigned GlobalInitFlag : 1;

   /// The function's noinline attribute.
   unsigned InlineStrategy : 2;

   /// The linkage of the function.
   unsigned Linkage : NumPILLinkageBits;

   /// Set if the function may be referenced from C code and should thus be
   /// preserved and exported more widely than its Swift linkage and usage
   /// would indicate.
   unsigned HasCReferences : 1;

   /// Whether cross-module references to this function should always use
   /// weak linking.
   unsigned IsWeakImported : 1;

   /// Whether the implementation can be dynamically replaced.
   unsigned IsDynamicReplaceable : 1;

   /// If true, this indicates that a class method implementation will always be
   /// invoked with a `self` argument of the exact base class type.
   unsigned ExactSelfClass : 1;

   /// True if this function is inlined at least once. This means that the
   /// debug info keeps a pointer to this function.
   unsigned Inlined : 1;

   /// True if this function is a zombie function. This means that the function
   /// is dead and not referenced from anywhere inside the PIL. But it is kept
   /// for other purposes:
   /// *) It is inlined and the debug info keeps a reference to the function.
   /// *) It is a dead method of a class which has higher visibility than the
   ///    method itself. In this case we need to create a vtable stub for it.
   /// *) It is a function referenced by the specialization information.
   unsigned Zombie : 1;

   /// True if this function is in Ownership SSA form and thus must pass
   /// ownership verification.
   ///
   /// This enables the verifier to easily prove that before the Ownership Model
   /// Eliminator runs on a function, we only see a non-semantic-arc world and
   /// after the pass runs, we only see a semantic-arc world.
   unsigned HasOwnership : 1;

   /// Set if the function body was deserialized from canonical PIL. This implies
   /// that the function's home module performed PIL diagnostics prior to
   /// serialization.
   unsigned WasDeserializedCanonical : 1;

   /// True if this is a reabstraction thunk of escaping function type whose
   /// single argument is a potentially non-escaping closure. This is an escape
   /// hatch to allow non-escaping functions to be stored or passed as an
   /// argument with escaping function type. The thunk argument's function type
   /// is not necessarily @noescape. The only relevant aspect of the argument is
   /// that it may have unboxed capture (i.e. @inout_aliasable parameters).
   unsigned IsWithoutActuallyEscapingThunk : 1;

   /// If != OptimizationMode::NotSet, the optimization mode specified with an
   /// function attribute.
   unsigned OptMode : NumOptimizationModeBits;

   /// The function's effects attribute.
   unsigned EffectsKindAttr : NumEffectsKindBits;

   static void
   validateSubclassScope(SubclassScope scope, IsThunk_t isThunk,
                         const GenericSpecializationInformation *genericInfo) {
#ifndef NDEBUG
      // The _original_ function for a method can turn into a thunk through
      // signature optimization, meaning it needs to retain its subclassScope, but
      // other thunks and specializations are implementation details and so
      // shouldn't be connected to their parent class.
      bool thunkCanHaveSubclassScope;
      switch (isThunk) {
         case IsNotThunk:
         case IsSignatureOptimizedThunk:
            thunkCanHaveSubclassScope = true;
            break;
         case IsThunk:
         case IsReabstractionThunk:
            thunkCanHaveSubclassScope = false;
            break;
      }
      auto allowsInterestingScopes = thunkCanHaveSubclassScope && !genericInfo;
      assert(
         (allowsInterestingScopes ||
                  scope == SubclassScope::NotApplicable) &&
         "SubclassScope on specialization or non-signature-optimized thunk");
#endif
   }

   PILFunction(PILModule &module, PILLinkage linkage, StringRef mangledName,
               CanPILFunctionType loweredType, GenericEnvironment *genericEnv,
               Optional<PILLocation> loc, IsBare_t isBarePILFunction,
               IsTransparent_t isTrans, IsSerialized_t isSerialized,
               ProfileCounter entryCount, IsThunk_t isThunk,
               SubclassScope classSubclassScope, Inline_t inlineStrategy,
               EffectsKind E, PILFunction *insertBefore,
               const PILDebugScope *debugScope,
               IsDynamicallyReplaceable_t isDynamic,
               IsExactSelfClass_t isExactSelfClass);

   static PILFunction *
   create(PILModule &M, PILLinkage linkage, StringRef name,
          CanPILFunctionType loweredType, GenericEnvironment *genericEnv,
          Optional<PILLocation> loc, IsBare_t isBarePILFunction,
          IsTransparent_t isTrans, IsSerialized_t isSerialized,
          ProfileCounter entryCount, IsDynamicallyReplaceable_t isDynamic,
          IsExactSelfClass_t isExactSelfClass,
          IsThunk_t isThunk = IsNotThunk,
          SubclassScope classSubclassScope = SubclassScope::NotApplicable,
          Inline_t inlineStrategy = InlineDefault,
          EffectsKind EffectsKindAttr = EffectsKind::Unspecified,
          PILFunction *InsertBefore = nullptr,
          const PILDebugScope *DebugScope = nullptr);

   /// Set has ownership to the given value. True means that the function has
   /// ownership, false means it does not.
   ///
   /// Only for use by FunctionBuilders!
   void setHasOwnership(bool newValue) { HasOwnership = newValue; }

public:
   ~PILFunction();

   PILModule &getModule() const { return Module; }

   PILType getLoweredType() const {
      return PILType::getPrimitiveObjectType(LoweredType);
   }
   CanPILFunctionType getLoweredFunctionType() const {
      return LoweredType;
   }
   CanPILFunctionType
   getLoweredFunctionTypeInContext(TypeExpansionContext context) const;

   PILType getLoweredTypeInContext(TypeExpansionContext context) const {
      return PILType::getPrimitiveObjectType(
         getLoweredFunctionTypeInContext(context));
   }

   PILFunctionConventions getConventions() const {
      return PILFunctionConventions(LoweredType, getModule());
   }

   PILProfiler *getProfiler() const { return Profiler; }

   PILFunction *getDynamicallyReplacedFunction() const {
      return ReplacedFunction;
   }
   void setDynamicallyReplacedFunction(PILFunction *f) {
      assert(ReplacedFunction == nullptr && "already set");
      assert(!hasObjCReplacement());

      if (f == nullptr)
         return;
      ReplacedFunction = f;
      ReplacedFunction->incrementRefCount();
   }

   /// This function should only be called when PILFunctions are bulk deleted.
   void dropDynamicallyReplacedFunction() {
      if (!ReplacedFunction)
         return;
      ReplacedFunction->decrementRefCount();
      ReplacedFunction = nullptr;
   }

   bool hasObjCReplacement() const {
      return !ObjCReplacementFor.empty();
   }

   Identifier getObjCReplacement() const {
      return ObjCReplacementFor;
   }

   void setObjCReplacement(AbstractFunctionDecl *replacedDecl);
   void setObjCReplacement(Identifier replacedDecl);

   void setProfiler(PILProfiler *InheritedProfiler) {
      assert(!Profiler && "Function already has a profiler");
      Profiler = InheritedProfiler;
   }

   void createProfiler(AstNode Root, PILDeclRef forDecl,
                       ForDefinition_t forDefinition);

   void discardProfiler() { Profiler = nullptr; }

   ProfileCounter getEntryCount() const { return EntryCount; }

   void setEntryCount(ProfileCounter Count) { EntryCount = Count; }

   bool isNoReturnFunction() const;

   /// Unsafely rewrite the lowered type of this function.
   ///
   /// This routine does not touch the entry block arguments
   /// or return instructions; you need to do that yourself
   /// if you care.
   ///
   /// This routine does not update all the references in the module
   /// You have to do that yourself
   void rewriteLoweredTypeUnsafe(CanPILFunctionType newType) {
      LoweredType = newType;
   }

   /// Return the number of entities referring to this function (other
   /// than the PILModule).
   unsigned getRefCount() const { return RefCount; }

   /// Increment the reference count.
   void incrementRefCount() {
      RefCount++;
      assert(RefCount != 0 && "Overflow of reference count!");
   }

   /// Decrement the reference count.
   void decrementRefCount() {
      assert(RefCount != 0 && "Expected non-zero reference count on decrement!");
      RefCount--;
   }

   /// Drops all uses belonging to instructions in this function. The only valid
   /// operation performable on this object after this is called is called the
   /// destructor or deallocation.
   void dropAllReferences() {
      // @todo
//      for (PILBasicBlock &BB : *this)
//         BB.dropAllReferences();
   }

   /// Notify that this function was inlined. This implies that it is still
   /// needed for debug info generation, even if it is removed afterwards.
   void setInlined() {
      assert(!isZombie() && "Can't inline a zombie function");
      Inlined = true;
   }

   /// Returns true if this function was inlined.
   bool isInlined() const { return Inlined; }

   /// Mark this function as removed from the module's function list, but kept
   /// as "zombie" for debug info or vtable stub generation.
   void setZombie() {
      assert(!isZombie() && "Function is a zombie function already");
      Zombie = true;
   }

   /// Returns true if this function is dead, but kept in the module's zombie list.
   bool isZombie() const { return Zombie; }

   /// Returns true if this function has qualified ownership instructions in it.
   bool hasOwnership() const { return HasOwnership; }

   /// Sets the HasOwnership flag to false. This signals to PIL that no
   /// ownership instructions should be in this function any more.
   void setOwnershipEliminated() { setHasOwnership(false); }

   /// Returns true if this function was deserialized from canonical
   /// PIL. (.swiftmodule files contain canonical PIL; .sib files may be 'raw'
   /// PIL). If so, diagnostics should not be reapplied.
   bool wasDeserializedCanonical() const { return WasDeserializedCanonical; }

   void setWasDeserializedCanonical(bool val = true) {
      WasDeserializedCanonical = val;
   }

   /// Returns true if this is a reabstraction thunk of escaping function type
   /// whose single argument is a potentially non-escaping closure. i.e. the
   /// thunks' function argument may itself have @inout_aliasable parameters.
   bool isWithoutActuallyEscapingThunk() const {
      return IsWithoutActuallyEscapingThunk;
   }

   void setWithoutActuallyEscapingThunk(bool val = true) {
      assert(!val || isThunk() == IsReabstractionThunk);
      IsWithoutActuallyEscapingThunk = val;
   }

   /// Returns the calling convention used by this entry point.
   PILFunctionTypeRepresentation getRepresentation() const {
      return getLoweredFunctionType()->getRepresentation();
   }

   ResilienceExpansion getResilienceExpansion() const {
      return (isSerialized()
              ? ResilienceExpansion::Minimal
              : ResilienceExpansion::Maximal);
   }

   // Returns the type expansion context to be used inside this function.
   TypeExpansionContext getTypeExpansionContext() const {
      return TypeExpansionContext(*this);
   }

   const lowering::TypeLowering &
   getTypeLowering(lowering::AbstractionPattern orig, Type subst);

   const lowering::TypeLowering &getTypeLowering(Type t) const;

   PILType getLoweredType(lowering::AbstractionPattern orig, Type subst) const;

   PILType getLoweredType(Type t) const;

   PILType getLoweredLoadableType(Type t) const;

   PILType getLoweredType(PILType t) const;

   const lowering::TypeLowering &getTypeLowering(PILType type) const;

   bool isTypeABIAccessible(PILType type) const;

   /// Returns true if this function has a calling convention that has a self
   /// argument.
   bool hasSelfParam() const {
      return getLoweredFunctionType()->hasSelfParam();
   }

   /// Returns true if the function has parameters that are consumed by the
   // callee.
   bool hasOwnedParameters() const {
      for (auto &ParamInfo : getLoweredFunctionType()->getParameters()) {
         if (ParamInfo.isConsumed())
            return true;
      }
      return false;
   }

   // Returns true if the function has indirect out parameters.
   bool hasIndirectFormalResults() const {
      return getLoweredFunctionType()->hasIndirectFormalResults();
   }

   /// Returns true if this function either has a self metadata argument or
   /// object that Self metadata may be derived from.
   ///
   /// Note that this is not the same as hasSelfParam().
   ///
   /// For closures that capture DynamicSelfType, hasSelfMetadataParam()
   /// is true and hasSelfParam() is false. For methods on value types,
   /// hasSelfParam() is true and hasSelfMetadataParam() is false.
   bool hasSelfMetadataParam() const;

   /// Return the mangled name of this PILFunction.
   StringRef getName() const { return Name; }

   /// A convenience function which checks if the function has a specific
   /// \p name. It is equivalent to getName() == Name, but as it is not
   /// inlined it can be called from the debugger.
   bool hasName(const char *Name) const;

   /// True if this is a declaration of a function defined in another module.
   bool isExternalDeclaration() const { return BlockList.empty(); }

   /// Returns true if this is a definition of a function defined in this module.
   bool isDefinition() const { return !isExternalDeclaration(); }

   /// Get this function's linkage attribute.
   PILLinkage getLinkage() const { return PILLinkage(Linkage); }

   /// Set the function's linkage attribute.
   void setLinkage(PILLinkage linkage) { Linkage = unsigned(linkage); }

   /// Returns true if this function can be inlined into a fragile function
   /// body.
   bool hasValidLinkageForFragileInline() const {
      return (isSerialized() == IsSerialized ||
              isSerialized() == IsSerializable);
   }

   /// Returns true if this function can be referenced from a fragile function
   /// body.
   bool hasValidLinkageForFragileRef() const;

   /// Get's the effective linkage which is used to derive the llvm linkage.
   /// Usually this is the same as getLinkage(), except in one case: if this
   /// function is a method in a class which has higher visibility than the
   /// method itself, the function can be referenced from vtables of derived
   /// classes in other compilation units.
   PILLinkage getEffectiveSymbolLinkage() const {
      return effectiveLinkageForClassMember(getLinkage(),
                                            getClassSubclassScope());
   }

   /// Helper method which returns true if this function has "external" linkage.
   bool isAvailableExternally() const {
      return is_available_externally(getLinkage());
   }

   /// Helper method which returns true if the linkage of the PILFunction
   /// indicates that the object's definition might be required outside the
   /// current PILModule.
   bool isPossiblyUsedExternally() const;

   /// In addition to isPossiblyUsedExternally() it returns also true if this
   /// is a (private or internal) vtable method which can be referenced by
   /// vtables of derived classes outside the compilation unit.
   bool isExternallyUsedSymbol() const;

   /// Return whether this function may be referenced by C code.
   bool hasCReferences() const { return HasCReferences; }
   void setHasCReferences(bool value) { HasCReferences = value; }

   /// Returns the availability context used to determine if the function's
   /// symbol should be weakly referenced across module boundaries.
   AvailabilityContext getAvailabilityForLinkage() const {
      return Availability;
   }

   void setAvailabilityForLinkage(AvailabilityContext availability) {
      Availability = availability;
   }

   /// Returns whether this function's symbol must always be weakly referenced
   /// across module boundaries.
   bool isAlwaysWeakImported() const { return IsWeakImported; }

   void setAlwaysWeakImported(bool value) {
      IsWeakImported = value;
   }

   bool isWeakImported() const;

   /// Returns whether this function implementation can be dynamically replaced.
   IsDynamicallyReplaceable_t isDynamicallyReplaceable() const {
      return IsDynamicallyReplaceable_t(IsDynamicReplaceable);
   }
   void setIsDynamic(IsDynamicallyReplaceable_t value = IsDynamic) {
      IsDynamicReplaceable = value;
      assert(!Transparent || !IsDynamicReplaceable);
   }

   IsExactSelfClass_t isExactSelfClass() const {
      return IsExactSelfClass_t(ExactSelfClass);
   }
   void setIsExactSelfClass(IsExactSelfClass_t t) {
      ExactSelfClass = t;
   }

   /// Get the DeclContext of this function.
   DeclContext *getDeclContext() const { return DeclCtxt; }

   /// \returns True if the function is marked with the @_semantics attribute
   /// and has special semantics that the optimizer can use to optimize the
   /// function.
   bool hasSemanticsAttrs() const { return !SemanticsAttrSet.empty(); }

   /// \returns True if the function has a semantic attribute that starts with a
   /// specific string.
   ///
   /// TODO: This needs a better name.
   bool hasSemanticsAttrThatStartsWith(StringRef S) {
      return count_if(getSemanticsAttrs(), [&S](const std::string &Attr) -> bool {
         return StringRef(Attr).startswith(S);
      });
   }

   /// \returns the semantics tag that describes this function.
   ArrayRef<std::string> getSemanticsAttrs() const { return SemanticsAttrSet; }

   /// \returns True if the function has the semantics flag \p Value;
   bool hasSemanticsAttr(StringRef Value) const {
      return count(SemanticsAttrSet, Value);
   }

   /// Add the given semantics attribute to the attr list set.
   void addSemanticsAttr(StringRef Ref) {
      if (hasSemanticsAttr(Ref))
         return;
      SemanticsAttrSet.push_back(Ref);
      std::sort(SemanticsAttrSet.begin(), SemanticsAttrSet.end());
   }

   /// Remove the semantics
   void removeSemanticsAttr(StringRef Ref) {
      auto Iter =
         std::remove(SemanticsAttrSet.begin(), SemanticsAttrSet.end(), Ref);
      SemanticsAttrSet.erase(Iter);
   }

   /// \returns the range of specialize attributes.
   ArrayRef<PILSpecializeAttr*> getSpecializeAttrs() const {
      return SpecializeAttrSet;
   }

   /// Removes all specialize attributes from this function.
   void clearSpecializeAttrs() { SpecializeAttrSet.clear(); }

   void addSpecializeAttr(PILSpecializeAttr *Attr);


   /// Get this function's optimization mode or OptimizationMode::NotSet if it is
   /// not set for this specific function.
   OptimizationMode getOptimizationMode() const {
      return OptimizationMode(OptMode);
   }

   /// Returns the optimization mode for the function. If no mode is set for the
   /// function, returns the global mode, i.e. the mode of the module's options.
   OptimizationMode getEffectiveOptimizationMode() const;

   void setOptimizationMode(OptimizationMode mode) {
      OptMode = unsigned(mode);
   }

   /// \returns True if the function is optimizable (i.e. not marked as no-opt),
   ///          or is raw PIL (so that the mandatory passes still run).
   bool shouldOptimize() const;

   /// Returns true if this function should be optimized for size.
   bool optimizeForSize() const {
      return getEffectiveOptimizationMode() == OptimizationMode::ForSize;
   }

   /// Returns true if this is a function that should have its ownership
   /// verified.
   bool shouldVerifyOwnership() const;

   /// Check if the function has a location.
   /// FIXME: All functions should have locations, so this method should not be
   /// necessary.
   bool hasLocation() const {
      return DebugScope && !DebugScope->Loc.isNull();
   }

   /// Get the source location of the function.
   PILLocation getLocation() const {
      assert(DebugScope && "no scope/location");
      return getDebugScope()->Loc;
   }

   /// Initialize the debug scope of the function and also set the DeclCtxt.
   void setDebugScope(const PILDebugScope *DS) {
      DebugScope = DS;
      DeclCtxt = (DS ? DebugScope->Loc.getAsDeclContext() : nullptr);
   }

   /// Initialize the debug scope for debug info on PIL level (-gsil).
   void setPILDebugScope(const PILDebugScope *DS) {
      DebugScope = DS;
   }

   /// Get the source location of the function.
   const PILDebugScope *getDebugScope() const { return DebugScope; }

   /// Get this function's bare attribute.
   IsBare_t isBare() const { return IsBare_t(Bare); }
   void setBare(IsBare_t isB) { Bare = isB; }

   /// Get this function's transparent attribute.
   IsTransparent_t isTransparent() const { return IsTransparent_t(Transparent); }
   void setTransparent(IsTransparent_t isT) {
      Transparent = isT;
      assert(!Transparent || !IsDynamicReplaceable);
   }

   /// Get this function's serialized attribute.
   IsSerialized_t isSerialized() const { return IsSerialized_t(Serialized); }
   void setSerialized(IsSerialized_t isSerialized) { Serialized = isSerialized; }

   /// Get this function's thunk attribute.
   IsThunk_t isThunk() const { return IsThunk_t(Thunk); }
   void setThunk(IsThunk_t isThunk) {
      validateSubclassScope(getClassSubclassScope(), isThunk, SpecializationInfo);
      Thunk = isThunk;
   }

   /// Get the class visibility (relevant for class methods).
   SubclassScope getClassSubclassScope() const {
      return SubclassScope(ClassSubclassScope);
   }
   void setClassSubclassScope(SubclassScope scope) {
      validateSubclassScope(scope, isThunk(), SpecializationInfo);
      ClassSubclassScope = static_cast<unsigned>(scope);
   }

   /// Get this function's noinline attribute.
   Inline_t getInlineStrategy() const { return Inline_t(InlineStrategy); }
   void setInlineStrategy(Inline_t inStr) { InlineStrategy = inStr; }

   /// \return the function side effects information.
   EffectsKind getEffectsKind() const { return EffectsKind(EffectsKindAttr); }

   /// \return True if the function is annotated with the @_effects attribute.
   bool hasEffectsKind() const {
      return EffectsKind(EffectsKindAttr) != EffectsKind::Unspecified;
   }

   /// Set the function side effect information.
   void setEffectsKind(EffectsKind E) {
      EffectsKindAttr = unsigned(E);
   }

   /// Get this function's global_init attribute.
   ///
   /// The implied semantics are:
   /// - side-effects can occur any time before the first invocation.
   /// - all calls to the same global_init function have the same side-effects.
   /// - any operation that may observe the initializer's side-effects must be
   ///   preceded by a call to the initializer.
   ///
   /// This is currently true if the function is an addressor that was lazily
   /// generated from a global variable access. Note that the initialization
   /// function itself does not need this attribute. It is private and only
   /// called within the addressor.
   bool isGlobalInit() const { return GlobalInitFlag; }
   void setGlobalInit(bool isGI) { GlobalInitFlag = isGI; }

   /// Return whether this function has a foreign implementation which can
   /// be emitted on demand.
   bool hasForeignBody() const;

   /// Return whether this function corresponds to a Clang node.
   bool hasClangNode() const {
      return ClangNodeOwner != nullptr;
   }

   /// Set the owning declaration of the Clang node associated with this
   /// function.  We have to store an owner (a Swift declaration) instead of
   /// directly referencing the original declaration due to current
   /// limitations in the serializer.
   void setClangNodeOwner(ValueDecl *owner) {
      assert(owner->hasClangNode());
      ClangNodeOwner = owner;
   }

   /// Return the owning declaration of the Clang node associated with this
   /// function.  This should only be used for serialization.
   ValueDecl *getClangNodeOwner() const {
      return ClangNodeOwner;
   }

   /// Return the Clang node associated with this function if it has one.
   ClangNode getClangNode() const {
      return (ClangNodeOwner ? ClangNodeOwner->getClangNode() : ClangNode());
   }
   const clang::Decl *getClangDecl() const {
      return (ClangNodeOwner ? ClangNodeOwner->getClangDecl() : nullptr);
   }

   /// Returns whether this function is a specialization.
   bool isSpecialization() const { return SpecializationInfo != nullptr; }

   /// Return the specialization information.
   const GenericSpecializationInformation *getSpecializationInfo() const {
      assert(isSpecialization());
      return SpecializationInfo;
   }

   void setSpecializationInfo(const GenericSpecializationInformation *Info) {
      assert(!isSpecialization());
      validateSubclassScope(getClassSubclassScope(), isThunk(), Info);
      SpecializationInfo = Info;
   }

   /// Retrieve the generic environment containing the mapping from interface
   /// types to context archetypes for this function. Only present if the
   /// function has a body.
   GenericEnvironment *getGenericEnvironment() const {
      return GenericEnv;
   }
   void setGenericEnvironment(GenericEnvironment *env) {
      GenericEnv = env;
   }

   /// Map the given type, which is based on an interface PILFunctionType and may
   /// therefore be dependent, to a type based on the context archetypes of this
   /// PILFunction.
   Type mapTypeIntoContext(Type type) const;

   /// Map the given type, which is based on an interface PILFunctionType and may
   /// therefore be dependent, to a type based on the context archetypes of this
   /// PILFunction.
   PILType mapTypeIntoContext(PILType type) const;

   /// Converts the given function definition to a declaration.
   void convertToDeclaration();

   /// Return the identity substitutions necessary to forward this call if it is
   /// generic.
   SubstitutionMap getForwardingSubstitutionMap();

   //===--------------------------------------------------------------------===//
   // Block List Access
   //===--------------------------------------------------------------------===//

   BlockListType &getBlocks() { return BlockList; }
   const BlockListType &getBlocks() const { return BlockList; }

   using iterator = BlockListType::iterator;
   using reverse_iterator = BlockListType::reverse_iterator;
   using const_iterator = BlockListType::const_iterator;

   bool empty() const { return BlockList.empty(); }
   iterator begin() { return BlockList.begin(); }
   iterator end() { return BlockList.end(); }
   reverse_iterator rbegin() { return BlockList.rbegin(); }
   reverse_iterator rend() { return BlockList.rend(); }
   const_iterator begin() const { return BlockList.begin(); }
   const_iterator end() const { return BlockList.end(); }
   unsigned size() const { return BlockList.size(); }

   PILBasicBlock &front() { return *begin(); }
   const PILBasicBlock &front() const { return *begin(); }

   PILBasicBlock *getEntryBlock() { return &front(); }
   const PILBasicBlock *getEntryBlock() const { return &front(); }

   PILBasicBlock *createBasicBlock();
   PILBasicBlock *createBasicBlockAfter(PILBasicBlock *afterBB);
   PILBasicBlock *createBasicBlockBefore(PILBasicBlock *beforeBB);

   /// Splice the body of \p F into this function at end.
   void spliceBody(PILFunction *F) {
      getBlocks().splice(begin(), F->getBlocks());
   }

   /// Return the unique basic block containing a return inst if it
   /// exists. Otherwise, returns end.
   iterator findReturnBB() {
      return std::find_if(begin(), end(),
                          [](const PILBasicBlock &BB) -> bool {
                             const TermInst *TI = BB.getTerminator();
                             return isa<ReturnInst>(TI);
                          });
   }

   /// Return the unique basic block containing a return inst if it
   /// exists. Otherwise, returns end.
   const_iterator findReturnBB() const {
      return std::find_if(begin(), end(),
                          [](const PILBasicBlock &BB) -> bool {
                             const TermInst *TI = BB.getTerminator();
                             return isa<ReturnInst>(TI);
                          });
   }

   /// Return the unique basic block containing a throw inst if it
   /// exists. Otherwise, returns end.
   iterator findThrowBB() {
      return std::find_if(begin(), end(),
                          [](const PILBasicBlock &BB) -> bool {
                             const TermInst *TI = BB.getTerminator();
                             return isa<ThrowInst>(TI);
                          });
   }

   /// Return the unique basic block containing a throw inst if it
   /// exists. Otherwise, returns end.
   const_iterator findThrowBB() const {
      return std::find_if(begin(), end(),
                          [](const PILBasicBlock &BB) -> bool {
                             const TermInst *TI = BB.getTerminator();
                             return isa<ThrowInst>(TI);
                          });
   }

   /// Loop over all blocks in this function and add all function exiting blocks
   /// to output.
   void findExitingBlocks(llvm::SmallVectorImpl<PILBasicBlock *> &output) const {
      for (auto &Block : const_cast<PILFunction &>(*this)) {
         if (Block.getTerminator()->isFunctionExiting()) {
            output.emplace_back(&Block);
         }
      }
   }

   //===--------------------------------------------------------------------===//
   // Argument Helper Methods
   //===--------------------------------------------------------------------===//

   PILArgument *getArgument(unsigned i) {
      assert(!empty() && "Cannot get argument of a function without a body");
      return begin()->getArgument(i);
   }

   const PILArgument *getArgument(unsigned i) const {
      assert(!empty() && "Cannot get argument of a function without a body");
      return begin()->getArgument(i);
   }

   ArrayRef<PILArgument *> getArguments() const {
      assert(!empty() && "Cannot get arguments of a function without a body");
      return begin()->getArguments();
   }

   ArrayRef<PILArgument *> getIndirectResults() const {
      assert(!empty() && "Cannot get arguments of a function without a body");
      return begin()->getArguments().slice(
         0, getConventions().getNumIndirectPILResults());
   }

   ArrayRef<PILArgument *> getArgumentsWithoutIndirectResults() const {
      assert(!empty() && "Cannot get arguments of a function without a body");
      return begin()->getArguments().slice(
         getConventions().getNumIndirectPILResults());
   }

   const PILArgument *getSelfArgument() const {
      assert(hasSelfParam() && "This method can only be called if the "
                               "PILFunction has a self parameter");
      return getArguments().back();
   }

   const PILArgument *getSelfMetadataArgument() const {
      assert(hasSelfMetadataParam() && "This method can only be called if the "
                                       "PILFunction has a self-metadata parameter");
      return getArguments().back();
   }

   //===--------------------------------------------------------------------===//
   // Miscellaneous
   //===--------------------------------------------------------------------===//

   /// verify - Run the IR verifier to make sure that the PILFunction follows
   /// invariants.
   void verify(bool SingleFunction = true) const;

   /// Verify that all non-cond-br critical edges have been split.
   ///
   /// This is a fast subset of the checks performed in the PILVerifier.
   void verifyCriticalEdges() const;

   /// Pretty-print the PILFunction.
   void dump(bool Verbose) const;
   void dump() const;

   /// Pretty-print the PILFunction.
   /// Useful for dumping the function when running in a debugger.
   /// Warning: no error handling is done. Fails with an assert if the file
   /// cannot be opened.
   void dump(const char *FileName) const;

   /// Pretty-print the PILFunction to the tream \p OS.
   ///
   /// \param Verbose Dump PIL location information in verbose mode.
   void print(raw_ostream &OS, bool Verbose = false) const {
      PILPrintContext PrintCtx(OS, Verbose);
      print(PrintCtx);
   }

   /// Pretty-print the PILFunction with the context \p PrintCtx.
   void print(PILPrintContext &PrintCtx) const;

   /// Pretty-print the PILFunction's name using PIL syntax,
   /// '@function_mangled_name'.
   void printName(raw_ostream &OS) const;

   /// Assigns consecutive numbers to all the PILNodes in the function.
   /// For instructions, both the instruction node and the value nodes of
   /// any results will be assigned numbers; the instruction node will
   /// be numbered the same as the first result, if there are any results.
   void numberValues(llvm::DenseMap<const PILNode*, unsigned> &nodeToNumberMap)
   const;

   AstContext &getAstContext() const;

   /// This function is meant for use from the debugger.  You can just say 'call
   /// F->viewCFG()' and a ghostview window should pop up from the program,
   /// displaying the CFG of the current function with the code for each basic
   /// block inside.  This depends on there being a 'dot' and 'gv' program in
   /// your path.
   void viewCFG() const;
   /// Like ViewCFG, but the graph does not show the contents of basic blocks.
   void viewCFGOnly() const;

};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const PILFunction &F) {
   F.print(OS);
   return OS;
}

} // end polar namespace

//===----------------------------------------------------------------------===//
// ilist_traits for PILFunction
//===----------------------------------------------------------------------===//

namespace llvm {

template <>
struct ilist_traits<::polar::PILFunction> :
   public ilist_node_traits<::polar::PILFunction> {
   using PILFunction = ::polar::PILFunction;

public:
   static void deleteNode(PILFunction *V) { V->~PILFunction(); }

private:
   void createNode(const PILFunction &);
};

} // end llvm namespace

#endif
