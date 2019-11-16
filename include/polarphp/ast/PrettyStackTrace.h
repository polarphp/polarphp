//===--- PrettyStackTrace.h - Crash trace information -----------*- C++ -*-===//
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
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
// This file defines RAII classes that give better diagnostic output
// about when, exactly, a crash is occurring.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_PRETTYSTACKTRACE_H
#define POLARPHP_AST_PRETTYSTACKTRACE_H

#include "llvm/Support/PrettyStackTrace.h"
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/Type.h"

namespace polar::ast {

using polar::utils::PrettyStackTraceEntry;
using polar::parser::SourceLoc;

void print_source_loc_description(raw_ostream &out, SourceLoc loc,
                                  AstContext &context, bool addNewline = true);

/// PrettyStackTraceLocation - Observe that we are doing some
/// processing starting at a fixed location.
class PrettyStackTraceLocation : public PrettyStackTraceEntry
{
   AstContext &m_context;
   SourceLoc m_loc;
   const char *m_action;
public:
   PrettyStackTraceLocation(AstContext &context, const char *action, SourceLoc loc)
      : m_context(context),
        m_loc(loc),
        m_action(action)
   {}

   virtual void print(raw_ostream &outStream) const;
};

void print_decl_description(raw_ostream &out, const Decl *decl,
                            AstContext &context, bool addNewline = true);

/// PrettyStackTraceDecl - Observe that we are processing a specific
/// declaration.
class PrettyStackTraceDecl : public PrettyStackTraceEntry
{
   const Decl *m_theDecl;
   const char *m_action;
public:
   PrettyStackTraceDecl(const char *action, const Decl *decl)
      : m_theDecl(decl),
        m_action(action)
   {}

   virtual void print(raw_ostream &outStream) const;
};

void print_expr_description(raw_ostream &out, Expr *expr,
                            AstContext &context, bool addNewline = true);

/// PrettyStackTraceExpr - Observe that we are processing a specific
/// expression.
class PrettyStackTraceExpr : public PrettyStackTraceEntry
{
   AstContext &m_context;
   Expr *m_theExpr;
   const char *m_action;
public:
   PrettyStackTraceExpr(AstContext &context, const char *action, Expr *expr)
      : m_context(context),
        m_theExpr(expr),
        m_action(action)
   {}

   virtual void print(raw_ostream &outStream) const;
};

void print_stmt_description(raw_ostream &out, Stmt *stmt,
                            AstContext &context, bool addNewline = true);

/// PrettyStackTraceStmt - Observe that we are processing a specific
/// statement.
class PrettyStackTraceStmt : public PrettyStackTraceEntry
{
   AstContext &m_context;
   Stmt *m_theStmt;
   const char *m_action;
public:
   PrettyStackTraceStmt(AstContext &context, const char *action, Stmt *stmt)
      : m_context(context),
        m_theStmt(stmt),
        m_action(action)
   {}

   virtual void print(raw_ostream &outStream) const;
};

void printPatternDescription(raw_ostream &out, Pattern *P,
                             AstContext &m_context, bool addNewline = true);


void print_type_description(raw_ostream &out, Type type,
                            AstContext &context, bool addNewline = true);

/// PrettyStackTraceType - Observe that we are processing a specific type.
class PrettyStackTraceType : public PrettyStackTraceEntry
{
   AstContext &m_context;
   Type m_theType;
   const char *m_action;
public:
   PrettyStackTraceType(AstContext &context, const char *action, Type type)
      : m_context(context),
        m_theType(type),
        m_action(action)
   {}

   virtual void print(raw_ostream &outStream) const;
};

/// Observe that we are processing a specific type representation.
class PrettyStackTraceTypeRepr : public PrettyStackTraceEntry
{
   AstContext &m_context;
   TypeRepr *m_theType;
   const char *m_action;
public:
   PrettyStackTraceTypeRepr(AstContext &C, const char *action, TypeRepr *type)
      : m_context(C), m_theType(type), m_action(action) {}
   virtual void print(raw_ostream &OS) const;
};


class PrettyStackTraceGenericSignature : public PrettyStackTraceEntry
{
   const char *m_action;
   GenericSignature *m_genericSig;
   std::optional<unsigned> m_requirement;

public:
   PrettyStackTraceGenericSignature(const char *action,
                                    GenericSignature *genericSig,
                                    std::optional<unsigned> requirement = std::nullopt)
      : m_action(action),
        m_genericSig(genericSig),
        m_requirement(requirement)
   {}

   void setRequirement(std::optional<unsigned> requirement)
   {
      m_requirement = requirement;
   }

   void print(raw_ostream &out) const override;
};

} // polar::ast

#endif // POLARPHP_AST_PRETTYSTACKTRACE_H
