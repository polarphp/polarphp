//===--- Types.h - Swift Language Type ASTs ---------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//
//
// This file defines the TypeBase class and subclasses.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_TYPES_H
#define POLARPHP_AST_TYPES_H

#include "polarphp/ast/DeclContext.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/adt/ArrayRefView.h"
#include "polarphp/basic/InlineBitfield.h"
#include "polarphp/basic/UUID.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/basic/adt/PointerEmbeddedInt.h"
#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/basic/adt/SmallBitVector.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/TrailingObjects.h"

namespace polar::ast {

enum class AllocationArena;
class ArchetypeType;
class AssociatedTypeDecl;
class AstContext;
class ClassDecl;
class DependentMemberType;
class GenericTypeParamDecl;
class GenericTypeParamType;
class GenericParamList;
class GenericSignature;
class Identifier;
class InOutType;
class OpenedArchetypeType;
enum class ReferenceCounting : uint8_t;
enum class ResilienceExpansion : unsigned;
class TypeAliasDecl;
class TypeDecl;
class NominalTypeDecl;
class GenericTypeDecl;
class EnumDecl;
class EnumElementDecl;
class StructDecl;
class TypeVariableType;
class ValueDecl;
enum PointerTypeKind : unsigned;
struct ValueOwnershipKind;

using polar::basic::SmallBitVector;

enum class TypeKind : uint8_t
{
#define TYPE(id, parent) id,
#define LAST_TYPE(id) Last_Type = id,
#define TYPE_RANGE(Id, FirstId, LastId) \
   First_##Id##Type = FirstId, Last_##Id##Type = LastId,
#include "polarphp/ast/TypeNodesDefs.h"
};

enum : unsigned
{
   NumTypeKindBits =
   polar::basic::count_bits_used(static_cast<unsigned>(TypeKind::Last_Type))
};

/// Various properties of types that are primarily defined recursively
/// on structural types.
class RecursiveTypeProperties
{
public:
   /// A single property.
   ///
   /// Note that the property polarities should be chosen so that 0 is
   /// the correct default value and bitwise-or correctly merges things.
   enum Property : unsigned
   {
      /// This type expression contains a TypeVariableType.
      HasTypeVariable      = 0x01,

      /// This type expression contains an ArchetypeType.
      HasArchetype         = 0x02,

      /// This type expression contains a GenericTypeParamType.
      HasTypeParameter     = 0x04,

      /// This type expression contains an UnresolvedType.
      HasUnresolvedType    = 0x08,

      /// Whether this type expression contains an unbound generic type.
      HasUnboundGeneric    = 0x10,

      /// This type expression contains an LValueType other than as a
      /// function input, and can be loaded to convert to an rvalue.
      IsLValue             = 0x20,

      /// This type expression contains an opened existential ArchetypeType.
      HasOpenedExistential = 0x40,

      /// This type expression contains a DynamicSelf type.
      HasDynamicSelf       = 0x80,

      /// This type contains an Error type.
      HasError             = 0x100,

      /// This type contains a DependentMemberType.
      HasDependentMember   = 0x200,

      Last_Property = HasDependentMember
   };
   enum { BitWidth = polar::basic::count_bits_used(Property::Last_Property) };

private:
   unsigned m_bits;

public:
   RecursiveTypeProperties()
      : m_bits(0)
   {}

   RecursiveTypeProperties(Property prop)
      :
        m_bits(prop)
   {}

   explicit RecursiveTypeProperties(unsigned bits)
      : m_bits(bits)
   {}

   /// Return these properties as a bitfield.
   unsigned getBits() const
   {
      return m_bits;
   }

   /// Does a type with these properties structurally contain a type
   /// variable?
   bool hasTypeVariable() const
   {
      return m_bits & HasTypeVariable;
   }

   /// Does a type with these properties structurally contain an
   /// archetype?
   bool hasArchetype() const
   {
      return m_bits & HasArchetype;
   }

   /// Does a type with these properties have a type parameter somewhere in it?
   bool hasTypeParameter() const
   {
      return m_bits & HasTypeParameter;
   }

   /// Does a type with these properties have an unresolved type somewhere in it?
   bool hasUnresolvedType() const
   {
      return m_bits & HasUnresolvedType;
   }

   /// Is a type with these properties an lvalue?
   bool isLValue() const
   {
      return m_bits & IsLValue;
   }

   /// Does this type contain an error?
   bool hasError() const
   {
      return m_bits & HasError;
   }

   /// Does this type contain a dependent member type, possibly with a
   /// non-type parameter base, such as a type variable or concrete type?
   bool hasDependentMember() const
   {
      return m_bits & HasDependentMember;
   }

   /// Does a type with these properties structurally contain an
   /// archetype?
   bool hasOpenedExistential() const
   {
      return m_bits & HasOpenedExistential;
   }

   /// Does a type with these properties structurally contain a
   /// reference to DynamicSelf?
   bool hasDynamicSelf() const
   {
      return m_bits & HasDynamicSelf;
   }

   /// Does a type with these properties structurally contain an unbound
   /// generic type?
   bool hasUnboundGeneric() const
   {
      return m_bits & HasUnboundGeneric;
   }

   /// Returns the set of properties present in either set.
   friend RecursiveTypeProperties operator|(Property lhs, Property rhs)
   {
      return RecursiveTypeProperties(unsigned(lhs) | unsigned(rhs));
   }

   friend RecursiveTypeProperties operator|(RecursiveTypeProperties lhs,
                                            RecursiveTypeProperties rhs)
   {
      return RecursiveTypeProperties(lhs.m_bits | rhs.m_bits);
   }

   /// Add any properties in the right-hand set to this set.
   RecursiveTypeProperties &operator|=(RecursiveTypeProperties other)
   {
      m_bits |= other.m_bits;
      return *this;
   }
   /// Restrict this to only the properties in the right-hand set.
   RecursiveTypeProperties &operator&=(RecursiveTypeProperties other)
   {
      m_bits &= other.m_bits;
      return *this;
   }

   /// Remove the HasTypeParameter property from this set.
   void removeHasTypeParameter()
   {
      m_bits &= ~HasTypeParameter;
   }

   /// Remove the HasDependentMember property from this set.
   void removeHasDependentMember()
   {
      m_bits &= ~HasDependentMember;
   }

   /// Test for a particular property in this set.
   bool operator&(Property prop) const
   {
      return m_bits & prop;
   }
};

inline RecursiveTypeProperties operator~(RecursiveTypeProperties::Property property)
{
   return RecursiveTypeProperties(~unsigned(property));
}

/// The result of a type trait check.
enum class TypeTraitResult
{
   /// The type cannot have the trait.
   IsNot,
   /// The generic type can be bound to a type that has the trait.
   CanBe,
   /// The type has the trait irrespective of generic substitutions.
   Is,
};

/// Specifies which normally-unsafe type mismatches should be accepted when
/// checking overrides.
enum class TypeMatchFlags
{
   /// Allow properly-covariant overrides.
   AllowOverride = 1 << 0,
   /// Allow a parameter with IUO type to be overridden by a parameter with non-
   /// optional type.
   AllowNonOptionalForIUOParam = 1 << 1,
   /// Allow any mismatches of Optional or ImplicitlyUnwrappedOptional at the
   /// top level of a type.
   ///
   /// This includes function parameters and result types as well as tuple
   /// elements, but excludes generic parameters.
   AllowTopLevelOptionalMismatch = 1 << 2,
   /// Allow any ABI-compatible types to be considered matching.
   AllowABICompatible = 1 << 3,
   /// Allow escaping function parameters to override optional non-escaping ones.
   ///
   /// This is necessary because Objective-C allows optional function paramaters
   /// to be non-escaping, but Swift currently does not.
   IgnoreNonEscapingForOptionalFunctionParam = 1 << 4
};
using TypeMatchOptions = OptionSet<TypeMatchFlags>;

/// TypeBase - Base class for all types in polar.
class alignas(1 << TypeAlignInBits) TypeBase
{
   private:
   friend class AstContext;
   TypeBase(const TypeBase&) = delete;
   void operator=(const TypeBase&) = delete;

