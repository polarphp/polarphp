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

#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

namespace {

template<typename T>
std::string print_to_string(const T &value)
{
   std::string res;
   RawStringOutStream(res) << value;
   return res;
}

/// print_to_string - Print the given value to a stream which only has \arg
/// bytesLeftInBuffer bytes left in the buffer. This is useful for testing edge
/// cases in the buffer handling logic.
template<typename T>
std::string print_to_string(const T &value,
                            unsigned bytesLeftInBuffer)
{
   // FIXME: This is relying on internal knowledge of how raw_ostream works to
   // get the buffer position right.
   SmallString<256> SVec;
   assert(bytesLeftInBuffer < 256 && "Invalid buffer count!");
   RawSvectorOutStream outstream(SVec);
   unsigned StartIndex = 256 - bytesLeftInBuffer;
   for (unsigned i = 0; i != StartIndex; ++i) {
      outstream << '?';
   }
   outstream << value;
   return outstream.getStr().substr(StartIndex);
}

template<typename T> std::string printToStringUnbuffered(const T &value)
{
   std::string res;
   RawStringOutStream outstream(res);
   outstream.setUnbuffered();
   outstream << value;
   return res;
}

TEST(RawOutStreamTest, testTypesBuffered)
{
   // Char
   EXPECT_EQ("c", print_to_string('c'));

   // String
   EXPECT_EQ("hello", print_to_string("hello"));
   EXPECT_EQ("hello", print_to_string(std::string("hello")));

   // Int
   EXPECT_EQ("0", print_to_string(0));
   EXPECT_EQ("2425", print_to_string(2425));
   EXPECT_EQ("-2425", print_to_string(-2425));

   // Long long
   EXPECT_EQ("0", print_to_string(0LL));
   EXPECT_EQ("257257257235709", print_to_string(257257257235709LL));
   EXPECT_EQ("-257257257235709", print_to_string(-257257257235709LL));

   // Double
   EXPECT_EQ("1.100000e+00", print_to_string(1.1));

   // void*
   EXPECT_EQ("0x0", print_to_string((void*) nullptr));
   EXPECT_EQ("0xbeef", print_to_string((void*) 0xbeefLL));
   EXPECT_EQ("0xdeadbeef", print_to_string((void*) 0xdeadbeefLL));

   // Min and max.
   EXPECT_EQ("18446744073709551615", print_to_string(UINT64_MAX));
   EXPECT_EQ("-9223372036854775808", print_to_string(INT64_MIN));
}

TEST(RawOutStreamTest, testTypesUnbuffered)
{
   // Char
   EXPECT_EQ("c", printToStringUnbuffered('c'));

   // String
   EXPECT_EQ("hello", printToStringUnbuffered("hello"));
   EXPECT_EQ("hello", printToStringUnbuffered(std::string("hello")));

   // Int
   EXPECT_EQ("0", printToStringUnbuffered(0));
   EXPECT_EQ("2425", printToStringUnbuffered(2425));
   EXPECT_EQ("-2425", printToStringUnbuffered(-2425));

   // Long long
   EXPECT_EQ("0", printToStringUnbuffered(0LL));
   EXPECT_EQ("257257257235709", printToStringUnbuffered(257257257235709LL));
   EXPECT_EQ("-257257257235709", printToStringUnbuffered(-257257257235709LL));

   // Double
   EXPECT_EQ("1.100000e+00", printToStringUnbuffered(1.1));

   // void*
   EXPECT_EQ("0x0", printToStringUnbuffered((void*) nullptr));
   EXPECT_EQ("0xbeef", printToStringUnbuffered((void*) 0xbeefLL));
   EXPECT_EQ("0xdeadbeef", printToStringUnbuffered((void*) 0xdeadbeefLL));

   // Min and max.
   EXPECT_EQ("18446744073709551615", printToStringUnbuffered(UINT64_MAX));
   EXPECT_EQ("-9223372036854775808", printToStringUnbuffered(INT64_MIN));
}

TEST(RawOutStreamTest, testBufferEdge)
{
   EXPECT_EQ("1.20", print_to_string(format("%.2f", 1.2), 1));
   EXPECT_EQ("1.20", print_to_string(format("%.2f", 1.2), 2));
   EXPECT_EQ("1.20", print_to_string(format("%.2f", 1.2), 3));
   EXPECT_EQ("1.20", print_to_string(format("%.2f", 1.2), 4));
   EXPECT_EQ("1.20", print_to_string(format("%.2f", 1.2), 10));
}

TEST(RawOutStreamTest, testTinyBuffer)
{
   std::string str;
   RawStringOutStream outstream(str);
   outstream.setBufferSize(1);
   outstream << "hello";
   outstream << 1;
   outstream << 'w' << 'o' << 'r' << 'l' << 'd';
   EXPECT_EQ("hello1world", outstream.getStr());
}

TEST(RawOutStreamTest, testWriteEscaped)
{
   std::string str;

   str = "";
   RawStringOutStream(str).writeEscaped("hi");
   EXPECT_EQ("hi", str);

   str = "";
   RawStringOutStream(str).writeEscaped("\\\t\n\"");
   EXPECT_EQ("\\\\\\t\\n\\\"", str);

   str = "";
   RawStringOutStream(str).writeEscaped("\1\10\200");
   EXPECT_EQ("\\001\\010\\200", str);
}

TEST(RawOutStreamTest, testJustify)
{
   EXPECT_EQ("xyz   ", print_to_string(left_justify("xyz", 6), 6));
   EXPECT_EQ("abc",    print_to_string(left_justify("abc", 3), 3));
   EXPECT_EQ("big",    print_to_string(left_justify("big", 1), 3));
   EXPECT_EQ("   xyz", print_to_string(right_justify("xyz", 6), 6));
   EXPECT_EQ("abc",    print_to_string(right_justify("abc", 3), 3));
   EXPECT_EQ("big",    print_to_string(right_justify("big", 1), 3));
   EXPECT_EQ("   on    ",    print_to_string(center_justify("on", 9), 9));
   EXPECT_EQ("   off    ",    print_to_string(center_justify("off", 10), 10));
   EXPECT_EQ("single ",    print_to_string(center_justify("single", 7), 7));
   EXPECT_EQ("std::nullopt",    print_to_string(center_justify("std::nullopt", 1), 4));
   EXPECT_EQ("std::nullopt",    print_to_string(center_justify("std::nullopt", 1), 1));
}

TEST(RawOutStreamTest, testFormatHex)
{
   EXPECT_EQ("0x1234",     print_to_string(format_hex(0x1234, 6), 6));
   EXPECT_EQ("0x001234",   print_to_string(format_hex(0x1234, 8), 8));
   EXPECT_EQ("0x00001234", print_to_string(format_hex(0x1234, 10), 10));
   EXPECT_EQ("0x1234",     print_to_string(format_hex(0x1234, 4), 6));
   EXPECT_EQ("0xff",       print_to_string(format_hex(255, 4), 4));
   EXPECT_EQ("0xFF",       print_to_string(format_hex(255, 4, true), 4));
   EXPECT_EQ("0x1",        print_to_string(format_hex(1, 3), 3));
   EXPECT_EQ("0x12",       print_to_string(format_hex(0x12, 3), 4));
   EXPECT_EQ("0x123",      print_to_string(format_hex(0x123, 3), 5));
   EXPECT_EQ("FF",         print_to_string(format_hex_no_prefix(0xFF, 2, true), 4));
   EXPECT_EQ("ABCD",       print_to_string(format_hex_no_prefix(0xABCD, 2, true), 4));
   EXPECT_EQ("0xffffffffffffffff",
             print_to_string(format_hex(UINT64_MAX, 18), 18));
   EXPECT_EQ("0x8000000000000000",
             print_to_string(format_hex((INT64_MIN), 18), 18));
}

TEST(RawOutStreamTest, testFormatDecimal)
{
   EXPECT_EQ("   0",        print_to_string(format_decimal(0, 4), 4));
   EXPECT_EQ("  -1",        print_to_string(format_decimal(-1, 4), 4));
   EXPECT_EQ("    -1",      print_to_string(format_decimal(-1, 6), 6));
   EXPECT_EQ("1234567890",  print_to_string(format_decimal(1234567890, 10), 10));
   EXPECT_EQ("  9223372036854775807",
             print_to_string(format_decimal(INT64_MAX, 21), 21));
   EXPECT_EQ(" -9223372036854775808",
             print_to_string(format_decimal(INT64_MIN, 21), 21));
}

static std::string formatted_bytes_str(ArrayRef<uint8_t> bytes,
                                       std::optional<uint64_t> offset = std::nullopt,
                                       uint32_t numPerLine = 16,
                                       uint8_t byteGroupSize = 4) {
   std::string S;
   RawStringOutStream str(S);
   str << format_bytes(bytes, offset, numPerLine, byteGroupSize);
   str.flush();
   return S;
}

static std::string format_bytes_with_ascii_str(ArrayRef<uint8_t> bytes,
                                               std::optional<uint64_t> offset = std::nullopt,
                                               uint32_t numPerLine = 16,
                                               uint8_t byteGroupSize = 4) {
   std::string S;
   RawStringOutStream str(S);
   str << format_bytes_with_ascii(bytes, offset, numPerLine, byteGroupSize);
   str.flush();
   return S;
}

TEST(RawOutStreamTest, testFormattedHexBytes)
{
   std::vector<uint8_t> Buf = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
                               'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
                               's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0',
                               '1', '2', '3', '4', '5', '6', '7', '8', '9'};
   ArrayRef<uint8_t> B(Buf);
   // Test invalid input.
   EXPECT_EQ("", formatted_bytes_str(ArrayRef<uint8_t>()));
   EXPECT_EQ("", format_bytes_with_ascii_str(ArrayRef<uint8_t>()));
   //----------------------------------------------------------------------
   // Test hex byte output with the default 4 byte groups
   //----------------------------------------------------------------------
   EXPECT_EQ("61", formatted_bytes_str(B.takeFront()));
   EXPECT_EQ("61626364 65", formatted_bytes_str(B.takeFront(5)));
   // Test that 16 bytes get written to a line correctly.
   EXPECT_EQ("61626364 65666768 696a6b6c 6d6e6f70",
             formatted_bytes_str(B.takeFront(16)));
   // Test raw bytes with default 16 bytes per line wrapping.
   EXPECT_EQ("61626364 65666768 696a6b6c 6d6e6f70\n71",
             formatted_bytes_str(B.takeFront(17)));
   // Test raw bytes with 1 bytes per line wrapping.
   EXPECT_EQ("61\n62\n63\n64\n65\n66",
             formatted_bytes_str(B.takeFront(6), std::nullopt, 1));
   // Test raw bytes with 7 bytes per line wrapping.
   EXPECT_EQ("61626364 656667\n68696a6b 6c6d6e\n6f7071",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 7));
   // Test raw bytes with 8 bytes per line wrapping.
   EXPECT_EQ("61626364 65666768\n696a6b6c 6d6e6f70\n71",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 8));
   //----------------------------------------------------------------------
   // Test hex byte output with the 1 byte groups
   //----------------------------------------------------------------------
   EXPECT_EQ("61 62 63 64 65",
             formatted_bytes_str(B.takeFront(5), std::nullopt, 16, 1));
   // Test that 16 bytes get written to a line correctly.
   EXPECT_EQ("61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70",
             formatted_bytes_str(B.takeFront(16), std::nullopt, 16, 1));
   // Test raw bytes with default 16 bytes per line wrapping.
   EXPECT_EQ("61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f 70\n71",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 16, 1));
   // Test raw bytes with 7 bytes per line wrapping.
   EXPECT_EQ("61 62 63 64 65 66 67\n68 69 6a 6b 6c 6d 6e\n6f 70 71",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 7, 1));
   // Test raw bytes with 8 bytes per line wrapping.
   EXPECT_EQ("61 62 63 64 65 66 67 68\n69 6a 6b 6c 6d 6e 6f 70\n71",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 8, 1));

   //----------------------------------------------------------------------
   // Test hex byte output with the 2 byte groups
   //----------------------------------------------------------------------
   EXPECT_EQ("6162 6364 65", formatted_bytes_str(B.takeFront(5), std::nullopt, 16, 2));
   // Test that 16 bytes get written to a line correctly.
   EXPECT_EQ("6162 6364 6566 6768 696a 6b6c 6d6e 6f70",
             formatted_bytes_str(B.takeFront(16), std::nullopt, 16, 2));
   // Test raw bytes with default 16 bytes per line wrapping.
   EXPECT_EQ("6162 6364 6566 6768 696a 6b6c 6d6e 6f70\n71",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 16, 2));
   // Test raw bytes with 7 bytes per line wrapping.
   EXPECT_EQ("6162 6364 6566 67\n6869 6a6b 6c6d 6e\n6f70 71",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 7, 2));
   // Test raw bytes with 8 bytes per line wrapping.
   EXPECT_EQ("6162 6364 6566 6768\n696a 6b6c 6d6e 6f70\n71",
             formatted_bytes_str(B.takeFront(17), std::nullopt, 8, 2));

   //----------------------------------------------------------------------
   // Test hex bytes with offset with the default 4 byte groups.
   //----------------------------------------------------------------------
   EXPECT_EQ("0000: 61", formatted_bytes_str(B.takeFront(), 0x0));
   EXPECT_EQ("1000: 61", formatted_bytes_str(B.takeFront(), 0x1000));
   EXPECT_EQ("1000: 61\n1001: 62",
             formatted_bytes_str(B.takeFront(2), 0x1000, 1));
   //----------------------------------------------------------------------
   // Test hex bytes with ASCII with the default 4 byte groups.
   //----------------------------------------------------------------------
   EXPECT_EQ("61626364 65666768 696a6b6c 6d6e6f70  |abcdefghijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16)));
   EXPECT_EQ("61626364 65666768  |abcdefgh|\n"
             "696a6b6c 6d6e6f70  |ijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16), std::nullopt, 8));
   EXPECT_EQ("61626364 65666768  |abcdefgh|\n696a6b6c           |ijkl|",
             format_bytes_with_ascii_str(B.takeFront(12), std::nullopt, 8));
   std::vector<uint8_t> Unprintable = {'a', '\x1e', 'b', '\x1f'};
   // Make sure the ASCII is still lined up correctly when fewer bytes than 16
   // bytes per line are available. The ASCII should still be aligned as if 16
   // bytes of hex might be displayed.
   EXPECT_EQ("611e621f                             |a.b.|",
             format_bytes_with_ascii_str(Unprintable));
   //----------------------------------------------------------------------
   // Test hex bytes with ASCII with offsets with the default 4 byte groups.
   //----------------------------------------------------------------------
   EXPECT_EQ("0000: 61626364 65666768 "
             "696a6b6c 6d6e6f70  |abcdefghijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16), 0));
   EXPECT_EQ("0000: 61626364 65666768  |abcdefgh|\n"
             "0008: 696a6b6c 6d6e6f70  |ijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16), 0, 8));
   EXPECT_EQ("0000: 61626364 656667  |abcdefg|\n"
             "0007: 68696a6b 6c      |hijkl|",
             format_bytes_with_ascii_str(B.takeFront(12), 0, 7));

   //----------------------------------------------------------------------
   // Test hex bytes with ASCII with offsets with the default 2 byte groups.
   //----------------------------------------------------------------------
   EXPECT_EQ("0000: 6162 6364 6566 6768 "
             "696a 6b6c 6d6e 6f70  |abcdefghijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16), 0, 16, 2));
   EXPECT_EQ("0000: 6162 6364 6566 6768  |abcdefgh|\n"
             "0008: 696a 6b6c 6d6e 6f70  |ijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16), 0, 8, 2));
   EXPECT_EQ("0000: 6162 6364 6566 67  |abcdefg|\n"
             "0007: 6869 6a6b 6c       |hijkl|",
             format_bytes_with_ascii_str(B.takeFront(12), 0, 7, 2));

   //----------------------------------------------------------------------
   // Test hex bytes with ASCII with offsets with the default 1 byte groups.
   //----------------------------------------------------------------------
   EXPECT_EQ("0000: 61 62 63 64 65 66 67 68 "
             "69 6a 6b 6c 6d 6e 6f 70  |abcdefghijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16), 0, 16, 1));
   EXPECT_EQ("0000: 61 62 63 64 65 66 67 68  |abcdefgh|\n"
             "0008: 69 6a 6b 6c 6d 6e 6f 70  |ijklmnop|",
             format_bytes_with_ascii_str(B.takeFront(16), 0, 8, 1));
   EXPECT_EQ("0000: 61 62 63 64 65 66 67  |abcdefg|\n"
             "0007: 68 69 6a 6b 6c        |hijkl|",
             format_bytes_with_ascii_str(B.takeFront(12), 0, 7, 1));
}

TEST(RawFdOutStreamTest, testMultipleRawFdOutStreamToStdout)
{
   std::error_code errorCode;
   { RawFdOutStream("-", errorCode, polar::fs::OpenFlags::F_None); }
   { RawFdOutStream("-", errorCode, polar::fs::OpenFlags::F_None); }
}

} // anonymous namespace
