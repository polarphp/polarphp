//===--- TypeRepr.h - Swift Language Type Representation --------*- C++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/28.
//===----------------------------------------------------------------------===//
//
// This file defines the TypeRepr and related classes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_TYPEREPR_H
#define POLARPHP_AST_TYPEREPR_H

#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/ast/Type.h"
#include "polarphp/ast/TypeAlignments.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/STLExtras.h"
#include "polarphp/basic/InlineBitfield.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/TrailingObjects.h"

namespace polar::ast {

class AstWalker;
class DeclContext;
class GenericEnvironment;
class IdentTypeRepr;
class TupleTypeRepr;
class TypeDecl;

using polar::basic::bitmax;
using polar::basic::count_bits_used;

enum class TypeReprKind : std::uint8_t
{
#define TYPEREPR(ID, PARENT) ID,
#define LAST_TYPEREPR(ID) Last_TypeRepr = ID,
#include "polarphp/ast/TypeReprNodesDef.h"
};
enum : unsigned
{
   NumTypeReprKindBits =
   count_bits_used(static_cast<unsigned>(TypeReprKind::Last_TypeRepr))
};

/// Representation of a type as written in source.
class /*alignas(8)*/ TypeRepr
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

      POLAR_INLINE_BITFIELD_FULL(
            TupleTypeRepr, TypeRepr, 1+32,
            /// Whether this tuple has '...' and its position.
            HasEllipsis : 1,
            : NumPadBits,
            /// The number of elements contained.
            NumElements : 32
            );

      POLAR_INLINE_BITFIELD_EMPTY(IdentTypeRepr, TypeRepr);
      POLAR_INLINE_BITFIELD_EMPTY(ComponentIdentTypeRepr, IdentTypeRepr);

      POLAR_INLINE_BITFIELD_FULL(
            GenericIdentTypeRepr, ComponentIdentTypeRepr, 32,
            : NumPadBits,
            NumGenericArgs : 32
            );

      POLAR_INLINE_BITFIELD_FULL(
            CompoundIdentTypeRepr, IdentTypeRepr, 32,
            : NumPadBits,
            NumComponents : 32
            );

      POLAR_INLINE_BITFIELD_FULL(
            CompositionTypeRepr, TypeRepr, 32,
            : NumPadBits,
            NumTypes : 32
            );

      POLAR_INLINE_BITFIELD_FULL(
            PILBoxTypeRepr, TypeRepr, 32,
            NumGenericArgs : NumPadBits,
            NumFields : 32
            );

   } m_bits;

   TypeRepr(TypeReprKind kind)
   {
      m_bits.OpaqueBits = 0;
      m_bits.TypeRepr.Kind = static_cast<unsigned>(kind);
      m_bits.TypeRepr.Invalid = false;
      m_bits.TypeRepr.Warned = false;
   }

private:
   SourceLoc getLocImpl() const
   {
      return getStartLoc();
   }

public:
   TypeReprKind getKind() const
   {
      return static_cast<TypeReprKind>(m_bits.TypeRepr.Kind);
   }

   /// Is this type representation known to be invalid?
   bool isInvalid() const
   {
      return m_bits.TypeRepr.Invalid;
   }

   /// Note that this type representation describes an invalid type.
   void setInvalid()
   {
      m_bits.TypeRepr.Invalid = true;
   }

   /// If a warning is produced about this type repr, keep track of that so we
   /// don't emit another one upon further reanalysis.
   bool isWarnedAbout() const
   {
      return m_bits.TypeRepr.Warned;
   }

   void setWarned()
   {
      m_bits.TypeRepr.Warned = true;
   }

   /// Get the representative location for pointing at this type.
   SourceLoc getLoc() const;

   SourceLoc getStartLoc() const;
   SourceLoc getEndLoc() const;
   SourceRange getSourceRange() const;

   /// Is this type grammatically a type-simple?
   inline bool isSimple() const; // bottom of this file

   static bool classof(const TypeRepr *typeRepr)
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

   void *operator new(size_t bytes, void *data)
   {
      assert(data);
      return data;
   }

   // Make placement new and vanilla new/delete illegal for TypeReprs.
   void *operator new(size_t bytes) = delete;
   void operator delete(void *data) = delete;

   void print(raw_ostream &OS, const PrintOptions &opts = PrintOptions()) const;
   void print(ASTPrinter &printer, const PrintOptions &opts) const;
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
   ErrorTypeRepr() : TypeRepr(TypeReprKind::Error) {}
   ErrorTypeRepr(SourceLoc loc)
      : TypeRepr(TypeReprKind::Error),
        m_range(loc) {}
   ErrorTypeRepr(SourceRange range)
      : TypeRepr(TypeReprKind::Error),
        m_range(range)
   {}

   static bool classof(const TypeRepr *typeRepr)
   {
      return typeRepr->getKind() == TypeReprKind::Error;
   }

   static bool classof(const ErrorTypeRepr *typeRepr)
   {
      return true;
   }

