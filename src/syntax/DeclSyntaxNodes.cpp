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

#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::syntax {

///
/// ReservedNonModifierSyntax
///
void ReservedNonModifierSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ReservedNonModifierSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Modifier, CHILD_TOKEN_CHOICES.at(Cursor::Modifier));
#endif
}

#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType ReservedNonModifierSyntax::CHILD_TOKEN_CHOICES
{
   {
      ReservedNonModifierSyntax::Modifier, {
         TokenKindType::T_INCLUDE, TokenKindType::T_INCLUDE_ONCE, TokenKindType::T_EVAL,
               TokenKindType::T_REQUIRE, TokenKindType::T_REQUIRE_ONCE, TokenKindType::T_LOGICAL_OR,
               TokenKindType::T_LOGICAL_XOR, TokenKindType::T_LOGICAL_AND, TokenKindType::T_INSTANCEOF,
               TokenKindType::T_NEW, TokenKindType::T_CLONE, TokenKindType::T_EXIT,
               TokenKindType::T_IF, TokenKindType::T_ELSEIF, TokenKindType::T_ELSE, TokenKindType::T_ECHO,
               TokenKindType::T_DO, TokenKindType::T_WHILE, TokenKindType::T_FOR, TokenKindType::T_FOREACH,
               TokenKindType::T_DECLARE, TokenKindType::T_AS, TokenKindType::T_TRY, TokenKindType::T_CATCH,
               TokenKindType::T_FINALLY, TokenKindType::T_THROW, TokenKindType::T_USE, TokenKindType::T_INSTEADOF,
               TokenKindType::T_GLOBAL, TokenKindType::T_VAR, TokenKindType::T_UNSET, TokenKindType::T_ISSET,
               TokenKindType::T_EMPTY, TokenKindType::T_CONTINUE, TokenKindType::T_GOTO, TokenKindType::T_FUNCTION,
               TokenKindType::T_CONST, TokenKindType::T_RETURN, TokenKindType::T_PRINT, TokenKindType::T_YIELD,
               TokenKindType::T_LIST, TokenKindType::T_SWITCH, TokenKindType::T_CASE, TokenKindType::T_DEFAULT,
               TokenKindType::T_BREAK, TokenKindType::T_ARRAY, TokenKindType::T_CALLABLE, TokenKindType::T_EXTENDS,
               TokenKindType::T_IMPLEMENTS, TokenKindType::T_NAMESPACE, TokenKindType::T_TRAIT, TokenKindType::T_INTERFACE,
               TokenKindType::T_CLASS, TokenKindType::T_CLASS_CONST, TokenKindType::T_TRAIT_CONST, TokenKindType::T_FUNC_CONST,
               TokenKindType::T_METHOD_CONST, TokenKindType::T_LINE, TokenKindType::T_FILE, TokenKindType::T_DIR, TokenKindType::T_NS_CONST,
               TokenKindType::T_FN
      }
   }
};
#endif

TokenSyntax ReservedNonModifierSyntax::getModifier()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Modifier).get()};
}

ReservedNonModifierSyntax ReservedNonModifierSyntax::withModifier(std::optional<TokenSyntax> modifier)
{
   RefCountPtr<RawSyntax> modifierRaw;
   if (modifier.has_value()) {
      modifierRaw = modifier->getRaw();
   } else {
      /// not very good
      modifierRaw = make_missing_token(T_INCLUDE);
   }
   return m_data->replaceChild<ReservedNonModifierSyntax>(modifierRaw, Cursor::Modifier);
}

///
/// SemiReservedSytnax
///
#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType SemiReservedSytnax::CHILD_TOKEN_CHOICES
{
   {
      SemiReservedSytnax::Cursor::ModifierChoiceToken, {
         TokenKindType::T_STATIC, TokenKindType::T_ABSTRACT, TokenKindType::T_FINAL,
               TokenKindType::T_PRIVATE, TokenKindType::T_PROTECTED, TokenKindType::T_PUBLIC
      }
   }
};
#endif

void SemiReservedSytnax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SemiReservedSytnax::CHILDREN_COUNT);
   // check node choices
   if (const RefCountPtr<RawSyntax> &modifierChild = raw->getChild(Cursor::Modifier)) {
      if (modifierChild->isToken()) {
         syntax_assert_child_token(raw, ModifierChoiceToken,
                                   CHILD_TOKEN_CHOICES.at(SemiReservedSytnax::ModifierChoiceToken));
         return;
      }
      assert(modifierChild->kindOf(SyntaxKind::ReservedNonModifier));
   }
#endif
}

Syntax SemiReservedSytnax::getModifier()
{
   return Syntax{m_root, m_data->getChild(Cursor::Modifier).get()};
}

SemiReservedSytnax SemiReservedSytnax::withModifier(std::optional<Syntax> modifier)
{
   RefCountPtr<RawSyntax> modifierRaw;
   if (modifier.has_value()) {
      modifierRaw = modifier->getRaw();
   } else {
      modifierRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<SemiReservedSytnax>(modifierRaw, Cursor::Modifier);
}

///
/// IdentifierSyntax
///
void IdentifierSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == IdentifierSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &itemChild = raw->getChild(Cursor::NameItem)) {
      if (itemChild->isToken()) {
         syntax_assert_child_token(raw, NameItem, std::set{TokenKindType::T_IDENTIFIER_STRING});
         return;
      }
      assert(itemChild->kindOf(SyntaxKind::SemiReserved));
   }
#endif
}

Syntax IdentifierSyntax::getNameItem()
{
   return Syntax{m_root, m_data->getChild(Cursor::NameItem).get()};
}

IdentifierSyntax IdentifierSyntax::withNameItem(std::optional<Syntax> item)
{
   RefCountPtr<RawSyntax> nameItemRaw;
   if (item.has_value()) {
      nameItemRaw = item->getRaw();
   } else {
      nameItemRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<IdentifierSyntax>(nameItemRaw, Cursor::NameItem);
}

///
/// NamespaceNameSyntax
///
void NamespaceNameSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == NamespaceNameSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, ParentNs, std::set{SyntaxKind::NamespaceName});
   syntax_assert_child_token(raw, NsSeparator, std::set{TokenKindType::T_NS_SEPARATOR});
   syntax_assert_child_token(raw, Name, std::set{TokenKindType::T_IDENTIFIER_STRING});
#endif
}

std::optional<NamespaceNameSyntax> NamespaceNameSyntax::getParentNs()
{
   RefCountPtr<SyntaxData> parentNsData = m_data->getChild(Cursor::ParentNs);
   if (!parentNsData) {
      return std::nullopt;
   }
   return NamespaceNameSyntax{m_root, parentNsData.get()};
}

std::optional<TokenSyntax> NamespaceNameSyntax::getNsSeparator()
{
   RefCountPtr<SyntaxData> separatorData = m_data->getChild(Cursor::NsSeparator);
   if (!separatorData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, separatorData.get()};
}

TokenSyntax NamespaceNameSyntax::getName()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Name).get()};
}

NamespaceNameSyntax NamespaceNameSyntax::withParentNs(std::optional<NamespaceNameSyntax> parentNs)
{
   RefCountPtr<RawSyntax> parentNsRaw;
   if (parentNs.has_value()) {
      parentNsRaw = parentNs->getRaw();
   } else {
      parentNsRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceNameSyntax>(parentNsRaw, Cursor::ParentNs);
}

NamespaceNameSyntax NamespaceNameSyntax::withNsSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = nullptr;
   }
   return m_data->replaceChild<NamespaceNameSyntax>(separatorRaw, Cursor::Name);
}

NamespaceNameSyntax NamespaceNameSyntax::withName(std::optional<TokenSyntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = make_missing_token(T_NS_SEPARATOR);
   }
   return m_data->replaceChild<NamespaceNameSyntax>(nameRaw, Cursor::NsSeparator);
}

///
/// NameSyntax
///
void NameSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NameSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, NsToken, std::set{TokenKindType::T_NAMESPACE});
   syntax_assert_child_token(raw, NsSeparator, std::set{TokenKindType::T_NS_SEPARATOR});
   if (const RefCountPtr<RawSyntax> &nsChild = raw->getChild(Cursor::Namespace)) {
      assert(nsChild->kindOf(SyntaxKind::NamespaceName));
   }
#endif
}

std::optional<TokenSyntax> NameSyntax::getNsToken()
{
   RefCountPtr<SyntaxData> nsTokenData = m_data->getChild(Cursor::NsToken);
   if (!nsTokenData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, nsTokenData.get()};
}

std::optional<TokenSyntax> NameSyntax::getNsSeparator()
{
   RefCountPtr<SyntaxData> separatorData = m_data->getChild(Cursor::NsSeparator);
   if (!separatorData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, separatorData.get()};
}

NamespaceNameSyntax NameSyntax::getNamespaceName()
{
   return NamespaceNameSyntax{m_root, m_data->getChild(Cursor::Namespace).get()};
}

NameSyntax NameSyntax::withNsToken(std::optional<TokenSyntax> nsToken)
{
   RefCountPtr<RawSyntax> nsTokenRaw;
   if (nsToken.has_value()) {
      nsTokenRaw = nsToken->getRaw();
   } else {
      nsTokenRaw = nullptr;
   }
   return m_data->replaceChild<NameSyntax>(nsTokenRaw, Cursor::NsToken);
}

NameSyntax NameSyntax::withNsSeparator(std::optional<TokenSyntax> separatorToken)
{
   RefCountPtr<RawSyntax> separatorTokenRaw;
   if (separatorToken.has_value()) {
      separatorTokenRaw = separatorToken->getRaw();
   } else {
      separatorTokenRaw = nullptr;
   }
   return m_data->replaceChild<NameSyntax>(separatorTokenRaw, Cursor::NsSeparator);
}

NameSyntax NameSyntax::withNamespaceName(std::optional<NamespaceNameSyntax> ns)
{
   RefCountPtr<RawSyntax> nsRaw;
   if (ns.has_value()) {
      nsRaw = ns->getRaw();
   } else {
      nsRaw = RawSyntax::missing(SyntaxKind::NamespaceName);
   }
   return m_data->replaceChild<NameSyntax>(nsRaw, Cursor::Namespace);
}

///
/// NameListItemSyntax
///
void NameListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NameSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CommaToken, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, Name, std::set{SyntaxKind::Name});
#endif
}

std::optional<TokenSyntax> NameListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::CommaToken);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

NameSyntax NameListItemSyntax::getName()
{
   return NameSyntax {m_root, m_data->getChild(Cursor::Name).get()};
}

