//===--- Type.h - Swift Language Type ASTs ----------------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/25.
//
//===----------------------------------------------------------------------===//
//
// This file defines the Type class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_TYPE_H
#define POLARPHP_AST_TYPE_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/ArrayRefView.h"
#include "polarphp/basic/adt/OptionSet.h"
#include "polarphp/utils/Casting.h"
#include "polarphp/ast/PrintOptions.h"
#include "polarphp/ast/TypeAlignments.h"

#include <functional>
#include <string>
#include <functional>
#include <string>
#include <optional>

/// forward declare class with namespace
namespace polar::utils {
class RawOutStream;
}

namespace polar::ast {

using polar::basic::ArrayRef;
using polar::basic::DenseMap;
using polar::basic::FunctionRef;
using polar::basic::OptionSet;
using polar::basic::ArrayRefView;
using polar::utils::RawOutStream;

/// forward declare class
class AstPrinter;
class ArchetypeType;
class ClassDecl;
class CanType;
class EnumDecl;
class GenericSignature;
class LazyResolver;
class ModuleDecl;
class NominalTypeDecl;
class GenericTypeDecl;
class NormalProtocolConformance;
class ProtocolConformanceRef;
class ProtocolDecl;
class ProtocolType;
class StructDecl;
class SubstitutableType;
class SubstitutionMap;
class TypeBase;
class Type;
class TypeWalker;

/// Type substitution mapping from substitutable types to their
/// replacements.
using TypeSubstitutionMap = DenseMap<SubstitutableType *, Type>;

/// Function used to provide substitutions.
///
/// Returns a null \c Type to indicate that there is no substitution for
/// this substitutable type; otherwise, the replacement type.
using TypeSubstitutionFunc = FunctionRef<Type(SubstitutableType *dependentType)>;

/// A function object suitable for use as a \c TypeSubstitutionFn that
/// replaces archetypes with their interface types.
struct MapTypeOutOfContext
{
   Type operator()(SubstitutableType *type) const;
};

/// A function object suitable for use as a \c TypeSubstitutionFn that
/// queries an underlying \c TypeSubstitutionMap.
struct QueryTypeSubstitutionMap
{
   const TypeSubstitutionMap &substitutions;
   Type operator()(SubstitutableType *type) const;
};

/// A function object suitable for use as a \c TypeSubstitutionFn that
/// queries an underlying \c TypeSubstitutionMap, or returns the original type
/// if no match was found.
struct QueryTypeSubstitutionMapOrIdentity
{
   const TypeSubstitutionMap &substitutions;

   Type operator()(SubstitutableType *type) const;
};

/// Function used to resolve conformances.
using GenericFunction = auto(CanType dependentType,
Type conformingReplacementType,
ProtocolDecl *conformedProtocol) -> std::optional<ProtocolConformanceRef>;
using LookupConformanceFunc = FunctionRef<GenericFunction>;

/// Functor class suitable for use as a \c LookupConformanceFn that provides
/// only abstract conformances for generic types. Asserts that the replacement
/// type is an opaque generic type.
class MakeAbstractConformanceForGenericType {
public:
   std::optional<ProtocolConformanceRef>
   operator()(CanType dependentType,
              Type conformingReplacementType,
              ProtocolDecl *conformedProtocol) const;
};

/// Functor class suitable for use as a \c LookupConformanceFn that fetches
/// conformances from a generic signature.
class LookUpConformanceInSignature {
   const GenericSignature &Sig;
public:
   LookUpConformanceInSignature(const GenericSignature &Sig)
      : Sig(Sig) {}

