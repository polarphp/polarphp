// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/30.

#ifndef POLARPHP_PARSER_CONFUSABLES_H
#define POLARPHP_PARSER_CONFUSABLES_H

#include <stdint.h>

namespace polar::parser {

/// Given a UTF-8 codepoint, determines whether it appears on the Unicode
/// specification table of confusable characters and maps to punctuation,
/// and either returns either the expected ASCII character or 0.
char try_convert_confusable_character_to_ascii(uint32_t codepoint);

} // polar::parser

#endif // POLARPHP_PARSER_CONFUSABLES_H
