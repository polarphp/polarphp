//===--- PILDeclRef.h - Defines the PILDeclRef struct -----------*- C++ -*-===//
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
// This file defines the PILDeclRef struct, which is used to identify a PIL
// global identifier that can be used as the operand of a FunctionRefInst
// instruction or that can have a PIL Function associated with it.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PIL_DECL_REF_H
#define POLARPHP_PIL_PIL_DECL_REF_H

#include "polarphp/ast/ClangNode.h"
#include "polarphp/ast/TypeAlignments.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/PrettyStackTrace.h"

namespace llvm {
class raw_ostream;
}

namespace polar {

class PILFunctionType;
class ClassDecl;
class AstContext;
class AbstractFunctionDecl;
class AbstractClosureExpr;
class ValueDecl;
class FuncDecl;
class ClosureExpr;
class AutoClosureExpr;
class AnyFunctionRef;
enum class EffectsKind : uint8_t;
enum class PILLinkage : unsigned char;
enum IsSerialized_t : unsigned char;
enum class SubclassScope : unsigned char;
class PILModule;
class PILLocation;
class ClangNode;

/// How a method is dispatched.
enum class MethodDispatch {
   // The method implementation can be referenced statically.
      Static,
   // The method implementation uses class_method dispatch.
      Class,
};

/// Get the method dispatch mechanism for a method.
MethodDispatch getMethodDispatch(AbstractFunctionDecl *method);

/// True if calling the given method or property should use ObjC dispatch.
bool requiresForeignEntryPoint(ValueDecl *vd);

/// True if the entry point is natively foreign.
bool requiresForeignToNativeThunk(ValueDecl *vd);

enum ForDefinition_t : bool {
   NotForDefinition = false,
   ForDefinition = true
};

/// A key for referencing a Swift declaration in PIL.
///
/// This can currently be either a reference to a ValueDecl for functions,
/// methods, constructors, and other named entities, or a reference to a
/// AbstractClosureExpr for an anonymous function.  In addition to the AST
/// reference, there are discriminators for referencing different
/// implementation-level entities associated with a single language-level
/// declaration, such as uncurry levels of a function, the allocating and
/// initializing entry points of a constructor, etc.
struct PILDeclRef {
   using Loc = llvm::PointerUnion<ValueDecl *, AbstractClosureExpr *>;

   /// Represents the "kind" of the PILDeclRef. For some Swift decls there
   /// are multiple PIL entry points, and the kind is used to distinguish them.
   enum class Kind : unsigned {
      /// This constant references the FuncDecl or AbstractClosureExpr
      /// in loc.
         Func,

      /// Allocator - this constant references the allocating constructor
      /// entry point of a class ConstructorDecl or the constructor of a value
      /// ConstructorDecl.
         Allocator,
      /// Initializer - this constant references the initializing constructor
      /// entry point of the class ConstructorDecl in loc.
         Initializer,

      /// EnumElement - this constant references the injection function for
      /// an EnumElementDecl.
         EnumElement,

      /// Destroyer - this constant references the destroying destructor for the
      /// DestructorDecl in loc.
         Destroyer,

      /// Deallocator - this constant references the deallocating
      /// destructor for the DestructorDecl in loc.
         Deallocator,

      /// GlobalAccessor - this constant references the lazy-initializing
      /// accessor for the global VarDecl in loc.
         GlobalAccessor,

      /// References the generator for a default argument of a function.
         DefaultArgGenerator,

      /// References the initializer expression for a stored property
      /// of a nominal type.
         StoredPropertyInitializer,

      /// References the ivar initializer for the ClassDecl in loc.
      ///
      /// Only classes that are allocated using Objective-C's allocation
      /// routines have an ivar initializer, which is emitted as
      /// .cxx_construct.
         IVarInitializer,

      /// References the ivar destroyer for the ClassDecl in loc.
      ///
      /// Only classes that are allocated using Objective-C's allocation
      /// routines have an ivar destroyer, which is emitted as
      /// .cxx_destruct.
         IVarDestroyer,

      /// References the wrapped value injection function used to initialize
      /// the backing storage property from a wrapped value.
         PropertyWrapperBackingInitializer,
   };