NameListItemSyntax NameListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<NameListItemSyntax>(commaRaw, Cursor::CommaToken);
}

NameListItemSyntax NameListItemSyntax::withName(std::optional<NameSyntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = RawSyntax::missing(SyntaxKind::NameListItem);
   }
   return m_data->replaceChild<NameListItemSyntax>(nameRaw, Cursor::Name);
}

///
/// InitializerClauseSyntax
///
void InitializerClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == InitializerClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, EqualToken, std::set{TokenKindType::T_EQUAL});
   if (const RefCountPtr<RawSyntax> &valueExpr = raw->getChild(Cursor::ValueExpr)) {
      valueExpr->kindOf(SyntaxKind::Expr);
   }
#endif
}

TokenSyntax InitializerClauseSyntax::getEqualToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::EqualToken).get()};
}

ExprSyntax InitializerClauseSyntax::getValueExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::ValueExpr).get()};
}

InitializerClauseSyntax InitializerClauseSyntax::withEqualToken(std::optional<TokenSyntax> equalToken)
{
   RefCountPtr<RawSyntax> equalTokenRaw;
   if (equalToken.has_value()) {
      equalTokenRaw = equalToken->getRaw();
   } else {
      equalTokenRaw = make_missing_token(T_EQUAL);
   }
   return m_data->replaceChild<InitializerClauseSyntax>(equalTokenRaw, Cursor::EqualToken);
}

InitializerClauseSyntax InitializerClauseSyntax::withValueExpr(std::optional<ExprSyntax> valueExpr)
{
   RefCountPtr<RawSyntax> valueExprRaw;
   if (valueExpr.has_value()) {
      valueExprRaw = valueExpr->getRaw();
   } else {
      valueExprRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<InitializerClauseSyntax>(valueExprRaw, Cursor::ValueExpr);
}

///
/// TypeClauseSyntax
///

#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType TypeClauseSyntax::CHILD_TOKEN_CHOICES
{
   {
      TypeClauseSyntax::Type, {
         TokenKindType::T_ARRAY,
               TokenKindType::T_CALLABLE
      }
   }
};
#endif

void TypeClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == TypeClauseSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &typeChild = raw->getChild(Cursor::Type)) {
      if (typeChild->isToken()) {
         syntax_assert_child_token(raw, Type, CHILD_TOKEN_CHOICES.at(Cursor::Type));
         return;
      }
      assert(typeChild->kindOf(SyntaxKind::Name));
   }
#endif
}

Syntax TypeClauseSyntax::getType()
{
   return Syntax {m_root, m_data->getChild(Cursor::Type).get()};
}

TypeClauseSyntax TypeClauseSyntax::withType(std::optional<Syntax> type)
{
   RefCountPtr<RawSyntax> typeRaw;
   if (type.has_value()) {
      typeRaw = type->getRaw();
   } else {
      typeRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<TypeClauseSyntax>(typeRaw, Cursor::Type);
}

///
/// TypeExprClauseSyntax
///
void TypeExprClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == TypeExprClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, QuestionToken, std::set{TokenKindType::T_QUESTION_MARK});
   if (const RefCountPtr<RawSyntax> &typeChild = raw->getChild(Cursor::TypeClause)) {
      assert(typeChild->kindOf(SyntaxKind::TypeClause));
   }
#endif
}

std::optional<TokenSyntax> TypeExprClauseSyntax::getQuestionToken()
{
   RefCountPtr<SyntaxData> questionTokenData = m_data->getChild(Cursor::QuestionToken);
   if (!questionTokenData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, questionTokenData.get()};
}

TypeClauseSyntax TypeExprClauseSyntax::getTypeClause()
{
   return TypeClauseSyntax {m_root, m_data->getChild(Cursor::TypeClause).get()};
}

TypeExprClauseSyntax TypeExprClauseSyntax::withQuestionToken(std::optional<TokenSyntax> questionToken)
{
   RefCountPtr<RawSyntax> questionTokenRaw;
   if (questionToken.has_value()) {
      questionTokenRaw = questionToken->getRaw();
   } else {
      questionTokenRaw = nullptr;
   }
   return m_data->replaceChild<TypeExprClauseSyntax>(questionTokenRaw, Cursor::QuestionToken);
}

TypeExprClauseSyntax TypeExprClauseSyntax::withType(std::optional<TypeClauseSyntax> type)
{
   RefCountPtr<RawSyntax> typeRaw;
   if (type.has_value()) {
      typeRaw = type->getRaw();
   } else {
      typeRaw = nullptr;
   }
   return m_data->replaceChild<TypeExprClauseSyntax>(typeRaw, Cursor::TypeClause);
}

///
/// ReturnTypeClauseSyntax
///
void ReturnTypeClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ReturnTypeClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ColonToken, std::set{TokenKindType::T_COLON});
   if (const RefCountPtr<RawSyntax> &typeExprChild = raw->getChild(Cursor::TypeExpr)) {
      assert(typeExprChild->kindOf(SyntaxKind::TypeExprClause));
   }
#endif
}

TokenSyntax ReturnTypeClauseSyntax::getColon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ColonToken).get()};
}

TypeExprClauseSyntax ReturnTypeClauseSyntax::getType()
{
   return TypeExprClauseSyntax {m_root, m_data->getChild(Cursor::TypeExpr).get()};
}

ReturnTypeClauseSyntax ReturnTypeClauseSyntax::withColon(std::optional<TokenSyntax> colon)
{
   RefCountPtr<RawSyntax> colonRaw;
   if (colon.has_value()) {
      colonRaw = colon->getRaw();
   } else {
      colonRaw = make_missing_token(T_COLON);
   }
   return m_data->replaceChild<ReturnTypeClauseSyntax>(colonRaw, Cursor::ColonToken);
}

ReturnTypeClauseSyntax ReturnTypeClauseSyntax::withType(std::optional<TypeExprClauseSyntax> type)
{
   RefCountPtr<RawSyntax> typeRaw;
   if (type.has_value()) {
      typeRaw = type->getRaw();
   } else {
      typeRaw = RawSyntax::missing(SyntaxKind::TypeExprClause);
   }
   return m_data->replaceChild<ReturnTypeClauseSyntax>(typeRaw, Cursor::TypeExpr);
}

///
/// ParameterSyntax
///
void ParameterSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ParameterSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ReferenceMark, std::set{TokenKindType::T_AMPERSAND});
   syntax_assert_child_token(raw, VariadicMark, std::set{TokenKindType::T_ELLIPSIS});
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
   if (const RefCountPtr<RawSyntax> &typeHintChild = raw->getChild(Cursor::TypeHint)) {
      assert(typeHintChild->kindOf(SyntaxKind::TypeExprClause));
   }
   if (const RefCountPtr<RawSyntax> &initializerChild = raw->getChild(Cursor::Initializer)) {
      assert(initializerChild->kindOf(SyntaxKind::InitializerClause));
   }
#endif
}

std::optional<TypeExprClauseSyntax> ParameterSyntax::getTypeHint()
{
   RefCountPtr<SyntaxData> typeHintData = m_data->getChild(Cursor::TypeHint);
   if (!typeHintData) {
      return std::nullopt;
   }
   return TypeExprClauseSyntax {m_root, typeHintData.get()};
}

std::optional<TokenSyntax> ParameterSyntax::getReferenceMark()
{
   RefCountPtr<SyntaxData> referenceData = m_data->getChild(Cursor::ReferenceMark);
   if (!referenceData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, referenceData.get()};
}

std::optional<TokenSyntax> ParameterSyntax::getVariadicMark()
{
   RefCountPtr<SyntaxData> variadicData = m_data->getChild(Cursor::VariadicMark);
   if (!variadicData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, variadicData.get()};
}

TokenSyntax ParameterSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

std::optional<InitializerClauseSyntax> ParameterSyntax::getInitializer()
{
   RefCountPtr<SyntaxData> initializerData = m_data->getChild(Cursor::Initializer);
   if (!initializerData) {
      return std::nullopt;
   }
   return InitializerClauseSyntax {m_root, initializerData.get()};
}

ParameterSyntax ParameterSyntax::withTypeHint(std::optional<TypeExprClauseSyntax> typeHint)
{
   RefCountPtr<RawSyntax> typeHintRaw;
   if (typeHint.has_value()) {
      typeHintRaw = typeHint->getRaw();
   } else {
      typeHintRaw = nullptr;
   }
   return m_data->replaceChild<ParameterSyntax>(typeHintRaw, Cursor::TypeHint);
}

ParameterSyntax ParameterSyntax::withReferenceMark(std::optional<TokenSyntax> referenceMark)
{
   RefCountPtr<RawSyntax> referenceMarkRaw;
   if (referenceMark.has_value()) {
      referenceMarkRaw = referenceMark->getRaw();
   } else {
      referenceMarkRaw = nullptr;
   }
   return m_data->replaceChild<ParameterSyntax>(referenceMarkRaw, Cursor::ReferenceMark);
}

ParameterSyntax ParameterSyntax::withVariadicMark(std::optional<TokenSyntax> variadicMark)
{
   RefCountPtr<RawSyntax> variadicMarkRaw;
   if (variadicMark.has_value()) {
      variadicMarkRaw = variadicMark->getRaw();
   } else {
      variadicMarkRaw = nullptr;
   }
   return m_data->replaceChild<ParameterSyntax>(variadicMarkRaw, Cursor::VariadicMark);
}

ParameterSyntax ParameterSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> variableRaw;
   if (variable.has_value()) {
      variableRaw = variable->getRaw();
   } else {
      variableRaw = make_missing_token(T_VARIABLE);
   }
   return m_data->replaceChild<ParameterSyntax>(variableRaw, Cursor::Variable);
}

ParameterSyntax ParameterSyntax::withInitializer(std::optional<InitializerClauseSyntax> initializer)
{
   RefCountPtr<RawSyntax> initializerRaw;
   if (initializer.has_value()) {
      initializerRaw = initializer->getRaw();
   } else {
      initializerRaw = RawSyntax::missing(SyntaxKind::InitializerClause);
   }
   return m_data->replaceChild<ParameterSyntax>(initializerRaw, Cursor::Initializer);
}

///
/// ParameterListItemSyntax
///
void ParameterListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ParameterListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Comma, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, Parameter, std::set{SyntaxKind::Parameter});
#endif
}

std::optional<TokenSyntax> ParameterListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::Comma);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

ParameterSyntax ParameterListItemSyntax::getParameter()
{
   return ParameterSyntax {m_root, m_data->getChild(Cursor::Parameter).get()};
}

