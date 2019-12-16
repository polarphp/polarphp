//===--- PILType.h - Defines the PILType type -------------------*- C++ -*-===//
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
// This file defines the PILType class, which is used to refer to PIL
// representation types.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILTYPE_H
#define POLARPHP_PIL_PILTYPE_H

#include "polarphp/ast/CanTypeVisitor.h"
#include "polarphp/ast/Types.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/Support/ErrorHandling.h"
#include "polarphp/pil/lang/PILAllocated.h"
#include "polarphp/pil/lang/PILArgumentConvention.h"
#include "llvm/ADT/Hashing.h"
#include "polarphp/pil/lang/PILDeclRef.h"

namespace polar {
class PILFunction;
class AstContext;
class VarDecl;
class TypeBase;
class CanType;
enum class PILFunctionTypeRepresentation : uint8_t;
class StructDecl;
class EnumDecl;
class NominalTypeDecl;
class CanGenericSignature;
class TypeExpansionContext;
class Type;
class SubstitutionMap;
class PILFunctionType;
class AnyMetatypeType;
class EnumElementDecl;
class TupleType;
class BuiltinFloatType;
class InterfaceConformanceRef;
class PILBlockStorageType;

namespace lowering {
class AbstractionPattern;
class TypeConverter;
}

} // end namespace polar

namespace polar {

/// How an existential type container is represented.
enum class ExistentialRepresentation {
   /// The type is not existential.
      None,
   /// The container uses an opaque existential container, with a fixed-sized
   /// buffer. The type is address-only and is manipulated using the
   /// {init,open,deinit}_existential_addr family of instructions.
      Opaque,
   /// The container uses a class existential container, which holds a reference
   /// to the class instance that conforms to the protocol. The type is
   /// reference-counted and is manipulated using the
   /// {init,open}_existential_ref family of instructions.
      Class,
   /// The container uses a metatype existential container, which holds a
   /// reference to the type metadata for a type that
   /// conforms to the protocol. The type is trivial, and is manipulated using
   /// the {init,open}_existential_metatype family of instructions.
      Metatype,
   /// The container uses a boxed existential container, which is a
   /// reference-counted buffer that indirectly contains the
   /// conforming value. The type is manipulated
   /// using the {alloc,open,dealloc}_existential_box family of instructions.
   /// The container may be able to directly adopt a class reference using
   /// init_existential_ref for some class types.
      Boxed,
};

/// The value category.
enum class PILValueCategory : uint8_t {
   /// An object is a value of the type.
      Object,

   /// An address is a pointer to an allocated variable of the type
   /// (possibly uninitialized).
      Address,
};

/// PILType - A Swift type that has been lowered to a PIL representation type.
/// In addition to the Swift type system, PIL adds "address" types that can
/// reference any Swift type (but cannot take the address of an address). *T
/// is the type of an address pointing at T.
///
class PILType {
public:
   /// The unsigned is a PILValueCategory.
   using ValueType = llvm::PointerIntPair<TypeBase *, 2, unsigned>;
private:
   ValueType value;

   /// Private constructor. PILTypes are normally vended by
   /// TypeConverter::getLoweredType().
   PILType(CanType ty, PILValueCategory category)
      : value(ty.getPointer(), unsigned(category)) {
      if (!ty) return;
      assert(ty->isLegalPILType() &&
             "constructing PILType with type that should have been "
             "eliminated by PIL lowering");
   }

   PILType(ValueType value) : value(value) {
   }

   friend class lowering::TypeConverter;

   friend struct llvm::DenseMapInfo<PILType>;
public:
   PILType() = default;

   /// getPrimitiveType - Form a PILType for a primitive type that does not
   /// require any special handling (i.e., not a function or aggregate type).
   static PILType getPrimitiveType(CanType T, PILValueCategory category) {
      return PILType(T, category);
   }

   /// Form the type of an r-value, given a Swift type that either does
   /// not require any special handling or has already been
   /// appropriately lowered.
   static PILType getPrimitiveObjectType(CanType T) {
      return PILType(T, PILValueCategory::Object);
   }