private:
   SourceLoc getStartLocImpl() const
   {
      return m_range.getStart();
   }

   SourceLoc getEndLocImpl() const
   {
      return m_range.getEnd();
   }

   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A type with attributes.
/// \code
///   @convention(thin) Foo
/// \endcode
class AttributedTypeRepr : public TypeRepr
{
   // FIXME: TypeAttributes isn't a great use of space.
   TypeAttributes Attrs;
   TypeRepr *Ty;

public:
   AttributedTypeRepr(const TypeAttributes &Attrs, TypeRepr *Ty)
      : TypeRepr(TypeReprKind::Attributed), Attrs(Attrs), Ty(Ty) {
   }

   const TypeAttributes &getAttrs() const { return Attrs; }
   void setAttrs(const TypeAttributes &attrs) { Attrs = attrs; }
   TypeRepr *getTypeRepr() const { return Ty; }

   void printAttrs(llvm::raw_ostream &OS) const;
   void printAttrs(ASTPrinter &printer, const PrintOptions &Options) const;

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Attributed;
   }
   static bool classof(const AttributedTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return Attrs.AtLoc; }
   SourceLoc getEndLocImpl() const { return Ty->getEndLoc(); }
   SourceLoc getLocImpl() const { return Ty->getLoc(); }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

class ComponentIdentTypeRepr;

/// This is the abstract base class for types with identifier components.
/// \code
///   Foo.Bar<Gen>
/// \endcode
class IdentTypeRepr : public TypeRepr {
protected:
   explicit IdentTypeRepr(TypeReprKind K) : TypeRepr(K) {}

public:
   /// Copies the provided array and creates a CompoundIdentTypeRepr or just
   /// returns the single entry in the array if it contains only one.
   static IdentTypeRepr *create(AstContext &C,
                                ArrayRef<ComponentIdentTypeRepr *> Components);

   class ComponentRange;
   inline ComponentRange getComponentRange();

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::SimpleIdent  ||
            T->getKind() == TypeReprKind::GenericIdent ||
            T->getKind() == TypeReprKind::CompoundIdent;
   }
   static bool classof(const IdentTypeRepr *T) { return true; }
};

class ComponentIdentTypeRepr : public IdentTypeRepr {
   SourceLoc Loc;

   /// Either the identifier or declaration that describes this
   /// component.
   ///
   /// The initial parsed representation is always an identifier, and
   /// name binding will resolve this to a specific declaration.
   llvm::PointerUnion<Identifier, TypeDecl *> IdOrDecl;

   /// The declaration context from which the bound declaration was
   /// found. only valid if IdOrDecl is a TypeDecl.
   DeclContext *DC;

protected:
   ComponentIdentTypeRepr(TypeReprKind K, SourceLoc Loc, Identifier Id)
      : IdentTypeRepr(K), Loc(Loc), IdOrDecl(Id), DC(nullptr) {}

public:
   SourceLoc getIdLoc() const { return Loc; }
   Identifier getIdentifier() const;

   /// Replace the identifier with a new identifier, e.g., due to typo
   /// correction.
   void overwriteIdentifier(Identifier newId) { IdOrDecl = newId; }

   /// Return true if this has been name-bound already.
   bool isBound() const { return IdOrDecl.is<TypeDecl *>(); }

   TypeDecl *getBoundDecl() const { return IdOrDecl.dyn_cast<TypeDecl*>(); }

   DeclContext *getDeclContext() const {
      assert(isBound());
      return DC;
   }

   void setValue(TypeDecl *TD, DeclContext *DC) {
      IdOrDecl = TD;
      this->DC = DC;
   }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::SimpleIdent ||
            T->getKind() == TypeReprKind::GenericIdent;
   }
   static bool classof(const ComponentIdentTypeRepr *T) { return true; }

protected:
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;

   SourceLoc getLocImpl() const { return Loc; }
   friend class TypeRepr;
};

/// A simple identifier type like "Int".
class SimpleIdentTypeRepr : public ComponentIdentTypeRepr {
public:
   SimpleIdentTypeRepr(SourceLoc Loc, Identifier Id)
      : ComponentIdentTypeRepr(TypeReprKind::SimpleIdent, Loc, Id) {}

