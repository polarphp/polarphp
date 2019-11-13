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

#include "polarphp/syntax/internal/TokenEnumDefs.h"
#include "polarphp/syntax/serialization/TokenKindTypeSerialization.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

using polar::syntax::internal::TokenKindType;
using nlohmann::json;

namespace {

TEST(TokenJsonSerializationTest, testTokenKindEnum)
{
   {
      json asJsonValue = TokenKindType::T_AS;
      ASSERT_TRUE(asJsonValue == "T_AS");
   }
   {
      json asJsonValue = "T_AS";
      ASSERT_EQ(asJsonValue.get<TokenKindType>(), TokenKindType::T_AS);
   }
   {
      json whileJsonValue = TokenKindType::T_WHILE;
      ASSERT_TRUE(whileJsonValue == "T_WHILE");
   }
   {
      json whileJsonValue = "T_WHILE";
      ASSERT_EQ(whileJsonValue.get<TokenKindType>(), TokenKindType::T_WHILE);
   }
   {
      json unknownJsonValue = "T_XXX";
      ASSERT_EQ(unknownJsonValue.get<TokenKindType>(), TokenKindType::T_UNKNOWN_MARK);
   }
}

}
