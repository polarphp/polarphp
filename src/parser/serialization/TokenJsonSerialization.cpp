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

#include <list>

namespace polar::parser {

using FlagType = TokenFlags::FlagType;

void to_json(json &jsonObject, const TokenFlags &flags)
{
   std::list<TokenFlags::FlagType> flagList{};
   if (flags.isAtStartOfLine()) {
      flagList.push_back(FlagType::AtStartOfLine);
   }
   if (flags.isInvalidLexValue()) {
      flagList.push_back(FlagType::InvalidLexValue);
   }
   if (flags.isEscapedIdentifier()) {
      flagList.push_back(FlagType::EscapedIdentifier);
   }
   if (flags.isNeedCorrectLNumberOverflow()) {
      flagList.push_back(FlagType::NeedCorrectLNumberOverflow);
   }
   jsonObject = flagList;
}

void to_json(json &jsonObject, const Token &token)
{
   jsonObject = json::object();
   jsonObject["name"] = token.getName();
}

} // polar::parser
