// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/07/06.

#ifndef POLARPHP_PARSER_LEXER_FLAGS_H
#define POLARPHP_PARSER_LEXER_FLAGS_H

#include "polarphp/basic/FlagSet.h"
#include <cstdint>

namespace polar::parser {

using polar::FlagSet;

class LexerFlags final : public FlagSet<std::uint16_t>
{
protected:
   enum {
      LexingBinaryString,
      HeredocScanAhead,
      IncrementLineNumber,
      HeredocIndentationUsesSpaces,
      ReserveHeredocSpaces,
      LexExceptionOccurred,
      CheckHeredocIndentation
   };
public:
   explicit LexerFlags(std::uint16_t bits)
      : FlagSet(bits)
   {}
   constexpr LexerFlags()
   {}

   FLAGSET_DEFINE_FLAG_ACCESSORS(LexingBinaryString, isLexingBinaryString, setLexingBinaryString)
   FLAGSET_DEFINE_FLAG_ACCESSORS(HeredocScanAhead, isHeredocScanAhead, setHeredocScanAhead)
   FLAGSET_DEFINE_FLAG_ACCESSORS(IncrementLineNumber, isIncrementLineNumber, setIncrementLineNumber)
   FLAGSET_DEFINE_FLAG_ACCESSORS(HeredocIndentationUsesSpaces, isHeredocIndentationUsesSpaces, setHeredocIndentationUsesSpaces)
   FLAGSET_DEFINE_FLAG_ACCESSORS(LexExceptionOccurred, isLexExceptionOccurred, setLexExceptionOccurred)
   FLAGSET_DEFINE_FLAG_ACCESSORS(ReserveHeredocSpaces, isReserveHeredocSpaces, setReserveHeredocSpaces)
   FLAGSET_DEFINE_FLAG_ACCESSORS(CheckHeredocIndentation, isCheckHeredocIndentation, setCheckHeredocIndentation)
   FLAGSET_DEFINE_EQUALITY(LexerFlags)
};

} // polar::parser

#endif // POLARPHP_PARSER_LEXER_FLAGS_H
