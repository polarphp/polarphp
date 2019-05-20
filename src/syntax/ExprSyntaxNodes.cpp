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

#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"

namespace polar::syntax {

void NullExpr::validate()
{
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == 1);
}

TokenSyntax NullExpr::getNullKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::NulllKeyword).get()};
}

NullExpr NullExpr::withNullKeyword(std::optional<TokenSyntax> keyword)
{
   RefCountPtr<RawSyntax> raw;
   if (keyword.has_value()) {
      raw = keyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_NULL,
                               OwnedString::makeUnowned((get_token_text(TokenKindType::T_NULL))));
   }
   return m_data->replaceChild<NullExpr>(raw, Cursor::NulllKeyword);
}

void ClassRefParentExpr::validate()
{
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == 1);
}

TokenSyntax ClassRefParentExpr::getParentKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ParentKeyword).get()};
}

ClassRefParentExpr ClassRefParentExpr::withParentKeyword(std::optional<TokenSyntax> parentKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (parentKeyword.has_value()) {
      raw = parentKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CLASS_REF_PARENT,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)));
   }
   return m_data->replaceChild<ClassRefParentExpr>(raw, Cursor::ParentKeyword);
}

void ClassRefSelfExpr::validate()
{
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == 1);
}

TokenSyntax ClassRefSelfExpr::getSelfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SelfKeyword).get()};
}

ClassRefSelfExpr ClassRefSelfExpr::withSelfKeyword(std::optional<TokenSyntax> selfKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (selfKeyword.has_value()) {
      raw = selfKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CLASS_REF_SELF,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)));
   }
   return m_data->replaceChild<ClassRefSelfExpr>(raw, Cursor::SelfKeyword);
}

TokenSyntax ClassRefStaticExpr::getStaticKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::StaticKeyword).get()};
}

ClassRefStaticExpr ClassRefStaticExpr::withStaticKeyword(std::optional<TokenSyntax> staticKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (staticKeyword.has_value()) {
      raw = staticKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_CLASS_REF_STATIC,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)));
   }
   return m_data->replaceChild<ClassRefStaticExpr>(raw, Cursor::StaticKeyword);
}

} // polar::syntax
