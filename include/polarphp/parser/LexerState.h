// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/05.

#ifndef POLARPHP_PARSER_LEXER_STATE_H
#define POLARPHP_PARSER_LEXER_STATE_H

#include <optional>
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/ParsedTrivia.h"

namespace polar::parser {

class Lexer;

/// Lexer state can be saved/restored to/from objects of this class.

class LexerState
{
public:
   LexerState()
   {}

   bool isValid() const
   {
      return m_loc.isValid();
   }

   LexerState advance(unsigned offset) const
   {
      assert(isValid());
      return LexerState(m_loc.getAdvancedLoc(offset));
   }

private:
   explicit LexerState(SourceLoc loc)
      : m_loc(loc)
   {}

   SourceLoc m_loc;
   std::optional<ParsedTrivia> m_leadingTrivia;
   friend class Lexer;
};

} // polar::parser

#endif // POLARPHP_PARSER_LEXER_STATE_H
