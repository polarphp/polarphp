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
#include "polarphp/basic/SourceMgr.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/parser/Token.h"
#include "polarphp/ast/DiagnosticConsumer.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "llvm/Support/MemoryBuffer.h"

#include <iostream>
#include <vector>
#include <cstdlib>

#if __has_include(<sys/mman.h>)
# include <sys/mman.h>
# define HAS_MMAP 1
#else
# define HAS_MMAP 0
#endif

using polar::kernel::LangOptions;
using polar::syntax::TriviaKind;
using polar::syntax::TokenKindType;
using polar::basic::SourceManager;
using polar::basic::SourceLoc;
using polar::parser::Lexer;
using polar::parser::tokenize;
using polar::parser::Token;
using llvm::StringRef;
using llvm::ArrayRef;

using polar::parser::ParsedTrivia;
using polar::parser::TriviaRetentionMode;
using polar::parser::CommentRetentionMode;

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

   std::vector<Token> tokenizeWithLexer(const LangOptions &langOpts,
                                        const SourceManager &sourceMgr, unsigned bufferId,
                                        bool keepComments)
   {
      std::vector<Token> tokens;
      tokenize(langOpts, sourceMgr, bufferId, 0, 0,
               nullptr,
               (keepComments ? CommentRetentionMode::ReturnAsTokens
                             : CommentRetentionMode::AttachToNextToken),
               TriviaRetentionMode::WithoutTrivia,
               [&](Lexer &lexer, const Token &token, const ParsedTrivia &leadingTrivia,
               const ParsedTrivia &trailingTrivia) mutable
      {
         tokens.push_back(token);
      }, [&](Lexer &lexer) {
         lexer.setCheckHeredocIndentation(true);
         lexer.registerLexicalExceptionHandler([&](StringRef msg, int code){
            m_exceptionMsgs.push_back(msg.str());
         });
      });
      assert(tokens.back().is(TokenKindType::END));
      tokens.pop_back(); // Remove EOF.
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
         tokens = tokenizeWithLexer(langOpts, sourceMgr, bufId, keepComments);
      }
      EXPECT_EQ(expectedTokens.size(), tokens.size());
      for (unsigned i = 0, e = expectedTokens.size(); i != e; ++i) {
         EXPECT_EQ(expectedTokens[i], tokens[i].getKind()) << "i = " << i;
      }
      return tokens;
   }

   void dumpTokens(const std::vector<Token> tokens) const
   {
      for (auto &token : tokens) {
         token.dump();
      }
   }

   SourceLoc getLocForEndOfToken(SourceLoc loc)
   {
      return Lexer::getLocForEndOfToken(sourceMgr, loc);
   }

   void SetUp()
   {
      m_exceptionMsgs.clear();
   }

   LangOptions langOpts;
   SourceManager sourceMgr;
   std::vector<std::string> m_exceptionMsgs;
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
   const char *source =
         R"(
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

TEST_F(LexerTest, testSimpleOperatorTokens)
{
   const char *source =
         R"(
         ; : , . [ ] ( ) | ^ & + - / * = % ! ~ $ < > ? @ \
         )";
   std::vector<TokenKindType> expectedTokens{
      TokenKindType::T_SEMICOLON, TokenKindType::T_COLON,
            TokenKindType::T_COMMA, TokenKindType::T_STR_CONCAT,
            TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_RIGHT_SQUARE_BRACKET,
            TokenKindType::T_LEFT_PAREN, TokenKindType::T_RIGHT_PAREN,
            TokenKindType::T_VBAR, TokenKindType::T_CARET,
            TokenKindType::T_AMPERSAND, TokenKindType::T_PLUS_SIGN,
            TokenKindType::T_MINUS_SIGN, TokenKindType::T_DIV_SIGN,
            TokenKindType::T_MUL_SIGN, TokenKindType::T_EQUAL,
            TokenKindType::T_MOD_SIGN, TokenKindType::T_EXCLAMATION_MARK,
            TokenKindType::T_TILDE, TokenKindType::T_DOLLAR_SIGN,
            TokenKindType::T_LEFT_ANGLE, TokenKindType::T_RIGHT_ANGLE,
            TokenKindType::T_QUESTION_MARK, TokenKindType::T_ERROR_SUPPRESS_SIGN,
            TokenKindType::T_NS_SEPARATOR
   };
   checkLex(source, expectedTokens, /*KeepComments=*/false);
}

