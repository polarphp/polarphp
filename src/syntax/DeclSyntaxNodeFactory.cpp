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

#include "polarphp/syntax/factory/DeclSyntaxNodeFactory.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::syntax {

///
/// make collection nodes
///
NameListSyntax DeclSyntaxNodeFactory::makeNameList(const std::vector<NameSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (const NameSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::NameList, layout, SourcePresence::Present, arena);
   return make<NameListSyntax>(target);
}

NamespacePartListSyntax DeclSyntaxNodeFactory::makeNamespacePartList(
      const std::vector<NamespacePartSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const NamespacePartSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::NamespacePartList, layout, SourcePresence::Present, arena);
   return make<NamespacePartListSyntax>(target);
}

ParameterListSyntax DeclSyntaxNodeFactory::makeParameterList(
      const std::vector<ParameterSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const ParameterSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ParameterList, layout, SourcePresence::Present, arena);
   return make<ParameterListSyntax>(target);
}

ClassModifierListSyntax DeclSyntaxNodeFactory::makeClassModififerList(
      const std::vector<ClassModifierSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const ClassModifierSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassModifierList, layout, SourcePresence::Present, arena);
   return make<ClassModifierListSyntax>(target);
}

MemberDeclListSyntax DeclSyntaxNodeFactory::makeMemberDeclList(
      const std::vector<MemberDeclListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const MemberDeclListItemSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberDeclList, layout, SourcePresence::Present, arena);
   return make<MemberDeclListSyntax>(target);
}

MemberModifierListSyntax DeclSyntaxNodeFactory::makeMemberModifierList(
      const std::vector<MemberModifierSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const MemberModifierSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberModifierList, layout, SourcePresence::Present, arena);
   return make<MemberModifierListSyntax>(target);
}

ClassPropertyListSyntax DeclSyntaxNodeFactory::makeClassPropertyList(
      const std::vector<ClassPropertyClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const ClassPropertyClauseSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassPropertyList, layout, SourcePresence::Present, arena);
   return make<ClassPropertyListSyntax>(target);
}

ClassConstListSyntax DeclSyntaxNodeFactory::makeClassConstList(
      const std::vector<ClassConstClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const ClassConstClauseSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassConstList, layout, SourcePresence::Present, arena);
   return make<ClassConstListSyntax>(target);
}

ClassTraitAdaptationListSyntax DeclSyntaxNodeFactory::makeClassTraitAdaptationList(
      const std::vector<ClassTraitAdaptationSyntax> &elements, RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   for (const ClassTraitAdaptationSyntax &item : elements) {
      layout.push_back(item.getRaw());
   }
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAdaptationList, layout, SourcePresence::Present, arena);
   return make<ClassTraitAdaptationListSyntax>(target);
}

