// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/12.

#ifndef POLARPHP_LLPARSER_SYNTAXKINDS_H
#define POLARPHP_LLPARSER_SYNTAXKINDS_H

#include "polarphp/basic/InlineBitfield.h"

namespace polar::llparser {

using polar::count_bits_used;

enum class SyntaxKind {
   Decl, // Common Syntax
   Expr,
   Stmt,
   Type,
   Pattern,
   CodeBlockItem,
   CodeBlock,

   CustomAttribute, // Attribute Syntax
   Attribute,
   LabeledSpecializeEntry,
   NamedAttributeStringArgument,
   DeclName,
   ImplementsAttributeArguments,
   ObjCSelectorPiece,
   DifferentiableAttributeArguments,
   DifferentiationParamsClause,
   DifferentiationParams,
   DifferentiationParam,
   DifferentiableAttributeFuncSpecifier,
   FunctionDeclName,

   AvailabilityArgument, // Availability Syntax
   AvailabilityLabeledArgument,
   AvailabilityVersionRestriction,
   VersionTuple,

   TypeInitializerClause, // Decl Syntax
   ParameterClause,
   ReturnClause,
   FunctionSignature,
   IfConfigClause,
   PoundSourceLocationArgs,
   DeclModifier,
   InheritedType,
   TypeInheritanceClause,
   MemberDeclBlock,
   MemberDeclListItem,
   SourceFile,
   InitializerClause,
   FunctionParameter,
   AccessLevelModifier,
   AccessPathComponent,
   AccessorParameter,
   AccessorBlock,
   PatternBinding,
   EnumCaseElement,
   OperatorPrecedenceAndTypes,
   PrecedenceGroupRelation,
   PrecedenceGroupNameElement,
   PrecedenceGroupAssignment,
   PrecedenceGroupAssociativity,

   DeclNameArgument, // Expr Syntax
   DeclNameArguments,
   TupleExprElement,
   ArrayElement,
   DictionaryElement,
   ClosureCaptureItem,
   ClosureCaptureSignature,
   ClosureParam,
   ClosureSignature,
   StringSegment,
   ExpressionSegment,
   ObjcNamePiece,

   GenericWhereClause, // Generic Syntax
   GenericRequirement,
   SameTypeRequirement,
   GenericParameter,
   GenericParameterClause,
   ConformanceRequirement,

   TypeAnnotation, // Pattern Syntax
   TuplePatternElement,
   WhereClause,
   YieldList,
   ConditionElement,
   AvailabilityCondition,
   MatchingPatternCondition,
   OptionalBindingCondition,
   ElseIfContinuation,
   ElseBlock,
   SwitchCase,
   SwitchDefaultLabel,
   CaseItem,
   SwitchCaseLabel,
   CatchClause,

   CompositionTypeElement, // Type Syntax
   TupleTypeElement,
   GenericArgument,
   GenericArgumentClause,

   TokenList, // Attribute Syntax Collection
   NonEmptyTokenList,
   AttributeList,
   SpecializeAttributeSpecList,
   ObjCSelector,
   DifferentiationParamList,

   AvailabilitySpecList, // Availability Syntax Collection

   CodeBlockItemList, // Common Syntax Collection

   FunctionParameterList, // Decl Syntax Collection
   IfConfigClauseList,
   InheritedTypeList,
   MemberDeclList,
   ModifierList,
   AccessPath,
   AccessorList,
   PatternBindingList,
   EnumCaseElementList,
   IdentifierList,
   PrecedenceGroupAttributeList,
   PrecedenceGroupNameList,

   TupleExprElementList, // Expr Syntax Collection
   ArrayElementList,
   DictionaryElementList,
   StringLiteralSegments,
   DeclNameArgumentList,
   ExprList,
   ClosureCaptureItemList,
   ClosureParamList,
   ObjcName,

   GenericRequirementList, // Generic Syntax Collection
   GenericParameterList,

   TuplePatternElementList, // Pattern Syntax Collection