TEST_F(LexerTest, testCompoundOperatorTokens)
{
   const char *source =
         R"(
         => ++ -- === !== != <> <=> <= >= += -=
         *= ** /= .= %= <<= >>= &= ^= ??= && || << >>
         -> :: ?? ...
         )";
   std::vector<TokenKindType> expectedTokens{
      TokenKindType::T_DOUBLE_ARROW, TokenKindType::T_INC,
            TokenKindType::T_DEC, TokenKindType::T_IS_IDENTICAL,
            TokenKindType::T_IS_NOT_IDENTICAL, TokenKindType::T_IS_NOT_EQUAL,
            TokenKindType::T_IS_NOT_EQUAL,TokenKindType::T_SPACESHIP,
            TokenKindType::T_IS_SMALLER_OR_EQUAL, TokenKindType::T_IS_GREATER_OR_EQUAL,
            TokenKindType::T_PLUS_EQUAL, TokenKindType::T_MINUS_EQUAL,
            TokenKindType::T_MUL_EQUAL, TokenKindType::T_POW,
            TokenKindType::T_DIV_EQUAL, TokenKindType::T_STR_CONCAT_EQUAL,
            TokenKindType::T_MOD_EQUAL, TokenKindType::T_SL_EQUAL,
            TokenKindType::T_SR_EQUAL, TokenKindType::T_AND_EQUAL,
            TokenKindType::T_XOR_EQUAL, TokenKindType::T_COALESCE_EQUAL,
            TokenKindType::T_BOOLEAN_AND, TokenKindType::T_BOOLEAN_OR,
            TokenKindType::T_SL, TokenKindType::T_SR,
            TokenKindType::T_OBJECT_OPERATOR, TokenKindType::T_PAAMAYIM_NEKUDOTAYIM,
            TokenKindType::T_COALESCE, TokenKindType::T_ELLIPSIS,
   };
   checkLex(source, expectedTokens, /*KeepComments=*/false);
}

TEST_F(LexerTest, testPreDefineLiteralTokens)
{
   const char *source =
         R"(
         __CLASS__ __TRAIT__ __FUNCTION__ __METHOD__ __LINE__ __FILE__ __DIR__
         __NAMESPACE__
         )";
   std::vector<TokenKindType> expectedTokens{
      TokenKindType::T_CLASS_CONST, TokenKindType::T_TRAIT_CONST,
            TokenKindType::T_FUNC_CONST, TokenKindType::T_METHOD_CONST,
            TokenKindType::T_LINE, TokenKindType::T_FILE,
            TokenKindType::T_DIR, TokenKindType::T_NS_CONST,
   };
   checkLex(source, expectedTokens, /*KeepComments=*/false);
}

TEST_F(LexerTest, testSingleQuoteStr)
{
   {
      const char *source =
            R"(
            'polarphp is very good'
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_CONSTANT_ENCAPSED_STRING
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token = tokens.at(0);
      ASSERT_EQ(token.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token.getValue<std::string>(), "polarphp is very good");
   }
   {
      /// test escape
      const char *source =
            R"(
            'polarphp \r\n \n \t is very good, version is $version, develop by \'Chinese coder\'. \\ hahaha'
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_CONSTANT_ENCAPSED_STRING
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token = tokens.at(0);
      ASSERT_EQ(token.getValueType(), Token::ValueType::String);
      std::string expectStr = R"(polarphp \r\n \n \t is very good, version is $version, develop by 'Chinese coder'. \ hahaha)";
      ASSERT_EQ(token.getValue<std::string>(), expectStr);
   }
   {
      /// test unclose string
      const char *source =
            R"(
            'polarphp \r\n \n \t is very good,
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_ENCAPSED_AND_WHITESPACE
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token = tokens.at(0);
      ASSERT_EQ(token.getValueType(), Token::ValueType::Unknown);
      ASSERT_FALSE(token.hasValue());
   }
}