   /// The ValueDecl or AbstractClosureExpr represented by this PILDeclRef.
   Loc loc;
   /// The Kind of this PILDeclRef.
   Kind kind : 4;
   /// True if the PILDeclRef is a curry thunk.
   unsigned isCurried : 1;
   /// True if this references a foreign entry point for the referenced decl.
   unsigned isForeign : 1;
   /// True if this is a direct reference to a class's method implementation
   /// that isn't dynamically dispatched.
   unsigned isDirectReference : 1;
   /// The default argument index for a default argument getter.
   unsigned defaultArgIndex : 10;

   /// Produces a null PILDeclRef.
   PILDeclRef() : loc(), kind(Kind::Func),
                  isCurried(0), isForeign(0), isDirectReference(0),
                  defaultArgIndex(0) {}

   /// Produces a PILDeclRef of the given kind for the given decl.
   explicit PILDeclRef(ValueDecl *decl, Kind kind,
                       bool isCurried = false,
                       bool isForeign = false);

   /// Produces a PILDeclRef for the given ValueDecl or
   /// AbstractClosureExpr:
   /// - If 'loc' is a func or closure, this returns a Func PILDeclRef.
   /// - If 'loc' is a ConstructorDecl, this returns the Allocator PILDeclRef
   ///   for the constructor.
   /// - If 'loc' is an EnumElementDecl, this returns the EnumElement
   ///   PILDeclRef for the enum element.
   /// - If 'loc' is a DestructorDecl, this returns the Destructor PILDeclRef
   ///   for the containing ClassDecl.
   /// - If 'loc' is a global VarDecl, this returns its GlobalAccessor
   ///   PILDeclRef.
   ///
   /// If 'isCurried' is true, the loc must be a method or enum element;
   /// the PILDeclRef will then refer to a curry thunk with type
   /// (Self) -> (Args...) -> Result, rather than a direct reference to
   /// the actual method whose lowered type is (Args..., Self) -> Result.
   explicit PILDeclRef(Loc loc,
                       bool isCurried = false,
                       bool isForeign = false);

   /// Produce a PIL constant for a default argument generator.
   static PILDeclRef getDefaultArgGenerator(Loc loc, unsigned defaultArgIndex);

   bool isNull() const { return loc.isNull(); }
   explicit operator bool() const { return !isNull(); }

   bool hasDecl() const { return loc.is<ValueDecl *>(); }
   bool hasClosureExpr() const;
   bool hasAutoClosureExpr() const;
   bool hasFuncDecl() const;

   ValueDecl *getDecl() const { return loc.get<ValueDecl *>(); }
   AbstractClosureExpr *getAbstractClosureExpr() const {
      return loc.dyn_cast<AbstractClosureExpr *>();
   }
   ClosureExpr *getClosureExpr() const;
   AutoClosureExpr *getAutoClosureExpr() const;
   FuncDecl *getFuncDecl() const;
   AbstractFunctionDecl *getAbstractFunctionDecl() const;

   llvm::Optional<AnyFunctionRef> getAnyFunctionRef() const;

   PILLocation getAsRegularLocation() const;

   enum class ManglingKind {
      Default,
      DynamicThunk,
   };

   /// Produce a mangled form of this constant.
   std::string mangle(ManglingKind MKind = ManglingKind::Default) const;

   /// True if the PILDeclRef references a function.
   bool isFunc() const {
      return kind == Kind::Func;
   }

   /// True if the PILDeclRef references a setter function.
   bool isSetter() const;

   /// True if the PILDeclRef references a constructor entry point.
   bool isConstructor() const {
      return kind == Kind::Allocator || kind == Kind::Initializer;
   }
   /// True if the PILDeclRef references a destructor entry point.
   bool isDestructor() const {
      return kind == Kind::Destroyer || kind == Kind::Deallocator;
   }
   /// True if the PILDeclRef references an enum entry point.
   bool isEnumElement() const {
      return kind == Kind::EnumElement;
   }
   /// True if the PILDeclRef references a global variable accessor.
   bool isGlobal() const {
      return kind == Kind::GlobalAccessor;
   }
   /// True if the PILDeclRef references the generator for a default argument of
   /// a function.
   bool isDefaultArgGenerator() const {
      return kind == Kind::DefaultArgGenerator;
   }
   /// True if the PILDeclRef references the initializer for a stored property
   /// of a nominal type.
   bool isStoredPropertyInitializer() const {
      return kind == Kind::StoredPropertyInitializer;
   }
   /// True if the PILDeclRef references the initializer for the backing storage
   /// of a property wrapper.
   bool isPropertyWrapperBackingInitializer() const {
      return kind == Kind::PropertyWrapperBackingInitializer;
   }

