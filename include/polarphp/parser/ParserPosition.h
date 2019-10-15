//===--- ParserPosition.h - Parser Position ---------------------*- C++ -*-===//
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
// Parser position where Parser can jump to.
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/15.

#ifndef POLARPHP_PARSER_PARSER_POSITION_H
#define POLARPHP_PARSER_PARSER_POSITION_H

#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/LexerState.h"

namespace polar::parser {

class ParserPosition
{
public:
   ParserPosition() = default;
   ParserPosition &operator=(const ParserPosition &) = default;

   bool isValid() const
   {
      return m_lexerState.isValid();
   }

private:
   ParserPosition(LexerState lexerState, SourceLoc previousLoc)
      : m_lexerState(lexerState),
        m_previousLoc(previousLoc) {}

   LexerState m_lexerState;
   SourceLoc m_previousLoc;
   friend class Parser;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSER_POSITION_H
