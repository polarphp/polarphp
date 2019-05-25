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

#include "polarphp/syntax/builder/ExprSyntaxNodeBuilders.h"

namespace polar::syntax {

///
/// NullExprSyntaxBuilder
///

NullExprSyntaxBuilder &NullExprSyntaxBuilder::useNullKeyword(TokenSyntax nullKeyword)
{
   m_layout[cursor_index(Cursor::NulllKeyword)] = nullKeyword.getRaw();
   return *this;
}

NullExprSyntax NullExprSyntaxBuilder::build()
{
   CursorIndex nullKeywordIndex = cursor_index(Cursor::NulllKeyword);
   if (!m_layout[nullKeywordIndex]) {
      m_layout[nullKeywordIndex] = RawSyntax::missing(TokenKindType::T_NULL,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_NULL)));
   }
   RefCountPtr<RawSyntax> rawNullExpr = RawSyntax::make(SyntaxKind::NullExpr, m_layout,
                                                        SourcePresence::Present, m_arena);
   return make<NullExprSyntax>(rawNullExpr);
}

///
/// ClassRefParentExprSyntaxBuilder
///

ClassRefParentExprSyntaxBuilder &ClassRefParentExprSyntaxBuilder::useParentKeyword(TokenSyntax parentKeyword)
{
   m_layout[cursor_index(Cursor::ParentKeyword)] = parentKeyword.getRaw();
   return *this;
}

ClassRefParentExprSyntax ClassRefParentExprSyntaxBuilder::build()
{
   CursorIndex parentKeywordIndex = cursor_index(Cursor::ParentKeyword);
   if (!m_layout[parentKeywordIndex]) {
      m_layout[parentKeywordIndex] = RawSyntax::missing(TokenKindType::T_CLASS_REF_PARENT,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)));
   }
   RefCountPtr<RawSyntax> rawParentKeyword = RawSyntax::make(SyntaxKind::ClassRefParentExpr, m_layout, SourcePresence::Present,
                                                             m_arena);
   return make<ClassRefParentExprSyntax>(rawParentKeyword);
}

///
/// ClassRefSelfExprSyntax
///
ClassRefSelfExprSyntaxBuilder &ClassRefSelfExprSyntaxBuilder::useSelfKeyword(TokenSyntax selfKeyword)
{
   m_layout[cursor_index(Cursor::SelfKeyword)] = selfKeyword.getRaw();
   return *this;
}

ClassRefSelfExprSyntax ClassRefSelfExprSyntaxBuilder::build()
{
   CursorIndex selfKeywordIndex = cursor_index(Cursor::SelfKeyword);
   if (!m_layout[selfKeywordIndex]) {
      m_layout[selfKeywordIndex] = RawSyntax::missing(TokenKindType::T_CLASS_REF_SELF,
                                                      OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)));
   }
   RefCountPtr<RawSyntax> rawParentKeyword = RawSyntax::make(SyntaxKind::ClassRefSelfExpr, m_layout, SourcePresence::Present,
                                                             m_arena);
   return make<ClassRefSelfExprSyntax>(rawParentKeyword);
}

///
/// ClassRefStaticExprSyntax
///
ClassRefStaticExprSyntaxBuilder &ClassRefStaticExprSyntaxBuilder::useStaticKeyword(TokenSyntax staticKeyword)
{
   m_layout[cursor_index(Cursor::StaticKeyword)] = staticKeyword.getRaw();
   return *this;
}

ClassRefStaticExprSyntax ClassRefStaticExprSyntaxBuilder::build()
{
   CursorIndex staticKeywordIndex = cursor_index(Cursor::StaticKeyword);
   if (!m_layout[staticKeywordIndex]) {
      m_layout[staticKeywordIndex] = RawSyntax::missing(TokenKindType::T_CLASS_REF_STATIC,
                                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)));
   }
   RefCountPtr<RawSyntax> rawParentKeyword = RawSyntax::make(SyntaxKind::ClassRefStaticExpr, m_layout, SourcePresence::Present,
                                                             m_arena);
   return make<ClassRefStaticExprSyntax>(rawParentKeyword);
}

} // polar::syntax