ParameterListItemSyntax ParameterListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<ParameterListItemSyntax>(commaRaw, Cursor::Comma);
}

ParameterListItemSyntax ParameterListItemSyntax::withParameter(std::optional<TokenSyntax> parameter)
{
   RefCountPtr<RawSyntax> parameterRaw;
   if (parameter.has_value()) {
      parameterRaw = parameter->getRaw();
   } else {
      parameterRaw = RawSyntax::missing(SyntaxKind::Parameter);
   }
   return m_data->replaceChild<ParameterListItemSyntax>(parameterRaw, Cursor::Parameter);
}

///
/// ParameterClauseSyntax
///
void ParameterClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ParameterClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
   if (const RefCountPtr<RawSyntax> &parametersChild = raw->getChild(Cursor::Parameters)) {
      assert(parametersChild->kindOf(SyntaxKind::ParameterList));
   }
#endif
}

TokenSyntax ParameterClauseSyntax::getLeftParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParen).get()};
}

std::optional<ParameterListSyntax> ParameterClauseSyntax::getParameters()
{
   RefCountPtr<SyntaxData> paramsData = m_data->getChild(Cursor::Parameters);
   if (!paramsData) {
      return std::nullopt;
   }
   return ParameterListSyntax {m_root, paramsData.get()};
}

TokenSyntax ParameterClauseSyntax::getRightParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParen).get()};
}

ParameterClauseSyntax ParameterClauseSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> leftParenRaw;
   if (leftParen.has_value()) {
      leftParenRaw = leftParen->getRaw();
   } else {
      leftParenRaw = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<ParameterClauseSyntax>(leftParenRaw, Cursor::LeftParen);
}

ParameterClauseSyntax ParameterClauseSyntax::withParameters(std::optional<ParameterListSyntax> parameters)
{
   RefCountPtr<RawSyntax> parametersRaw;
   if (parameters.has_value()) {
      parametersRaw = parameters->getRaw();
   } else {
      parametersRaw = nullptr;
   }
   return m_data->replaceChild<ParameterClauseSyntax>(parametersRaw, Cursor::Parameters);
}

ParameterClauseSyntax ParameterClauseSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rightParenRaw;
   if (rightParen.has_value()) {
      rightParenRaw = rightParen->getRaw();
   } else {
      rightParenRaw = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<ParameterClauseSyntax>(rightParenRaw, Cursor::RightParen);
}

///
/// FunctionDefinitionSyntax
///
void FunctionDefinitionSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == FunctionDefinitionSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, FuncToken, std::set{TokenKindType::T_FUNCTION});
   syntax_assert_child_token(raw, ReturnRefToken, std::set{TokenKindType::T_AMPERSAND});
   syntax_assert_child_token(raw, FuncName, std::set{TokenKindType::T_IDENTIFIER_STRING});
   if (const RefCountPtr<RawSyntax> &parameterClause = raw->getChild(Cursor::ParameterListClause)) {
      assert(parameterClause->kindOf(SyntaxKind::ParameterListClause));
   }
   if (const RefCountPtr<RawSyntax> &returnTypeClause = raw->getChild(Cursor::ReturnType)) {
      assert(returnTypeClause->kindOf(SyntaxKind::ReturnTypeClause));
   }
   if (const RefCountPtr<RawSyntax> &body = raw->getChild(Cursor::Body)) {
      assert(body->kindOf(SyntaxKind::CodeBlock));
   }
#endif
}

TokenSyntax FunctionDefinitionSyntax::getFuncToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::FuncToken).get()};
}

std::optional<TokenSyntax> FunctionDefinitionSyntax::getReturnRefToken()
{
   RefCountPtr<SyntaxData> returnRefFlagTokenData = m_data->getChild(Cursor::ReturnRefToken);
   if (!returnRefFlagTokenData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, returnRefFlagTokenData.get()};
}

TokenSyntax FunctionDefinitionSyntax::getFuncName()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::FuncName).get()};
}

ParameterClauseSyntax FunctionDefinitionSyntax::getParameterClause()
{
   return ParameterClauseSyntax {m_root, m_data->getChild(Cursor::ParameterListClause).get()};
}

std::optional<TokenSyntax> FunctionDefinitionSyntax::getReturnType()
{
   RefCountPtr<SyntaxData> returnTypeData = m_data->getChild(Cursor::ReturnType);
   if (!returnTypeData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, returnTypeData.get()};
}

InnerCodeBlockStmtSyntax FunctionDefinitionSyntax::getBody()
{
   return InnerCodeBlockStmtSyntax {m_root, m_data->getChild(Cursor::Body).get()};
}

FunctionDefinitionSyntax
FunctionDefinitionSyntax::withFuncToken(std::optional<TokenSyntax> funcToken)
{
   RefCountPtr<RawSyntax> funcTokenRaw;
   if (funcToken.has_value()) {
      funcTokenRaw = funcToken->getRaw();
   } else {
      funcTokenRaw = make_missing_token(T_FUNCTION);
   }
   return m_data->replaceChild<FunctionDefinitionSyntax>(funcTokenRaw, Cursor::FuncToken);
}

FunctionDefinitionSyntax
FunctionDefinitionSyntax::withReturnRefToken(std::optional<TokenSyntax> returnRefFlagToken)
{
   RefCountPtr<RawSyntax> returnRefFlagTokenRaw;
   if (returnRefFlagToken.has_value()) {
      returnRefFlagTokenRaw = returnRefFlagToken->getRaw();
   } else {
      returnRefFlagTokenRaw = nullptr;
   }
   return m_data->replaceChild<FunctionDefinitionSyntax>(returnRefFlagTokenRaw, Cursor::ReturnRefToken);
}

FunctionDefinitionSyntax
FunctionDefinitionSyntax::withFuncName(std::optional<TokenSyntax> funcName)
{
   RefCountPtr<RawSyntax> funcNameRaw;
   if (funcName.has_value()) {
      funcNameRaw = funcName->getRaw();
   } else {
      funcNameRaw = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<FunctionDefinitionSyntax>(funcNameRaw, Cursor::FuncName);
}

FunctionDefinitionSyntax
FunctionDefinitionSyntax::withParameterClause(std::optional<ParameterClauseSyntax> parameterClause)
{
   RefCountPtr<RawSyntax> parameterClauseRaw;
   if (parameterClause.has_value()) {
      parameterClauseRaw = parameterClause->getRaw();
   } else {
      parameterClauseRaw = RawSyntax::missing(SyntaxKind::ParameterListClause);
   }
   return m_data->replaceChild<FunctionDefinitionSyntax>(parameterClauseRaw, Cursor::ParameterListClause);
}

FunctionDefinitionSyntax
FunctionDefinitionSyntax::withReturnType(std::optional<TokenSyntax> returnType)
{
   RefCountPtr<RawSyntax> returnTypeRaw;
   if (returnType.has_value()) {
      returnTypeRaw = returnType->getRaw();
   } else {
      returnTypeRaw = nullptr;
   }
   return m_data->replaceChild<FunctionDefinitionSyntax>(returnTypeRaw, Cursor::ReturnType);
}

FunctionDefinitionSyntax
FunctionDefinitionSyntax::withBody(std::optional<InnerCodeBlockStmtSyntax> body)
{
   RefCountPtr<RawSyntax> bodyRaw;
   if (body.has_value()) {
      bodyRaw = body->getRaw();
   } else {
      bodyRaw = RawSyntax::missing(SyntaxKind::InnerCodeBlockStmt);
   }
   return m_data->replaceChild<FunctionDefinitionSyntax>(bodyRaw, Cursor::Body);
}

///
/// ClassModifierSyntax
///
#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType ClassModifierSyntax::CHILD_TOKEN_CHOICES
{
   {
      ClassModifierSyntax::Modifier, {
         TokenKindType::T_ABSTRACT,
               TokenKindType::T_FINAL,
      }
   }
};
#endif

void ClassModifierSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassModifierSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Modifier, CHILD_TOKEN_CHOICES.at(Cursor::Modifier));
#endif
}

TokenSyntax ClassModifierSyntax::getModifier()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Modifier).get()};
}

ClassModifierSyntax ClassModifierSyntax::withModifier(std::optional<TokenSyntax> modifier)
{
   RefCountPtr<RawSyntax> modifierRaw;
   if (modifier.has_value()) {
      modifierRaw = modifier->getRaw();
   } else {
      modifierRaw = make_missing_token(T_ABSTRACT);
   }
   return m_data->replaceChild<ClassModifierSyntax>(modifierRaw, Cursor::Modifier);
}

///
/// ExtendsFromClauseSyntax
///
void ExtendsFromClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ExtendsFromClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ExtendToken, std::set{TokenKindType::T_EXTENDS});
   if (const RefCountPtr<RawSyntax> &nameChild = raw->getChild(Cursor::Name)) {
      assert(nameChild->kindOf(SyntaxKind::Name));
   }
#endif
}

TokenSyntax ExtendsFromClauseSyntax::getExtendToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ExtendToken).get()};
}

NameSyntax ExtendsFromClauseSyntax::getName()
{
   return NameSyntax {m_root, m_data->getChild(Cursor::Name).get()};
}

ExtendsFromClauseSyntax ExtendsFromClauseSyntax::withExtendToken(std::optional<TokenSyntax> extendToken)
{
   RefCountPtr<RawSyntax> extendTokenRaw;
   if (extendToken.has_value()) {
      extendTokenRaw = extendToken->getRaw();
   } else {
      extendTokenRaw = make_missing_token(T_EXTENDS);
   }
   return m_data->replaceChild<ExtendsFromClauseSyntax>(extendTokenRaw, Cursor::ExtendToken);
}

ExtendsFromClauseSyntax ExtendsFromClauseSyntax::withName(std::optional<NameSyntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = RawSyntax::missing(SyntaxKind::Name);
   }
   return m_data->replaceChild<ExtendsFromClauseSyntax>(nameRaw, Cursor::Name);
}

///
/// ImplementsClauseSyntax
///
void ImplementsClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ImplementsClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ImplementToken, std::set{TokenKindType::T_IMPLEMENTS});
   if (const RefCountPtr<RawSyntax> &interfaceChild = raw->getChild(Cursor::Interfaces)) {
      assert(interfaceChild->kindOf(SyntaxKind::ImplementsClause));
   }
#endif
}

TokenSyntax ImplementsClauseSyntax::getImplementToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ImplementToken).get()};
}

NameListSyntax ImplementsClauseSyntax::getInterfaces()
{
   return NameListSyntax {m_root, m_data->getChild(Cursor::Interfaces).get()};
}