///
/// make normal nodes
///
ReservedNonModifierSyntax
DeclSyntaxNodeFactory::makeReservedNonModifier(TokenSyntax modifier, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ReservedNonModifier, {
                                                      modifier.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ReservedNonModifierSyntax>(target);
}

SemiReservedSytnax
DeclSyntaxNodeFactory::makeSemiReserved(Syntax modifier, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::SemiReserved, {
                                                      modifier.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<SemiReservedSytnax>(target);
}

IdentifierSyntax
DeclSyntaxNodeFactory::makeIdentifier(Syntax name, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::Identifier, {
                                                      name.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<IdentifierSyntax>(target);
}

NamespacePartSyntax
DeclSyntaxNodeFactory::makeNamespacePart(std::optional<TokenSyntax> separator, TokenSyntax name, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::NamespacePart, {
                                                      separator.has_value() ? separator->getRaw() : nullptr,
                                                      name.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<NamespacePartSyntax>(target);
}

NameSyntax
DeclSyntaxNodeFactory::makeName(std::optional<TokenSyntax> nsToken, std::optional<TokenSyntax> separator,
                                NamespacePartListSyntax namespaceParts, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::Name, {
                                                      nsToken.has_value() ? nsToken->getRaw() : nullptr,
                                                      separator.has_value() ? separator->getRaw() : nullptr,
                                                      namespaceParts.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<NameSyntax>(target);
}

InitializerClauseSyntax
DeclSyntaxNodeFactory::makeInitializerClause(TokenSyntax equalToken, ExprSyntax valueExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::InitializerClause, {
                                                      equalToken.getRaw(),
                                                      valueExpr.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<InitializerClauseSyntax>(target);
}

TypeClauseSyntax
DeclSyntaxNodeFactory::makeTypeClause(Syntax type, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::TypeClause, {
                                                      type.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<TypeClauseSyntax>(target);
}

TypeExprClauseSyntax
DeclSyntaxNodeFactory::makeTypeExprClause(std::optional<TokenSyntax> questionToken, TypeClauseSyntax typeClause,
                                          RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::TypeExprClause, {
                                                      questionToken.has_value() ? questionToken->getRaw() : nullptr,
                                                      typeClause.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<TypeExprClauseSyntax>(target);
}

ReturnTypeClauseSyntax
DeclSyntaxNodeFactory::makeReturnTypeClause(TokenSyntax colonToken, TypeExprClauseSyntax typeExpr, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ReturnTypeClause, {
                                                      colonToken.getRaw(),
                                                      typeExpr.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ReturnTypeClauseSyntax>(target);
}

ParameterSyntax
DeclSyntaxNodeFactory::makeParameter(std::optional<TypeExprClauseSyntax> typeHint, std::optional<TokenSyntax> referenceMark,
                                     std::optional<TokenSyntax> variadicMark, TokenSyntax variable,
                                     std::optional<InitializerClauseSyntax> initializer, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ParameterItem, {
                                                      typeHint.has_value() ? typeHint->getRaw() : nullptr,
                                                      referenceMark.has_value() ? referenceMark->getRaw() : nullptr,
                                                      variadicMark.has_value() ? variadicMark->getRaw() : nullptr,
                                                      variable.getRaw(),
                                                      initializer.has_value() ? initializer->getRaw() : nullptr,
                                                   }, SourcePresence::Present, arena);
   return make<ParameterSyntax>(target);
}

ParameterClauseSyntax
DeclSyntaxNodeFactory::makeParameterClause(TokenSyntax leftParen, ParameterListSyntax parameters, TokenSyntax rightParen,
                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ParameterListClause, {
                                                      leftParen.getRaw(),
                                                      parameters.getRaw(),
                                                      rightParen.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ParameterClauseSyntax>(target);
}

FunctionDefinitionSyntax
DeclSyntaxNodeFactory::makeFunctionDefinition(TokenSyntax funcToken, std::optional<TokenSyntax> returnRefToken,
                                              TokenSyntax funcName, ParameterClauseSyntax parameterListClause,
                                              std::optional<ReturnTypeClauseSyntax> returnType, InnerCodeBlockStmtSyntax body,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ParameterListClause, {
                                                      funcToken.getRaw(),
                                                      returnRefToken.has_value() ? returnRefToken->getRaw() : nullptr,
                                                      funcName.getRaw(),
                                                      parameterListClause.getRaw(),
                                                      returnType.has_value() ? returnType->getRaw() : nullptr,
                                                      body.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<FunctionDefinitionSyntax>(target);
}

ClassModifierSyntax
DeclSyntaxNodeFactory::makeClassModifier(TokenSyntax modifier, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassModifier, {
                                                      modifier.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ClassModifierSyntax>(target);
}

ExtendsFromClauseSyntax
DeclSyntaxNodeFactory::makeExtendsFromClause(TokenSyntax extendsToken, NameSyntax name, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ExtendsFromClause, {
                                                      extendsToken.getRaw(),
                                                      name.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ExtendsFromClauseSyntax>(target);
}

ImplementClauseSyntax
DeclSyntaxNodeFactory::makeImplementClause(TokenSyntax implementToken, NameListSyntax interfaces, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ImplementsClause, {
                                                      implementToken.getRaw(),
                                                      interfaces.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ImplementClauseSyntax>(target);
}

InterfaceExtendsClauseSyntax
DeclSyntaxNodeFactory::makeInterfaceExtendsClause(TokenSyntax extendsToken, NameListSyntax interfaces,
                                                  RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::InterfaceExtendsClause, {
                                                      extendsToken.getRaw(),
                                                      interfaces.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<InterfaceExtendsClauseSyntax>(target);
}

ClassPropertyClauseSyntax
DeclSyntaxNodeFactory::makeClassPropertyClause(TokenSyntax variable, std::optional<InitializerClauseSyntax> initializer,
                                               RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassPropertyClause, {
                                                      variable.getRaw(),
                                                      initializer.has_value() ? initializer->getRaw() : nullptr,
                                                   }, SourcePresence::Present, arena);
   return make<ClassPropertyClauseSyntax>(target);
}

ClassConstClauseSyntax
DeclSyntaxNodeFactory::makeClassConstClause(IdentifierSyntax identifier, std::optional<InitializerClauseSyntax> initializer,
                                            RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassConstClause, {
                                                      identifier.getRaw(),
                                                      initializer.has_value() ? initializer->getRaw() : nullptr,
                                                   }, SourcePresence::Present, arena);
   return make<ClassConstClauseSyntax>(target);
}

MemberModifierSyntax
DeclSyntaxNodeFactory::makeMemberModifier(TokenSyntax modifier, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberModifier, {
                                                      modifier.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<MemberModifierSyntax>(target);
}

ClassPropertyDeclSyntax
DeclSyntaxNodeFactory::makeClassPropertyDecl(MemberModifierListSyntax modifiers, std::optional<TypeExprClauseSyntax> typeHint,
                                             ClassPropertyListSyntax propertyList, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassPropertyDecl, {
                                                      modifiers.getRaw(),
                                                      typeHint.has_value() ? typeHint->getRaw() : nullptr,
                                                      propertyList.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ClassPropertyDeclSyntax>(target);
}

ClassConstDeclSyntax
DeclSyntaxNodeFactory::makeClassConstDecl(MemberModifierListSyntax modifiers, TokenSyntax constToken, ClassConstListSyntax constList,
                                          RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassConstDecl, {
                                                      modifiers.getRaw(),
                                                      constToken.getRaw(),
                                                      constList.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ClassConstDeclSyntax>(target);
}

ClassMethodDeclSyntax
DeclSyntaxNodeFactory::makeClassMethodDecl(MemberModifierListSyntax modifiers, TokenSyntax functionToken, std::optional<TokenSyntax> returnRefToken,
                                           IdentifierSyntax funcName, ParameterClauseSyntax ParameterListClause,
                                           std::optional<ReturnTypeClauseSyntax> returnType, std::optional<MemberDeclBlockSyntax> body,
                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassMethodDecl, {
                                                      modifiers.getRaw(),
                                                      functionToken.getRaw(),
                                                      returnRefToken.has_value() ? returnRefToken->getRaw() : nullptr,
                                                      funcName.getRaw(),
                                                      ParameterListClause.getRaw(),
                                                      returnType.has_value() ? returnType->getRaw() : nullptr,
                                                      body.has_value() ? body->getRaw() : nullptr,
                                                   }, SourcePresence::Present, arena);
   return make<ClassMethodDeclSyntax>(target);
}

ClassTraitMethodReferenceSyntax
DeclSyntaxNodeFactory::makeClassTraitMethodReference(Syntax reference, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitMethodReference, {
                                                      reference.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<ClassTraitMethodReferenceSyntax>(target);
}

ClassAbsoluteTraitMethodReferenceSyntax
DeclSyntaxNodeFactory::makeClassAbsoluteTraitMethodReference(NameSyntax baseName, TokenSyntax separator,
                                                             IdentifierSyntax memberName, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassAbsoluteTraitMethodReference, {
                                                      baseName.getRaw(),
                                                      separator.getRaw(),
                                                      memberName.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<ClassAbsoluteTraitMethodReferenceSyntax>(target);
}

ClassTraitPrecedenceSyntax
DeclSyntaxNodeFactory::makeClassTraitPrecedence(ClassAbsoluteTraitMethodReferenceSyntax reference, TokenSyntax insteadOfToken,
                                                NameListSyntax names, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitPrecedence, {
                                                      reference.getRaw(),
                                                      insteadOfToken.getRaw(),
                                                      names.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<ClassTraitPrecedenceSyntax>(target);
}

ClassTraitAliasSyntax
DeclSyntaxNodeFactory::makeClassTraitAlias(ClassTraitMethodReferenceSyntax methodReference, TokenSyntax asToken,
                                           std::optional<Syntax> modifier, std::optional<Syntax> aliasName,
                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAlias, {
                                                      methodReference.getRaw(),
                                                      asToken.getRaw(),
                                                      modifier.has_value() ? modifier->getRaw() : nullptr,
                                                      aliasName.has_value() ? aliasName->getRaw() : nullptr,
                                                   }, SourcePresence::Present, arena);
   return make<ClassTraitAliasSyntax>(target);
}

ClassTraitAdaptationSyntax
DeclSyntaxNodeFactory::makeClassTraitAdaptation(Syntax adaptation, TokenSyntax semicolon, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAdaptation, {
                                                      adaptation.getRaw(),
                                                      semicolon.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<ClassTraitAdaptationSyntax>(target);
}

ClassTraitAdaptationBlockSyntax
DeclSyntaxNodeFactory::makeClassTraitAdaptationBlock(TokenSyntax leftBrace, ClassTraitAdaptationListSyntax adaptationList,
                                                     TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAdaptationBlock, {
                                                      leftBrace.getRaw(),
                                                      adaptationList.getRaw(),
                                                      rightBrace.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<ClassTraitAdaptationBlockSyntax>(target);
}

ClassTraitDeclSyntax
DeclSyntaxNodeFactory::makeClassTraitDecl(TokenSyntax useToken, NameListSyntax nameList, std::optional<ClassTraitAdaptationBlockSyntax> block,
                                          RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitDecl, {
                                                      useToken.getRaw(),
                                                      nameList.getRaw(),
                                                      block.has_value() ? block->getRaw() : nullptr,
                                                   }, SourcePresence::Present, arena);
   return make<ClassTraitDeclSyntax>(target);
}

MemberDeclListItemSyntax
DeclSyntaxNodeFactory::makeMemberDeclListItem(DeclSyntax decl, std::optional<TokenSyntax> semicolon,
                                              RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberDeclListItem, {
                                                      decl.getRaw(),
                                                      semicolon.has_value() ? semicolon->getRaw() : nullptr,
                                                   }, SourcePresence::Present, arena);
   return make<MemberDeclListItemSyntax>(target);
}

MemberDeclBlockSyntax
DeclSyntaxNodeFactory::makeMemberDeclBlock(TokenSyntax leftBrace, MemberDeclListSyntax members,
                                           TokenSyntax rightBrace, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberDeclBlock, {
                                                      leftBrace.getRaw(),
                                                      members.getRaw(),
                                                      rightBrace.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<MemberDeclBlockSyntax>(target);
}

ClassDefinitionSyntax
DeclSyntaxNodeFactory::makeClassDefinition(std::optional<ClassModifierListSyntax> modifiers, TokenSyntax classToken,
                                           TokenSyntax name, std::optional<ExtendsFromClauseSyntax> extendsFrom,
                                           std::optional<ImplementClauseSyntax> implementsList, MemberDeclBlockSyntax members,
                                           RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassDefinition, {
                                                      modifiers.has_value() ? modifiers->getRaw() : nullptr,
                                                      classToken.getRaw(),
                                                      name.getRaw(),
                                                      extendsFrom.has_value() ? extendsFrom->getRaw() : nullptr,
                                                      implementsList.has_value() ? implementsList->getRaw() : nullptr,
                                                      members.getRaw()
                                                   }, SourcePresence::Present, arena);
   return make<ClassDefinitionSyntax>(target);
}

TraitDefinitionSyntax
DeclSyntaxNodeFactory::makeTraitDefinition(TokenSyntax traitToken, TokenSyntax name,
                                           MemberDeclBlockSyntax members, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::TraitDefinition, {
                                                      traitToken.getRaw(),
                                                      name.getRaw(),
                                                      members.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<TraitDefinitionSyntax>(target);
}

SourceFileSyntax
DeclSyntaxNodeFactory::makeSourceFile(TopStmtListSyntax statements, TokenSyntax eofToken, RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::SourceFile, {
                                                      statements.getRaw(),
                                                      eofToken.getRaw(),
                                                   }, SourcePresence::Present, arena);
   return make<SourceFileSyntax>(target);
}

///
/// empty normal nodes
///
NameListSyntax DeclSyntaxNodeFactory::makeBlankNameList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::NameList, {},
                                                   SourcePresence::Present, arena);
   return make<NameListSyntax>(target);
}

NamespacePartListSyntax DeclSyntaxNodeFactory::makeBlankNamespacePartList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::NamespacePartList, {},
                                                   SourcePresence::Present, arena);
   return make<NamespacePartListSyntax>(target);
}

ParameterListSyntax DeclSyntaxNodeFactory::makeBlankParameterList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ParameterList, {},
                                                   SourcePresence::Present, arena);
   return make<ParameterListSyntax>(target);
}

ClassModifierListSyntax DeclSyntaxNodeFactory::makeBlankClassModififerList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassModifierList, {},
                                                   SourcePresence::Present, arena);
   return make<ClassModifierListSyntax>(target);
}

MemberDeclListSyntax DeclSyntaxNodeFactory::makeBlankMemberDeclList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberDeclList, {},
                                                   SourcePresence::Present, arena);
   return make<MemberDeclListSyntax>(target);
}

MemberModifierListSyntax DeclSyntaxNodeFactory::makeBlankMemberModifierList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberModifierList, {},
                                                   SourcePresence::Present, arena);
   return make<MemberModifierListSyntax>(target);
}

ClassPropertyListSyntax DeclSyntaxNodeFactory::makeBlankClassPropertyList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassPropertyList, {},
                                                   SourcePresence::Present, arena);
   return make<ClassPropertyListSyntax>(target);
}

ClassConstListSyntax DeclSyntaxNodeFactory::makeBlankClassConstList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassConstList, {},
                                                   SourcePresence::Present, arena);
   return make<ClassConstListSyntax>(target);
}