   /// This union contains to the AstContext for canonical types, and is
   /// otherwise lazily populated by AstContext when the canonical form of a
   /// non-canonical type is requested. The disposition of the union is stored
   /// outside of the union for performance. See m_bits.TypeBase.IsCanonical.
   union
   {
      CanType m_canonicalType;
      const AstContext *m_context;
   };

   /// Returns true if the given type is a sugared type.
   ///
   /// Only intended for use in compile-time assertions.
   // Specializations of this are at the end of the file.
   template <typename T>
   static constexpr bool isSugaredType()
   {
      return false;
   }

   protected:
   enum { NumAFTExtInfoBits = 6 };
   enum { NumSILExtInfoBits = 6 };
   union {
      uint64_t OpaqueBits;

      POLAR_INLINE_BITFIELD_BASE(
               TypeBase, polar::basic::bitmax(NumTypeKindBits,8) +
               RecursiveTypeProperties::BitWidth + 1,
               /// kind - The discriminator that indicates what subclass of type this is.
               kind : polar::basic::bitmax(NumTypeKindBits,8),

               Properties : RecursiveTypeProperties::BitWidth,

               /// Whether this type is canonical or not.
               IsCanonical : 1
               );

      POLAR_INLINE_BITFIELD(
               ErrorType, TypeBase, 1,
               /// Whether there is an original type.
               HasOriginalType : 1
               );

      POLAR_INLINE_BITFIELD(
               SugarType, TypeBase, 1,
               HasCachedType : 1
               );

      enum { NumFlagBits = 8 };
      POLAR_INLINE_BITFIELD(
               ParenType, SugarType, NumFlagBits,
               /// Whether there is an original type.
               m_flags : NumFlagBits
               );

      POLAR_INLINE_BITFIELD_FULL(
               AnyFunctionType, TypeBase, NumAFTExtInfoBits+16,
               /// Extra information which affects how the function is called, like
               /// regparm and the calling convention.
               ExtInfo : NumAFTExtInfoBits,

               : NumPadBits,
               numParams : 16
               );

      POLAR_INLINE_BITFIELD_FULL(
               ArchetypeType, TypeBase, 1+1+1+16,
               ExpandedNestedTypes : 1,
               HasSuperclass : 1,
               HasLayoutConstraint : 1,
               : NumPadBits,
               NumProtocols : 16
               );

      POLAR_INLINE_BITFIELD_FULL(
               TypeVariableType, TypeBase, 64-NumTypeBaseBits,
               /// The unique number assigned to this type variable.
               id : 32 - NumTypeBaseBits,

               /// Type variable options.
               Options : 3,

               ///  Index into the list of type variables, as used by the
               ///  constraint graph.
               GraphIndex : 29
               );

      POLAR_INLINE_BITFIELD(
               SILFunctionType, TypeBase, NumSILExtInfoBits+3+1+2,
               ExtInfo : NumSILExtInfoBits,
               CalleeConvention : 3,
               HasErrorResult : 1,
               CoroutineKind : 2
               );

      POLAR_INLINE_BITFIELD(
               AnyMetatypeType, TypeBase, 2,
               /// The representation of the metatype.
               ///
               /// Zero indicates that no representation has been set; otherwise,
               /// the value is the representation + 1
               Representation : 2
               );

      POLAR_INLINE_BITFIELD_FULL(
               ProtocolCompositionType, TypeBase, 1+32,
               /// Whether we have an explicitly-stated class constraint not
               /// implied by any of our members.
               HasExplicitAnyObject : 1,

               : NumPadBits,

               /// The number of protocols being composed.
               Count : 32
               );

      POLAR_INLINE_BITFIELD_FULL(
               TupleType, TypeBase, 1+32,
               /// Whether an element of the tuple is inout, __shared or __owned.
               /// Values cannot have such tuple types in the language.
               HasElementWithOwnership : 1,

               : NumPadBits,

               /// The number of elements of the tuple.
               Count : 32
               );

      POLAR_INLINE_BITFIELD_FULL(
               BoundGenericType, TypeBase, 32,
               : NumPadBits,

               /// The number of generic arguments.
               GenericArgCount : 32
               );

      POLAR_INLINE_BITFIELD_FULL(
               TypeAliasType, SugarType, 1+1,
               : NumPadBits,

               /// Whether we have a parent type.
               HasParent : 1,

               /// Whether we have a substitution map.
               HasSubstitutionMap : 1
               );

   } m_bits;

   protected:
   TypeBase(TypeKind kind, const AstContext *canTypeCtx,
            RecursiveTypeProperties properties)
      : m_context(nullptr)
   {
      m_bits.OpaqueBits = 0;
      m_bits.TypeBase.kind = static_cast<unsigned>(kind);
      m_bits.TypeBase.IsCanonical = false;
      // If this type is canonical, switch the CanonicalType union to AstContext.
      if (canTypeCtx) {
         m_bits.TypeBase.IsCanonical = true;
         m_context = canTypeCtx;
      }
      setRecursiveProperties(properties);
   }

   void setRecursiveProperties(RecursiveTypeProperties properties)
   {
      m_bits.TypeBase.Properties = properties.getBits();
      assert(m_bits.TypeBase.Properties == properties.getBits() && "m_bits dropped!");
   }

   public:
   /// getKind - Return what kind of type this is.
   TypeKind getKind() const
   {
      return static_cast<TypeKind>(m_bits.TypeBase.kind);
   }

   /// isCanonical - Return true if this is a canonical type.
   bool isCanonical() const
   {
      return m_bits.TypeBase.IsCanonical;
   }

   /// hasCanonicalTypeComputed - Return true if we've already computed a
   /// canonical version of this type.
   bool hasCanonicalTypeComputed() const
   {
      return !m_canonicalType.isNull();
   }

   private:
   CanType computeCanonicalType();

   public:
   /// getCanonicalType - Return the canonical version of this type, which has
   /// sugar from all levels stripped off.
   CanType getCanonicalType() const
   {
      if (isCanonical()) {
         return CanType(const_cast<TypeBase*>(this));
      }

      if (hasCanonicalTypeComputed()) {
         return m_canonicalType;
      }

      return const_cast<TypeBase*>(this)->computeCanonicalType();
   }

   /// getCanonicalType - Stronger canonicalization which folds away equivalent
   /// associated types, or type parameters that have been made concrete.
   CanType getCanonicalType(GenericSignature *sig);

   /// Reconstitute type sugar, e.g., for array types, dictionary
   /// types, optionals, etc.
   TypeBase *reconstituteSugar(bool recursive);

   /// getAstContext - Return the AstContext that this type belongs to.
   AstContext &getAstContext()
   {
      // If this type is canonical, it has the AstContext in it.
      if (isCanonical()) {
         return *const_cast<AstContext*>(m_context);
      }
      // If not, canonicalize it to get the m_context.
      return *const_cast<AstContext*>(getCanonicalType()->m_context);
   }

   /// isEqual - Return true if these two types are equal, ignoring sugar.
   ///
   /// To compare sugar, check for pointer equality of the underlying TypeBase *
   /// values, obtained by calling getPointer().
   bool isEqual(Type other);

   /// getDesugaredType - If this type is a sugared type, remove all levels of
   /// sugar until we get down to a non-sugar type.
   TypeBase *getDesugaredType();

