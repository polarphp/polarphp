// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/15.

#include "polarphp/syntax/factory/ExprSyntaxNodeFactory.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::syntax {

///
/// make collection nodes
///

ExprListSyntax
ExprSyntaxNodeFactory::makeExprList(const std::vector<ExprSyntax> elements,
                                    RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ExprSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ExprList, layout, SourcePresence::Present,
                                                   arena);
   return make<ExprListSyntax>(target);
}

LexicalVarListSyntax
ExprSyntaxNodeFactory::makeLexicalVarList(const std::vector<LexicalVarItemSyntax> elements,
                                          RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const LexicalVarItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LexicalVarList, layout, SourcePresence::Present,
            arena);
   return make<LexicalVarListSyntax>(target);
}

ArrayPairListSyntax
ExprSyntaxNodeFactory::makeArrayPairList(const std::vector<ArrayPairListItemSyntax> elements,
                                         RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ArrayPairListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayPairList, layout, SourcePresence::Present,
            arena);
   return make<ArrayPairListSyntax>(target);
}

EncapsItemListSyntax
ExprSyntaxNodeFactory::makeEncapsItemList(const std::vector<EncapsListItemSyntax> elements,
                                          RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const EncapsListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsList, layout, SourcePresence::Present,
            arena);
   return make<EncapsItemListSyntax>(target);
}

ArgumentListSyntax
ExprSyntaxNodeFactory::makeArgumentList(const std::vector<ArgumentListItemSyntax> elements,
                                        RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const ArgumentListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArgumentList, layout, SourcePresence::Present,
            arena);
   return make<ArgumentListSyntax>(target);
}

IssetVariablesListSyntax
ExprSyntaxNodeFactory::makeIssetVariablesList(const std::vector<IssetVariableListItemSyntax> elements,
                                              RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const IssetVariableListItemSyntax &element : elements) {
      layout.push_back(element.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetVariablesList, layout, SourcePresence::Present,
            arena);
   return make<IssetVariablesListSyntax>(target);
}

///
/// make normal nodes
///
ParenDecoratedExprSyntax
ExprSyntaxNodeFactory::makeParenDecoratedExpr(TokenSyntax leftParen, ExprSyntax expr,
                                              TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ParenDecoratedExpr, {
               leftParen.getRaw(),
               expr.getRaw(),
               rightParen.getRaw()
            }, SourcePresence::Present, arena);
   return make<ParenDecoratedExprSyntax>(target);
}

NullExprSyntax
ExprSyntaxNodeFactory::makeNullExpr(TokenSyntax nullKeyword, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NullExpr, {
               nullKeyword.getRaw()
            }, SourcePresence::Present, arena);
   return make<NullExprSyntax>(target);
}

OptionalExprSyntax
ExprSyntaxNodeFactory::makeOptionalExpr(std::optional<ExprSyntax> expr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::OptionalExpr, {
               expr.has_value() ? expr->getRaw() : nullptr
            }, SourcePresence::Present, arena);
   return make<OptionalExprSyntax>(target);
}

ExprListItemSyntax
ExprSyntaxNodeFactory::makeExprListItem(ExprSyntax expr, std::optional<TokenSyntax> trailingComma,
                                        RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExprListItem, {
               expr.getRaw(),
               trailingComma.has_value() ? trailingComma->getRaw() : nullptr
            }, SourcePresence::Present, arena);
   return make<ExprListItemSyntax>(target);
}

VariableExprSyntax
ExprSyntaxNodeFactory::makeVariableExpr(ExprSyntax var, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::VariableExpr, {
               var.getRaw(),
            }, SourcePresence::Present, arena);
   return make<VariableExprSyntax>(target);
}

ReferencedVariableExprSyntax
ExprSyntaxNodeFactory::makeReferencedVariableExpr(TokenSyntax refToken, VariableExprSyntax variableExpr,
                                                  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ReferencedVariableExpr, {
               refToken.getRaw(),
               variableExpr.getRaw()
            }, SourcePresence::Present, arena);
   return make<ReferencedVariableExprSyntax>(target);
}

ClassConstIdentifierExprSyntax
ExprSyntaxNodeFactory::makeClassConstIdentifierExpr(Syntax className, TokenSyntax separatorToken,
                                                    IdentifierSyntax identifier, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassConstIdentifierExpr, {
               className.getRaw(),
               separatorToken.getRaw(),
               identifier.getRaw()
            }, SourcePresence::Present, arena);
   return make<ClassConstIdentifierExprSyntax>(target);
}

ConstExprSyntax
ExprSyntaxNodeFactory::makeConstExpr(Syntax identifier, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstExpr, {
               identifier.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ConstExprSyntax>(target);
}

NewVariableClauseSyntax
ExprSyntaxNodeFactory::makeNewVariableClause(ExprSyntax varNode, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NewVariableClause, {
               varNode.getRaw(),
            }, SourcePresence::Present, arena);
   return make<NewVariableClauseSyntax>(target);
}

CallableVariableExprSyntax
ExprSyntaxNodeFactory::makeCallableVariableExpr(ExprSyntax var, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CallableVariableExpr, {
               var.getRaw(),
            }, SourcePresence::Present, arena);
   return make<CallableVariableExprSyntax>(target);
}

CallableFuncNameClauseSyntax
ExprSyntaxNodeFactory::makeCallableFuncNameClause(Syntax funcName, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CallableFuncNameClause, {
               funcName.getRaw(),
            }, SourcePresence::Present, arena);
   return make<CallableFuncNameClauseSyntax>(target);
}

MemberNameClauseSyntax
ExprSyntaxNodeFactory::makeMemberNameClause(Syntax name, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::MemberNameClause, {
               name.getRaw(),
            }, SourcePresence::Present, arena);
   return make<MemberNameClauseSyntax>(target);
}

PropertyNameClauseSyntax
ExprSyntaxNodeFactory::makePropertyNameClause(Syntax name, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PropertyNameClause, {
               name.getRaw(),
            }, SourcePresence::Present, arena);
   return make<PropertyNameClauseSyntax>(target);
}

InstancePropertyExprSyntax
ExprSyntaxNodeFactory::makeInstancePropertyExpr(Syntax objectRef, TokenSyntax separator,
                                                Syntax propertyName, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InstancePropertyExpr, {
               objectRef.getRaw(),
               separator.getRaw(),
               propertyName.getRaw()
            }, SourcePresence::Present, arena);
   return make<InstancePropertyExprSyntax>(target);
}

StaticPropertyExprSyntax
ExprSyntaxNodeFactory::makeStaticPropertyExpr(Syntax className, TokenSyntax separator,
                                              SimpleVariableExprSyntax memberName, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticPropertyExpr, {
               className.getRaw(),
               separator.getRaw(),
               memberName.getRaw()
            }, SourcePresence::Present, arena);
   return make<StaticPropertyExprSyntax>(target);
}

ArgumentSyntax
ExprSyntaxNodeFactory::makeArgument(std::optional<TokenSyntax> ellipsisToken, ExprSyntax expr,
                                    RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::Argument, {
               ellipsisToken.has_value() ? ellipsisToken->getRaw() : nullptr,
               expr.getRaw()
            }, SourcePresence::Present, arena);
   return make<ArgumentSyntax>(target);
}

ArgumentListItemSyntax
ExprSyntaxNodeFactory::makeArgumentListItem(std::optional<TokenSyntax> comma, ArgumentSyntax argument,
                                            RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArgumentListItem, {
               argument.getRaw(),
               comma.has_value() ? comma->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<ArgumentListItemSyntax>(target);
}

ArgumentListClauseSyntax
ExprSyntaxNodeFactory::makeArgumentListClause(TokenSyntax leftParen, std::optional<ArgumentListSyntax> arguments,
                                              TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArgumentListClause, {
               leftParen.getRaw(),
               arguments.has_value() ? arguments->getRaw() : nullptr,
               rightParen.getRaw()
            }, SourcePresence::Present, arena);
   return make<ArgumentListClauseSyntax>(target);
}

