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

#include "polarphp/parser/serialization/TokenJsonSerialization.h"
#include "polarphp/syntax/serialization/TokenKindTypeSerialization.h"
#include "polarphp/parser/Token.h"

#include <set>

namespace polar::parser {

using FlagType = TokenFlags::FlagType;

void to_json(json &jsonObject, const TokenFlags &flags)
{
   std::set<TokenFlags::FlagType> flagList{};
   if (flags.isAtStartOfLine()) {
      flagList.insert(FlagType::AtStartOfLine);
   }
   if (flags.isInvalidLexValue()) {
      flagList.insert(FlagType::InvalidLexValue);
   }
   if (flags.isEscapedIdentifier()) {
      flagList.insert(FlagType::EscapedIdentifier);
   }
   if (flags.isNeedCorrectLNumberOverflow()) {
      flagList.insert(FlagType::NeedCorrectLNumberOverflow);
   }
   jsonObject = flagList;
}

void from_json(const json &jsonObject, TokenFlags &flags)
{
   auto items = jsonObject.get<std::set<FlagType>>();
   if (items.find(FlagType::AtStartOfLine) != items.end()) {
      flags.setAtStartOfLine(true);
   }
   if (items.find(FlagType::InvalidLexValue) != items.end()) {
      flags.setInvalidLexValue(true);
   }
   if (items.find(FlagType::EscapedIdentifier) != items.end()) {
      flags.setEscapedIdentifier(true);
   }
   if (items.find(FlagType::NeedCorrectLNumberOverflow) != items.end()) {
      flags.setNeedCorrectLNumberOverflow(true);
   }
}

void to_json(json &jsonObject, const Token &token)
{
   jsonObject = json::object();
   bool hasValue = token.hasValue();
   jsonObject["name"] = token.getName();
   jsonObject["kind"] = token.getKind();
   jsonObject["category"] = token.getCategory();
   jsonObject["flags"] = token.getFlags();
   if (hasValue) {
      jsonObject["valueType"] = token.getValueType();
   }
   if (!token.isInvalidLexValue()) {
      ValueType valueType = token.getValueType();
      TokenKindType kind = token.getKind();
      if (valueType == ValueType::String) {
         std::string value = token.getValue<std::string>();
         if (kind == TokenKindType::T_VARIABLE) {
            value = '$' + value;
         }
         jsonObject["value"] = value;
      } else if (valueType == ValueType::Double) {
         jsonObject["value"] = token.getValue<double>();
      } else if (valueType == ValueType::LongLong) {
         jsonObject["value"] = token.getValue<std::int64_t>();
      }
   }
   jsonObject["hasValue"] = hasValue;
   jsonObject["hasComment"] = token.hasComment();
   jsonObject["definedText"] = token.getDefinedText();
}

void from_json(const json &jsonObject, Token &token)
{
   TokenKindType kind = jsonObject.at("token").get<TokenKindType>();
   token.setKind(kind);
   if (jsonObject.contains("flags")) {
      TokenFlags flags = jsonObject.at("flags").get<TokenFlags>();
      token.setFlags(flags);
   }
   bool hasValue = jsonObject.at("hasValue").get<bool>();
   if (hasValue) {
      ValueType valueType = jsonObject.at("valueType").get<ValueType>();
      if (valueType == ValueType::String) {
         token.setValue(jsonObject.at("value").get<std::string>());
      } else if (valueType == ValueType::Double) {
         token.setValue(jsonObject.at("value").get<double>());
      } else if (valueType == ValueType::LongLong) {
         token.setValue(jsonObject.at("value").get<std::int64_t>());
      }
   }
}

} // polar::parser