   std::optional<ProtocolConformanceRef>
   operator()(CanType dependentType,
              Type conformingReplacementType,
              ProtocolDecl *conformedProtocol) const;
};

/// Flags that can be passed when substituting into a type.
enum class SubstFlags
{
   /// If a type cannot be produced because some member type is
   /// missing, place an 'error' type into the position of the base.
   UseErrorType = 0x01,
   /// Allow substitutions to recurse into SILFunctionTypes.
   /// Normally, SILType::subst() should be used for lowered
   /// types, however in special cases where the substitution
   /// is just changing between contextual and interface type
   /// representations, using Type::subst() is allowed.
   AllowLoweredTypes = 0x02,
   /// Map member types to their desugared witness type.
   DesugarMemberTypes = 0x04,
};

/// Options for performing substitutions into a type.
struct SubstOptions : public OptionSet<SubstFlags>
{
   // Note: The unfortunate use of TypeBase * here, rather than Type,
   // is due to a libc++ quirk that requires the result type to be
   // complete.
   typedef std::function<TypeBase *(const NormalProtocolConformance *,
                                    AssociatedTypeDecl *)>
   GetTentativeTypeWitness;

   /// Function that retrieves a tentative type witness for a protocol
   /// conformance with the state \c CheckingTypeWitnesses.
   GetTentativeTypeWitness getTentativeTypeWitness;

   SubstOptions(std::nullopt_t)
      : OptionSet(std::nullopt)
   {}

   SubstOptions(SubstFlags flags)
      : OptionSet(flags)
   {}

   SubstOptions(OptionSet<SubstFlags> options)
      : OptionSet(options)
   {}
};

inline SubstOptions operator|(SubstFlags lhs, SubstFlags rhs)
{
   return SubstOptions(lhs) | rhs;
}

/// Type - This is a simple value object that contains a pointer to a type
/// class.  This is potentially sugared.  We use this throughout the codebase
/// instead of a raw "TypeBase*" to disable equality comparison, which is unsafe
/// for sugared types.
class Type
{
   TypeBase *m_ptr;
public:
   /*implicit*/ Type(TypeBase *ptr = 0)
      : m_ptr(ptr)
   {}

   TypeBase *getPointer() const
   {
      return m_ptr;
   }

   bool isNull() const
   {
      return m_ptr == 0;
   }

   TypeBase *operator->() const
   {
      return m_ptr;
   }

   explicit operator bool() const
   {
      return m_ptr != 0;
   }

   /// Walk this type.
   ///
   /// Returns true if the walk was aborted.
   bool walk(TypeWalker &walker) const;
   bool walk(TypeWalker &&walker) const
   {
      return walk(walker);
   }

   /// Look through the given type and its children to find a type for
   /// which the given predicate returns true.
   ///
   /// \param pred A predicate function object. It should return true if the give
   /// type node satisfies the criteria.
   ///
   /// \returns true if the predicate returns true for the given type or any of
   /// its children.
   bool findIf(FunctionRef<bool(Type)> pred) const;

   /// Transform the given type by applying the user-provided function to
   /// each type.
   ///
   /// This routine applies the given function to transform one type into
   /// another. If the function leaves the type unchanged, recurse into the
   /// child type nodes and transform those. If any child type node changes,
   /// the parent type node will be rebuilt.
   ///
   /// If at any time the function returns a null type, the null will be
   /// propagated out.
   ///
   /// \param fn A function object with the signature \c Type(Type), which
   /// accepts a type and returns either a transformed type or a null type.
   ///
   /// \returns the result of transforming the type.
   Type transform(FunctionRef<Type(Type)> func) const;

   /// Transform the given type by applying the user-provided function to
   /// each type.
   ///
   /// This routine applies the given function to transform one type into
   /// another. If the function leaves the type unchanged, recurse into the
   /// child type nodes and transform those. If any child type node changes,
   /// the parent type node will be rebuilt.
   ///
   /// If at any time the function returns a null type, the null will be
   /// propagated out.
   ///
   /// If the function returns \c None, the transform operation will
   ///
   /// \param fn A function object with the signature
   /// \c Optional<Type>(TypeBase *), which accepts a type pointer and returns a
   /// transformed type, a null type (which will propagate the null type to the
   /// outermost \c transform() call), or None (to indicate that the transform
   /// operation should recursively transform the subtypes). The function object
   /// should use \c dyn_cast rather \c getAs, because the transform itself
   /// handles desugaring.
   ///
   /// \returns the result of transforming the type.
   Type transformRec(FunctionRef<std::optional<Type>(TypeBase *)> func) const;

