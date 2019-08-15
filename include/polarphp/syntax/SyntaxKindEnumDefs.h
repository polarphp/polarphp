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
   NamespaceUse,
   NamespaceUseType,
   NamespaceUnprefixedUseDeclaration,
   NamespaceUseDeclaration,
   NamespaceInlineUseDeclaration,
   NamespaceGroupUseDeclaration,
   NamespaceMixedGroupUseDeclaration,
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
   ImplementsClause,
   InterfaceExtendsClause,
   ExtendsFromClause,
   ClassPropertyClause,
   ClassConstClause,
   ClassPropertyDecl,
   ClassConstDecl,
   ClassMethodDecl,
   /// class traint use clauses
   ClassTraitMethodReference,
   ClassAbsoluteTraitMethodReference,
   ClassTraitPrecedence,
   ClassTraitAlias,
   ClassTraitAdaptation,
   ClassTraitAdaptationBlock,
   ClassTraitDecl,
   MemberModifier,
   MemberDeclBlock,
   MemberDeclListItem,
   ClassDefinition,
   InterfaceDefinition,
   TraitDefinition,

   UnknownDecl,
   LastDecl,

   /// expr syntax node
   FirstExpr,
   ParenDecoratedExpr,
   NullExpr,
   OptionalExpr,
   VariableExpr,
   ClassConstIdentifierExpr,

   ConstExpr,
   StaticMemberExpr,
   NewVariableClause,
   CallableVariableExpr,
   CallableFuncNameClause,
   MemberNameClause,
   PropertyNameClause,
   InstancePropertyExpr,
   StaticPropertyExpr,

   // argument clauses
   Argument,
   ArgumentListItem,
   ArgumentListClause,

   DereferencableClause,
   VariableClassNameClause,
   ClassNameClause,
   ClassNameReference,

   BraceDecoratedExprClause,
   BraceDecoratedVariableExpr,
   ArrayKeyValuePairItem,
   ArrayUnpackPairItem,
   ArrayPairItem,
   ListRecursivePairItem,
   ListPairItem,
   SimpleVariableExpr,
   ArrayCreateExpr,
   SimplifiedArrayCreateExpr,
   ArrayAccessExpr,
   BraceDecoratedArrayAccessExpr,
   SimpleFunctionCallExpr,
   FunctionCallExpr,
   InstanceMethodCallExpr,
   StaticMethodCallExpr,
   FloatLiteralExpr,
   IntegerLiteralExpr,
   StringLiteralExpr,
   EncapsVarOffset,
   EncapsArrayVar,
   EncapsObjProp,
   EncapsDollarCurlyExpr,
   EncapsDollarCurlyVar,
   EncapsDollarCurlyArray,
   EncapsCurlyVar,
   EncapsVar,
   EncapsListItem,
   HeredocExpr,
   EncapsListStringExpr,
   DereferencableScalarExpr,
   ScalarExpr,
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
   NameList,
   NamespacePartList,
   NamespaceUseDeclarationList,
   NamespaceInlineUseDeclarationList,
   NamespaceUnprefixedUseDeclarationList,
   ConstDeclareItemList,
   ParameterList,
   LexicalVarList,
   ClassPropertyList,
   ClassConstList,
   ClassModifierList,
   ClassTraitAdaptationList,
   /// for array
   ArrayPairItemList,
   ListPairItemList,
   MemberModifierList,
   MemberDeclList,
   EncapsList,
   ArgumentList,

   // NOTE: Unknown must be the last kind.
   Unknown,
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
