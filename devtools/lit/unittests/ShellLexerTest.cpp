// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
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

TEST(ShellLexerTest, testRedirectionTokens)
{
   {
      std::list<std::any> tokens = ShLexer("a2>c").lex();
      ASSERT_EQ(tokens.size(), 3);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         std::any tokenAny = *iter;
         ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
         ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
         ASSERT_EQ(std::get<0>(token), "a2");
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
   }
   {
      std::list<std::any> tokens = ShLexer("a 2>c").lex();
      ASSERT_EQ(tokens.size(), 3);
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
         ASSERT_EQ(std::get<0>(token), ">");
         ASSERT_EQ(std::get<1>(token), 2);
      }
      ++iter;
      {
         std::any tokenAny = *iter;
         ASSERT_EQ(tokenAny.type(), typeid(ShellTokenType));
         ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
         ASSERT_EQ(std::get<0>(token), "c");
         ASSERT_EQ(std::get<1>(token), -1);
      }
   }
}

TEST(ShellLexerTest, testQuoting)
{
   {
      std::list<std::any> tokens = ShLexer(R"('a')").lex();
      ASSERT_EQ(tokens.size(), 1);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         std::any tokenAny = *iter;
         ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
         ASSERT_EQ(std::get<0>(token), "a");
         ASSERT_EQ(std::get<1>(token), -1);
      }
   }
   {
      std::list<std::any> tokens = ShLexer(R"("hello\"world")").lex();
      ASSERT_EQ(tokens.size(), 1);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         std::any tokenAny = *iter;
         ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
         ASSERT_EQ(std::get<0>(token), "hello\"world");
         ASSERT_EQ(std::get<1>(token), -1);
      }
   }
   {
      std::list<std::any> tokens = ShLexer(R"("hello\'world")").lex();
      ASSERT_EQ(tokens.size(), 1);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         std::any tokenAny = *iter;
         ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
         ASSERT_EQ(std::get<0>(token), "hello\\'world");
         ASSERT_EQ(std::get<1>(token), -1);
      }
   }
   {
      std::list<std::any> tokens = ShLexer(R"("hello\\\\world")").lex();
      ASSERT_EQ(tokens.size(), 1);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         std::any tokenAny = *iter;
         ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
         ASSERT_EQ(std::get<0>(token), "hello\\\\world");
         ASSERT_EQ(std::get<1>(token), -1);
      }
   }
   {
      std::list<std::any> tokens = ShLexer(R"(he"llo wo"rld)").lex();
      ASSERT_EQ(tokens.size(), 1);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         std::any tokenAny = *iter;
         ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
         ASSERT_EQ(std::get<0>(token), "hello world");
         ASSERT_EQ(std::get<1>(token), -1);
      }
   }
   {
      std::list<std::any> tokens = ShLexer(R"(a\ b a\\b)").lex();
      ASSERT_EQ(tokens.size(), 2);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         {
            std::any tokenAny = *iter;
            ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
            ASSERT_EQ(std::get<0>(token), "a b");
            ASSERT_EQ(std::get<1>(token), -1);
         }
         ++iter;
         {
            std::any tokenAny = *iter;
            ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
            ASSERT_EQ(std::get<0>(token), "a\\b");
            ASSERT_EQ(std::get<1>(token), -1);
         }
      }
   }
   {
      std::list<std::any> tokens = ShLexer(R"("" "")").lex();
      ASSERT_EQ(tokens.size(), 2);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         {
            std::any tokenAny = *iter;
            ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
            ASSERT_EQ(std::get<0>(token), "");
            ASSERT_EQ(std::get<1>(token), -1);
         }
         ++iter;
         {
            std::any tokenAny = *iter;
            ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
            ASSERT_EQ(std::get<0>(token), "");
            ASSERT_EQ(std::get<1>(token), -1);
         }
      }
   }
   {
      std::list<std::any> tokens = ShLexer(R"(a\ b)", true).lex();
      ASSERT_EQ(tokens.size(), 2);
      std::list<std::any>::iterator iter = tokens.begin();
      {
         {
            std::any tokenAny = *iter;
            ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
            ASSERT_EQ(std::get<0>(token), "a\\");
            ASSERT_EQ(std::get<1>(token), -1);
         }
         ++iter;
         {
            std::any tokenAny = *iter;
            ShellTokenType &token = std::any_cast<ShellTokenType &>(tokenAny);
            ASSERT_EQ(std::get<0>(token), "b");
            ASSERT_EQ(std::get<1>(token), -1);
         }
      }
   }
}