TEST_F(LexerTest, testLexLabelString)
{
   {
      const char *source =
            R"(
            RestartLabel:
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_IDENTIFIER_STRING, TokenKindType::T_COLON
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token = tokens.at(0);
      ASSERT_EQ(token.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token.getValue<std::string>(), "RestartLabel");
   }

   {
      const char *source =
            R"(
            ->someLabel
            "${name} ${arr[2]}"
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_OBJECT_OPERATOR, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES,
               TokenKindType::T_STRING_VARNAME, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_LNUMBER,
               TokenKindType::T_RIGHT_SQUARE_BRACKET, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_DOUBLE_QUOTE
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      {
         Token token = tokens.at(1);
         ASSERT_EQ(token.getValueType(), Token::ValueType::String);
         ASSERT_EQ(token.getValue<std::string>(), "someLabel");
      }
      {
         Token token = tokens.at(4);
         ASSERT_EQ(token.getValueType(), Token::ValueType::String);
         ASSERT_EQ(token.getValue<std::string>(), "name");
      }
      {
         Token token = tokens.at(7);
         ASSERT_EQ(token.getValueType(), Token::ValueType::String);
         ASSERT_EQ(token.getValue<std::string>(), "arr");
      }
   }
}

TEST_F(LexerTest, testLexLNumber)
{
   {
      const char *source =
            R"(
            2018 -2019
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_LNUMBER, TokenKindType::T_MINUS_SIGN,
               TokenKindType::T_LNUMBER,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      {
         Token token = tokens.at(0);
         ASSERT_EQ(token.getValueType(), Token::ValueType::LongLong);
         ASSERT_EQ(token.getValue<std::int64_t>(), 2018);
      }
      {
         Token token = tokens.at(2);
         ASSERT_EQ(token.getValueType(), Token::ValueType::LongLong);
         ASSERT_EQ(token.getValue<std::int64_t>(), 2019);
      }
   }
   {
      /// test max and min value
      const char *source =
            R"(
            -9223372036854775808
            9223372036854775808
            --9223372036854775808
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_MINUS_SIGN, TokenKindType::T_DNUMBER,
               TokenKindType::T_DNUMBER,TokenKindType::T_DEC,
               TokenKindType::T_DNUMBER
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(4);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::Double);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::Double);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::Double);
      ASSERT_TRUE(token1.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token2.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token3.isNeedCorrectLNumberOverflow());
   }
   {
      /// test octal number
      const char *source =
            R"(
            0777777777777777777777
            -01000000000000000000000
            01000000000000000000000
            --01000000000000000000000
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_LNUMBER, TokenKindType::T_MINUS_SIGN,
               TokenKindType::T_DNUMBER,TokenKindType::T_DNUMBER,
               TokenKindType::T_DEC, TokenKindType::T_DNUMBER
      };

      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(0);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);
      Token token4 = tokens.at(5);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::Double);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::Double);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::Double);

      ASSERT_EQ(token1.getValue<std::int64_t>(), std::numeric_limits<std::int64_t>::max());
      ASSERT_TRUE(token2.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token3.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token4.isNeedCorrectLNumberOverflow());
   }
   {
      /// multi prefix '0' chars
      /// test octal number
      const char *source =
            R"(
            0000000007
            00000000000777777777777777777777
            00000000000000000000000000000000
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_LNUMBER, TokenKindType::T_LNUMBER,
               TokenKindType::T_LNUMBER,
      };

      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(0);
      Token token2 = tokens.at(1);
      Token token3 = tokens.at(2);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::LongLong);
      ASSERT_FALSE(token1.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token2.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token3.isNeedCorrectLNumberOverflow());
      ASSERT_EQ(token1.getValue<std::int64_t>(), 7);
      ASSERT_EQ(token2.getValue<std::int64_t>(), std::numeric_limits<std::int64_t>::max());
      ASSERT_EQ(token3.getValue<std::int64_t>(), 0);
   }
   {
      /// test illform octal number
      const char *source =
            R"(
            08123
            0071239
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_LNUMBER, TokenKindType::T_LNUMBER,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(0);
      Token token2 = tokens.at(1);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::Unknown);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::Unknown);
      ASSERT_TRUE(token1.isInvalidLexValue());
      ASSERT_TRUE(token2.isInvalidLexValue());
      std::vector<std::string> expectExceptionMsgs{
         "Invalid numeric literal",
         "Invalid numeric literal"
      };
      ASSERT_EQ(m_exceptionMsgs, expectExceptionMsgs);
   }
}

