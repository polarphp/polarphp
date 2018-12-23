// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/SourceMgr.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::utils;
using namespace polar::basic;

namespace {

class SourceMgrTest : public testing::Test
{
public:
   SourceMgr SM;
   unsigned mainBufferID;
   std::string output;

   void setMainBuffer(StringRef text, StringRef bufferName)
   {
      std::unique_ptr<MemoryBuffer> MainBuffer =
            MemoryBuffer::getMemBuffer(text, bufferName);
      mainBufferID = SM.addNewSourceBuffer(std::move(MainBuffer), polar::utils::SMLocation());
   }

   SMLocation getLoc(unsigned offset)
   {
      return SMLocation::getFromPointer(
               SM.getMemoryBuffer(mainBufferID)->getBufferStart() + offset);
   }

   SMRange getRange(unsigned offset, unsigned Length)
   {
      return SMRange(getLoc(offset), getLoc(offset + Length));
   }

   void printMessage(SMLocation Loc, SourceMgr::DiagKind Kind,
                     const Twine &Msg, ArrayRef<SMRange> Ranges,
                     ArrayRef<SMFixIt> FixIts)
   {
      RawStringOutStream OS(output);
      SM.printMessage(OS, Loc, Kind, Msg, Ranges, FixIts);
   }
};

TEST_F(SourceMgrTest, testBasicError)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:5: error: message\n"
             "aaa bbb\n"
             "    ^\n",
             output);
}

TEST_F(SourceMgrTest, testBasicWarning)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Warning, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:5: warning: message\n"
             "aaa bbb\n"
             "    ^\n",
             output);
}

TEST_F(SourceMgrTest, testBasicRemark)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Remark, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:5: remark: message\n"
             "aaa bbb\n"
             "    ^\n",
             output);
}

TEST_F(SourceMgrTest, testBasicNote)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Note, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:5: note: message\n"
             "aaa bbb\n"
             "    ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtEndOfLine)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(6), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:7: error: message\n"
             "aaa bbb\n"
             "      ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtNewline)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(7), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:8: error: message\n"
             "aaa bbb\n"
             "       ^\n",
             output);
}


