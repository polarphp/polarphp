//===--- TypeRepr.h - Swift Language Type Representation --------*- context++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
// This file defines the TypeRepr and related classes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_TYPEREPR_H
#define POLARPHP_AST_TYPEREPR_H

#include "polarphp/ast/Attr.h"
#include "polarphp/ast/DeclContext.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeAlignments.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/Basic/InlineBitfield.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/TrailingObjects.h"

namespace polar::ast {

/// forward declare class
class AstWalker;
class DeclContext;
class GenericEnvironment;
class IdentTypeRepr;
class TupleTypeRepr;
class TypeDecl;

enum class TypeReprKind : uint8_t {
#define TYPEREPR(ID, PARENT) ID,
#define LAST_TYPEREPR(ID) Last_TypeRepr = ID,
#include "TypeReprNodesDefs.h"
};
enum : unsigned {
   NumTypeReprKindBits =
   polar::basic::count_bits_used(static_cast<unsigned>(TypeReprKind::Last_TypeRepr))
};

/// Representation of a type as written in source.
class alignas(8) TypeRepr
{
   TypeRepr(const TypeRepr&) = delete;
   void operator=(const TypeRepr&) = delete;

   protected:
   union {
      uint64_t OpaqueBits;

      POLAR_INLINE_BITFIELD_BASE(
               TypeRepr, bitmax(NumTypeReprKindBits,8)+1+1,
               /// The subclass of TypeRepr that this is.
               Kind : bitmax(NumTypeReprKindBits,8),

               /// Whether this type representation is known to contain an invalid
               /// type.
               Invalid : 1,

               /// Whether this type representation had a warning emitted related to it.
               /// This is a hack related to how we resolve type exprs multiple times in
               /// generic contexts.
               Warned : 1
               );

      POLAR_INLINE_BITFIELD_FULL(TupleTypeRepr, TypeRepr, 1+32,
                                 /// Whether this tuple has '...' and its position.
                                 HasEllipsis : 1,
                                 : NumPadBits,
                                 /// The number of elements contained.
                                 NumElements : 32
                                 );

      POLAR_INLINE_BITFIELD_EMPTY(IdentTypeRepr, TypeRepr);
      POLAR_INLINE_BITFIELD_EMPTY(ComponentIdentTypeRepr, IdentTypeRepr);

      POLAR_INLINE_BITFIELD_FULL(GenericIdentTypeRepr, ComponentIdentTypeRepr, 32,
                                 : NumPadBits,
                                 NumGenericArgs : 32
                                 );

      POLAR_INLINE_BITFIELD_FULL(CompoundIdentTypeRepr, IdentTypeRepr, 32,
                                 : NumPadBits,
                                 NumComponents : 32
                                 );

      POLAR_INLINE_BITFIELD_FULL(CompositionTypeRepr, TypeRepr, 32,
                                 : NumPadBits,
                                 NumTypes : 32
                                 );

      POLAR_INLINE_BITFIELD_FULL(SILBoxTypeRepr, TypeRepr, 32,
                                 NumGenericArgs : NumPadBits,
                                 NumFields : 32
                                 );

   } bits;

   TypeRepr(TypeReprKind kind)
   {
      bits.OpaqueBits = 0;
      bits.TypeRepr.Kind = static_cast<unsigned>(kind);
      bits.TypeRepr.Invalid = false;
      bits.TypeRepr.Warned = false;
   }

   private:
   SourceLoc getLocImpl() const
   {
      return getStartLoc();
   }

   public:
   TypeReprKind getKind() const
   {
      return static_cast<TypeReprKind>(bits.TypeRepr.Kind);
   }

   /// Is this type representation known to be invalid?
   bool isInvalid() const
   {
      return bits.TypeRepr.Invalid;
   }

   /// Note that this type representation describes an invalid type.
   void setInvalid()
   {
      bits.TypeRepr.Invalid = true;
   }

   /// If a warning is produced about this type repr, keep track of that so we
   /// don't emit another one upon further reanalysis.
   bool isWarnedAbout() const
   {
      return bits.TypeRepr.Warned;
   }

   void setWarned()
   {
      bits.TypeRepr.Warned = true;
   }

   /// Get the representative location for pointing at this type.
   SourceLoc getLoc() const;