TEST_F(LexerTest, testLexHexNumber)
{
   {
      const char *source =
            R"(
            0x10
            -0xaf2
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_LNUMBER, TokenKindType::T_MINUS_SIGN,
               TokenKindType::T_LNUMBER,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(0);
      Token token2 = tokens.at(2);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token1.getValue<std::int64_t>(), 16);
      ASSERT_EQ(token2.getValue<std::int64_t>(), 2802);
      ASSERT_FALSE(token1.isInvalidLexValue());
      ASSERT_FALSE(token2.isInvalidLexValue());
   }
   {
      /// test multi prefix '0' chars
      const char *source =
            R"(
            0x010
            0x00000000000000000000000000000000000001
            0x0000000000000000000000000000000000000
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_LNUMBER, TokenKindType::T_LNUMBER,
               TokenKindType::T_LNUMBER,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(0);
      Token token2 = tokens.at(1);
      Token token3 = tokens.at(2);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token1.getValue<std::int64_t>(), 16);
      ASSERT_EQ(token2.getValue<std::int64_t>(), 1);
      ASSERT_EQ(token3.getValue<std::int64_t>(), 0);
      ASSERT_FALSE(token1.isInvalidLexValue());
      ASSERT_FALSE(token2.isInvalidLexValue());
      ASSERT_FALSE(token3.isInvalidLexValue());
   }
   {
      /// test overflow
      /// max:  7fffffffffffffff
      /// min: -8000000000000000
      const char *source =
            R"(
            0x7fffffffffffffff
            0x8000000000000000
            -0x8000000000000000
            --0x8000000000000000
            )";
      std::vector<TokenKindType> expectedTokens{
         TokenKindType::T_LNUMBER, TokenKindType::T_DNUMBER,
               TokenKindType::T_MINUS_SIGN, TokenKindType::T_DNUMBER,
               TokenKindType::T_DEC, TokenKindType::T_DNUMBER,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(0);
      Token token2 = tokens.at(1);
      Token token3 = tokens.at(3);
      Token token4 = tokens.at(5);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::Double);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::Double);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::Double);
      ASSERT_FALSE(token1.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token2.isNeedCorrectLNumberOverflow());
      ASSERT_TRUE(token3.isNeedCorrectLNumberOverflow());
      ASSERT_FALSE(token4.isNeedCorrectLNumberOverflow());
   }
}

