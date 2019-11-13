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
#include "polarphp/parser/serialization/TokenJsonSerialization.h"
#include "polarphp/parser/Token.h"
#include "gtest/gtest.h"
#include "nlohmann/json.hpp"

#include <list>

using polar::syntax::internal::TokenKindType;
using polar::parser::Token;
using polar::parser::TokenFlags;
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

TEST(TokenJsonSerializationTest, testTokenValueTypeEnum)
{
   {
      json doubleJsonValue = Token::ValueType::Double;
      ASSERT_TRUE(doubleJsonValue == "Double");
      json stringJsonValue = Token::ValueType::String;
      ASSERT_TRUE(stringJsonValue == "String");
      json longLongJsonValue = Token::ValueType::LongLong;
      ASSERT_TRUE(longLongJsonValue == "LongLong");
      json unknownValueType = Token::ValueType::Unknown;
      ASSERT_TRUE(unknownValueType == "Unknown");
   }
   {
      using ValueType = Token::ValueType;
      json doubleJsonValue = "Double";
      ASSERT_EQ(doubleJsonValue.get<ValueType>(), ValueType::Double);
      json stringJsonValue = "String";
      ASSERT_EQ(stringJsonValue.get<ValueType>(), ValueType::String);
      json longLongJsonValue = "LongLong";
      ASSERT_EQ(longLongJsonValue.get<ValueType>(), ValueType::LongLong);
      json invalidJsonValue = "Int";
      ASSERT_EQ(invalidJsonValue.get<ValueType>(), ValueType::Invalid);
   }
}

TEST(TokenJsonSerializationTest, testTokenFlags)
{
   TokenFlags tokenFlags;
   tokenFlags.setAtStartOfLine(true);
   tokenFlags.setNeedCorrectLNumberOverflow(true);
   json flagsJsonObject = tokenFlags;
   ASSERT_EQ(flagsJsonObject.size(), 2);
   auto flagList = flagsJsonObject.get<std::list<TokenFlags::FlagType>>();
   ASSERT_TRUE(std::find(flagList.begin(), flagList.end(), TokenFlags::FlagType::AtStartOfLine) != flagList.end());
   ASSERT_TRUE(std::find(flagList.begin(), flagList.end(), TokenFlags::FlagType::NeedCorrectLNumberOverflow) != flagList.end());
   ASSERT_TRUE(std::find(flagList.begin(), flagList.end(), TokenFlags::FlagType::InvalidLexValue) == flagList.end());
}

} // anonymous namespace
