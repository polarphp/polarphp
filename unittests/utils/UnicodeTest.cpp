// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/09.

#include "polarphp/utils/Unicode.h"
#include "polarphp/basic/adt/StringRef.h"
#include "gtest/gtest.h"

namespace polar {
namespace sys {
namespace unicode {

namespace {

TEST(UnicodeTest, testColumnWidthUtf8)
{
   EXPECT_EQ(0, column_width_utf8(""));
   EXPECT_EQ(1, column_width_utf8(" "));
   EXPECT_EQ(1, column_width_utf8("a"));
   EXPECT_EQ(1, column_width_utf8("~"));

   EXPECT_EQ(6, column_width_utf8("abcdef"));

   EXPECT_EQ(-1, column_width_utf8("\x01"));
   EXPECT_EQ(-1, column_width_utf8("aaaaaaaaaa\x01"));
   EXPECT_EQ(-1, column_width_utf8("\342\200\213")); // 200B ZERO WIDTH SPACE

   // 00AD SOFT HYPHEN is displayed on most terminals as a space or a dash. Some
   // text editors display it only when a line is broken at it, some use it as a
   // line-break hint, but don't display. We choose terminal-oriented
   // interpretation.
   EXPECT_EQ(1, column_width_utf8("\302\255"));

   EXPECT_EQ(0, column_width_utf8("\314\200"));     // 0300 COMBINING GRAVE ACCENT
   EXPECT_EQ(1, column_width_utf8("\340\270\201")); // 0E01 THAI CHARACTER KO KAI
   EXPECT_EQ(2, column_width_utf8("\344\270\200")); // CJK UNIFIED IDEOGRAPH-4E00

   EXPECT_EQ(4, column_width_utf8("\344\270\200\344\270\200"));
   EXPECT_EQ(3, column_width_utf8("q\344\270\200"));
   EXPECT_EQ(3, column_width_utf8("\314\200\340\270\201\344\270\200"));

   // Invalid UTF-8 strings, column_width_utf8 should error out.
   EXPECT_EQ(-2, column_width_utf8("\344"));
   EXPECT_EQ(-2, column_width_utf8("\344\270"));
   EXPECT_EQ(-2, column_width_utf8("\344\270\033"));
   EXPECT_EQ(-2, column_width_utf8("\344\270\300"));
   EXPECT_EQ(-2, column_width_utf8("\377\366\355"));

   EXPECT_EQ(-2, column_width_utf8("qwer\344"));
   EXPECT_EQ(-2, column_width_utf8("qwer\344\270"));
   EXPECT_EQ(-2, column_width_utf8("qwer\344\270\033"));
   EXPECT_EQ(-2, column_width_utf8("qwer\344\270\300"));
   EXPECT_EQ(-2, column_width_utf8("qwer\377\366\355"));

   // UTF-8 sequences longer than 4 bytes correspond to unallocated Unicode
   // characters.
   EXPECT_EQ(-2, column_width_utf8("\370\200\200\200\200"));     // U+200000
   EXPECT_EQ(-2, column_width_utf8("\374\200\200\200\200\200")); // U+4000000
}

TEST(UnicodeTest, testIsPrintable)
{
   EXPECT_FALSE(is_printable(0)); // <control-0000>-<control-001F>
   EXPECT_FALSE(is_printable(0x01));
   EXPECT_FALSE(is_printable(0x1F));
   EXPECT_TRUE(is_printable(' '));
   EXPECT_TRUE(is_printable('A'));
   EXPECT_TRUE(is_printable('~'));
   EXPECT_FALSE(is_printable(0x7F)); // <control-007F>..<control-009F>
   EXPECT_FALSE(is_printable(0x90));
   EXPECT_FALSE(is_printable(0x9F));

   EXPECT_TRUE(is_printable(0xAC));
   EXPECT_TRUE(is_printable(0xAD)); // SOFT HYPHEN is displayed on most terminals
   // as either a space or a dash.
   EXPECT_TRUE(is_printable(0xAE));

   EXPECT_TRUE(is_printable(0x0377));  // GREEK SMALL LETTER PAMPHYLIAN DIGAMMA
   EXPECT_FALSE(is_printable(0x0378)); // <reserved-0378>..<reserved-0379>

   EXPECT_FALSE(is_printable(0x0600)); // ARABIC NUMBER SIGN

   EXPECT_FALSE(is_printable(0x1FFFF)); // <reserved-1F774>..<noncharacter-1FFFF>
   EXPECT_TRUE(is_printable(0x20000));  // CJK UNIFIED IDEOGRAPH-20000

   EXPECT_FALSE(is_printable(0x10FFFF)); // noncharacter
}

} // namespace

} // unicode
} // sys
} // polar