DereferencableClauseSyntax
ExprSyntaxNodeFactory::makeDereferencableClause(ExprSyntax dereferencableExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DereferencableClause, {
               dereferencableExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<DereferencableClauseSyntax>(target);
}

VariableClassNameClauseSyntax
ExprSyntaxNodeFactory::makeVariableClassNameClause(DereferencableClauseSyntax dereferencableExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::VariableClassNameClause, {
               dereferencableExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<VariableClassNameClauseSyntax>(target);
}

ClassNameClauseSyntax
ExprSyntaxNodeFactory::makeClassNameClause(Syntax name, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassNameClause, {
               name.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ClassNameClauseSyntax>(target);
}

ClassNameRefClauseSyntax
ExprSyntaxNodeFactory::makeClassNameRefClause(Syntax name, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassNameRefClause, {
               name.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ClassNameRefClauseSyntax>(target);
}

BraceDecoratedExprClauseSyntax
ExprSyntaxNodeFactory::makeBraceDecoratedExprClause(TokenSyntax leftParen, ExprSyntax expr,
                                                    TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BraceDecoratedExprClause, {
               leftParen.getRaw(),
               expr.getRaw(),
               rightParen.getRaw()
            }, SourcePresence::Present, arena);
   return make<BraceDecoratedExprClauseSyntax>(target);
}

BraceDecoratedVariableExprSyntax
ExprSyntaxNodeFactory::makeBraceDecoratedVariableExpr(TokenSyntax dollarSign, BraceDecoratedExprClauseSyntax decoratedExpr,
                                                      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BraceDecoratedVariableExpr, {
               dollarSign.getRaw(),
               decoratedExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<BraceDecoratedVariableExprSyntax>(target);
}

ArrayKeyValuePairItemSyntax
ExprSyntaxNodeFactory::makeArrayKeyValuePairItem(std::optional<ExprSyntax> keyExpr, std::optional<TokenSyntax> doubleArrowToken,
                                                 ExprSyntax value, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayKeyValuePairItem, {
               keyExpr.has_value() ? keyExpr->getRaw() : nullptr,
               doubleArrowToken.has_value() ? doubleArrowToken->getRaw() : nullptr,
               value.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ArrayKeyValuePairItemSyntax>(target);
}

ArrayUnpackPairItemSyntax
ExprSyntaxNodeFactory::makeArrayUnpackPairItem(TokenSyntax ellipsisToken, ExprSyntax unpackExpr,
                                               RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayUnpackPairItem, {
               ellipsisToken.getRaw(),
               unpackExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ArrayUnpackPairItemSyntax>(target);
}

ArrayPairSyntax
ExprSyntaxNodeFactory::makeArrayPair(Syntax item, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayPair, {
               item.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ArrayPairSyntax>(target);
}

ArrayPairListItemSyntax
ExprSyntaxNodeFactory::makeArrayPairListItem(std::optional<TokenSyntax> comma, std::optional<Syntax> arrayPair,
                                             RefCountPtr<SyntaxArena> arena)
{
   SyntaxKind kind = arrayPair->getKind();
   assert(kind == SyntaxKind::ArrayPair || kind == SyntaxKind::ListRecursivePairItem);
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayPairListItem, {
               comma.has_value() ? comma->getRaw() : nullptr,
               arrayPair.has_value() ? arrayPair->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<ArrayPairListItemSyntax>(target);
}

ListRecursivePairItemSyntax
ExprSyntaxNodeFactory::makeListRecursivePairItem(std::optional<ExprSyntax> keyExpr, std::optional<TokenSyntax> doubleArrowToken,
                                                 TokenSyntax listToken, TokenSyntax leftParen,
                                                 ArrayPairListSyntax arrayPairList, TokenSyntax rightParen,
                                                 RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ListRecursivePairItem, {
               keyExpr.has_value() ? keyExpr->getRaw() : nullptr,
               doubleArrowToken.has_value() ? doubleArrowToken->getRaw() : nullptr,
               listToken.getRaw(),
               leftParen.getRaw(),
               arrayPairList.getRaw(),
               rightParen.getRaw()
            }, SourcePresence::Present, arena);
   return make<ListRecursivePairItemSyntax>(target);
}

SimpleVariableExprSyntax
ExprSyntaxNodeFactory::makeSimpleVariableExpr(std::optional<TokenSyntax> dollarSign, Syntax variable,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimpleVariableExpr, {
               dollarSign.has_value() ? dollarSign->getRaw() : nullptr,
               variable.getRaw(),
            }, SourcePresence::Present, arena);
   return make<SimpleVariableExprSyntax>(target);
}

ArrayCreateExprSyntax
ExprSyntaxNodeFactory::makeArrayCreateExpr(TokenSyntax arrayToken, TokenSyntax leftParen,
                                           ArrayPairListSyntax pairItemList, TokenSyntax rightParen,
                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayCreateExpr, {
               arrayToken.getRaw(),
               leftParen.getRaw(),
               pairItemList.getRaw(),
               rightParen.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ArrayCreateExprSyntax>(target);
}

SimplifiedArrayCreateExprSyntax
ExprSyntaxNodeFactory::makeSimplifiedArrayCreateExpr(TokenSyntax leftSquareBracket, ArrayPairListSyntax pairItemList,
                                                     TokenSyntax rightSquareBracket, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimplifiedArrayCreateExpr, {
               leftSquareBracket.getRaw(),
               pairItemList.getRaw(),
               rightSquareBracket.getRaw(),
            }, SourcePresence::Present, arena);
   return make<SimplifiedArrayCreateExprSyntax>(target);
}

ArrayAccessExprSyntax
ExprSyntaxNodeFactory::makeArrayAccessExpr(Syntax arrayRef, TokenSyntax leftSquareBracket,
                                           Syntax offset, TokenSyntax rightSquareBracket,
                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayAccessExpr, {
               arrayRef.getRaw(),
               leftSquareBracket.getRaw(),
               offset.getRaw(),
               rightSquareBracket.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ArrayAccessExprSyntax>(target);
}

BraceDecoratedArrayAccessExprSyntax
ExprSyntaxNodeFactory::makeBraceDecoratedArrayAccessExpr(Syntax arrayRef, BraceDecoratedExprClauseSyntax offsetExpr,
                                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BraceDecoratedArrayAccessExpr, {
               arrayRef.getRaw(),
               offsetExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<BraceDecoratedArrayAccessExprSyntax>(target);
}

SimpleFunctionCallExprSyntax
ExprSyntaxNodeFactory::makeSimpleFunctionCallExpr(Syntax funcName, ArgumentListClauseSyntax argumentsClause,
                                                  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimpleFunctionCallExpr, {
               funcName.getRaw(),
               argumentsClause.getRaw(),
            }, SourcePresence::Present, arena);
   return make<SimpleFunctionCallExprSyntax>(target);
}

FunctionCallExprSyntax
ExprSyntaxNodeFactory::makeFunctionCallExpr(ExprSyntax callable, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FunctionCallExpr, {
               callable.getRaw(),
            }, SourcePresence::Present, arena);
   return make<FunctionCallExprSyntax>(target);
}

InstanceMethodCallExprSyntax
ExprSyntaxNodeFactory::makeInstanceMethodCallExpr(InstancePropertyExprSyntax qualifiedMethodName,
                                                  ArgumentListClauseSyntax argumentListClause,
                                                  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InstanceMethodCallExpr, {
               qualifiedMethodName.getRaw(),
               argumentListClause.getRaw(),
            }, SourcePresence::Present, arena);
   return make<InstanceMethodCallExprSyntax>(target);
}

StaticMethodCallExprSyntax
ExprSyntaxNodeFactory::makeStaticMethodCallExpr(Syntax className, TokenSyntax separator,
                                                MemberNameClauseSyntax methodName, ArgumentListClauseSyntax arguments,
                                                RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticMethodCallExpr, {
               className.getRaw(),
               separator.getRaw(),
               methodName.getRaw(),
               arguments.getRaw()
            }, SourcePresence::Present, arena);
   return make<StaticMethodCallExprSyntax>(target);
}

DereferencableScalarExprSyntax
ExprSyntaxNodeFactory::makeDereferencableScalarExpr(Syntax scalarValue, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DereferencableScalarExpr, {
               scalarValue.getRaw(),
            }, SourcePresence::Present, arena);
   return make<DereferencableScalarExprSyntax>(target);
}

AnonymousClassDefinitionClauseSyntax
ExprSyntaxNodeFactory::makeAnonymousClassDefinitionClause(
      TokenSyntax classToken, std::optional<ArgumentListClauseSyntax> ctorArguments,
      std::optional<ExtendsFromClauseSyntax> extendsFrom, std::optional<ImplementClauseSyntax> implementsList,
      MemberDeclBlockSyntax members, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::AnonymousClassDefinitionClause, {
               classToken.getRaw(),
               ctorArguments.has_value() ? ctorArguments->getRaw() : nullptr,
               extendsFrom.has_value() ? extendsFrom->getRaw() : nullptr,
               implementsList.has_value() ? implementsList->getRaw() : nullptr,
               members.getRaw(),
            }, SourcePresence::Present, arena);
   return make<AnonymousClassDefinitionClauseSyntax>(target);
}

SimpleInstanceCreateExprSyntax
ExprSyntaxNodeFactory::makeSimpleInstanceCreateExpr(
      TokenSyntax newToken, ClassNameRefClauseSyntax className,
      std::optional<ArgumentListClauseSyntax> ctorArgsClause, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimpleInstanceCreateExpr, {
               newToken.getRaw(),
               className.getRaw(),
               ctorArgsClause.has_value() ? ctorArgsClause->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<SimpleInstanceCreateExprSyntax>(target);
}

AnonymousInstanceCreateExprSyntax
ExprSyntaxNodeFactory::makeAnonymousInstanceCreateExpr(
      TokenSyntax newToken, AnonymousClassDefinitionClauseSyntax anonymousClassDef,
      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::AnonymousInstanceCreateExpr, {
               newToken.getRaw(),
               anonymousClassDef.getRaw(),
            }, SourcePresence::Present, arena);
   return make<AnonymousInstanceCreateExprSyntax>(target);
}

ClassicLambdaExprSyntax
ExprSyntaxNodeFactory::makeClassicLambdaExpr(TokenSyntax funcToken, std::optional<TokenSyntax> returnRefToken,
                                             ParameterClauseSyntax parameterListClause, std::optional<UseLexicalVarClauseSyntax> lexicalVarsClause,
                                             std::optional<ReturnTypeClauseSyntax> returnType, InnerCodeBlockStmtSyntax body,
                                             RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassicLambdaExpr, {
               funcToken.getRaw(),
               returnRefToken.has_value() ? returnRefToken->getRaw() : nullptr,
               parameterListClause.getRaw(),
               lexicalVarsClause.has_value() ? lexicalVarsClause->getRaw() : nullptr,
               returnType.has_value() ? returnType->getRaw() : nullptr,
               body.getRaw()
            }, SourcePresence::Present, arena);
   return make<ClassicLambdaExprSyntax>(target);
}

SimplifiedLambdaExprSyntax
ExprSyntaxNodeFactory::makeSimplifiedLambdaExpr(TokenSyntax fnToken, std::optional<TokenSyntax> returnRefToken,
                                                ParameterClauseSyntax parameterListClause, std::optional<ReturnTypeClauseSyntax> returnType,
                                                TokenSyntax doubleArrowToken, ExprSyntax body,
                                                RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimplifiedLambdaExpr, {
               fnToken.getRaw(),
               returnRefToken.has_value() ? returnRefToken->getRaw() : nullptr,
               parameterListClause.getRaw(),
               returnType.has_value() ? returnType->getRaw() : nullptr,
               doubleArrowToken.getRaw(),
               body.getRaw()
            }, SourcePresence::Present, arena);
   return make<SimplifiedLambdaExprSyntax>(target);
}