ImplementsClauseSyntax ImplementsClauseSyntax::withImplementToken(std::optional<TokenSyntax> implementToken)
{
   RefCountPtr<RawSyntax> implementTokenRaw;
   if (implementToken.has_value()) {
      implementTokenRaw = implementToken->getRaw();
   } else {
      implementTokenRaw = make_missing_token(T_IMPLEMENTS);
   }
   return m_data->replaceChild<ImplementsClauseSyntax>(implementTokenRaw, Cursor::ImplementToken);
}

ImplementsClauseSyntax ImplementsClauseSyntax::withInterfaces(std::optional<NameListSyntax> interfaces)
{
   RefCountPtr<RawSyntax> interfacesRaw;
   if (interfaces.has_value()) {
      interfacesRaw = interfaces->getRaw();
   } else {
      interfacesRaw = RawSyntax::missing(SyntaxKind::NameList);
   }
   return m_data->replaceChild<ImplementsClauseSyntax>(interfacesRaw, Cursor::Interfaces);
}

///
/// MemberModifierSyntax
///
#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType MemberModifierSyntax::CHILD_TOKEN_CHOICES
{
   {
      MemberModifierSyntax::Modifier, {
         TokenKindType::T_PUBLIC, TokenKindType::T_PROTECTED,
               TokenKindType::T_PRIVATE, TokenKindType::T_STATIC,
               TokenKindType::T_ABSTRACT, TokenKindType::T_FINAL,
      }
   }
};
#endif

void MemberModifierSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == MemberModifierSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Modifier, CHILD_TOKEN_CHOICES.at(Cursor::Modifier));
#endif
}

TokenSyntax MemberModifierSyntax::getModifier()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Modifier).get()};
}

MemberModifierSyntax MemberModifierSyntax::withModifier(std::optional<TokenSyntax> modifier)
{
   RefCountPtr<RawSyntax> modifierRaw;
   if (modifier.has_value()) {
      modifierRaw = modifier->getRaw();
   } else {
      modifierRaw = make_missing_token(T_PUBLIC);
   }
   return m_data->replaceChild<MemberModifierSyntax>(modifierRaw, Cursor::Modifier);
}

///
/// ClassPropertyDeclSyntax
///
void ClassPropertyDeclSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassPropertyDeclSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &modifiersChild = raw->getChild(Cursor::Modifiers)) {
      assert(modifiersChild->kindOf(SyntaxKind::ClassModifierList));
   }
   if (const RefCountPtr<RawSyntax> &typeHintChild = raw->getChild(Cursor::TypeHint)) {
      assert(typeHintChild->kindOf(SyntaxKind::TypeExprClause));
   }
   if (const RefCountPtr<RawSyntax> &propertyListChild = raw->getChild(Cursor::PropertyList)) {
      assert(propertyListChild->kindOf(SyntaxKind::ClassPropertyList));
   }
#endif
}

MemberModifierListSyntax ClassPropertyDeclSyntax::getModifiers()
{
   return MemberModifierListSyntax {m_root, m_data->getChild(Cursor::Modifiers).get()};
}

std::optional<TypeExprClauseSyntax> ClassPropertyDeclSyntax::getTypeHint()
{
   RefCountPtr<SyntaxData> typeHintData = m_data->getChild(Cursor::TypeHint);
   if (!typeHintData) {
      return std::nullopt;
   }
   return TypeExprClauseSyntax {m_root, typeHintData.get()};
}

ClassPropertyListSyntax ClassPropertyDeclSyntax::getPropertyList()
{
   return ClassPropertyListSyntax {m_root, m_data->getChild(Cursor::PropertyList).get()};
}

ClassPropertyDeclSyntax ClassPropertyDeclSyntax::withModifiers(std::optional<MemberModifierListSyntax> modifiers)
{
   RefCountPtr<RawSyntax> modifiersRaw;
   if (modifiers.has_value()) {
      modifiersRaw = modifiers->getRaw();
   } else {
      modifiersRaw = RawSyntax::missing(SyntaxKind::ClassModifierList);
   }
   return m_data->replaceChild<ClassPropertyDeclSyntax>(modifiersRaw, Cursor::Modifiers);
}

ClassPropertyDeclSyntax ClassPropertyDeclSyntax::withTypeHint(std::optional<TypeExprClauseSyntax> typeHint)
{
   RefCountPtr<RawSyntax> typeHintRaw;
   if (typeHint.has_value()) {
      typeHintRaw = typeHint->getRaw();
   } else {
      typeHintRaw = nullptr;
   }
   return m_data->replaceChild<ClassPropertyDeclSyntax>(typeHintRaw, Cursor::TypeHint);
}

ClassPropertyDeclSyntax ClassPropertyDeclSyntax::withPropertyList(std::optional<ClassPropertyListSyntax> propertyList)
{
   RefCountPtr<RawSyntax> propertyListRaw;
   if (propertyList.has_value()) {
      propertyListRaw = propertyList->getRaw();
   } else {
      propertyListRaw = RawSyntax::missing(SyntaxKind::ClassPropertyList);
   }
   return m_data->replaceChild<ClassPropertyDeclSyntax>(propertyListRaw, Cursor::PropertyList);
}

///
/// ClassConstDeclSyntax
///
void ClassConstDeclSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassConstDeclSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ConstToken, std::set{TokenKindType::T_CONST});
   if (const RefCountPtr<RawSyntax> &modifiersChild = raw->getChild(Cursor::Modifiers)) {
      assert(modifiersChild->kindOf(SyntaxKind::ClassModifierList));
   }
   if (const RefCountPtr<RawSyntax> &constListChild = raw->getChild(Cursor::ConstList)) {
      assert(constListChild->kindOf(SyntaxKind::ClassConstList));
   }
#endif
}

MemberModifierListSyntax ClassConstDeclSyntax::getModifiers()
{
   return MemberModifierListSyntax {m_root, m_data->getChild(Cursor::Modifiers).get()};
}

std::optional<TokenSyntax> ClassConstDeclSyntax::getConstToken()
{
   RefCountPtr<SyntaxData> constTokenData = m_data->getChild(Cursor::ConstToken);
   if (!constTokenData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, constTokenData.get()};
}

ClassPropertyListSyntax ClassConstDeclSyntax::getConstList()
{
   return ClassPropertyListSyntax {m_root, m_data->getChild(Cursor::ConstList).get()};
}

ClassConstDeclSyntax ClassConstDeclSyntax::withModifiers(std::optional<MemberModifierListSyntax> modifiers)
{
   RefCountPtr<RawSyntax> modifiersRaw;
   if (modifiers.has_value()) {
      modifiersRaw = modifiers->getRaw();
   } else {
      modifiersRaw = RawSyntax::missing(SyntaxKind::ClassModifierList);
   }
   return m_data->replaceChild<ClassConstDeclSyntax>(modifiersRaw, Cursor::Modifiers);
}

ClassConstDeclSyntax ClassConstDeclSyntax::withConstToken(std::optional<TokenSyntax> constToken)
{
   RefCountPtr<RawSyntax> constTokenRaw;
   if (constToken.has_value()) {
      constTokenRaw = constToken->getRaw();
   } else {
      constTokenRaw = make_missing_token(T_CONST);
   }
   return m_data->replaceChild<ClassConstDeclSyntax>(constTokenRaw, Cursor::ConstToken);
}

ClassConstDeclSyntax ClassConstDeclSyntax::withConstList(std::optional<ClassConstListSyntax> constList)
{
   RefCountPtr<RawSyntax> constListRaw;
   if (constList.has_value()) {
      constListRaw = constList->getRaw();
   } else {
      constListRaw = RawSyntax::missing(SyntaxKind::ClassConstList);
   }
   return m_data->replaceChild<ClassConstDeclSyntax>(constListRaw, Cursor::ConstList);
}

///
/// ClassMethodDeclSyntax
///
void ClassMethodDeclSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassMethodDeclSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, FunctionToken, std::set{TokenKindType::T_FUNCTION});
   syntax_assert_child_token(raw, ReturnRefToken, std::set{TokenKindType::T_AMPERSAND});
   if (const RefCountPtr<RawSyntax> &modifiersChild = raw->getChild(Cursor::Modifiers)) {
      assert(modifiersChild->kindOf(SyntaxKind::ClassModifierList));
   }
   if (const RefCountPtr<RawSyntax> &funcNameChild = raw->getChild(Cursor::FuncName)) {
      assert(funcNameChild->kindOf(SyntaxKind::Identifier));
   }
   if (const RefCountPtr<RawSyntax> &parameterClauseChild = raw->getChild(Cursor::ParameterListClause)) {
      assert(parameterClauseChild->kindOf(SyntaxKind::ParameterListClause));
   }
   if (const RefCountPtr<RawSyntax> &returnTypeChild = raw->getChild(Cursor::ReturnType)) {
      assert(returnTypeChild->kindOf(SyntaxKind::ReturnTypeClause));
   }
   if (const RefCountPtr<RawSyntax> &bodyChild = raw->getChild(Cursor::Body)) {
      assert(bodyChild->kindOf(SyntaxKind::MemberDeclBlock));
   }
#endif
}

MemberModifierListSyntax ClassMethodDeclSyntax::getModifiers()
{
   return MemberModifierListSyntax {m_root, m_data->getChild(Cursor::Modifiers).get()};
}

TokenSyntax ClassMethodDeclSyntax::getFunctionToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::FunctionToken).get()};
}

std::optional<TokenSyntax> ClassMethodDeclSyntax::getReturnRefToken()
{
   RefCountPtr<SyntaxData> returnRefData = m_data->getChild(Cursor::ReturnRefToken);
   if (!returnRefData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, returnRefData.get()};
}

IdentifierSyntax ClassMethodDeclSyntax::getFuncName()
{
   return IdentifierSyntax {m_root, m_data->getChild(Cursor::FuncName).get()};
}

ParameterClauseSyntax ClassMethodDeclSyntax::getParameterClause()
{
   return ParameterClauseSyntax {m_root, m_data->getChild(Cursor::ParameterListClause).get()};
}

std::optional<ReturnTypeClauseSyntax> ClassMethodDeclSyntax::getReturnType()
{
   RefCountPtr<SyntaxData> returnTypeData = m_data->getChild(Cursor::ReturnType);
   if (!returnTypeData) {
      return std::nullopt;
   }
   return ReturnTypeClauseSyntax {m_root, returnTypeData.get()};
}

