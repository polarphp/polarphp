// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/28.

#include "gtest/gtest.h"
#include "polarphp/vm/lang/Argument.h"

using polar::vmapi::ValueArgument;
using polar::vmapi::RefArgument;
using polar::vmapi::VariadicArgument;
using polar::vmapi::Type;

TEST(ArgumentTest, testConstructor)
{
   {
      ValueArgument arg("arg1");
      ASSERT_STREQ(arg.getName(), "arg1");
      ASSERT_STREQ(arg.getClassName(), nullptr);
      ASSERT_EQ(arg.getType(), Type::Null);
      ASSERT_TRUE(arg.isRequired());
      ASSERT_FALSE(arg.isReference());
      ASSERT_FALSE(arg.isVariadic());
   }
   {
      ValueArgument arg("argname", Type::Array);
      ASSERT_EQ(arg.getType(), Type::Array);
   }
   {
      ValueArgument arg("argname", Type::Array, false);
      ASSERT_EQ(arg.getType(), Type::Array);
      ASSERT_FALSE(arg.isRequired());
   }
   {
      ValueArgument arg("argname", "classname", false);
      ASSERT_TRUE(arg.isRequired());
      ASSERT_STREQ(arg.getClassName(), "classname");
      ASSERT_FALSE(arg.isNullable());
   }
   {
      ValueArgument arg("argname", "classname1", false, false);
      ASSERT_FALSE(arg.isRequired());
      ASSERT_STREQ(arg.getClassName(), "classname1");
      ASSERT_FALSE(arg.isNullable());
   }
}

TEST(ArgumentTest, testCopyConstructor)
{
   {
      ValueArgument arg("argname", "classname1", false, false);
      ValueArgument copy(arg);
      ASSERT_FALSE(copy.isRequired());
      ASSERT_STREQ(copy.getClassName(), "classname1");
      ASSERT_FALSE(copy.isNullable());
   }
   {
      ValueArgument arg("arg1");
      ValueArgument copy(arg);
      ASSERT_STREQ(copy.getName(), "arg1");
      ASSERT_STREQ(copy.getClassName(), nullptr);
      ASSERT_EQ(copy.getType(), Type::Null);
      ASSERT_TRUE(copy.isRequired());
      ASSERT_FALSE(copy.isReference());
   }
}

TEST(ArgumentTest, testMoveConstructor)
{
   {
      ValueArgument arg("argname", "classname1", false, false);
      ValueArgument move(std::move(arg));
      ASSERT_FALSE(move.isRequired());
      ASSERT_STREQ(move.getClassName(), "classname1");
      ASSERT_FALSE(move.isNullable());
   }
   {
      ValueArgument arg("arg1");
      ValueArgument move(std::move(arg));
      ASSERT_STREQ(move.getName(), "arg1");
      ASSERT_STREQ(move.getClassName(), nullptr);
      ASSERT_EQ(move.getType(), Type::Null);
      ASSERT_TRUE(move.isRequired());
      ASSERT_FALSE(move.isReference());
   }
}

TEST(ArgumentTest, testAssignOperator)
{
   {
      ValueArgument arg("argname", "classname1", false, false);
      ValueArgument copy = arg;
      ASSERT_FALSE(copy.isRequired());
      ASSERT_STREQ(copy.getClassName(), "classname1");
      ASSERT_FALSE(copy.isNullable());
   }
   {
      ValueArgument arg("argname", "classname1", false, false);
      ValueArgument move = std::move(arg);
      ASSERT_FALSE(move.isRequired());
      ASSERT_STREQ(move.getClassName(), "classname1");
      ASSERT_FALSE(move.isNullable());
   }
}

TEST(ArgumentTest, testRefArguments)
{
   {
      RefArgument arg("argname", "classname1", false);
      ASSERT_FALSE(arg.isRequired());
      ASSERT_STREQ(arg.getClassName(), "classname1");
      ASSERT_FALSE(arg.isNullable());
      ASSERT_FALSE(arg.isRequired());
      ASSERT_FALSE(arg.isVariadic());
      ASSERT_TRUE(arg.isReference());
   }
   {
      RefArgument arg("argname", Type::Array, true);
      ASSERT_TRUE(arg.isRequired());
      ASSERT_STREQ(arg.getClassName(), nullptr);
      ASSERT_EQ(arg.getType(), Type::Array);
      ASSERT_FALSE(arg.isNullable());
      ASSERT_TRUE(arg.isRequired());
      ASSERT_FALSE(arg.isVariadic());
      ASSERT_TRUE(arg.isReference());
   }
}

TEST(ArgumentTest, testVaridicArguments)
{
   VariadicArgument arg("argname", Type::Undefined, true);
   ASSERT_FALSE(arg.isRequired());
   ASSERT_STREQ(arg.getClassName(), nullptr);
   ASSERT_FALSE(arg.isNullable());
   ASSERT_FALSE(arg.isRequired());
   ASSERT_TRUE(arg.isVariadic());
   ASSERT_TRUE(arg.isReference());
   ASSERT_EQ(arg.getType(), Type::Undefined);
}