   /// If this type is a (potentially sugared) type of the specified kind, remove
   /// the minimal amount of sugar required to get a pointer to the type.
   template <typename T>
   T *getAs()
   {
      static_assert(!isSugaredType<T>(), "getAs desugars types");
      auto type = getDesugaredType();
      POLAR_ASSUME(type != nullptr);
      return dyn_cast<T>(type);
   }

   template <typename T>
   bool is()
   {
      //      static_assert(!isSugaredType<T>(), "isa desugars types");
      //      return isa<T>(getDesugaredType());
   }

   template <typename T>
   T *castTo()
   {
      static_assert(!isSugaredType<T>(), "castTo desugars types");
      return cast<T>(getDesugaredType());
   }

   /// getRecursiveProperties - Returns the properties defined on the
   /// structure of this type.
   RecursiveTypeProperties getRecursiveProperties() const
   {
      return RecursiveTypeProperties(m_bits.TypeBase.Properties);
   }

   /// hasReferenceSemantics() - Do objects of this type have reference
   /// semantics?
   bool hasReferenceSemantics();

   /// Is this a nominally uninhabited type, such as 'Never'?
   bool isUninhabited();

   /// Is this an uninhabited type, such as 'Never' or '(Never, Int)'?
   bool isStructurallyUninhabited();

   /// Is this the 'Any' type?
   bool isAny();

   /// Does the type have outer parenthesis?
   bool hasParenSugar() const
   {
      return getKind() == TypeKind::Paren;
   }

   /// Are values of this type essentially just class references,
   /// possibly with some sort of additional information?
   ///
   ///   - any of the builtin reference types
   ///   - a class type
   ///   - a bound generic class type
   ///   - a class-bounded archetype type
   ///   - a class-bounded existential type
   ///   - a dynamic Self type
   bool isAnyClassReferenceType();

   /// allowsOwnership() - Are variables of this type permitted to have
   /// ownership attributes?
   bool allowsOwnership();

   /// Determine whether this type involves a type variable.
   bool hasTypeVariable() const
   {
      return getRecursiveProperties().hasTypeVariable();
   }

   /// Determine where this type is a type variable or a dependent
   /// member root in a type variable.
   bool isTypeVariableOrMember();

   /// Determine whether this type involves a UnresolvedType.
   bool hasUnresolvedType() const
   {
      return getRecursiveProperties().hasUnresolvedType();
   }

   /// Determine whether the type involves an archetype.
   bool hasArchetype() const
   {
      return getRecursiveProperties().hasArchetype();
   }

   /// Determine whether the type involves an opened existential archetype.
   bool hasOpenedExistential() const
   {
      return getRecursiveProperties().hasOpenedExistential();
   }

   /// Determine whether the type involves the given opened existential
   /// archetype.
   bool hasOpenedExistential(OpenedArchetypeType *opened);

   /// Determine whether the type is an opened existential type.
   ///
   /// To determine whether there is an opened existential type
   /// anywhere in the type, use \c hasOpenedExistential.
   bool isOpenedExistential() const;

   /// Determine whether the type is an opened existential type with Error inside
   bool isOpenedExistentialWithError();

   /// Retrieve the set of opened existential archetypes that occur
   /// within this type.
   void getOpenedExistentials(SmallVectorImpl<OpenedArchetypeType *> &opened);

   /// Erase the given opened existential type by replacing it with its
   /// existential type throughout the given type.
   Type eraseOpenedExistential(OpenedArchetypeType *opened);

   /// Erase DynamicSelfType from the given type by replacing it with its
   /// underlying type.
   Type eraseDynamicSelfType();

   /// Given a declaration context, returns a function type with the 'self'
   /// type curried as the input if the declaration context describes a type.
   /// Otherwise, returns the type itself.
   Type addCurriedSelfType(const DeclContext *dc);

   /// Map a contextual type to an interface type.
   Type mapTypeOutOfContext();

   /// Compute and return the set of type variables that occur within this
   /// type.
   ///
   /// \param typeVariables This vector is populated with the set of
   /// type variables referenced by this type.
   void getTypeVariables(SmallVectorImpl<TypeVariableType *> &typeVariables);

   /// Determine whether this type is a type parameter, which is either a
   /// GenericTypeParamType or a DependentMemberType.
   ///
   /// Note that this routine will return \c false for types that include type
   /// parameters in nested positions, e.g, \c T is a type parameter but
   /// \c X<T> is not a type parameter. Use \c hasTypeParameter to determine
   /// whether a type parameter exists at any position.
   bool isTypeParameter();

   /// Determine whether this type can dynamically be an optional type.
   ///
   /// \param includeExistential Whether an existential type should be considered
   /// such a type.
   bool canDynamicallyBeOptionalType(bool includeExistential);

   /// Determine whether this type contains a type parameter somewhere in it.
   bool hasTypeParameter()
   {
      return getRecursiveProperties().hasTypeParameter();
   }

   /// Find any unresolved dependent member type within this type.
   ///
   /// "Unresolved" dependent member types have no known associated type,
   /// and are only used transiently in the type checker.
   const DependentMemberType *findUnresolvedDependentMemberType();

   /// Return the root generic parameter of this type parameter type.
   GenericTypeParamType *getRootGenericParam();

   /// Determines whether this type is an lvalue. This includes both straight
   /// lvalue types as well as tuples or optionals of lvalues.
   bool hasLValueType()
   {
      return getRecursiveProperties().isLValue();
   }

   /// Is a type with these properties materializable: that is, is it a
   /// first-class value type?
   bool isMaterializable();

   /// Determine whether the type is dependent on DynamicSelf.
   bool hasDynamicSelfType() const
   {
      return getRecursiveProperties().hasDynamicSelf();
   }

   /// Determine whether the type contains an unbound generic type.
   bool hasUnboundGenericType() const
   {
      return getRecursiveProperties().hasUnboundGeneric();
   }

   /// Determine whether this type contains an error type.
   bool hasError() const
   {
      return getRecursiveProperties().hasError();
   }

   /// Does this type contain a dependent member type, possibly with a
   /// non-type parameter base, such as a type variable or concrete type?
   bool hasDependentMember() const
   {
      return getRecursiveProperties().hasDependentMember();
   }

   /// Check if this type is a valid type for the LHS of an assignment.
   /// This mainly means hasLValueType(), but empty tuples and tuples of empty
   /// tuples also qualify.
   bool isAssignableType();

   /// isExistentialType - Determines whether this type is an existential type,
   /// whose real (runtime) type is unknown but which is known to conform to
   /// some set of protocols. Protocol and protocol-conformance types are
   /// existential types.
   bool isExistentialType();

   /// isAnyExistentialType - Determines whether this type is any kind of
   /// existential type: a protocol type, a protocol composition type, or
   /// an existential metatype.
   bool isAnyExistentialType();

   /// Determines whether this type is an existential type with a class protocol
   /// bound.
   bool isClassExistentialType();

   /// Opens an existential instance or meta-type and returns the opened type.
   Type openAnyExistentialType(OpenedArchetypeType *&opened);

   /// Determines the element type of a known
   /// [Autoreleasing]Unsafe[Mutable][Raw]Pointer variant, or returns null if the
   /// type is not a pointer.
   Type getAnyPointerElementType(PointerTypeKind &ptk);
   Type getAnyPointerElementType()
   {
      PointerTypeKind ignore;
      return getAnyPointerElementType(ignore);
   }

   /// Determine whether the given type is "specialized", meaning that
   /// it involves generic types for which generic arguments have been provided.
   /// For example, the types Vector<Int> and Vector<Int>.Element are both
   /// specialized, but the type Vector is not.
   bool isSpecialized();

