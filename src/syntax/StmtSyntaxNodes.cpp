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

#include "polarphp/syntax/syntaxnode/StmtSyntaxNodes.h"

namespace polar::syntax {

void ThrowStmtSyntax::validate()
{
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ThrowStmtSyntax::CHILDREN_COUNT);
}

TokenSyntax ThrowStmtSyntax::getThrowKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ThrowKeyword).get()};
}

ExprSyntax ThrowStmtSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

ThrowStmtSyntax ThrowStmtSyntax::withThrowKeyword(std::optional<TokenSyntax> throwKeyword)
{
   RefCountPtr<RawSyntax> raw;
   if (throwKeyword.has_value()) {
      raw = throwKeyword->getRaw();
   } else {
      raw = RawSyntax::missing(TokenKindType::T_THROW,
                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_THROW)));
   }
   return m_data->replaceChild<ThrowStmtSyntax>(raw, TokenKindType::T_THROW);
}

ThrowStmtSyntax ThrowStmtSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> raw;
   if (expr.has_value()) {
      raw = expr->getRaw();
   } else {
      raw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ThrowStmtSyntax>(raw, Cursor::Expr);
}

} // polar::syntax
