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
   ReservedNonModifier,
   SemiReserved,
   Identifier,
   NamespacePart,
   Name,
   NamespaceUseType,
   UnprefixedUseDeclaration,
   UseDeclaration,
   InlineUseDeclaration,
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

   /// decl syntax node
   SourceFile,

   /// stmt syntax node
   FirstStmt,
   ContinueStmt,
   BreakStmt,
   FallthroughStmt,
   WhileStmt,
   DoWhileStmt,
   SwitchCase,
   SwitchDefaultLabel,
   SwitchCaseLabel,
   SwitchStmt,
   ElseIfClause,
   IfStmt,
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
   ElseIfList,
   CodeBlockItemList,
   ExprList,
   TokenList,
   NonEmptyTokenList,
   NamespacePartList,
   NamespaceUseDeclarationList,
   NamespaceInlineUseDeclarationList,
   NamespaceUnprefixedUseDeclarationList,

   // NOTE: Unknown must be the last kind.
   Unknown,
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
