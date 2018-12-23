// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/04.

#include "polarphp/utils/FormattedStream.h"
#include "polarphp/utils/FormatCommon.h"
#include "polarphp/utils/FormatVariadic.h"
#include "polarphp/global/Global.h"

namespace polar {
namespace utils {

namespace {

std::optional<AlignStyle> translate_loc_char(char character)
{
   switch (character) {
   case '-':
      return AlignStyle::Left;
   case '=':
      return AlignStyle::Center;
   case '+':
      return AlignStyle::Right;
   default:
      return std::nullopt;
   }
   POLAR_BUILTIN_UNREACHABLE;
}

} // anonymous namespace

bool FormatvObjectBase::consumeFieldLayout(StringRef &spec, AlignStyle &where,
                                           size_t &align, char &pad)
{
   where = AlignStyle::Right;
   align = 0;
   pad = ' ';
   if (spec.empty()) {
      return true;
   }
   if (spec.getSize() > 1) {
      // A maximum of 2 characters at the beginning can be used for something
      // other
      // than the width.
      // If Spec[1] is a loc char, then Spec[0] is a pad char and Spec[2:...]
      // contains the width.
      // Otherwise, if Spec[0] is a loc char, then Spec[1:...] contains the width.
      // Otherwise, Spec[0:...] contains the width.
      if (auto loc = translate_loc_char(spec[1])) {
         pad = spec[0];
         where = *loc;
         spec = spec.dropFront(2);
      } else if (auto loc = translate_loc_char(spec[0])) {
         where = *loc;
         spec = spec.dropFront(1);
      }
   }
   bool failed = spec.consumeInteger(0, align);
   return !failed;
}

std::optional<ReplacementItem>
FormatvObjectBase::parseReplacementItem(StringRef spec)
{
   StringRef repString = spec.trim("{}");

   // If the replacement sequence does not start with a non-negative integer,
   // this is an error.
   char pad = ' ';
   std::size_t align = 0;
   AlignStyle where = AlignStyle::Right;
   StringRef options;
   size_t index = 0;
   repString = repString.trim();
   if (repString.consumeInteger(0, index)) {
      assert(false && "Invalid replacement sequence index!");
      return ReplacementItem{};
   }
   repString = repString.trim();
   if (!repString.empty() && repString.front() == ',') {
      repString = repString.dropFront();
      if (!consumeFieldLayout(repString, where, align, pad)) {
         assert(false && "Invalid replacement field layout specification!");
      }
   }
   repString = repString.trim();
   if (!repString.empty() && repString.front() == ':') {
      options = repString.dropFront().trim();
      repString = StringRef();
   }
   repString = repString.trim();
   if (!repString.empty()) {
      assert(false && "Unexpected characters found in replacement string!");
   }

   return ReplacementItem{spec, index, align, where, pad, options};
}

std::pair<ReplacementItem, StringRef>
FormatvObjectBase::splitLiteralAndReplacement(StringRef fmt)
{
   std::size_t from = 0;
   while (from < fmt.getSize() && from != StringRef::npos) {
      std::size_t bo = fmt.findFirstOf('{', from);
      // Everything up until the first brace is a literal.
      if (bo != 0) {
         return std::make_pair(ReplacementItem{fmt.substr(0, bo)}, fmt.substr(bo));
      }
      StringRef braces =
            fmt.dropFront(bo).takeWhile([](char character)
      {
         return character == '{';
      });
      // If there is more than one brace, then some of them are escaped.  Treat
      // these as replacements.
      if (braces.getSize() > 1) {
         size_t numEscapedBraces = braces.getSize() / 2;
         StringRef middle = fmt.substr(bo, numEscapedBraces);
         StringRef right = fmt.dropFront(bo + numEscapedBraces * 2);
         return std::make_pair(ReplacementItem{middle}, right);
      }
      // An unterminated open brace is undefined.  We treat the rest of the string
      // as a literal replacement, but we assert to indicate that this is
      // undefined and that we consider it an error.
      std::size_t bc = fmt.findFirstOf('}', bo);
      if (bc == StringRef::npos) {
         assert(
                  false &&
                  "Unterminated brace sequence.  Escape with {{ for a literal brace.");
         return std::make_pair(ReplacementItem{fmt}, StringRef());
      }

      // Even if there is a closing brace, if there is another open brace before
      // this closing brace, treat this portion as literal, and try again with the
      // next one.
      std::size_t bo2 = fmt.findFirstOf('{', bo + 1);
      if (bo2 < bc) {
         return std::make_pair(ReplacementItem{fmt.substr(0, bo2)},
                               fmt.substr(bo2));
      }
      StringRef spec = fmt.slice(bo + 1, bc);
      StringRef right = fmt.substr(bc + 1);

      auto ri = parseReplacementItem(spec);
      if (ri.has_value()) {
         return std::make_pair(*ri, right);
      }
      // If there was an error parsing the replacement item, treat it as an
      // invalid replacement spec, and just continue.
      from = bc + 1;
   }
   return std::make_pair(ReplacementItem{fmt}, StringRef());
}

std::vector<ReplacementItem>
FormatvObjectBase::parseFormatString(StringRef fmt)
{
   std::vector<ReplacementItem> replacements;
   ReplacementItem replacementItem;
   while (!fmt.empty()) {
      std::tie(replacementItem, fmt) = splitLiteralAndReplacement(fmt);
      if (replacementItem.m_type != ReplacementType::Empty) {
         replacements.push_back(replacementItem);
      }
   }
   return replacements;
}

} // utils
} // polar
