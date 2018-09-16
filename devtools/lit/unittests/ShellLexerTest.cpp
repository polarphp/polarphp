// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/16.

#include <gtest/gtest.h>
#include "ShellUtil.h"
#include <filesystem>

using polar::lit::ShLexer;
using polar::lit::ShellTokenType;

TEST(ShellLexerTest, testBasic)
{
   std::list<std::any> tokens = ShLexer("a|b>c&d<e;f").lex();
   ASSERT_EQ(tokens.size(), 11);
   std::list<std::any>::iterator iter = tokens.begin();
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "a");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "|");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "b");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), ">");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "c");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "&");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "d");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "<");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "e");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), ";");
      ASSERT_EQ(std::get<1>(token), -1);
   }
   ++iter;
   {
      std::any tokenAny = *iter;
      ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
      ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
      ASSERT_EQ(std::get<0>(token), "f");
      ASSERT_EQ(std::get<1>(token), -1);
   }
}

