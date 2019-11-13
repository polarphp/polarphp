// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/13.

#include "polarphp/basic/adt/StringRef.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

using namespace polar::basic;
using nlohmann::json;

namespace {

TEST(JsonSerializationTest, testStringRef)
{
   StringRef str = "polarphp";
   json jsonObject = str;
   std::string actualStr = jsonObject.get<std::string>();
   ASSERT_EQ(actualStr, str);
}
}
