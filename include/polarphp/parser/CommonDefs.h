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

#ifndef POLARPHP_PARSER_COMMON_DEFS_H
#define POLARPHP_PARSER_COMMON_DEFS_H

#include <string>

namespace polar::parser {

class AstNode;

union ParserStackElement
{
   AstNode *ast;
   std::string *str;
   unsigned long num;
};

} // polar::parser

#endif // POLARPHP_PARSER_COMMON_DEFS_H
