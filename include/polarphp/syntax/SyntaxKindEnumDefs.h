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
   TypeClause,
   TypeExprClause,
   Name,
   NamespaceUseType,
   NamespaceUnprefixedUseDeclaration,
   NamespaceUseDeclaration,
   NamespaceInlineUseDeclaration,
   NamespaceGroupUseDeclaration,
   NamespacemixedGroupUseDeclaration,
   ConstDeclareItem,
   ConstDefinition,
   ReturnTypeClause,
   InitializeClause, // for variable assign or initialize
   ParameterItem,
   ParameterClauseSyntax,
   LexicalVarItem,
   FunctionDefinition,
   /// for class definition
   ClassModifier,

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
   CommonStmt,
   InnerStmt,
   TopStmt,
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

   /// collection syntax node
   ConditionElementList,
   SwitchCaseList,
   ElseIfList,
   CodeBlockItemList,
   InnerStmtList,
   TopStmtList,
   ExprList,
   NamespacePartList,
   NamespaceUseDeclarationList,
   NamespaceInlineUseDeclarationList,
   NamespaceUnprefixedUseDeclarationList,
   ConstDeclareItemList,
   ParameterList,
   LexicalVarList,
   ClassModifierList,

   // NOTE: Unknown must be the last kind.
   Unknown,
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
