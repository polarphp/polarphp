// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/17.

#ifndef POLARPHP_SYNTAX_FACTORY_DECL_SYNTAX_NODE_FACTORY_H
#define POLARPHP_SYNTAX_FACTORY_DECL_SYNTAX_NODE_FACTORY_H

#include "polarphp/syntax/AbstractFactory.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodesFwd.h"

namespace polar::syntax {

class DeclSyntaxNodeFactory final : public AbstractFactory
{
public:
   ///
   /// make collection nodes
   ///
   static NameListSyntax makeNameList(
         const std::vector<NameListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ParameterListSyntax makeParameterList(
         const std::vector<ParameterSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassModifierListSyntax makeClassModififerList(
         const std::vector<ClassModifierSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberDeclListSyntax makeMemberDeclList(
         const std::vector<MemberDeclListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberModifierListSyntax makeMemberModifierList(
         const std::vector<MemberModifierSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassPropertyListSyntax makeClassPropertyList(
         const std::vector<ClassPropertyClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstListSyntax makeClassConstList(
         const std::vector<ClassConstClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAdaptationListSyntax makeClassTraitAdaptationList(
         const std::vector<ClassTraitAdaptationSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// make normal nodes
   ///
   static ReservedNonModifierSyntax makeReservedNonModifier(TokenSyntax modifier, RefCountPtr<SyntaxArena> arena = nullptr);
   static SemiReservedSytnax makeSemiReserved(Syntax modifier, RefCountPtr<SyntaxArena> arena = nullptr);
   static IdentifierSyntax makeIdentifier(Syntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceNameSyntax makeNamespaceName(std::optional<NamespaceNameSyntax> namespaceNs, std::optional<TokenSyntax> separator,
                                                TokenSyntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static NameSyntax makeName(std::optional<TokenSyntax> nsToken, std::optional<TokenSyntax> separator,
                              NamespaceNameSyntax namespaceName, RefCountPtr<SyntaxArena> arena = nullptr);
   static NameListItemSyntax makeNameListItem(std::optional<TokenSyntax> comma, NameSyntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static InitializerClauseSyntax makeInitializerClause(TokenSyntax equalToken, ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static TypeClauseSyntax makeTypeClause(Syntax type, RefCountPtr<SyntaxArena> arena = nullptr);
   static TypeExprClauseSyntax makeTypeExprClause(std::optional<TokenSyntax> questionToken, TypeClauseSyntax typeClause,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ReturnTypeClauseSyntax makeReturnTypeClause(TokenSyntax colonToken, TypeExprClauseSyntax typeExpr, RefCountPtr<SyntaxArena> arena = nullptr);
   static ParameterSyntax makeParameter(std::optional<TypeExprClauseSyntax> typeHint, std::optional<TokenSyntax> referenceMark,
                                        std::optional<TokenSyntax> variadicMark, TokenSyntax variable,
                                        std::optional<InitializerClauseSyntax> initializer, RefCountPtr<SyntaxArena> arena = nullptr);
   static ParameterClauseSyntax makeParameterClause(TokenSyntax leftParen, ParameterListSyntax parameters, TokenSyntax rightParen,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static FunctionDefinitionSyntax makeFunctionDefinition(TokenSyntax funcToken, std::optional<TokenSyntax> returnRefToken,
                                                          TokenSyntax funcName, ParameterClauseSyntax parameterListClause,
                                                          std::optional<ReturnTypeClauseSyntax> returnType, InnerCodeBlockStmtSyntax body,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassModifierSyntax makeClassModifier(TokenSyntax modifier, RefCountPtr<SyntaxArena> arena = nullptr);
   static ExtendsFromClauseSyntax makeExtendsFromClause(TokenSyntax extendsToken, NameSyntax name, RefCountPtr<SyntaxArena> arena = nullptr);
   static ImplementClauseSyntax makeImplementClause(TokenSyntax implementToken, NameListSyntax interfaces, RefCountPtr<SyntaxArena> arena = nullptr);
   static InterfaceExtendsClauseSyntax makeInterfaceExtendsClause(TokenSyntax extendsToken, NameListSyntax interfaces,
                                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassPropertyClauseSyntax makeClassPropertyClause(TokenSyntax variable, std::optional<InitializerClauseSyntax> initializer,
                                                            RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstClauseSyntax makeClassConstClause(IdentifierSyntax identifier, std::optional<InitializerClauseSyntax> initializer,
                                                      RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberModifierSyntax makeMemberModifier(TokenSyntax modifier, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassPropertyDeclSyntax makeClassPropertyDecl(MemberModifierListSyntax modifiers, std::optional<TypeExprClauseSyntax> typeHint,
                                                        ClassPropertyListSyntax propertyList, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstDeclSyntax makeClassConstDecl(MemberModifierListSyntax modifiers, TokenSyntax constToken, ClassConstListSyntax constList,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassMethodDeclSyntax makeClassMethodDecl(MemberModifierListSyntax modifiers, TokenSyntax functionToken, std::optional<TokenSyntax> returnRefToken,
                                                    IdentifierSyntax funcName, ParameterClauseSyntax ParameterListClause,
                                                    std::optional<ReturnTypeClauseSyntax> returnType, std::optional<MemberDeclBlockSyntax> body,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitMethodReferenceSyntax makeClassTraitMethodReference(Syntax reference, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassAbsoluteTraitMethodReferenceSyntax makeClassAbsoluteTraitMethodReference(NameSyntax baseName, TokenSyntax separator,
                                                                                        IdentifierSyntax memberName, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitPrecedenceSyntax makeClassTraitPrecedence(ClassAbsoluteTraitMethodReferenceSyntax reference, TokenSyntax insteadOfToken,
                                                              NameListSyntax names, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAliasSyntax makeClassTraitAlias(ClassTraitMethodReferenceSyntax methodReference, TokenSyntax asToken,
                                                    std::optional<Syntax> modifier, std::optional<Syntax> aliasName,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAdaptationSyntax makeClassTraitAdaptation(Syntax adaptation, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAdaptationBlockSyntax makeClassTraitAdaptationBlock(TokenSyntax leftBrace, ClassTraitAdaptationListSyntax adaptationList,
                                                                        TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitDeclSyntax makeClassTraitDecl(TokenSyntax useToken, NameListSyntax nameList, std::optional<ClassTraitAdaptationBlockSyntax> block,
                                                  RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberDeclListItemSyntax makeMemberDeclListItem(DeclSyntax decl, std::optional<TokenSyntax> semicolon,
                                                          RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberDeclBlockSyntax makeMemberDeclBlock(TokenSyntax leftBrace, MemberDeclListSyntax members,
                                                    TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassDefinitionSyntax makeClassDefinition(std::optional<ClassModifierListSyntax> modifiers, TokenSyntax classToken,
                                                    TokenSyntax name, std::optional<ExtendsFromClauseSyntax> extendsFrom,
                                                    std::optional<ImplementClauseSyntax> implementsList, MemberDeclBlockSyntax members,
                                                    RefCountPtr<SyntaxArena> arena = nullptr);
   static TraitDefinitionSyntax makeTraitDefinition(TokenSyntax traitToken, TokenSyntax name,
                                                    MemberDeclBlockSyntax members, RefCountPtr<SyntaxArena> arena = nullptr);
   static SourceFileSyntax makeSourceFile(TopStmtListSyntax statements, TokenSyntax eofToken, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// empty normal nodes
   ///
   static NameListSyntax makeBlankNameList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ParameterListSyntax makeBlankParameterList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassModifierListSyntax makeBlankClassModififerList(RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberDeclListSyntax makeBlankMemberDeclList(RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberModifierListSyntax makeBlankMemberModifierList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassPropertyListSyntax makeBlankClassPropertyList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstListSyntax makeBlankClassConstList(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAdaptationListSyntax makeBlankClassTraitAdaptationList(RefCountPtr<SyntaxArena> arena = nullptr);

   static ReservedNonModifierSyntax makeBlankReservedNonModifier(RefCountPtr<SyntaxArena> arena = nullptr);
   static SemiReservedSytnax makeBlankSemiReserved(RefCountPtr<SyntaxArena> arena = nullptr);
   static IdentifierSyntax makeBlankIdentifier( RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespaceNameSyntax makeBlankNamespacePart(RefCountPtr<SyntaxArena> arena = nullptr);
   static NameSyntax makeBlankName(RefCountPtr<SyntaxArena> arena = nullptr);
   static NameListItemSyntax makeBlankNameListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static InitializerClauseSyntax makeBlankInitializerClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static TypeClauseSyntax makeBlankTypeClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static TypeExprClauseSyntax makeBlankTypeExprClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ReturnTypeClauseSyntax makeBlankReturnTypeClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ParameterSyntax makeBlankParameter(RefCountPtr<SyntaxArena> arena = nullptr);
   static ParameterClauseSyntax makeBlankParameterClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static FunctionDefinitionSyntax makeBlankFunctionDefinition(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassModifierSyntax makeBlankClassModifier(RefCountPtr<SyntaxArena> arena = nullptr);
   static ExtendsFromClauseSyntax makeBlankExtendsFromClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ImplementClauseSyntax makeBlankImplementClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static InterfaceExtendsClauseSyntax makeBlankInterfaceExtendsClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassPropertyClauseSyntax makeBlankClassPropertyClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstClauseSyntax makeBlankClassConstClause(RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberModifierSyntax makeBlankMemberModifier(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassPropertyDeclSyntax makeBlankClassPropertyDecl(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstDeclSyntax makeBlankClassConstDecl(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassMethodDeclSyntax makeBlankClassMethodDecl(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitMethodReferenceSyntax makeBlankClassTraitMethodReference(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassAbsoluteTraitMethodReferenceSyntax makeBlankClassAbsoluteTraitMethodReference(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitPrecedenceSyntax makeBlankClassTraitPrecedence(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAliasSyntax makeBlankClassTraitAlias(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAdaptationSyntax makeBlankClassTraitAdaptation(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAdaptationBlockSyntax makeBlankClassTraitAdaptationBlock(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitDeclSyntax makeBlankClassTraitDecl(RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberDeclListItemSyntax makeBlankMemberDeclListItem(RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberDeclBlockSyntax makeBlankMemberDeclBlock(RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassDefinitionSyntax makeBlankClassDefinition(RefCountPtr<SyntaxArena> arena = nullptr);
   static TraitDefinitionSyntax makeBlankTraitDefinition(RefCountPtr<SyntaxArena> arena = nullptr);
   static SourceFileSyntax makeBlankSourceFile(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_DECL_SYNTAX_NODE_FACTORY_H