LambdaExprSyntax
ExprSyntaxNodeFactory::makeLambdaExpr(std::optional<TokenSyntax> staticToken, ExprSyntax lambdaExpr,
                                      RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LambdaExpr, {
               staticToken.has_value() ? staticToken->getRaw() : nullptr,
               lambdaExpr.getRaw()
            }, SourcePresence::Present, arena);
   return make<LambdaExprSyntax>(target);
}

ScalarExprSyntax
ExprSyntaxNodeFactory::makeScalarExpr(Syntax value, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ScalarExpr, {
               value.getRaw()
            }, SourcePresence::Present, arena);
   return make<ScalarExprSyntax>(target);
}

InstanceCreateExprSyntax
ExprSyntaxNodeFactory::makeInstanceCreateExpr(ExprSyntax createExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InstanceCreateExpr, {
               createExpr.getRaw()
            }, SourcePresence::Present, arena);
   return make<InstanceCreateExprSyntax>(target);
}

ClassRefParentExprSyntax
ExprSyntaxNodeFactory::makeClassRefParentExpr(TokenSyntax parentKeyword,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassRefParentExpr, {
               parentKeyword.getRaw()
            }, SourcePresence::Present, arena);
   return make<ClassRefParentExprSyntax>(target);
}

ClassRefSelfExprSyntax
ExprSyntaxNodeFactory::makeClassRefSelfExpr(TokenSyntax selfKeyword, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassRefSelfExpr, {
               selfKeyword.getRaw()
            }, SourcePresence::Present, arena);
   return make<ClassRefSelfExprSyntax>(target);
}

ClassRefStaticExprSyntax
ExprSyntaxNodeFactory::makeClassRefStaticExpr(TokenSyntax staticKeyword,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassRefStaticExpr, {
               staticKeyword.getRaw()
            }, SourcePresence::Present, arena);
   return make<ClassRefStaticExprSyntax>(target);
}

IntegerLiteralExprSyntax
ExprSyntaxNodeFactory::makeIntegerLiteralExpr(TokenSyntax digits, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IntegerLiteralExpr, {
               digits.getRaw()
            }, SourcePresence::Present, arena);
   return make<IntegerLiteralExprSyntax>(target);
}

FloatLiteralExprSyntax
ExprSyntaxNodeFactory::makeFloatLiteralExpr(TokenSyntax floatDigits,  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FloatLiteralExpr, {
               floatDigits.getRaw()
            }, SourcePresence::Present, arena);
   return make<FloatLiteralExprSyntax>(target);
}

StringLiteralExprSyntax
ExprSyntaxNodeFactory::makeStringLiteralExpr(TokenSyntax leftQuote, TokenSyntax text,
                                             TokenSyntax rightQuote, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StringLiteralExpr, {
               leftQuote.getRaw(),
               text.getRaw(),
               rightQuote.getRaw()
            }, SourcePresence::Present, arena);
   return make<StringLiteralExprSyntax>(target);
}

BooleanLiteralExprSyntax
ExprSyntaxNodeFactory::makeBooleanLiteralExpr(TokenSyntax boolean,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BooleanLiteralExpr, {
               boolean.getRaw()
            }, SourcePresence::Present, arena);
   return make<BooleanLiteralExprSyntax>(target);
}

IssetVariableSyntax
ExprSyntaxNodeFactory::makeIssetVariable(ExprSyntax expr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetVariable, {
               expr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<IssetVariableSyntax>(target);
}

IssetVariableListItemSyntax
ExprSyntaxNodeFactory::makeIssetVariableListItem(std::optional<TokenSyntax> comma, IssetVariableSyntax variable,
                                                 RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetVariableListItem, {
               comma.has_value() ? comma->getRaw() : nullptr,
               variable.getRaw(),
            }, SourcePresence::Present, arena);
   return make<IssetVariableListItemSyntax>(target);
}

IssetVariablesClauseSyntax
ExprSyntaxNodeFactory::makeIssetVariablesClause(TokenSyntax leftParen, IssetVariablesListSyntax isSetVariablesList,
                                                TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetVariablesClause, {
               leftParen.getRaw(),
               isSetVariablesList.getRaw(),
               rightParen.getRaw()
            }, SourcePresence::Present, arena);
   return make<IssetVariablesClauseSyntax>(target);
}

IssetFuncExprSyntax
ExprSyntaxNodeFactory::makeIssetFuncExpr(TokenSyntax isSetToken, IssetVariablesClauseSyntax isSetVariablesClause,
                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetFuncExpr, {
               isSetToken.getRaw(),
               isSetVariablesClause.getRaw(),
            }, SourcePresence::Present, arena);
   return make<IssetFuncExprSyntax>(target);
}