TEST_F(LexerTest, testLexDNumber)
{
   const char *source =
         R"(
         0.0
         1.2e2
         3.2e-2
         2E2
         1.79769e+309
         )";
   std::vector<TokenKindType> expectedTokens{
      TokenKindType::T_DNUMBER, TokenKindType::T_DNUMBER,
            TokenKindType::T_DNUMBER, TokenKindType::T_DNUMBER,
            TokenKindType::T_DNUMBER,
   };
   std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
   Token token1 = tokens.at(0);
   Token token2 = tokens.at(1);
   Token token3 = tokens.at(2);
   Token token4 = tokens.at(3);
   Token token5 = tokens.at(4);
   ASSERT_EQ(token1.getValueType(), Token::ValueType::Double);
   ASSERT_EQ(token2.getValueType(), Token::ValueType::Double);
   ASSERT_EQ(token3.getValueType(), Token::ValueType::Double);
   ASSERT_EQ(token4.getValueType(), Token::ValueType::Double);
   ASSERT_EQ(token5.getValueType(), Token::ValueType::Double);
   ASSERT_DOUBLE_EQ(token1.getValue<double>(), 0);
   ASSERT_DOUBLE_EQ(token2.getValue<double>(), 1.2e2);
   ASSERT_DOUBLE_EQ(token3.getValue<double>(), 3.2e-2);
   ASSERT_DOUBLE_EQ(token4.getValue<double>(), 2E2);
   ASSERT_EQ(token5.getValue<double>(), INFINITY);
}

TEST_F(LexerTest, testLexDoubleQuoteString)
{
   {
      /// test normal string
      const char *source =
            R"(
            ""
            "polarphp is developed by Chinese Ma Nong"
            "polarphp is
            develop by
            Chinese Ma Nong"
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_CONSTANT_ENCAPSED_STRING, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOUBLE_QUOTE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(4);
      Token token3 = tokens.at(7);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "");
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValue<std::string>(), "polarphp is developed by Chinese Ma Nong");
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      std::string expected =
            R"(polarphp is
            develop by
            Chinese Ma Nong)";
      ASSERT_EQ(token3.getValue<std::string>(), expected);
   }
   {
      /// test $varname
      const char *source =
            R"(
            "polarphp version: $version, very welcome."
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_VARIABLE,  TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOUBLE_QUOTE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "polarphp version: ");
      ASSERT_EQ(token2.getValue<std::string>(), "version");
      ASSERT_EQ(token3.getValue<std::string>(), ", very welcome.");
   }
   {
      const char *source =
            R"(
            "name is $info[123]"
            "name is $info->name."
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_VARIABLE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOUBLE_QUOTE,

               TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_VARIABLE, TokenKindType::T_OBJECT_OPERATOR,
               TokenKindType::T_IDENTIFIER_STRING, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOUBLE_QUOTE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);

      Token token4 = tokens.at(6);
      Token token5 = tokens.at(7);
      Token token6 = tokens.at(9);
      Token token7 = tokens.at(10);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "[123]");

      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token5.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token6.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token7.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValue<std::string>(), "name is ");
      ASSERT_EQ(token5.getValue<std::string>(), "info");
      ASSERT_EQ(token6.getValue<std::string>(), "name");
      ASSERT_EQ(token7.getValue<std::string>(), ".");
   }
   {
      /// test ${xxx}
      const char *source =
            R"(
            "name is ${info}."
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_RIGHT_BRACE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOUBLE_QUOTE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), ".");
   }
   {
      /// ${info[1]}
      const char *source =
            R"(
            "name is ${info[1]}."
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_LNUMBER,
               TokenKindType::T_RIGHT_SQUARE_BRACKET, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_CONSTANT_ENCAPSED_STRING, TokenKindType::T_DOUBLE_QUOTE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      Token token4 = tokens.at(8);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::int64_t>(), 1);
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }
   {
      /// ${info["name"]}
      const char *source =
            R"(
            "name is ${info["name"]}."
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_CONSTANT_ENCAPSED_STRING, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_RIGHT_SQUARE_BRACKET, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_CONSTANT_ENCAPSED_STRING, TokenKindType::T_DOUBLE_QUOTE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(6);
      Token token4 = tokens.at(10);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "name");
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }
   {
      /// "name is ${info->name}."
      /// lex stage is valid but at parse stage is invalid
      const char *source =
            R"(
            "name is ${info->name}."
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_OBJECT_OPERATOR, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_RIGHT_BRACE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_DOUBLE_QUOTE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      Token token4 = tokens.at(7);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "name");
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }
   {
      /// test unclosed string
      /// TODO
      const char *source =
            R"(
            "polarphp is very good

            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_DOUBLE_QUOTE, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      std::string expected =
            R"(polarphp is very good

            )";
      ASSERT_EQ(token1.getValue<std::string>(), expected);
   }
}

