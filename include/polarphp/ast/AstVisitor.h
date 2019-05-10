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
// This file defines the AstVisitor class, and the DeclVisitor, ExprVisitor, and
// StmtVisitor template typedefs.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_AST_VISITOR_H
#define POLARPHP_AST_AST_VISITOR_H

#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Expr.h"
//#include "polarphp/ast/Module.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/TypeRepr.h"
#include "polarphp/utils/ErrorHandling.h"

namespace polar::ast {

class ParameterList;

/// AstVisitor - This is a simple visitor class for Swift expressions.
template<typename ImplClass,
         typename ExprRetType = void,
         typename StmtRetType = void,
         typename DeclRetType = void,
         typename PatternRetTy = void,
         typename TypeReprRetType = void,
         typename AttributeRetType = void,
         typename... Args>
class AstVisitor
{
public:
   using AstVisitorType = AstVisitor;

   DeclRetType visit(Decl *decl, Args... args)
   {
//      switch (decl->getKind())
//      {
//#define DECL(CLASS, PARENT) \
//      case DeclKind::CLASS: \
//   return static_cast<ImplClass*>(this) \
//   ->visit##CLASS##Decl(static_cast<CLASS##Decl*>(decl), \
//   ::std::forward<Args>(args)...);
//#include "polarphp/ast/DeclNodesDefs.h"
//      }
//      polar_unreachable("Not reachable, all cases handled");
   }

   ExprRetType visit(Expr *expr, Args... args)
   {
//      switch (expr->getKind()) {

//#define EXPR(CLASS, PARENT) \
//      case ExprKind::CLASS: \
//   return static_cast<ImplClass*>(this) \
//   ->visit##CLASS##Expr(static_cast<CLASS##Expr*>(E), \
//   ::std::forward<Args>(args)...);
//#include "polarphp/ast/DeclNodesDefs.h"

//      }
//      polar_unreachable("Not reachable, all cases handled");
   }

   // Provide default implementations of abstract "visit" implementations that
   // just chain to their base class.  This allows visitors to just implement
   // the base behavior and handle all subclasses if they desire.  Since this is
   // a template, it will only instantiate cases that are used and thus we still
   // require full coverage of the AST nodes by the visitor.
#define ABSTRACT_EXPR(CLASS, PARENT)                                \
   ExprRetType visit##CLASS##Expr(CLASS##Expr *expr, Args... args) {  \
   return static_cast<ImplClass*>(this)->visit##PARENT(expr, \
   ::std::forward<Args>(args)...);  \
}
//#define EXPR(CLASS, PARENT) ABSTRACT_EXPR(CLASS, PARENT)
//#include "polarphp/ast/DeclNodesDefs.h"

   StmtRetType visit(Stmt *stmt, Args... args)
   {
//      switch (stmt->getKind()) {

//#define STMT(CLASS, PARENT) \
//      case StmtKind::CLASS: \
//   return static_cast<ImplClass*>(this) \
//   ->visit##CLASS##Stmt(static_cast<CLASS##Stmt*>(S), \
//   ::std::forward<Args>(args)...);
//#include "polarphp/ast/DeclNodesDefs.h"

//      }
//      polar_unreachable("Not reachable, all cases handled");
   }

#define DECL(CLASS, PARENT) \
   DeclRetType visit##CLASS##Decl(CLASS##Decl *decl, Args... args) {\
   return static_cast<ImplClass*>(this)->visit##PARENT(decl, \
   ::std::forward<Args>(args)...); \
}
//#define ABSTRACT_DECL(CLASS, PARENT) DECL(CLASS, PARENT)
//#include "polarphp/ast/DeclNodesDefs.h"

   TypeReprRetType visit(TypeRepr *typeRepr, Args... args)
   {
//      switch (T->getKind()) {
//#define TYPEREPR(CLASS, PARENT) \
//      case TypeReprKind::CLASS: \
//   return static_cast<ImplClass*>(this) \
//   ->visit##CLASS##TypeRepr(static_cast<CLASS##TypeRepr*>(typeRepr), \
//   ::std::forward<Args>(args)...);
//#include "polarphp/ast/TypeReprNodesDefs.h"
//      }
      polar_unreachable("Not reachable, all cases handled");
   }

   TypeReprRetType visitTypeRepr(TypeRepr *T, Args... args)
   {
      return TypeReprRetType();
   }

#define TYPEREPR(CLASS, PARENT) \
   TypeReprRetType visit##CLASS##TypeRepr(CLASS##TypeRepr *T, Args... args) {\
   return static_cast<ImplClass*>(this)->visit##PARENT(T, \
   ::std::forward<Args>(args)...); \
}
//#define ABSTRACT_TYPEREPR(CLASS, PARENT) TYPEREPR(CLASS, PARENT)
//#include "polarphp/ast/TypeReprNodesDefs.h"

   AttributeRetType visit(DeclAttribute *A, Args... args) {
//      switch (A->getKind()) {
//#define DECL_ATTR(_, CLASS, ...)                           \
//      case DAK_##CLASS:                                              \
//   return static_cast<ImplClass*>(this)                        \
//   ->visit##CLASS##Attr(static_cast<CLASS##Attr*>(A), \
//   ::std::forward<Args>(args)...);
//#include "polarphp/ast/AttrDefs.h"

//      case DAK_Count:
//         polar_unreachable("Not an attribute kind");
//      }
   }

#define DECL_ATTR(NAME,CLASS,...) \
   AttributeRetType visit##CLASS##Attr(CLASS##Attr *A, Args... args) { \
   return static_cast<ImplClass*>(this)->visitDeclAttribute(       \
   A, ::std::forward<Args>(args)...);                       \
}
#include "polarphp/ast/AttrDefs.h"

   bool visit(ParameterList *params)
   {
      return static_cast<ImplClass*>(this)->visitParameterList(params);
   }

   bool visitParameterList(ParameterList *params)
   {
      return false;
   }
};

template<typename ImplClass, typename ExprRetType = void, typename... Args>
using ExprVisitor = AstVisitor<ImplClass, ExprRetType, void, void, void, void,
void, Args...>;

template<typename ImplClass, typename StmtRetType = void, typename... Args>
using StmtVisitor = AstVisitor<ImplClass, void, StmtRetType, void, void, void,
void, Args...>;

template<typename ImplClass, typename DeclRetType = void, typename... Args>
using DeclVisitor = AstVisitor<ImplClass, void, void, DeclRetType, void, void,
void, Args...>;

template<typename ImplClass, typename TypeReprRetType = void, typename... Args>
using TypeReprVisitor = AstVisitor<ImplClass, void, void, void, void, TypeReprRetType,
void, Args...>;

template<typename ImplClass, typename AttributeRetType = void, typename... Args>
using AttributeVisitor = AstVisitor<ImplClass, void, void, void, void, void,
AttributeRetType, Args...>;

} // polar::ast

#endif // POLARPHP_AST_AST_VISITOR_H