ClassTraitAdaptationListSyntax DeclSyntaxNodeFactory::makeBlankClassTraitAdaptationList(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAdaptationList, {},
                                                   SourcePresence::Present, arena);
   return make<ClassTraitAdaptationListSyntax>(target);
}

ReservedNonModifierSyntax DeclSyntaxNodeFactory::makeBlankReservedNonModifier(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ReservedNonModifier, {
                                                      make_missing_token(T_FUNCTION) // Modifier
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ReservedNonModifierSyntax>(target);
}

SemiReservedSytnax DeclSyntaxNodeFactory::makeBlankSemiReserved(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::SemiReserved, {
                                                      RawSyntax::missing(SyntaxKind::Unknown) // Modifier
                                                   },
                                                   SourcePresence::Present, arena);
   return make<SemiReservedSytnax>(target);
}

IdentifierSyntax DeclSyntaxNodeFactory::makeBlankIdentifier( RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::Identifier, {
                                                      RawSyntax::missing(SyntaxKind::Unknown) // NameItem
                                                   },
                                                   SourcePresence::Present, arena);
   return make<IdentifierSyntax>(target);
}

NamespacePartSyntax DeclSyntaxNodeFactory::makeBlankNamespacePart(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::NamespacePart, {
                                                      nullptr, // NsSeparator
                                                      make_missing_token(T_IDENTIFIER_STRING) // Name
                                                   },
                                                   SourcePresence::Present, arena);
   return make<NamespacePartSyntax>(target);
}

