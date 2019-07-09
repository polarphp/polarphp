// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/14.

#include "gtest/gtest.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/parser/SourceMgr.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/ast/DiagnosticConsumer.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/utils/MemoryBuffer.h"

#include <thread>

#if __has_include(<sys/mman.h>)
# include <sys/mman.h>
# define HAS_MMAP 1
#else
# define HAS_MMAP 0
#endif

using polar::kernel::LangOptions;
using polar::syntax::TriviaKind;
using polar::syntax::TokenKindType;
using polar::parser::SourceManager;
using polar::parser::SourceLoc;
using polar::parser::Lexer;
using polar::parser::tokenize;
using polar::parser::Token;
using polar::basic::StringRef;
using polar::basic::ArrayRef;

// The test fixture.
class LexerTest : public ::testing::Test
{
public:
   std::vector<Token> tokenizeAndKeepEOF(unsigned bufferId)
   {
      Lexer lexer(langOpts, sourceMgr, bufferId, /*Diags=*/nullptr);
      std::vector<Token> tokens;
      do {
         tokens.emplace_back();
         lexer.lex(tokens.back());
      } while (tokens.back().isNot(TokenKindType::END));
      return tokens;
   }

   std::vector<Token> checkLex(StringRef source,
                               ArrayRef<TokenKindType> expectedTokens,
                               bool keepComments = false,
                               bool keepEOF = false)
   {
      unsigned bufId = sourceMgr.addMemBufferCopy(source);
      std::vector<Token> tokens;
      if (keepEOF) {
         tokens = tokenizeAndKeepEOF(bufId);
      } else {
         tokens = tokenize(langOpts, sourceMgr, bufId, 0, 0, /*Diags=*/nullptr, keepComments);
      }
      EXPECT_EQ(expectedTokens.size(), tokens.size());
      for (unsigned i = 0, e = expectedTokens.size(); i != e; ++i) {
         EXPECT_EQ(expectedTokens[i], tokens[i].getKind()) << "i = " << i;
      }
      return tokens;
   }

   SourceLoc getLocForEndOfToken(SourceLoc loc)
   {
      return Lexer::getLocForEndOfToken(sourceMgr, loc);
   }

   LangOptions langOpts;
   SourceManager sourceMgr;
};

TEST_F(LexerTest, testSimpleToken)
{
   const char *source = "+-*/%{}->";
   std::vector<TokenKindType> expectedTokens{
      TokenKindType::T_PLUS_SIGN,
            TokenKindType::T_MINUS_SIGN,
            TokenKindType::T_MUL_SIGN,
            TokenKindType::T_DIV_SIGN,
            TokenKindType::T_MOD_SIGN,
            TokenKindType::T_LEFT_BRACE,
            TokenKindType::T_RIGHT_BRACE,
            TokenKindType::T_OBJECT_OPERATOR
   };
   checkLex(source, expectedTokens, /*KeepComments=*/false);
}

TEST_F(LexerTest, testSimpleKeyword)
{
   const char *source = R"(
         true false this self static parent for while foreach
if else elseif include namespace use
         include_once static:: require thread_local
         yield __halt_compiler parent module package
         yield from await (double) new null async

)";
   std::vector<TokenKindType> expectedTokens{
      TokenKindType::T_TRUE, TokenKindType::T_FALSE,
            TokenKindType::T_OBJ_REF, TokenKindType::T_CLASS_REF_SELF,
            TokenKindType::T_STATIC, TokenKindType::T_CLASS_REF_PARENT,
            TokenKindType::T_FOR, TokenKindType::T_WHILE,
            TokenKindType::T_FOREACH, TokenKindType::T_IF,
            TokenKindType::T_ELSE, TokenKindType::T_ELSEIF,
            TokenKindType::T_INCLUDE, TokenKindType::T_NAMESPACE,
            TokenKindType::T_USE, TokenKindType::T_INCLUDE_ONCE,
            TokenKindType::T_CLASS_REF_STATIC, TokenKindType::T_PAAMAYIM_NEKUDOTAYIM,
            TokenKindType::T_REQUIRE, TokenKindType::T_THREAD_LOCAL,
            TokenKindType::T_YIELD, TokenKindType::T_HALT_COMPILER,
            TokenKindType::T_CLASS_REF_PARENT, TokenKindType::T_MODULE,
            TokenKindType::T_PACKAGE, TokenKindType::T_YIELD_FROM,
            TokenKindType::T_AWAIT, TokenKindType::T_DOUBLE_CAST,
            TokenKindType::T_NEW, TokenKindType::T_NULL,
            TokenKindType::T_ASYNC
   };
   checkLex(source, expectedTokens, /*KeepComments=*/false);
}