std::optional<MemberDeclBlockSyntax> ClassMethodDeclSyntax::getBody()
{
   RefCountPtr<SyntaxData> bodyData = m_data->getChild(Cursor::Body);
   if (!bodyData) {
      return std::nullopt;
   }
   return MemberDeclBlockSyntax {m_root, bodyData.get()};
}

ClassMethodDeclSyntax ClassMethodDeclSyntax::withModifiers(std::optional<MemberModifierListSyntax> modifiers)
{
   RefCountPtr<RawSyntax> modifiersRaw;
   if (modifiers.has_value()) {
      modifiersRaw = modifiers->getRaw();
   } else {
      modifiersRaw = RawSyntax::missing(SyntaxKind::MemberModifierList);
   }
   return m_data->replaceChild<ClassMethodDeclSyntax>(modifiersRaw, Cursor::Modifiers);
}

ClassMethodDeclSyntax ClassMethodDeclSyntax::withFunctionToken(std::optional<TokenSyntax> functionToken)
{
   RefCountPtr<RawSyntax> functionTokenRaw;
   if (functionToken.has_value()) {
      functionTokenRaw = functionToken->getRaw();
   } else {
      functionTokenRaw = make_missing_token(T_FUNCTION);
   }
   return m_data->replaceChild<ClassMethodDeclSyntax>(functionTokenRaw, Cursor::FunctionToken);
}

ClassMethodDeclSyntax ClassMethodDeclSyntax::withReturnRefToken(std::optional<TokenSyntax> returnRefToken)
{
   RefCountPtr<RawSyntax> returnRefTokenRaw;
   if (returnRefToken.has_value()) {
      returnRefTokenRaw = returnRefToken->getRaw();
   } else {
      returnRefTokenRaw = make_missing_token(T_AMPERSAND);
   }
   return m_data->replaceChild<ClassMethodDeclSyntax>(returnRefTokenRaw, Cursor::ReturnRefToken);
}

ClassMethodDeclSyntax ClassMethodDeclSyntax::withFuncName(std::optional<IdentifierSyntax> funcName)
{
   RefCountPtr<RawSyntax> funcNameRaw;
   if (funcName.has_value()) {
      funcNameRaw = funcName->getRaw();
   } else {
      funcNameRaw = RawSyntax::missing(SyntaxKind::Identifier);
   }
   return m_data->replaceChild<ClassMethodDeclSyntax>(funcNameRaw, Cursor::FuncName);
}

ClassMethodDeclSyntax ClassMethodDeclSyntax::withParameterClause(std::optional<ParameterClauseSyntax> parameterClause)
{
   RefCountPtr<RawSyntax> parameterClauseRaw;
   if (parameterClause.has_value()) {
      parameterClauseRaw = parameterClause->getRaw();
   } else {
      parameterClauseRaw = RawSyntax::missing(SyntaxKind::ParameterListClause);
   }
   return m_data->replaceChild<ClassMethodDeclSyntax>(parameterClauseRaw, Cursor::ParameterListClause);
}

ClassMethodDeclSyntax ClassMethodDeclSyntax::withReturnType(std::optional<ReturnTypeClauseSyntax> returnType)
{
   RefCountPtr<RawSyntax> returnTypeRaw;
   if (returnType.has_value()) {
      returnTypeRaw = returnType->getRaw();
   } else {
      returnTypeRaw = RawSyntax::missing(SyntaxKind::ReturnTypeClause);
   }
   return m_data->replaceChild<ClassMethodDeclSyntax>(returnTypeRaw, Cursor::ReturnType);
}

ClassMethodDeclSyntax ClassMethodDeclSyntax::withBody(std::optional<MemberDeclBlockSyntax> body)
{
   RefCountPtr<RawSyntax> bodyRaw;
   if (body.has_value()) {
      bodyRaw = body->getRaw();
   } else {
      bodyRaw = RawSyntax::missing(SyntaxKind::MemberDeclBlock);
   }
   return m_data->replaceChild<ClassMethodDeclSyntax>(bodyRaw, Cursor::Body);
}

///
/// ClassTraitMethodReferenceSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ClassTraitMethodReferenceSyntax::CHILD_NODE_CHOICES
{
   {
      ClassTraitMethodReferenceSyntax::Reference, {
         SyntaxKind::Identifier,
               SyntaxKind::ClassAbsoluteTraitMethodReference
      }
   }
};
#endif

void ClassTraitMethodReferenceSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassTraitMethodReferenceSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Reference, CHILD_NODE_CHOICES.at(Cursor::Reference));
#endif
}

Syntax ClassTraitMethodReferenceSyntax::getReference()
{
   return Syntax {m_root, m_data->getChild(Cursor::Reference).get()};
}

ClassTraitMethodReferenceSyntax ClassTraitMethodReferenceSyntax::withReference(std::optional<Syntax> reference)
{
   RefCountPtr<RawSyntax> referenceRaw;
   if (reference.has_value()) {
      referenceRaw = reference->getRaw();
   } else {
      referenceRaw = RawSyntax::missing(SyntaxKind::Identifier);
   }
   return m_data->replaceChild<ClassTraitMethodReferenceSyntax>(referenceRaw, Cursor::Reference);
}

///
/// ClassAbsoluteTraitMethodReferenceSyntax
///
void ClassAbsoluteTraitMethodReferenceSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassAbsoluteTraitMethodReferenceSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Separator, std::set{TokenKindType::T_PAAMAYIM_NEKUDOTAYIM});
   if (const RefCountPtr<RawSyntax> &baseNameChild = raw->getChild(Cursor::BaseName)) {
      assert(baseNameChild->kindOf(SyntaxKind::Name));
   }
   if (const RefCountPtr<RawSyntax> &memberNameChild = raw->getChild(Cursor::MemberName)) {
      assert(memberNameChild->kindOf(SyntaxKind::Identifier));
   }
#endif
}

NameSyntax ClassAbsoluteTraitMethodReferenceSyntax::getBaseName()
{
   return NameSyntax {m_root, m_data->getChild(Cursor::BaseName).get()};
}

TokenSyntax ClassAbsoluteTraitMethodReferenceSyntax::getSeparator()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Separator).get()};
}

IdentifierSyntax ClassAbsoluteTraitMethodReferenceSyntax::getMemberName()
{
   return IdentifierSyntax {m_root, m_data->getChild(Cursor::MemberName).get()};
}

ClassAbsoluteTraitMethodReferenceSyntax
ClassAbsoluteTraitMethodReferenceSyntax::withBaseName(std::optional<NameSyntax> baseName)
{
   RefCountPtr<RawSyntax> baseNameRaw;
   if (baseName.has_value()) {
      baseNameRaw = baseName->getRaw();
   } else {
      baseNameRaw = RawSyntax::missing(SyntaxKind::Name);
   }
   return m_data->replaceChild<ClassAbsoluteTraitMethodReferenceSyntax>(baseNameRaw, Cursor::BaseName);
}

ClassAbsoluteTraitMethodReferenceSyntax
ClassAbsoluteTraitMethodReferenceSyntax::withSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = make_missing_token(T_PAAMAYIM_NEKUDOTAYIM);
   }
   return m_data->replaceChild<ClassAbsoluteTraitMethodReferenceSyntax>(separatorRaw, Cursor::Separator);
}

ClassAbsoluteTraitMethodReferenceSyntax
ClassAbsoluteTraitMethodReferenceSyntax::withMemberName(std::optional<IdentifierSyntax> memberName)
{
   RefCountPtr<RawSyntax> memberNameRaw;
   if (memberName.has_value()) {
      memberNameRaw = memberName->getRaw();
   } else {
      memberNameRaw = RawSyntax::missing(SyntaxKind::Identifier);
   }
   return m_data->replaceChild<ClassAbsoluteTraitMethodReferenceSyntax>(memberNameRaw, Cursor::MemberName);
}

///
/// ClassTraitPrecedenceSyntax
///
void ClassTraitPrecedenceSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassTraitPrecedenceSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, InsteadOfToken, std::set{TokenKindType::T_INSTEADOF});
   if (const RefCountPtr<RawSyntax> &methodReferenceChild = raw->getChild(Cursor::MethodReference)) {
      assert(methodReferenceChild->kindOf(SyntaxKind::ClassAbsoluteTraitMethodReference));
   }
   if (const RefCountPtr<RawSyntax> &nameListChild = raw->getChild(Cursor::Names)) {
      assert(nameListChild->kindOf(SyntaxKind::NameList));
   }
#endif
}

ClassAbsoluteTraitMethodReferenceSyntax ClassTraitPrecedenceSyntax::getMethodReference()
{
   return ClassAbsoluteTraitMethodReferenceSyntax {m_root, m_data->getChild(Cursor::MethodReference).get()};
}

TokenSyntax ClassTraitPrecedenceSyntax::getInsteadOfToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::InsteadOfToken).get()};
}

NameListSyntax ClassTraitPrecedenceSyntax::getNames()
{
   return NameListSyntax {m_root, m_data->getChild(Cursor::Names).get()};
}

ClassTraitPrecedenceSyntax
ClassTraitPrecedenceSyntax::withMethodReference(std::optional<ClassAbsoluteTraitMethodReferenceSyntax> methodReference)
{
   RefCountPtr<RawSyntax> methodReferenceRaw;
   if (methodReference.has_value()) {
      methodReferenceRaw = methodReference->getRaw();
   } else {
      methodReferenceRaw = RawSyntax::missing(SyntaxKind::ClassAbsoluteTraitMethodReference);
   }
   return m_data->replaceChild<ClassTraitPrecedenceSyntax>(methodReferenceRaw, Cursor::MethodReference);
}

ClassTraitPrecedenceSyntax ClassTraitPrecedenceSyntax::withInsteadOfToken(std::optional<TokenSyntax> insteadOfToken)
{
   RefCountPtr<RawSyntax> insteadOfTokenRaw;
   if (insteadOfToken.has_value()) {
      insteadOfTokenRaw = insteadOfToken->getRaw();
   } else {
      insteadOfTokenRaw = make_missing_token(T_INSTEADOF);
   }
   return m_data->replaceChild<ClassTraitPrecedenceSyntax>(insteadOfTokenRaw, Cursor::InsteadOfToken);
}

ClassTraitPrecedenceSyntax ClassTraitPrecedenceSyntax::withNames(std::optional<NameListSyntax> names)
{
   RefCountPtr<RawSyntax> namesRaw;
   if (names.has_value()) {
      namesRaw = names->getRaw();
   } else {
      namesRaw = RawSyntax::missing(SyntaxKind::NameList);
   }
   return m_data->replaceChild<ClassTraitPrecedenceSyntax>(namesRaw, Cursor::Names);
}