NameSyntax DeclSyntaxNodeFactory::makeBlankName(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::Name, {
                                                      nullptr, // NsToken
                                                      nullptr, // NsSeparator
                                                      RawSyntax::missing(SyntaxKind::NamespacePartList) // Namespace
                                                   },
                                                   SourcePresence::Present, arena);
   return make<NameSyntax>(target);
}

InitializerClauseSyntax DeclSyntaxNodeFactory::makeBlankInitializerClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::InitializerClause, {
                                                      make_missing_token(T_EQUAL), // EqualToken
                                                      RawSyntax::missing(SyntaxKind::UnknownExpr) // ValueExpr
                                                   },
                                                   SourcePresence::Present, arena);
   return make<InitializerClauseSyntax>(target);
}

TypeClauseSyntax DeclSyntaxNodeFactory::makeBlankTypeClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::TypeClause, {
                                                      RawSyntax::missing(SyntaxKind::Unknown) // Type
                                                   },
                                                   SourcePresence::Present, arena);
   return make<TypeClauseSyntax>(target);
}

TypeExprClauseSyntax DeclSyntaxNodeFactory::makeBlankTypeExprClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::TypeExprClause, {
                                                      nullptr, // QuestionToken
                                                      RawSyntax::missing(SyntaxKind::TypeClause) // TypeClause
                                                   },
                                                   SourcePresence::Present, arena);
   return make<TypeExprClauseSyntax>(target);
}