EmptyFuncExprSyntax
ExprSyntaxNodeFactory::makeEmptyFuncExpr(TokenSyntax emptyToken, ParenDecoratedExprSyntax argumentsClause,
                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EmptyFuncExpr, {
               emptyToken.getRaw(),
               argumentsClause.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EmptyFuncExprSyntax>(target);
}

IncludeExprSyntax
ExprSyntaxNodeFactory::makeIncludeExpr(TokenSyntax includeToken, ExprSyntax argExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IncludeExpr, {
               includeToken.getRaw(),
               argExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<IncludeExprSyntax>(target);
}

RequireExprSyntax
ExprSyntaxNodeFactory::makeRequireExpr(TokenSyntax requireToken, ExprSyntax argExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::RequireExpr, {
               requireToken.getRaw(),
               argExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<RequireExprSyntax>(target);
}

EvalFuncExprSyntax
ExprSyntaxNodeFactory::makeEvalFuncExpr(TokenSyntax evalToken, ParenDecoratedExprSyntax argumentsClause,
                                        RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EvalFuncExpr, {
               evalToken.getRaw(),
               argumentsClause.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EvalFuncExprSyntax>(target);
}

PrintFuncExprSyntax
ExprSyntaxNodeFactory::makePrintFuncExpr(TokenSyntax printToken, ExprSyntax argsExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PrintFuncExpr, {
               printToken.getRaw(),
               argsExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<PrintFuncExprSyntax>(target);
}

FuncLikeExprSyntax
ExprSyntaxNodeFactory::makeFuncLikeExpr(ExprSyntax funcLikeExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FuncLikeExpr, {
               funcLikeExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<FuncLikeExprSyntax>(target);
}

ArrayStructureAssignmentExprSyntax
ExprSyntaxNodeFactory::makeArrayStructureAssignmentExpr(SimplifiedArrayCreateExprSyntax arrayStructure,
                                                        TokenSyntax equalToken, ExprSyntax valueExpr,
                                                        RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayStructureAssignmentExpr, {
               arrayStructure.getRaw(),
               equalToken.getRaw(),
               valueExpr.getRaw()
            }, SourcePresence::Present, arena);
   return make<ArrayStructureAssignmentExprSyntax>(target);
}

ListStructureClauseSyntax
ExprSyntaxNodeFactory::makeListStructureClause(TokenSyntax listToken, TokenSyntax leftParen,
                                               ArrayPairListSyntax pairItemList, TokenSyntax rightParen,
                                               RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ListStructureClause, {
               listToken.getRaw(),
               leftParen.getRaw(),
               pairItemList.getRaw(),
               rightParen.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ListStructureClauseSyntax>(target);
}

ListStructureAssignmentExprSyntax
ExprSyntaxNodeFactory::makeListStructureAssignmentExpr(
      ListStructureClauseSyntax listStrcuture, TokenSyntax equalToken,
      ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ListStructureAssignmentExpr, {
               listStrcuture.getRaw(),
               equalToken.getRaw(),
               valueExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ListStructureAssignmentExprSyntax>(target);
}

AssignmentExprSyntax
ExprSyntaxNodeFactory::makeAssignmentExpr(VariableExprSyntax target, TokenSyntax assignToken,
                                          ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> targetSyntaxNode = RawSyntax::make(
            SyntaxKind::AssignmentExpr, {
               target.getRaw(),
               assignToken.getRaw(),
               valueExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<AssignmentExprSyntax>(targetSyntaxNode);
}

CompoundAssignmentExprSyntax
ExprSyntaxNodeFactory::makeCompoundAssignmentExpr(VariableExprSyntax target, TokenSyntax compoundAssignToken,
                                                  ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> targetSyntaxNode = RawSyntax::make(
            SyntaxKind::CompoundAssignmentExpr, {
               target.getRaw(),
               compoundAssignToken.getRaw(),
               valueExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<CompoundAssignmentExprSyntax>(targetSyntaxNode);
}

LogicalExprSyntax
ExprSyntaxNodeFactory::makeLogicalExpr(ExprSyntax lhs, TokenSyntax logicalOperator, ExprSyntax rhs,
                                       RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LogicalExpr, {
               lhs.getRaw(),
               logicalOperator.getRaw(),
               rhs.getRaw(),
            }, SourcePresence::Present, arena);
   return make<LogicalExprSyntax>(target);
}

BitLogicalExprSyntax
ExprSyntaxNodeFactory::makeBitLogicalExpr(ExprSyntax lhs, TokenSyntax bitLogicalOperator,
                                          ExprSyntax rhs, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BitLogicalExpr, {
               lhs.getRaw(),
               bitLogicalOperator.getRaw(),
               rhs.getRaw(),
            }, SourcePresence::Present, arena);
   return make<BitLogicalExprSyntax>(target);
}

RelationExprSyntax
ExprSyntaxNodeFactory::makeRelationExpr(ExprSyntax lhs, TokenSyntax relationOperator, ExprSyntax rhs,
                                        RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::RelationExpr, {
               lhs.getRaw(),
               relationOperator.getRaw(),
               rhs.getRaw(),
            }, SourcePresence::Present, arena);
   return make<RelationExprSyntax>(target);
}

CastExprSyntax
ExprSyntaxNodeFactory::makeCastExpr(TokenSyntax castOperator, ExprSyntax valueExpr,
                                    RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CastExpr, {
               castOperator.getRaw(),
               valueExpr.getRaw(),
            }, SourcePresence::Present, arena);
   return make<CastExprSyntax>(target);
}

ExitExprArgClauseSyntax
ExprSyntaxNodeFactory::makeExitExprArgClause(TokenSyntax leftParen, std::optional<ExprSyntax> expr,
                                             TokenSyntax rightParen, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExitExprArgClause, {
               leftParen.getRaw(),
               expr.has_value() ? expr->getRaw() : nullptr,
               rightParen.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ExitExprArgClauseSyntax>(target);
}

ExitExprSyntax
ExprSyntaxNodeFactory::makeExitExpr(TokenSyntax exitToken, ExitExprArgClauseSyntax argClause,
                                    RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExitExpr, {
               exitToken.getRaw(),
               argClause.getRaw(),
            }, SourcePresence::Present, arena);
   return make<ExitExprSyntax>(target);
}

YieldExprSyntax
ExprSyntaxNodeFactory::makeYieldExpr(TokenSyntax yieldToken, std::optional<ExprSyntax> keyExpr,
                                     std::optional<TokenSyntax> doubleArrowToken, std::optional<ExprSyntax> valueExpr,
                                     RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::YieldExpr, {
               yieldToken.getRaw(),
               keyExpr.has_value() ? keyExpr->getRaw() : nullptr,
               doubleArrowToken.has_value() ? doubleArrowToken->getRaw() : nullptr,
               valueExpr.has_value() ? valueExpr->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<YieldExprSyntax>(target);
}

YieldFromExprSyntax
ExprSyntaxNodeFactory::makeYieldFromExpr(TokenSyntax yieldFromToken, ExprSyntax expr,
                                         RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::YieldFromExpr, {
               yieldFromToken.getRaw(),
               expr.getRaw()
            }, SourcePresence::Present, arena);
   return make<YieldFromExprSyntax>(target);
}

CloneExprSyntax
ExprSyntaxNodeFactory::makeCloneExpr(TokenSyntax cloneToken, ExprSyntax expr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CloneExpr, {
               cloneToken.getRaw(),
               expr.getRaw()
            }, SourcePresence::Present, arena);
   return make<CloneExprSyntax>(target);
}

EncapsVariableOffsetSyntax
ExprSyntaxNodeFactory::makeEncapsVariableOffset(std::optional<TokenSyntax> minusSign, TokenSyntax offset,
                                                RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsVariableOffset, {
               minusSign.has_value() ? minusSign->getRaw() : nullptr,
               offset.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EncapsVariableOffsetSyntax>(target);
}

EncapsArrayVarSyntax
ExprSyntaxNodeFactory::makeEncapsArrayVar(TokenSyntax varToken, TokenSyntax leftSquareBracket,
                                          EncapsVariableOffsetSyntax offset, TokenSyntax rightSquareBracket,
                                          RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsVariableOffset, {
               varToken.getRaw(),
               leftSquareBracket.getRaw(),
               offset.getRaw(),
               rightSquareBracket.getRaw()
            }, SourcePresence::Present, arena);
   return make<EncapsArrayVarSyntax>(target);
}

EncapsObjPropSyntax
ExprSyntaxNodeFactory::makeEncapsObjProp(TokenSyntax varToken, TokenSyntax objOperatorToken,
                                         TokenSyntax identifierToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsObjProp, {
               varToken.getRaw(),
               objOperatorToken.getRaw(),
               identifierToken.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EncapsObjPropSyntax>(target);
}

EncapsDollarCurlyExprSyntax
ExprSyntaxNodeFactory::makeEncapsDollarCurlyExpr(TokenSyntax dollarOpenCurlyToken, ExprSyntax expr,
                                                 TokenSyntax closeCurlyToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsDollarCurlyExpr, {
               dollarOpenCurlyToken.getRaw(),
               expr.getRaw(),
               closeCurlyToken.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EncapsDollarCurlyExprSyntax>(target);
}

EncapsDollarCurlyVarSyntax
ExprSyntaxNodeFactory::makeEncapsDollarCurlyVariable(TokenSyntax dollarOpenCurlyToken, TokenSyntax varname,
                                                     TokenSyntax closeCurlyToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsDollarCurlyVar, {
               dollarOpenCurlyToken.getRaw(),
               varname.getRaw(),
               closeCurlyToken.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EncapsDollarCurlyVarSyntax>(target);
}

EncapsDollarCurlyArraySyntax
ExprSyntaxNodeFactory::makeEncapsDollarCurlyArray(TokenSyntax dollarOpenCurlyToken, TokenSyntax varname,
                                                  TokenSyntax leftSquareBracket, ExprSyntax indexExpr,
                                                  TokenSyntax rightSquareBracket, TokenSyntax closeCurlyToken,
                                                  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsDollarCurlyArray, {
               dollarOpenCurlyToken.getRaw(),
               varname.getRaw(),
               leftSquareBracket.getRaw(),
               indexExpr.getRaw(),
               rightSquareBracket.getRaw(),
               closeCurlyToken.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EncapsDollarCurlyArraySyntax>(target);
}

EncapsCurlyVariableSyntax
ExprSyntaxNodeFactory::makeEncapsCurlyVariable(TokenSyntax curlyOpen, VariableExprSyntax variable,
                                               TokenSyntax closeCurlyToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsCurlyVariable, {
               curlyOpen.getRaw(),
               variable.getRaw(),
               closeCurlyToken.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EncapsCurlyVariableSyntax>(target);
}

EncapsVariableSyntax
ExprSyntaxNodeFactory::makeEncapsVariable(Syntax var, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsVariable, {
               var.getRaw(),
            }, SourcePresence::Present, arena);
   return make<EncapsVariableSyntax>(target);
}

EncapsListItemSyntax
ExprSyntaxNodeFactory::makeEncapsListItem(std::optional<TokenSyntax> strLiteral, std::optional<EncapsVariableSyntax> encapsVar,
                                          RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsListItem, {
               strLiteral.has_value() ? strLiteral->getRaw() : nullptr,
               encapsVar.has_value() ? encapsVar->getRaw() : nullptr,
            }, SourcePresence::Present, arena);
   return make<EncapsListItemSyntax>(target);
}

BackticksClauseSyntax
ExprSyntaxNodeFactory::makeBackticksClause(Syntax backticks, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BackticksClause, {
               backticks.getRaw(),
            }, SourcePresence::Present, arena);
   return make<BackticksClauseSyntax>(target);
}

HeredocExprSyntax
ExprSyntaxNodeFactory::makeHeredocExpr(TokenSyntax startHeredocToken, std::optional<Syntax> text,
                                       TokenSyntax endHeredocToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::HeredocExpr, {
               startHeredocToken.getRaw(),
               text.has_value() ? text->getRaw() : nullptr,
               endHeredocToken.getRaw()
            }, SourcePresence::Present, arena);
   return make<HeredocExprSyntax>(target);
}

EncapsListStringExprSyntax
ExprSyntaxNodeFactory::makeEncapsListStringExpr(TokenSyntax leftQuote, EncapsItemListSyntax encapsList,
                                                TokenSyntax rightQuote, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsListStringExpr, {
               leftQuote.getRaw(),
               encapsList.getRaw(),
               rightQuote.getRaw()
            }, SourcePresence::Present, arena);
   return make<EncapsListStringExprSyntax>(target);
}

TernaryExprSyntax
ExprSyntaxNodeFactory::makeTernaryExpr(ExprSyntax conditionExpr, TokenSyntax questionMark,
                                       std::optional<ExprSyntax> firstChoice, TokenSyntax colonMark,
                                       ExprSyntax secondChoice, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TernaryExpr, {
               conditionExpr.getRaw(),
               questionMark.getRaw(),
               firstChoice.has_value() ? firstChoice->getRaw() : nullptr,
               colonMark.getRaw(),
               secondChoice.getRaw()
            }, SourcePresence::Present, arena);
   return make<TernaryExprSyntax>(target);
}

SequenceExprSyntax
ExprSyntaxNodeFactory::makeSequenceExpr(ExprListSyntax elements, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SequenceExpr, {
               elements.getRaw()
            }, SourcePresence::Present, arena);
   return make<SequenceExprSyntax>(target);
}

PrefixOperatorExprSyntax
ExprSyntaxNodeFactory::makePrefixOperatorExpr(std::optional<TokenSyntax> operatorToken, ExprSyntax expr,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PrefixOperatorExpr, {
               operatorToken.has_value() ? operatorToken->getRaw() : nullptr,
               expr.getRaw()
            }, SourcePresence::Present, arena);
   return make<PrefixOperatorExprSyntax>(target);
}

PostfixOperatorExprSyntax
ExprSyntaxNodeFactory::makePostfixOperatorExpr(ExprSyntax expr, TokenSyntax operatorToken,
                                               RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PostfixOperatorExpr, {
               expr.getRaw(),
               operatorToken.getRaw()
            }, SourcePresence::Present, arena);
   return make<PostfixOperatorExprSyntax>(target);
}

BinaryOperatorExprSyntax
ExprSyntaxNodeFactory::makeBinaryOperatorExpr(ExprSyntax lhs, TokenSyntax operatorToken,
                                              ExprSyntax rhs, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BinaryOperatorExpr, {
               lhs.getRaw(),
               operatorToken.getRaw(),
               rhs.getRaw()
            }, SourcePresence::Present, arena);
   return make<BinaryOperatorExprSyntax>(target);
}

/// make blank nodes
ExprListSyntax
ExprSyntaxNodeFactory::makeBlankExprList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExprList, {},
            SourcePresence::Present, arena);
   return make<ExprListSyntax>(target);
}

