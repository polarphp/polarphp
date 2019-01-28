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

#include "polarphp/vm/lang/Constant.h"
#include "polarphp/basic/adt/StringRef.h"
#include "gtest/gtest.h"
#include <any>

using polar::vmapi::Type;
using polar::vmapi::Constant;
using polar::basic::StringRef;

TEST(ConstantTest, test)
{
   {
      Constant trueFlagConst("TrueConst", true);
      ASSERT_EQ(trueFlagConst.getName(), "TrueConst");
      std::any rawValue = trueFlagConst.getValue();
      ASSERT_TRUE(rawValue.type() == typeid(bool));
      ASSERT_TRUE(std::any_cast<bool>(rawValue) == true);
   }
   {
      Constant falseFlagConst("FalseConst", false);
      ASSERT_EQ(falseFlagConst.getName(), "FalseConst");
      std::any rawValue = falseFlagConst.getValue();
      ASSERT_TRUE(rawValue.type() == typeid(bool));
      ASSERT_TRUE(std::any_cast<bool>(rawValue) == false);
   }
   {
      Constant doubleConst("DoubleConst", 3.14);
      ASSERT_EQ(doubleConst.getName(), "DoubleConst");
      std::any rawValue = doubleConst.getValue();
      ASSERT_TRUE(rawValue.type() == typeid(double));
      ASSERT_TRUE(std::any_cast<double>(rawValue) == 3.14);
   }
   {
      Constant longConst("LongConst", 2019);
      ASSERT_EQ(longConst.getName(), "LongConst");
      std::any rawValue = longConst.getValue();
      ASSERT_TRUE(rawValue.type() == typeid(vmapi_long));
      ASSERT_TRUE(std::any_cast<vmapi_long>(rawValue) == 2019);
   }
   {
      Constant stringConst("StringConst", "polarphp");
      ASSERT_EQ(stringConst.getName(), "StringConst");
      std::any rawValue = stringConst.getValue();
      ASSERT_TRUE(rawValue.type() == typeid(StringRef));
      ASSERT_TRUE(std::any_cast<StringRef>(rawValue) == "polarphp");
   }
}