   /// Form the type for the address of an object, given a Swift type
   /// that either does not require any special handling or has already
   /// been appropriately lowered.
   static PILType getPrimitiveAddressType(CanType T) {
      return PILType(T, PILValueCategory::Address);
   }

   bool isNull() const { return value.getPointer() == nullptr; }

   explicit operator bool() const { return bool(value.getPointer()); }

   PILValueCategory getCategory() const {
      return PILValueCategory(value.getInt());
   }

   /// Returns the \p Category variant of this type.
   PILType getCategoryType(PILValueCategory Category) const {
      return PILType(getAstType(), Category);
   }

   /// Returns the variant of this type that matches \p Ty.getCategory()
   PILType copyCategory(PILType Ty) const {
      return getCategoryType(Ty.getCategory());
   }

   /// Returns the address variant of this type.  Instructions which
   /// manipulate memory will generally work with object addresses.
   PILType getAddressType() const {
      return PILType(getAstType(), PILValueCategory::Address);
   }

   /// Returns the object variant of this type.  Note that address-only
   /// types are not legal to manipulate directly as objects in PIL.
   PILType getObjectType() const {
      return PILType(getAstType(), PILValueCategory::Object);
   }

   /// Returns the canonical AST type referenced by this PIL type.
   ///
   /// NOTE:
   /// 1. The returned AST type may not be a proper formal type.
   ///    For example, it may contain a PILFunctionType instead of a
   ///    FunctionType.
   /// 2. The returned type may not be the same as the original
   ///    unlowered type that produced this PILType (even after
   ///    canonicalization). If you need it, you must pass it separately.
   ///    For example, `AnyObject.Type` may get lowered to
   ///    `$@thick AnyObject.Type`, for which the AST type will be
   ///    `@thick AnyObject.Type`.
   ///    More generally, you cannot recover a formal type from
   ///    a lowered type. See docs/PIL.rst for more details.
   CanType getAstType() const {
      return CanType(value.getPointer());
   }

   // FIXME -- Temporary until LLDB adopts getAstType()
   LLVM_ATTRIBUTE_DEPRECATED(CanType getSwiftRValueType() const,
                             "Please use getAstType()") {
      return getAstType();
   }

   /// Returns the AbstractCC of a function type.
   /// The PILType must refer to a function type.
   PILFunctionTypeRepresentation getFunctionRepresentation() const {
      return castTo<PILFunctionType>()->getRepresentation();
   }

   /// Cast the Swift type referenced by this PIL type, or return null if the
   /// cast fails.
   template<typename TYPE>
   typename CanTypeWrapperTraits<TYPE>::type
   getAs() const { return dyn_cast<TYPE>(getAstType()); }

   /// Cast the Swift type referenced by this PIL type, which must be of the
   /// specified subtype.
   template<typename TYPE>
   typename CanTypeWrapperTraits<TYPE>::type
   castTo() const { return cast<TYPE>(getAstType()); }

   /// Returns true if the Swift type referenced by this PIL type is of the
   /// specified subtype.
   template<typename TYPE>
   bool is() const { return isa<TYPE>(getAstType()); }

   bool isVoid() const {
      return value.getPointer()->isVoid();
   }

   /// Retrieve the ClassDecl for a type that maps to a Swift class or
   /// bound generic class type.
   ClassDecl *getClassOrBoundGenericClass() const {
      return getAstType().getClassOrBoundGenericClass();
   }

   /// Retrieve the StructDecl for a type that maps to a Swift struct or
   /// bound generic struct type.
   StructDecl *getStructOrBoundGenericStruct() const {
      return getAstType().getStructOrBoundGenericStruct();
   }

   /// Retrieve the EnumDecl for a type that maps to a Swift enum or
   /// bound generic enum type.
   EnumDecl *getEnumOrBoundGenericEnum() const {
      return getAstType().getEnumOrBoundGenericEnum();
   }

   /// Retrieve the NominalTypeDecl for a type that maps to a Swift
   /// nominal or bound generic nominal type.
   NominalTypeDecl *getNominalOrBoundGenericNominal() const {
      return getAstType().getNominalOrBoundGenericNominal();
   }

   /// True if the type is an address type.
   bool isAddress() const { return getCategory() == PILValueCategory::Address; }

   /// True if the type is an object type.
   bool isObject() const { return getCategory() == PILValueCategory::Object; }

