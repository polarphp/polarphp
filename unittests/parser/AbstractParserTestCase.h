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
#include "polarphp/syntax/Syntax.h"
#include "llvm/Support/MemoryBuffer.h"

namespace polar::unittest {

using polar::kernel::LangOptions;
using polar::syntax::TriviaKind;
using polar::syntax::TokenKindType;
using polar::syntax::RefCountPtr;
using polar::syntax::Syntax;
using polar::parser::SourceManager;
using polar::parser::SourceLoc;
using polar::parser::Lexer;
using polar::parser::Token;
using polar::basic::ArrayRef;
using polar::parser::ParsedTrivia;
using polar::parser::TriviaRetentionMode;
using polar::parser::CommentRetentionMode;
using llvm::StringRef;

class AbstractParserTestCase : public ::testing::Test
{
public:
   RefCountPtr<RawSyntax> parseSource(StringRef source);
   virtual ~AbstractParserTestCase();
private:
   LangOptions m_langOpts;
   SourceManager m_sourceMgr;
};

} // polar::uittest

#endif // UNITTEST_PARSER_ABSTRACT_PARSER_TESTCASE_H