///
/// ClassTraitAliasSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ClassTraitAliasSyntax::CHILD_NODE_CHOICES
{
   {
      ClassTraitAliasSyntax::Modifier, {
         SyntaxKind::ReservedNonModifier,
               SyntaxKind::MemberModifier
      }
   }
};
#endif

void ClassTraitAliasSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassTraitAliasSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, AsToken, std::set{TokenKindType::T_AS});
   if (const RefCountPtr<RawSyntax> &methodRefChild = raw->getChild(Cursor::MethodReference)) {
      assert(methodRefChild->kindOf(SyntaxKind::ClassTraitMethodReference));
   }
   syntax_assert_child_kind(raw, Modifier, CHILD_NODE_CHOICES.at(Cursor::Modifier));
   if (const RefCountPtr<RawSyntax> &aliasName = raw->getChild(Cursor::AliasName)) {
      if (aliasName->isToken()) {
         syntax_assert_child_token(raw, AliasName, std::set{TokenKindType::T_IDENTIFIER_STRING});
      } else {
         assert(aliasName->kindOf(SyntaxKind::Identifier));
      }
   }
   assert(raw->getChild(Cursor::AliasName) || raw->getChild(Cursor::AliasName));
#endif
}

ClassTraitMethodReferenceSyntax ClassTraitAliasSyntax::getMethodReference()
{
   return ClassTraitMethodReferenceSyntax {m_root, m_data->getChild(Cursor::MethodReference).get()};
}

TokenSyntax ClassTraitAliasSyntax::getAsToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::AsToken).get()};
}

std::optional<Syntax> ClassTraitAliasSyntax::getModifier()
{
   RefCountPtr<SyntaxData> modifierData = m_data->getChild(Cursor::Modifier);
   if (!modifierData) {
      return std::nullopt;
   }
   return Syntax {m_root, modifierData.get()};
}

std::optional<Syntax> ClassTraitAliasSyntax::getAliasName()
{
   RefCountPtr<SyntaxData> aliasData = m_data->getChild(Cursor::AliasName);
   if (!aliasData) {
      return std::nullopt;
   }
   return Syntax {m_root, aliasData.get()};
}

ClassTraitAliasSyntax ClassTraitAliasSyntax::withMethodReference(std::optional<ClassTraitMethodReferenceSyntax> methodReference)
{
   RefCountPtr<RawSyntax> methodReferenceRaw;
   if (methodReference.has_value()) {
      methodReferenceRaw = methodReference->getRaw();
   } else {
      methodReferenceRaw = RawSyntax::missing(SyntaxKind::ClassTraitMethodReference);
   }
   return m_data->replaceChild<ClassTraitAliasSyntax>(methodReferenceRaw, Cursor::MethodReference);
}

ClassTraitAliasSyntax ClassTraitAliasSyntax::withAsToken(std::optional<TokenSyntax> asToken)
{
   RefCountPtr<RawSyntax> asTokenRaw;
   if (asToken.has_value()) {
      asTokenRaw = asToken->getRaw();
   } else {
      asTokenRaw = make_missing_token(T_AS);
   }
   return m_data->replaceChild<ClassTraitAliasSyntax>(asTokenRaw, Cursor::AsToken);
}

ClassTraitAliasSyntax ClassTraitAliasSyntax::withModifier(std::optional<Syntax> modifier)
{
   RefCountPtr<RawSyntax> modifierRaw;
   if (modifier.has_value()) {
      modifierRaw = modifier->getRaw();
   } else {
      modifierRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ClassTraitAliasSyntax>(modifierRaw, Cursor::Modifier);
}

ClassTraitAliasSyntax ClassTraitAliasSyntax::withAliasName(std::optional<Syntax> aliasName)
{
   RefCountPtr<RawSyntax> aliasNameRaw;
   if (aliasName.has_value()) {
      aliasNameRaw = aliasName->getRaw();
   } else {
      aliasNameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ClassTraitAliasSyntax>(aliasNameRaw, Cursor::AliasName);
}

///
/// ClassTraitAdaptationSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ClassTraitAdaptationSyntax::CHILD_NODE_CHOICES
{
   {
      ClassTraitAdaptationSyntax::Adaptation, {
         SyntaxKind::ClassTraitPrecedence, SyntaxKind::ClassTraitAlias,
      }
   }
};
#endif

void ClassTraitAdaptationSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassTraitAdaptationSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Adaptation, CHILD_NODE_CHOICES.at(Cursor::Adaptation));
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
#endif
}

Syntax ClassTraitAdaptationSyntax::getAdaptation()
{
   return Syntax {m_root, m_data->getChild(Cursor::Adaptation).get()};
}

TokenSyntax ClassTraitAdaptationSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

ClassTraitAdaptationSyntax ClassTraitAdaptationSyntax::withAdaptation(std::optional<Syntax> adaptation)
{
   RefCountPtr<RawSyntax> adaptationRaw;
   if (adaptation.has_value()) {
      adaptationRaw = adaptation->getRaw();
   } else {
      adaptationRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ClassTraitAdaptationSyntax>(adaptationRaw, Cursor::Adaptation);
}

ClassTraitAdaptationSyntax ClassTraitAdaptationSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> semicolonRaw;
   if (semicolon.has_value()) {
      semicolonRaw = semicolon->getRaw();
   } else {
      semicolonRaw = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<ClassTraitAdaptationSyntax>(semicolonRaw, Cursor::Semicolon);
}

///
/// ClassTraitAdaptationBlockSyntax
///
void ClassTraitAdaptationBlockSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassTraitAdaptationBlockSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_BRACE});
   syntax_assert_child_token(raw, RightBrace, std::set{TokenKindType::T_RIGHT_BRACE});
   if (const RefCountPtr<RawSyntax> &adaptationChild = raw->getChild(Cursor::AdaptationList)) {
      assert(adaptationChild->kindOf(SyntaxKind::ClassTraitAdaptationList));
   }
#endif
}

TokenSyntax ClassTraitAdaptationBlockSyntax::getLeftBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

ClassTraitAdaptationListSyntax ClassTraitAdaptationBlockSyntax::getAdaptaionList()
{
   return ClassTraitAdaptationListSyntax {m_root, m_data->getChild(Cursor::AdaptationList).get()};
}

TokenSyntax ClassTraitAdaptationBlockSyntax::getRightBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightBrace).get()};
}

ClassTraitAdaptationBlockSyntax ClassTraitAdaptationBlockSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> leftBraceRaw;
   if (leftBrace.has_value()) {
      leftBraceRaw = leftBrace->getRaw();
   } else {
      leftBraceRaw = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<ClassTraitAdaptationBlockSyntax>(leftBraceRaw, Cursor::LeftBrace);
}

ClassTraitAdaptationBlockSyntax
ClassTraitAdaptationBlockSyntax::withAdaptationList(std::optional<ClassTraitAdaptationListSyntax> adaptaionList)
{
   RefCountPtr<RawSyntax> adaptaionListRaw;
   if (adaptaionList.has_value()) {
      adaptaionListRaw = adaptaionList->getRaw();
   } else {
      adaptaionListRaw = RawSyntax::missing(SyntaxKind::ClassTraitAdaptationList);
   }
   return m_data->replaceChild<ClassTraitAdaptationBlockSyntax>(adaptaionListRaw, Cursor::AdaptationList);
}

ClassTraitAdaptationBlockSyntax ClassTraitAdaptationBlockSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rightBraceRaw;
   if (rightBrace.has_value()) {
      rightBraceRaw = rightBrace->getRaw();
   } else {
      rightBraceRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<ClassTraitAdaptationBlockSyntax>(rightBraceRaw, Cursor::RightBrace);
}

///
/// ClassTraitDeclSyntax
///
void ClassTraitDeclSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassTraitDeclSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, UseToken, std::set{TokenKindType::T_USE});
   if (const RefCountPtr<RawSyntax> &nameListChild = raw->getChild(Cursor::NameList)) {
      assert(nameListChild->kindOf(SyntaxKind::NameList));
   }
   if (const RefCountPtr<RawSyntax> &adaptationBlockChild = raw->getChild(Cursor::AdaptationBlock)) {
      assert(adaptationBlockChild->kindOf(SyntaxKind::ClassTraitAdaptationBlock));
   }
#endif
}

TokenSyntax ClassTraitDeclSyntax::getUseToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::UseToken).get()};
}

NameListSyntax ClassTraitDeclSyntax::getNameList()
{
   return NameListSyntax {m_root, m_data->getChild(Cursor::NameList).get()};
}

std::optional<ClassTraitAdaptationBlockSyntax> ClassTraitDeclSyntax::getAdaptationBlock()
{
   RefCountPtr<SyntaxData> adaptationBlockData = m_data->getChild(Cursor::AdaptationBlock);
   if (!adaptationBlockData) {
      return std::nullopt;
   }
   return ClassTraitAdaptationBlockSyntax {m_root, adaptationBlockData.get()};
}

ClassTraitDeclSyntax ClassTraitDeclSyntax::withUseToken(std::optional<TokenSyntax> useToken)
{
   RefCountPtr<RawSyntax> useTokenRaw;
   if (useToken.has_value()) {
      useTokenRaw = useToken->getRaw();
   } else {
      useTokenRaw = make_missing_token(T_USE);
   }
   return m_data->replaceChild<ClassTraitDeclSyntax>(useTokenRaw, Cursor::UseToken);
}

ClassTraitDeclSyntax ClassTraitDeclSyntax::withNameList(std::optional<NameListSyntax> nameList)
{
   RefCountPtr<RawSyntax> nameListRaw;
   if (nameList.has_value()) {
      nameListRaw = nameList->getRaw();
   } else {
      nameListRaw = RawSyntax::missing(SyntaxKind::NameList);
   }
   return m_data->replaceChild<ClassTraitDeclSyntax>(nameListRaw, Cursor::NameList);
}

ClassTraitDeclSyntax ClassTraitDeclSyntax::withAdaptationBlock(std::optional<ClassTraitAdaptationBlockSyntax> adaptationBlock)
{
   RefCountPtr<RawSyntax> adaptationBlockRaw;
   if (adaptationBlock.has_value()) {
      adaptationBlockRaw = adaptationBlock->getRaw();
   } else {
      adaptationBlockRaw = nullptr;
   }
   return m_data->replaceChild<ClassTraitDeclSyntax>(adaptationBlockRaw, Cursor::AdaptationBlock);
}