   /// isAddressOnly - True if the type, or the referenced type of an address
   /// type, is address-only.  For example, it could be a resilient struct or
   /// something of unknown size.
   ///
   /// This is equivalent to, but possibly faster than, calling
   /// tc.getTypeLowering(type).isAddressOnly().
   static bool isAddressOnly(CanType type,
                             lowering::TypeConverter &tc,
                             CanGenericSignature sig,
                             TypeExpansionContext expansion);

   /// Return true if this type must be returned indirectly.
   ///
   /// This is equivalent to, but possibly faster than, calling
   /// tc.getTypeLowering(type).isReturnedIndirectly().
   static bool isFormallyReturnedIndirectly(CanType type,
                                            lowering::TypeConverter &tc,
                                            CanGenericSignature sig) {
      return isAddressOnly(type, tc, sig, TypeExpansionContext::minimal());
   }

   /// Return true if this type must be passed indirectly.
   ///
   /// This is equivalent to, but possibly faster than, calling
   /// tc.getTypeLowering(type).isPassedIndirectly().
   static bool isFormallyPassedIndirectly(CanType type,
                                          lowering::TypeConverter &tc,
                                          CanGenericSignature sig) {
      return isAddressOnly(type, tc, sig, TypeExpansionContext::minimal());
   }

   /// True if the type, or the referenced type of an address type, is loadable.
   /// This is the opposite of isAddressOnly.
   bool isLoadable(const PILFunction &F) const {
      return !isAddressOnly(F);
   }

   /// True if either:
   /// 1) The type, or the referenced type of an address type, is loadable.
   /// 2) The PIL Module conventions uses lowered addresses
   bool isLoadableOrOpaque(const PILFunction &F) const;

   /// True if the type, or the referenced type of an address type, is
   /// address-only. This is the opposite of isLoadable.
   bool isAddressOnly(const PILFunction &F) const;

   /// True if the underlying AST type is trivial, meaning it is loadable and can
   /// be trivially copied, moved or detroyed. Returns false for address types
   /// even though they are technically trivial.
   bool isTrivial(const PILFunction &F) const;

   /// True if the type, or the referenced type of an address type, is known to
   /// be a scalar reference-counted type such as a class, box, or thick function
   /// type. Returns false for non-trivial aggregates.
   bool isReferenceCounted(PILModule &M) const;

   /// Returns true if the referenced type is a function type that never
   /// returns.
   bool isNoReturnFunction(PILModule &M) const;

   /// Returns true if the referenced AST type has reference semantics, even if
   /// the lowered PIL type is known to be trivial.
   bool hasReferenceSemantics() const {
      return getAstType().hasReferenceSemantics();
   }

   /// Returns true if the referenced type is any sort of class-reference type,
   /// meaning anything with reference semantics that is not a function type.
   bool isAnyClassReferenceType() const {
      return getAstType().isAnyClassReferenceType();
   }

   /// Returns true if the referenced type is guaranteed to have a
   /// single-retainable-pointer representation.
   bool hasRetainablePointerRepresentation() const {
      return getAstType()->hasRetainablePointerRepresentation();
   }

   /// Returns true if the referenced type is an existential type.
   bool isExistentialType() const {
      return getAstType().isExistentialType();
   }

   /// Returns true if the referenced type is any kind of existential type.
   bool isAnyExistentialType() const {
      return getAstType().isAnyExistentialType();
   }

   /// Returns true if the referenced type is a class existential type.
   bool isClassExistentialType() const {
      return getAstType()->isClassExistentialType();
   }

   /// Returns true if the referenced type is an opened existential type
   /// (which is actually a kind of archetype).
   bool isOpenedExistential() const {
      return getAstType()->isOpenedExistential();
   }

   /// Returns true if the referenced type is expressed in terms of one
   /// or more opened existential types.
   bool hasOpenedExistential() const {
      return getAstType()->hasOpenedExistential();
   }

   /// Returns the representation used by an existential type. If the concrete
   /// type is provided, this may return a specialized representation kind that
   /// can be used for that type. Otherwise, returns the most general
   /// representation kind for the type. Returns None if the type is not an
   /// existential type.
   ExistentialRepresentation
   getPreferredExistentialRepresentation(Type containedType = Type()) const;