LexicalVarListSyntax
ExprSyntaxNodeFactory::makeBlankLexicalVarList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LexicalVarList, {},
            SourcePresence::Present, arena);
   return make<LexicalVarListSyntax>(target);
}

ArrayPairListSyntax
ExprSyntaxNodeFactory::makeBlankArrayPairList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayPairList, {},
            SourcePresence::Present, arena);
   return make<ArrayPairListSyntax>(target);
}

EncapsItemListSyntax
ExprSyntaxNodeFactory::makeBlankEncapsItemList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsList, {},
            SourcePresence::Present, arena);
   return make<EncapsItemListSyntax>(target);
}

ArgumentListSyntax
ExprSyntaxNodeFactory::makeBlankArgumentList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArgumentList, {},
            SourcePresence::Present, arena);
   return make<ArgumentListSyntax>(target);
}

IssetVariablesListSyntax
ExprSyntaxNodeFactory::makeBlankIssetVariablesList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetVariablesList, {},
            SourcePresence::Present, arena);
   return make<IssetVariablesListSyntax>(target);
}

ParenDecoratedExprSyntax
ExprSyntaxNodeFactory::makeBlankParenDecoratedExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ParenDecoratedExpr, {
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::Expr), // Expr
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
            }, SourcePresence::Present, arena);
   return make<ParenDecoratedExprSyntax>(target);
}

NullExprSyntax
ExprSyntaxNodeFactory::makeBlankNullExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NullExpr, {
               make_missing_token(T_NULL) // NullKeyword
            }, SourcePresence::Present, arena);
   return make<NullExprSyntax>(target);
}

OptionalExprSyntax
ExprSyntaxNodeFactory::makeBlankOptionalExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::OptionalExpr, {
               RawSyntax::missing(SyntaxKind::Expr),// Expr
            }, SourcePresence::Present, arena);
   return make<OptionalExprSyntax>(target);
}

ExprListItemSyntax
ExprSyntaxNodeFactory::makeBlankExprListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExprListItem, {
               RawSyntax::missing(SyntaxKind::Expr),// Expr
               nullptr // TrailingComma
            }, SourcePresence::Present, arena);
   return make<ExprListItemSyntax>(target);
}

VariableExprSyntax
ExprSyntaxNodeFactory::makeBlankVariableExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::VariableExpr, {
               RawSyntax::missing(SyntaxKind::Expr),// Var
            }, SourcePresence::Present, arena);
   return make<VariableExprSyntax>(target);
}

ReferencedVariableExprSyntax
ExprSyntaxNodeFactory::makeBlankReferencedVariableExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ReferencedVariableExpr, {
               make_missing_token(T_AMPERSAND), // RefToken
               RawSyntax::missing(SyntaxKind::VariableExpr), // VariableExpr
            }, SourcePresence::Present, arena);
   return make<ReferencedVariableExprSyntax>(target);
}

ClassConstIdentifierExprSyntax
ExprSyntaxNodeFactory::makeBlankClassConstIdentifierExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassConstIdentifierExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // ClassName
               make_missing_token(T_PAAMAYIM_NEKUDOTAYIM), // SeparatorToken
               RawSyntax::missing(SyntaxKind::Identifier), // Identifier
            }, SourcePresence::Present, arena);
   return make<ClassConstIdentifierExprSyntax>(target);
}

ConstExprSyntax
ExprSyntaxNodeFactory::makeBlankConstExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ConstExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // Identifier
            }, SourcePresence::Present, arena);
   return make<ConstExprSyntax>(target);
}

NewVariableClauseSyntax
ExprSyntaxNodeFactory::makeBlankNewVariableClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::NewVariableClause, {
               RawSyntax::missing(SyntaxKind::UnknownExpr), // VarNode
            }, SourcePresence::Present, arena);
   return make<NewVariableClauseSyntax>(target);
}

CallableVariableExprSyntax
ExprSyntaxNodeFactory::makeBlankCallableVariableExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CallableVariableExpr, {
               RawSyntax::missing(SyntaxKind::UnknownExpr), // Var
            }, SourcePresence::Present, arena);
   return make<CallableVariableExprSyntax>(target);
}

CallableFuncNameClauseSyntax
ExprSyntaxNodeFactory::makeBlankCallableFuncNameClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CallableFuncNameClause, {
               RawSyntax::missing(SyntaxKind::Unknown), // FuncName
            }, SourcePresence::Present, arena);
   return make<CallableFuncNameClauseSyntax>(target);
}

MemberNameClauseSyntax
ExprSyntaxNodeFactory::makeBlankMemberNameClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::MemberNameClause, {
               RawSyntax::missing(SyntaxKind::Unknown), // Name
            }, SourcePresence::Present, arena);
   return make<MemberNameClauseSyntax>(target);
}

PropertyNameClauseSyntax
ExprSyntaxNodeFactory::makeBlankPropertyNameClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PropertyNameClause, {
               RawSyntax::missing(SyntaxKind::Unknown), // Name
            }, SourcePresence::Present, arena);
   return make<PropertyNameClauseSyntax>(target);
}

InstancePropertyExprSyntax
ExprSyntaxNodeFactory::makeBlankInstancePropertyExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InstancePropertyExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // ObjectRef
               make_missing_token(T_OBJECT_OPERATOR), // Separator
               RawSyntax::missing(SyntaxKind::Unknown) // PropertyName
            }, SourcePresence::Present, arena);
   return make<InstancePropertyExprSyntax>(target);
}

StaticPropertyExprSyntax
ExprSyntaxNodeFactory::makeBlankStaticPropertyExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticPropertyExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // ClassName
               make_missing_token(T_PAAMAYIM_NEKUDOTAYIM), // Separator
               RawSyntax::missing(SyntaxKind::SimpleVariableExpr) // MemberName
            }, SourcePresence::Present, arena);
   return make<StaticPropertyExprSyntax>(target);
}

ArgumentSyntax
ExprSyntaxNodeFactory::makeBlankArgument(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::Argument, {
               nullptr, // EllipsisToken
               RawSyntax::missing(SyntaxKind::Expr) // Expr
            }, SourcePresence::Present, arena);
   return make<ArgumentSyntax>(target);
}

ArgumentListItemSyntax
ExprSyntaxNodeFactory::makeBlankArgumentListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArgumentListItem, {
               RawSyntax::missing(SyntaxKind::Argument), // Argument
               nullptr // TrailingComma
            }, SourcePresence::Present, arena);
   return make<ArgumentListItemSyntax>(target);
}

ArgumentListClauseSyntax
ExprSyntaxNodeFactory::makeBlankArgumentListClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArgumentListClause, {
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::ArgumentList), // Arguments
               make_missing_token(T_LEFT_PAREN) // RightParenToken
            }, SourcePresence::Present, arena);
   return make<ArgumentListClauseSyntax>(target);
}

DereferencableClauseSyntax
ExprSyntaxNodeFactory::makeBlankDereferencableClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DereferencableClause, {
               RawSyntax::missing(SyntaxKind::Expr), // Arguments
            }, SourcePresence::Present, arena);
   return make<DereferencableClauseSyntax>(target);
}

VariableClassNameClauseSyntax
ExprSyntaxNodeFactory::makeBlankVariableClassNameClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::VariableClassNameClause, {
               RawSyntax::missing(SyntaxKind::DereferencableClause), // DereferencableExpr
            }, SourcePresence::Present, arena);
   return make<VariableClassNameClauseSyntax>(target);
}

ClassNameClauseSyntax
ExprSyntaxNodeFactory::makeBlankClassNameClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassNameClause, {
               RawSyntax::missing(SyntaxKind::Unknown), // Name
            }, SourcePresence::Present, arena);
   return make<ClassNameClauseSyntax>(target);
}

ClassNameRefClauseSyntax
ExprSyntaxNodeFactory::makeBlankClassNameRefClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassNameRefClause, {
               RawSyntax::missing(SyntaxKind::Unknown), // Name
            }, SourcePresence::Present, arena);
   return make<ClassNameRefClauseSyntax>(target);
}

BraceDecoratedExprClauseSyntax
ExprSyntaxNodeFactory::makeBlankBraceDecoratedExprClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BraceDecoratedExprClause, {
               make_missing_token(T_LEFT_PAREN), // LeftBrace
               RawSyntax::missing(SyntaxKind::Unknown), // Name
               make_missing_token(T_RIGHT_PAREN) // RightParen
            }, SourcePresence::Present, arena);
   return make<BraceDecoratedExprClauseSyntax>(target);
}

BraceDecoratedVariableExprSyntax
ExprSyntaxNodeFactory::makeBlankBraceDecoratedVariableExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BraceDecoratedVariableExpr, {
               make_missing_token(T_DOLLAR_SIGN), // DollarSign
               RawSyntax::missing(SyntaxKind::BraceDecoratedExprClause), // DecoratedExpr
            }, SourcePresence::Present, arena);
   return make<BraceDecoratedVariableExprSyntax>(target);
}