ReturnTypeClauseSyntax DeclSyntaxNodeFactory::makeBlankReturnTypeClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ReturnTypeClause, {
                                                      make_missing_token(T_COLON), // ColonToken
                                                      RawSyntax::missing(SyntaxKind::TypeExprClause) // TypeExpr
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ReturnTypeClauseSyntax>(target);
}

ParameterSyntax DeclSyntaxNodeFactory::makeBlankParameter(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ParameterItem, {
                                                      nullptr, // TypeHint
                                                      nullptr, // ReferenceMark
                                                      nullptr, // VariadicMark
                                                      make_missing_token(T_VARIABLE), // Variable
                                                      nullptr // Initializer
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ParameterSyntax>(target);
}

ParameterClauseSyntax DeclSyntaxNodeFactory::makeBlankParameterClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ParameterListClause, {
                                                      make_missing_token(T_LEFT_PAREN), // LeftParen
                                                      RawSyntax::missing(SyntaxKind::ParameterList), // Parameters
                                                      make_missing_token(T_RIGHT_PAREN), // RightParen
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ParameterClauseSyntax>(target);
}


FunctionDefinitionSyntax DeclSyntaxNodeFactory::makeBlankFunctionDefinition(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::FunctionDefinition, {
                                                      make_missing_token(T_FUNCTION), // FuncToken
                                                      nullptr, // ReturnRefToken
                                                      make_missing_token(T_IDENTIFIER_STRING), // FuncName
                                                      RawSyntax::missing(SyntaxKind::ParameterListClause), // ParameterListClause
                                                      nullptr, // ReturnType
                                                      RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt), // Body
                                                   },
                                                   SourcePresence::Present, arena);
   return make<FunctionDefinitionSyntax>(target);
}

