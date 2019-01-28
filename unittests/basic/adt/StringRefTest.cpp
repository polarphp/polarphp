// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors

// Created by polarboy on 2018/07/06.

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

namespace polar {
namespace basic {

std::ostream &operator<<(std::ostream &outstream,
                         const std::pair<StringRef, StringRef> &pair)
{
   outstream << "(" << pair.first << ", " << pair.second << ")";
   return outstream;
}

} // basic
} // polar

// Check that we can't accidentally assign a temporary std::string to a
// StringRef. (Unfortunately we can't make use of the same thing with
// constructors.)
//
// Disable this check under MSVC; even MSVC 2015 isn't consistent between
// std::is_assignable and actually writing such an assignment.
#if !defined(_MSC_VER)
static_assert(
      !std::is_assignable<StringRef&, std::string>::value,
      "Assigning from prvalue std::string");
static_assert(
      !std::is_assignable<StringRef&, std::string &&>::value,
      "Assigning from xvalue std::string");
static_assert(
      std::is_assignable<StringRef&, std::string &>::value,
      "Assigning from lvalue std::string");
static_assert(
      std::is_assignable<StringRef&, const char *>::value,
      "Assigning from prvalue C string");
static_assert(
      std::is_assignable<StringRef&, const char * &&>::value,
      "Assigning from xvalue C string");
static_assert(
      std::is_assignable<StringRef&, const char * &>::value,
      "Assigning from lvalue C string");
#endif

namespace {

TEST(StringRefTest, testConstruction)
{
   EXPECT_EQ("", StringRef());
   EXPECT_EQ("hello", StringRef("hello"));
   EXPECT_EQ("hello", StringRef("hello world", 5));
   EXPECT_EQ("hello", StringRef(std::string("hello")));
}

}

TEST(StringRefTest, testEmptyInitializerList)
{
   StringRef str = {};
   EXPECT_TRUE(str.empty());
   str = {};
   EXPECT_TRUE(str.empty());
   StringRef str1("abc abc");
   ASSERT_TRUE(str1.find('c') != StringRef::npos);
}

TEST(StringRefTest, testIteration)
{
   StringRef str("hello");
   const char *p = "hello";
   for (const char *it = str.begin(), *ie = str.end(); it != ie; ++it, ++p) {
      EXPECT_EQ(*it, *p);
   }
}

TEST(StringRefTest, testStringOps)
{
   const char *p = "hello";
   EXPECT_EQ(p, StringRef(p, 0).getData());
   EXPECT_TRUE(StringRef().empty());
   EXPECT_EQ((size_t) 5, StringRef("hello").size());
   EXPECT_EQ(-1, StringRef("aab").compare("aad"));
   EXPECT_EQ( 0, StringRef("aab").compare("aab"));
   EXPECT_EQ( 1, StringRef("aab").compare("aaa"));
   EXPECT_EQ(-1, StringRef("aab").compare("aabb"));
   EXPECT_EQ( 1, StringRef("aab").compare("aa"));
   EXPECT_EQ( 1, StringRef("\xFF").compare("\1"));

   EXPECT_EQ(-1, StringRef("AaB").compareLower("aAd"));
   EXPECT_EQ( 0, StringRef("AaB").compareLower("aab"));
   EXPECT_EQ( 1, StringRef("AaB").compareLower("AAA"));
   EXPECT_EQ(-1, StringRef("AaB").compareLower("aaBb"));
   EXPECT_EQ(-1, StringRef("AaB").compareLower("bb"));
   EXPECT_EQ( 1, StringRef("aaBb").compareLower("AaB"));
   EXPECT_EQ( 1, StringRef("bb").compareLower("AaB"));
   EXPECT_EQ( 1, StringRef("AaB").compareLower("aA"));
   EXPECT_EQ( 1, StringRef("\xFF").compareLower("\1"));

   EXPECT_EQ(-1, StringRef("aab").compareNumeric("aad"));
   EXPECT_EQ( 0, StringRef("aab").compareNumeric("aab"));
   EXPECT_EQ( 1, StringRef("aab").compareNumeric("aaa"));
   EXPECT_EQ(-1, StringRef("aab").compareNumeric("aabb"));
   EXPECT_EQ( 1, StringRef("aab").compareNumeric("aa"));
   EXPECT_EQ(-1, StringRef("1").compareNumeric("10"));
   EXPECT_EQ( 0, StringRef("10").compareNumeric("10"));
   EXPECT_EQ( 0, StringRef("10a").compareNumeric("10a"));
   EXPECT_EQ( 1, StringRef("2").compareNumeric("1"));
   EXPECT_EQ( 0, StringRef("polarvm_v1i64_ty").compareNumeric("polarvm_v1i64_ty"));
   EXPECT_EQ( 1, StringRef("\xFF").compareNumeric("\1"));
   EXPECT_EQ( 1, StringRef("V16").compareNumeric("V1_q0"));
   EXPECT_EQ(-1, StringRef("V1_q0").compareNumeric("V16"));
   EXPECT_EQ(-1, StringRef("V8_q0").compareNumeric("V16"));
   EXPECT_EQ( 1, StringRef("V16").compareNumeric("V8_q0"));
   EXPECT_EQ(-1, StringRef("V1_q0").compareNumeric("V8_q0"));
   EXPECT_EQ( 1, StringRef("V8_q0").compareNumeric("V1_q0"));
}

