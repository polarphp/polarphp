// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/06.

#ifndef POLARPHP_SYNTAX_INTERNAL_LEXER_BRIDGE_H
#define POLARPHP_SYNTAX_INTERNAL_LEXER_BRIDGE_H

#include "polarphp/syntax/internal/YYLocation.h"
#include "polarphp/syntax/internal/YYParserDefs.h"

namespace polar::syntax::internal {

int token_lex(std::any a, std::any b);

} // polar::syntax::internal

#endif // POLARPHP_SYNTAX_INTERNAL_LEXER_BRIDGE_H