   /// Returns true if the existential type can use operations for the given
   /// existential representation when working with values of the given type,
   /// or when working with an unknown type if containedType is null.
   bool
   canUseExistentialRepresentation(ExistentialRepresentation repr,
                                   Type containedType = Type()) const;

   /// True if the type contains a type parameter.
   bool hasTypeParameter() const {
      return getAstType()->hasTypeParameter();
   }

   /// True if the type is bridgeable to an ObjC object pointer type.
   bool isBridgeableObjectType() const {
      return getAstType()->isBridgeableObjectType();
   }

   static bool isClassOrClassMetatype(Type t) {
      if (auto *meta = t->getAs<AnyMetatypeType>()) {
         return bool(meta->getInstanceType()->getClassOrBoundGenericClass());
      } else {
         return bool(t->getClassOrBoundGenericClass());
      }
   }

   /// True if the type is a class type or class metatype type.
   bool isClassOrClassMetatype() {
      return isObject() && isClassOrClassMetatype(getAstType());
   }

   /// True if the type involves any archetypes.
   bool hasArchetype() const {
      return getAstType()->hasArchetype();
   }

   /// Returns the AstContext for the referenced Swift type.
   AstContext &getAstContext() const {
      return getAstType()->getAstContext();
   }

   /// True if the given type has at least the size and alignment of a native
   /// pointer.
   bool isPointerSizeAndAligned();

   /// True if `operTy` can be cast by single-reference value into `resultTy`.
   static bool canRefCast(PILType operTy, PILType resultTy, PILModule &M);

   /// True if the type is block-pointer-compatible, meaning it either is a block
   /// or is an Optional with a block payload.
   bool isBlockPointerCompatible() const {
      // Look through one level of optionality.
      PILType ty = *this;
      if (auto optPayload = ty.getOptionalObjectType()) {
         ty = optPayload;
      }

      auto fTy = ty.getAs<PILFunctionType>();
      if (!fTy)
         return false;
      return fTy->getRepresentation() == PILFunctionType::Representation::Block;
   }

   /// Given that this is a nominal type, return the lowered type of
   /// the given field.  Applies substitutions as necessary.  The
   /// result will be an address type if the base type is an address
   /// type or a class.
   PILType getFieldType(VarDecl *field, lowering::TypeConverter &TC,
                        TypeExpansionContext context) const;

   PILType getFieldType(VarDecl *field, PILModule &M,
                        TypeExpansionContext context) const;

   /// Given that this is an enum type, return the lowered type of the
   /// data for the given element.  Applies substitutions as necessary.
   /// The result will have the same value category as the base type.
   PILType getEnumElementType(EnumElementDecl *elt, lowering::TypeConverter &TC,
                              TypeExpansionContext context) const;

   PILType getEnumElementType(EnumElementDecl *elt, PILModule &M,
                              TypeExpansionContext context) const;

   /// Given that this is a tuple type, return the lowered type of the
   /// given tuple element.  The result will have the same value
   /// category as the base type.
   PILType getTupleElementType(unsigned index) const {
      return PILType(castTo<TupleType>().getElementType(index), getCategory());
   }

   /// Return the immediate superclass type of this type, or null if
   /// it's the most-derived type.
   PILType getSuperclass() const {
      auto superclass = getAstType()->getSuperclass();
      if (!superclass) return PILType();
      return PILType::getPrimitiveObjectType(superclass->getCanonicalType());
   }

   /// Return true if Ty is a subtype of this exact PILType, or false otherwise.
   bool isExactSuperclassOf(PILType Ty) const {
      return getAstType()->isExactSuperclassOf(Ty.getAstType());
   }

   /// Return true if Ty is a subtype of this PILType, or if this PILType
   /// contains archetypes that can be found to form a supertype of Ty, or false
   /// otherwise.
   bool isBindableToSuperclassOf(PILType Ty) const {
      return getAstType()->isBindableToSuperclassOf(Ty.getAstType());
   }

   /// Look through reference-storage types on this type.
   PILType getReferenceStorageReferentType() const {
      return PILType(getAstType().getReferenceStorageReferent(), getCategory());
   }

