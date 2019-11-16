//===----------- TokenSyntax.h - Swift Token Interface ----------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/07.
//===----------------------------------------------------------------------===//
//
// This file contains the interface for a `TokenSyntax`, which is a token
// that includes full-fidelity leading and trailing trivia.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_SYNTAX_TOKENSYNTAX_H
#define POLARPHP_SYNTAX_TOKENSYNTAX_H

#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/References.h"
#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/Trivia.h"

namespace polar::syntax {

class TokenSyntax final : public Syntax
{
public:
   TokenSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static TokenSyntax missingToken(const TokenKindType kind, OwnedString text)
   {
      return make<TokenSyntax>(RawSyntax::missing(kind, text));
   }

   Trivia getLeadingTrivia() const
   {
      return Trivia { getRaw()->getLeadingTrivia().vec() };
   }

   Trivia getTrailingTrivia() const
   {
      return Trivia { getRaw()->getTrailingTrivia().vec() };
   }

   TokenSyntax withLeadingTrivia(const Trivia &trivia) const
   {
      auto newRaw = getRaw()->withLeadingTrivia(trivia.pieces);
      return m_data->replaceSelf<TokenSyntax>(newRaw);
   }

   TokenSyntax withTrailingTrivia(const Trivia &trivia) const
   {
      auto newRaw = getRaw()->withTrailingTrivia(trivia.pieces);
      return m_data->replaceSelf<TokenSyntax>(newRaw);
   }

   bool isKeyword() const
   {
      return is_keyword_token(getTokenKind());
   }

   bool isDeclKeyword() const
   {
      return is_decl_keyword_token(getTokenKind());
   }

   bool isExprKeyword() const
   {
      return is_expr_keyword_token(getTokenKind());
   }

   bool isStmtKeyword() const
   {
      return is_stmt_keyword_token(getTokenKind());
   }

   bool isPunctuation() const
   {
      return is_punctuator_token(getTokenKind());
   }

   bool isMissing() const
   {
      return getRaw()->isMissing();
   }

   TokenKindType getTokenKind() const
   {
      return getRaw()->getTokenKind();
   }

   StringRef getText() const
   {
      return getRaw()->getTokenText();
   }

   static bool kindOf(SyntaxKind kind)
   {
      return is_token_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
protected:
   void validate() const
   {
      assert(getRaw()->isToken());
   }
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_TOKENSYNTAX_H