   /// Look through the given type and its children and apply fn to them.
   void visit(FunctionRef<void (Type)> func) const
   {
      findIf([&func](Type t) -> bool {
         func(t);
         return false;
      });
   }

   /// Replace references to substitutable types with new, concrete types and
   /// return the substituted result.
   ///
   /// \param substitutions The mapping from substitutable types to their
   /// replacements and conformances.
   ///
   /// \param options Options that affect the substitutions.
   ///
   /// \returns the substituted type, or a null type if an error occurred.
   Type subst(SubstitutionMap substitutions,
              SubstOptions options = std::nullopt) const;

   /// Replace references to substitutable types with new, concrete types and
   /// return the substituted result.
   ///
   /// \param substitutions A function mapping from substitutable types to their
   /// replacements.
   ///
   /// \param conformances A function for looking up conformances.
   ///
   /// \param options Options that affect the substitutions.
   ///
   /// \returns the substituted type, or a null type if an error occurred.
   Type subst(TypeSubstitutionFunc substitutions,
              LookupConformanceFunc conformances,
              SubstOptions options = std::nullopt) const;

   /// Replace references to substitutable types with error types.
   Type substDependentTypesWithErrorTypes() const;

   bool isPrivateStdlibType(bool treatNonBuiltinProtocolsAsPublic = true) const;

   void dump() const;
   void dump(RawOutStream &outStream, unsigned indent = 0) const;

   void print(RawOutStream &outStream, const PrintOptions &options = PrintOptions()) const;
   void print(AstPrinter &printer, const PrintOptions &options) const;

   /// Return the name of the type as a string, for use in diagnostics only.
   std::string getString(const PrintOptions &options = PrintOptions()) const;

   /// Return the name of the type, adding parens in cases where
   /// appending or prepending text to the result would cause that text
   /// to be appended to only a portion of the returned type. For
   /// example for a function type "Int -> Float", adding text after
   /// the type would make it appear that it's appended to "Float" as
   /// opposed to the entire type.
   std::string
   getStringAsComponent(const PrintOptions &options = PrintOptions()) const;

   /// Computes the join between two types.
   ///
   /// The join of two types is the most specific type that is a supertype of
   /// both \c type1 and \c type2, e.g., the least upper bound in the type
   /// lattice. For example, given a simple class hierarchy as follows:
   ///
   /// \code
   /// class A { }
   /// class B : A { }
   /// class C : A { }
   /// class D { }
   /// \endcode
   ///
   /// The join of B and C is A, the join of A and B is A.
   ///
   /// The Any type is considered the common supertype by default when no
   /// closer common supertype exists.
   ///
   /// In unsupported cases where we cannot yet compute an accurate join,
   /// we return None.
   ///
   /// \returns the join of the two types, if there is a concrete type
   /// that can express the join, or Any if the only join would be a
   /// more-general existential type, or None if we cannot yet compute a
   /// correct join but one better than Any may exist.
   static std::optional<Type> join(Type first, Type second);

private:
   // Direct comparison is disabled for types, because they may not be canonical.
   void operator==(Type type) const = delete;
   void operator!=(Type type) const = delete;
};

/// CanType - This is a Type that is statically known to be canonical.  To get
/// one of these, use Type->getCanonicalType().  Since all CanType's can be used
/// as 'Type' (they just don't have sugar) we derive from Type.
class CanType : public Type
{
   bool isActuallyCanonicalOrNull() const;

   static bool isReferenceTypeImpl(CanType type, bool functionsCount);
   static bool isExistentialTypeImpl(CanType type);
   static bool isAnyExistentialTypeImpl(CanType type);
   static CanType getOptionalObjectTypeImpl(CanType type);
   static CanType getReferenceStorageReferentImpl(CanType type);
   static CanType getWithoutSpecifierTypeImpl(CanType type);

public:
   explicit CanType(TypeBase *ptr = 0)
      : Type(ptr)
   {
      assert(isActuallyCanonicalOrNull() &&
             "Forming a CanType out of a non-canonical type!");
   }