   /// Transform the function type PILType by replacing all of its interface
   /// generic args with the appropriate item from the substitution.
   ///
   /// Only call this with function types!
   PILType substGenericArgs(lowering::TypeConverter &TC, SubstitutionMap SubMap,
                            TypeExpansionContext context) const;

   PILType substGenericArgs(PILModule &M, SubstitutionMap SubMap,
                            TypeExpansionContext context) const;

   /// If the original type is generic, pass the signature as genericSig.
   ///
   /// If the replacement types are generic, you must push a generic context
   /// first.
   PILType subst(lowering::TypeConverter &tc, TypeSubstitutionFn subs,
                 LookupConformanceFn conformances,
                 CanGenericSignature genericSig = CanGenericSignature(),
                 bool shouldSubstituteOpaqueArchetypes = false) const;

   PILType subst(PILModule &M, TypeSubstitutionFn subs,
                 LookupConformanceFn conformances,
                 CanGenericSignature genericSig = CanGenericSignature(),
                 bool shouldSubstituteOpaqueArchetypes = false) const;

   PILType subst(lowering::TypeConverter &tc, SubstitutionMap subs) const;

   PILType subst(PILModule &M, SubstitutionMap subs) const;

   /// Return true if this type references a "ref" type that has a single pointer
   /// representation. Class existentials do not always qualify.
   bool isHeapObjectReferenceType() const;

   /// Returns true if this PILType is an aggregate that contains \p Ty
   bool aggregateContainsRecord(PILType Ty, PILModule &PILMod,
                                TypeExpansionContext context) const;

   /// Returns true if this PILType is an aggregate with unreferenceable storage,
   /// meaning it cannot be fully destructured in PIL.
   bool aggregateHasUnreferenceableStorage() const;

   /// Returns the lowered type for T if this type is Optional<T>;
   /// otherwise, return the null type.
   PILType getOptionalObjectType() const;

   /// Unwraps one level of optional type.
   /// Returns the lowered T if the given type is Optional<T>.
   /// Otherwise directly returns the given type.
   PILType unwrapOptionalType() const;

   /// Returns true if this is the AnyObject PILType;
   bool isAnyObject() const { return getAstType()->isAnyObject(); }

   /// Returns a PILType with any archetypes mapped out of context.
   PILType mapTypeOutOfContext() const;

   /// Given two PIL types which are representations of the same type,
   /// check whether they have an abstraction difference.
   bool hasAbstractionDifference(PILFunctionTypeRepresentation rep,
                                 PILType type2);

   /// Returns true if this PILType could be potentially a lowering of the given
   /// formal type. Meant for verification purposes/assertions.
   bool isLoweringOf(TypeExpansionContext context, PILModule &M,
                     CanType formalType);

   /// Returns the hash code for the PILType.
   llvm::hash_code getHashCode() const {
      return llvm::hash_combine(*this);
   }

   //
   // Accessors for types used in PIL instructions:
   //

   /// Get the NativeObject type as a PILType.
   static PILType getNativeObjectType(const AstContext &C);

   /// Get the BridgeObject type as a PILType.
   static PILType getBridgeObjectType(const AstContext &C);

   /// Get the RawPointer type as a PILType.
   static PILType getRawPointerType(const AstContext &C);

   /// Get a builtin integer type as a PILType.
   static PILType getBuiltinIntegerType(unsigned bitWidth, const AstContext &C);

   /// Get the IntegerLiteral type as a PILType.
   static PILType getBuiltinIntegerLiteralType(const AstContext &C);

   /// Get a builtin floating-point type as a PILType.
   static PILType getBuiltinFloatType(BuiltinFloatType::FPKind Kind,
                                      const AstContext &C);

   /// Get the builtin word type as a PILType;
   static PILType getBuiltinWordType(const AstContext &C);

   /// Given a value type, return an optional type wrapping it.
   static PILType getOptionalType(PILType valueType);

   /// Get the standard exception type.
   static PILType getExceptionType(const AstContext &C);

   /// Get the PIL token type.
   static PILType getPILTokenType(const AstContext &C);

   //
   // Utilities for treating PILType as a pointer-like type.
   //
   static PILType getFromOpaqueValue(void *P) {
      return PILType(ValueType::getFromOpaqueValue(P));
   }