TEST_F(LexerTest, testLexBackquoteString)
{
   {
      /// test empty backquoteString
      const char *source =
            R"(
            ``
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      ASSERT_EQ(token1.getValue<std::string>(), "");
   }
   {
      /// test pure string
      const char *source =
            R"(
            `polarphp is developed by Chinese Ma Nong`
            `polarphp is
            develop by
            Chinese Ma Nong`
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_BACKTICK,
               TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(4);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
   }
   {
      /// test embeded $varname
      const char *source =
            R"(
            `polarphp version: $version, very welcome.`
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_VARIABLE,  TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "polarphp version: ");
      ASSERT_EQ(token2.getValue<std::string>(), "version");
      ASSERT_EQ(token3.getValue<std::string>(), ", very welcome.");
   }

   {
      const char *source =
            R"(
            `name is $info[123]`
            `name is $info->name.`
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_VARIABLE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_BACKTICK,

               TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_VARIABLE, TokenKindType::T_OBJECT_OPERATOR,
               TokenKindType::T_IDENTIFIER_STRING, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);

      Token token4 = tokens.at(6);
      Token token5 = tokens.at(7);
      Token token6 = tokens.at(9);
      Token token7 = tokens.at(10);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "[123]");

      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token5.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token6.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token7.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValue<std::string>(), "name is ");
      ASSERT_EQ(token5.getValue<std::string>(), "info");
      ASSERT_EQ(token6.getValue<std::string>(), "name");
      ASSERT_EQ(token7.getValue<std::string>(), ".");
   }

   {
      /// test ${xxx}
      const char *source =
            R"(
            `name is ${info}.`
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_RIGHT_BRACE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), ".");
   }
   {
      /// ${info[1]}
      const char *source =
            R"(
            `name is ${info[1]}.`
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_LNUMBER,
               TokenKindType::T_RIGHT_SQUARE_BRACKET, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_ENCAPSED_AND_WHITESPACE, TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      Token token4 = tokens.at(8);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::int64_t>(), 1);
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }
   {
      /// ${info["name"]}
      const char *source =
            R"(
            `name is ${info["name"]}.`
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_CONSTANT_ENCAPSED_STRING, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_RIGHT_SQUARE_BRACKET, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_ENCAPSED_AND_WHITESPACE, TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepCom980ments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(6);
      Token token4 = tokens.at(10);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "name");
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }
   {
      /// "name is ${info->name}."
      /// lex stage is valid but at parse stage is invalid
      const char *source =
            R"(
            `name is ${info->name}.`
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_OBJECT_OPERATOR, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_RIGHT_BRACE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_BACKTICK,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      Token token4 = tokens.at(7);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "name");
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }
   {
      /// test unclosed string
      /// TODO
      const char *source =
            R"(
            `polarphp is very good

            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_BACKTICK, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      std::string expected =
            R"(polarphp is very good

            )";
      ASSERT_EQ(token1.getValue<std::string>(), expected);
   }
}

TEST_F(LexerTest, testLexNowDoc)
{
   {
      /// test empty now doc
      const char *source =
            R"(
            <<<'POLARPHP'
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "");
   }
   {
      /// test normal nowdoc
      const char *source =
            R"(
            <<<'POLARPHP'
            polarphp is developed by Chinese Ma Nong
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      std::string expected = "polarphp is developed by Chinese Ma Nong";
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), expected);
   }

   {
      /// test escape chars
      const char *source =
            R"(
            <<<'POLARPHP'
            'polarphp \r\n \n \t is very good, version is $version,
            develop by \'Chinese coder\'. \\ hahaha'
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      std::string expected =
            R"('polarphp \r\n \n \t is very good, version is $version,
develop by \'Chinese coder\'. \\ hahaha')";
      Token token1 = tokens.at(1);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), expected);
   }

   {
      /// test indentation
      const char *source =
            R"(
            <<<'POLARPHP'
            'polarphp \r\n \n \t is very good, version is $version,
            develop by \'Chinese coder\'. \\ hahaha'
               POLARPHP;+'name';
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ERROR,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
               TokenKindType::T_PLUS_SIGN, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(5);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "Invalid body indentation level (expecting an indentation level of at least 15)");
      ASSERT_EQ(token2.getValue<std::string>(), "name");
   }
   {
      /// test unclosed nowdoc
      const char *source =
            R"(
            <<<'POLARPHP'
            'polarphp \r\n \n \t is very good, version is $version,
            develop by \'Chinese coder\'. \\ hahaha'

            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      std::string expected =
            R"(            'polarphp \r\n \n \t is very good, version is $version,
            develop by \'Chinese coder\'. \\ hahaha'

            )";
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), expected);
   }
}

TEST_F(LexerTest, testLexHereDoc)
{
   {
      /// test empty heredoc
      const char *source =
            R"(
            <<<POLARPHP
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "");
   }
   {
      /// test normal heredoc
      const char *source =
            R"(
            <<<POLARPHP
            polarphp is developed by Chinese Ma Nong
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      std::string expected = "polarphp is developed by Chinese Ma Nong";
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), expected);
   }

   {
      /// test escape chars
      const char *source =
            R"(
            <<<POLARPHP
            'polarphp \r\n \n \t is very good, version is $version,
            develop by \'Chinese coder\'. \\ hahaha'
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_VARIABLE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      std::string expected =
            "'polarphp \r\n \n \t is very good, version is ";
      ASSERT_EQ(token1.getValue<std::string>(), expected);
      ASSERT_EQ(token2.getValue<std::string>(), "version");
      expected = ",\ndevelop by \\'Chinese coder\\'. \\ hahaha'";
      ASSERT_EQ(token3.getValue<std::string>(), expected);
   }

   {
      /// test indentation
      const char *source =
            R"(
            <<<POLARPHP
            'polarphp \r\n \n \t is very good, version is $version,
            develop by \'Chinese coder\'. \\ hahaha'
               POLARPHP;+'name';
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ERROR,
               TokenKindType::T_VARIABLE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
               TokenKindType::T_PLUS_SIGN, TokenKindType::T_CONSTANT_ENCAPSED_STRING,
               TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);

      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);
      Token token4 = tokens.at(7);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "Invalid body indentation level (expecting an indentation level of at least 15)");
      ASSERT_EQ(token2.getValue<std::string>(), "version");
      ASSERT_EQ(token3.getValue<std::string>(), ",\n            develop by \\'Chinese coder\\'. \\\\ hahaha'");
      ASSERT_EQ(token4.getValue<std::string>(), "name");
   }

   {
      /// test $info[123] and $info->name
      const char *source =
            R"(
            <<<POLARPHP
            name is $info[123]
            name is $info->name.
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_VARIABLE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_VARIABLE, TokenKindType::T_OBJECT_OPERATOR,
               TokenKindType::T_IDENTIFIER_STRING, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);

      Token token1 = tokens.at(1);
      Token token2 = tokens.at(2);
      Token token3 = tokens.at(3);

      Token token4 = tokens.at(4);
      Token token5 = tokens.at(6);
      Token token6 = tokens.at(7);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      std::string expected = "[123]\nname is ";
      ASSERT_EQ(token3.getValue<std::string>(), expected);

      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token5.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token6.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValue<std::string>(), "info");
      ASSERT_EQ(token5.getValue<std::string>(), "name");
      ASSERT_EQ(token6.getValue<std::string>(), ".");
   }

   {
      /// name is ${info}.
      const char *source =
            R"(
            <<<POLARPHP
            name is ${info}.
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_RIGHT_BRACE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), ".");
   }

   {
      /// name is ${info[1]}.
      const char *source =
            R"(
            <<<POLARPHP
            name is ${info[1]}.
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_LNUMBER,
               TokenKindType::T_RIGHT_SQUARE_BRACKET, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_ENCAPSED_AND_WHITESPACE, TokenKindType::T_END_HEREDOC,
               TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      Token token4 = tokens.at(8);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::LongLong);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::int64_t>(), 1);
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }

   {
      /// name is ${info["name"]}.
      const char *source =
            R"(
            <<<POLARPHP
            name is ${info["name"]}.
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_STRING_VARNAME,
               TokenKindType::T_LEFT_SQUARE_BRACKET, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_CONSTANT_ENCAPSED_STRING, TokenKindType::T_DOUBLE_QUOTE,
               TokenKindType::T_RIGHT_SQUARE_BRACKET, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_ENCAPSED_AND_WHITESPACE, TokenKindType::T_END_HEREDOC,
               TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(6);
      Token token4 = tokens.at(10);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "name");
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }

   {
      /// name is ${info["name"]}.
      const char *source =
            R"(
            <<<POLARPHP
            name is ${info->name}.
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_OBJECT_OPERATOR, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_RIGHT_BRACE, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_END_HEREDOC, TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      Token token4 = tokens.at(7);
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), "name is ");
      ASSERT_EQ(token2.getValue<std::string>(), "info");
      ASSERT_EQ(token3.getValue<std::string>(), "name");
      ASSERT_EQ(token4.getValue<std::string>(), ".");
   }

   {
      /// test unclosed nowdoc
      const char *source =
            R"(
            <<<POLARPHP
            polarphp is very good,
            develop by Chinese Ma Nong

            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      std::string expected =
            R"(            polarphp is very good,
            develop by Chinese Ma Nong

            )";
      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token1.getValue<std::string>(), expected);
   }

   {
      /// test nested heredoc, note this only allowed at lex stage, not allowed at parse stage
      const char *source =
            R"(
            <<<POLARPHP
            polarphp version: ${version->name;<<<XXX
               some text here
               XXX;
            }
            POLARPHP;
            )";
      std::vector<TokenKindType> expectedTokens {
         TokenKindType::T_START_HEREDOC, TokenKindType::T_ENCAPSED_AND_WHITESPACE,
               TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_OBJECT_OPERATOR, TokenKindType::T_IDENTIFIER_STRING,
               TokenKindType::T_SEMICOLON, TokenKindType::T_START_HEREDOC,
               TokenKindType::T_ENCAPSED_AND_WHITESPACE, TokenKindType::T_END_HEREDOC,
               TokenKindType::T_SEMICOLON, TokenKindType::T_RIGHT_BRACE,
               TokenKindType::T_ENCAPSED_AND_WHITESPACE, TokenKindType::T_END_HEREDOC,
               TokenKindType::T_SEMICOLON,
      };
      std::vector<Token> tokens = checkLex(source, expectedTokens, /*KeepComments=*/false);
      Token token1 = tokens.at(1);
      Token token2 = tokens.at(3);
      Token token3 = tokens.at(5);
      Token token4 = tokens.at(8);
      Token token5 = tokens.at(12);

      ASSERT_EQ(token1.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token2.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token3.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token4.getValueType(), Token::ValueType::String);
      ASSERT_EQ(token5.getValueType(), Token::ValueType::String);

      ASSERT_EQ(token1.getValue<std::string>(), "polarphp version: ");
      ASSERT_EQ(token2.getValue<std::string>(), "version");
      ASSERT_EQ(token3.getValue<std::string>(), "name");
      ASSERT_EQ(token4.getValue<std::string>(), "some text here");
      ASSERT_EQ(token5.getValue<std::string>(), "");
   }
}