ClassModifierSyntax DeclSyntaxNodeFactory::makeBlankClassModifier(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassModifier, {
                                                      make_missing_token(T_ABSTRACT), // Modifier
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassModifierSyntax>(target);
}

ExtendsFromClauseSyntax DeclSyntaxNodeFactory::makeBlankExtendsFromClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ExtendsFromClause, {
                                                      make_missing_token(T_EXTENDS), // ExtendToken
                                                      RawSyntax::missing(SyntaxKind::Name) // Name
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ExtendsFromClauseSyntax>(target);
}

ImplementClauseSyntax DeclSyntaxNodeFactory::makeBlankImplementClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ImplementsClause, {
                                                      make_missing_token(T_IMPLEMENTS), // ImplementToken
                                                      RawSyntax::missing(SyntaxKind::NameList) // Interfaces
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ImplementClauseSyntax>(target);
}

InterfaceExtendsClauseSyntax DeclSyntaxNodeFactory::makeBlankInterfaceExtendsClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::InterfaceExtendsClause, {
                                                      make_missing_token(T_EXTENDS), // ExtendsToken
                                                      RawSyntax::missing(SyntaxKind::NameList) // Interfaces
                                                   },
                                                   SourcePresence::Present, arena);
   return make<InterfaceExtendsClauseSyntax>(target);
}