   /// Determine whether this type is a legal formal type.
   ///
   /// A type is illegal as a formal type if it is:
   ///   - an l-value type,
   ///   - a reference storage type,
   ///   - a metatype with a representation,
   ///   - a lowered function type (i.e. SILFunctionType),
   ///   - an optional whose object type is not a formal type, or
   ///   - a tuple type with an element that is not a formal type.
   ///
   /// These are the types of the Swift type system.
   bool isLegalFormalType();

   /// Check if this type is equal to the empty tuple type.
   bool isVoid();

   /// Check if this type is equal to Swift.Bool.
   bool isBool();

   /// Check if this type is equal to Builtin.IntN.
   bool isBuiltinIntegerType(unsigned bitWidth);

   /// If this is a class type or a bound generic class type, returns the
   /// (possibly generic) class.
   ClassDecl *getClassOrBoundGenericClass();

   /// If this is a struct type or a bound generic struct type, returns
   /// the (possibly generic) class.
   StructDecl *getStructOrBoundGenericStruct();

   /// If this is an enum or a bound generic enum type, returns the
   /// (possibly generic) enum.
   EnumDecl *getEnumOrBoundGenericEnum();

   /// Determine whether this type may have a superclass, which holds for
   /// classes, bound generic classes, and archetypes that are only instantiable
   /// with a class type.
   bool mayHaveSuperclass();

   /// Determine whether this type satisfies a class layout constraint, written
   /// `T: AnyObject` in the source.
   ///
   /// A class layout constraint is satisfied when we have a single retainable
   /// pointer as the representation, which includes:
   /// - @objc existentials
   /// - class constrained archetypes
   /// - classes
   bool satisfiesClassConstraint();

   /// Determine whether this type can be used as a base type for AST
   /// name lookup, which is the case for nominal types, protocol compositions
   /// and archetypes.
   ///
   /// Generally, the static vs instance and mutating vs nonmutating distinction
   /// is handled elsewhere, so metatypes, lvalue types and inout types are not
   /// allowed here.
   ///
   /// Similarly, tuples formally have members, but this does not go through
   /// name lookup.
   bool mayHaveMembers()
   {
      return (is<ArchetypeType>() ||
              isExistentialType() ||
              getAnyNominal());
   }

   /// Retrieve the superclass of this type.
   ///
   /// \param useArchetypes Whether to use context archetypes for outer generic
   /// parameters if the class is nested inside a generic function.
   ///
   /// \returns The superclass of this type, or a null type if it has no
   ///          superclass.
   Type getSuperclass(bool useArchetypes = true);

   /// True if this type is the exact superclass of another type.
   ///
   /// \param type       The potential subclass.
   ///
   /// \returns True if this type is \c type or a superclass of \c type.
   ///
   /// If this type is a bound generic class \c Foo<T>, the method only
   /// returns true if the generic parameters of \c type exactly match the
   /// superclass of \c type. For instance, if \c type is a
   /// class DerivedClass: Base<Int>, then \c Base<T> (where T is an archetype)
   /// will return false. `isBindableToSuperclassOf` should be used
   /// for queries that care whether a generic class type can be substituted into
   /// a type's subclass.
   bool isExactSuperclassOf(Type type);

   /// Get the substituted base class type, starting from a base class
   /// declaration and a substituted derived class type.
   ///
   /// For example, given the following declarations:
   ///
   /// class A<T, U> {}
   /// class B<V> : A<Int, V> {}
   /// class C<X, Y> : B<Y> {}
   ///
   /// Calling `C<String, NSObject>`->getSuperclassForDecl(`A`) will return
   /// `A<Int, NSObject>`.
   ///
   /// \param useArchetypes Whether to use context archetypes for outer generic
   /// parameters if the class is nested inside a generic function.
   Type getSuperclassForDecl(const ClassDecl *classDecl,
                             bool useArchetypes = true);

   /// True if this type is the superclass of another type, or a generic
   /// type that could be bound to the superclass.
   ///
   /// \param type       The potential subclass.
   ///
   /// \returns True if this type is \c type, a superclass of \c type, or an
   ///          archetype-parameterized type that can be bound to a superclass
   ///          of \c type.
   bool isBindableToSuperclassOf(Type type);

   /// True if this type contains archetypes that could be substituted with
   /// concrete types to form the argument type.
   bool isBindableTo(Type type);

   /// Determines whether this type is similar to \p other as defined by
   /// \p matchOptions.
   bool matches(Type other, TypeMatchOptions matchOptions);

   bool matchesParameter(Type other, TypeMatchOptions matchMode);

   /// Determines whether this function type is similar to \p
   /// other as defined by \p matchOptions and the callback \p
   /// paramsAndResultMatch which determines in a client-specific way
   /// whether the parameters and result of the types match.
   bool matchesFunctionType(Type other, TypeMatchOptions matchOptions,
                            FunctionRef<bool()> paramsAndResultMatch);

   /// Determines whether this type has a retainable pointer
   /// representation, i.e. whether it is representable as a single,
   /// possibly nil pointer that can be unknown-retained and
   /// unknown-released.
   bool hasRetainablePointerRepresentation();

   /// Given that this type is a reference type, which kind of reference
   /// counting does it use?
   ReferenceCounting getReferenceCounting();

   /// If this is a nominal type or a bound generic nominal type,
   /// returns the (possibly generic) nominal type declaration.
   NominalTypeDecl *getNominalOrBoundGenericNominal();

   /// If this is a nominal type, bound generic nominal type, or
   /// unbound generic nominal type, return the (possibly generic) nominal type
   /// declaration.
   NominalTypeDecl *getAnyNominal();

   /// Given that this is a nominal type or bound generic nominal
   /// type, return its parent type; this will be a null type if the type
   /// is not a nested type.
   Type getNominalParent();

   /// If this is a GenericType, bound generic nominal type, or
   /// unbound generic nominal type, return the (possibly generic) nominal type
   /// declaration.
   GenericTypeDecl *getAnyGeneric();

   /// removeArgumentLabels -  Retrieve a version of this type with all
   /// argument labels removed.
   Type removeArgumentLabels(unsigned numArgumentLabels);

   /// Retrieve the type without any parentheses around it.
   Type getWithoutParens();

   /// Replace the base type of the result type of the given function
   /// type with a new result type, as per a DynamicSelf or other
   /// covariant return transformation.  The optionality of the
   /// existing result will be preserved.
   ///
   /// \param newResultType The new result type.
   ///
   /// \param uncurryLevel The number of uncurry levels to apply before
   /// replacing the type. With uncurry level == 0, this simply
   /// replaces the current type with the new result type.
   Type replaceCovariantResultType(Type newResultType,
                                   unsigned uncurryLevel);

   /// Returns a new function type exactly like this one but with the self
   /// parameter replaced. Only makes sense for function members of types.
   Type replaceSelfParameterType(Type newSelf);

   /// getRValueType - For an @lvalue type, retrieves the underlying object type.
   /// Otherwise, returns the type itself.
   Type getRValueType();

   /// getInOutObjectType - For an inout type, retrieves the underlying object
   /// type.  Otherwise, returns the type itself.
   Type getInOutObjectType();

   /// getWithoutSpecifierType - For a non-materializable type
   /// e.g. @lvalue or inout, retrieves the underlying object type.
   /// Otherwise, returns the type itself.
   Type getWithoutSpecifierType();

   /// getMetatypeInstanceType - Looks through metatypes.
   Type getMetatypeInstanceType();

   /// Determine the set of substitutions that should be applied to a
   /// type spelled within the given DeclContext to treat it as a
   /// member of this type.
   ///
   /// For example, given:
   /// \code
   /// struct X<T, U> { }
   /// extension X {
   ///   typealias SomeArray = [T]
   /// }
   /// \endcode
   ///
   /// Asking for the member substitutions of \c X<Int,String> within
   /// the context of the extension above will produce substitutions T
   /// -> Int and U -> String suitable for mapping the type of
   /// \c SomeArray.
   ///
   /// \param genericEnv If non-null and the type is nested inside of a
   /// generic function, generic parameters of the outer context are
   /// mapped to context archetypes of this generic environment.
   SubstitutionMap getContextSubstitutionMap(ModuleDecl *module,
                                             const DeclContext *dc,
                                             GenericEnvironment *genericEnv = nullptr);

