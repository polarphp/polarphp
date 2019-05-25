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

} // polar::syntax
