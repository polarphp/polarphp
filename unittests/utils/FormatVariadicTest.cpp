// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors

// Created by polarboy on 2018/07/10.

#include "polarphp/utils/FormatVariadic.h"
#include "polarphp/utils/FormatAdapters.h"
#include "polarphp/utils/Error.h"
#include "gtest/gtest.h"

using namespace polar::utils;
using namespace polar::basic;

namespace {

struct Format : public FormatAdapter<int>
{
   Format(int N) : FormatAdapter<int>(std::move(N)) {}
   void format(RawOutStream &outstream, StringRef Opt) override { outstream << "Format"; }
};

using utils::internal::UsesFormatMember;
using utils::internal::UsesMissingProvider;

static_assert(UsesFormatMember<Format>::value, "");
static_assert(UsesFormatMember<Format &>::value, "");
static_assert(UsesFormatMember<Format &&>::value, "");
static_assert(UsesFormatMember<const Format>::value, "");
static_assert(UsesFormatMember<const Format &>::value, "");
static_assert(UsesFormatMember<const volatile Format>::value, "");
static_assert(UsesFormatMember<const volatile Format &>::value, "");

struct NoFormat {};
static_assert(UsesMissingProvider<NoFormat>::value, "");

TEST(FormatVariadicTest, testEmptyFormatString)
{
   auto replacements = FormatvObjectBase::parseFormatString("");
   EXPECT_EQ(0U, replacements.size());
}

TEST(FormatVariadicTest, testNoReplacements)
{
   const StringRef kFormatString = "This is a test";
   auto replacements = FormatvObjectBase::parseFormatString(kFormatString);
   ASSERT_EQ(1U, replacements.size());
   EXPECT_EQ(kFormatString, replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Literal, replacements[0].m_type);
}

TEST(FormatVariadicTest, testEscapedBrace)
{
   // {{ should be replaced with {
   auto replacements = FormatvObjectBase::parseFormatString("{{");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("{", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Literal, replacements[0].m_type);

   // An even number N of braces should be replaced with N/2 braces.
   replacements = FormatvObjectBase::parseFormatString("{{{{{{");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("{{{", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Literal, replacements[0].m_type);
}

TEST(FormatVariadicTest, testValidReplacementSequence)
{
   // 1. Simple replacement - parameter index only
   auto replacements = FormatvObjectBase::parseFormatString("{0}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(0u, replacements[0].m_align);
   EXPECT_EQ("", replacements[0].m_options);

   replacements = FormatvObjectBase::parseFormatString("{1}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(1u, replacements[0].m_index);
   EXPECT_EQ(0u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ("", replacements[0].m_options);

   // 2. Parameter index with right alignment
   replacements = FormatvObjectBase::parseFormatString("{0,3}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(3u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ("", replacements[0].m_options);

   // 3. And left alignment
   replacements = FormatvObjectBase::parseFormatString("{0,-3}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(3u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Left, replacements[0].m_where);
   EXPECT_EQ("", replacements[0].m_options);

   // 4. And center alignment
   replacements = FormatvObjectBase::parseFormatString("{0,=3}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(3u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Center, replacements[0].m_where);
   EXPECT_EQ("", replacements[0].m_options);

   // 4. Parameter index with option string
   replacements = FormatvObjectBase::parseFormatString("{0:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(0u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ("foo", replacements[0].m_options);

   // 5. Parameter index with alignment before option string
   replacements = FormatvObjectBase::parseFormatString("{0,-3:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(3u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Left, replacements[0].m_where);
   EXPECT_EQ("foo", replacements[0].m_options);

   // 7. Parameter indices, options, and alignment can all have whitespace.
   replacements = FormatvObjectBase::parseFormatString("{ 0, -3 : foo }");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(3u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Left, replacements[0].m_where);
   EXPECT_EQ("foo", replacements[0].m_options);

   // 8. Everything after the first option specifier is part of the style, even
   // if it contains another option specifier.
   replacements = FormatvObjectBase::parseFormatString("{0:0:1}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("0:0:1", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(0u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ("0:1", replacements[0].m_options);

   // 9. Custom padding character
   replacements = FormatvObjectBase::parseFormatString("{0,p+4:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("0,p+4:foo", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(4u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ('p', replacements[0].m_pad);
   EXPECT_EQ("foo", replacements[0].m_options);

   // Format string special characters are allowed as padding character
   replacements = FormatvObjectBase::parseFormatString("{0,-+4:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("0,-+4:foo", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(4u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ('-', replacements[0].m_pad);
   EXPECT_EQ("foo", replacements[0].m_options);

   replacements = FormatvObjectBase::parseFormatString("{0,+-4:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("0,+-4:foo", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(4u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Left, replacements[0].m_where);
   EXPECT_EQ('+', replacements[0].m_pad);
   EXPECT_EQ("foo", replacements[0].m_options);

   replacements = FormatvObjectBase::parseFormatString("{0,==4:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("0,==4:foo", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(4u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Center, replacements[0].m_where);
   EXPECT_EQ('=', replacements[0].m_pad);
   EXPECT_EQ("foo", replacements[0].m_options);

   replacements = FormatvObjectBase::parseFormatString("{0,:=4:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ("0,:=4:foo", replacements[0].m_spec);
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(4u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Center, replacements[0].m_where);
   EXPECT_EQ(':', replacements[0].m_pad);
   EXPECT_EQ("foo", replacements[0].m_options);
}

TEST(FormatVariadicTest, testDefaultReplacementValues)
{
   // 2. If options string is missing, it defaults to empty.
   auto replacements = FormatvObjectBase::parseFormatString("{0,3}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(3u, replacements[0].m_align);
   EXPECT_EQ("", replacements[0].m_options);

   // Including if the colon is present but contains no text.
   replacements = FormatvObjectBase::parseFormatString("{0,3:}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(3u, replacements[0].m_align);
   EXPECT_EQ("", replacements[0].m_options);

   // 3. If alignment is missing, it defaults to 0, right, space
   replacements = FormatvObjectBase::parseFormatString("{0:foo}");
   ASSERT_EQ(1u, replacements.size());
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ(' ', replacements[0].m_pad);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(0u, replacements[0].m_align);
   EXPECT_EQ("foo", replacements[0].m_options);
}

TEST(FormatVariadicTest, testMultipleReplacements)
{
   auto replacements =
         FormatvObjectBase::parseFormatString("{0} {1:foo}-{2,-3:bar}");
   ASSERT_EQ(5u, replacements.size());
   // {0}
   EXPECT_EQ(ReplacementType::Format, replacements[0].m_type);
   EXPECT_EQ(0u, replacements[0].m_index);
   EXPECT_EQ(0u, replacements[0].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[0].m_where);
   EXPECT_EQ("", replacements[0].m_options);

   // " "
   EXPECT_EQ(ReplacementType::Literal, replacements[1].m_type);
   EXPECT_EQ(" ", replacements[1].m_spec);

   // {1:foo} - Options=foo
   EXPECT_EQ(ReplacementType::Format, replacements[2].m_type);
   EXPECT_EQ(1u, replacements[2].m_index);
   EXPECT_EQ(0u, replacements[2].m_align);
   EXPECT_EQ(AlignStyle::Right, replacements[2].m_where);
   EXPECT_EQ("foo", replacements[2].m_options);

   // "-"
   EXPECT_EQ(ReplacementType::Literal, replacements[3].m_type);
   EXPECT_EQ("-", replacements[3].m_spec);

   // {2:bar,-3} - Options=bar, Align=-3
   EXPECT_EQ(ReplacementType::Format, replacements[4].m_type);
   EXPECT_EQ(2u, replacements[4].m_index);
   EXPECT_EQ(3u, replacements[4].m_align);
   EXPECT_EQ(AlignStyle::Left, replacements[4].m_where);
   EXPECT_EQ("bar", replacements[4].m_options);
}

TEST(FormatVariadicTest, testFormatNoReplacements)
{
   EXPECT_EQ("", formatv("").getStr());
   EXPECT_EQ("Test", formatv("Test").getStr());
}

TEST(FormatVariadicTest, testFormatBasicTypesOneReplacement)
{
   EXPECT_EQ("1", formatv("{0}", 1).getStr());
   EXPECT_EQ("c", formatv("{0}", 'c').getStr());
   EXPECT_EQ("-3", formatv("{0}", -3).getStr());
   EXPECT_EQ("Test", formatv("{0}", "Test").getStr());
   EXPECT_EQ("Test2", formatv("{0}", StringRef("Test2")).getStr());
   EXPECT_EQ("Test3", formatv("{0}", std::string("Test3")).getStr());
}

TEST(FormatVariadicTest, testIntegralHexFormatting)
{
   // 1. Trivial cases.  Make sure hex is not the default.
   EXPECT_EQ("0", formatv("{0}", 0).getStr());
   EXPECT_EQ("2748", formatv("{0}", 0xABC).getStr());
   EXPECT_EQ("-2748", formatv("{0}", -0xABC).getStr());

   // 3. various hex prefixes.
   EXPECT_EQ("0xFF", formatv("{0:X}", 255).getStr());
   EXPECT_EQ("0xFF", formatv("{0:X+}", 255).getStr());
   EXPECT_EQ("0xff", formatv("{0:x}", 255).getStr());
   EXPECT_EQ("0xff", formatv("{0:x+}", 255).getStr());
   EXPECT_EQ("FF", formatv("{0:X-}", 255).getStr());
   EXPECT_EQ("ff", formatv("{0:x-}", 255).getStr());

   // 5. Precision pads left of the most significant digit but right of the
   // prefix (if one exists).
   EXPECT_EQ("0xFF", formatv("{0:X2}", 255).getStr());
   EXPECT_EQ("0xFF", formatv("{0:X+2}", 255).getStr());
   EXPECT_EQ("0x0ff", formatv("{0:x3}", 255).getStr());
   EXPECT_EQ("0x0ff", formatv("{0:x+3}", 255).getStr());
   EXPECT_EQ("00FF", formatv("{0:X-4}", 255).getStr());
   EXPECT_EQ("00ff", formatv("{0:x-4}", 255).getStr());

   // 6. Try some larger types.
   EXPECT_EQ("0xDEADBEEFDEADBEEF",
             formatv("{0:X16}", -2401053088876216593LL).getStr());
   EXPECT_EQ("0xFEEBDAEDFEEBDAED",
             formatv("{0:X16}", 0xFEEBDAEDFEEBDAEDULL).getStr());
   EXPECT_EQ("0x00000000DEADBEEF", formatv("{0:X16}", 0xDEADBEEF).getStr());

   // 7. Padding should take into account the prefix
   EXPECT_EQ("0xff", formatv("{0,4:x}", 255).getStr());
   EXPECT_EQ(" 0xff", formatv("{0,5:x+}", 255).getStr());
   EXPECT_EQ("  FF", formatv("{0,4:X-}", 255).getStr());
   EXPECT_EQ("   ff", formatv("{0,5:x-}", 255).getStr());

   // 8. Including when it's been zero-padded
   EXPECT_EQ("  0x0ff", formatv("{0,7:x3}", 255).getStr());
   EXPECT_EQ(" 0x00ff", formatv("{0,7:x+4}", 255).getStr());
   EXPECT_EQ("  000FF", formatv("{0,7:X-5}", 255).getStr());
   EXPECT_EQ(" 0000ff", formatv("{0,7:x-6}", 255).getStr());

   // 9. Precision with default format specifier should work too
   EXPECT_EQ("    255", formatv("{0,7:3}", 255).getStr());
   EXPECT_EQ("   0255", formatv("{0,7:4}", 255).getStr());
   EXPECT_EQ("  00255", formatv("{0,7:5}", 255).getStr());
   EXPECT_EQ(" 000255", formatv("{0,7:6}", 255).getStr());
}

TEST(FormatVariadicTest, testPointerFormatting)
{
   // 1. Trivial cases.  Hex is default.  Default Precision is pointer width.
   if (sizeof(void *) == 4) {
      EXPECT_EQ("0x00000000", formatv("{0}", (void *)0).getStr());
      EXPECT_EQ("0x00000ABC", formatv("{0}", (void *)0xABC).getStr());
   } else {
      EXPECT_EQ("0x0000000000000000", formatv("{0}", (void *)0).getStr());
      EXPECT_EQ("0x0000000000000ABC", formatv("{0}", (void *)0xABC).getStr());
   }

   // 2. But we can reduce the precision explicitly.
   EXPECT_EQ("0x0", formatv("{0:0}", (void *)0).getStr());
   EXPECT_EQ("0xABC", formatv("{0:0}", (void *)0xABC).getStr());
   EXPECT_EQ("0x0000", formatv("{0:4}", (void *)0).getStr());
   EXPECT_EQ("0x0ABC", formatv("{0:4}", (void *)0xABC).getStr());

   // 3. various hex prefixes.
   EXPECT_EQ("0x0ABC", formatv("{0:X4}", (void *)0xABC).getStr());
   EXPECT_EQ("0x0abc", formatv("{0:x4}", (void *)0xABC).getStr());
   EXPECT_EQ("0ABC", formatv("{0:X-4}", (void *)0xABC).getStr());
   EXPECT_EQ("0abc", formatv("{0:x-4}", (void *)0xABC).getStr());
}

TEST(FormatVariadicTest, testIntegralNumberFormatting)
{
   // 1. Test comma grouping with default widths and precisions.
   EXPECT_EQ("0", formatv("{0:N}", 0).getStr());
   EXPECT_EQ("10", formatv("{0:N}", 10).getStr());
   EXPECT_EQ("100", formatv("{0:N}", 100).getStr());
   EXPECT_EQ("1,000", formatv("{0:N}", 1000).getStr());
   EXPECT_EQ("1,234,567,890", formatv("{0:N}", 1234567890).getStr());
   EXPECT_EQ("-10", formatv("{0:N}", -10).getStr());
   EXPECT_EQ("-100", formatv("{0:N}", -100).getStr());
   EXPECT_EQ("-1,000", formatv("{0:N}", -1000).getStr());
   EXPECT_EQ("-1,234,567,890", formatv("{0:N}", -1234567890).getStr());

   // 2. If there is no comma, width and precision pad to the same absolute
   // size.
   EXPECT_EQ(" 1", formatv("{0,2:N}", 1).getStr());

   // 3. But if there is a comma or negative sign, width factors them in but
   // precision doesn't.
   EXPECT_EQ(" 1,000", formatv("{0,6:N}", 1000).getStr());
   EXPECT_EQ(" -1,000", formatv("{0,7:N}", -1000).getStr());

   // 4. Large widths all line up.
   EXPECT_EQ("      1,000", formatv("{0,11:N}", 1000).getStr());
   EXPECT_EQ("     -1,000", formatv("{0,11:N}", -1000).getStr());
   EXPECT_EQ("   -100,000", formatv("{0,11:N}", -100000).getStr());
}

TEST(FormatVariadicTest, testStringFormatting)
{
   const char FooArray[] = "FooArray";
   const char *FooPtr = "FooPtr";
   StringRef FooRef("FooRef");
   constexpr StringLiteral FooLiteral("FooLiteral");
   std::string FooString("FooString");
   // 1. Test that we can print various types of strings.
   EXPECT_EQ(FooArray, formatv("{0}", FooArray).getStr());
   EXPECT_EQ(FooPtr, formatv("{0}", FooPtr).getStr());
   EXPECT_EQ(FooRef, formatv("{0}", FooRef).getStr());
   EXPECT_EQ(FooLiteral, formatv("{0}", FooLiteral).getStr());
   EXPECT_EQ(FooString, formatv("{0}", FooString).getStr());

   // 2. Test that the precision specifier prints the correct number of
   // characters.
   EXPECT_EQ("FooA", formatv("{0:4}", FooArray).getStr());
   EXPECT_EQ("FooP", formatv("{0:4}", FooPtr).getStr());
   EXPECT_EQ("FooR", formatv("{0:4}", FooRef).getStr());
   EXPECT_EQ("FooS", formatv("{0:4}", FooString).getStr());

   // 3. And that padding works.
   EXPECT_EQ("  FooA", formatv("{0,6:4}", FooArray).getStr());
   EXPECT_EQ("  FooP", formatv("{0,6:4}", FooPtr).getStr());
   EXPECT_EQ("  FooR", formatv("{0,6:4}", FooRef).getStr());
   EXPECT_EQ("  FooS", formatv("{0,6:4}", FooString).getStr());
}

TEST(FormatVariadicTest, testCharFormatting)
{
   // 1. Not much to see here.  Just print a char with and without padding.
   EXPECT_EQ("C", formatv("{0}", 'C').getStr());
   EXPECT_EQ("  C", formatv("{0,3}", 'C').getStr());

   // 2. char is really an integral type though, where the only difference is
   // that the "default" is to print the ASCII.  So if a non-default presentation
   // specifier exists, it should print as an integer.
   EXPECT_EQ("37", formatv("{0:D}", (char)37).getStr());
   EXPECT_EQ("  037", formatv("{0,5:D3}", (char)37).getStr());
}

TEST(FormatVariadicTest, testBoolTest)
{
   // 1. Default style is lowercase text (same as 't')
   EXPECT_EQ("true", formatv("{0}", true).getStr());
   EXPECT_EQ("false", formatv("{0}", false).getStr());
   EXPECT_EQ("true", formatv("{0:t}", true).getStr());
   EXPECT_EQ("false", formatv("{0:t}", false).getStr());

   // 2. T - uppercase text
   EXPECT_EQ("TRUE", formatv("{0:T}", true).getStr());
   EXPECT_EQ("FALSE", formatv("{0:T}", false).getStr());

   // 3. D / d - integral
   EXPECT_EQ("1", formatv("{0:D}", true).getStr());
   EXPECT_EQ("0", formatv("{0:D}", false).getStr());
   EXPECT_EQ("1", formatv("{0:d}", true).getStr());
   EXPECT_EQ("0", formatv("{0:d}", false).getStr());

   // 4. Y - uppercase yes/no
   EXPECT_EQ("YES", formatv("{0:Y}", true).getStr());
   EXPECT_EQ("NO", formatv("{0:Y}", false).getStr());

   // 5. y - lowercase yes/no
   EXPECT_EQ("yes", formatv("{0:y}", true).getStr());
   EXPECT_EQ("no", formatv("{0:y}", false).getStr());
}

TEST(FormatVariadicTest, testDoubleFormatting) {
   // Test exponents, fixed point, and percent formatting.

   // 1. Signed, unsigned, and zero exponent format.
   EXPECT_EQ("0.000000E+00", formatv("{0:E}", 0.0).getStr());
   EXPECT_EQ("-0.000000E+00", formatv("{0:E}", -0.0).getStr());
   EXPECT_EQ("1.100000E+00", formatv("{0:E}", 1.1).getStr());
   EXPECT_EQ("-1.100000E+00", formatv("{0:E}", -1.1).getStr());
   EXPECT_EQ("1.234568E+03", formatv("{0:E}", 1234.5678).getStr());
   EXPECT_EQ("-1.234568E+03", formatv("{0:E}", -1234.5678).getStr());
   EXPECT_EQ("1.234568E-03", formatv("{0:E}", .0012345678).getStr());
   EXPECT_EQ("-1.234568E-03", formatv("{0:E}", -.0012345678).getStr());

   // 2. With padding and precision.
   EXPECT_EQ("  0.000E+00", formatv("{0,11:E3}", 0.0).getStr());
   EXPECT_EQ(" -1.100E+00", formatv("{0,11:E3}", -1.1).getStr());
   EXPECT_EQ("  1.235E+03", formatv("{0,11:E3}", 1234.5678).getStr());
   EXPECT_EQ(" -1.235E-03", formatv("{0,11:E3}", -.0012345678).getStr());

   // 3. Signed, unsigned, and zero fixed point format.
   EXPECT_EQ("0.00", formatv("{0:F}", 0.0).getStr());
   EXPECT_EQ("-0.00", formatv("{0:F}", -0.0).getStr());
   EXPECT_EQ("1.10", formatv("{0:F}", 1.1).getStr());
   EXPECT_EQ("-1.10", formatv("{0:F}", -1.1).getStr());
   EXPECT_EQ("1234.57", formatv("{0:F}", 1234.5678).getStr());
   EXPECT_EQ("-1234.57", formatv("{0:F}", -1234.5678).getStr());
   EXPECT_EQ("0.00", formatv("{0:F}", .0012345678).getStr());
   EXPECT_EQ("-0.00", formatv("{0:F}", -.0012345678).getStr());

   // 2. With padding and precision.
   EXPECT_EQ("   0.000", formatv("{0,8:F3}", 0.0).getStr());
   EXPECT_EQ("  -1.100", formatv("{0,8:F3}", -1.1).getStr());
   EXPECT_EQ("1234.568", formatv("{0,8:F3}", 1234.5678).getStr());
   EXPECT_EQ("  -0.001", formatv("{0,8:F3}", -.0012345678).getStr());
}

TEST(FormatVariadicTest, testCustomPaddingCharacter)
{
   // 1. Padding with custom character
   EXPECT_EQ("==123", formatv("{0,=+5}", 123).getStr());
   EXPECT_EQ("=123=", formatv("{0,==5}", 123).getStr());
   EXPECT_EQ("123==", formatv("{0,=-5}", 123).getStr());
   // 2. Combined with zero padding
   EXPECT_EQ("=00123=", formatv("{0,==7:5}", 123).getStr());
}

struct FormatTuple
{
   const char *Fmt;
   explicit FormatTuple(const char *Fmt) : Fmt(Fmt) {}

   template <typename... Ts>
   auto operator()(Ts &&... Values) const
   -> decltype(formatv(Fmt, std::forward<Ts>(Values)...)) {
      return formatv(Fmt, std::forward<Ts>(Values)...);
   }
};

TEST(FormatVariadicTest, testBigTest)
{
   using Tuple =
   std::tuple<char, int, const char *, StringRef, std::string, double, float,
   void *, int, double, int64_t, uint64_t, double, uint8_t>;
   Tuple Ts[] = {
      Tuple('a', 1, "Str", StringRef(), std::string(), 3.14159, -.17532f,
      (void *)nullptr, 123456, 6.02E23, -908234908423, 908234908422234,
      std::numeric_limits<double>::quiet_NaN(), 0xAB),
      Tuple('x', 0xDDB5B, "LongerStr", "StringRef", "std::string", -2.7,
      .08215f, (void *)nullptr, 0, 6.62E-34, -908234908423,
      908234908422234, std::numeric_limits<double>::infinity(), 0x0)};
   // Test long string formatting with many edge cases combined.
   const char *Intro =
         "There are {{{0}} items in the tuple, and {{{1}} tuple(s) in the array.";
   const char *Header =
         "{0,6}|{1,8}|{2,=10}|{3,=10}|{4,=13}|{5,7}|{6,7}|{7,10}|{8,"
         "-7}|{9,10}|{10,16}|{11,17}|{12,6}|{13,4}";
   const char *Line =
         "{0,6}|{1,8:X}|{2,=10}|{3,=10:5}|{4,=13}|{5,7:3}|{6,7:P2}|{7,"
         "10:X8}|{8,-7:N}|{9,10:E4}|{10,16:N}|{11,17:D}|{12,6}|{13,"
         "4:X}";

   std::string S;
   RawStringOutStream Stream(S);
   Stream << formatv(Intro, std::tuple_size<Tuple>::value,
                     array_lengthof(Ts))
          << "\n";
   Stream << formatv(Header, "Char", "HexInt", "Str", "Ref", "std::str",
                     "double", "float", "pointer", "comma", "exp", "bigint",
                     "bigint2", "limit", "byte")
          << "\n";
   for (auto &Item : Ts) {
      Stream << apply_tuple(FormatTuple(Line), Item) << "\n";
   }
   Stream.flush();
   const char *Expected =
       R"foo(There are {14} items in the tuple, and {2} tuple(s) in the array.
  Char|  HexInt|   Str    |   Ref    |  std::str   | double|  float|   pointer|comma  |       exp|          bigint|          bigint2| limit|byte
     a|     0x1|   Str    |          |             |  3.142|-17.53%|0x00000000|123,456|6.0200E+23|-908,234,908,423|  908234908422234|   nan|0xAB
     x| 0xDDB5B|LongerStr |  Strin   | std::string | -2.700|  8.21%|0x00000000|0      |6.6200E-34|-908,234,908,423|  908234908422234|   INF| 0x0
)foo";

   EXPECT_EQ(Expected, S);
}

TEST(FormatVariadicTest, testRange)
{
   std::vector<int> IntRange = {1, 1, 2, 3, 5, 8, 13};

   // 1. Simple range with default separator and element style.
   EXPECT_EQ("1, 1, 2, 3, 5, 8, 13",
             formatv("{0}", make_range(IntRange.begin(), IntRange.end())).getStr());
   EXPECT_EQ("1, 2, 3, 5, 8",
             formatv("{0}", make_range(IntRange.begin() + 1, IntRange.end() - 1))
             .getStr());

   // 2. Non-default separator
   EXPECT_EQ(
            "1/1/2/3/5/8/13",
            formatv("{0:$[/]}", make_range(IntRange.begin(), IntRange.end())).getStr());

   // 3. Default separator, non-default element style.
   EXPECT_EQ(
            "0x1, 0x1, 0x2, 0x3, 0x5, 0x8, 0xd",
            formatv("{0:@[x]}", make_range(IntRange.begin(), IntRange.end())).getStr());

   // 4. Non-default separator and element style.
   EXPECT_EQ(
            "0x1 + 0x1 + 0x2 + 0x3 + 0x5 + 0x8 + 0xd",
            formatv("{0:$[ + ]@[x]}", make_range(IntRange.begin(), IntRange.end()))
            .getStr());

   // 5. Element style and/or separator using alternate delimeters to allow using
   // delimeter characters as part of the separator.
   EXPECT_EQ(
            "<0x1><0x1><0x2><0x3><0x5><0x8><0xd>",
            formatv("<{0:$[><]@(x)}>", make_range(IntRange.begin(), IntRange.end()))
            .getStr());
   EXPECT_EQ(
            "[0x1][0x1][0x2][0x3][0x5][0x8][0xd]",
            formatv("[{0:$(][)@[x]}]", make_range(IntRange.begin(), IntRange.end()))
            .getStr());
   EXPECT_EQ(
            "(0x1)(0x1)(0x2)(0x3)(0x5)(0x8)(0xd)",
            formatv("({0:$<)(>@<x>})", make_range(IntRange.begin(), IntRange.end()))
            .getStr());

   // 5. Empty range.
   EXPECT_EQ("", formatv("{0:$[+]@[x]}",
                         make_range(IntRange.begin(), IntRange.begin()))
             .getStr());

   // 6. Empty separator and style.
   EXPECT_EQ("11235813",
             formatv("{0:$[]@<>}", make_range(IntRange.begin(), IntRange.end()))
             .getStr());
}

TEST(FormatVariadicTest, testAdapter)
{
   class Negative : public FormatAdapter<int> {
   public:
      explicit Negative(int N) : FormatAdapter<int>(std::move(N)) {}
      void format(RawOutStream &S, StringRef Options) override { S << -m_item; }
   };

   EXPECT_EQ("-7", formatv("{0}", Negative(7)).getStr());

   int N = 171;

   EXPECT_EQ("  171  ",
             formatv("{0}", fmt_align(N, AlignStyle::Center, 7)).getStr());
   EXPECT_EQ("--171--",
             formatv("{0}", fmt_align(N, AlignStyle::Center, 7, '-')).getStr());
   EXPECT_EQ(" 171   ", formatv("{0}", fmt_pad(N, 1, 3)).getStr());
   EXPECT_EQ("171171171171171", formatv("{0}", fmt_repeat(N, 5)).getStr());

   EXPECT_EQ(" ABABABABAB   ",
             formatv("{0:X-}", fmt_pad(fmt_repeat(N, 5), 1, 3)).getStr());
   EXPECT_EQ("   AB    AB    AB    AB    AB     ",
             formatv("{0,=34:X-}", fmt_repeat(fmt_pad(N, 1, 3), 5)).getStr());
}

TEST(FormatVariadicTest, testMoveConstructor)
{
   auto fmt = formatv("{0} {1}", 1, 2);
   auto fmt2 = std::move(fmt);
   std::string S = fmt2;
   EXPECT_EQ("1 2", S);
}

TEST(FormatVariadicTest, testImplicitConversions)
{
   std::string S = formatv("{0} {1}", 1, 2);
   EXPECT_EQ("1 2", S);

   SmallString<4> S2 = formatv("{0} {1}", 1, 2);
   EXPECT_EQ("1 2", S2);
}

TEST(FormatVariadicTest, testFormatAdapter)
{
   EXPECT_EQ("Format", formatv("{0}", Format(1)).getStr());

   Format var(1);
   EXPECT_EQ("Format", formatv("{0}", var).getStr());
   EXPECT_EQ("Format", formatv("{0}", std::move(var)).getStr());

   // Not supposed to compile
   // const Format cvar(1);
   // EXPECT_EQ("Format", formatv("{0}", cvar).getStr());
}

TEST(FormatVariadicTest, testFormatFormatvObject)
{
   EXPECT_EQ("Format", formatv("F{0}t", formatv("o{0}a", "rm")).getStr());
   EXPECT_EQ("[   ! ]", formatv("[{0,+5}]", formatv("{0,-2}", "!")).getStr());
}

struct Recorder {
   int Copied = 0, Moved = 0;
   Recorder() = default;
   Recorder(const Recorder &Copy) : Copied(1 + Copy.Copied), Moved(Copy.Moved) {}
   Recorder(const Recorder &&Move)
      : Copied(Move.Copied), Moved(1 + Move.Moved) {}
};

} // anonymous namespace

namespace polar {
namespace utils {
template <>
struct FormatProvider<Recorder> {
   static void format(const Recorder &R, RawOutStream &outstream, StringRef style)
   {
      outstream << R.Copied << "C " << R.Moved << "M";
   }
};
}
} // namespace

namespace {

TEST(FormatVariadicTest, testCopiesAndMoves)
{
   Recorder R;
   EXPECT_EQ("0C 0M", formatv("{0}", R).getStr());
   EXPECT_EQ("0C 3M", formatv("{0}", std::move(R)).getStr());
   EXPECT_EQ("0C 3M", formatv("{0}", Recorder()).getStr());
   EXPECT_EQ(0, R.Copied);
   EXPECT_EQ(0, R.Moved);
}

} // anonymous namespace

namespace adl {
struct X {};
RawOutStream &operator<<(RawOutStream &out, const X &) { return out << "X"; }
} // namespace adl

TEST(FormatVariadicTest, FormatStreamable)
{
   adl::X X;
   EXPECT_EQ("X", formatv("{0}", X).getStr());
}

TEST(FormatVariadicTest, FormatError)
{
   auto E1 = make_error<StringError>("X", inconvertible_error_code());
   EXPECT_EQ("X", formatv("{0}", E1).getStr());
   EXPECT_TRUE(E1.isA<StringError>()); // not consumed
   EXPECT_EQ("X", formatv("{0}", fmt_consume(std::move(E1))).getStr());
   EXPECT_FALSE(E1.isA<StringError>()); // consumed
}