   /// Deprecated version of the above.
   TypeSubstitutionMap getContextSubstitutions(const DeclContext *dc,
                                               GenericEnvironment *genericEnv = nullptr);

   /// Get the substitutions to apply to the type of the given member as seen
   /// from this base type.
   ///
   /// \param genericEnv If non-null, generic parameters of the member are
   /// mapped to context archetypes of this generic environment.
   SubstitutionMap getMemberSubstitutionMap(ModuleDecl *module,
                                            const ValueDecl *member,
                                            GenericEnvironment *genericEnv = nullptr);

   /// Deprecated version of the above.
   TypeSubstitutionMap getMemberSubstitutions(const ValueDecl *member,
                                              GenericEnvironment *genericEnv = nullptr);

   /// Retrieve the type of the given member as seen through the given base
   /// type, substituting generic arguments where necessary.
   ///
   /// This routine allows one to take a concrete type (the "this" type) and
   /// and a member of that type (or one of its superclasses), then determine
   /// what type an access to that member through the base type will have.
   /// For example, given:
   ///
   /// \code
   /// class Vector<T> {
   ///   func add(value : T) { }
   /// }
   /// \endcode
   ///
   /// Given the type \c Vector<Int> and the member \c add, the resulting type
   /// of the member will be \c (self : Vector<Int>) -> (value : Int) -> ().
   ///
   /// \param module The module in which the substitution occurs.
   ///
   /// \param member The member whose type we are substituting.
   ///
   /// \param memberType The type of the member, in which archetypes will be
   /// replaced by the generic arguments provided by the base type. If null,
   /// the member's type will be used.
   ///
   /// \returns the resulting member type.
   Type getTypeOfMember(ModuleDecl *module, const ValueDecl *member,
                        Type memberType = Type());

   /// Get the type of a superclass member as seen from the subclass,
   /// substituting generic parameters, dynamic Self return, and the
   /// 'self' argument type as appropriate.
   Type adjustSuperclassMemberDeclType(const ValueDecl *baseDecl,
                                       const ValueDecl *derivedDecl,
                                       Type memberType);

   /// Return T if this type is Optional<T>; otherwise, return the null type.
   Type getOptionalObjectType();

   // Return type underlying type of a swift_newtype annotated imported struct;
   // otherwise, return the null type.
   Type getSwiftNewtypeUnderlyingType();

   /// Return the type T after looking through all of the optional
   /// types.
   Type lookThroughAllOptionalTypes();

   /// Return the type T after looking through all of the optional
   /// types.
   Type lookThroughAllOptionalTypes(SmallVectorImpl<Type> &optionals);

   /// Whether this is the AnyObject type.
   bool isAnyObject();

   /// Whether this is an existential composition containing
   /// Error.
   bool isExistentialWithError();

   void dump() const POLAR_ATTRIBUTE_USED;
   void dump(RawOutStream &outStream, unsigned indent = 0) const;

   void dumpPrint() const POLAR_ATTRIBUTE_USED;
   void print(RawOutStream &outStream,
              const PrintOptions &printOptions = PrintOptions()) const;
   void print(AstPrinter &printer, const PrintOptions &printOptions) const;

   /// Does this type have grammatically simple syntax?
   bool hasSimpleTypeRepr() const;

   /// Return the name of the type as a string, for use in diagnostics only.
   std::string getString(const PrintOptions &printOptions = PrintOptions()) const;

   /// Return the name of the type, adding parens in cases where
   /// appending or prepending text to the result would cause that text
   /// to be appended to only a portion of the returned type. For
   /// example for a function type "Int -> Float", adding text after
   /// the type would make it appear that it's appended to "Float" as
   /// opposed to the entire type.
   std::string
         getStringAsComponent(const PrintOptions &printOptions = PrintOptions()) const;

   /// Return whether this type is or can be substituted for a bridgeable
   /// object type.
   TypeTraitResult canBeClass();

   private:
   // Make vanilla new/delete illegal for Types.
   void *operator new(size_t bytes) throw() = delete;
   void operator delete(void *data) throw() = delete;
   public:
   // Only allow allocation of Types using the allocator in AstContext
   // or by doing a placement new.
   void *operator new(size_t bytes, const AstContext &ctx,
                      AllocationArena arena, unsigned alignment = 8);
   void *operator new(size_t bytes, void *mem) throw()
   {
      return mem;
   }
};

/// AnyGenericType - This abstract class helps types ensure that fields
/// exist at the same offset in memory to improve code generation of the
/// compiler itself.
class AnyGenericType : public TypeBase
{
private:
   friend class NominalOrBoundGenericNominalType;

   /// theDecl - This is the TypeDecl which declares the given type. It
   /// specifies the name and other useful information about this type.
   union {
      GenericTypeDecl *m_genDecl;
      NominalTypeDecl *m_nomDecl;
   };

   /// The type of the parent, in which this type is nested.
   Type m_parent;

   template <typename... Args>
   AnyGenericType(NominalTypeDecl *theDecl, Type parent, Args &&...args)
      : TypeBase(std::forward<Args>(args)...),
        m_nomDecl(theDecl),
        m_parent(parent)
   {}

protected:
   template <typename... Args>
   AnyGenericType(GenericTypeDecl *theDecl, Type parent, Args &&...args)
      : TypeBase(std::forward<Args>(args)...),
        m_genDecl(theDecl),
        m_parent(parent)
   {}

public:

   /// Returns the declaration that declares this type.
   GenericTypeDecl *getDecl() const
   {
      return m_genDecl;
   }

   /// Returns the type of the parent of this type. This will
   /// be null for top-level types or local types, and for non-generic types
   /// will simply be the same as the declared type of the declaration context
   /// of theDecl. For types nested within generic types, however, this will
   /// involve \c BoundGenericType nodes that provide context for the nested
   /// type, e.g., the type Dictionary<String, Int>.ItemRange would be
   /// represented as a NominalType with Dictionary<String, Int> as its parent
   /// type.
   Type getParent() const
   {
      return m_parent;
   }

   // Implement isa/cast/dyncast/etc.
   static bool classof(const TypeBase *type)
   {
      return type->getKind() >= TypeKind::First_AnyGenericType &&
            type->getKind() <= TypeKind::Last_AnyGenericType;
   }
};

BEGIN_CAN_TYPE_WRAPPER(AnyGenericType, Type)
PROXY_CAN_TYPE_SIMPLE_GETTER(getParent)

END_CAN_TYPE_WRAPPER(AnyGenericType, Type);

/// NominalOrBoundGenericNominal - This abstract class helps types ensure that
/// fields exist at the same offset in memory to improve code generation of the
/// compiler itself.
class NominalOrBoundGenericNominalType : public AnyGenericType
{
public:
   template <typename... Args>
   NominalOrBoundGenericNominalType(Args &&...args)
      : AnyGenericType(std::forward<Args>(args)...)
   {}

   /// Returns the declaration that declares this type.
   NominalTypeDecl *getDecl() const
   {
      return m_nomDecl;
   }