   explicit CanType(Type type) : Type(type)
   {
      assert(isActuallyCanonicalOrNull() &&
             "Forming a CanType out of a non-canonical type!");
   }

   void visit(FunctionRef<void (CanType)> func) const
   {
      findIf([&func](Type t) -> bool {
         func(CanType(t));
         return false;
      });
   }

   bool findIf(FunctionRef<bool (CanType)> func) const
   {
      return Type::findIf([&func](Type t) {
         return func(CanType(t));
      });
   }

   // Provide a few optimized accessors that are really type-class queries.

   /// Do values of this type have reference semantics?
   bool hasReferenceSemantics() const
   {
      return isReferenceTypeImpl(*this, /*functions count*/ true);
   }

   /// Are values of this type essentially just class references,
   /// possibly with some extra metadata?
   ///
   ///   - any of the builtin reference types
   ///   - a class type
   ///   - a bound generic class type
   ///   - a class-bounded archetype type
   ///   - a class-bounded existential type
   ///   - a dynamic Self type
   bool isAnyClassReferenceType() const
   {
      return isReferenceTypeImpl(*this, /*functions count*/ false);
   }

   /// Is this type existential?
   bool isExistentialType() const
   {
      return isExistentialTypeImpl(*this);
   }

   /// Is this type an existential or an existential metatype?
   bool isAnyExistentialType() const
   {
      return isAnyExistentialTypeImpl(*this);
   }

   ClassDecl *getClassOrBoundGenericClass() const; // in Types.h
   StructDecl *getStructOrBoundGenericStruct() const; // in Types.h
   EnumDecl *getEnumOrBoundGenericEnum() const; // in Types.h
   NominalTypeDecl *getNominalOrBoundGenericNominal() const; // in Types.h
   CanType getNominalParent() const; // in Types.h
   NominalTypeDecl *getAnyNominal() const;
   GenericTypeDecl *getAnyGeneric() const;

   CanType getOptionalObjectType() const
   {
      return getOptionalObjectTypeImpl(*this);
   }

   CanType getReferenceStorageReferent() const
   {
      return getReferenceStorageReferentImpl(*this);
   }

   CanType getWithoutSpecifierType() const
   {
      return getWithoutSpecifierTypeImpl(*this);
   }

   // Direct comparison is allowed for CanTypes - they are known canonical.
   bool operator==(CanType type) const
   {
      return getPointer() == type.getPointer();
   }

   bool operator!=(CanType type) const
   {
      return !operator==(type);
   }

   bool operator<(CanType type) const
   {
      return getPointer() < type.getPointer();
   }
};

template <typename Proxied>
class CanTypeWrapper;

// Define a database of CanType wrapper classes for ease of metaprogramming.
// By definition, this maps 'FOO' to 'CanFOO'.
template <typename Proxied>
struct CanTypeWrapperTraits;

template <>
struct CanTypeWrapperTraits<Type>
{
   typedef CanType type;
};

// A wrapper which preserves the fact that a type is canonical.
//
// Intended to be used as follows:
//   DEFINE_LEAF_CAN_TYPE_WRAPPER(BuiltinNativeObject, BuiltinType)
// or
//   BEGIN_CAN_TYPE_WRAPPER(MetatypeType, Type)
//     PROXY_CAN_TYPE_SIMPLE_GETTER(getInstanceType)
//   END_CAN_TYPE_WRAPPER(MetatypeType, Type)
#define BEGIN_CAN_TYPE_WRAPPER(TYPE, BASE)                          \
   typedef CanTypeWrapper<TYPE> Can##TYPE;                             \
   template <>                                                         \
   class CanTypeWrapper<TYPE> : public Can##BASE {                     \
   public:                                                             \
   explicit CanTypeWrapper(TYPE *theType = nullptr)                  \
   : Can##BASE(theType) {}                                         \
   \
   TYPE *getPointer() const {                                        \
   return static_cast<TYPE*>(Type::getPointer());                  \
}                                                                 \
   TYPE *operator->() const { return getPointer(); }                 \
   operator TYPE *() const { return getPointer(); }                  \
   explicit operator bool() const { return getPointer() != nullptr; }

#define PROXY_CAN_TYPE_SIMPLE_GETTER(METHOD)                        \
   CanType METHOD() const { return CanType(getPointer()->METHOD()); }

#define END_CAN_TYPE_WRAPPER(TYPE, BASE)                            \
};                                                                  \
   template <> struct CanTypeWrapperTraits<TYPE> {                     \
   typedef Can##TYPE type;                                           \
}