TEST_F(SourceMgrTest, testLocationAtEmptyBuffer)
{
   setMainBuffer("", "file.in");
   printMessage(getLoc(0), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:1: error: message\n"
             "\n"
             "^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationJustOnSoleNewline)
{
   setMainBuffer("\n", "file.in");
   printMessage(getLoc(0), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:1: error: message\n"
             "\n"
             "^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationJustAfterSoleNewline)
{
   setMainBuffer("\n", "file.in");
   printMessage(getLoc(1), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:2:1: error: message\n"
             "\n"
             "^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationJustAfterNonNewline)
{
   setMainBuffer("123", "file.in");
   printMessage(getLoc(3), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:4: error: message\n"
             "123\n"
             "   ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationOnFirstLineOfMultiline)
{
   setMainBuffer("1234\n6789\n", "file.in");
   printMessage(getLoc(3), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:4: error: message\n"
             "1234\n"
             "   ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationOnEOLOfFirstLineOfMultiline)
{
   setMainBuffer("1234\n6789\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:1:5: error: message\n"
             "1234\n"
             "    ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationOnSecondLineOfMultiline)
{
   setMainBuffer("1234\n6789\n", "file.in");
   printMessage(getLoc(5), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:2:1: error: message\n"
             "6789\n"
             "^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationOnSecondLineOfMultilineNoSecondEOL)
{
   setMainBuffer("1234\n6789", "file.in");
   printMessage(getLoc(5), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:2:1: error: message\n"
             "6789\n"
             "^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationOnEOLOfSecondSecondLineOfMultiline)
{
   setMainBuffer("1234\n6789\n", "file.in");
   printMessage(getLoc(9), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);

   EXPECT_EQ("file.in:2:5: error: message\n"
             "6789\n"
             "    ^\n",
             output);
}

#define STRING_LITERAL_253_BYTES \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n1234567890\n" \
   "1234567890\n"


//===----------------------------------------------------------------------===//
// 255-byte buffer tests
//===----------------------------------------------------------------------===//

TEST_F(SourceMgrTest, testLocationBeforeEndOf255ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "12"                       // + 2 = 255 bytes
                 , "file.in");
   printMessage(getLoc(253), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:1: error: message\n"
             "12\n"
             "^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtEndOf255ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "12"                       // + 2 = 255 bytes
                 , "file.in");
   printMessage(getLoc(254), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:2: error: message\n"
             "12\n"
             " ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationPastEndOf255ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "12"                       // + 2 = 255 bytes
                 , "file.in");
   printMessage(getLoc(255), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:3: error: message\n"
             "12\n"
             "  ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationBeforeEndOf255ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "1\n"                      // + 2 = 255 bytes
                 , "file.in");
   printMessage(getLoc(253), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:1: error: message\n"
             "1\n"
             "^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtEndOf255ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "1\n"                      // + 2 = 255 bytes
                 , "file.in");
   printMessage(getLoc(254), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:2: error: message\n"
             "1\n"
             " ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationPastEndOf255ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "1\n"                      // + 2 = 255 bytes
                 , "file.in");
   printMessage(getLoc(255), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:25:1: error: message\n"
             "\n"
             "^\n",
             output);
}

//===----------------------------------------------------------------------===//
// 256-byte buffer tests
//===----------------------------------------------------------------------===//

TEST_F(SourceMgrTest, testLocationBeforeEndOf256ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "123"                      // + 3 = 256 bytes
                 , "file.in");
   printMessage(getLoc(254), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:2: error: message\n"
             "123\n"
             " ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtEndOf256ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "123"                      // + 3 = 256 bytes
                 , "file.in");
   printMessage(getLoc(255), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:3: error: message\n"
             "123\n"
             "  ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationPastEndOf256ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "123"                      // + 3 = 256 bytes
                 , "file.in");
   printMessage(getLoc(256), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:4: error: message\n"
             "123\n"
             "   ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationBeforeEndOf256ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "12\n"                     // + 3 = 256 bytes
                 , "file.in");
   printMessage(getLoc(254), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:2: error: message\n"
             "12\n"
             " ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtEndOf256ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "12\n"                     // + 3 = 256 bytes
                 , "file.in");
   printMessage(getLoc(255), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:3: error: message\n"
             "12\n"
             "  ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationPastEndOf256ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "12\n"                     // + 3 = 256 bytes
                 , "file.in");
   printMessage(getLoc(256), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:25:1: error: message\n"
             "\n"
             "^\n",
             output);
}

//===----------------------------------------------------------------------===//
// 257-byte buffer tests
//===----------------------------------------------------------------------===//

TEST_F(SourceMgrTest, testLocationBeforeEndOf257ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "1234"                     // + 4 = 257 bytes
                 , "file.in");
   printMessage(getLoc(255), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:3: error: message\n"
             "1234\n"
             "  ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtEndOf257ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "1234"                     // + 4 = 257 bytes
                 , "file.in");
   printMessage(getLoc(256), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:4: error: message\n"
             "1234\n"
             "   ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationPastEndOf257ByteBuffer)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "1234"                     // + 4 = 257 bytes
                 , "file.in");
   printMessage(getLoc(257), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:5: error: message\n"
             "1234\n"
             "    ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationBeforeEndOf257ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "123\n"                    // + 4 = 257 bytes
                 , "file.in");
   printMessage(getLoc(255), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:3: error: message\n"
             "123\n"
             "  ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationAtEndOf257ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "123\n"                    // + 4 = 257 bytes
                 , "file.in");
   printMessage(getLoc(256), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:24:4: error: message\n"
             "123\n"
             "   ^\n",
             output);
}

TEST_F(SourceMgrTest, testLocationPastEndOf257ByteBufferEndingInNewline)
{
   setMainBuffer(STRING_LITERAL_253_BYTES   // first 253 bytes
                 "123\n"                    // + 4 = 257 bytes
                 , "file.in");
   printMessage(getLoc(257), SourceMgr::DK_Error, "message", std::nullopt, std::nullopt);
   EXPECT_EQ("file.in:25:1: error: message\n"
             "\n"
             "^\n",
             output);
}


TEST_F(SourceMgrTest, testBasicRange)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", getRange(4, 3), std::nullopt);

   EXPECT_EQ("file.in:1:5: error: message\n"
             "aaa bbb\n"
             "    ^~~\n",
             output);
}

TEST_F(SourceMgrTest, testRangeWithTab)
{
   setMainBuffer("aaa\tbbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", getRange(3, 3), std::nullopt);

   EXPECT_EQ("file.in:1:5: error: message\n"
             "aaa     bbb\n"
             "   ~~~~~^~\n",
             output);
}

TEST_F(SourceMgrTest, testMultiLineRange)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", getRange(4, 7), std::nullopt);

   EXPECT_EQ("file.in:1:5: error: message\n"
             "aaa bbb\n"
             "    ^~~\n",
             output);
}

TEST_F(SourceMgrTest, testMultipleRanges)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   SMRange Ranges[] = { getRange(0, 3), getRange(4, 3) };
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", Ranges, std::nullopt);

   EXPECT_EQ("file.in:1:5: error: message\n"
             "aaa bbb\n"
             "~~~ ^~~\n",
             output);
}

TEST_F(SourceMgrTest, testOverlappingRanges)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   SMRange Ranges[] = { getRange(0, 3), getRange(2, 4) };
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", Ranges, std::nullopt);

   EXPECT_EQ("file.in:1:5: error: message\n"
             "aaa bbb\n"
             "~~~~^~\n",
             output);
}

TEST_F(SourceMgrTest, testBasicFixit)
{
   setMainBuffer("aaa bbb\nccc ddd\n", "file.in");
   printMessage(getLoc(4), SourceMgr::DK_Error, "message", std::nullopt,
                make_array_ref(SMFixIt(getRange(4, 3), "zzz")));

   EXPECT_EQ("file.in:1:5: error: message\n"
             "aaa bbb\n"
             "    ^~~\n"
             "    zzz\n",
             output);
}

TEST_F(SourceMgrTest, testFixitForTab)
{
   setMainBuffer("aaa\tbbb\nccc ddd\n", "file.in");
   printMessage(getLoc(3), SourceMgr::DK_Error, "message", std::nullopt,
                make_array_ref(SMFixIt(getRange(3, 1), "zzz")));

   EXPECT_EQ("file.in:1:4: error: message\n"
             "aaa     bbb\n"
             "   ^^^^^\n"
             "   zzz\n",
             output);
}

} // anonymous namespace