   SourceLoc getStartLoc() const;
   SourceLoc getEndLoc() const;
   SourceRange getSourceRange() const;

   /// Is this type grammatically a type-simple?
   inline bool isSimple() const; // bottom of this file

   static bool classof(const TypeRepr *type)
   {
      return true;
   }

   /// Walk this type representation.
   TypeRepr *walk(AstWalker &walker);
   TypeRepr *walk(AstWalker &&walker)
   {
      return walk(walker);
   }

   //*** Allocation Routines ************************************************/

   void *operator new(size_t bytes, const AstContext &context,
                      unsigned alignment = alignof(TypeRepr));

   void *operator new(size_t bytes, void *data) {
      assert(data);
      return data;
   }

   // Make placement new and vanilla new/delete illegal for TypeReprs.
   void *operator new(size_t bytes) = delete;
   void operator delete(void *data) = delete;

   void print(RawOutStream &outStream, const PrintOptions &opts = PrintOptions()) const;
   void print(AstPrinter &printer, const PrintOptions &opts) const;
   void dump() const;

   /// Clone the given type representation.
   TypeRepr *clone(const AstContext &ctx) const;
};

/// A TypeRepr for a type with a syntax error.  Can be used both as a
/// top-level TypeRepr and as a part of other TypeRepr.
///
/// The client should make sure to emit a diagnostic at the construction time
/// (in the parser).  All uses of this type should be ignored and not
/// re-diagnosed.
class ErrorTypeRepr : public TypeRepr
{
   SourceRange m_range;

public:
   ErrorTypeRepr() : TypeRepr(TypeReprKind::Error)
   {}

   ErrorTypeRepr(SourceLoc m_loc)
      : TypeRepr(TypeReprKind::Error),
        m_range(m_loc)
   {}

   ErrorTypeRepr(SourceRange range)
      : TypeRepr(TypeReprKind::Error),
        m_range(range)
   {}

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Error;
   }