///
/// ImplementsClauseSyntax
///
void InterfaceExtendsClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == InterfaceExtendsClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ExtendsToken, std::set{TokenKindType::T_EXTENDS});
   if (const RefCountPtr<RawSyntax> &interfaceChild = raw->getChild(Cursor::Interfaces)) {
      assert(interfaceChild->kindOf(SyntaxKind::ImplementsClause));
   }
#endif
}

TokenSyntax InterfaceExtendsClauseSyntax::getExtendsToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ExtendsToken).get()};
}

NameListSyntax InterfaceExtendsClauseSyntax::getInterfaces()
{
   return NameListSyntax {m_root, m_data->getChild(Cursor::Interfaces).get()};
}

InterfaceExtendsClauseSyntax InterfaceExtendsClauseSyntax::withExtendsToken(std::optional<TokenSyntax> extendsToken)
{
   RefCountPtr<RawSyntax> extendsTokenRaw;
   if (extendsToken.has_value()) {
      extendsTokenRaw = extendsToken->getRaw();
   } else {
      extendsTokenRaw = make_missing_token(T_EXTENDS);
   }
   return m_data->replaceChild<InterfaceExtendsClauseSyntax>(extendsTokenRaw, Cursor::ExtendsToken);
}

InterfaceExtendsClauseSyntax InterfaceExtendsClauseSyntax::withInterfaces(std::optional<NameListSyntax> interfaces)
{
   RefCountPtr<RawSyntax> interfacesRaw;
   if (interfaces.has_value()) {
      interfacesRaw = interfaces->getRaw();
   } else {
      interfacesRaw = RawSyntax::missing(SyntaxKind::NameList);
   }
   return m_data->replaceChild<InterfaceExtendsClauseSyntax>(interfacesRaw, Cursor::Interfaces);
}

///
/// ClassPropertyClauseSyntax
///
void ClassPropertyClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassPropertyClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
   if (const RefCountPtr<RawSyntax> &initializerChild = raw->getChild(Cursor::Initializer)) {
      assert(initializerChild->kindOf(SyntaxKind::InitializerClause));
   }
#endif
}

TokenSyntax ClassPropertyClauseSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

std::optional<InitializerClauseSyntax> ClassPropertyClauseSyntax::getInitializer()
{
   RefCountPtr<SyntaxData> initializerData = m_data->getChild(Cursor::Initializer);
   if (!initializerData) {
      return std::nullopt;
   }
   return InitializerClauseSyntax {m_root, initializerData.get()};
}

ClassPropertyClauseSyntax ClassPropertyClauseSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> variableRaw;
   if (variable.has_value()) {
      variableRaw = variable->getRaw();
   } else {
      variableRaw = make_missing_token(T_VARIABLE);
   }
   return m_data->replaceChild<ClassPropertyClauseSyntax>(variableRaw, Cursor::Variable);
}

ClassPropertyClauseSyntax ClassPropertyClauseSyntax::withInitializer(std::optional<InitializerClauseSyntax> initializer)
{
   RefCountPtr<RawSyntax> initializerRaw;
   if (initializer.has_value()) {
      initializerRaw = initializer->getRaw();
   } else {
      initializerRaw = nullptr;
   }
   return m_data->replaceChild<ClassPropertyClauseSyntax>(initializerRaw, Cursor::Initializer);
}

///
/// ClassConstClauseSyntax
///
void ClassConstClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassPropertyClauseSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &identifierChild = raw->getChild(Cursor::Identifier)) {
      assert(identifierChild->kindOf(SyntaxKind::Identifier));
   }
   if (const RefCountPtr<RawSyntax> &initializerChild = raw->getChild(Cursor::Initializer)) {
      assert(initializerChild->kindOf(SyntaxKind::InitializerClause));
   }
#endif
}

IdentifierSyntax ClassConstClauseSyntax::getIdentifier()
{
   return IdentifierSyntax {m_root, m_data->getChild(Cursor::Identifier).get()};
}

InitializerClauseSyntax ClassConstClauseSyntax::getInitializer()
{
   return InitializerClauseSyntax {m_root, m_data->getChild(Cursor::Initializer).get()};
}

ClassConstClauseSyntax ClassConstClauseSyntax::withIdentifier(std::optional<IdentifierSyntax> identifier)
{
   RefCountPtr<RawSyntax> identifierRaw;
   if (identifier.has_value()) {
      identifierRaw = identifier->getRaw();
   } else {
      identifierRaw = RawSyntax::missing(SyntaxKind::Identifier);
   }
   return m_data->replaceChild<ClassConstClauseSyntax>(identifierRaw, Cursor::Identifier);
}

ClassConstClauseSyntax ClassConstClauseSyntax::withInitializer(std::optional<InitializerClauseSyntax> initializer)
{
   RefCountPtr<RawSyntax> initializerRaw;
   if (initializer.has_value()) {
      initializerRaw = initializer->getRaw();
   } else {
      initializerRaw = RawSyntax::missing(SyntaxKind::InitializerClause);
   }
   return m_data->replaceChild<ClassConstClauseSyntax>(initializerRaw, Cursor::Initializer);
}

///
/// ClassConstListItemSyntax
///
void ClassConstListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassConstListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Comma, std::set{TokenKindType::T_COMMA});
   syntax_assert_child_kind(raw, ConstDecl, std::set{SyntaxKind::ClassConstClause});
#endif
}

std::optional<TokenSyntax>
ClassConstListItemSyntax::getComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::Comma);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

ClassConstClauseSyntax
ClassConstListItemSyntax::getConstDecl()
{
   return ClassConstClauseSyntax{m_root, m_data->getChild(Cursor::ConstDecl).get()};
}

ClassConstListItemSyntax
ClassConstListItemSyntax::withComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> rawComma;
   if (comma.has_value()) {
      rawComma = comma->getRaw();
   } else {
      rawComma = nullptr;
   }
   return m_data->replaceChild<ClassConstListItemSyntax>(rawComma, Cursor::Comma);
}

ClassConstListItemSyntax
ClassConstListItemSyntax::withConstDecl(std::optional<ClassConstClauseSyntax> decl)
{
   RefCountPtr<RawSyntax> rawDecl;
   if (decl.has_value()) {
      rawDecl = decl->getRaw();
   } else {
      rawDecl = RawSyntax::missing(SyntaxKind::ClassConstClause);
   }
   return m_data->replaceChild<ClassConstListItemSyntax>(rawDecl, Cursor::ConstDecl);
}

///
/// MemberDeclListItemSyntax
///
void MemberDeclListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == MemberDeclListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Semicolon, std::set{TokenKindType::T_SEMICOLON});
   if (const RefCountPtr<RawSyntax> &declChild = raw->getChild(Cursor::Decl)) {
      assert(declChild->kindOf(SyntaxKind::Decl));
   }
#endif
}

DeclSyntax MemberDeclListItemSyntax::getDecl()
{
   return DeclSyntax {m_root, m_data->getChild(Cursor::Decl).get()};
}

TokenSyntax MemberDeclListItemSyntax::getSemicolon()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Semicolon).get()};
}

MemberDeclListItemSyntax MemberDeclListItemSyntax::withDecl(std::optional<DeclSyntax> decl)
{
   RefCountPtr<RawSyntax> declRaw;
   if (decl.has_value()) {
      declRaw = decl->getRaw();
   } else {
      declRaw = RawSyntax::missing(SyntaxKind::Decl);
   }
   return m_data->replaceChild<MemberDeclListItemSyntax>(declRaw, Cursor::Decl);
}

MemberDeclListItemSyntax MemberDeclListItemSyntax::withSemicolon(std::optional<TokenSyntax> semicolon)
{
   RefCountPtr<RawSyntax> semicolonRaw;
   if (semicolon.has_value()) {
      semicolonRaw = semicolon->getRaw();
   } else {
      semicolonRaw = make_missing_token(T_SEMICOLON);
   }
   return m_data->replaceChild<MemberDeclListItemSyntax>(semicolonRaw, Cursor::Semicolon);
}

///
/// MemberDeclBlockSyntax
///
void MemberDeclBlockSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == MemberDeclBlockSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_BRACE});
   syntax_assert_child_token(raw, RightBrace, std::set{TokenKindType::T_RIGHT_BRACE});
   if (const RefCountPtr<RawSyntax> &membersChild = raw->getChild(Cursor::Members)) {
      assert(membersChild->kindOf(SyntaxKind::MemberDeclList));
   }
#endif
}

TokenSyntax MemberDeclBlockSyntax::getLeftBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

MemberDeclListSyntax MemberDeclBlockSyntax::getMembers()
{
   return MemberDeclListSyntax {m_root, m_data->getChild(Cursor::Members).get()};
}

TokenSyntax MemberDeclBlockSyntax::getRightBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightBrace).get()};
}

MemberDeclBlockSyntax MemberDeclBlockSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> leftBraceRaw;
   if (leftBrace.has_value()) {
      leftBraceRaw = leftBrace->getRaw();
   } else {
      leftBraceRaw = make_missing_token(T_LEFT_BRACE);
   }
   return m_data->replaceChild<MemberDeclBlockSyntax>(leftBraceRaw, Cursor::LeftBrace);
}

MemberDeclBlockSyntax MemberDeclBlockSyntax::withMembers(std::optional<MemberDeclListSyntax> members)
{
   RefCountPtr<RawSyntax> membersRaw;
   if (members.has_value()) {
      membersRaw = members->getRaw();
   } else {
      membersRaw = RawSyntax::missing(SyntaxKind::MemberDeclList);
   }
   return m_data->replaceChild<MemberDeclBlockSyntax>(membersRaw, Cursor::Members);
}

MemberDeclBlockSyntax MemberDeclBlockSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rightBraceRaw;
   if (rightBrace.has_value()) {
      rightBraceRaw = rightBrace->getRaw();
   } else {
      rightBraceRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<MemberDeclBlockSyntax>(rightBraceRaw, Cursor::RightBrace);
}

///
/// ClassDefinitionSyntax
///
void ClassDefinitionSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == ClassDefinitionSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ClassToken, std::set{TokenKindType::T_CLASS});
   syntax_assert_child_token(raw, Name, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_kind(raw, Modififers, std::set{SyntaxKind::ClassModifierList});
   syntax_assert_child_kind(raw, ExtendsFrom, std::set{SyntaxKind::ExtendsFromClause});
   syntax_assert_child_kind(raw, ImplementsList, std::set{SyntaxKind::ImplementsClause});
   syntax_assert_child_kind(raw, Members, std::set{SyntaxKind::MemberDeclBlock});
#endif
}