   // Implement isa/cast/dyncast/etc.
   static bool classof(const TypeBase *type)
   {
      return type->getKind() >= TypeKind::First_NominalOrBoundGenericNominalType &&
            type->getKind() <= TypeKind::Last_NominalOrBoundGenericNominalType;
   }
};
DEFINE_EMPTY_CAN_TYPE_WRAPPER(NominalOrBoundGenericNominalType, AnyGenericType);

/// ErrorType - This represents a type that was erroneously constructed.  This
/// is produced when parsing types and when name binding type aliases, and is
/// installed in declaration that use these erroneous types.  All uses of a
/// declaration of invalid type should be ignored and not re-diagnosed.
class ErrorType final : public TypeBase
{
   friend class AstContext;
   // The Error type is always canonical.
   ErrorType(AstContext &context, Type originalType,
             RecursiveTypeProperties properties)
      : TypeBase(TypeKind::Error, &context, properties)
   {
      assert(properties.hasError());
      if (originalType) {
         m_bits.ErrorType.HasOriginalType = true;
         *reinterpret_cast<Type *>(this + 1) = originalType;
      } else {
         m_bits.ErrorType.HasOriginalType = false;
      }
   }

public:
   static Type get(const AstContext &context);

   /// Produce an error type which records the original type we were trying to
   /// substitute when we ran into a problem.
   static Type get(Type originalType);

   /// Retrieve the original type that this error type replaces, or none if
   /// there is no such type.
   Type getOriginalType() const
   {
      if (m_bits.ErrorType.HasOriginalType) {
         return *reinterpret_cast<const Type *>(this + 1);
      }
      return Type();
   }

   // Implement isa/cast/dyncast/etc.
   static bool classof(const TypeBase *type)
   {
      return type->getKind() == TypeKind::Error;
   }
};

DEFINE_EMPTY_CAN_TYPE_WRAPPER(ErrorType, Type);

/// UnresolvedType - This represents a type variable that cannot be resolved to
/// a concrete type because the expression is ambiguous.  This is produced when
/// parsing expressions and producing diagnostics.  Any instance of this should
/// cause the entire expression to be ambiguously typed.
class UnresolvedType : public TypeBase
{
   friend class AstContext;
   // The Unresolved type is always canonical.
   UnresolvedType(AstContext &C)
      : TypeBase(TypeKind::Unresolved, &C,
                 RecursiveTypeProperties(RecursiveTypeProperties::HasUnresolvedType))
   {}

public:
   // Implement isa/cast/dyncast/etc.
   static bool classof(const TypeBase *type)
   {
      return type->getKind() == TypeKind::Unresolved;
   }
};

DEFINE_EMPTY_CAN_TYPE_WRAPPER(UnresolvedType, Type);


/// BuiltinType - An abstract class for all the builtin types.
class BuiltinType : public TypeBase
{
protected:
   BuiltinType(TypeKind kind, const AstContext &canTypeCtx)
      : TypeBase(kind, &canTypeCtx, RecursiveTypeProperties())
   {}

public:
   static bool classof(const TypeBase *type)
   {
      return type->getKind() >= TypeKind::First_BuiltinType &&
            type->getKind() <= TypeKind::Last_BuiltinType;
   }
};

DEFINE_EMPTY_CAN_TYPE_WRAPPER(BuiltinType, Type);


// TODO: As part of AST modernization, replace with a proper
// 'ParameterTypeElt' or similar, and have FunctionTypes only have a list
// of 'ParameterTypeElt's. Then, this information can be removed from
// TupleTypeElt.
//
/// Provide parameter type relevant flags, i.e. variadic, autoclosure, and
/// escaping.
class ParameterTypeFlags
{
   enum ParameterFlags : uint8_t
   {
      None        = 0,
      Variadic    = 1 << 0,
      AutoClosure = 1 << 1,
      Escaping    = 1 << 2,
      OwnershipShift = 3,
      Ownership   = 7 << OwnershipShift,

      NumBits = 6
   };
   OptionSet<ParameterFlags> m_value;
   static_assert(NumBits < 8*sizeof(OptionSet<ParameterFlags>), "overflowed");

   ParameterTypeFlags(OptionSet<ParameterFlags, uint8_t> val)
      : m_value(val)
   {}

public:
   ParameterTypeFlags() = default;
   static ParameterTypeFlags fromRaw(uint8_t raw)
   {
      return ParameterTypeFlags(OptionSet<ParameterFlags>(raw));
   }

   ParameterTypeFlags(bool variadic, bool autoclosure, bool escaping)
      : m_value((variadic ? Variadic : 0) | (autoclosure ? AutoClosure : 0) |
                (escaping ? Escaping : 0))
   {}

   /// Create one from what's present in the parameter type
   inline static ParameterTypeFlags
   fromParameterType(Type paramTy, bool isVariadic, bool isAutoClosure);

   bool isNone() const
   {
      return !m_value;
   }

   bool isVariadic() const
   {
      return m_value.contains(Variadic);
   }

   bool isAutoClosure() const
   {
      return m_value.contains(AutoClosure);
   }

   bool isEscaping() const
   {
      return m_value.contains(Escaping);
   }

   ParameterTypeFlags withVariadic(bool variadic) const
   {
      return ParameterTypeFlags(variadic ? m_value | ParameterTypeFlags::Variadic
                                         : m_value - ParameterTypeFlags::Variadic);
   }

   ParameterTypeFlags withEscaping(bool escaping) const
   {
      return ParameterTypeFlags(escaping ? m_value | ParameterTypeFlags::Escaping
                                         : m_value - ParameterTypeFlags::Escaping);
   }



   ParameterTypeFlags withAutoClosure(bool isAutoClosure) const
   {
      return ParameterTypeFlags(isAutoClosure
                                ? m_value | ParameterTypeFlags::AutoClosure
                                : m_value - ParameterTypeFlags::AutoClosure);
   }

   bool operator ==(const ParameterTypeFlags &other) const
   {
      return m_value.toRaw() == other.m_value.toRaw();
   }

   bool operator!=(const ParameterTypeFlags &other) const
   {
      return m_value.toRaw() != other.m_value.toRaw();
   }

   uint8_t toRaw() const
   {
      return m_value.toRaw();
   }
};

/// The representation form of a function.
enum class FunctionTypeRepresentation : uint8_t
{
   /// A "thick" function that carries a context pointer to reference captured
   /// state. The default native function representation.
   Swift = 0,

   /// A thick function that is represented as an Objective-C block.
   Block,

   /// A "thin" function that needs no context.
   Thin,

   /// A C function pointer, which is thin and also uses the C calling
   /// convention.
   CFunctionPointer,

   /// The value of the greatest AST function representation.
   Last = CFunctionPointer,
};

/// AnyFunctionType - A function type has zero or more input parameters and a
/// single result. The result type may be a tuple. For example:
///   "(int) -> int" or "(a : int, b : int) -> (int, int)".
///
/// There are two kinds of function types:  monomorphic (FunctionType) and
/// polymorphic (GenericFunctionType). Both type families additionally can
/// be 'thin', indicating that a function value has no capture context and can be
/// represented at the binary level as a single function pointer.
class AnyFunctionType : public TypeBase
{
   const Type m_output;

public:
   using Representation = FunctionTypeRepresentation;

   class Param {
   public:
      explicit Param(Type t,
                     Identifier l = Identifier(),
                     ParameterTypeFlags f = ParameterTypeFlags())
         : m_type(t),
           m_label(l),
           m_flags(f)
      {
         assert(!t || !t->is<InOutType>() && "set flags instead");
      }

   private:
      /// The type of the parameter. For a variadic parameter, this is the
      /// element type.
      Type m_type;

      // The label associated with the parameter, if any.
      Identifier m_label;

      /// Parameter specific flags.
      ParameterTypeFlags m_flags =
      {};

   public:
      /// FIXME: Remove this. Return the formal type of the parameter in the
      /// function type, including the InOutType if there is one.
      ///
      /// For example, 'inout Int' => 'inout Int', 'Int...' => 'Int'.
      Type getOldType() const;