   // SmallVector::emplace_back will never need to call this because
   // we reserve the right size, but it does try statically.
   SimpleIdentTypeRepr(const SimpleIdentTypeRepr &repr)
      : SimpleIdentTypeRepr(repr.getLoc(), repr.getIdentifier()) {
      llvm_unreachable("should not be called dynamically");
   }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::SimpleIdent;
   }
   static bool classof(const SimpleIdentTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return getIdLoc(); }
   SourceLoc getEndLocImpl() const { return getIdLoc(); }
   friend class TypeRepr;
};

/// An identifier type with generic arguments.
/// \code
///   Bar<Gen>
/// \endcode
class GenericIdentTypeRepr final : public ComponentIdentTypeRepr,
      private llvm::TrailingObjects<GenericIdentTypeRepr, TypeRepr *> {
   friend TrailingObjects;
   SourceRange AngleBrackets;

   GenericIdentTypeRepr(SourceLoc Loc, Identifier Id,
                        ArrayRef<TypeRepr*> GenericArgs,
                        SourceRange AngleBrackets)
      : ComponentIdentTypeRepr(TypeReprKind::GenericIdent, Loc, Id),
        AngleBrackets(AngleBrackets) {
      m_bits.GenericIdentTypeRepr.NumGenericArgs = GenericArgs.size();
      assert(!GenericArgs.empty());
#ifndef NDEBUG
      for (auto arg : GenericArgs)
         assert(arg != nullptr);
#endif
      std::uninitialized_copy(GenericArgs.begin(), GenericArgs.end(),
                              getTrailingObjects<TypeRepr*>());
   }

public:
   static GenericIdentTypeRepr *create(const AstContext &C,
                                       SourceLoc Loc,
                                       Identifier Id,
                                       ArrayRef<TypeRepr*> GenericArgs,
                                       SourceRange AngleBrackets);

   unsigned getNumGenericArgs() const {
      return m_bits.GenericIdentTypeRepr.NumGenericArgs;
   }

   ArrayRef<TypeRepr*> getGenericArgs() const {
      return {getTrailingObjects<TypeRepr*>(),
               m_bits.GenericIdentTypeRepr.NumGenericArgs};
   }
   SourceRange getAngleBrackets() const { return AngleBrackets; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::GenericIdent;
   }
   static bool classof(const GenericIdentTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return getIdLoc(); }
   SourceLoc getEndLocImpl() const { return AngleBrackets.End; }
   friend class TypeRepr;
};

