//===--- AstVisitor.h - Decl, Expr and Stmt Visitor -------------*- C++ -*-===//
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
// This file defines the AstVisitor class, and the DeclVisitor, ExprVisitor, and
// StmtVisitor template typedefs.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ASTVISITOR_H
#define POLARPHP_AST_ASTVISITOR_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeRepr.h"
#include "llvm/Support/ErrorHandling.h"

namespace polar {
class ParameterList;

/// AstVisitor - This is a simple visitor class for Swift expressions.
template<typename ImplClass,
   typename ExprRetTy = void,
   typename StmtRetTy = void,
   typename DeclRetTy = void,
   typename PatternRetTy = void,
   typename TypeReprRetTy = void,
   typename AttributeRetTy = void,
   typename... Args>
class AstVisitor {
public:
   typedef AstVisitor AstVisitorType;

   DeclRetTy visit(Decl *D, Args... AA) {
      switch (D->getKind()) {
#define DECL(CLASS, PARENT) \
    case DeclKind::CLASS: \
      return static_cast<ImplClass*>(this) \
        ->visit##CLASS##Decl(static_cast<CLASS##Decl*>(D), \
                             ::std::forward<Args>(AA)...);
#include "polarphp/ast/DeclNodesDef.h"
      }
      llvm_unreachable("Not reachable, all cases handled");
   }

   ExprRetTy visit(Expr *E, Args... AA) {
      switch (E->getKind()) {

#define EXPR(CLASS, PARENT) \
    case ExprKind::CLASS: \
      return static_cast<ImplClass*>(this) \
        ->visit##CLASS##Expr(static_cast<CLASS##Expr*>(E), \
                             ::std::forward<Args>(AA)...);
#include "polarphp/ast/ExprNodesDef.h"

      }
      llvm_unreachable("Not reachable, all cases handled");
   }

   // Provide default implementations of abstract "visit" implementations that
   // just chain to their base class.  This allows visitors to just implement
   // the base behavior and handle all subclasses if they desire.  Since this is
   // a template, it will only instantiate cases that are used and thus we still
   // require full coverage of the AST nodes by the visitor.
#define ABSTRACT_EXPR(CLASS, PARENT)                                \
  ExprRetTy visit##CLASS##Expr(CLASS##Expr *E, Args... AA) {  \
     return static_cast<ImplClass*>(this)->visit##PARENT(E, \
                                                ::std::forward<Args>(AA)...);  \
  }
#define EXPR(CLASS, PARENT) ABSTRACT_EXPR(CLASS, PARENT)
#include "polarphp/ast/ExprNodesDef.h"

   StmtRetTy visit(Stmt *S, Args... AA) {
      switch (S->getKind()) {

#define STMT(CLASS, PARENT) \
    case StmtKind::CLASS: \
      return static_cast<ImplClass*>(this) \
        ->visit##CLASS##Stmt(static_cast<CLASS##Stmt*>(S), \
                             ::std::forward<Args>(AA)...);
#include "polarphp/ast/StmtNodesDef.h"

      }
      llvm_unreachable("Not reachable, all cases handled");
   }

#define DECL(CLASS, PARENT) \
  DeclRetTy visit##CLASS##Decl(CLASS##Decl *D, Args... AA) {\
    return static_cast<ImplClass*>(this)->visit##PARENT(D, \
                                                 ::std::forward<Args>(AA)...); \
  }
#define ABSTRACT_DECL(CLASS, PARENT) DECL(CLASS, PARENT)
#include "polarphp/ast/DeclNodesDef.h"

   PatternRetTy visit(Pattern *P, Args... AA) {
      switch (P->getKind()) {
#define PATTERN(CLASS, PARENT) \
    case PatternKind::CLASS: \
      return static_cast<ImplClass*>(this) \
        ->visit##CLASS##Pattern(static_cast<CLASS##Pattern*>(P), \
                                ::std::forward<Args>(AA)...);
#include "polarphp/ast/PatternNodesDef.h"
      }
      llvm_unreachable("Not reachable, all cases handled");
   }

   TypeReprRetTy visit(TypeRepr *T, Args... AA) {
      switch (T->getKind()) {
#define TYPEREPR(CLASS, PARENT) \
    case TypeReprKind::CLASS: \
      return static_cast<ImplClass*>(this) \
        ->visit##CLASS##TypeRepr(static_cast<CLASS##TypeRepr*>(T), \
                                 ::std::forward<Args>(AA)...);
#include "polarphp/ast/TypeReprNodesDef.h"
      }
      llvm_unreachable("Not reachable, all cases handled");
   }

   TypeReprRetTy visitTypeRepr(TypeRepr *T, Args... AA) {
      return TypeReprRetTy();
   }

#define TYPEREPR(CLASS, PARENT) \
  TypeReprRetTy visit##CLASS##TypeRepr(CLASS##TypeRepr *T, Args... AA) {\
    return static_cast<ImplClass*>(this)->visit##PARENT(T, \
                                                 ::std::forward<Args>(AA)...); \
  }
#define ABSTRACT_TYPEREPR(CLASS, PARENT) TYPEREPR(CLASS, PARENT)
#include "polarphp/ast/TypeReprNodesDef.h"

   AttributeRetTy visit(DeclAttribute *A, Args... AA) {
      switch (A->getKind()) {
#define DECL_ATTR(_, CLASS, ...)                           \
    case DAK_##CLASS:                                              \
      return static_cast<ImplClass*>(this)                        \
               ->visit##CLASS##Attr(static_cast<CLASS##Attr*>(A), \
                                    ::std::forward<Args>(AA)...);
#include "polarphp/ast/AttrDef.h"

         case DAK_Count:
            llvm_unreachable("Not an attribute kind");
      }
   }

#define DECL_ATTR(NAME,CLASS,...) \
  AttributeRetTy visit##CLASS##Attr(CLASS##Attr *A, Args... AA) { \
    return static_cast<ImplClass*>(this)->visitDeclAttribute(       \
             A, ::std::forward<Args>(AA)...);                       \
  }
#include "polarphp/ast/AttrDef.h"

   bool visit(ParameterList *PL) {
      return static_cast<ImplClass*>(this)->visitParameterList(PL);
   }

   bool visitParameterList(ParameterList *PL) { return false; }
};


template<typename ImplClass, typename ExprRetTy = void, typename... Args>
using ExprVisitor = AstVisitor<ImplClass, ExprRetTy, void, void, void, void,
   void, Args...>;

template<typename ImplClass, typename StmtRetTy = void, typename... Args>
using StmtVisitor = AstVisitor<ImplClass, void, StmtRetTy, void, void, void,
   void, Args...>;

template<typename ImplClass, typename DeclRetTy = void, typename... Args>
using DeclVisitor = AstVisitor<ImplClass, void, void, DeclRetTy, void, void,
   void, Args...>;

template<typename ImplClass, typename PatternRetTy = void, typename... Args>
using PatternVisitor = AstVisitor<ImplClass, void,void,void, PatternRetTy, void,
   void, Args...>;

template<typename ImplClass, typename TypeReprRetTy = void, typename... Args>
using TypeReprVisitor = AstVisitor<ImplClass, void,void,void,void,TypeReprRetTy,
   void, Args...>;

template<typename ImplClass, typename AttributeRetTy = void, typename... Args>
using AttributeVisitor = AstVisitor<ImplClass, void,void,void,void,void,
   AttributeRetTy, Args...>;

} // end namespace polar

#endif