   void *getOpaqueValue() const {
      return value.getOpaqueValue();
   }

   bool operator==(PILType rhs) const {
      return value.getOpaqueValue() == rhs.value.getOpaqueValue();
   }

   bool operator!=(PILType rhs) const {
      return value.getOpaqueValue() != rhs.value.getOpaqueValue();
   }

   /// Return the mangled name of this type, ignoring its prefix. Meant for
   /// diagnostic purposes.
   std::string getMangledName() const;

   std::string getAsString() const;

   void dump() const;

   void print(raw_ostream &OS) const;
};

// Statically prevent PILTypes from being directly cast to a type
// that's not legal as a PIL value.
#define NON_PIL_TYPE(ID)                                             \
template<> Can##ID##Type PILType::getAs<ID##Type>() const = delete;  \
template<> Can##ID##Type PILType::castTo<ID##Type>() const = delete; \
template<> bool PILType::is<ID##Type>() const = delete;

NON_PIL_TYPE(Function)
NON_PIL_TYPE(AnyFunction)
NON_PIL_TYPE(LValue)

#undef NON_PIL_TYPE

CanPILFunctionType getNativePILFunctionType(
   lowering::TypeConverter &TC, TypeExpansionContext context,
   lowering::AbstractionPattern origType, CanAnyFunctionType substType,
   Optional<PILDeclRef> origConstant = None,
   Optional<PILDeclRef> constant = None,
   Optional<SubstitutionMap> reqtSubs = None,
   InterfaceConformanceRef witnessMethodConformance = InterfaceConformanceRef());

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS, PILType T) {
   T.print(OS);
   return OS;
}

inline PILType PILBlockStorageType::getCaptureAddressType() const {
   return PILType::getPrimitiveAddressType(getCaptureType());
}

/// The hash of a PILType is the hash of its opaque value.
static inline llvm::hash_code hash_value(PILType V) {
   return llvm::hash_value(V.getOpaqueValue());
}

inline PILType PILField::getAddressType() const {
   return PILType::getPrimitiveAddressType(getLoweredType());
}
inline PILType PILField::getObjectType() const {
   return PILType::getPrimitiveObjectType(getLoweredType());
}

CanType getPILBoxFieldLoweredType(TypeExpansionContext context,
                                  PILBoxType *type, polar::lowering::TypeConverter &TC,
                                  unsigned index);

inline PILType getPILBoxFieldType(TypeExpansionContext context,
                                  PILBoxType *type, polar::lowering::TypeConverter &TC,
                                  unsigned index) {
   return PILType::getPrimitiveAddressType(
      getPILBoxFieldLoweredType(context, type, TC, index));
}

} // end polar namespace

namespace llvm {

// Allow the low bit of PILType to be used for nefarious purposes, e.g. putting
// a PILType into a PointerUnion.
template<>
struct PointerLikeTypeTraits<polar::PILType> {
public:
   static inline void *getAsVoidPointer(polar::PILType T) {
      return T.getOpaqueValue();
   }
   static inline polar::PILType getFromVoidPointer(void *P) {
      return polar::PILType::getFromOpaqueValue(P);
   }
   // PILType is just a wrapper around its ValueType, so it has a bit available.
   enum { NumLowBitsAvailable =
      PointerLikeTypeTraits<polar::PILType::ValueType>::NumLowBitsAvailable };
};


// Allow PILType to be used as a DenseMap key.
template<>
struct DenseMapInfo<polar::PILType> {
   using PILType = polar::PILType;
   using PointerMapInfo = DenseMapInfo<void*>;
public:
   static PILType getEmptyKey() {
      return PILType::getFromOpaqueValue(PointerMapInfo::getEmptyKey());
   }
   static PILType getTombstoneKey() {
      return PILType::getFromOpaqueValue(PointerMapInfo::getTombstoneKey());
   }
   static unsigned getHashValue(PILType t) {
      return PointerMapInfo::getHashValue(t.getOpaqueValue());
   }
   static bool isEqual(PILType LHS, PILType RHS) {
      return LHS == RHS;
   }
};

} // end llvm namespace

#endif