#define DEFINE_EMPTY_CAN_TYPE_WRAPPER(TYPE, BASE)                   \
   BEGIN_CAN_TYPE_WRAPPER(TYPE, BASE)                                  \
   END_CAN_TYPE_WRAPPER(TYPE, BASE)

// Disallow direct uses of isa/cast/dyn_cast on Type to eliminate a
// certain class of bugs.
template <typename X>
inline bool
isa(const Type&) = delete; // Use TypeBase::is instead.

template <typename X>
inline typename polar::utils::CastRetty<X, Type>::RetType
cast(const Type&) = delete; // Use TypeBase::castTo instead.
template <typename X>
inline typename polar::utils::CastRetty<X, Type>::RetType
dyn_cast(const Type&) = delete; // Use TypeBase::getAs instead.
template <typename X> inline typename polar::utils::CastRetty<X, Type>::RetType
dyn_cast_or_null(const Type&) = delete;

// Permit direct uses of isa/cast/dyn_cast on CanType and preserve
// canonicality.
template <typename X>
inline bool isa(CanType type)
{
   return isa<X>(type.getPointer());
}

template <typename X>
inline CanTypeWrapper<X> cast(CanType type)
{
   //return CanTypeWrapper<X>(cast<X>(type.getPointer()));
}

template <typename X>
inline CanTypeWrapper<X> cast_or_null(CanType type)
{
   return CanTypeWrapper<X>(cast_or_null<X>(type.getPointer()));
}

template <typename X>
inline CanTypeWrapper<X> dyn_cast(CanType type)
{
   auto Ty = type.getPointer();
   POLAR_ASSUME(Ty != nullptr);
   return CanTypeWrapper<X>(dyn_cast<X>(Ty));
}

template <typename X>
inline CanTypeWrapper<X> dyn_cast_or_null(CanType type)
{
   return CanTypeWrapper<X>(dyn_cast_or_null<X>(type.getPointer()));
}

// Permit direct uses of isa/cast/dyn_cast on CanTypeWrapper<T> and
// preserve canonicality.
template <typename X, typename P>
inline bool isa(CanTypeWrapper<P> type)
{
   return isa<X>(type.getPointer());
}

template <typename X, typename P>
inline CanTypeWrapper<X> cast(CanTypeWrapper<P> type)
{
   return CanTypeWrapper<X>(cast<X>(type.getPointer()));
}

template <typename X, typename P>
inline CanTypeWrapper<X> dyn_cast(CanTypeWrapper<P> type)
{
   auto Ty = type.getPointer();
   POLAR_ASSUME(Ty != nullptr);
   return CanTypeWrapper<X>(dyn_cast<X>(Ty));
}

template <typename X, typename P>
inline CanTypeWrapper<X> dyn_cast_or_null(CanTypeWrapper<P> type)
{
   return CanTypeWrapper<X>(dyn_cast_or_null<X>(type.getPointer()));
}

class GenericTypeParamType;

/// A reference to a canonical generic signature.
class CanGenericSignature
{
   GenericSignature *m_signature;

public:
   CanGenericSignature()
      : m_signature(nullptr)
   {}

   CanGenericSignature(std::nullptr_t)
      : m_signature(nullptr)
   {}

   // in Decl.h
   explicit CanGenericSignature(GenericSignature *signature);
   ArrayRef<CanTypeWrapper<GenericTypeParamType>> getGenericParams() const;

   /// Retrieve the canonical generic environment associated with this
   /// generic signature.
   GenericEnvironment *getGenericEnvironment() const;

