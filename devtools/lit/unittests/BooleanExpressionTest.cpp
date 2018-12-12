// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/04.

#include <gtest/gtest.h>
#include "BooleanExpression.h"
#include "LitGlobal.h"
#include <boost/regex.hpp>

#define ASSERT_EXCEPTION( TRY_BLOCK, EXCEPTION_TYPE, MESSAGE )        \
   try                                                                   \
{                                                                     \
   TRY_BLOCK                                                         \
   FAIL() << "exception '" << MESSAGE << "' not thrown at all!";     \
   }                                                                     \
   catch( const EXCEPTION_TYPE& e )                                      \
{                                                                     \
   ASSERT_STREQ( MESSAGE, e.what() )                                    \
   << " exception message is incorrect. Expected the following " \
   "message:\n\n"                                             \
   << MESSAGE << "\n";                                           \
   }                                                                     \
   catch(...)                                                          \
{                                                                     \
   FAIL() << "exception '" << MESSAGE                                \
   << "' not thrown with expected type '" << #EXCEPTION_TYPE  \
   << "'!";                                                   \
   }

namespace {

using namespace polar::lit;

TEST(BooleanExpressionTest, testVariables)
{
   std::vector<std::string> variables = {
      "its-true", "false-lol-true", "under_score",
      "e=quals", "d1g1ts"};
   ASSERT_TRUE(BooleanExpression::evaluate("true", variables).value());
   ASSERT_TRUE(BooleanExpression::evaluate("its-true", variables).value());
   ASSERT_TRUE(BooleanExpression::evaluate("false-lol-true", variables).value());
   ASSERT_TRUE(BooleanExpression::evaluate("under_score", variables).value());
   ASSERT_TRUE(BooleanExpression::evaluate("e=quals", variables).value());
   ASSERT_TRUE(BooleanExpression::evaluate("d1g1ts", variables).value());

   ASSERT_FALSE(BooleanExpression::evaluate("false", variables).value());
   ASSERT_FALSE(BooleanExpression::evaluate("True", variables).value());
   ASSERT_FALSE(BooleanExpression::evaluate("true-ish", variables).value());
   ASSERT_FALSE(BooleanExpression::evaluate("not_true", variables).value());
   ASSERT_FALSE(BooleanExpression::evaluate("tru", variables).value());
}

TEST(BooleanExpressionTest, testTriple)
{
   std::string triple = "arch-vendor-os";
   ASSERT_TRUE(BooleanExpression::evaluate("arch-", {}, triple).value());
   ASSERT_TRUE(BooleanExpression::evaluate("ar", {}, triple).value());
   ASSERT_TRUE(BooleanExpression::evaluate("ch-vend", {}, triple).value());
   ASSERT_TRUE(BooleanExpression::evaluate("-vendor-", {}, triple).value());
   ASSERT_TRUE(BooleanExpression::evaluate("-os", {}, triple).value());
   ASSERT_FALSE(BooleanExpression::evaluate("arch-os", {}, triple).value());
}

TEST(BooleanExpressionTest, testOperators)
{
   ASSERT_TRUE(BooleanExpression::evaluate("true || true", {}).value());
   ASSERT_TRUE(BooleanExpression::evaluate("true || false", {}).value());
   ASSERT_TRUE(BooleanExpression::evaluate("false || true", {}).value());
   ASSERT_FALSE(BooleanExpression::evaluate("false || false", {}).value());

   ASSERT_TRUE(BooleanExpression::evaluate("true && true", {}).value());
   ASSERT_FALSE(BooleanExpression::evaluate("true && false", {}).value());
   ASSERT_FALSE(BooleanExpression::evaluate("false && true", {}).value());
   ASSERT_FALSE(BooleanExpression::evaluate("false && false", {}).value());

   ASSERT_FALSE(BooleanExpression::evaluate("!true", {}).value());
   ASSERT_TRUE(BooleanExpression::evaluate("!false", {}).value());

   ASSERT_TRUE(BooleanExpression::evaluate("   ((!((false) ))   ) ", {}).value());
   ASSERT_TRUE(BooleanExpression::evaluate("true && (true && (true))", {}).value());
   ASSERT_TRUE(BooleanExpression::evaluate("!false && !false && !! !false", {}).value());
   ASSERT_TRUE(BooleanExpression::evaluate("false && false || true", {}).value());
   ASSERT_TRUE(BooleanExpression::evaluate("(false && false) || true", {}).value());
   ASSERT_FALSE(BooleanExpression::evaluate("false && (false || true)", {}).value());
}

TEST(BooleanExpressionTest, testErrors)
{
   ASSERT_EXCEPTION({BooleanExpression::evaluate("ba#d", {});}, ValueError, "couldn't parse text: '#d'\nin expression: 'ba#d'");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("true and true", {});},ValueError, "expected: <end of expression>\n"
                                                                                    "have: 'and'\n"
                                                                                    "in expression: 'true and true'");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("|| true", {});},ValueError, "expected: '!' or '(' or identifier\n"
                                                                              "have: '||'\n"
                                                                              "in expression: '|| true'");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("true &&", {});},ValueError, "expected: '!' or '(' or identifier\n"
                                                                              "have: <end of expression>\n"
                                                                              "in expression: 'true &&'");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("", {});},ValueError, "expected: '!' or '(' or identifier\n"
                                                                       "have: <end of expression>\n"
                                                                       "in expression: ''");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("*", {});},ValueError, "couldn't parse text: '*'\n"
                                                                        "in expression: '*'");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("no wait stop", {});},ValueError,"expected: <end of expression>\n"
                                                                                  "have: 'wait'\n"
                                                                                  "in expression: 'no wait stop'");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("no-$-please", {});},ValueError,"couldn't parse text: '$-please'\n"
                                                                                 "in expression: 'no-$-please'");
   ASSERT_EXCEPTION({BooleanExpression::evaluate("(((true && true) || true)", {});},
                    ValueError,"expected: ')'\n"
                               "have: <end of expression>\n"
                               "in expression: '(((true && true) || true)'");

   ASSERT_EXCEPTION({BooleanExpression::evaluate("true (true)", {});},
                    ValueError, "expected: <end of expression>\n"
                                "have: '('\n"
                                "in expression: 'true (true)'");

   ASSERT_EXCEPTION({BooleanExpression::evaluate("( )", {});},
                    ValueError, "expected: '!' or '(' or identifier\n"
                                "have: ')'\n"
                                "in expression: '( )'");

}

} // anonymous namespace