ClassPropertyClauseSyntax DeclSyntaxNodeFactory::makeBlankClassPropertyClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassPropertyClause, {
                                                      make_missing_token(T_VARIABLE), // Variable
                                                      nullptr // Initializer
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassPropertyClauseSyntax>(target);
}

ClassConstClauseSyntax DeclSyntaxNodeFactory::makeBlankClassConstClause(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassConstClause, {
                                                      RawSyntax::missing(SyntaxKind::Identifier), // Identifier
                                                      RawSyntax::missing(SyntaxKind::InitializerClause) // Initializer
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassConstClauseSyntax>(target);
}

MemberModifierSyntax DeclSyntaxNodeFactory::makeBlankMemberModifier(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberModifier, {
                                                      make_missing_token(T_PUBLIC) // Modifier
                                                   },
                                                   SourcePresence::Present, arena);
   return make<MemberModifierSyntax>(target);
}

ClassPropertyDeclSyntax DeclSyntaxNodeFactory::makeBlankClassPropertyDecl(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassPropertyDecl, {
                                                      RawSyntax::missing(SyntaxKind::MemberModifierList), // Modifiers
                                                      RawSyntax::missing(SyntaxKind::TypeExprClause), // TypeHint
                                                      RawSyntax::missing(SyntaxKind::ClassPropertyList), // PropertyList
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassPropertyDeclSyntax>(target);
}

ClassConstDeclSyntax DeclSyntaxNodeFactory::makeBlankClassConstDecl(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassConstDecl, {
                                                      RawSyntax::missing(SyntaxKind::MemberModifierList), // Modifiers
                                                      make_missing_token(T_CONST), // ConstToken
                                                      RawSyntax::missing(SyntaxKind::ClassConstList), // ConstList
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassConstDeclSyntax>(target);
}

ClassMethodDeclSyntax DeclSyntaxNodeFactory::makeBlankClassMethodDecl(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassConstDecl, {
                                                      RawSyntax::missing(SyntaxKind::MemberModifierList), // Modifiers
                                                      make_missing_token(T_FUNCTION), // FunctionToken
                                                      nullptr, // ReturnRefToken
                                                      RawSyntax::missing(SyntaxKind::Identifier), // FuncName
                                                      RawSyntax::missing(SyntaxKind::ParameterListClause), // ConstList
                                                      nullptr, // ReturnTypeClauseSyntax
                                                      nullptr, // Body
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassMethodDeclSyntax>(target);
}

ClassTraitMethodReferenceSyntax DeclSyntaxNodeFactory::makeBlankClassTraitMethodReference(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitMethodReference, {
                                                      RawSyntax::missing(SyntaxKind::Unknown), // Reference
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassTraitMethodReferenceSyntax>(target);
}

ClassAbsoluteTraitMethodReferenceSyntax DeclSyntaxNodeFactory::makeBlankClassAbsoluteTraitMethodReference(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassAbsoluteTraitMethodReference, {
                                                      RawSyntax::missing(SyntaxKind::Name), // BaseName
                                                      make_missing_token(T_PAAMAYIM_NEKUDOTAYIM), // Separator
                                                      RawSyntax::missing(SyntaxKind::Identifier) // MemberName
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassAbsoluteTraitMethodReferenceSyntax>(target);
}

ClassTraitPrecedenceSyntax DeclSyntaxNodeFactory::makeBlankClassTraitPrecedence(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitPrecedence, {
                                                      RawSyntax::missing(SyntaxKind::ClassAbsoluteTraitMethodReference), // MethodReference
                                                      make_missing_token(T_INSTEADOF), // InsteadOfToken
                                                      RawSyntax::missing(SyntaxKind::NameList) // Names
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassTraitPrecedenceSyntax>(target);
}

ClassTraitAliasSyntax DeclSyntaxNodeFactory::makeBlankClassTraitAlias(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAlias, {
                                                      RawSyntax::missing(SyntaxKind::ClassTraitMethodReference), // MethodReference
                                                      make_missing_token(T_AS), // AsToken
                                                      nullptr, // Modifier
                                                      nullptr, // AliasName
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassTraitAliasSyntax>(target);
}

ClassTraitAdaptationSyntax DeclSyntaxNodeFactory::makeBlankClassTraitAdaptation(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAdaptation, {
                                                      RawSyntax::missing(SyntaxKind::Unknown), // Adaptation
                                                      make_missing_token(T_SEMICOLON), // Semicolon
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassTraitAdaptationSyntax>(target);
}

ClassTraitAdaptationBlockSyntax DeclSyntaxNodeFactory::makeBlankClassTraitAdaptationBlock(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitAdaptationBlock, {
                                                      make_missing_token(T_LEFT_PAREN), // LeftBrace
                                                      RawSyntax::missing(SyntaxKind::ClassTraitAdaptationList), // AdaptationList
                                                      make_missing_token(T_RIGHT_PAREN), // RightBrace
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassTraitAdaptationBlockSyntax>(target);
}

ClassTraitDeclSyntax DeclSyntaxNodeFactory::makeBlankClassTraitDecl(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassTraitDecl, {
                                                      make_missing_token(T_USE), // UseToken
                                                      RawSyntax::missing(SyntaxKind::NameList), // NameList
                                                      nullptr, // AdaptationBlock
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassTraitDeclSyntax>(target);
}

MemberDeclListItemSyntax DeclSyntaxNodeFactory::makeBlankMemberDeclListItem(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberDeclListItem, {
                                                      RawSyntax::missing(SyntaxKind::Decl), // Decl
                                                      nullptr, // Semicolon
                                                   },
                                                   SourcePresence::Present, arena);
   return make<MemberDeclListItemSyntax>(target);
}

MemberDeclBlockSyntax DeclSyntaxNodeFactory::makeBlankMemberDeclBlock(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::MemberDeclBlock, {
                                                      make_missing_token(T_LEFT_PAREN), // LeftBrace
                                                      RawSyntax::missing(SyntaxKind::MemberDeclList), // Members
                                                      make_missing_token(T_RIGHT_PAREN), // RightBrace
                                                   },
                                                   SourcePresence::Present, arena);
   return make<MemberDeclBlockSyntax>(target);
}

ClassDefinitionSyntax DeclSyntaxNodeFactory::makeBlankClassDefinition(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::ClassDefinition, {
                                                      nullptr, // Modififers
                                                      make_missing_token(T_CLASS), // ClassToken
                                                      make_missing_token(T_IDENTIFIER_STRING), // Name
                                                      nullptr, // ExtendsFrom
                                                      nullptr, // ImplementsList
                                                      RawSyntax::missing(SyntaxKind::MemberDeclBlock) // Members
                                                   },
                                                   SourcePresence::Present, arena);
   return make<ClassDefinitionSyntax>(target);
}

TraitDefinitionSyntax DeclSyntaxNodeFactory::makeBlankTraitDefinition(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::TraitDefinition, {
                                                      nullptr, // Modififers
                                                      make_missing_token(T_TRAIT), // TraitToken
                                                      make_missing_token(T_IDENTIFIER_STRING), // Name
                                                      RawSyntax::missing(SyntaxKind::MemberDeclBlock) // Members
                                                   },
                                                   SourcePresence::Present, arena);
   return make<TraitDefinitionSyntax>(target);
}

SourceFileSyntax DeclSyntaxNodeFactory::makeBlankSourceFile(RefCountPtr<SyntaxArena> arena)
{
   RefCountPtr<RawSyntax> target = RawSyntax::make(SyntaxKind::SourceFile, {
                                                      RawSyntax::missing(SyntaxKind::TopStmtList), // Statements
                                                      make_missing_token(END), // EOFToken
                                                   },
                                                   SourcePresence::Present, arena);
   return make<SourceFileSyntax>(target);
}
} // polar::syntax