   /// True if the PILDeclRef references the ivar initializer or deinitializer of
   /// a class.
   bool isIVarInitializerOrDestroyer() const {
      return kind == Kind::IVarInitializer || kind == Kind::IVarDestroyer;
   }

   /// True if the PILDeclRef references an allocating or deallocating entry
   /// point.
   bool isInitializerOrDestroyer() const {
      return kind == Kind::Initializer || kind == Kind::Destroyer;
   }

   /// True if the function should be treated as transparent.
   bool isTransparent() const;
   /// True if the function should have its body serialized.
   IsSerialized_t isSerialized() const;
   /// True if the function has noinline attribute.
   bool isNoinline() const;
   /// True if the function has __always inline attribute.
   bool isAlwaysInline() const;

   /// \return True if the function has an effects attribute.
   bool hasEffectsAttribute() const;

   /// \return the effects kind of the function.
   EffectsKind getEffectsAttribute() const;

   /// Return the expected linkage of this declaration.
   PILLinkage getLinkage(ForDefinition_t forDefinition) const;

   /// Return the hash code for the PIL declaration.
   llvm::hash_code getHashCode() const {
      return llvm::hash_combine(loc.getOpaqueValue(),
                                static_cast<int>(kind),
                                isCurried, isForeign, isDirectReference,
                                defaultArgIndex);
   }

   bool operator==(PILDeclRef rhs) const {
      return loc.getOpaqueValue() == rhs.loc.getOpaqueValue()
             && kind == rhs.kind
             && isCurried == rhs.isCurried
             && isForeign == rhs.isForeign
             && isDirectReference == rhs.isDirectReference
             && defaultArgIndex == rhs.defaultArgIndex;
   }
   bool operator!=(PILDeclRef rhs) const {
      return !(*this == rhs);
   }

   void print(llvm::raw_ostream &os) const;
   void dump() const;

   unsigned getParameterListCount() const;

   // Returns the PILDeclRef for an entity at a shallower uncurry level.
   PILDeclRef asCurried(bool curried = true) const {
      assert(!isCurried && "can't safely go to deeper uncurry level");
      // Curry thunks are never foreign.
      bool willBeForeign = isForeign && !curried;
      bool willBeDirect = isDirectReference;
      return PILDeclRef(loc.getOpaqueValue(), kind,
                        curried, willBeDirect, willBeForeign,
                        defaultArgIndex);
   }

   /// Returns the foreign (or native) entry point corresponding to the same
   /// decl.
   PILDeclRef asForeign(bool foreign = true) const {
      assert(!isCurried);
      return PILDeclRef(loc.getOpaqueValue(), kind,
                        isCurried, isDirectReference, foreign, defaultArgIndex);
   }

   PILDeclRef asDirectReference(bool direct = true) const {
      PILDeclRef r = *this;
      // The 'direct' distinction only makes sense for curry thunks.
      if (r.isCurried)
         r.isDirectReference = direct;
      return r;
   }

   /// True if the decl ref references a thunk from a natively foreign
   /// declaration to Swift calling convention.
   bool isForeignToNativeThunk() const;

   /// True if the decl ref references a thunk from a natively Swift declaration
   /// to foreign C or ObjC calling convention.
   bool isNativeToForeignThunk() const;

   /// True if the decl ref references a method which introduces a new vtable
   /// entry.
   bool requiresNewVTableEntry() const;

   /// True if the decl ref references a method which introduces a new witness
   /// table entry.
   bool requiresNewWitnessTableEntry() const;

   /// True if the decl is a method which introduces a new witness table entry.
   static bool requiresNewWitnessTableEntry(AbstractFunctionDecl *func);