TEST(StringRefTest, testOperators)
{
   EXPECT_EQ("", StringRef());
   EXPECT_TRUE(StringRef("aab") < StringRef("aad"));
   EXPECT_FALSE(StringRef("aab") < StringRef("aab"));
   EXPECT_TRUE(StringRef("aab") <= StringRef("aab"));
   EXPECT_FALSE(StringRef("aab") <= StringRef("aaa"));
   EXPECT_TRUE(StringRef("aad") > StringRef("aab"));
   EXPECT_FALSE(StringRef("aab") > StringRef("aab"));
   EXPECT_TRUE(StringRef("aab") >= StringRef("aab"));
   EXPECT_FALSE(StringRef("aaa") >= StringRef("aab"));
   EXPECT_EQ(StringRef("aab"), StringRef("aab"));
   EXPECT_FALSE(StringRef("aab") == StringRef("aac"));
   EXPECT_FALSE(StringRef("aab") != StringRef("aab"));
   EXPECT_TRUE(StringRef("aab") != StringRef("aac"));
   EXPECT_EQ('a', StringRef("aab")[1]);
}

TEST(StringRefTest, testSubstr)
{
   StringRef str("hello");
   EXPECT_EQ("lo", str.substr(3));
   EXPECT_EQ("", str.substr(100));
   EXPECT_EQ("hello", str.substr(0, 100));
   EXPECT_EQ("o", str.substr(4, 10));
}

TEST(StringRefTest, testSlice)
{
   StringRef str("hello");
   EXPECT_EQ("l", str.slice(2, 3));
   EXPECT_EQ("ell", str.slice(1, 4));
   EXPECT_EQ("llo", str.slice(2, 100));
   EXPECT_EQ("", str.slice(2, 1));
   EXPECT_EQ("", str.slice(10, 20));
}

TEST(StringRefTest, testSplit)
{
   StringRef str("hello");
   EXPECT_EQ(std::make_pair(StringRef("hello"), StringRef("")),
             str.split('X'));
   EXPECT_EQ(std::make_pair(StringRef("h"), StringRef("llo")),
             str.split('e'));
   EXPECT_EQ(std::make_pair(StringRef(""), StringRef("ello")),
             str.split('h'));
   EXPECT_EQ(std::make_pair(StringRef("he"), StringRef("lo")),
             str.split('l'));
   EXPECT_EQ(std::make_pair(StringRef("hell"), StringRef("")),
             str.split('o'));

   EXPECT_EQ(std::make_pair(StringRef("hello"), StringRef("")),
             str.rsplit('X'));
   EXPECT_EQ(std::make_pair(StringRef("h"), StringRef("llo")),
             str.rsplit('e'));
   EXPECT_EQ(std::make_pair(StringRef(""), StringRef("ello")),
             str.rsplit('h'));
   EXPECT_EQ(std::make_pair(StringRef("hel"), StringRef("o")),
             str.rsplit('l'));
   EXPECT_EQ(std::make_pair(StringRef("hell"), StringRef("")),
             str.rsplit('o'));

   EXPECT_EQ(std::make_pair(StringRef("hello"), StringRef("")),
             str.rsplit("::"));
   EXPECT_EQ(std::make_pair(StringRef("hel"), StringRef("o")),
             str.rsplit("l"));
}