   SwitchCaseList, // Stmt Syntax Collection
   CatchClauseList,
   CaseItemList,
   ConditionElementList,

   CompositionTypeElementList, // Type Syntax Collection
   TupleTypeElementList,
   GenericArgumentList,

   UnknownDecl, //  Decl Syntax Kind
   TypealiasDecl,
   AssociatedtypeDecl,
   IfConfigDecl,
   PoundErrorDecl,
   PoundWarningDecl,
   PoundSourceLocation,
   ClassDecl,
   StructDecl,
   InterfaceDecl,
   ExtensionDecl,
   FunctionDecl,
   InitializerDecl,
   DeinitializerDecl,
   SubscriptDecl,
   ImportDecl,
   AccessorDecl,
   VariableDecl,
   EnumCaseDecl,
   EnumDecl,
   OperatorDecl,
   PrecedenceGroupDecl,
   FirstDecl = UnknownDecl,
   LastDecl = PrecedenceGroupDecl,

   UnknownExpr, // Expr
   InOutExpr,
   PoundColumnExpr,
   TryExpr,
   IdentifierExpr,
   SuperRefExpr,
   NilLiteralExpr,
   DiscardAssignmentExpr,
   AssignmentExpr,
   SequenceExpr,
   PoundLineExpr,
   PoundFileExpr,
   PoundFunctionExpr,
   PoundDsohandleExpr,
   SymbolicReferenceExpr,
   PrefixOperatorExpr,
   BinaryOperatorExpr,
   ArrowExpr,
   FloatLiteralExpr,
   TupleExpr,
   ArrayExpr,
   DictionaryExpr,
   IntegerLiteralExpr,
   BooleanLiteralExpr,
   TernaryExpr,
   MemberAccessExpr,
   IsExpr,
   AsExpr,
   TypeExpr,
   ClosureExpr,
   UnresolvedPatternExpr,
   FunctionCallExpr,
   SubscriptExpr,
   OptionalChainingExpr,
   ForcedValueExpr,
   PostfixUnaryExpr,
   SpecializeExpr,
   StringLiteralExpr,
   KeyPathExpr,
   KeyPathBaseExpr,
   ObjcKeyPathExpr,
   ObjcSelectorExpr,
   EditorPlaceholderExpr,
   ObjectLiteralExpr,
   FirstExpr = UnknownExpr,
   LastExpr = ObjectLiteralExpr,

   UnknownPattern, // Pattern Syntax Kind
   EnumCasePattern,
   IsTypePattern,
   OptionalPattern,
   IdentifierPattern,
   AsTypePattern,
   TuplePattern,
   WildcardPattern,
   ExpressionPattern,
   ValueBindingPattern,
   FirstPattern = UnknownPattern,
   LastPattern = ValueBindingPattern,

   UnknownStmt, // Stmt Syntax Kind
   ContinueStmt,
   WhileStmt,
   DeferStmt,
   ExpressionStmt,
   RepeatWhileStmt,
   GuardStmt,
   ForInStmt,
   SwitchStmt,
   DoStmt,
   ReturnStmt,
   YieldStmt,
   FallthroughStmt,
   BreakStmt,
   DeclarationStmt,
   ThrowStmt,
   IfStmt,
   PoundAssertStmt,
   FirstStmt = UnknownStmt,
   LastStmt = PoundAssertStmt,

   SimpleTypeIdentifier, // Type Syntax Kind
   MemberTypeIdentifier,
   ClassRestrictionType,
   ArrayType,
   DictionaryType,
   MetatypeType,
   OptionalType,
   SomeType,
   ImplicitlyUnwrappedOptionalType,
   CompositionType,
   TupleType,
   FunctionType,
   AttributedType,
   FirstType = SimpleTypeIdentifier,
   LastType = AttributedType,

   Unknown
};

enum : unsigned {
   NumSyntaxKindBits = count_bits_used(static_cast<unsigned>(SyntaxKind::Unknown))
};

} // polar::llparser

#endif //POLARPHP_LLPARSER_SYNTAXKINDS_H
