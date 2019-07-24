// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/07/24.

#ifndef UNITTEST_PARSER_ABSTRACT_PARSER_TESTCASE_H
#define UNITTEST_PARSER_ABSTRACT_PARSER_TESTCASE_H

#include "gtest/gtest.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/parser/SourceMgr.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/parser/Token.h"
#include "polarphp/ast/DiagnosticConsumer.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/utils/MemoryBuffer.h"

namespace polar::unittest {

class AbstractParserTestCase : public ::testing::Test
{

};

} // polar::uittest

#endif // UNITTEST_PARSER_ABSTRACT_PARSER_TESTCASE_H
