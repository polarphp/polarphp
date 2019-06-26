// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/26.

#ifndef POLARPHP_SYNTAX_INTERNAL_LEXER_DEFS_H
#define POLARPHP_SYNTAX_INTERNAL_LEXER_DEFS_H

#include "polarphp/parser/internal/YYLexerConditionDefs.h"

/// for utf8 encoding
#define YYCTYPE   unsigned char
#define YYGETCONDITION() lexer->getCondState()
#define YYSETCONDITION(state) lexer->setCondState(state)
#define COND_NAME(name) yyc##name

#endif // POLARPHP_SYNTAX_INTERNAL_LEXER_DEFS_H