TEST(StringRefTest, testSplit2)
{
   SmallVector<StringRef, 5> parts;
   SmallVector<StringRef, 5> expected;

   expected.push_back("ab"); expected.push_back("c");
   StringRef(",ab,,c,").split(parts, ",", -1, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back(""); expected.push_back("ab"); expected.push_back("");
   expected.push_back("c"); expected.push_back("");
   StringRef(",ab,,c,").split(parts, ",", -1, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("");
   StringRef("").split(parts, ",", -1, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   StringRef("").split(parts, ",", -1, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   StringRef(",").split(parts, ",", -1, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back(""); expected.push_back("");
   StringRef(",").split(parts, ",", -1, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back("b");
   StringRef("a,b").split(parts, ",", -1, true);
   EXPECT_TRUE(parts == expected);

   // test MaxSplit
   expected.clear(); parts.clear();
   expected.push_back("a,,b,c");
   StringRef("a,,b,c").split(parts, ",", 0, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a,,b,c");
   StringRef("a,,b,c").split(parts, ",", 0, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back(",b,c");
   StringRef("a,,b,c").split(parts, ",", 1, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back(",b,c");
   StringRef("a,,b,c").split(parts, ",", 1, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back(""); expected.push_back("b,c");
   StringRef("a,,b,c").split(parts, ",", 2, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back("b,c");
   StringRef("a,,b,c").split(parts, ",", 2, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back(""); expected.push_back("b");
   expected.push_back("c");
   StringRef("a,,b,c").split(parts, ",", 3, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back("b"); expected.push_back("c");
   StringRef("a,,b,c").split(parts, ",", 3, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a"); expected.push_back("b"); expected.push_back("c");
   StringRef("a,,b,c").split(parts, ',', 3, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("");
   StringRef().split(parts, ",", 0, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back(StringRef());
   StringRef("").split(parts, ",", 0, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   StringRef("").split(parts, ",", 0, false);
   EXPECT_TRUE(parts == expected);
   StringRef().split(parts, ",", 0, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a");
   expected.push_back("");
   expected.push_back("b");
   expected.push_back("c,d");
   StringRef("a,,b,c,d").split(parts, ",", 3, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("");
   StringRef().split(parts, ',', 0, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back(StringRef());
   StringRef("").split(parts, ',', 0, true);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   StringRef("").split(parts, ',', 0, false);
   EXPECT_TRUE(parts == expected);
   StringRef().split(parts, ',', 0, false);
   EXPECT_TRUE(parts == expected);

   expected.clear(); parts.clear();
   expected.push_back("a");
   expected.push_back("");
   expected.push_back("b");
   expected.push_back("c,d");
   StringRef("a,,b,c,d").split(parts, ',', 3, true);
   EXPECT_TRUE(parts == expected);
}

TEST(StringRefTest, testTrim)
{
   StringRef Str0("hello");
   StringRef str1(" hello ");
   StringRef str2("  hello  ");

   EXPECT_EQ(StringRef("hello"), Str0.rtrim());
   EXPECT_EQ(StringRef(" hello"), str1.rtrim());
   EXPECT_EQ(StringRef("  hello"), str2.rtrim());
   EXPECT_EQ(StringRef("hello"), Str0.ltrim());
   EXPECT_EQ(StringRef("hello "), str1.ltrim());
   EXPECT_EQ(StringRef("hello  "), str2.ltrim());
   EXPECT_EQ(StringRef("hello"), Str0.trim());
   EXPECT_EQ(StringRef("hello"), str1.trim());
   EXPECT_EQ(StringRef("hello"), str2.trim());

   EXPECT_EQ(StringRef("ello"), Str0.trim("hhhhhhhhhhh"));

   EXPECT_EQ(StringRef(""), StringRef("").trim());
   EXPECT_EQ(StringRef(""), StringRef(" ").trim());
   EXPECT_EQ(StringRef("\0", 1), StringRef(" \0 ", 3).trim());
   EXPECT_EQ(StringRef("\0\0", 2), StringRef("\0\0", 2).trim());
   EXPECT_EQ(StringRef("x"), StringRef("\0\0x\0\0", 5).trim('\0'));
}

TEST(StringRefTest, testStartsWith)
{
   StringRef str("hello");
   EXPECT_TRUE(str.startsWith(""));
   EXPECT_TRUE(str.startsWith("he"));
   EXPECT_FALSE(str.startsWith("helloworld"));
   EXPECT_FALSE(str.startsWith("hi"));
}

TEST(StringRefTest, testStartsWithLower)
{
   StringRef str("heLLo");
   EXPECT_TRUE(str.startsWithLower(""));
   EXPECT_TRUE(str.startsWithLower("he"));
   EXPECT_TRUE(str.startsWithLower("hell"));
   EXPECT_TRUE(str.startsWithLower("HELlo"));
   EXPECT_FALSE(str.startsWithLower("helloworld"));
   EXPECT_FALSE(str.startsWithLower("hi"));
}

TEST(StringRefTest, testConsumeFront)
{
   StringRef str("hello");
   EXPECT_TRUE(str.consumeFront(""));
   EXPECT_EQ("hello", str);
   EXPECT_TRUE(str.consumeFront("he"));
   EXPECT_EQ("llo", str);
   EXPECT_FALSE(str.consumeFront("lloworld"));
   EXPECT_EQ("llo", str);
   EXPECT_FALSE(str.consumeFront("lol"));
   EXPECT_EQ("llo", str);
   EXPECT_TRUE(str.consumeFront("llo"));
   EXPECT_EQ("", str);
   EXPECT_FALSE(str.consumeFront("o"));
   EXPECT_TRUE(str.consumeFront(""));
}


TEST(StringRefTest, testEndsWith)
{
   StringRef str("hello");
   EXPECT_TRUE(str.endsWith(""));
   EXPECT_TRUE(str.endsWith("lo"));
   EXPECT_FALSE(str.endsWith("helloworld"));
   EXPECT_FALSE(str.endsWith("worldhello"));
   EXPECT_FALSE(str.endsWith("so"));
}

TEST(StringRefTest, testEndsWithLower)
{
   StringRef str("heLLo");
   EXPECT_TRUE(str.endsWithLower(""));
   EXPECT_TRUE(str.endsWithLower("lo"));
   EXPECT_TRUE(str.endsWithLower("LO"));
   EXPECT_TRUE(str.endsWithLower("ELlo"));
   EXPECT_FALSE(str.endsWithLower("helloworld"));
   EXPECT_FALSE(str.endsWithLower("hi"));
}

TEST(StringRefTest, testConsumeBack)
{
   StringRef str("hello");
   EXPECT_TRUE(str.consumeBack(""));
   EXPECT_EQ("hello", str);
   EXPECT_TRUE(str.consumeBack("lo"));
   EXPECT_EQ("hel", str);
   EXPECT_FALSE(str.consumeBack("helhel"));
   EXPECT_EQ("hel", str);
   EXPECT_FALSE(str.consumeBack("hle"));
   EXPECT_EQ("hel", str);
   EXPECT_TRUE(str.consumeBack("hel"));
   EXPECT_EQ("", str);
   EXPECT_FALSE(str.consumeBack("h"));
   EXPECT_TRUE(str.consumeBack(""));
}

TEST(StringRefTest, testFind)
{
   StringRef str("helloHELLO");
   StringRef longStr("hellx xello hell ello world foo bar hello HELLO");

   struct {
      StringRef str;
      char C;
      std::size_t From;
      std::size_t Pos;
      std::size_t LowerPos;
   } CharExpectations[] = {
   {str, 'h', 0U, 0U, 0U},
   {str, 'e', 0U, 1U, 1U},
   {str, 'l', 0U, 2U, 2U},
   {str, 'l', 3U, 3U, 3U},
   {str, 'o', 0U, 4U, 4U},
   {str, 'L', 0U, 7U, 2U},
   {str, 'z', 0U, StringRef::npos, StringRef::npos},
};

   struct {
      StringRef str;
      StringRef S;
      std::size_t From;
      std::size_t Pos;
      std::size_t LowerPos;
   } StrExpectations[] = {
   {str, "helloword", 0, StringRef::npos, StringRef::npos},
   {str, "hello", 0, 0U, 0U},
   {str, "ello", 0, 1U, 1U},
   {str, "zz", 0, StringRef::npos, StringRef::npos},
   {str, "ll", 2U, 2U, 2U},
   {str, "ll", 3U, StringRef::npos, 7U},
   {str, "LL", 2U, 7U, 2U},
   {str, "LL", 3U, 7U, 7U},
   {str, "", 0U, 0U, 0U},
   {longStr, "hello", 0U, 36U, 36U},
   {longStr, "foo", 0U, 28U, 28U},
   {longStr, "hell", 2U, 12U, 12U},
   {longStr, "HELL", 2U, 42U, 12U},
   {longStr, "", 0U, 0U, 0U}};

   for (auto &E : CharExpectations) {
      EXPECT_EQ(E.Pos, E.str.find(E.C, E.From));
      EXPECT_EQ(E.LowerPos, E.str.findLower(E.C, E.From));
      EXPECT_EQ(E.LowerPos, E.str.findLower(toupper(E.C), E.From));
   }

   for (auto &E : StrExpectations) {
      EXPECT_EQ(E.Pos, E.str.find(E.S, E.From));
      EXPECT_EQ(E.LowerPos, E.str.findLower(E.S, E.From));
      EXPECT_EQ(E.LowerPos, E.str.findLower(E.S.toUpper(), E.From));
   }

   EXPECT_EQ(3U, str.rfind('l'));
   EXPECT_EQ(StringRef::npos, str.rfind('z'));
   EXPECT_EQ(StringRef::npos, str.rfind("helloworld"));
   EXPECT_EQ(0U, str.rfind("hello"));
   EXPECT_EQ(1U, str.rfind("ello"));
   EXPECT_EQ(StringRef::npos, str.rfind("zz"));

   EXPECT_EQ(8U, str.rfindLower('l'));
   EXPECT_EQ(8U, str.rfindLower('L'));
   EXPECT_EQ(StringRef::npos, str.rfindLower('z'));
   EXPECT_EQ(StringRef::npos, str.rfindLower("HELLOWORLD"));
   EXPECT_EQ(5U, str.rfind("HELLO"));
   EXPECT_EQ(6U, str.rfind("ELLO"));
   EXPECT_EQ(StringRef::npos, str.rfind("ZZ"));

   EXPECT_EQ(2U, str.findFirstOf('l'));
   EXPECT_EQ(1U, str.findFirstOf("el"));
   EXPECT_EQ(StringRef::npos, str.findFirstOf("xyz"));

   str = "hello";
   EXPECT_EQ(1U, str.findFirstNotOf('h'));
   EXPECT_EQ(4U, str.findFirstNotOf("hel"));
   EXPECT_EQ(StringRef::npos, str.findFirstNotOf("hello"));

   EXPECT_EQ(3U, str.findLastNotOf('o'));
   EXPECT_EQ(1U, str.findLastNotOf("lo"));
   EXPECT_EQ(StringRef::npos, str.findLastNotOf("helo"));
}

TEST(StringRefTest, testCount)
{
   StringRef str("hello");
   EXPECT_EQ(2U, str.count('l'));
   EXPECT_EQ(1U, str.count('o'));
   EXPECT_EQ(0U, str.count('z'));
   EXPECT_EQ(0U, str.count("helloworld"));
   EXPECT_EQ(1U, str.count("hello"));
   EXPECT_EQ(1U, str.count("ello"));
   EXPECT_EQ(0U, str.count("zz"));
}

TEST(StringRefTest, testEditDistance)
{
   StringRef hello("hello");
   EXPECT_EQ(2U, hello.editDistance("hill"));

   StringRef Industry("industry");
   EXPECT_EQ(6U, Industry.editDistance("interest"));

   StringRef soylent("soylent green is people");
   EXPECT_EQ(19U, soylent.editDistance("people soiled our green"));
   EXPECT_EQ(26U, soylent.editDistance("people soiled our green",
                                       /* allow replacements = */ false));
   EXPECT_EQ(9U, soylent.editDistance("people soiled our green",
                                      /* allow replacements = */ true,
                                      /* max edit distance = */ 8));
   EXPECT_EQ(53U, soylent.editDistance("people soiled our green "
                                       "people soiled our green "
                                       "people soiled our green "));
}

TEST(StringRefTest, testMisc)
{

   std::string storage;
   RawStringOutStream outstream(storage);
   outstream << StringRef("hello");
   EXPECT_EQ("hello", outstream.getStr());
}

TEST(StringRefTest, testHashing)
{
   EXPECT_EQ(hash_value(std::string()), hash_value(StringRef()));
   EXPECT_EQ(hash_value(std::string()), hash_value(StringRef("")));
   std::string S = "hello world";
   HashCode H = hash_value(S);
   EXPECT_EQ(H, hash_value(StringRef("hello world")));
   EXPECT_EQ(H, hash_value(StringRef(S)));
   EXPECT_NE(H, hash_value(StringRef("hello worl")));
   EXPECT_EQ(hash_value(std::string("hello worl")),
             hash_value(StringRef("hello worl")));
   EXPECT_NE(H, hash_value(StringRef("hello world ")));
   EXPECT_EQ(hash_value(std::string("hello world ")),
             hash_value(StringRef("hello world ")));
   EXPECT_EQ(H, hash_value(StringRef("hello world\0")));
   EXPECT_NE(hash_value(std::string("ello worl")),
             hash_value(StringRef("hello world").slice(1, -1)));
}

struct UnsignedPair {
   const char *str;
   uint64_t Expected;
} sg_unsigned[] =
{ {"0", 0},
{"255", 255},
{"256", 256},
{"65535", 65535},
{"65536", 65536},
{"4294967295", 4294967295ULL},
{"4294967296", 4294967296ULL},
{"18446744073709551615", 18446744073709551615ULL},
{"042", 34},
{"0x42", 66},
{"0b101010", 42}
};

struct SignedPair {
   const char *str;
   int64_t Expected;
} sg_signed[] =
{
{"0", 0},
{"-0", 0},
{"127", 127},
{"128", 128},
{"-128", -128},
{"-129", -129},
{"32767", 32767},
{"32768", 32768},
{"-32768", -32768},
{"-32769", -32769},
{"2147483647", 2147483647LL},
{"2147483648", 2147483648LL},
{"-2147483648", -2147483648LL},
{"-2147483649", -2147483649LL},
{"-9223372036854775808", -(9223372036854775807LL) - 1},
{"042", 34},
{"0x42", 66},
{"0b101010", 42},
{"-042", -34},
{"-0x42", -66},
{"-0b101010", -42}
};

TEST(StringRefTest, getAsInteger)
{
   uint8_t U8;
   uint16_t U16;
   uint32_t U32;
   uint64_t U64;

   for (size_t i = 0; i < array_lengthof(sg_unsigned); ++i) {
      bool U8Success = StringRef(sg_unsigned[i].str).getAsInteger(0, U8);
      if (static_cast<uint8_t>(sg_unsigned[i].Expected) == sg_unsigned[i].Expected) {
         ASSERT_FALSE(U8Success);
         EXPECT_EQ(U8, sg_unsigned[i].Expected);
      } else {
         ASSERT_TRUE(U8Success);
      }
      bool U16Success = StringRef(sg_unsigned[i].str).getAsInteger(0, U16);
      if (static_cast<uint16_t>(sg_unsigned[i].Expected) == sg_unsigned[i].Expected) {
         ASSERT_FALSE(U16Success);
         EXPECT_EQ(U16, sg_unsigned[i].Expected);
      } else {
         ASSERT_TRUE(U16Success);
      }
      bool U32Success = StringRef(sg_unsigned[i].str).getAsInteger(0, U32);
      if (static_cast<uint32_t>(sg_unsigned[i].Expected) == sg_unsigned[i].Expected) {
         ASSERT_FALSE(U32Success);
         EXPECT_EQ(U32, sg_unsigned[i].Expected);
      } else {
         ASSERT_TRUE(U32Success);
      }
      bool U64Success = StringRef(sg_unsigned[i].str).getAsInteger(0, U64);
      if (static_cast<uint64_t>(sg_unsigned[i].Expected) == sg_unsigned[i].Expected) {
         ASSERT_FALSE(U64Success);
         EXPECT_EQ(U64, sg_unsigned[i].Expected);
      } else {
         ASSERT_TRUE(U64Success);
      }
   }

   int8_t S8;
   int16_t S16;
   int32_t S32;
   int64_t S64;

   for (size_t i = 0; i < array_lengthof(sg_signed); ++i) {
      bool S8Success = StringRef(sg_signed[i].str).getAsInteger(0, S8);
      if (static_cast<int8_t>(sg_signed[i].Expected) == sg_signed[i].Expected) {
         ASSERT_FALSE(S8Success);
         EXPECT_EQ(S8, sg_signed[i].Expected);
      } else {
         ASSERT_TRUE(S8Success);
      }
      bool S16Success = StringRef(sg_signed[i].str).getAsInteger(0, S16);
      if (static_cast<int16_t>(sg_signed[i].Expected) == sg_signed[i].Expected) {
         ASSERT_FALSE(S16Success);
         EXPECT_EQ(S16, sg_signed[i].Expected);
      } else {
         ASSERT_TRUE(S16Success);
      }
      bool S32Success = StringRef(sg_signed[i].str).getAsInteger(0, S32);
      if (static_cast<int32_t>(sg_signed[i].Expected) == sg_signed[i].Expected) {
         ASSERT_FALSE(S32Success);
         EXPECT_EQ(S32, sg_signed[i].Expected);
      } else {
         ASSERT_TRUE(S32Success);
      }
      bool S64Success = StringRef(sg_signed[i].str).getAsInteger(0, S64);
      if (static_cast<int64_t>(sg_signed[i].Expected) == sg_signed[i].Expected) {
         ASSERT_FALSE(S64Success);
         EXPECT_EQ(S64, sg_signed[i].Expected);
      } else {
         ASSERT_TRUE(S64Success);
      }
   }
}


static const char* BadStrings[] = {
   ""                      // empty string
   , "18446744073709551617"  // value just over max
   , "123456789012345678901" // value way too large
   , "4t23v"                 // illegal decimal characters
   , "0x123W56"              // illegal hex characters
   , "0b2"                   // illegal bin characters
   , "08"                    // illegal oct characters
   , "0o8"                   // illegal oct characters
   , "-123"                  // negative unsigned value
   , "0x"
   , "0b"
};

TEST(StringRefTest, testGetAsUnsignedIntegerBadStrings)
{
   unsigned long long U64;
   for (size_t i = 0; i < array_lengthof(BadStrings); ++i) {
      bool isBadNumber = StringRef(BadStrings[i]).getAsInteger(0, U64);
      ASSERT_TRUE(isBadNumber);
   }
}

struct ConsumeUnsignedPair {
   const char *Str;
   uint64_t Expected;
   const char *Leftover;
} sg_consumeUnsigned[] = {
{"0", 0, ""},
{"255", 255, ""},
{"256", 256, ""},
{"65535", 65535, ""},
{"65536", 65536, ""},
{"4294967295", 4294967295ULL, ""},
{"4294967296", 4294967296ULL, ""},
{"255A376", 255, "A376"},
{"18446744073709551615", 18446744073709551615ULL, ""},
{"18446744073709551615ABC", 18446744073709551615ULL, "ABC"},
{"042", 34, ""},
{"0x42", 66, ""},
{"0x42-0x34", 66, "-0x34"},
{"0b101010", 42, ""},
{"0429F", 042, "9F"},            // Auto-sensed octal radix, invalid digit
{"0x42G12", 0x42, "G12"},        // Auto-sensed hex radix, invalid digit
{"0b10101020101", 42, "20101"}}; // Auto-sensed binary radix, invalid digit.

struct ConsumeSignedPair {
   const char *Str;
   int64_t Expected;
   const char *Leftover;
} sg_consumeSigned[] = {
{"0", 0, ""},
{"-0", 0, ""},
{"0-1", 0, "-1"},
{"-0-1", 0, "-1"},
{"127", 127, ""},
{"128", 128, ""},
{"127-1", 127, "-1"},
{"128-1", 128, "-1"},
{"-128", -128, ""},
{"-129", -129, ""},
{"-128-1", -128, "-1"},
{"-129-1", -129, "-1"},
{"32767", 32767, ""},
{"32768", 32768, ""},
{"32767-1", 32767, "-1"},
{"32768-1", 32768, "-1"},
{"-32768", -32768, ""},
{"-32769", -32769, ""},
{"-32768-1", -32768, "-1"},
{"-32769-1", -32769, "-1"},
{"2147483647", 2147483647LL, ""},
{"2147483648", 2147483648LL, ""},
{"2147483647-1", 2147483647LL, "-1"},
{"2147483648-1", 2147483648LL, "-1"},
{"-2147483648", -2147483648LL, ""},
{"-2147483649", -2147483649LL, ""},
{"-2147483648-1", -2147483648LL, "-1"},
{"-2147483649-1", -2147483649LL, "-1"},
{"-9223372036854775808", -(9223372036854775807LL) - 1, ""},
{"-9223372036854775808-1", -(9223372036854775807LL) - 1, "-1"},
{"042", 34, ""},
{"042-1", 34, "-1"},
{"0x42", 66, ""},
{"0x42-1", 66, "-1"},
{"0b101010", 42, ""},
{"0b101010-1", 42, "-1"},
{"-042", -34, ""},
{"-042-1", -34, "-1"},
{"-0x42", -66, ""},
{"-0x42-1", -66, "-1"},
{"-0b101010", -42, ""},
{"-0b101010-1", -42, "-1"}};

TEST(StringRefTest, testConsumeIntegerUnsigned)
{
   uint8_t U8;
   uint16_t U16;
   uint32_t U32;
   uint64_t U64;

   for (size_t i = 0; i < array_lengthof(sg_consumeUnsigned); ++i) {
      StringRef str = sg_consumeUnsigned[i].Str;
      bool U8Success = str.consumeInteger(0, U8);
      if (static_cast<uint8_t>(sg_consumeUnsigned[i].Expected) ==
          sg_consumeUnsigned[i].Expected) {
         ASSERT_FALSE(U8Success);
         EXPECT_EQ(U8, sg_consumeUnsigned[i].Expected);
         EXPECT_EQ(str, sg_consumeUnsigned[i].Leftover);
      } else {
         ASSERT_TRUE(U8Success);
      }

      str = sg_consumeUnsigned[i].Str;
      bool U16Success = str.consumeInteger(0, U16);
      if (static_cast<uint16_t>(sg_consumeUnsigned[i].Expected) ==
          sg_consumeUnsigned[i].Expected) {
         ASSERT_FALSE(U16Success);
         EXPECT_EQ(U16, sg_consumeUnsigned[i].Expected);
         EXPECT_EQ(str, sg_consumeUnsigned[i].Leftover);
      } else {
         ASSERT_TRUE(U16Success);
      }

      str = sg_consumeUnsigned[i].Str;
      bool U32Success = str.consumeInteger(0, U32);
      if (static_cast<uint32_t>(sg_consumeUnsigned[i].Expected) ==
          sg_consumeUnsigned[i].Expected) {
         ASSERT_FALSE(U32Success);
         EXPECT_EQ(U32, sg_consumeUnsigned[i].Expected);
         EXPECT_EQ(str, sg_consumeUnsigned[i].Leftover);
      } else {
         ASSERT_TRUE(U32Success);
      }

      str = sg_consumeUnsigned[i].Str;
      bool U64Success = str.consumeInteger(0, U64);
      if (static_cast<uint64_t>(sg_consumeUnsigned[i].Expected) ==
          sg_consumeUnsigned[i].Expected) {
         ASSERT_FALSE(U64Success);
         EXPECT_EQ(U64, sg_consumeUnsigned[i].Expected);
         EXPECT_EQ(str, sg_consumeUnsigned[i].Leftover);
      } else {
         ASSERT_TRUE(U64Success);
      }
   }
}

TEST(StringRefTest, testConsumeIntegerSigned)
{
   int8_t S8;
   int16_t S16;
   int32_t S32;
   int64_t S64;

   for (size_t i = 0; i < array_lengthof(sg_consumeSigned); ++i) {
      StringRef str = sg_consumeSigned[i].Str;
      bool S8Success = str.consumeInteger(0, S8);
      if (static_cast<int8_t>(sg_consumeSigned[i].Expected) ==
          sg_consumeSigned[i].Expected) {
         ASSERT_FALSE(S8Success);
         EXPECT_EQ(S8, sg_consumeSigned[i].Expected);
         EXPECT_EQ(str, sg_consumeSigned[i].Leftover);
      } else {
         ASSERT_TRUE(S8Success);
      }

      str = sg_consumeSigned[i].Str;
      bool S16Success = str.consumeInteger(0, S16);
      if (static_cast<int16_t>(sg_consumeSigned[i].Expected) ==
          sg_consumeSigned[i].Expected) {
         ASSERT_FALSE(S16Success);
         EXPECT_EQ(S16, sg_consumeSigned[i].Expected);
         EXPECT_EQ(str, sg_consumeSigned[i].Leftover);
      } else {
         ASSERT_TRUE(S16Success);
      }

      str = sg_consumeSigned[i].Str;
      bool S32Success = str.consumeInteger(0, S32);
      if (static_cast<int32_t>(sg_consumeSigned[i].Expected) ==
          sg_consumeSigned[i].Expected) {
         ASSERT_FALSE(S32Success);
         EXPECT_EQ(S32, sg_consumeSigned[i].Expected);
         EXPECT_EQ(str, sg_consumeSigned[i].Leftover);
      } else {
         ASSERT_TRUE(S32Success);
      }

      str = sg_consumeSigned[i].Str;
      bool S64Success = str.consumeInteger(0, S64);
      if (static_cast<int64_t>(sg_consumeSigned[i].Expected) ==
          sg_consumeSigned[i].Expected) {
         ASSERT_FALSE(S64Success);
         EXPECT_EQ(S64, sg_consumeSigned[i].Expected);
         EXPECT_EQ(str, sg_consumeSigned[i].Leftover);
      } else {
         ASSERT_TRUE(S64Success);
      }
   }
}

struct GetDoubleStrings {
   const char *str;
   bool AllowInexact;
   bool ShouldFail;
   double D;
} sg_doubleStrings[] = {{"0", false, false, 0.0},
{"0.0", false, false, 0.0},
{"-0.0", false, false, -0.0},
{"123.45", false, true, 123.45},
{"123.45", true, false, 123.45},
{"1.8e308", true, false, std::numeric_limits<double>::infinity()},
{"1.8e308", false, true, std::numeric_limits<double>::infinity()},
{"0x0.0000000000001P-1023", false, true, 0.0},
{"0x0.0000000000001P-1023", true, false, 0.0},
};

TEST(StringRefTest, testGetAsDouble)
{
   for (const auto &entry : sg_doubleStrings) {
      double Result;
      StringRef S(entry.str);
      EXPECT_EQ(entry.ShouldFail, S.getAsDouble(Result, entry.AllowInexact));
      if (!entry.ShouldFail) {
         EXPECT_EQ(Result, entry.D);
      }
   }
}

static const char *join_input[] = { "a", "b", "c" };
static const char join_result1[] = "a";
static const char join_result2[] = "a:b:c";
static const char join_result3[] = "a::b::c";

TEST(StringRefTest, testJoinStrings)
{
   std::vector<StringRef> v1;
   std::vector<std::string> v2;
   for (size_t i = 0; i < array_lengthof(join_input); ++i) {
      v1.push_back(join_input[i]);
      v2.push_back(join_input[i]);
   }

   bool v1_join1 = join(v1.begin(), v1.begin() + 1, ":") == join_result1;
   EXPECT_TRUE(v1_join1);
   bool v1_join2 = join(v1.begin(), v1.end(), ":") == join_result2;
   EXPECT_TRUE(v1_join2);
   bool v1_join3 = join(v1.begin(), v1.end(), "::") == join_result3;
   EXPECT_TRUE(v1_join3);

   bool v2_join1 = join(v2.begin(), v2.begin() + 1, ":") == join_result1;
   EXPECT_TRUE(v2_join1);
   bool v2_join2 = join(v2.begin(), v2.end(), ":") == join_result2;
   EXPECT_TRUE(v2_join2);
   bool v2_join3 = join(v2.begin(), v2.end(), "::") == join_result3;
   EXPECT_TRUE(v2_join3);
   v2_join3 = join(v2, "::") == join_result3;
   EXPECT_TRUE(v2_join3);
}

TEST(StringRefTest, testAllocatorCopy)
{
   BumpPtrAllocator alloc;
   // First test empty strings.  We don't want these to allocate anything on the
   // allocator.
   StringRef strEmpty = "";
   StringRef StrEmptyc = strEmpty.copy(alloc);
   EXPECT_TRUE(strEmpty.equals(StrEmptyc));
   EXPECT_EQ(StrEmptyc.getData(), nullptr);
   EXPECT_EQ(StrEmptyc.size(), 0u);
   EXPECT_EQ(alloc.getTotalMemory(), 0u);

   StringRef str1 = "hello";
   StringRef str2 = "bye";
   StringRef str1c = str1.copy(alloc);
   StringRef Str2c = str2.copy(alloc);
   EXPECT_TRUE(str1.equals(str1c));
   EXPECT_NE(str1.getData(), str1c.getData());
   EXPECT_TRUE(str2.equals(Str2c));
   EXPECT_NE(str2.getData(), Str2c.getData());
}

TEST(StringRefTest, testDrop) {
   StringRef test("StringRefTest::Drop");

   StringRef dropped = test.dropFront(5);
   EXPECT_EQ(dropped, "gRefTest::Drop");

   dropped = test.dropBack(5);
   EXPECT_EQ(dropped, "StringRefTest:");

   dropped = test.dropFront(0);
   EXPECT_EQ(dropped, test);

   dropped = test.dropBack(0);
   EXPECT_EQ(dropped, test);

   dropped = test.dropFront(test.size());
   EXPECT_TRUE(dropped.empty());

   dropped = test.dropBack(test.size());
   EXPECT_TRUE(dropped.empty());
}

TEST(StringRefTest, testTake)
{
   StringRef test("StringRefTest::Take");

   StringRef taken = test.takeFront(5);
   EXPECT_EQ(taken, "Strin");

   taken = test.takeBack(5);
   EXPECT_EQ(taken, ":Take");

   taken = test.takeFront(test.size());
   EXPECT_EQ(taken, test);

   taken = test.takeBack(test.size());
   EXPECT_EQ(taken, test);

   taken = test.takeFront(0);
   EXPECT_TRUE(taken.empty());

   taken = test.takeBack(0);
   EXPECT_TRUE(taken.empty());
}

TEST(StringRefTest, testFindIf)
{
   StringRef punct("test.String");
   StringRef noPunct("ABCDEFG");
   StringRef empty;

   auto isPunct = [](char c) { return ::ispunct(c); };
   auto IsAlpha = [](char c) { return ::isalpha(c); };
   EXPECT_EQ(4U, punct.findIf(isPunct));
   EXPECT_EQ(StringRef::npos, noPunct.findIf(isPunct));
   EXPECT_EQ(StringRef::npos, empty.findIf(isPunct));

   EXPECT_EQ(4U, punct.findIfNot(IsAlpha));
   EXPECT_EQ(StringRef::npos, noPunct.findIfNot(IsAlpha));
   EXPECT_EQ(StringRef::npos, empty.findIfNot(IsAlpha));
}

TEST(StringRefTest, testTakeWhileUntil)
{
   StringRef test("String With 1 Number");

   StringRef taken = test.takeWhile([](char c) { return ::isdigit(c); });
   EXPECT_EQ("", taken);

   taken = test.takeUntil([](char c) { return ::isdigit(c); });
   EXPECT_EQ("String With ", taken);

   taken = test.takeWhile([](char c) { return true; });
   EXPECT_EQ(test, taken);

   taken = test.takeUntil([](char c) { return true; });
   EXPECT_EQ("", taken);

   test = "";
   taken = test.takeWhile([](char c) { return true; });
   EXPECT_EQ("", taken);
}

TEST(StringRefTest, testDropWhileUntil)
{
   StringRef test("String With 1 Number");

   StringRef taken = test.dropWhile([](char c) { return ::isdigit(c); });
   EXPECT_EQ(test, taken);

   taken = test.dropUntil([](char c) { return ::isdigit(c); });
   EXPECT_EQ("1 Number", taken);

   taken = test.dropWhile([](char c) { return true; });
   EXPECT_EQ("", taken);

   taken = test.dropUntil([](char c) { return true; });
   EXPECT_EQ(test, taken);

   StringRef EmptyString = "";
   taken = EmptyString.dropWhile([](char c) { return true; });
   EXPECT_EQ("", taken);
}

TEST(StringRefTest, testStringLiteral)
{
   constexpr StringLiteral Strings[] = {"Foo", "Bar"};
   EXPECT_EQ(StringRef("Foo"), Strings[0]);
   EXPECT_EQ(StringRef("Bar"), Strings[1]);
}
