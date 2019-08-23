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
   ParameterListClause,
   LexicalVarItem,
   UseLexicalVarClause,
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
   ExprListItem,

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
   ClassNameRefClause,

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

   // function like lang structure
   IsSetVarItem,
   IsSetVariablesClause,
   IsSetFuncExpr,
   EmptyFuncExpr,
   IncludeExpr,
   RequireExpr,
   EvalFuncExpr,
   PrintFuncExpr,
   FuncLikeExpr,

   ArrayStructureAssignmentExpr,
   ListStructureAssignmentExpr,
   AssignmentExpr,
   CompoundAssignmentExpr,
   BitLogicalExpr,
   LogicalExpr,
   RelationExpr,
   CastExpr,
   ExitExprArgClause,
   ExitExpr,
   YieldExpr,
   YieldFromExpr,
   CloneExpr,

   EncapsVarOffset,
   EncapsArrayVar,
   EncapsObjProp,
   EncapsDollarCurlyExpr,
   EncapsDollarCurlyVar,
   EncapsDollarCurlyArray,
   EncapsCurlyVar,
   EncapsVar,
   EncapsListItem,
   BackticksClause,

   HeredocExpr,
   EncapsListStringExpr,
   DereferencableScalarExpr,
   AnonymousClassDefinitionClause,
   SimpleInstanceCreateExpr,
   AnonymousInstanceCreateExpr,
   ClassicLambdaExpr,
   SimplifiedLambdaExpr,
   LambdaExpr,
   InstanceCreateExpr,
   ScalarExpr,
   BooleanLiteralExpr,
   TernaryExpr,
   SequenceExpr,
   ClassRefParentExpr,
   ClassRefStaticExpr,
   ClassRefSelfExpr,
   PrefixOperatorExpr,
   PostfixOperatorExpr,
   BinaryOperatorExpr,
   ShellCmdExpr,
   UnknownExpr,
   LastExpr,

   /// decl syntax node
   SourceFile,

   /// stmt syntax node
   FirstStmt,
   EmptyStmt,
   ConditionElement,
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
   EchoStmt,
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
   EchoExprList,
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
   IsSetVariablesList,

   // NOTE: Unknown must be the last kind.
   Unknown,
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_KIND_ENUM_DEFS_H