   static bool classof(const ErrorTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_range.m_start;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_range.m_end;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A type with attributes.
/// \code
///   @convention(thin) Foo
/// \endcode
class AttributedTypeRepr : public TypeRepr
{
   // FIXME: TypeAttributes isn't a great use of space.
   TypeAttributes m_attrs;
   TypeRepr *m_type;

public:
   AttributedTypeRepr(const TypeAttributes &attrs, TypeRepr *type)
      : TypeRepr(TypeReprKind::Attributed),
        m_attrs(attrs),
        m_type(type)
   {}

   const TypeAttributes &getAttrs() const
   {
      return m_attrs;
   }

   void setAttrs(const TypeAttributes &attrs)
   {
      m_attrs = attrs;
   }

   TypeRepr *getTypeRepr() const
   {
      return m_type;
   }

   void printAttrs(RawOutStream &outStream) const;
   void printAttrs(AstPrinter &printer, const PrintOptions &options) const;

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Attributed;
   }

   static bool classof(const AttributedTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_attrs.m_atLoc;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_type->getEndLoc();
   }

   SourceLoc getLocImpl() const
   {
      return m_type->getLoc();
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

class ComponentIdentTypeRepr;

/// This is the abstract m_base class for types with identifier components.
/// \code
///   Foo.Bar<Gen>
/// \endcode
class IdentTypeRepr : public TypeRepr
{
protected:
   explicit IdentTypeRepr(TypeReprKind K) : TypeRepr(K) {}

public:
   /// Copies the provided array and creates a CompoundIdentTypeRepr or just
   /// returns the single entry in the array if it contains only one.
   static IdentTypeRepr *create(AstContext &context,
                                ArrayRef<ComponentIdentTypeRepr *> components);

   class ComponentRange;
   inline ComponentRange getComponentRange();

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::SimpleIdent  ||
            type->getKind() == TypeReprKind::GenericIdent ||
            type->getKind() == TypeReprKind::CompoundIdent;
   }
   static bool classof(const IdentTypeRepr *type)
   {
      return true;
   }
};

class ComponentIdentTypeRepr : public IdentTypeRepr
{
   SourceLoc m_m_loc;

   /// Either the identifier or declaration that describes this
   /// component.
   ///
   /// The initial parsed representation is always an identifier, and
   /// name binding will resolve this to a specific declaration.
   PointerUnion<Identifier, TypeDecl *> m_idOrDecl;

   /// The declaration context from which the bound declaration was
   /// found. only valid if m_idOrDecl is a TypeDecl.
   DeclContext *m_dc;

protected:
   ComponentIdentTypeRepr(TypeReprKind kind, SourceLoc m_loc, Identifier id)
      : IdentTypeRepr(kind),
        m_m_loc(m_loc),
        m_idOrDecl(id),
        m_dc(nullptr)
   {}

public:
   SourceLoc getIdLoc() const
   {
      return m_m_loc;
   }

   Identifier getIdentifier() const;

   /// Replace the identifier with a new identifier, e.g., due to typo
   /// correction.
   void overwriteIdentifier(Identifier newId)
   {
      m_idOrDecl = newId;
   }

   /// Return true if this has been name-bound already.
   bool isBound() const
   {
      return m_idOrDecl.is<TypeDecl *>();
   }

   TypeDecl *getBoundDecl() const
   {
      return m_idOrDecl.dynamicCast<TypeDecl*>();
   }

   DeclContext *getDeclContext() const
   {
      assert(isBound());
      return m_dc;
   }

   void setValue(TypeDecl *typeDecl, DeclContext *dc)
   {
      m_idOrDecl = typeDecl;
      this->m_dc = dc;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::SimpleIdent ||
            type->getKind() == TypeReprKind::GenericIdent;
   }

   static bool classof(const ComponentIdentTypeRepr *type)
   {
      return true;
   }

protected:
   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;

   SourceLoc getLocImpl() const
   {
      return m_m_loc;
   }

   friend class TypeRepr;
};

/// A simple identifier type like "Int".
class SimpleIdentTypeRepr : public ComponentIdentTypeRepr
{
public:
   SimpleIdentTypeRepr(SourceLoc m_loc, Identifier id)
      : ComponentIdentTypeRepr(TypeReprKind::SimpleIdent, m_loc, id)
   {}

   // SmallVector::emplace_back will never need to call this because
   // we reserve the right size, but it does try statically.
   SimpleIdentTypeRepr(const SimpleIdentTypeRepr &repr)
      : SimpleIdentTypeRepr(repr.getLoc(), repr.getIdentifier())
   {
      polar_unreachable("should not be called dynamically");
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::SimpleIdent;
   }
   static bool classof(const SimpleIdentTypeRepr *type) { return true; }

private:
   SourceLoc getStartLocImpl() const
   {
      return getIdLoc();
   }

   SourceLoc getEndLocImpl() const
   {
      return getIdLoc();
   }

   friend class TypeRepr;
};

/// An identifier type with generic arguments.
/// \code
///   Bar<Gen>
/// \endcode
class GenericIdentTypeRepr final : public ComponentIdentTypeRepr,
      private TrailingObjects<GenericIdentTypeRepr, TypeRepr *>
{
   friend class TrailingObjects;
   SourceRange m_angleBrackets;

   GenericIdentTypeRepr(SourceLoc m_loc, Identifier id,
                        ArrayRef<TypeRepr*> genericArgs,
                        SourceRange angleBrackets)
      : ComponentIdentTypeRepr(TypeReprKind::GenericIdent, m_loc, id),
        m_angleBrackets(angleBrackets)
   {
      bits.GenericIdentTypeRepr.NumGenericArgs = genericArgs.getSize();
      assert(!genericArgs.empty());
#ifndef NDEBUG
      for (auto arg : genericArgs) {
         assert(arg != nullptr);
      }
#endif
      std::uninitialized_copy(genericArgs.begin(), genericArgs.end(),
                              getTrailingObjects<TypeRepr*>());
   }

public:
   static GenericIdentTypeRepr *create(const AstContext &context,
                                       SourceLoc m_loc,
                                       Identifier id,
                                       ArrayRef<TypeRepr*> genericArgs,
                                       SourceRange angleBrackets);

   ArrayRef<TypeRepr*> getGenericArgs() const
   {
      return {getTrailingObjects<TypeRepr*>(),
               bits.GenericIdentTypeRepr.NumGenericArgs};
   }

   SourceRange getAngleBrackets() const
   {
      return m_angleBrackets;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::GenericIdent;
   }

   static bool classof(const GenericIdentTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return getIdLoc();
   }

   SourceLoc getEndLocImpl() const
   {
      return m_angleBrackets.m_end;
   }

   friend class TypeRepr;
};

/// A type with identifier components.
/// \code
///   Foo.Bar<Gen>
/// \endcode
class CompoundIdentTypeRepr final : public IdentTypeRepr,
      private TrailingObjects<CompoundIdentTypeRepr,
      ComponentIdentTypeRepr *>
{
   friend class TrailingObjects;

   CompoundIdentTypeRepr(ArrayRef<ComponentIdentTypeRepr *> components)
      : IdentTypeRepr(TypeReprKind::CompoundIdent)
   {
      bits.CompoundIdentTypeRepr.NumComponents = components.getSize();
      assert(components.getSize() > 1 &&
             "should have just used the single ComponentIdentTypeRepr directly");
      std::uninitialized_copy(components.begin(), components.end(),
                              getTrailingObjects<ComponentIdentTypeRepr*>());
   }

public:
   static CompoundIdentTypeRepr *create(const AstContext &ctx,
                                        ArrayRef<ComponentIdentTypeRepr*> components);

   ArrayRef<ComponentIdentTypeRepr*> getComponents() const
   {
      return {getTrailingObjects<ComponentIdentTypeRepr*>(),
               bits.CompoundIdentTypeRepr.NumComponents};
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::CompoundIdent;
   }

   static bool classof(const CompoundIdentTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return getComponents().front()->getStartLoc();
   }

   SourceLoc getEndLocImpl() const
   {
      return getComponents().back()->getEndLoc();
   }

   SourceLoc getLocImpl() const
   {
      return getComponents().back()->getLoc();
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// This wraps an IdentTypeRepr and provides an Iterator interface for the
/// components (or the single component) it represents.
class IdentTypeRepr::ComponentRange
{
   IdentTypeRepr *m_idType;

public:
   explicit ComponentRange(IdentTypeRepr *type)
      : m_idType(type)
   {}

   typedef ComponentIdentTypeRepr * const* Iterator;

   Iterator begin() const
   {
      //      if (isa<ComponentIdentTypeRepr>(m_idType)) {
      //         return reinterpret_cast<Iterator>(&m_idType);
      //      }
      //      return cast<CompoundIdentTypeRepr>(m_idType)->getComponents().begin();
   }

   Iterator end() const
   {
      //      if (isa<ComponentIdentTypeRepr>(m_idType)) {
      //         return reinterpret_cast<Iterator>(&m_idType) + 1;
      //      }
      //      return cast<CompoundIdentTypeRepr>(m_idType)->getComponents().end();
   }

   bool empty() const { return begin() == end(); }

   ComponentIdentTypeRepr *front() const
   {
      return *begin();
   }

   ComponentIdentTypeRepr *back() const
   {
      return *(end()-1);
   }
};

inline IdentTypeRepr::ComponentRange IdentTypeRepr::getComponentRange()
{
   return ComponentRange(this);
}

/// A function type.
/// \code
///   (Foo) -> Bar
///   (Foo, Bar) -> Baz
///   (x: Foo, y: Bar) -> Baz
/// \endcode
class FunctionTypeRepr : public TypeRepr
{
   // These three are only used in SIL mode, which is the only time
   // we can have polymorphic function values.
   GenericParamList *m_genericParams;
   GenericEnvironment *m_genericEnv;

   TupleTypeRepr *m_argsType;
   TypeRepr *m_retType;
   SourceLoc m_arrowLoc;
   SourceLoc m_throwsLoc;

public:
   FunctionTypeRepr(GenericParamList *genericParams, TupleTypeRepr *argsTy,
                    SourceLoc throwsLoc, SourceLoc arrowLoc, TypeRepr *retTy)
      : TypeRepr(TypeReprKind::Function),
        m_genericParams(genericParams),
        m_genericEnv(nullptr),
        m_argsType(argsTy),
        m_retType(retTy),
        m_arrowLoc(arrowLoc),
        m_throwsLoc(throwsLoc)
   {}

   GenericParamList *getGenericParams() const
   {
      return m_genericParams;
   }

   GenericEnvironment *getGenericEnvironment() const
   {
      return m_genericEnv;
   }

   void setGenericEnvironment(GenericEnvironment *genericEnv)
   {
      assert(m_genericEnv == nullptr);
      m_genericEnv = genericEnv;
   }

   TupleTypeRepr *getArgsTypeRepr() const
   {
      return m_argsType;
   }

   TypeRepr *getResultTypeRepr() const
   {
      return m_retType;
   }

   bool throws() const
   {
      return m_throwsLoc.isValid();
   }

   SourceLoc getArrowLoc() const
   {
      return m_arrowLoc;
   }

   SourceLoc getThrowsLoc() const
   {
      return m_throwsLoc;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Function;
   }

   static bool classof(const FunctionTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const;
   SourceLoc getEndLocImpl() const
   {
      return m_retType->getEndLoc();
   }

   SourceLoc getLocImpl() const
   {
      return m_arrowLoc;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An array type.
/// \code
///   [Foo]
/// \endcode
class ArrayTypeRepr : public TypeRepr
{
   TypeRepr *m_m_base;
   SourceRange m_m_brackets;

public:
   ArrayTypeRepr(TypeRepr *m_base, SourceRange m_brackets)
      : TypeRepr(TypeReprKind::Array),
        m_m_base(m_base),
        m_m_brackets(m_brackets)
   {}

   TypeRepr *getBase() const
   {
      return m_m_base;
   }

   SourceRange getBrackets() const
   {
      return m_m_brackets;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Array;
   }

   static bool classof(const ArrayTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_m_brackets.m_start;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_m_brackets.m_end;
   }
   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A dictionary type.
/// \code
///   [K : V]
/// \endcode
class DictionaryTypeRepr : public TypeRepr
{
   TypeRepr *m_key;
   TypeRepr *m_value;
   SourceLoc m_colonLoc;
   SourceRange m_brackets;

public:
   DictionaryTypeRepr(TypeRepr *key, TypeRepr *value,
                      SourceLoc colonLoc, SourceRange brackets)
      : TypeRepr(TypeReprKind::Dictionary),
        m_key(key),
        m_value(value),
        m_colonLoc(colonLoc),
        m_brackets(brackets)
   {}

   TypeRepr *getKey() const
   {
      return m_key;
   }

   TypeRepr *getValue() const
   {
      return m_value;
   }

   SourceRange getBrackets() const
   {
      return m_brackets;
   }

   SourceLoc getColonLoc() const
   {
      return m_colonLoc;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Dictionary;
   }

   static bool classof(const DictionaryTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_brackets.m_start;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_brackets.m_end;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An optional type.
/// \code
///   Foo?
/// \endcode
class OptionalTypeRepr : public TypeRepr
{
   TypeRepr *m_base;
   SourceLoc m_questionLoc;

public:
   OptionalTypeRepr(TypeRepr *base, SourceLoc question)
      : TypeRepr(TypeReprKind::Optional),
        m_base(base),
        m_questionLoc(question)
   {
   }

   TypeRepr *getBase() const
   {
      return m_base;
   }

   SourceLoc getQuestionLoc() const
   {
      return m_questionLoc;
   }

   static bool classof(const TypeRepr *type) {
      return type->getKind() == TypeReprKind::Optional;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_base->getStartLoc();
   }

   SourceLoc getEndLocImpl() const
   {
      return m_questionLoc.isValid() ? m_questionLoc : m_base->getEndLoc();
   }

   SourceLoc getLocImpl() const
   {
      return m_questionLoc.isValid() ? m_questionLoc : m_base->getLoc();
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An implicitly unwrapped optional type.
/// \code
///   Foo!
/// \endcode
class ImplicitlyUnwrappedOptionalTypeRepr : public TypeRepr
{
   TypeRepr *m_base;
   SourceLoc m_exclamationLoc;

public:
   ImplicitlyUnwrappedOptionalTypeRepr(TypeRepr *m_base, SourceLoc Exclamation)
      : TypeRepr(TypeReprKind::ImplicitlyUnwrappedOptional),
        m_base(m_base),
        m_exclamationLoc(Exclamation)
   {}

   TypeRepr *getBase() const
   {
      return m_base;
   }

   SourceLoc getExclamationLoc() const
   {
      return m_exclamationLoc;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::ImplicitlyUnwrappedOptional;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_base->getStartLoc();
   }

   SourceLoc getEndLocImpl() const
   {
      return m_exclamationLoc;
   }

   SourceLoc getLocImpl() const
   {
      return m_exclamationLoc;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A parsed element within a tuple type.
struct TupleTypeReprElement
{
   Identifier Name;
   SourceLoc NameLoc;
   Identifier SecondName;
   SourceLoc SecondNameLoc;
   SourceLoc UnderscoreLoc;
   SourceLoc m_colonLoc;
   TypeRepr *Type;
   SourceLoc TrailingCommaLoc;

   TupleTypeReprElement() {}
   TupleTypeReprElement(TypeRepr *Type): Type(Type) {}
};

/// A tuple type.
/// \code
///   (Foo, Bar)
///   (x: Foo)
///   (_ x: Foo)
/// \endcode
class TupleTypeRepr final : public TypeRepr,
      private TrailingObjects<TupleTypeRepr, TupleTypeReprElement,
      std::pair<SourceLoc, unsigned>>
{
   friend class TrailingObjects;
   typedef std::pair<SourceLoc, unsigned> SourceLocAndIdx;

   SourceRange m_parens;

   size_t getNumTrailingObjects(OverloadToken<TupleTypeReprElement>) const
   {
      return bits.TupleTypeRepr.NumElements;
   }

   TupleTypeRepr(ArrayRef<TupleTypeReprElement> elements,
                 SourceRange m_parens, SourceLoc Ellipsis, unsigned ellipsisIdx);

public:
   unsigned getNumElements() const
   {
      return bits.TupleTypeRepr.NumElements;
   }

   bool hasElementNames() const
   {
      for (auto &Element : getElements()) {
         if (Element.NameLoc.isValid()) {
            return true;
         }
      }
      return false;
   }

   ArrayRef<TupleTypeReprElement> getElements() const
   {
      return { getTrailingObjects<TupleTypeReprElement>(),
               bits.TupleTypeRepr.NumElements };
   }

   void getElementTypes(SmallVectorImpl<TypeRepr *> &Types) const
   {
      for (auto &Element : getElements()) {
         Types.push_back(Element.Type);
      }
   }

   TypeRepr *getElementType(unsigned i) const
   {
      return getElement(i).Type;
   }

   TupleTypeReprElement getElement(unsigned i) const
   {
      return getElements()[i];
   }

   void getElementNames(SmallVectorImpl<Identifier> &Names)
   {
      for (auto &Element : getElements()) {
         Names.push_back(Element.Name);
      }
   }

   Identifier getElementName(unsigned i) const
   {
      return getElement(i).Name;
   }

   SourceLoc getElementNameLoc(unsigned i) const
   {
      return getElement(i).NameLoc;
   }

   SourceLoc getUnderscoreLoc(unsigned i) const
   {
      return getElement(i).UnderscoreLoc;
   }

   bool isNamedParameter(unsigned i) const
   {
      return getUnderscoreLoc(i).isValid();
   }

   SourceRange getParens() const
   {
      return m_parens;
   }

   bool hasEllipsis() const
   {
      return bits.TupleTypeRepr.HasEllipsis;
   }

   SourceLoc getEllipsisLoc() const
   {
      return hasEllipsis() ?
               getTrailingObjects<SourceLocAndIdx>()[0].first : SourceLoc();
      }

      unsigned getEllipsisIndex() const {
      return hasEllipsis() ?
      getTrailingObjects<SourceLocAndIdx>()[0].second :
      bits.TupleTypeRepr.NumElements;
   }

   void removeEllipsis()
   {
      if (hasEllipsis()) {
         bits.TupleTypeRepr.HasEllipsis = false;
         getTrailingObjects<SourceLocAndIdx>()[0] = {
            SourceLoc(),
            getNumElements()
         };
      }
   }

   bool isParenType() const
   {
      return bits.TupleTypeRepr.NumElements == 1 &&
            getElementNameLoc(0).isInvalid() &&
            !hasEllipsis();
   }

   static TupleTypeRepr *create(const AstContext &context,
                                ArrayRef<TupleTypeReprElement> elements,
                                SourceRange parens,
                                SourceLoc Ellipsis, unsigned ellipsisIdx);
   static TupleTypeRepr *create(const AstContext &context,
                                ArrayRef<TupleTypeReprElement> elements,
                                SourceRange m_parens)
   {
      return create(context, elements, m_parens,
                    SourceLoc(), elements.getSize());
   }

   static TupleTypeRepr *createEmpty(const AstContext &context, SourceRange m_parens);

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Tuple;
   }

   static bool classof(const TupleTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_parens.m_start;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_parens.m_end;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A type composite type.
/// \code
///   Foo & Bar
/// \endcode
class CompositionTypeRepr final : public TypeRepr,
      private TrailingObjects<CompositionTypeRepr, TypeRepr*>
{
   friend class TrailingObjects;
   SourceLoc m_firstTypeLoc;
   SourceRange m_compositionRange;

   CompositionTypeRepr(ArrayRef<TypeRepr *> Types,
                       SourceLoc m_firstTypeLoc,
                       SourceRange m_compositionRange)
      : TypeRepr(TypeReprKind::Composition), m_firstTypeLoc(m_firstTypeLoc),
        m_compositionRange(m_compositionRange)
   {
      bits.CompositionTypeRepr.NumTypes = Types.getSize();
      std::uninitialized_copy(Types.begin(), Types.end(),
                              getTrailingObjects<TypeRepr*>());
   }

public:
   ArrayRef<TypeRepr *> getTypes() const
   {
      return {getTrailingObjects<TypeRepr*>(), bits.CompositionTypeRepr.NumTypes};
   }

   SourceLoc getSourceLoc() const
   {
      return m_firstTypeLoc;
   }

   SourceRange getCompositionRange() const
   {
      return m_compositionRange;
   }

   static CompositionTypeRepr *create(const AstContext &context,
                                      ArrayRef<TypeRepr*> m_protocols,
                                      SourceLoc firstTypeLoc,
                                      SourceRange compositionRange);

   static CompositionTypeRepr *createEmptyComposition(AstContext &context,
                                                      SourceLoc anyLoc)
   {
      return CompositionTypeRepr::create(context, {}, anyLoc, {anyLoc, anyLoc});
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Composition;
   }

   static bool classof(const CompositionTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_firstTypeLoc;
   }

   SourceLoc getLocImpl() const
   {
      return m_compositionRange.m_start;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_compositionRange.m_end;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A 'metatype' type.
/// \code
///   Foo.Type
/// \endcode
class MetatypeTypeRepr : public TypeRepr {
   TypeRepr *m_base;
   SourceLoc m_metaLoc;

public:
   MetatypeTypeRepr(TypeRepr *base, SourceLoc metaLoc)
      : TypeRepr(TypeReprKind::Metatype),
        m_base(base),
        m_metaLoc(metaLoc) {
   }

   TypeRepr *getBase() const
   {
      return m_base;
   }

   SourceLoc getMetaLoc() const
   {
      return m_metaLoc;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Metatype;
   }

   static bool classof(const MetatypeTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_base->getStartLoc();
   }

   SourceLoc getEndLocImpl() const
   {
      return m_metaLoc;
   }

   SourceLoc getLocImpl() const
   {
      return m_metaLoc;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A 'protocol' type.
/// \code
///   Foo.Protocol
/// \endcode
class ProtocolTypeRepr : public TypeRepr
{
   TypeRepr *m_base;
   SourceLoc m_protocolLoc;

public:
   ProtocolTypeRepr(TypeRepr *base, SourceLoc protocolLoc)
      : TypeRepr(TypeReprKind::Protocol),
        m_base(base),
        m_protocolLoc(protocolLoc)
   {
   }

   TypeRepr *getBase() const
   {
      return m_base;
   }

   SourceLoc getProtocolLoc() const
   {
      return m_protocolLoc;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Protocol;
   }

   static bool classof(const ProtocolTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_base->getStartLoc();
   }

   SourceLoc getEndLocImpl() const
   {
      return m_protocolLoc;
   }

   SourceLoc getLocImpl() const
   {
      return m_protocolLoc;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

class SpecifierTypeRepr : public TypeRepr
{
   TypeRepr *m_base;
   SourceLoc m_specifierLoc;

public:
   SpecifierTypeRepr(TypeReprKind Kind, TypeRepr *base, SourceLoc loc)
      : TypeRepr(Kind),
        m_base(base),
        m_specifierLoc(loc)
   {
   }

   TypeRepr *getBase() const
   {
      return m_base;
   }

   SourceLoc getSpecifierLoc() const
   {
      return m_specifierLoc;
   }

   static bool classof(const TypeRepr *type) {
      return type->getKind() == TypeReprKind::InOut ||
            type->getKind() == TypeReprKind::Shared ||
            type->getKind() == TypeReprKind::Owned;
   }
   static bool classof(const SpecifierTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_specifierLoc;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_base->getEndLoc();
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An 'inout' type.
/// \code
///   x : inout Int
/// \endcode
class InOutTypeRepr : public SpecifierTypeRepr
{
public:
   InOutTypeRepr(TypeRepr *m_base, SourceLoc inOutLoc)
      : SpecifierTypeRepr(TypeReprKind::InOut, m_base, inOutLoc)
   {}

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::InOut;
   }

   static bool classof(const InOutTypeRepr *type)
   {
      return true;
   }
};

/// A 'shared' type.
/// \code
///   x : shared Int
/// \endcode
class SharedTypeRepr : public SpecifierTypeRepr
{
public:
   SharedTypeRepr(TypeRepr *m_base, SourceLoc sharedLoc)
      : SpecifierTypeRepr(TypeReprKind::Shared, m_base, sharedLoc)
   {}

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Shared;
   }

   static bool classof(const SharedTypeRepr *type)
   {
      return true;
   }
};

/// A 'owned' type.
/// \code
///   x : owned Int
/// \endcode
class OwnedTypeRepr : public SpecifierTypeRepr
{
public:
   OwnedTypeRepr(TypeRepr *m_base, SourceLoc OwnedLoc)
      : SpecifierTypeRepr(TypeReprKind::Owned, m_base, OwnedLoc)
   {}

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Owned;
   }

   static bool classof(const OwnedTypeRepr *type)
   {
      return true;
   }
};

/// A TypeRepr for a known, fixed type.
///
/// Fixed type representations should be used sparingly, in places
/// where we need to specify some type (usually some built-in type)
/// that cannot be spelled in the language proper.
class FixedTypeRepr : public TypeRepr
{
   Type m_type;
   SourceLoc m_loc;

public:
   FixedTypeRepr(Type type, SourceLoc loc)
      : TypeRepr(TypeReprKind::Fixed),
        m_type(type),
        m_loc(loc)
   {}

   // SmallVector::emplace_back will never need to call this because
   // we reserve the right size, but it does try statically.
   FixedTypeRepr(const FixedTypeRepr &repr)
      : FixedTypeRepr(repr.m_type, repr.m_loc)
   {
      polar_unreachable("should not be called dynamically");
   }

   /// Retrieve the location.
   SourceLoc getLoc() const
   {
      return m_loc;
   }

   /// Retrieve the fixed type.
   Type getType() const
   {
      return m_type;
   }

   static bool classof(const TypeRepr *type)
   {
      return type->getKind() == TypeReprKind::Fixed;
   }

   static bool classof(const FixedTypeRepr *type)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_loc;
   }

   SourceLoc getEndLocImpl() const
   {
      return m_loc;
   }

   void printImpl(AstPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

inline bool TypeRepr::isSimple() const
{
   // NOTE: Please keep this logic in sync with TypeBase::hasSimpleTypeRepr().
   switch (getKind()) {
   case TypeReprKind::Attributed:
   case TypeReprKind::Error:
   case TypeReprKind::Function:
   case TypeReprKind::InOut:
   case TypeReprKind::Composition:
      return false;
   case TypeReprKind::SimpleIdent:
   case TypeReprKind::GenericIdent:
   case TypeReprKind::CompoundIdent:
   case TypeReprKind::Metatype:
   case TypeReprKind::Protocol:
   case TypeReprKind::Dictionary:
   case TypeReprKind::Optional:
   case TypeReprKind::ImplicitlyUnwrappedOptional:
   case TypeReprKind::Tuple:
   case TypeReprKind::Fixed:
   case TypeReprKind::Array:
   case TypeReprKind::Shared:
   case TypeReprKind::Owned:
      return true;
   }
   polar_unreachable("bad TypeRepr kind");
}


} // polar::ast

#endif // POLARPHP_AST_TYPEREPR_H
