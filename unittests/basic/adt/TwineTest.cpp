// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/16.

#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/FormatAdapters.h"
#include "polarphp/utils/FormatVariadic.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

namespace {

std::string repr(const Twine &value)
{
   std::string res;
   RawStringOutStream outstream(res);
   value.printRepr(outstream);
   return outstream.getStr();
}

TEST(TwineTest, testConstruction)
{
   EXPECT_EQ("", Twine().getStr());
   EXPECT_EQ("hi", Twine("hi").getStr());
   EXPECT_EQ("hi", Twine(std::string("hi")).getStr());
   EXPECT_EQ("hi", Twine(StringRef("hi")).getStr());
   EXPECT_EQ("hi", Twine(StringRef(std::string("hi"))).getStr());
   EXPECT_EQ("hi", Twine(StringRef("hithere", 2)).getStr());
   EXPECT_EQ("hi", Twine(SmallString<4>("hi")).getStr());
   EXPECT_EQ("hi", Twine(formatv("{0}", "hi")).getStr());
}

TEST(TwineTest, testNumbers)
{
   EXPECT_EQ("123", Twine(123U).getStr());
   EXPECT_EQ("123", Twine(123).getStr());
   EXPECT_EQ("-123", Twine(-123).getStr());
   EXPECT_EQ("123", Twine(123).getStr());
   EXPECT_EQ("-123", Twine(-123).getStr());

   EXPECT_EQ("7b", Twine::utohexstr(123).getStr());
}

TEST(TwineTest, testCharacters)
{
   EXPECT_EQ("x", Twine('x').getStr());
   EXPECT_EQ("x", Twine(static_cast<unsigned char>('x')).getStr());
   EXPECT_EQ("x", Twine(static_cast<signed char>('x')).getStr());
}

TEST(TwineTest, testConcat)
{
   // Check verse repr, since we care about the actual representation not just
   // the result.

   // Concat with null.
   EXPECT_EQ("(Twine null empty)",
             repr(Twine("hi").concat(Twine::createNull())));
   EXPECT_EQ("(Twine null empty)",
             repr(Twine::createNull().concat(Twine("hi"))));

   // Concat with empty.
   EXPECT_EQ("(Twine cstring:\"hi\" empty)",
             repr(Twine("hi").concat(Twine())));
   EXPECT_EQ("(Twine cstring:\"hi\" empty)",
             repr(Twine().concat(Twine("hi"))));
   EXPECT_EQ("(Twine smallstring:\"hi\" empty)",
             repr(Twine().concat(Twine(SmallString<5>("hi")))));
   EXPECT_EQ("(Twine formatv:\"howdy\" empty)",
             repr(Twine(formatv("howdy")).concat(Twine())));
   EXPECT_EQ("(Twine formatv:\"howdy\" empty)",
             repr(Twine().concat(Twine(formatv("howdy")))));
   EXPECT_EQ("(Twine smallstring:\"hey\" cstring:\"there\")",
             repr(Twine(SmallString<7>("hey")).concat(Twine("there"))));

   // Concatenation of unary ropes.
   EXPECT_EQ("(Twine cstring:\"a\" cstring:\"b\")",
             repr(Twine("a").concat(Twine("b"))));

   // Concatenation of other ropes.
   EXPECT_EQ("(Twine rope:(Twine cstring:\"a\" cstring:\"b\") cstring:\"c\")",
             repr(Twine("a").concat(Twine("b")).concat(Twine("c"))));
   EXPECT_EQ("(Twine cstring:\"a\" rope:(Twine cstring:\"b\" cstring:\"c\"))",
             repr(Twine("a").concat(Twine("b").concat(Twine("c")))));
   EXPECT_EQ("(Twine cstring:\"a\" rope:(Twine smallstring:\"b\" cstring:\"c\"))",
             repr(Twine("a").concat(Twine(SmallString<3>("b")).concat(Twine("c")))));
}

TEST(TwineTest, testToNullTerminatedStringRef)
{
   SmallString<8> storage;
   EXPECT_EQ(0, *Twine("hello").toNullTerminatedStringRef(storage).end());
   EXPECT_EQ(0,
             *Twine(StringRef("hello")).toNullTerminatedStringRef(storage).end());
   EXPECT_EQ(0, *Twine(SmallString<11>("hello"))
             .toNullTerminatedStringRef(storage)
             .end());
   EXPECT_EQ(0, *Twine(formatv("{0}{1}", "how", "dy"))
             .toNullTerminatedStringRef(storage)
             .end());
}

TEST(TwineTest, testLazyEvaluation)
{
   struct Formatter : FormatAdapter<int>
   {
      explicit Formatter(int &count) : FormatAdapter(0), count(count) {}
      int &count;

      void format(RawOutStream &outstream, StringRef style) { ++count; }
   };

   int count = 0;
   Formatter formatter(count);
   (void)Twine(formatv("{0}", formatter));
   EXPECT_EQ(0, count);
   (void)Twine(formatv("{0}", formatter)).getStr();
   EXPECT_EQ(1, count);
}

// I suppose linking in the entire code generator to add a unit test to check
// the code size of the concat operation is overkill... :)

} // anonymous namespace
