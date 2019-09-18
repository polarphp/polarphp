// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/14.

#ifndef POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H
#define POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H

#include "polarphp/syntax/AbstractFactory.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodesFwd.h"
#include <optional>

namespace polar::syntax {

class ExprSyntaxNodeFactory final : public AbstractFactory
{
public:
   ///
   /// make collection nodes
   ///
   static ExprListSyntax makeExprList(const std::vector<ExprListItemSyntax> elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVariableListSyntax makeLexicalVariableList(const std::vector<LexicalVariableListItemSyntax> elements,
                                                       RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairListSyntax makeArrayPairList(const std::vector<ArrayPairListItemSyntax> elements,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsItemListSyntax makeEncapsItemList(const std::vector<EncapsListItemSyntax> elements,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListSyntax makeArgumentList(const std::vector<ArgumentListItemSyntax> elements,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariablesListSyntax makeIssetVariablesList(const std::vector<IssetVariableListItemSyntax> elements,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   ///
   /// make normal nodes
   ///
   ///
   static ParenDecoratedExprSyntax makeParenDecoratedExpr(TokenSyntax leftParen, ExprSyntax expr,
                                                          TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena = nullptr);
   static NullExprSyntax makeNullExpr(TokenSyntax nullKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static OptionalExprSyntax makeOptionalExpr(std::optional<ExprSyntax> expr, RefCountPtr<SyntaxArena> arena = nullptr);
   static ExprListItemSyntax makeExprListItem(std::optional<TokenSyntax> comma, ExprSyntax expr,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static VariableExprSyntax makeVariableExpr(ExprSyntax var, RefCountPtr<SyntaxArena> arena = nullptr);
   static ReferencedVariableExprSyntax makeReferencedVariableExpr(TokenSyntax refToken, VariableExprSyntax variableExpr,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstIdentifierExprSyntax makeClassConstIdentifierExpr(Syntax className, TokenSyntax separatorToken,
                                                                      IdentifierSyntax identifier, RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstExprSyntax makeConstExpr(Syntax identifier, RefCountPtr<SyntaxArena> arena = nullptr);
   static NewVariableClauseSyntax makeNewVariableClause(ExprSyntax varNode, RefCountPtr<SyntaxArena> arena = nullptr);
   static CallableVariableExprSyntax makeCallableVariableExpr(ExprSyntax var, RefCountPtr<SyntaxArena> arena = nullptr);
   static CallableFuncNameClauseSyntax makeCallableFuncNameClause(Syntax funcName, RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberNameClauseSyntax makeMemberNameClause(Syntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static PropertyNameClauseSyntax makePropertyNameClause(Syntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static InstancePropertyExprSyntax makeInstancePropertyExpr(Syntax objectRef, TokenSyntax separator,
                                                              Syntax propertyName, RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticPropertyExprSyntax makeStaticPropertyExpr(Syntax className, TokenSyntax separator,
                                                          SimpleVariableExprSyntax memberName, RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentSyntax makeArgument(std::optional<TokenSyntax> ellipsisToken, ExprSyntax expr,
                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListItemSyntax makeArgumentListItem(std::optional<TokenSyntax> comma, ArgumentSyntax argument,
                                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListClauseSyntax makeArgumentListClause(TokenSyntax leftParen, std::optional<ArgumentListSyntax> arguments,
                                                          TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena = nullptr);
   static DereferencableClauseSyntax makeDereferencableClause(ExprSyntax dereferencableExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static VariableClassNameClauseSyntax makeVariableClassNameClause(DereferencableClauseSyntax dereferencableExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassNameClauseSyntax makeClassNameClause(Syntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassNameRefClauseSyntax makeClassNameRefClause(Syntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static BraceDecoratedExprClauseSyntax makeBraceDecoratedExprClause(TokenSyntax leftParen, ExprSyntax expr,
                                                                      TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena = nullptr);
   static BraceDecoratedVariableExprSyntax makeBraceDecoratedVariableExpr(TokenSyntax dollarSign, BraceDecoratedExprClauseSyntax decoratedExpr,
                                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayKeyValuePairItemSyntax makeArrayKeyValuePairItem(std::optional<ExprSyntax> keyExpr, std::optional<TokenSyntax> doubleArrowToken,
                                                                ExprSyntax value, RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayUnpackPairItemSyntax makeArrayUnpackPairItem(TokenSyntax ellipsisToken, ExprSyntax unpackExpr,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairSyntax makeArrayPair(Syntax item, RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairListItemSyntax makeArrayPairListItem(std::optional<TokenSyntax> comma, std::optional<Syntax> arrayPair,
                                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static ListRecursivePairItemSyntax makeListRecursivePairItem(std::optional<ExprSyntax> keyExpr, std::optional<TokenSyntax> doubleArrowToken,
                                                                TokenSyntax listToken, TokenSyntax leftParen,
                                                                ArrayPairListSyntax arrayPairList, TokenSyntax rightParen,
                                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static ListPairItemSyntax makeListPairItem(Syntax item, std::optional<TokenSyntax> trailingComma,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleVariableExprSyntax makeSimpleVariableExpr(std::optional<TokenSyntax> dollarSign, Syntax variable,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayCreateExprSyntax makeArrayCreateExpr(TokenSyntax arrayToken, TokenSyntax leftParen,
                                                    ArrayPairListSyntax pairItemList, TokenSyntax rightParen,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static SimplifiedArrayCreateExprSyntax makeSimplifiedArrayCreateExpr(TokenSyntax leftSquareBracket, ArrayPairListSyntax pairItemList,
                                                                        TokenSyntax rightSquareBracket, RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayAccessExprSyntax makeArrayAccessExpr(Syntax arrayRef, TokenSyntax leftSquareBracket,
                                                    Syntax offset, TokenSyntax rightSquareBracket,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static BraceDecoratedArrayAccessExprSyntax makeBraceDecoratedArrayAccessExpr(Syntax arrayRef, BraceDecoratedExprClauseSyntax offsetExpr,
                                                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleFunctionCallExprSyntax makeSimpleFunctionCallExpr(Syntax funcName, ArgumentListClauseSyntax argumentsClause,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static FunctionCallExprSyntax makeFunctionCallExpr(ExprSyntax callable, RefCountPtr<SyntaxArena> arena = nullptr);
   static InstanceMethodCallExprSyntax makeInstanceMethodCallExpr(InstancePropertyExprSyntax qualifiedMethodName,
                                                                  ArgumentListClauseSyntax argumentListClause,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticMethodCallExprSyntax makeStaticMethodCallExpr(Syntax className, TokenSyntax separator,
                                                              MemberNameClauseSyntax methodName, ArgumentListClauseSyntax arguments,
                                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static DereferencableScalarExprSyntax makeDereferencableScalarExpr(Syntax scalarValue, RefCountPtr<SyntaxArena> arena = nullptr);
   static AnonymousClassDefinitionClauseSyntax makeAnonymousClassDefinitionClause(
         TokenSyntax classToken, std::optional<ArgumentListClauseSyntax> ctorArguments,
         std::optional<ExtendsFromClauseSyntax> extendsFrom, std::optional<ImplementsClauseSyntax> implementsList,
         MemberDeclBlockSyntax members, RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleInstanceCreateExprSyntax makeSimpleInstanceCreateExpr(
         TokenSyntax newToken, ClassNameRefClauseSyntax className,
         std::optional<ArgumentListClauseSyntax> ctorArgsClause, RefCountPtr<SyntaxArena> arena = nullptr);
   static AnonymousInstanceCreateExprSyntax makeAnonymousInstanceCreateExpr(
         TokenSyntax newToken, AnonymousClassDefinitionClauseSyntax anonymousClassDef,
         RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassicLambdaExprSyntax makeClassicLambdaExpr(TokenSyntax funcToken, std::optional<TokenSyntax> returnRefToken,
                                                        ParameterClauseSyntax parameterListClause, std::optional<UseLexicalVariableClauseSyntax> lexicalVarsClause,
                                                        std::optional<ReturnTypeClauseSyntax> returnType, InnerCodeBlockStmtSyntax body,
                                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static SimplifiedLambdaExprSyntax makeSimplifiedLambdaExpr(TokenSyntax fnToken, std::optional<TokenSyntax> returnRefToken,
                                                              ParameterClauseSyntax parameterListClause, std::optional<ReturnTypeClauseSyntax> returnType,
                                                              TokenSyntax doubleArrowToken, ExprSyntax body,
                                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static LambdaExprSyntax makeLambdaExpr(std::optional<TokenSyntax> staticToken, ExprSyntax lambdaExpr,
                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static ScalarExprSyntax makeScalarExpr(Syntax value, RefCountPtr<SyntaxArena> arena = nullptr);
   static InstanceCreateExprSyntax makeInstanceCreateExpr(ExprSyntax createExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeClassRefParentExpr(TokenSyntax parentKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeClassRefSelfExpr(TokenSyntax selfKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeClassRefStaticExpr(TokenSyntax staticKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeIntegerLiteralExpr(TokenSyntax digits, RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeFloatLiteralExpr(TokenSyntax floatDigits, RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeStringLiteralExpr(TokenSyntax leftQuote, TokenSyntax text,
                                                        TokenSyntax rightQuote, RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBooleanLiteralExpr(TokenSyntax boolean, RefCountPtr<SyntaxArena> arena = nullptr);

   static IssetVariableSyntax makeIssetVariable(ExprSyntax expr, RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariableListItemSyntax makeIssetVariableListItem(std::optional<TokenSyntax> comma, IssetVariableSyntax variable,
                                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariablesClauseSyntax makeIssetVariablesClause(TokenSyntax leftParen, IssetVariablesListSyntax isSetVariablesList,
                                                              TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetFuncExprSyntax makeIssetFuncExpr(TokenSyntax isSetToken, IssetVariablesClauseSyntax isSetVariablesClause,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static EmptyFuncExprSyntax makeEmptyFuncExpr(TokenSyntax emptyToken, ParenDecoratedExprSyntax argumentsClause,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static IncludeExprSyntax makeIncludeExpr(TokenSyntax includeToken, ExprSyntax argExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static RequireExprSyntax makeRequireExpr(TokenSyntax requireToken, ExprSyntax argExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static EvalFuncExprSyntax makeEvalFuncExpr(TokenSyntax evalToken, ParenDecoratedExprSyntax argumentsClause,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static PrintFuncExprSyntax makePrintFuncExpr(TokenSyntax printToken, ExprSyntax argsExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static FuncLikeExprSyntax makeFuncLikeExpr(ExprSyntax funcLikeExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayStructureAssignmentExprSyntax makeArrayStructureAssignmentExpr(SimplifiedArrayCreateExprSyntax arrayStructure,
                                                                              TokenSyntax equalToken, ExprSyntax valueExpr,
                                                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static ListStructureClauseSyntax makeListStructureClause(TokenSyntax listToken, TokenSyntax leftParen,
                                                            ArrayPairListSyntax pairItemList, TokenSyntax rightParen,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static ListStructureAssignmentExprSyntax makeListStructureAssignmentExpr(
         ListStructureClauseSyntax listStrcuture, TokenSyntax equalToken,
         ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeAssignmentExpr(VariableExprSyntax target, TokenSyntax assignToken,
                                                  ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static CompoundAssignmentExprSyntax makeCompoundAssignmentExpr(VariableExprSyntax target, TokenSyntax compoundAssignToken,
                                                                  ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static LogicalExprSyntax makeLogicalExpr(ExprSyntax lhs, TokenSyntax logicalOperator, ExprSyntax rhs,
                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static BitLogicalExprSyntax makeBitLogicalExpr(ExprSyntax lhs, TokenSyntax bitLogicalOperator,
                                                  ExprSyntax rhs, RefCountPtr<SyntaxArena> arena = nullptr);
   static RelationExprSyntax makeRelationExpr(ExprSyntax lhs, TokenSyntax relationOperator, ExprSyntax rhs,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static CastExprSyntax makeCastExpr(TokenSyntax castOperator, ExprSyntax valueExpr,
                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static ExitExprArgClauseSyntax makeExitExprArgClause(TokenSyntax leftParen, std::optional<ExprSyntax> expr,
                                                        TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena = nullptr);
   static ExitExprSyntax makeExitExpr(TokenSyntax exitToken, std::optional<ExitExprArgClauseSyntax> argClause,
                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static YieldExprSyntax makeYieldExpr(TokenSyntax yieldToken, std::optional<ExprSyntax> keyExpr,
                                        std::optional<TokenSyntax> doubleArrowToken, std::optional<ExprSyntax> valueExpr,
                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static YieldFromExprSyntax makeYieldFromExpr(TokenSyntax yieldFromToken, ExprSyntax expr,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static CloneExprSyntax makeCloneExpr(TokenSyntax cloneToken, ExprSyntax expr, RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsVariableOffsetSyntax makeEncapsVariableOffset(std::optional<TokenSyntax> minusSign, TokenSyntax offset,
                                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsArrayVarSyntax makeEncapsArrayVar(TokenSyntax varToken, TokenSyntax leftSquareBracket,
                                                  EncapsVariableOffsetSyntax offset, TokenSyntax rightSquareBracket,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsObjPropSyntax makeEncapsObjProp(TokenSyntax varToken, TokenSyntax objOperatorToken,
                                                TokenSyntax identifierToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsDollarCurlyExprSyntax makeEncapsDollarCurlyExpr(TokenSyntax dollarOpenCurlyToken, ExprSyntax expr,
                                                                TokenSyntax closeCurlyToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsDollarCurlyVarSyntax makeEncapsDollarCurlyVariable(TokenSyntax dollarOpenCurlyToken, TokenSyntax varname,
                                                                   TokenSyntax closeCurlyToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsDollarCurlyArraySyntax makeEncapsDollarCurlyArray(TokenSyntax dollarOpenCurlyToken, TokenSyntax varname,
                                                                  TokenSyntax leftSquareBracket, ExprSyntax indexExpr,
                                                                  TokenSyntax rightSquareBracket, TokenSyntax closeCurlyToken,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsCurlyVariableSyntax makeEncapsCurlyVariable(TokenSyntax curlyOpen, VariableExprSyntax variable,
                                                            TokenSyntax closeCurlyToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsVariableSyntax makeEncapsVariable(Syntax var, RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsListItemSyntax makeEncapsListItem(std::optional<TokenSyntax> strLiteral, std::optional<EncapsVariableSyntax> encapsVar,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static BackticksClauseSyntax makeBackticksClause(Syntax backticks, RefCountPtr<SyntaxArena> arena = nullptr);
   static HeredocExprSyntax makeHeredocExpr(TokenSyntax startHeredocToken, std::optional<Syntax> text,
                                            TokenSyntax endHeredocToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsListStringExprSyntax makeEncapsListStringExpr(TokenSyntax leftQuote, EncapsItemListSyntax encapsList,
                                                              TokenSyntax rightQuote, RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeTernaryExpr(ExprSyntax conditionExpr, TokenSyntax questionMark, std::optional<ExprSyntax> firstChoice,
                                            TokenSyntax colonMark, ExprSyntax secondChoice, RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeSequenceExpr(ExprListSyntax elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makePrefixOperatorExpr(std::optional<TokenSyntax> operatorToken, ExprSyntax expr,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makePostfixOperatorExpr(ExprSyntax expr, TokenSyntax operatorToken,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBinaryOperatorExpr(ExprSyntax lhs, TokenSyntax operatorToken,
                                                          ExprSyntax rhs, RefCountPtr<SyntaxArena> arena = nullptr);
   static InstanceofExprSyntax makeInstanceofExpr(ExprSyntax instanceExpr, TokenSyntax instanceofToken,
                                                  ClassNameRefClauseSyntax classNameRef, RefCountPtr<SyntaxArena> arena = nullptr);
   static ShellCmdExprSyntax makeShellCmdExpr(TokenSyntax leftBacktick, BackticksClauseSyntax backticksExpr,
                                              TokenSyntax rightBacktick, RefCountPtr<SyntaxArena> arena = nullptr);
   static UseLexicalVariableClauseSyntax makeUseLexicalVariableClause(TokenSyntax useToken, TokenSyntax leftParen,
                                                            LexicalVariableListSyntax lexicalVars, TokenSyntax rightParen,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVariableSyntax makeLexicalVariable(std::optional<TokenSyntax> referenceToken, TokenSyntax variable,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVariableListItemSyntax makeLexicalVariableListItem(std::optional<TokenSyntax> comma, LexicalVariableSyntax variable,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);

   /// make blank nodes
   static ExprListSyntax makeBlankExprList(RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVariableListSyntax makeBlankLexicalVarList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairListSyntax makeBlankArrayPairList(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsItemListSyntax makeBlankEncapsItemList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListSyntax makeBlankArgumentList(RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariablesListSyntax makeBlankIssetVariablesList(RefCountPtr<SyntaxArena> arena = nullptr);

   static ParenDecoratedExprSyntax makeBlankParenDecoratedExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static NullExprSyntax makeBlankNullExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static OptionalExprSyntax makeBlankOptionalExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ExprListItemSyntax makeBlankExprListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static VariableExprSyntax makeBlankVariableExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ReferencedVariableExprSyntax makeBlankReferencedVariableExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstIdentifierExprSyntax makeBlankClassConstIdentifierExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ConstExprSyntax makeBlankConstExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static NewVariableClauseSyntax makeBlankNewVariableClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static CallableVariableExprSyntax makeBlankCallableVariableExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static CallableFuncNameClauseSyntax makeBlankCallableFuncNameClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberNameClauseSyntax makeBlankMemberNameClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static PropertyNameClauseSyntax makeBlankPropertyNameClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static InstancePropertyExprSyntax makeBlankInstancePropertyExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticPropertyExprSyntax makeBlankStaticPropertyExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentSyntax makeBlankArgument(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListItemSyntax makeBlankArgumentListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListClauseSyntax makeBlankArgumentListClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static DereferencableClauseSyntax makeBlankDereferencableClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static VariableClassNameClauseSyntax makeBlankVariableClassNameClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassNameClauseSyntax makeBlankClassNameClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassNameRefClauseSyntax makeBlankClassNameRefClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static BraceDecoratedExprClauseSyntax makeBlankBraceDecoratedExprClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static BraceDecoratedVariableExprSyntax makeBlankBraceDecoratedVariableExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayKeyValuePairItemSyntax makeBlankArrayKeyValuePairItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayUnpackPairItemSyntax makeBlankArrayUnpackPairItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairSyntax makeBlankArrayPair(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairListItemSyntax makeBlankArrayPairListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static ListRecursivePairItemSyntax makeBlankListRecursivePairItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static ListPairItemSyntax makeBlankListPairItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleVariableExprSyntax makeBlankSimpleVariableExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayCreateExprSyntax makeBlankArrayCreateExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static SimplifiedArrayCreateExprSyntax makeBlankSimplifiedArrayCreateExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayAccessExprSyntax makeBlankArrayAccessExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BraceDecoratedArrayAccessExprSyntax makeBlankBraceDecoratedArrayAccessExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleFunctionCallExprSyntax makeBlankSimpleFunctionCallExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static FunctionCallExprSyntax makeBlankFunctionCallExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static InstanceMethodCallExprSyntax makeBlankInstanceMethodCallExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static StaticMethodCallExprSyntax makeBlankStaticMethodCallExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static DereferencableScalarExprSyntax makeBlankDereferencableScalarExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static AnonymousClassDefinitionClauseSyntax makeBlankAnonymousClassDefinitionClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleInstanceCreateExprSyntax makeBlankSimpleInstanceCreateExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static AnonymousInstanceCreateExprSyntax makeBlankAnonymousInstanceCreateExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassicLambdaExprSyntax makeBlankClassicLambdaExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static SimplifiedLambdaExprSyntax makeBlankSimplifiedLambdaExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static LambdaExprSyntax makeBlankLambdaExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ScalarExprSyntax makeBlankScalarExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static InstanceCreateExprSyntax makeBlankInstanceCreateExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeBlankClassRefParentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeBlankClassRefSelfExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeBlankClassRefStaticExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeBlankIntegerLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeBlankFloatLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeBlankStringLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBlankBooleanLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariableListItemSyntax makeBlankIssetVariableListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariablesClauseSyntax makeBlankIssetVariablesClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetFuncExprSyntax makeBlankIssetFuncExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static EmptyFuncExprSyntax makeBlankEmptyFuncExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static IncludeExprSyntax makeBlankIncludeExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static RequireExprSyntax makeBlankRequireExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static EvalFuncExprSyntax makeBlankEvalFuncExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static PrintFuncExprSyntax makeBlankPrintFuncExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static FuncLikeExprSyntax makeBlankFuncLikeExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayStructureAssignmentExprSyntax makeBlankArrayStructureAssignmentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ListStructureClauseSyntax makeBlankListStructureClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ListStructureAssignmentExprSyntax makeBlankListStructureAssignmentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeBlankAssignmentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static CompoundAssignmentExprSyntax makeBlankCompoundAssignmentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static LogicalExprSyntax makeBlankLogicalExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BitLogicalExprSyntax makeBlankBitLogicalExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static RelationExprSyntax makeBlankRelationExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static CastExprSyntax makeBlankCastExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ExitExprArgClauseSyntax makeBlankExitExprArgClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ExitExprSyntax makeBlankExitExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static YieldExprSyntax makeBlankYieldExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static YieldFromExprSyntax makeBlankYieldFromExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static CloneExprSyntax makeBlankCloneExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsVariableOffsetSyntax makeBlankEncapsVariableOffset(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsArrayVarSyntax makeBlankEncapsArrayVar(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsObjPropSyntax makeBlankEncapsObjProp(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsDollarCurlyExprSyntax makeBlankEncapsDollarCurlyExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsDollarCurlyVarSyntax makeBlankEncapsDollarCurlyVar(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsDollarCurlyArraySyntax makeBlankEncapsDollarCurlyArray(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsCurlyVariableSyntax makeBlankEncapsCurlyVar(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsVariableSyntax makeBlankEncapsVariable(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsListItemSyntax makeBlankEncapsListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static BackticksClauseSyntax makeBlankBackticksClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static HeredocExprSyntax makeBlankHeredocExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsListStringExprSyntax makeBlankEncapsListStringExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeBlankTernaryExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeBlankSequenceExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makeBlankPrefixOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makeBlankPostfixOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBlankBinaryOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBlankInstanceofExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ShellCmdExprSyntax makeBlankShellCmdExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static UseLexicalVariableClauseSyntax makeBlankUseLexicalVariableClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVariableSyntax makeBlankLexicalVariable(RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVariableListItemSyntax makeBlankLexicalVariableListItem(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H
