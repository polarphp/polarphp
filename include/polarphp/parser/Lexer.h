// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#ifndef POLARPHP_PARSER_LEXER_H
#define POLARPHP_PARSER_LEXER_H

#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/SourceMgr.h"
#include "polarphp/parser/Token.h"
#include "polarphp/parser/ParsedTrivia.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/SaveAndRestore.h"

#define polar_lex polar::parser::token_lex

namespace polar::parser {

using polar::ast::DiagnosticEngine;
using polar::ast::InFlightDiagnostic;
using polar::ast::Diag;
using polar::basic::LangOptions;
union ParserStackElement;

int token_lex(ParserStackElement *element);

/// Given a pointer to the starting byte of a UTF8 character, validate it and
/// advance the lexer past it.  This returns the encoded character or ~0U if
/// the encoding is invalid.
///
uint32_t validate_utf8_character_and_advance(const char *&ptr, const char *end);

enum class CommentRetentionMode
{
   None,
   AttachToNextToken,
   ReturnAsTokens
};

enum class TriviaRetentionMode
{
   WithoutTrivia,
   WithTrivia
};

enum class HashbangMode : bool
{
   Disallowed,
   Allowed
};

} // polar::parser

#endif // POLARPHP_PARSER_LEXER_H