      /// Return the formal type of the parameter.
      ///
      /// For example, 'inout Int' => 'Int', 'Int...' => 'Int'.
      Type getPlainType() const
      {
         return m_type;
      }

      /// The type of the parameter when referenced inside the function body
      /// as an rvalue.
      ///
      /// For example, 'inout Int' => 'Int', 'Int...' => '[Int]'.
      Type getParameterType(bool forCanonical = false,
                            AstContext *ctx = nullptr) const;

      bool hasLabel() const
      {
         return !m_label.empty();
      }

      Identifier getLabel() const
      {
         return m_label;
      }

      ParameterTypeFlags getParameterFlags() const
      {
         return m_flags;
      }

      /// Whether the parameter is varargs
      bool isVariadic() const
      {
         return m_flags.isVariadic();
      }

      /// Whether the parameter is marked '@autoclosure'
      bool isAutoClosure() const
      {
         return m_flags.isAutoClosure();
      }

      /// Whether the parameter is marked '@escaping'
      bool isEscaping() const
      {
         return m_flags.isEscaping();
      }

      bool operator==(Param const &b) const
      {
         return (m_label == b.m_label &&
                 getPlainType()->isEqual(b.getPlainType()) &&
                 m_flags == b.m_flags);
      }
      bool operator!=(Param const &b) const
      {
         return !(*this == b);
      }

      Param getWithoutLabel() const
      {
         return Param(m_type, Identifier(), m_flags);
      }
   };

   class CanParam : public Param
   {
      explicit CanParam(const Param &param)
         : Param(param)
      {}
   public:
      static CanParam getFromParam(const Param &param)
      {
         return CanParam(param);
      }

      CanType getOldType() const
      {
         return CanType(Param::getOldType());
      }

      CanType getPlainType() const
      {
         return CanType(Param::getPlainType());
      }

      CanType getParameterType() const
      {
         return CanType(Param::getParameterType(/*forCanonical*/ true));
      }
   };

   using CanParamArrayRef =
   ArrayRefView<Param,CanParam,CanParam::getFromParam,/*AccessOriginal*/true>;

   /// A class which abstracts out some details necessary for
   /// making a call.
   class ExtInfo
   {
      // If bits are added or removed, then TypeBase::AnyFunctionTypeBits
      // and NumMaskBits must be updated, and they must match.
      //
      //   |representation|noEscape|throws|
      //   |    0 .. 3    |    4   |   5  |
      //
      enum : unsigned
      {
         RepresentationMask     = 0xF << 0,
         NoEscapeMask           = 1 << 4,
         ThrowsMask             = 1 << 5,
         NumMaskBits            = 6
      };

      unsigned m_bits; // Naturally sized for speed.

      ExtInfo(unsigned m_bits) : m_bits(m_bits) {}

      friend class AnyFunctionType;

   public:
      // Constructor with all defaults.
      ExtInfo()
         : m_bits(0)
      {
         assert(getRepresentation() == Representation::Swift);
      }

      // Constructor for polymorphic type.
      ExtInfo(Representation Rep, bool throws)
      {
         m_bits = ((unsigned) Rep) | (throws ? ThrowsMask : 0);
      }

      // Constructor with no defaults.
      ExtInfo(Representation Rep,
              bool IsNoEscape,
              bool throws)
         : ExtInfo(Rep, throws)
      {
         m_bits |= (IsNoEscape ? NoEscapeMask : 0);
      }

      bool isNoEscape() const
      {
         return m_bits & NoEscapeMask;
      }

      bool throws() const
      {
         return m_bits & ThrowsMask;
      }

      Representation getRepresentation() const
      {
         unsigned rawRep = m_bits & RepresentationMask;
         assert(rawRep <= unsigned(Representation::Last)
                && "unexpected SIL representation");
         return Representation(rawRep);
      }

      // Note that we don't have setters. That is by design, use
      // the following with methods instead of mutating these objects.
      POLAR_NODISCARD
      ExtInfo withRepresentation(Representation Rep) const
      {
         return ExtInfo((m_bits & ~RepresentationMask)
                        | (unsigned)Rep);
      }

      POLAR_NODISCARD
      ExtInfo withNoEscape(bool NoEscape = true) const
      {
         if (NoEscape) {
            return ExtInfo(m_bits | NoEscapeMask);
         } else {
            return ExtInfo(m_bits & ~NoEscapeMask);
         }
      }

      POLAR_NODISCARD
      ExtInfo withThrows(bool throws = true) const
      {
         if (throws) {
            return ExtInfo(m_bits | ThrowsMask);
         } else {
            return ExtInfo(m_bits & ~ThrowsMask);
         }
      }

      unsigned getFuncAttrKey() const
      {
         return m_bits;
      }

      bool operator==(ExtInfo other) const
      {
         return m_bits == other.m_bits;
      }
      bool operator!=(ExtInfo other) const
      {
         return m_bits != other.m_bits;
      }
   };

protected:
   AnyFunctionType(TypeKind kind, const AstContext *canTypeContext,
                   Type output, RecursiveTypeProperties properties,
                   unsigned numParams, ExtInfo info)
      : TypeBase(kind, canTypeContext, properties),
        m_output(output)
   {
      m_bits.AnyFunctionType.ExtInfo = info.m_bits;
      m_bits.AnyFunctionType.numParams = numParams;
      assert(m_bits.AnyFunctionType.numParams == numParams && "Params dropped!");
      // The use of both assert() and static_assert() is intentional.
      assert(m_bits.AnyFunctionType.ExtInfo == info.m_bits && "m_bits were dropped!");
      static_assert(ExtInfo::NumMaskBits == NumAFTExtInfoBits,
                    "ExtInfo and AnyFunctionTypeBitfields must agree on bit size");
   }

public:
   /// Break an input type into an array of \c AnyFunctionType::Params.
   static void decomposeInput(Type type,
                              SmallVectorImpl<Param> &result);

   /// Take an array of parameters and turn it into an input type.
   ///
   /// The result type is only there as a way to extract the AstContext when
   /// needed.
   static Type composeInput(AstContext &ctx, ArrayRef<Param> params,
                            bool canonicalVararg);
   static Type composeInput(AstContext &ctx, CanParamArrayRef params,
                            bool canonicalVararg) {
      return composeInput(ctx, params.getOriginalArray(), canonicalVararg);
   }

   /// Given two arrays of parameters determine if they are equal.
   static bool equalParams(ArrayRef<Param> a, ArrayRef<Param> b);

   /// Given two arrays of parameters determine if they are equal.
   static bool equalParams(CanParamArrayRef a, CanParamArrayRef b);

   /// Given an array of parameters and an array of labels of the
   /// same length, update each parameter to have the corresponding label.
   static void relabelParams(MutableArrayRef<Param> params,
                             ArrayRef<Identifier> labels);

   Type getResult() const
   {
      return m_output;
   }

   ArrayRef<Param> getParams() const;
   unsigned getNumParams() const
   {
      return m_bits.AnyFunctionType.numParams;
   }

   GenericSignature *getOptGenericSignature() const;

   ExtInfo getExtInfo() const
   {
      return ExtInfo(m_bits.AnyFunctionType.ExtInfo);
   }

   /// Get the representation of the function type.
   Representation getRepresentation() const
   {
      return getExtInfo().getRepresentation();
   }

   /// True if the parameter declaration it is attached to is guaranteed
   /// to not persist the closure for longer than the duration of the call.
   bool isNoEscape() const
   {
      return getExtInfo().isNoEscape();
   }

   bool throws() const
   {
      return getExtInfo().throws();
   }

   /// Returns a new function type exactly like this one but with the ExtInfo
   /// replaced.
   AnyFunctionType *withExtInfo(ExtInfo info) const;

   void printParams(RawOutStream &outStream,
                    const PrintOptions &printOpts = PrintOptions()) const;
   void printParams(AstPrinter &printer, const PrintOptions &printOpts) const;