   /// Return a PILDeclRef to the declaration overridden by this one, or
   /// a null PILDeclRef if there is no override.
   PILDeclRef getOverridden() const;

   /// Return a PILDeclRef to the declaration whose vtable entry this declaration
   /// overrides. This may be different from "getOverridden" because some
   /// declarations do not always have vtable entries.
   PILDeclRef getNextOverriddenVTableEntry() const;

   /// Return the most derived override which requires a new vtable entry.
   /// If the method does not override anything or no override is vtable
   /// dispatched, will return the least derived method.
   PILDeclRef getOverriddenVTableEntry() const;

   /// Return the original protocol requirement that introduced the witness table
   /// entry overridden by this method.
   PILDeclRef getOverriddenWitnessTableEntry() const;

   /// Return the original protocol requirement that introduced the witness table
   /// entry overridden by this method.
   static AbstractFunctionDecl *getOverriddenWitnessTableEntry(
      AbstractFunctionDecl *func);

   /// True if the referenced entity is some kind of thunk.
   bool isThunk() const;

   /// True if the referenced entity is emitted by Swift on behalf of the Clang
   /// importer.
   bool isClangImported() const;

   /// True if the referenced entity is emitted by Clang on behalf of the Clang
   /// importer.
   bool isClangGenerated() const;
   static bool isClangGenerated(ClangNode node);

   bool isImplicit() const;

   /// Return the scope in which the parent class of a method (i.e. class
   /// containing this declaration) can be subclassed, returning NotApplicable if
   /// this is not a method, there is no such class, or the class cannot be
   /// subclassed.
   SubclassScope getSubclassScope() const;

   bool isDynamicallyReplaceable() const;

   bool canBeDynamicReplacement() const;

private:
   friend struct llvm::DenseMapInfo<polar::PILDeclRef>;
   /// Produces a PILDeclRef from an opaque value.
   explicit PILDeclRef(void *opaqueLoc,
                       Kind kind,
                       bool isCurried,
                       bool isDirectReference,
                       bool isForeign,
                       unsigned defaultArgIndex)
      : loc(Loc::getFromOpaqueValue(opaqueLoc)),
        kind(kind),
        isCurried(isCurried),
        isForeign(isForeign), isDirectReference(isDirectReference),
        defaultArgIndex(defaultArgIndex)
   {}

};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, PILDeclRef C) {
   C.print(OS);
   return OS;
}

} // end swift namespace

namespace llvm {

// DenseMap key support for PILDeclRef.
template<> struct DenseMapInfo<polar::PILDeclRef> {
   using PILDeclRef = polar::PILDeclRef;
   using Kind = PILDeclRef::Kind;
   using Loc = PILDeclRef::Loc;
   using PointerInfo = DenseMapInfo<void*>;
   using UnsignedInfo = DenseMapInfo<unsigned>;

   static PILDeclRef getEmptyKey() {
      return PILDeclRef(PointerInfo::getEmptyKey(), Kind::Func,
                        false, false, false, 0);
   }
   static PILDeclRef getTombstoneKey() {
      return PILDeclRef(PointerInfo::getTombstoneKey(), Kind::Func,
                        false, false, false, 0);
   }
   static unsigned getHashValue(polar::PILDeclRef Val) {
      unsigned h1 = PointerInfo::getHashValue(Val.loc.getOpaqueValue());
      unsigned h2 = UnsignedInfo::getHashValue(unsigned(Val.kind));
      unsigned h3 = (Val.kind == Kind::DefaultArgGenerator)
                    ? UnsignedInfo::getHashValue(Val.defaultArgIndex)
                    : UnsignedInfo::getHashValue(Val.isCurried);
      unsigned h4 = UnsignedInfo::getHashValue(Val.isForeign);
      unsigned h5 = UnsignedInfo::getHashValue(Val.isDirectReference);
      return h1 ^ (h2 << 4) ^ (h3 << 9) ^ (h4 << 7) ^ (h5 << 11);
   }
   static bool isEqual(polar::PILDeclRef const &LHS,
                       polar::PILDeclRef const &RHS) {
      return LHS == RHS;
   }
};

} // end llvm namespace

#endif // POLARPHP_PIL_PIL_DECL_REF_H