std::optional<ClassModifierListSyntax> ClassDefinitionSyntax::getModififers()
{
   RefCountPtr<SyntaxData> modifiersData = m_data->getChild(Cursor::Modififers);
   if (!modifiersData) {
      return std::nullopt;
   }
   return ClassModifierListSyntax {m_root, modifiersData.get()};
}

TokenSyntax ClassDefinitionSyntax::getClassToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ClassToken).get()};
}

TokenSyntax ClassDefinitionSyntax::getName()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Name).get()};
}

std::optional<ExtendsFromClauseSyntax> ClassDefinitionSyntax::getExtendsFrom()
{
   RefCountPtr<SyntaxData> extendsData = m_data->getChild(Cursor::ExtendsFrom);
   if (!extendsData) {
      return std::nullopt;
   }
   return ExtendsFromClauseSyntax {m_root, extendsData.get()};
}

std::optional<ImplementsClauseSyntax> ClassDefinitionSyntax::getImplementsList()
{
   RefCountPtr<SyntaxData> implementsData = m_data->getChild(Cursor::ExtendsFrom);
   if (!implementsData) {
      return std::nullopt;
   }
   return ImplementsClauseSyntax {m_root, implementsData.get()};
}

MemberDeclBlockSyntax ClassDefinitionSyntax::getMembers()
{
   return MemberDeclBlockSyntax {m_root, m_data->getChild(Cursor::Members).get()};
}

ClassDefinitionSyntax ClassDefinitionSyntax::withModifiers(std::optional<ClassModifierListSyntax> modifiers)
{
   RefCountPtr<RawSyntax> modifiersRaw;
   if (modifiers.has_value()) {
      modifiersRaw = modifiers->getRaw();
   } else {
      modifiersRaw = nullptr;
   }
   return m_data->replaceChild<ClassDefinitionSyntax>(modifiersRaw, Cursor::Modififers);
}

ClassDefinitionSyntax ClassDefinitionSyntax::withClassToken(std::optional<TokenSyntax> classToken)
{
   RefCountPtr<RawSyntax> classTokenRaw;
   if (classToken.has_value()) {
      classTokenRaw = classToken->getRaw();
   } else {
      classTokenRaw = make_missing_token(T_CLASS);
   }
   return m_data->replaceChild<ClassDefinitionSyntax>(classTokenRaw, Cursor::ClassToken);
}

ClassDefinitionSyntax ClassDefinitionSyntax::withName(std::optional<TokenSyntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<ClassDefinitionSyntax>(nameRaw, Cursor::Name);
}

ClassDefinitionSyntax ClassDefinitionSyntax::withExtendsFrom(std::optional<ExtendsFromClauseSyntax> extends)
{
   RefCountPtr<RawSyntax> extendsRaw;
   if (extends.has_value()) {
      extendsRaw = extends->getRaw();
   } else {
      extendsRaw = nullptr;
   }
   return m_data->replaceChild<ClassDefinitionSyntax>(extendsRaw, Cursor::ExtendsFrom);
}

ClassDefinitionSyntax ClassDefinitionSyntax::withImplementsList(std::optional<ImplementsClauseSyntax> implements)
{
   RefCountPtr<RawSyntax> implementsRaw;
   if (implements.has_value()) {
      implementsRaw = implements->getRaw();
   } else {
      implementsRaw = nullptr;
   }
   return m_data->replaceChild<ClassDefinitionSyntax>(implementsRaw, Cursor::ImplementsList);
}

ClassDefinitionSyntax ClassDefinitionSyntax::withMembers(std::optional<MemberDeclBlockSyntax> members)
{
   RefCountPtr<RawSyntax> membersRaw;
   if (members.has_value()) {
      membersRaw = members->getRaw();
   } else {
      membersRaw = RawSyntax::missing(SyntaxKind::MemberDeclBlock);
   }
   return m_data->replaceChild<ClassDefinitionSyntax>(membersRaw, Cursor::Members);
}

///
/// InterfaceDefinitionSyntax
///
void InterfaceDefinitionSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == InterfaceDefinitionSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, InterfaceToken, std::set{TokenKindType::T_INTERFACE});
   syntax_assert_child_token(raw, Name, std::set{TokenKindType::T_IDENTIFIER_STRING});
   if (const RefCountPtr<RawSyntax> &extendsChild = raw->getChild(Cursor::ExtendsFrom)) {
      assert(extendsChild->kindOf(SyntaxKind::InterfaceExtendsClause));
   }
   if (const RefCountPtr<RawSyntax> &membersChild = raw->getChild(Cursor::Members)) {
      assert(membersChild->kindOf(SyntaxKind::MemberDeclBlock));
   }
#endif
}

TokenSyntax InterfaceDefinitionSyntax::getInterfaceToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::InterfaceToken).get()};
}

TokenSyntax InterfaceDefinitionSyntax::getName()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Name).get()};
}

std::optional<InterfaceExtendsClauseSyntax> InterfaceDefinitionSyntax::getExtendsFrom()
{
   RefCountPtr<SyntaxData> extendsData = m_data->getChild(Cursor::ExtendsFrom);
   if (!extendsData) {
      return std::nullopt;
   }
   return InterfaceExtendsClauseSyntax {m_root, extendsData.get()};
}

MemberDeclBlockSyntax InterfaceDefinitionSyntax::getMembers()
{
   return MemberDeclBlockSyntax {m_root, m_data->getChild(Cursor::Members).get()};
}

InterfaceDefinitionSyntax InterfaceDefinitionSyntax::withInterfaceToken(std::optional<TokenSyntax> interfaceToken)
{
   RefCountPtr<RawSyntax> interfaceTokenRaw;
   if (interfaceToken.has_value()) {
      interfaceTokenRaw = interfaceToken->getRaw();
   } else {
      interfaceTokenRaw = make_missing_token(T_INTERFACE);
   }
   return m_data->replaceChild<InterfaceDefinitionSyntax>(interfaceTokenRaw, Cursor::InterfaceToken);
}

InterfaceDefinitionSyntax InterfaceDefinitionSyntax::withName(std::optional<TokenSyntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<InterfaceDefinitionSyntax>(nameRaw, Cursor::Name);
}

InterfaceDefinitionSyntax InterfaceDefinitionSyntax::withExtendsFrom(std::optional<InterfaceExtendsClauseSyntax> extendsFrom)
{
   RefCountPtr<RawSyntax> extendsFromRaw;
   if (extendsFrom.has_value()) {
      extendsFromRaw = extendsFrom->getRaw();
   } else {
      extendsFromRaw = nullptr;
   }
   return m_data->replaceChild<InterfaceDefinitionSyntax>(extendsFromRaw, Cursor::ExtendsFrom);
}

InterfaceDefinitionSyntax InterfaceDefinitionSyntax::withMembers(std::optional<MemberDeclBlockSyntax> members)
{
   RefCountPtr<RawSyntax> membersRaw;
   if (members.has_value()) {
      membersRaw = members->getRaw();
   } else {
      membersRaw = RawSyntax::missing(SyntaxKind::MemberDeclBlock);
   }
   return m_data->replaceChild<InterfaceDefinitionSyntax>(membersRaw, Cursor::Members);
}

///
/// TraitDefinitionSyntax
///
void TraitDefinitionSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == TraitDefinitionSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, TraitToken, std::set{TokenKindType::T_TRAIT});
   syntax_assert_child_token(raw, Name, std::set{TokenKindType::T_IDENTIFIER_STRING});
   if (const RefCountPtr<RawSyntax> &membersChild = raw->getChild(Cursor::Members)) {
      assert(membersChild->kindOf(SyntaxKind::MemberDeclBlock));
   }
#endif
}

TokenSyntax TraitDefinitionSyntax::getTraitToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::TraitToken).get()};
}

TokenSyntax TraitDefinitionSyntax::getName()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Name).get()};
}

MemberDeclBlockSyntax TraitDefinitionSyntax::getMembers()
{
   return MemberDeclBlockSyntax {m_root, m_data->getChild(Cursor::Members).get()};
}

TraitDefinitionSyntax TraitDefinitionSyntax::withTraitToken(std::optional<TokenSyntax> traitToken)
{
   RefCountPtr<RawSyntax> traitTokenRaw;
   if (traitToken.has_value()) {
      traitTokenRaw = traitToken->getRaw();
   } else {
      traitTokenRaw = make_missing_token(T_TRAIT);
   }
   return m_data->replaceChild<TraitDefinitionSyntax>(traitTokenRaw, Cursor::TraitToken);
}

TraitDefinitionSyntax TraitDefinitionSyntax::withName(std::optional<TokenSyntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<TraitDefinitionSyntax>(nameRaw, Cursor::TraitToken);
}

TraitDefinitionSyntax TraitDefinitionSyntax::withMembers(std::optional<MemberDeclBlockSyntax> members)
{
   RefCountPtr<RawSyntax> membersRaw;
   if (members.has_value()) {
      membersRaw = members->getRaw();
   } else {
      membersRaw = RawSyntax::missing(SyntaxKind::MemberDeclBlock);
   }
   return m_data->replaceChild<TraitDefinitionSyntax>(membersRaw, Cursor::Members);
}

///
/// SourceFileSyntax
///
void SourceFileSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SourceFileSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax SourceFileSyntax::getEofToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::EOFToken).get()};
}

TopStmtListSyntax SourceFileSyntax::getStatements()
{
   return TopStmtListSyntax{m_root, m_data->getChild(Cursor::Statements).get()};
}

SourceFileSyntax SourceFileSyntax::withStatements(std::optional<TopStmtListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::TopStmtList);
   }
   return m_data->replaceChild<SourceFileSyntax>(rawStatements, Cursor::Statements);
}

SourceFileSyntax SourceFileSyntax::addStatement(TopStmtSyntax statement)
{
   RefCountPtr<RawSyntax> rawStatements = getRaw()->getChild(Cursor::Statements);
   if (rawStatements) {
      rawStatements->append(statement.getRaw());
   } else {
      rawStatements = RawSyntax::make(SyntaxKind::TopStmtList, {statement.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SourceFileSyntax>(rawStatements, Cursor::Statements);
}

SourceFileSyntax SourceFileSyntax::withEofToken(std::optional<TokenSyntax> eofToken)
{
   RefCountPtr<RawSyntax> rawEofToken;
   if (eofToken.has_value()) {
      rawEofToken = eofToken->getRaw();
   } else {
      rawEofToken = RawSyntax::missing(TokenKindType::END, OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<SourceFileSyntax>(rawEofToken, Cursor::EOFToken);
}

} // polar::syntax
