// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#ifndef POLAR_SYNTAX_TOKEN_KINDS_H
#define POLAR_SYNTAX_TOKEN_KINDS_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/syntax/internal/YYParserDefs.h"
#include <map>

namespace polar::utils {
class RawOutStream;
}

namespace polar::syntax {

using polar::basic::StringRef;
using polar::utils::RawOutStream;
using TokenDescItemType = const std::tuple<const std::string, const std::string, const std::string>;
using TokenDescMap = const std::map<TokenKindType, TokenDescItemType>;

/// Check whether a token kind is known to have any specific text content.
/// e.g., tol::l_paren has determined text however tok::identifier doesn't.
bool is_token_text_determined(TokenKindType kind);
/// If a token kind has determined text, return the text; otherwise assert.
StringRef get_token_text(TokenKindType kind);
void dump_token_kind(RawOutStream &outStream, TokenKindType kind);
TokenDescItemType retrieve_token_desc_entry(TokenKindType kind);
TokenDescMap::const_iterator find_token_desc_entry(TokenKindType kind);
TokenDescMap::const_iterator token_desc_map_end();

} // polar::syntax

#endif // POLAR_SYNTAX_TOKEN_KINDS_H
