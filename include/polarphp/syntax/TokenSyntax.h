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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
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
protected:
   void validate() const
   {
      assert(getRaw()->isToken());
   }

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
      return Trivia { getRaw()->getLeadingTrivia().getVector() };
   }

   Trivia getTrailingTrivia() const
   {
      return Trivia { getRaw()->getTrailingTrivia().getVector() };
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

   /* TODO: If we really need them.
  bool isKeyword() const;

  bool isPunctuation() const;

  bool isOperator() const;

  bool isLiteral() const;
  */

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

   static bool kindof(SyntaxKind kind)
   {
      return is_token_kind(kind);
   }

   static bool classof(const Syntax *syntax)
   {
      return kindof(syntax->getKind());
   }
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_TOKENSYNTAX_H
