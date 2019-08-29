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
   static Syntax makeBlankCollection(SyntaxKind kind);
   ///
   /// make collection nodes
   ///
   static ExprListSyntax makeExprList(const std::vector<ExprSyntax> elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVarListSyntax makeLexicalVarList(const std::vector<LexicalVarItemSyntax> elements,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairItemListSyntax makeArrayPairItemList(const std::vector<ArrayPairItemSyntax> elements,
                                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static ListPairItemListSyntax makeListPairItemList(const std::vector<ListPairItemSyntax> elements,
                                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsItemListSyntax makeEncapsItemList(const std::vector<EncapsListItemSyntax> elements,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListSyntax makeArgumentList(const std::vector<ArgumentListItemSyntax> elements,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariablesListSyntax makeIssetVariablesList(const std::vector<IsSetVarItemSyntax> elements,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   ///
   /// make normal nodes
   ///
   ///
   static ParenDecoratedExprSyntax makeParenDecoratedExpr(TokenSyntax leftParen, ExprSyntax expr,
                                                          TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena = nullptr);
   static NullExprSyntax makeNullExpr(TokenSyntax nullKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static OptionalExprSyntax makeOptionalExpr(std::optional<ExprSyntax> expr, RefCountPtr<SyntaxArena> arena = nullptr);
   static ExprListItemSyntax makeExprListItem(ExprSyntax expr, std::optional<TokenSyntax> trailingComma,
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
   static ArgumentListItemSyntax makeArgumentListItem(ArgumentSyntax argument, std::optional<TokenSyntax> trailingComma,
                                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListClauseSyntax makeArgumentListClause(TokenSyntax leftParen, std::optional<ArgumentListSyntax> arguments,
                                                          TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena = nullptr);
   static DereferencableClauseSyntax makeDereferencableClause(ExprSyntax dereferencableExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static VariableClassNameClauseSyntax makeVariableClassNameClause(ExprSyntax dereferencableExpr, RefCountPtr<SyntaxArena> arena = nullptr);
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
   static ArrayPairItemSyntax makeArrayPairItem(Syntax item, std::optional<TokenSyntax> trailingComma,
                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static ListRecursivePairItemSyntax makeListRecursivePairItem(std::optional<ExprSyntax> keyExpr, std::optional<TokenSyntax> doubleArrowToken,
                                                                TokenSyntax listToken, TokenSyntax leftParen,
                                                                ListPairItemListSyntax listPairItemList, TokenSyntax rightParen,
                                                                RefCountPtr<SyntaxArena> arena = nullptr);
   static ListPairItemSyntax makeListPairItem(Syntax item, std::optional<TokenSyntax> trailingComma,
                                              RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleVariableExprSyntax makeSimpleVariableExpr(std::optional<TokenSyntax> dollarSign, Syntax variable,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayCreateExprSyntax makeArrayCreateExpr(TokenSyntax arrayToken, TokenSyntax leftParen,
                                                    ArrayPairItemListSyntax pairItemList, TokenSyntax rightParen,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static SimplifiedArrayCreateExprSyntax makeSimplifiedArrayCreateExpr(TokenSyntax leftSquareBracket, ArrayPairItemListSyntax pairItemList,
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
         ExtendsFromClauseSyntax extendsFrom, ImplementClauseSyntax implementsList,
         MemberDeclBlockSyntax members, RefCountPtr<SyntaxArena> arena = nullptr);
   static SimpleInstanceCreateExprSyntax makeSimpleInstanceCreateExpr(
         TokenSyntax newToken, ClassNameRefClauseSyntax className,
         std::optional<ArgumentListClauseSyntax> ctorArgsClause, RefCountPtr<SyntaxArena> arena = nullptr);
   static AnonymousInstanceCreateExprSyntax makeAnonymousInstanceCreateExpr(
         TokenSyntax newToken, AnonymousClassDefinitionClauseSyntax anonymousClassDef,
         RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassicLambdaExprSyntax makeClassicLambdaExpr(TokenSyntax funcToken, std::optional<TokenSyntax> returnRefToken,
                                                        ParameterClauseSyntax parameterListClause, std::optional<UseLexicalVarClauseSyntax> lexicalVarsClause,
                                                        std::optional<ReturnTypeClauseSyntax> returnType, InnerCodeBlockStmtSyntax body,
                                                        RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeClassRefParentExpr(TokenSyntax parentKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeClassRefSelfExpr(TokenSyntax selfKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeClassRefStaticExpr(TokenSyntax staticKeyword, RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeIntegerLiteralExpr(TokenSyntax digits, RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeFloatLiteralExpr(TokenSyntax floatDigits, RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeStringLiteralExpr(TokenSyntax str, RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBooleanLiteralExpr(TokenSyntax boolean, RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeTernaryExpr(ExprSyntax conditionExpr, TokenSyntax questionMark, ExprSyntax firstChoice,
                                            TokenSyntax colonMark, ExprSyntax secondChoice, RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeAssignmentExpr(TokenSyntax assignToken, RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeSequenceExpr(ExprListSyntax elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makePrefixOperatorExpr(std::optional<TokenSyntax> operatorToken, ExprSyntax expr,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makePostfixOperatorExpr(ExprSyntax expr, TokenSyntax operatorToken,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBinaryOperatorExpr(TokenSyntax operatorToken, RefCountPtr<SyntaxArena> arena = nullptr);

   /// make blank nodes
   static ExprListSyntax makeBlankExprList(RefCountPtr<SyntaxArena> arena = nullptr);
   static LexicalVarListSyntax makeBlankLexicalVarList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArrayPairItemListSyntax makeBlankArrayPairItemList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ListPairItemListSyntax makeBlankListPairItemList(RefCountPtr<SyntaxArena> arena = nullptr);
   static EncapsItemListSyntax makeBlankEncapsItemList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ArgumentListSyntax makeBlankArgumentList(RefCountPtr<SyntaxArena> arena = nullptr);
   static IssetVariablesListSyntax makeBlankIssetVariablesList(RefCountPtr<SyntaxArena> arena = nullptr);

   static NullExprSyntax makeNullExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefParentExprSyntax makeBlankClassRefParentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefSelfExprSyntax makeBlankClassRefSelfExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassRefStaticExprSyntax makeBlankClassRefStaticExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static IntegerLiteralExprSyntax makeBlankIntegerLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static FloatLiteralExprSyntax makeBlankFloatLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static StringLiteralExprSyntax makeBlankStringLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BooleanLiteralExprSyntax makeBlankBooleanLiteralExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static TernaryExprSyntax makeBlankTernaryExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static AssignmentExprSyntax makeBlankAssignmentExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static SequenceExprSyntax makeBlankSequenceExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static PrefixOperatorExprSyntax makeBlankPrefixOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static PostfixOperatorExprSyntax makeBlankPostfixOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static BinaryOperatorExprSyntax makeBlankBinaryOperatorExpr(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_EXPR_SYNTAX_NODE_FACTORY_H