ArrayKeyValuePairItemSyntax
ExprSyntaxNodeFactory::makeBlankArrayKeyValuePairItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayKeyValuePairItem, {
               nullptr, // KeyExpr
               nullptr, // DoubleArrowToken
               RawSyntax::missing(SyntaxKind::Expr), // Value
            }, SourcePresence::Present, arena);
   return make<ArrayKeyValuePairItemSyntax>(target);
}

ArrayUnpackPairItemSyntax
ExprSyntaxNodeFactory::makeBlankArrayUnpackPairItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayUnpackPairItem, {
               make_missing_token(T_ELLIPSIS), // EllipsisToken
               RawSyntax::missing(SyntaxKind::Expr), // ExprSyntax
            }, SourcePresence::Present, arena);
   return make<ArrayUnpackPairItemSyntax>(target);
}

ArrayPairSyntax
ExprSyntaxNodeFactory::makeBlankArrayPair(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayPair, {
               RawSyntax::missing(SyntaxKind::Unknown), // Item
            }, SourcePresence::Present, arena);
   return make<ArrayPairSyntax>(target);
}

ArrayPairListItemSyntax
ExprSyntaxNodeFactory::makeBlankArrayPairListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayPairListItem, {
               nullptr, // Comma
               RawSyntax::missing(SyntaxKind::ArrayPair), // ArrayPair
            }, SourcePresence::Present, arena);
   return make<ArrayPairListItemSyntax>(target);
}

ListRecursivePairItemSyntax
ExprSyntaxNodeFactory::makeBlankListRecursivePairItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ListRecursivePairItem, {
               nullptr, // KeyExpr
               nullptr, // DoubleArrowToken
               make_missing_token(T_LIST), // ListToken
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::ArrayPairList), // ArrayPairList
               make_missing_token(T_RIGHT_PAREN), // RightParen
            }, SourcePresence::Present, arena);
   return make<ListRecursivePairItemSyntax>(target);
}

SimpleVariableExprSyntax
ExprSyntaxNodeFactory::makeBlankSimpleVariableExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimpleVariableExpr, {
               nullptr, // DollarSign
               RawSyntax::missing(SyntaxKind::Unknown), // Variable
            }, SourcePresence::Present, arena);
   return make<SimpleVariableExprSyntax>(target);
}

ArrayCreateExprSyntax
ExprSyntaxNodeFactory::makeBlankArrayCreateExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayCreateExpr, {
               make_missing_token(T_ARRAY), // ArrayToken
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::ArrayPairList), // PairItemList
               make_missing_token(T_RIGHT_PAREN), // RightParen
            }, SourcePresence::Present, arena);
   return make<ArrayCreateExprSyntax>(target);
}

SimplifiedArrayCreateExprSyntax
ExprSyntaxNodeFactory::makeBlankSimplifiedArrayCreateExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayCreateExpr, {
               make_missing_token(T_LEFT_SQUARE_BRACKET), // LeftSquareBracket
               RawSyntax::missing(SyntaxKind::ArrayPairList), // PairItemList
               make_missing_token(T_RIGHT_SQUARE_BRACKET), // RightSquareBracket
            }, SourcePresence::Present, arena);
   return make<SimplifiedArrayCreateExprSyntax>(target);
}

ArrayAccessExprSyntax
ExprSyntaxNodeFactory::makeBlankArrayAccessExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayAccessExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // ArrayRef
               make_missing_token(T_LEFT_SQUARE_BRACKET), // LeftSquareBracket
               RawSyntax::missing(SyntaxKind::Unknown), // Offset
               make_missing_token(T_RIGHT_SQUARE_BRACKET), // RightSquareBracket
            }, SourcePresence::Present, arena);
   return make<ArrayAccessExprSyntax>(target);
}

BraceDecoratedArrayAccessExprSyntax
ExprSyntaxNodeFactory::makeBlankBraceDecoratedArrayAccessExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BraceDecoratedArrayAccessExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // ArrayRef
               RawSyntax::missing(SyntaxKind::BraceDecoratedExprClause), // OffsetExpr
            }, SourcePresence::Present, arena);
   return make<BraceDecoratedArrayAccessExprSyntax>(target);
}

SimpleFunctionCallExprSyntax
ExprSyntaxNodeFactory::makeBlankSimpleFunctionCallExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimpleFunctionCallExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // FuncName
               RawSyntax::missing(SyntaxKind::ArgumentListClause), // ArgumentsClause
            }, SourcePresence::Present, arena);
   return make<SimpleFunctionCallExprSyntax>(target);
}

FunctionCallExprSyntax
ExprSyntaxNodeFactory::makeBlankFunctionCallExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FunctionCallExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // Callable
            }, SourcePresence::Present, arena);
   return make<FunctionCallExprSyntax>(target);
}

InstanceMethodCallExprSyntax
ExprSyntaxNodeFactory::makeBlankInstanceMethodCallExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InstanceMethodCallExpr, {
               RawSyntax::missing(SyntaxKind::InstancePropertyExpr), // QualifiedMethodName
               RawSyntax::missing(SyntaxKind::ArgumentListClause), // ArgumentListClause
            }, SourcePresence::Present, arena);
   return make<InstanceMethodCallExprSyntax>(target);
}

StaticMethodCallExprSyntax
ExprSyntaxNodeFactory::makeBlankStaticMethodCallExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StaticMethodCallExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // ClassName
               make_missing_token(T_PAAMAYIM_NEKUDOTAYIM), // Separator
               RawSyntax::missing(SyntaxKind::MemberNameClause), // MethodName
               RawSyntax::missing(SyntaxKind::ArgumentListClause), // Arguments
            }, SourcePresence::Present, arena);
   return make<StaticMethodCallExprSyntax>(target);
}

DereferencableScalarExprSyntax
ExprSyntaxNodeFactory::makeBlankDereferencableScalarExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::DereferencableScalarExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // ScalarValue
            }, SourcePresence::Present, arena);
   return make<DereferencableScalarExprSyntax>(target);
}

AnonymousClassDefinitionClauseSyntax
ExprSyntaxNodeFactory::makeBlankAnonymousClassDefinitionClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::AnonymousClassDefinitionClause, {
               make_missing_token(T_CLASS), // ClassToken
               nullptr, // CtorArguments
               nullptr, // ExtendsFrom
               nullptr, // ImplementsList
               RawSyntax::missing(SyntaxKind::MemberDeclBlock), // Members
            }, SourcePresence::Present, arena);
   return make<AnonymousClassDefinitionClauseSyntax>(target);
}

SimpleInstanceCreateExprSyntax
ExprSyntaxNodeFactory::makeBlankSimpleInstanceCreateExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimpleInstanceCreateExpr, {
               make_missing_token(T_NEW), // NewToken
               RawSyntax::missing(SyntaxKind::ClassNameRefClause), // ClassName
               nullptr, // CtorArgsClause
            }, SourcePresence::Present, arena);
   return make<SimpleInstanceCreateExprSyntax>(target);
}

AnonymousInstanceCreateExprSyntax
ExprSyntaxNodeFactory::makeBlankAnonymousInstanceCreateExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::AnonymousInstanceCreateExpr, {
               make_missing_token(T_NEW), // NewToken
               RawSyntax::missing(SyntaxKind::AnonymousClassDefinitionClause), // AnonymousClassDef
            }, SourcePresence::Present, arena);
   return make<AnonymousInstanceCreateExprSyntax>(target);
}

ClassicLambdaExprSyntax
ExprSyntaxNodeFactory::makeBlankClassicLambdaExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassicLambdaExpr, {
               make_missing_token(T_FUNCTION), // NewToken
               nullptr, // ReturnRefToken
               RawSyntax::missing(SyntaxKind::ParameterListClause), // ParameterListClause
               nullptr, // LexicalVarsClause
               nullptr, // ReturnType
               RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // Body
            }, SourcePresence::Present, arena);
   return make<ClassicLambdaExprSyntax>(target);
}

SimplifiedLambdaExprSyntax
ExprSyntaxNodeFactory::makeBlankSimplifiedLambdaExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SimplifiedLambdaExpr, {
               make_missing_token(T_FN), // FnToken
               nullptr, // ReturnRefToken
               RawSyntax::missing(SyntaxKind::ParameterListClause), // ParameterListClause
               nullptr, // ReturnType
               make_missing_token(T_DOUBLE_ARROW), // DoubleArrowToken
               RawSyntax::missing(SyntaxKind::Expr), // Body
            }, SourcePresence::Present, arena);
   return make<SimplifiedLambdaExprSyntax>(target);
}

LambdaExprSyntax
ExprSyntaxNodeFactory::makeBlankLambdaExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LambdaExpr, {
               nullptr, // StaticToken
               RawSyntax::missing(SyntaxKind::Expr), // LambdaExpr
            }, SourcePresence::Present, arena);
   return make<LambdaExprSyntax>(target);
}

ScalarExprSyntax
ExprSyntaxNodeFactory::makeBlankScalarExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ScalarExpr, {
               RawSyntax::missing(SyntaxKind::Unknown), // Value
            }, SourcePresence::Present, arena);
   return make<ScalarExprSyntax>(target);
}

InstanceCreateExprSyntax
ExprSyntaxNodeFactory::makeBlankInstanceCreateExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::InstanceCreateExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // CreateExpr
            }, SourcePresence::Present, arena);
   return make<InstanceCreateExprSyntax>(target);
}

ClassRefParentExprSyntax
ExprSyntaxNodeFactory::makeBlankClassRefParentExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassRefParentExpr, {
               make_missing_token(T_CLASS_REF_PARENT) // ParentKeyword
            }, SourcePresence::Present, arena);
   return make<ClassRefParentExprSyntax>(target);
}