   GenericSignature *operator->() const
   {
      return m_signature;
   }

   operator GenericSignature *() const
   {
      return m_signature;
   }

   GenericSignature *getPointer() const
   {
      return m_signature;
   }

   bool operator==(const CanGenericSignature& other)
   {
      return m_signature == other.m_signature;
   }
};

template <typename T>
inline T *staticCastHelper(const Type &type)
{
   // The constructor of the ArrayRef<Type> must guarantee this invariant.
   // XXX -- We use reinterpret_cast instead of static_cast so that files
   // can avoid including Types.h if they want to.
   return reinterpret_cast<T*>(type.getPointer());
}

/// TypeArrayView allows arrays of 'Type' to have a static type.
template <typename T>
using TypeArrayView = ArrayRefView<Type, T*, staticCastHelper,
/*AllowOrigAccess*/true>;

} // polar::ast

namespace polar::basic {

using polar::utils::RawOutStream;
using AstType = polar::ast::Type;
using AstBaseType = polar::ast::TypeBase;
using polar::ast::CanType;

static inline RawOutStream &
operator<<(RawOutStream &outStream, AstType type)
{
   type.print(outStream);
   return outStream;
}

// Type hashes just like pointers.
template<>
struct DenseMapInfo<AstType>
{
   static AstType getEmptyKey()
   {
      return DenseMapInfo<AstBaseType*>::getEmptyKey();
   }

   static AstType getTombstoneKey()
   {
      return DenseMapInfo<AstBaseType*>::getTombstoneKey();
   }

   static unsigned getHashValue(AstType value)
   {
      return DenseMapInfo<AstBaseType*>::getHashValue(value.getPointer());
   }

   static bool isEqual(AstType lhs, AstType rhs)
   {
      return lhs.getPointer() == rhs.getPointer();
   }
};

template<>
struct DenseMapInfo<CanType>
      : public DenseMapInfo<AstType>
{
   static CanType getEmptyKey() {
      return CanType(DenseMapInfo<AstBaseType*>::getEmptyKey());
   }

   static CanType getTombstoneKey()
   {
      return CanType(DenseMapInfo<AstBaseType*>::getTombstoneKey());
   }
};
} // polar::basic

namespace polar::utils {

using AstType = polar::ast::Type;
using AstBaseType = polar::ast::TypeBase;

// A Type casts like a TypeBase*.
template<>
struct SimplifyType<const AstType>
{
   typedef AstBaseType *SimpleType;
   static SimpleType getSimplifiedValue(const AstType &value)
   {
      return value.getPointer();
   }
};

template<>
struct SimplifyType<AstType>
      : public SimplifyType<const AstType>
{};
} // polar::utils

namespace polar::utils {

using AstType = polar::ast::Type;
using AstBaseType = polar::ast::TypeBase;
using polar::ast::CanType;
using polar::ast::CanGenericSignature;
using polar::ast::GenericSignature;

// A Type is "pointer like".
template<>
struct PointerLikeTypeTraits<AstType>
{
public:
   static inline void *getAsVoidPointer(AstType type)
   {
      return (void*)type.getPointer();
   }

   static inline AstType getFromVoidPointer(void *ptr)
   {
      return (AstBaseType*)ptr;
   }

   enum { NumLowBitsAvailable = polar::ast::TypeAlignInBits };
};

template<>
struct PointerLikeTypeTraits<CanType> :
      public PointerLikeTypeTraits<AstType>
{
public:
   static inline CanType getFromVoidPointer(void *ptr)
   {
      return CanType((AstBaseType*)ptr);
   }
};

template<>
struct PointerLikeTypeTraits<CanGenericSignature>
{
public:
   static inline CanGenericSignature getFromVoidPointer(void *ptr)
   {
      return CanGenericSignature((GenericSignature*)ptr);
   }

   static inline void *getAsVoidPointer(CanGenericSignature signature)
   {
      return (void*)signature.getPointer();
   }
   enum { NumLowBitsAvailable = polar::ast::TypeAlignInBits };
};
} // polar::utils

#endif // POLARPHP_AST_TYPE_H
