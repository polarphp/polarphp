// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/12.

#include "polarphp/syntax/Trivia.h"
#include "polarphp/basic/adt/SmallString.h"

namespace polar::syntax {

using polar::basic::SmallString;

namespace {
/// text - characters
static const std::map<TriviaKind, std::tuple<SmallString<32>, SmallString<8>>> scg_triviaKindTable {
   {TriviaKind::Space, {StringRef("Space"), StringRef(" ")}},
   {TriviaKind::Tab, {StringRef("Tab"), StringRef("\t")}},
   {TriviaKind::VerticalTab, {StringRef("VerticalTab"), StringRef("\v")}},
   {TriviaKind::Formfeed, {StringRef("Formfeed"), StringRef("\f")}},
   {TriviaKind::Newline, {StringRef("Newline"), StringRef("\n")}},
   {TriviaKind::CarriageReturn, {StringRef("CarriageReturn"), StringRef("\r")}},
   {TriviaKind::CarriageReturnLineFeed, {StringRef("CarriageReturnLineFeed"), StringRef("\r\n")}},
   {TriviaKind::Backtick, {StringRef("Backtick"), StringRef("`")}},
   {TriviaKind::LineComment, {StringRef("LineComment"), StringRef("")}},
   {TriviaKind::BlockComment, {StringRef("BlockComment"), StringRef("")}},
   {TriviaKind::DocLineComment, {StringRef("DocLineComment"), StringRef("")}},
   {TriviaKind::DocBlockComment, {StringRef("DocBlockComment"), StringRef("")}},
   {TriviaKind::GarbageText, {StringRef("GarbageText"), StringRef("")}}
};
} // anonymous namespace

StringRef retrieve_trivia_kind_name(TriviaKind kind)
{
   auto iter = scg_triviaKindTable.find(kind);
   if (iter == scg_triviaKindTable.end()) {
      return StringRef();
   }
   return std::get<0>(iter->second);
}

StringRef retrieve_trivia_kind_characters(TriviaKind kind)
{
   auto iter = scg_triviaKindTable.find(kind);
   if (iter == scg_triviaKindTable.end()) {
      return StringRef();
   }
   return std::get<1>(iter->second).getStr();
}

} // polar::syntax
