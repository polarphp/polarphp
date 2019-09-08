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
#define make_token_with_text(name, text) \
   TokenSyntaxNodeFactory::make##name(OwnedString::makeRefCounted(text), parser->getEmptyTrivia(), parser->getEmptyTrivia())
#define make_lnumber_token(value) TokenSyntaxNodeFactory::makeLNumber(value, parser->getEmptyTrivia(), parser->getEmptyTrivia())
#define make_dnumber_token(value) TokenSyntaxNodeFactory::makeDNumber(value, parser->getEmptyTrivia(), parser->getEmptyTrivia())

#define make_decl(name, ...) DeclSyntaxNodeFactory::make##name(__VA_ARGS__)
#define make_blank_decl(name) DeclSyntaxNodeFactory::makeBlank##name()
#define make_expr(name, ...) ExprSyntaxNodeFactory::make##name(__VA_ARGS__)
#define make_blank_expr(name) ExprSyntaxNodeFactory::makeBlank##name()
#define make_stmt(name, ...) StmtSyntaxNodeFactory::make##name(__VA_ARGS__)
#define make_blank_stmt(name) StmtSyntaxNodeFactory::makeBlank##name()

#endif // POLARPHP_PARSER_INTERNAL_YYPARSER_EXTRAS_DEFS_H
