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

namespace polar::syntax {

///
/// ReservedNonModifierSyntax
///
void ReservedNonModifierSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ReservedNonModifierSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, Modifier, CHILD_TOKEN_CHOICES.at(Cursor::Modifier));
}

#ifdef POLAR_DEBUG_BUILD
const std::map<SyntaxChildrenCountType, std::set<TokenKindType>> ReservedNonModifierSyntax::CHILD_TOKEN_CHOICES
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
   RefCountPtr<RawSyntax> raw;
   if (modifier.has_value()) {
      raw = modifier->getRaw();
   } else {
      /// not very good
      raw = RawSyntax::missing(TokenKindType::T_INCLUDE,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_INCLUDE)));
   }
   return m_data->replaceChild<ReservedNonModifierSyntax>(raw, Cursor::Modifier);
}

///
/// SemiReservedSytnax
///
#ifdef POLAR_DEBUG_BUILD
const std::map<SyntaxChildrenCountType, std::set<TokenKindType>> SemiReservedSytnax::CHILD_TOKEN_CHOICES
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
}

Syntax SemiReservedSytnax::getModifier()
{
   return Syntax{m_root, m_data->getChild(Cursor::Modifier).get()};
}

SemiReservedSytnax SemiReservedSytnax::withModifier(std::optional<Syntax> modifier)
{
   RefCountPtr<RawSyntax> raw;
   if (modifier.has_value()) {
      raw = modifier->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<SemiReservedSytnax>(raw, Cursor::Modifier);
}

///
/// IdentifierSyntax
///
void IdentifierSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == IdentifierSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &itemChild = raw->getChild(Cursor::NameItem)) {
      if (itemChild->isToken()) {
         syntax_assert_child_token(raw, NameItem, std::set<TokenKindType>{TokenKindType::T_IDENTIFIER_STRING});
         return;
      }
      assert(itemChild->kindOf(SyntaxKind::SemiReserved));
   }
}

Syntax IdentifierSyntax::getNameItem()
{
   return Syntax{m_root, m_data->getChild(Cursor::NameItem).get()};
}

IdentifierSyntax IdentifierSyntax::withNameItem(std::optional<Syntax> item)
{
   RefCountPtr<RawSyntax> raw;
   if (item.has_value()) {
      raw = item->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<IdentifierSyntax>(raw, Cursor::NameItem);
}

///
/// SourceFileSyntax
///
void SourceFileSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SourceFileSyntax::CHILDREN_COUNT);
}

TokenSyntax SourceFileSyntax::getEofToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::EOFToken).get()};
}

CodeBlockItemListSyntax SourceFileSyntax::getStatements()
{
   return CodeBlockItemListSyntax{m_root, m_data->getChild(Cursor::Statements).get()};
}

SourceFileSyntax SourceFileSyntax::withStatements(std::optional<CodeBlockItemListSyntax> statements)
{
   RefCountPtr<RawSyntax> rawStatements;
   if (statements.has_value()) {
      rawStatements = statements->getRaw();
   } else {
      rawStatements = RawSyntax::missing(SyntaxKind::CodeBlockItemList);
   }
   return m_data->replaceChild<SourceFileSyntax>(rawStatements, Cursor::Statements);
}

SourceFileSyntax SourceFileSyntax::addStatement(CodeBlockItemSyntax statement)
{
   RefCountPtr<RawSyntax> rawStatements = getRaw()->getChild(Cursor::Statements);
   if (rawStatements) {
      rawStatements->append(statement.getRaw());
   } else {
      rawStatements = RawSyntax::make(SyntaxKind::CodeBlockItemList, {statement.getRaw()}, SourcePresence::Present);
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