   // Implement isa/cast/dyncast/etc.
   static bool classof(const TypeBase *T)
   {
      return T->getKind() >= TypeKind::First_AnyFunctionType &&
            T->getKind() <= TypeKind::Last_AnyFunctionType;
   }
};

BEGIN_CAN_TYPE_WRAPPER(AnyFunctionType, Type)
using ExtInfo = AnyFunctionType::ExtInfo;
using CanParamArrayRef = AnyFunctionType::CanParamArrayRef;

static CanAnyFunctionType get(CanGenericSignature signature,
                              CanParamArrayRef params,
                              CanType result,
                              ExtInfo info = ExtInfo());

CanGenericSignature getOptGenericSignature() const;

CanParamArrayRef getParams() const
{
   return CanParamArrayRef(getPointer()->getParams());
}

PROXY_CAN_TYPE_SIMPLE_GETTER(getResult)

CanAnyFunctionType withExtInfo(ExtInfo info) const {
   return CanAnyFunctionType(getPointer()->withExtInfo(info));
}
END_CAN_TYPE_WRAPPER(AnyFunctionType, Type);


/// FunctionType - A monomorphic function type, specified with an arrow.
///
/// For example:
///   let x : (Float, Int) -> Int
class FunctionType final : public AnyFunctionType,
      public FoldingSetNode,
      private TrailingObjects<FunctionType, AnyFunctionType::Param>
{
   friend class TrailingObjects;

public:
   /// 'Constructor' Factory Function
   static FunctionType *get(ArrayRef<Param> params, Type result,
                            ExtInfo info = ExtInfo());

   // Retrieve the input parameters of this function type.
   ArrayRef<Param> getParams() const {
      return {getTrailingObjects<Param>(), getNumParams()};
   }

   void Profile(FoldingSetNodeId &id)
   {
      Profile(id, getParams(), getResult(), getExtInfo());
   }

   static void Profile(FoldingSetNodeId &id,
                       ArrayRef<Param> params,
                       Type result,
                       ExtInfo info);

   // Implement isa/cast/dyncast/etc.
   static bool classof(const TypeBase *T)
   {
      return T->getKind() == TypeKind::Function;
   }

private:
   FunctionType(ArrayRef<Param> params, Type result, ExtInfo info,
                const AstContext *ctx, RecursiveTypeProperties properties);
};

BEGIN_CAN_TYPE_WRAPPER(FunctionType, AnyFunctionType)
static CanFunctionType get(CanParamArrayRef params, CanType result,
                           ExtInfo info = ExtInfo())
{
   auto fnType = FunctionType::get(params.getOriginalArray(), result, info);
   return cast<FunctionType>(fnType->getCanonicalType());
}

CanFunctionType with_ext_info(ExtInfo info) const
{
   //   return CanFunctionType(cast<FunctionType>(getPointer()->withExtInfo(info)));
}

END_CAN_TYPE_WRAPPER(FunctionType, AnyFunctionType);

/// Map the given parameter list onto a bitvector describing whether
/// the argument type at each index has a default argument associated with
/// it.
SmallBitVector compute_default_map(ArrayRef<AnyFunctionType::Param> params,
                                   const ValueDecl *paramOwner, bool skipCurriedSelf);

/// Turn a param list into a symbolic and printable representation that does not
/// include the types, something like (: , b:, c:)
std::string get_param_list_as_string(ArrayRef<AnyFunctionType::Param> parameters);

/// Describes a generic function type.
///
/// A generic function type describes a function that is polymorphic with
/// respect to some set of generic parameters and the requirements placed
/// on those parameters and dependent member types thereof. The input and
/// output types of the generic function can be expressed in terms of those
/// generic parameters.
class GenericFunctionType final : public AnyFunctionType,
      public FoldingSetNode,
      private TrailingObjects<GenericFunctionType, AnyFunctionType::Param>
{
   friend class TrailingObjects;

   GenericSignature *Signature;

   /// Construct a new generic function type.
   GenericFunctionType(GenericSignature *sig,
                       ArrayRef<Param> params,
                       Type result,
                       ExtInfo info,
                       const AstContext *ctx,
                       RecursiveTypeProperties properties);

public:
   /// Create a new generic function type.
   static GenericFunctionType *get(GenericSignature *sig,
                                   ArrayRef<Param> params,
                                   Type result,
                                   ExtInfo info = ExtInfo());

   // Retrieve the input parameters of this function type.
   ArrayRef<Param> getParams() const {
      return {getTrailingObjects<Param>(), getNumParams()};
   }

   /// Retrieve the generic signature of this function type.
   GenericSignature *getGenericSignature() const {
      return Signature;
   }

   /// Retrieve the generic parameters of this polymorphic function type.
   TypeArrayView<GenericTypeParamType> getGenericParams() const;

   /// Retrieve the requirements of this polymorphic function type.
   ArrayRef<Requirement> getRequirements() const;

   /// Substitute the given generic arguments into this generic
   /// function type and return the resulting non-generic type.
   FunctionType *substGenericArgs(SubstitutionMap subs);

   void Profile(FoldingSetNodeId &id)
   {
      Profile(id, getGenericSignature(), getParams(), getResult(),
              getExtInfo());
   }

   static void Profile(FoldingSetNodeId &id,
                       GenericSignature *sig,
                       ArrayRef<Param> params,
                       Type result,
                       ExtInfo info);

   // Implement isa/cast/dyncast/etc.
   static bool classof(const TypeBase *T)
   {
      return T->getKind() == TypeKind::GenericFunction;
   }
};

BEGIN_CAN_TYPE_WRAPPER(GenericFunctionType, AnyFunctionType)
/// Create a new generic function type.
static CanGenericFunctionType get(CanGenericSignature sig,
                                  CanParamArrayRef params,
                                  CanType result,
                                  ExtInfo info = ExtInfo())
{
   // Knowing that the argument types are independently canonical is
   // not sufficient to guarantee that the function type will be canonical.
   auto fnType = GenericFunctionType::get(sig, params.getOriginalArray(),
                                          result, info);
   return cast<GenericFunctionType>(fnType->getCanonicalType());
}

CanFunctionType substGenericArgs(SubstitutionMap subs) const;

CanGenericSignature getGenericSignature() const
{
   return CanGenericSignature(getPointer()->getGenericSignature());
}

ArrayRef<CanTypeWrapper<GenericTypeParamType>> getGenericParams() const
{
   return getGenericSignature().getGenericParams();
}

CanGenericFunctionType withExtInfo(ExtInfo info) const
{
//   return CanGenericFunctionType(
//            cast<GenericFunctionType>(getPointer()->withExtInfo(info)));
}
END_CAN_TYPE_WRAPPER(GenericFunctionType, AnyFunctionType);

//inline CanAnyFunctionType
//CanAnyFunctionType::get(CanGenericSignature signature, CanParamArrayRef params,
//                        CanType result, ExtInfo extInfo) {
//   if (signature) {
//      return CanGenericFunctionType::get(signature, params, result, extInfo);
//   } else {
//      return CanFunctionType::get(params, result, extInfo);
//   }
//}

//inline GenericSignature *AnyFunctionType::getOptGenericSignature() const
//{
//   if (auto genericFn = dyn_cast<GenericFunctionType>(this)) {
//      return genericFn->getGenericSignature();
//   } else {
//      return nullptr;
//   }
//}

//inline CanGenericSignature CanAnyFunctionType::getOptGenericSignature() const {
//   if (auto genericFn = dyn_cast<GenericFunctionType>(*this)) {
//      return genericFn.getGenericSignature();
//   } else {
//      return CanGenericSignature();
//   }
//}


} // polar::ast

#endif // POLARPHP_AST_TYPES_H