ClassRefSelfExprSyntax
ExprSyntaxNodeFactory::makeBlankClassRefSelfExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassRefSelfExpr, {
               make_missing_token(T_CLASS_REF_SELF) // SelfKeyword
            }, SourcePresence::Present, arena);
   return make<ClassRefSelfExprSyntax>(target);
}

ClassRefStaticExprSyntax
ExprSyntaxNodeFactory::makeBlankClassRefStaticExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ClassRefStaticExpr, {
               make_missing_token(T_CLASS_REF_STATIC) // StaticKeyword
            }, SourcePresence::Present, arena);
   return make<ClassRefStaticExprSyntax>(target);
}

IntegerLiteralExprSyntax
ExprSyntaxNodeFactory::makeBlankIntegerLiteralExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IntegerLiteralExpr, {
               make_missing_token(T_LNUMBER) // Digits
            }, SourcePresence::Present, arena);
   return make<IntegerLiteralExprSyntax>(target);
}

FloatLiteralExprSyntax
ExprSyntaxNodeFactory::makeBlankFloatLiteralExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FloatLiteralExpr, {
               make_missing_token(T_DNUMBER) // FloatDigits
            }, SourcePresence::Present, arena);
   return make<FloatLiteralExprSyntax>(target);
}

StringLiteralExprSyntax
ExprSyntaxNodeFactory::makeBlankStringLiteralExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::StringLiteralExpr, {
               make_missing_token(T_DOUBLE_QUOTE), // LeftQuote
               make_missing_token(T_CONSTANT_ENCAPSED_STRING), // Text
               make_missing_token(T_DOUBLE_QUOTE), // RightQuote
            }, SourcePresence::Present, arena);
   return make<StringLiteralExprSyntax>(target);
}

BooleanLiteralExprSyntax
ExprSyntaxNodeFactory::makeBlankBooleanLiteralExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BooleanLiteralExpr, {
               make_missing_token(T_TRUE)
            }, SourcePresence::Present, arena);
   return make<BooleanLiteralExprSyntax>(target);
}

IssetVariableListItemSyntax
ExprSyntaxNodeFactory::makeBlankIssetVariableListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetVariableListItem, {
               nullptr, // comma
               RawSyntax::missing(SyntaxKind::IssetVariable), // Variable
            }, SourcePresence::Present, arena);
   return make<IssetVariableListItemSyntax>(target);
}

IssetVariablesClauseSyntax
ExprSyntaxNodeFactory::makeBlankIssetVariablesClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetVariablesClause, {
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::IssetVariablesList), // IsSetVariablesList
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
            }, SourcePresence::Present, arena);
   return make<IssetVariablesClauseSyntax>(target);
}

IssetFuncExprSyntax
ExprSyntaxNodeFactory::makeBlankIssetFuncExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IssetFuncExpr, {
               make_missing_token(T_ISSET), // IssetToken
               RawSyntax::missing(SyntaxKind::IssetVariablesClause), // IssetVariablesClause
            }, SourcePresence::Present, arena);
   return make<IssetFuncExprSyntax>(target);
}

EmptyFuncExprSyntax
ExprSyntaxNodeFactory::makeBlankEmptyFuncExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EmptyFuncExpr, {
               make_missing_token(T_EMPTY), // EmptyToken
               RawSyntax::missing(SyntaxKind::ParenDecoratedExpr), // ArgumentsClause
            }, SourcePresence::Present, arena);
   return make<EmptyFuncExprSyntax>(target);
}

IncludeExprSyntax
ExprSyntaxNodeFactory::makeBlankIncludeExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::IncludeExpr, {
               make_missing_token(T_INCLUDE), // IncludeToken
               RawSyntax::missing(SyntaxKind::Expr), // ArgExpr
            }, SourcePresence::Present, arena);
   return make<IncludeExprSyntax>(target);
}

RequireExprSyntax
ExprSyntaxNodeFactory::makeBlankRequireExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::RequireExpr, {
               make_missing_token(T_REQUIRE), // RequireToken
               RawSyntax::missing(SyntaxKind::Expr), // ArgExpr
            }, SourcePresence::Present, arena);
   return make<RequireExprSyntax>(target);
}

EvalFuncExprSyntax
ExprSyntaxNodeFactory::makeBlankEvalFuncExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EvalFuncExpr, {
               make_missing_token(T_EVAL), // EvalToken
               RawSyntax::missing(SyntaxKind::ParenDecoratedExpr), // ArgumentsClause
            }, SourcePresence::Present, arena);
   return make<EvalFuncExprSyntax>(target);
}

PrintFuncExprSyntax
ExprSyntaxNodeFactory::makeBlankPrintFuncExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PrintFuncExpr, {
               make_missing_token(T_PRINT), // PrintToken
               RawSyntax::missing(SyntaxKind::Expr), // ArgsExpr
            }, SourcePresence::Present, arena);
   return make<PrintFuncExprSyntax>(target);
}

FuncLikeExprSyntax
ExprSyntaxNodeFactory::makeBlankFuncLikeExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::FuncLikeExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // FuncLikeExpr
            }, SourcePresence::Present, arena);
   return make<FuncLikeExprSyntax>(target);
}

ArrayStructureAssignmentExprSyntax
ExprSyntaxNodeFactory::makeBlankArrayStructureAssignmentExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ArrayStructureAssignmentExpr, {
               RawSyntax::missing(SyntaxKind::SimplifiedArrayCreateExpr), // ArrayStructure
               make_missing_token(T_EQUAL), // EqualToken
               RawSyntax::missing(SyntaxKind::Expr), // ValueExpr
            }, SourcePresence::Present, arena);
   return make<ArrayStructureAssignmentExprSyntax>(target);
}

ListStructureClauseSyntax
ExprSyntaxNodeFactory::makeBlankListStructureClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ListStructureClause, {
               make_missing_token(T_LIST), // ListToken
               make_missing_token(T_LEFT_PAREN), // LeftParen
               RawSyntax::missing(SyntaxKind::ArrayPairList), // PairItemList
               make_missing_token(T_RIGHT_PAREN), // RightParen
            }, SourcePresence::Present, arena);
   return make<ListStructureClauseSyntax>(target);
}

ListStructureAssignmentExprSyntax
ExprSyntaxNodeFactory::makeBlankListStructureAssignmentExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ListStructureAssignmentExpr, {
               RawSyntax::missing(SyntaxKind::ListStructureClause), // ListStrcuture
               make_missing_token(T_EQUAL), // EqualToken
               RawSyntax::missing(SyntaxKind::Expr) // ValueExpr
            }, SourcePresence::Present, arena);
   return make<ListStructureAssignmentExprSyntax>(target);
}

AssignmentExprSyntax
ExprSyntaxNodeFactory::makeBlankAssignmentExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::AssignmentExpr, {
               RawSyntax::missing(SyntaxKind::VariableExpr), // Target
               make_missing_token(T_EQUAL), // AssignToken
               RawSyntax::missing(SyntaxKind::Expr), // ValueExpr
            }, SourcePresence::Present, arena);
   return make<AssignmentExprSyntax>(target);
}

CompoundAssignmentExprSyntax
ExprSyntaxNodeFactory::makeBlankCompoundAssignmentExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CompoundAssignmentExpr, {
               RawSyntax::missing(SyntaxKind::VariableExpr), // Target
               make_missing_token(T_PLUS_EQUAL), // CompoundAssignToken
               RawSyntax::missing(SyntaxKind::Expr), // ValueExpr
            }, SourcePresence::Present, arena);
   return make<CompoundAssignmentExprSyntax>(target);
}

LogicalExprSyntax
ExprSyntaxNodeFactory::makeBlankLogicalExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LogicalExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // Lhs
               make_missing_token(T_LOGICAL_AND), // LogicalOperator
               RawSyntax::missing(SyntaxKind::Expr), // Rhs
            }, SourcePresence::Present, arena);
   return make<LogicalExprSyntax>(target);
}

BitLogicalExprSyntax
ExprSyntaxNodeFactory::makeBlankBitLogicalExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BitLogicalExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // Lhs
               make_missing_token(T_AMPERSAND), // BitLogicalOperator
               RawSyntax::missing(SyntaxKind::Expr), // Rhs
            }, SourcePresence::Present, arena);
   return make<BitLogicalExprSyntax>(target);
}

RelationExprSyntax
ExprSyntaxNodeFactory::makeBlankRelationExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::RelationExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // Lhs
               make_missing_token(T_IS_IDENTICAL), // RelationOperator
               RawSyntax::missing(SyntaxKind::Expr), // Rhs
            }, SourcePresence::Present, arena);
   return make<RelationExprSyntax>(target);
}

CastExprSyntax
ExprSyntaxNodeFactory::makeBlankCastExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CastExpr, {
               make_missing_token(T_INT_CAST), // CastOperator
               RawSyntax::missing(SyntaxKind::Expr), // ValueExpr
            }, SourcePresence::Present, arena);
   return make<CastExprSyntax>(target);
}

ExitExprArgClauseSyntax
ExprSyntaxNodeFactory::makeBlankExitExprArgClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExitExprArgClause, {
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               nullptr, // ValueExpr
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
            }, SourcePresence::Present, arena);
   return make<ExitExprArgClauseSyntax>(target);
}

ExitExprSyntax
ExprSyntaxNodeFactory::makeBlankExitExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ExitExpr, {
               make_missing_token(T_EXIT), // ExitToken
               RawSyntax::missing(SyntaxKind::ExitExprArgClause), // ArgClause
            }, SourcePresence::Present, arena);
   return make<ExitExprSyntax>(target);
}

