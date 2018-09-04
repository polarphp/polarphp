// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/04.

#include <gtest/gtest.h>
#include "BooleanExpression.h"

namespace {

using namespace polar::lit;

TEST(BooleanExpressionTest, testVariables)
{
   std::set<std::string> variables = {
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

} // anonymous namespace

