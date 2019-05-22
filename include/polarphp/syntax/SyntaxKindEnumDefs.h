// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/11.

#ifndef POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
#define POLARPHP_SYNTAX_KIND_ENUM_DEFS_H

#include <cstdint>

namespace polar::syntax {

enum class SyntaxKind : std::uint32_t
{
   Token,

   /// common syntax node
   Decl,
   Expr,
   Stmt,
   Type,
   CodeBlockItem,
   CodeBlock,
   ConditionElement,

   /// decl syntax node
   FirstDecl,
   UnknownDecl,
   LastDecl,

   /// expr syntax node
   FirstExpr,
   NullExpr,
   FloatLiteralExpr,
   IntegerLiteralExpr,
   StringLiteralExpr,
   BooleanLiteralExpr,
   TernaryExpr,
   AssignmentExpr,
   SequenceExpr,
   ClassRefParentExpr,
   ClassRefStaticExpr,
   ClassRefSelfExpr,
   PrefixOperatorExpr,
   PostfixOperatorExpr,
   BinaryOperatorExpr,
   UnknownExpr,
   LastExpr,

   /// stmt syntax node
   FirstStmt,
   ContinueStmt,
   BreakStmt,
   FallthroughStmt,
   WhileStmt,
   DeferStmt,
   ExpressionStmt,
   ThrowStmt,
   ReturnStmt,
   UnknownStmt,
   LastStmt,

   /// type syntax node
   FirstType,
   UnknownType,
   LastType,

   /// collection syntax node
   ConditionElementList,
   SwitchCaseList,
   CodeBlockItemList,
   ExprList,
   TokenList,
   NonEmptyTokenList,

   // NOTE: Unknown must be the last kind.
   Unknown,
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
