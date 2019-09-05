// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/09/05.

#ifndef POLARPHP_PARSER_INTERNAL_YYPARSER_EXTRAS_DEFS_H
#define POLARPHP_PARSER_INTERNAL_YYPARSER_EXTRAS_DEFS_H

#define empty_triva() parser->getEmptyTrivia()
#define make_token(name) TokenSyntaxNodeFactory::make##name(parser->getEmptyTrivia(), parser->getEmptyTrivia())
#define make_token_with_text(name, text) TokenSyntaxNodeFactory::make##name(text, parser->getEmptyTrivia(), parser->getEmptyTrivia())

#endif // POLARPHP_PARSER_INTERNAL_YYPARSER_EXTRAS_DEFS_H
