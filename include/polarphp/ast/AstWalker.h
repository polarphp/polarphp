//===--- AstWalker.h - Class for walking the AST ----------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/04/26.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AST_WALKER_H
#define POLARPHP_AST_AST_WALKER_H

#include "polarphp/basic/adt/PointerUnion.h"
#include <utility>

namespace polar::ast {

class Decl;
class Expr;
class ModuleDecl;
class Stmt;
class TypeRepr;
struct TypeLoc;
class ParameterList;

enum class AccessKind: unsigned char;

enum class SemaReferenceKind : uint8_t
{
   ModuleRef = 0,
   DeclRef,
   DeclMemberRef,
   DeclConstructorRef,
   TypeRef,
   EnumElementRef,
   SubscriptRef,
};

struct ReferenceMetaData
{
   SemaReferenceKind kind;
   std::optional<AccessKind> accKind;
   ReferenceMetaData(SemaReferenceKind kind, std::optional<AccessKind> accKind)
      : kind(kind),
        accKind(accKind)
   {}
};

/// An abstract class used to traverse an AST.
class AstWalker
{
public:
   enum class ParentKind
   {
      Module, Decl, Stmt, Expr, TypeRepr
   };

   class ParentType
   {
      ParentKind kind;
      void *ptr = nullptr;

   public:
      ParentType(ModuleDecl *module)
         : kind(ParentKind::Module),
           ptr(module)
      {}

      ParentType(Decl *decl)
         : kind(ParentKind::Decl),
           ptr(decl)
      {}

      ParentType(Stmt *stmt)
         : kind(ParentKind::Stmt),
           ptr(stmt)
      {}

      ParentType(Expr *expr)
         : kind(ParentKind::Expr),
           ptr(expr)
      {}

      ParentType(TypeRepr *typeRepr)
         : kind(ParentKind::TypeRepr),
           ptr(typeRepr)
      {}

      ParentType()
         : kind(ParentKind::Module),
           ptr(nullptr)
      {}

      bool isNull() const
      {
         return ptr == nullptr;
      }

      ParentKind getKind() const
      {
         assert(!isNull());
         return kind;
      }

      ModuleDecl *getAsModule() const
      {
         return kind == ParentKind::Module ? static_cast<ModuleDecl*>(ptr)
                                           : nullptr;
      }

      Decl *getAsDecl() const
      {
         return kind == ParentKind::Decl ? static_cast<Decl*>(ptr) : nullptr;
      }

      Stmt *getAsStmt() const
      {
         return kind == ParentKind::Stmt ? static_cast<Stmt*>(ptr) : nullptr;
      }

      Expr *getAsExpr() const
      {
         return kind == ParentKind::Expr ? static_cast<Expr*>(ptr) : nullptr;
      }

      TypeRepr *getAsTypeRepr() const
      {
         return kind==ParentKind::TypeRepr ? static_cast<TypeRepr*>(ptr) : nullptr;
      }
   };

   /// The parent of the node we are visiting.
   ParentType Parent;

   /// This method is called when first visiting an expression
   /// before walking into its children.
   ///
   /// \param E The expression to check.
   ///
   /// \returns a pair indicating whether to visit the children along with
   /// the expression that should replace this expression in the tree. If the
   /// latter is null, the traversal will be terminated.
   ///
   /// The default implementation returns \c {true, E}.
   virtual std::pair<bool, Expr *> walkToExprPre(Expr *E)
   {
      return { true, E };
   }

   /// This method is called after visiting an expression's children.
   /// If it returns null, the walk is terminated; otherwise, the
   /// returned expression is spliced in where the old expression
   /// previously appeared.
   ///
   /// The default implementation always returns its argument.
   virtual Expr *walkToExprPost(Expr *E)
   {
      return E;
   }

   /// This method is called when first visiting a statement before
   /// walking into its children.
   ///
   /// \param S The statement to check.
   ///
   /// \returns a pair indicating whether to visit the children along with
   /// the statement that should replace this statement in the tree. If the
   /// latter is null, the traversal will be terminated.
   ///
   /// The default implementation returns \c {true, S}.
   virtual std::pair<bool, Stmt *> walkToStmtPre(Stmt *S)
   {
      return { true, S };
   }

   /// This method is called after visiting a statement's children.  If
   /// it returns null, the walk is terminated; otherwise, the returned
   /// statement is spliced in where the old statement previously
   /// appeared.
   ///
   /// The default implementation always returns its argument.
   virtual Stmt *walkToStmtPost(Stmt *S) { return S; }

   /// walkToDeclPre - This method is called when first visiting a decl, before
   /// walking into its children.  If it returns false, the subtree is skipped.
   ///
   /// \param D The declaration to check. The callee may update this declaration
   /// in-place.
   virtual bool walkToDeclPre(Decl *D)
   {
      return true;
   }

   /// walkToDeclPost - This method is called after visiting the children of a
   /// decl.  If it returns false, the remaining traversal is terminated and
   /// returns failure.
   virtual bool walkToDeclPost(Decl *D)
   {
      return true;
   }

   /// This method is called when first visiting a TypeLoc, before
   /// walking into its TypeRepr children.  If it returns false, the subtree is
   /// skipped.
   ///
   /// \param TL The TypeLoc to check.
   virtual bool walkToTypeLocPre(TypeLoc &TL)
   {
      return true;
   }

   /// This method is called after visiting the children of a TypeLoc.
   /// If it returns false, the remaining traversal is terminated and returns
   /// failure.
   virtual bool walkToTypeLocPost(TypeLoc &TL)
   {
      return true;
   }


   /// This method is called when first visiting a TypeRepr, before
   /// walking into its children.  If it returns false, the subtree is skipped.
   ///
   /// \param T The TypeRepr to check.
   virtual bool walkToTypeReprPre(TypeRepr *T)
   {
      return true;
   }

   /// This method is called after visiting the children of a TypeRepr.
   /// If it returns false, the remaining traversal is terminated and returns
   /// failure.
   virtual bool walkToTypeReprPost(TypeRepr *T)
   {
      return true;
   }

   /// This method configures whether the walker should explore into the generic
   /// params in AbstractFunctionDecl and NominalTypeDecl.
   virtual bool shouldWalkIntoGenericParams()
   {
      return false;
   }

   /// This method configures whether the walker should walk into the
   /// initializers of lazy variables.  These initializers are semantically
   /// different from other initializers in their context and so sometimes
   /// should not be visited.
   ///
   /// Note that visiting the body of the lazy getter will find a
   /// LazyInitializerExpr with the initializer as its sub-expression.
   /// However, AstWalker does not walk into LazyInitializerExprs on its own.
   virtual bool shouldWalkIntoLazyInitializers()
   {
      return true;
   }

   /// walkToParameterListPre - This method is called when first visiting a
   /// ParameterList, before walking into its parameters.  If it returns false,
   /// the subtree is skipped.
   ///
   virtual bool walkToParameterListPre(ParameterList *PL)
   {
      return true;
   }

   /// walkToParameterListPost - This method is called after visiting the
   /// children of a parameter list.  If it returns false, the remaining
   /// traversal is terminated and returns failure.
   virtual bool walkToParameterListPost(ParameterList *PL)
   {
      return true;
   }

protected:
   AstWalker() = default;
   AstWalker(const AstWalker &) = default;
   virtual ~AstWalker() = default;
   virtual void anchor();
};

} // polar::ast

#endif // POLARPHP_AST_AST_WALKER_H
