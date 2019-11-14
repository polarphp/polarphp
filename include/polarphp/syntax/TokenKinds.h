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

#ifndef POLAR_SYNTAX_TOKEN_KINDS_H
#define POLAR_SYNTAX_TOKEN_KINDS_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/syntax/internal/TokenEnumDefs.h"
#include <map>

namespace polar::utils {
class RawOutStream;
} // polar::utils

namespace polar::syntax {

enum class TokenCategory
{
   Unknown,
   Internal,
   Keyword,
   DeclKeyword,
   StmtKeyword,
   ExprKeyword,
   Punctuator,
   Misc
};

using internal::TokenKindType;
using polar::basic::StringRef;
using polar::utils::RawOutStream;

/// Check whether a token kind is known to have any specific text content.
/// e.g., tol::l_paren has determined text however tok::identifier doesn't.
bool is_token_text_determined(TokenKindType kind);

/// If a token kind has determined text, return the text; otherwise assert.

StringRef get_token_text(TokenKindType kind);
StringRef get_token_kind_str(TokenKindType kind);
StringRef get_token_name(TokenKindType kind);
TokenCategory get_token_category(TokenKindType kind);

void dump_token_kind(RawOutStream &outStream, TokenKindType kind);
bool is_internal_token(TokenKindType kind);
bool is_keyword_token(TokenKindType kind);
bool is_decl_keyword_token(TokenKindType kind);
bool is_stmt_keyword_token(TokenKindType kind);
bool is_expr_keyword_token(TokenKindType kind);
bool is_punctuator_token(TokenKindType kind);
bool is_misc_token(TokenKindType kind);

} // polar::syntax

#endif // POLAR_SYNTAX_TOKEN_KINDS_H