YieldExprSyntax
ExprSyntaxNodeFactory::makeBlankYieldExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::YieldExpr, {
               make_missing_token(T_YIELD), // YieldToken
               RawSyntax::missing(SyntaxKind::Expr), // KeyExpr
               make_missing_token(T_DOUBLE_ARROW), // DoubleArrowToken
               RawSyntax::missing(SyntaxKind::Expr), // ValueExpr
            }, SourcePresence::Present, arena);
   return make<YieldExprSyntax>(target);
}

YieldFromExprSyntax
ExprSyntaxNodeFactory::makeBlankYieldFromExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::YieldFromExpr, {
               make_missing_token(T_YIELD_FROM), // YieldFromToken
               RawSyntax::missing(SyntaxKind::Expr), // Expr
            }, SourcePresence::Present, arena);
   return make<YieldFromExprSyntax>(target);
}

CloneExprSyntax
ExprSyntaxNodeFactory::makeBlankCloneExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::CloneExpr, {
               make_missing_token(T_CLONE), // CloneToken
               RawSyntax::missing(SyntaxKind::Expr), // Expr
            }, SourcePresence::Present, arena);
   return make<CloneExprSyntax>(target);
}

EncapsVariableOffsetSyntax
ExprSyntaxNodeFactory::makeBlankEncapsVariableOffset(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsVariableOffset, {
               nullptr, // MinusSign
               make_missing_token(T_IDENTIFIER_STRING), // Offset
            }, SourcePresence::Present, arena);
   return make<EncapsVariableOffsetSyntax>(target);
}

EncapsArrayVarSyntax
ExprSyntaxNodeFactory::makeBlankEncapsArrayVar(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsArrayVar, {
               make_missing_token(T_VARIABLE), // VarToken
               make_missing_token(T_LEFT_SQUARE_BRACKET), // LeftSquareBracket
               RawSyntax::missing(SyntaxKind::EncapsVariableOffset), // Offset
               make_missing_token(T_RIGHT_SQUARE_BRACKET) // RightSquareBracket
            }, SourcePresence::Present, arena);
   return make<EncapsArrayVarSyntax>(target);
}

EncapsObjPropSyntax
ExprSyntaxNodeFactory::makeBlankEncapsObjProp(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsObjProp, {
               make_missing_token(T_VARIABLE), // VarToken
               make_missing_token(T_OBJECT_OPERATOR), // ObjOperatorToken
               make_missing_token(T_IDENTIFIER_STRING), // IdentifierToken
            }, SourcePresence::Present, arena);
   return make<EncapsObjPropSyntax>(target);
}

EncapsDollarCurlyExprSyntax
ExprSyntaxNodeFactory::makeBlankEncapsDollarCurlyExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsDollarCurlyExpr, {
               make_missing_token(T_DOLLAR_OPEN_CURLY_BRACES), // DollarOpenCurlyToken
               RawSyntax::missing(SyntaxKind::Expr), // Expr
               make_missing_token(T_RIGHT_BRACE), // CloseCurlyToken
            }, SourcePresence::Present, arena);
   return make<EncapsDollarCurlyExprSyntax>(target);
}

EncapsDollarCurlyVarSyntax
ExprSyntaxNodeFactory::makeBlankEncapsDollarCurlyVar(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsDollarCurlyVar, {
               make_missing_token(T_DOLLAR_OPEN_CURLY_BRACES), // DollarOpenCurlyToken
               make_missing_token(T_IDENTIFIER_STRING), // Varname
               make_missing_token(T_RIGHT_BRACE), // CloseCurlyToken
            }, SourcePresence::Present, arena);
   return make<EncapsDollarCurlyVarSyntax>(target);
}

EncapsDollarCurlyArraySyntax
ExprSyntaxNodeFactory::makeBlankEncapsDollarCurlyArray(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsDollarCurlyVar, {
               make_missing_token(T_DOLLAR_OPEN_CURLY_BRACES), // DollarOpenCurlyToken
               make_missing_token(T_IDENTIFIER_STRING), // Varname
               make_missing_token(T_LEFT_SQUARE_BRACKET), // LeftSquareBracketToken
               RawSyntax::missing(SyntaxKind::Expr), // IndexExpr
               make_missing_token(T_RIGHT_SQUARE_BRACKET), // RightSquareBracketToken
               make_missing_token(T_RIGHT_BRACE), // CloseCurlyToken
            }, SourcePresence::Present, arena);
   return make<EncapsDollarCurlyArraySyntax>(target);
}

EncapsCurlyVariableSyntax
ExprSyntaxNodeFactory::makeBlankEncapsCurlyVar(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsCurlyVariable, {
               make_missing_token(T_CURLY_OPEN), // CurlyOpen
               make_missing_token(T_VARIABLE), // Variable
               make_missing_token(T_RIGHT_BRACE), // CloseCurlyToken
            }, SourcePresence::Present, arena);
   return make<EncapsCurlyVariableSyntax>(target);
}

EncapsVariableSyntax
ExprSyntaxNodeFactory::makeBlankEncapsVariable(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsVariable, {
               RawSyntax::missing(SyntaxKind::Unknown) // Var
            }, SourcePresence::Present, arena);
   return make<EncapsVariableSyntax>(target);
}

EncapsListItemSyntax
ExprSyntaxNodeFactory::makeBlankEncapsListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsListItem, {
               make_missing_token(T_ENCAPSED_AND_WHITESPACE), // StrLiteral
               nullptr // EncapsVariable
            }, SourcePresence::Present, arena);
   return make<EncapsListItemSyntax>(target);
}

BackticksClauseSyntax
ExprSyntaxNodeFactory::makeBlankBackticksClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BackticksClause, {
               RawSyntax::missing(SyntaxKind::Unknown), // Backticks
            }, SourcePresence::Present, arena);
   return make<BackticksClauseSyntax>(target);
}

HeredocExprSyntax
ExprSyntaxNodeFactory::makeBlankHeredocExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::HeredocExpr, {
               make_missing_token(T_START_HEREDOC), // StartHeredocToken
               nullptr, // TextClause
               make_missing_token(T_START_HEREDOC), // EndHeredocToken
            }, SourcePresence::Present, arena);
   return make<HeredocExprSyntax>(target);
}

EncapsListStringExprSyntax
ExprSyntaxNodeFactory::makeBlankEncapsListStringExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::EncapsListStringExpr, {
               make_missing_token(T_DOUBLE_QUOTE), // StartHeredocToken
               RawSyntax::missing(SyntaxKind::EncapsListItem), // EncapsList
               make_missing_token(T_DOUBLE_QUOTE), // EndHeredocToken
            }, SourcePresence::Present, arena);
   return make<EncapsListStringExprSyntax>(target);
}

TernaryExprSyntax
ExprSyntaxNodeFactory::makeBlankTernaryExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::TernaryExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // ConditionExpr
               make_missing_token(T_QUESTION_MARK), // QuestionMark
               nullptr, // FirstChoice
               make_missing_token(T_COLON), // ColonMark
               RawSyntax::missing(SyntaxKind::Expr) // SecondChoice
            }, SourcePresence::Present, arena);
   return make<TernaryExprSyntax>(target);
}

SequenceExprSyntax
ExprSyntaxNodeFactory::makeBlankSequenceExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::SequenceExpr, {
               RawSyntax::missing(SyntaxKind::ExprList)
            }, SourcePresence::Present, arena);
   return make<SequenceExprSyntax>(target);
}

PrefixOperatorExprSyntax
ExprSyntaxNodeFactory::makeBlankPrefixOperatorExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PrefixOperatorExpr, {
               nullptr, // OperatorToken
               RawSyntax::missing(SyntaxKind::Expr) //Expr
            }, SourcePresence::Present, arena);
   return make<PrefixOperatorExprSyntax>(target);
}

PostfixOperatorExprSyntax
ExprSyntaxNodeFactory::makeBlankPostfixOperatorExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::PrefixOperatorExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // Expr
               make_missing_token(T_INC) // OperatorToken
            }, SourcePresence::Present, arena);
   return make<PostfixOperatorExprSyntax>(target);
}

BinaryOperatorExprSyntax
ExprSyntaxNodeFactory::makeBlankBinaryOperatorExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::BinaryOperatorExpr, {
               RawSyntax::missing(SyntaxKind::Expr), // Lhs
               make_missing_token(T_PLUS_SIGN), // OperatorToken
               RawSyntax::missing(SyntaxKind::Expr), // Rhs
            }, SourcePresence::Present, arena);
   return make<BinaryOperatorExprSyntax>(target);
}

ShellCmdExprSyntax
ExprSyntaxNodeFactory::makeBlankShellCmdExpr(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::ShellCmdExpr, {
               make_missing_token(T_BACKTICK), // LeftBacktick
               nullptr, // BackticksExpr
               make_missing_token(T_BACKTICK), // RightBacktick
            }, SourcePresence::Present, arena);
   return make<ShellCmdExprSyntax>(target);
}

UseLexicalVarClauseSyntax
ExprSyntaxNodeFactory::makeBlankUseLexicalVarClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::UseLexicalVarClause, {
               make_missing_token(T_USE), // UseToken
               make_missing_token(T_LEFT_PAREN), // LeftParenToken
               RawSyntax::missing(SyntaxKind::LexicalVarList), // LexicalVars
               make_missing_token(T_RIGHT_PAREN), // RightParenToken
            }, SourcePresence::Present, arena);
   return make<UseLexicalVarClauseSyntax>(target);
}

LexicalVarItemSyntax
ExprSyntaxNodeFactory::makeBlankLexicalVarItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(
            SyntaxKind::LexicalVarItem, {
               nullptr, // ReferenceToken
               make_missing_token(T_VARIABLE), // Variable
               nullptr, // TrailingComma
            }, SourcePresence::Present, arena);
   return make<LexicalVarItemSyntax>(target);
}

} // polar::syntax