/// A type with identifier components.
/// \code
///   Foo.Bar<Gen>
/// \endcode
class CompoundIdentTypeRepr final : public IdentTypeRepr,
      private llvm::TrailingObjects<CompoundIdentTypeRepr,
      ComponentIdentTypeRepr *> {
   friend TrailingObjects;

   CompoundIdentTypeRepr(ArrayRef<ComponentIdentTypeRepr *> Components)
      : IdentTypeRepr(TypeReprKind::CompoundIdent) {
      m_bits.CompoundIdentTypeRepr.NumComponents = Components.size();
      assert(Components.size() > 1 &&
             "should have just used the single ComponentIdentTypeRepr directly");
      std::uninitialized_copy(Components.begin(), Components.end(),
                              getTrailingObjects<ComponentIdentTypeRepr*>());
   }

public:
   static CompoundIdentTypeRepr *create(const AstContext &Ctx,
                                        ArrayRef<ComponentIdentTypeRepr*> Components);

   ArrayRef<ComponentIdentTypeRepr*> getComponents() const {
      return {getTrailingObjects<ComponentIdentTypeRepr*>(),
               m_bits.CompoundIdentTypeRepr.NumComponents};
   }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::CompoundIdent;
   }
   static bool classof(const CompoundIdentTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const {
      return getComponents().front()->getStartLoc();
   }
   SourceLoc getEndLocImpl() const {
      return getComponents().back()->getEndLoc();
   }
   SourceLoc getLocImpl() const {
      return getComponents().back()->getLoc();
   }

   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// This wraps an IdentTypeRepr and provides an iterator interface for the
/// components (or the single component) it represents.
class IdentTypeRepr::ComponentRange {
   IdentTypeRepr *IdT;

public:
   explicit ComponentRange(IdentTypeRepr *T) : IdT(T) {}

   typedef ComponentIdentTypeRepr * const* iterator;

   iterator begin() const {
      if (isa<ComponentIdentTypeRepr>(IdT))
         return reinterpret_cast<iterator>(&IdT);
      return cast<CompoundIdentTypeRepr>(IdT)->getComponents().begin();
   }

   iterator end() const {
      if (isa<ComponentIdentTypeRepr>(IdT))
         return reinterpret_cast<iterator>(&IdT) + 1;
      return cast<CompoundIdentTypeRepr>(IdT)->getComponents().end();
   }

   bool empty() const { return begin() == end(); }

   ComponentIdentTypeRepr *front() const { return *begin(); }
   ComponentIdentTypeRepr *back() const { return *(end()-1); }
};

inline IdentTypeRepr::ComponentRange IdentTypeRepr::getComponentRange() {
   return ComponentRange(this);
}

/// A function type.
/// \code
///   (Foo) -> Bar
///   (Foo, Bar) -> Baz
///   (x: Foo, y: Bar) -> Baz
/// \endcode
class FunctionTypeRepr : public TypeRepr {
   // These three are only used in SIL mode, which is the only time
   // we can have polymorphic function values.
   GenericParamList *GenericParams;
   GenericEnvironment *GenericEnv;

   TupleTypeRepr *ArgsTy;
   TypeRepr *RetTy;
   SourceLoc ArrowLoc;
   SourceLoc ThrowsLoc;

public:
   FunctionTypeRepr(GenericParamList *genericParams, TupleTypeRepr *argsTy,
                    SourceLoc throwsLoc, SourceLoc arrowLoc, TypeRepr *retTy)
      : TypeRepr(TypeReprKind::Function),
        GenericParams(genericParams),
        GenericEnv(nullptr),
        ArgsTy(argsTy), RetTy(retTy),
        ArrowLoc(arrowLoc), ThrowsLoc(throwsLoc) {
   }

   GenericParamList *getGenericParams() const { return GenericParams; }
   GenericEnvironment *getGenericEnvironment() const { return GenericEnv; }

   void setGenericEnvironment(GenericEnvironment *genericEnv) {
      assert(GenericEnv == nullptr);
      GenericEnv = genericEnv;
   }

   TupleTypeRepr *getArgsTypeRepr() const { return ArgsTy; }
   TypeRepr *getResultTypeRepr() const { return RetTy; }
   bool throws() const { return ThrowsLoc.isValid(); }

   SourceLoc getArrowLoc() const { return ArrowLoc; }
   SourceLoc getThrowsLoc() const { return ThrowsLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Function;
   }
   static bool classof(const FunctionTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const;
   SourceLoc getEndLocImpl() const { return RetTy->getEndLoc(); }
   SourceLoc getLocImpl() const { return ArrowLoc; }

   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An array type.
/// \code
///   [Foo]
/// \endcode
class ArrayTypeRepr : public TypeRepr {
   TypeRepr *Base;
   SourceRange Brackets;

public:
   ArrayTypeRepr(TypeRepr *Base, SourceRange Brackets)
      : TypeRepr(TypeReprKind::Array), Base(Base), Brackets(Brackets) { }

   TypeRepr *getBase() const { return Base; }
   SourceRange getBrackets() const { return Brackets; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Array;
   }
   static bool classof(const ArrayTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return Brackets.Start; }
   SourceLoc getEndLocImpl() const { return Brackets.End; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A dictionary type.
/// \code
///   [K : V]
/// \endcode
class DictionaryTypeRepr : public TypeRepr {
   TypeRepr *Key;
   TypeRepr *Value;
   SourceLoc ColonLoc;
   SourceRange Brackets;

public:
   DictionaryTypeRepr(TypeRepr *key, TypeRepr *value,
                      SourceLoc colonLoc, SourceRange brackets)
      : TypeRepr(TypeReprKind::Dictionary), Key(key), Value(value),
        ColonLoc(colonLoc), Brackets(brackets) { }

   TypeRepr *getKey() const { return Key; }
   TypeRepr *getValue() const { return Value; }
   SourceRange getBrackets() const { return Brackets; }
   SourceLoc getColonLoc() const { return ColonLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Dictionary;
   }
   static bool classof(const DictionaryTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return Brackets.Start; }
   SourceLoc getEndLocImpl() const { return Brackets.End; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An optional type.
/// \code
///   Foo?
/// \endcode
class OptionalTypeRepr : public TypeRepr {
   TypeRepr *Base;
   SourceLoc QuestionLoc;

public:
   OptionalTypeRepr(TypeRepr *Base, SourceLoc Question)
      : TypeRepr(TypeReprKind::Optional), Base(Base), QuestionLoc(Question) {
   }

   TypeRepr *getBase() const { return Base; }
   SourceLoc getQuestionLoc() const { return QuestionLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Optional;
   }

private:
   SourceLoc getStartLocImpl() const { return Base->getStartLoc(); }
   SourceLoc getEndLocImpl() const {
      return QuestionLoc.isValid() ? QuestionLoc : Base->getEndLoc();
   }
   SourceLoc getLocImpl() const {
      return QuestionLoc.isValid() ? QuestionLoc : Base->getLoc();
   }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An implicitly unwrapped optional type.
/// \code
///   Foo!
/// \endcode
class ImplicitlyUnwrappedOptionalTypeRepr : public TypeRepr {
   TypeRepr *Base;
   SourceLoc ExclamationLoc;

public:
   ImplicitlyUnwrappedOptionalTypeRepr(TypeRepr *Base, SourceLoc Exclamation)
      : TypeRepr(TypeReprKind::ImplicitlyUnwrappedOptional),
        Base(Base),
        ExclamationLoc(Exclamation) {}

   TypeRepr *getBase() const { return Base; }
   SourceLoc getExclamationLoc() const { return ExclamationLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::ImplicitlyUnwrappedOptional;
   }

private:
   SourceLoc getStartLocImpl() const { return Base->getStartLoc(); }
   SourceLoc getEndLocImpl() const { return ExclamationLoc; }
   SourceLoc getLocImpl() const { return ExclamationLoc; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A parsed element within a tuple type.
struct TupleTypeReprElement {
   Identifier Name;
   SourceLoc NameLoc;
   Identifier SecondName;
   SourceLoc SecondNameLoc;
   SourceLoc UnderscoreLoc;
   SourceLoc ColonLoc;
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
      private llvm::TrailingObjects<TupleTypeRepr, TupleTypeReprElement,
      std::pair<SourceLoc, unsigned>> {
   friend TrailingObjects;
   typedef std::pair<SourceLoc, unsigned> SourceLocAndIdx;

   SourceRange Parens;

   size_t numTrailingObjects(OverloadToken<TupleTypeReprElement>) const {
      return m_bits.TupleTypeRepr.NumElements;
   }

   TupleTypeRepr(ArrayRef<TupleTypeReprElement> Elements,
                 SourceRange Parens, SourceLoc Ellipsis, unsigned EllipsisIdx);

public:
   unsigned getNumElements() const { return m_bits.TupleTypeRepr.NumElements; }
   bool hasElementNames() const {
      for (auto &Element : getElements()) {
         if (Element.NameLoc.isValid()) {
            return true;
         }
      }
      return false;
   }

   ArrayRef<TupleTypeReprElement> getElements() const {
      return { getTrailingObjects<TupleTypeReprElement>(),
               m_bits.TupleTypeRepr.NumElements };
   }

   void getElementTypes(SmallVectorImpl<TypeRepr *> &Types) const {
      for (auto &Element : getElements()) {
         Types.push_back(Element.Type);
      }
   }

   TypeRepr *getElementType(unsigned i) const {
      return getElement(i).Type;
   }

   TupleTypeReprElement getElement(unsigned i) const {
      return getElements()[i];
   }

   void getElementNames(SmallVectorImpl<Identifier> &Names) {
      for (auto &Element : getElements()) {
         Names.push_back(Element.Name);
      }
   }

   Identifier getElementName(unsigned i) const {
      return getElement(i).Name;
   }

   SourceLoc getElementNameLoc(unsigned i) const {
      return getElement(i).NameLoc;
   }

   SourceLoc getUnderscoreLoc(unsigned i) const {
      return getElement(i).UnderscoreLoc;
   }

   bool isNamedParameter(unsigned i) const {
      return getUnderscoreLoc(i).isValid();
   }

   SourceRange getParens() const { return Parens; }

   bool hasEllipsis() const {
      return m_bits.TupleTypeRepr.HasEllipsis;
   }

   SourceLoc getEllipsisLoc() const {
      return hasEllipsis() ?
               getTrailingObjects<SourceLocAndIdx>()[0].first : SourceLoc();
      }

      unsigned getEllipsisIndex() const {
      return hasEllipsis() ?
      getTrailingObjects<SourceLocAndIdx>()[0].second :
      m_bits.TupleTypeRepr.NumElements;
   }

   void removeEllipsis() {
      if (hasEllipsis()) {
         m_bits.TupleTypeRepr.HasEllipsis = false;
         getTrailingObjects<SourceLocAndIdx>()[0] = {
            SourceLoc(),
            getNumElements()
         };
      }
   }

   bool isParenType() const {
      return m_bits.TupleTypeRepr.NumElements == 1 &&
            getElementNameLoc(0).isInvalid() &&
            !hasEllipsis();
   }

   static TupleTypeRepr *create(const AstContext &C,
                                ArrayRef<TupleTypeReprElement> Elements,
                                SourceRange Parens,
                                SourceLoc Ellipsis, unsigned EllipsisIdx);
   static TupleTypeRepr *create(const AstContext &C,
                                ArrayRef<TupleTypeReprElement> Elements,
                                SourceRange Parens) {
      return create(C, Elements, Parens,
                    SourceLoc(), Elements.size());
   }
   static TupleTypeRepr *createEmpty(const AstContext &C, SourceRange Parens);

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Tuple;
   }
   static bool classof(const TupleTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return Parens.Start; }
   SourceLoc getEndLocImpl() const { return Parens.End; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A type composite type.
/// \code
///   Foo & Bar
/// \endcode
class CompositionTypeRepr final : public TypeRepr,
      private llvm::TrailingObjects<CompositionTypeRepr, TypeRepr*> {
   friend TrailingObjects;
   SourceLoc FirstTypeLoc;
   SourceRange CompositionRange;

   CompositionTypeRepr(ArrayRef<TypeRepr *> Types,
                       SourceLoc FirstTypeLoc,
                       SourceRange CompositionRange)
      : TypeRepr(TypeReprKind::Composition), FirstTypeLoc(FirstTypeLoc),
        CompositionRange(CompositionRange) {
      m_bits.CompositionTypeRepr.NumTypes = Types.size();
      std::uninitialized_copy(Types.begin(), Types.end(),
                              getTrailingObjects<TypeRepr*>());
   }

public:
   ArrayRef<TypeRepr *> getTypes() const {
      return {getTrailingObjects<TypeRepr*>(), m_bits.CompositionTypeRepr.NumTypes};
   }
   SourceLoc getSourceLoc() const { return FirstTypeLoc; }
   SourceRange getCompositionRange() const { return CompositionRange; }

   static CompositionTypeRepr *create(const AstContext &C,
                                      ArrayRef<TypeRepr*> Protocols,
                                      SourceLoc FirstTypeLoc,
                                      SourceRange CompositionRange);

   static CompositionTypeRepr *createEmptyComposition(AstContext &C,
                                                      SourceLoc AnyLoc) {
      return CompositionTypeRepr::create(C, {}, AnyLoc, {AnyLoc, AnyLoc});
   }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Composition;
   }
   static bool classof(const CompositionTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return FirstTypeLoc; }
   SourceLoc getLocImpl() const { return CompositionRange.Start; }
   SourceLoc getEndLocImpl() const { return CompositionRange.End; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A 'metatype' type.
/// \code
///   Foo.Type
/// \endcode
class MetatypeTypeRepr : public TypeRepr {
   TypeRepr *Base;
   SourceLoc MetaLoc;

public:
   MetatypeTypeRepr(TypeRepr *Base, SourceLoc MetaLoc)
      : TypeRepr(TypeReprKind::Metatype), Base(Base), MetaLoc(MetaLoc) {
   }

   TypeRepr *getBase() const { return Base; }
   SourceLoc getMetaLoc() const { return MetaLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Metatype;
   }
   static bool classof(const MetatypeTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return Base->getStartLoc(); }
   SourceLoc getEndLocImpl() const { return MetaLoc; }
   SourceLoc getLocImpl() const { return MetaLoc; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// A 'protocol' type.
/// \code
///   Foo.Protocol
/// \endcode
class ProtocolTypeRepr : public TypeRepr {
   TypeRepr *Base;
   SourceLoc ProtocolLoc;

public:
   ProtocolTypeRepr(TypeRepr *Base, SourceLoc ProtocolLoc)
      : TypeRepr(TypeReprKind::Protocol), Base(Base), ProtocolLoc(ProtocolLoc) {
   }

   TypeRepr *getBase() const { return Base; }
   SourceLoc getProtocolLoc() const { return ProtocolLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Protocol;
   }
   static bool classof(const ProtocolTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return Base->getStartLoc(); }
   SourceLoc getEndLocImpl() const { return ProtocolLoc; }
   SourceLoc getLocImpl() const { return ProtocolLoc; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

class SpecifierTypeRepr : public TypeRepr {
   TypeRepr *Base;
   SourceLoc SpecifierLoc;

public:
   SpecifierTypeRepr(TypeReprKind Kind, TypeRepr *Base, SourceLoc Loc)
      : TypeRepr(Kind), Base(Base), SpecifierLoc(Loc) {
   }

   TypeRepr *getBase() const { return Base; }
   SourceLoc getSpecifierLoc() const { return SpecifierLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::InOut ||
            T->getKind() == TypeReprKind::Shared ||
            T->getKind() == TypeReprKind::Owned;
   }
   static bool classof(const SpecifierTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return SpecifierLoc; }
   SourceLoc getEndLocImpl() const { return Base->getEndLoc(); }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// An 'inout' type.
/// \code
///   x : inout Int
/// \endcode
class InOutTypeRepr : public SpecifierTypeRepr {
public:
   InOutTypeRepr(TypeRepr *Base, SourceLoc InOutLoc)
      : SpecifierTypeRepr(TypeReprKind::InOut, Base, InOutLoc) {}

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::InOut;
   }
   static bool classof(const InOutTypeRepr *T) { return true; }
};

/// A 'shared' type.
/// \code
///   x : shared Int
/// \endcode
class SharedTypeRepr : public SpecifierTypeRepr {
public:
   SharedTypeRepr(TypeRepr *Base, SourceLoc SharedLoc)
      : SpecifierTypeRepr(TypeReprKind::Shared, Base, SharedLoc) {}

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Shared;
   }
   static bool classof(const SharedTypeRepr *T) { return true; }
};

/// A 'owned' type.
/// \code
///   x : owned Int
/// \endcode
class OwnedTypeRepr : public SpecifierTypeRepr {
public:
   OwnedTypeRepr(TypeRepr *Base, SourceLoc OwnedLoc)
      : SpecifierTypeRepr(TypeReprKind::Owned, Base, OwnedLoc) {}

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Owned;
   }
   static bool classof(const OwnedTypeRepr *T) { return true; }
};

/// A TypeRepr for a known, fixed type.
///
/// Fixed type representations should be used sparingly, in places
/// where we need to specify some type (usually some built-in type)
/// that cannot be spelled in the language proper.
class FixedTypeRepr : public TypeRepr {
   Type Ty;
   SourceLoc Loc;

public:
   FixedTypeRepr(Type Ty, SourceLoc Loc)
      : TypeRepr(TypeReprKind::Fixed), Ty(Ty), Loc(Loc) {}

   // SmallVector::emplace_back will never need to call this because
   // we reserve the right size, but it does try statically.
   FixedTypeRepr(const FixedTypeRepr &repr) : FixedTypeRepr(repr.Ty, repr.Loc) {
      llvm_unreachable("should not be called dynamically");
   }

   /// Retrieve the location.
   SourceLoc getLoc() const { return Loc; }

   /// Retrieve the fixed type.
   Type getType() const { return Ty; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::Fixed;
   }
   static bool classof(const FixedTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return Loc; }
   SourceLoc getEndLocImpl() const { return Loc; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

class SILBoxTypeReprField {
   SourceLoc VarOrLetLoc;
   llvm::PointerIntPair<TypeRepr*, 1, bool> FieldTypeAndMutable;

public:
   SILBoxTypeReprField(SourceLoc loc, bool isMutable, TypeRepr *fieldType)
      : VarOrLetLoc(loc), FieldTypeAndMutable(fieldType, isMutable) {
   }

   SourceLoc getLoc() const { return VarOrLetLoc; }
   TypeRepr *getFieldType() const { return FieldTypeAndMutable.getPointer(); }
   bool isMutable() const { return FieldTypeAndMutable.getInt(); }
};

/// TypeRepr for opaque return types.
///
/// This can occur in the return position of a function declaration, or the
/// top-level type of a property, to specify that the concrete return type
/// should be abstracted from callers, given a set of generic constraints that
/// the concrete return type satisfies:
///
/// func foo() -> some Collection { return [1,2,3] }
/// var bar: some SignedInteger = 1
///
/// It is currently illegal for this to appear in any other position.
class OpaqueReturnTypeRepr : public TypeRepr {
   /// The type repr for the immediate constraints on the opaque type.
   /// In valid code this must resolve to a class, protocol, or composition type.
   TypeRepr *Constraint;
   SourceLoc OpaqueLoc;

public:
   OpaqueReturnTypeRepr(SourceLoc opaqueLoc, TypeRepr *constraint)
      : TypeRepr(TypeReprKind::OpaqueReturn), Constraint(constraint),
        OpaqueLoc(opaqueLoc)
   {}

   TypeRepr *getConstraint() const { return Constraint; }
   SourceLoc getOpaqueLoc() const { return OpaqueLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::OpaqueReturn;
   }
   static bool classof(const OpaqueReturnTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const { return OpaqueLoc; }
   SourceLoc getEndLocImpl() const { return Constraint->getEndLoc(); }
   SourceLoc getLocImpl() const { return OpaqueLoc; }
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend class TypeRepr;
};

/// SIL-only TypeRepr for box types.
///
/// Boxes are either concrete: { var Int, let String }
/// or generic:                <T: Runcible> { var T, let String } <Int>
class SILBoxTypeRepr final : public TypeRepr,
      private llvm::TrailingObjects<SILBoxTypeRepr,
      SILBoxTypeReprField, TypeRepr *> {
   friend TrailingObjects;
   GenericParamList *GenericParams;
   GenericEnvironment *GenericEnv = nullptr;

   SourceLoc LBraceLoc, RBraceLoc;
   SourceLoc ArgLAngleLoc, ArgRAngleLoc;

   size_t numTrailingObjects(OverloadToken<SILBoxTypeReprField>) const {
      return m_bits.SILBoxTypeRepr.NumFields;
   }
   size_t numTrailingObjects(OverloadToken<TypeRepr*>) const {
      return m_bits.SILBoxTypeRepr.NumGenericArgs;
   }

public:
   using Field = SILBoxTypeReprField;

   SILBoxTypeRepr(GenericParamList *GenericParams,
                  SourceLoc LBraceLoc, ArrayRef<Field> Fields,
                  SourceLoc RBraceLoc,
                  SourceLoc ArgLAngleLoc, ArrayRef<TypeRepr *> GenericArgs,
                  SourceLoc ArgRAngleLoc)
      : TypeRepr(TypeReprKind::SILBox),
        GenericParams(GenericParams), LBraceLoc(LBraceLoc), RBraceLoc(RBraceLoc),
        ArgLAngleLoc(ArgLAngleLoc), ArgRAngleLoc(ArgRAngleLoc)
   {
      m_bits.SILBoxTypeRepr.NumFields = Fields.size();
      m_bits.SILBoxTypeRepr.NumGenericArgs = GenericArgs.size();

      std::uninitialized_copy(Fields.begin(), Fields.end(),
                              getTrailingObjects<SILBoxTypeReprField>());

      std::uninitialized_copy(GenericArgs.begin(), GenericArgs.end(),
                              getTrailingObjects<TypeRepr*>());
   }

   static SILBoxTypeRepr *create(AstContext &C,
                                 GenericParamList *GenericParams,
                                 SourceLoc LBraceLoc, ArrayRef<Field> Fields,
                                 SourceLoc RBraceLoc,
                                 SourceLoc ArgLAngleLoc, ArrayRef<TypeRepr *> GenericArgs,
                                 SourceLoc ArgRAngleLoc);

   void setGenericEnvironment(GenericEnvironment *Env) {
      assert(!GenericEnv);
      GenericEnv = Env;
   }

   ArrayRef<Field> getFields() const {
      return {getTrailingObjects<Field>(),
               m_bits.SILBoxTypeRepr.NumFields};
   }
   ArrayRef<TypeRepr *> getGenericArguments() const {
      return {getTrailingObjects<TypeRepr*>(),
               static_cast<size_t>(m_bits.SILBoxTypeRepr.NumGenericArgs)};
   }

   GenericParamList *getGenericParams() const {
      return GenericParams;
   }
   GenericEnvironment *getGenericEnvironment() const {
      return GenericEnv;
   }

   SourceLoc getLBraceLoc() const { return LBraceLoc; }
   SourceLoc getRBraceLoc() const { return RBraceLoc; }
   SourceLoc getArgumentLAngleLoc() const { return ArgLAngleLoc; }
   SourceLoc getArgumentRAngleLoc() const { return ArgRAngleLoc; }

   static bool classof(const TypeRepr *T) {
      return T->getKind() == TypeReprKind::SILBox;
   }
   static bool classof(const SILBoxTypeRepr *T) { return true; }

private:
   SourceLoc getStartLocImpl() const;
   SourceLoc getEndLocImpl() const;
   SourceLoc getLocImpl() const;
   void printImpl(ASTPrinter &printer, const PrintOptions &opts) const;
   friend TypeRepr;
};

inline bool TypeRepr::isSimple() const {
   // NOTE: Please keep this logic in sync with TypeBase::hasSimpleTypeRepr().
   switch (getKind()) {
   case TypeReprKind::Attributed:
   case TypeReprKind::Error:
   case TypeReprKind::Function:
   case TypeReprKind::InOut:
   case TypeReprKind::Composition:
   case TypeReprKind::OpaqueReturn:
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
   case TypeReprKind::SILBox:
   case TypeReprKind::Shared:
   case TypeReprKind::Owned:
      return true;
   }
   llvm_unreachable("bad TypeRepr kind");
}

} // polar::ast

#endif // POLARPHP_AST_TYPEREPR_H