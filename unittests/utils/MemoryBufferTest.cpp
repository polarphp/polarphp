// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/13.

#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/FileUtils.h"
#include "polarphp/utils/RawOutStream.h"
#include "../support/Error.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;
using namespace polar::fs;

namespace {

class MemoryBufferTest : public testing::Test
{
protected:
   MemoryBufferTest()
      : data("this is some data")
   { }

   void SetUp() override {}

   /// Common testing for different modes of getOpenFileSlice.
   /// Creates a temporary file with known contents, and uses
   /// MemoryBuffer::getOpenFileSlice to map it.
   /// If \p Reopen is true, the file is closed after creating and reopened
   /// anew before using MemoryBuffer.
   void testGetOpenFileSlice(bool Reopen);

   typedef std::unique_ptr<MemoryBuffer> OwningBuffer;

   std::string data;
};

TEST_F(MemoryBufferTest, testGet)
{
   // Default name and null-terminator flag
   OwningBuffer MB1(MemoryBuffer::getMemBuffer(data));
   EXPECT_TRUE(nullptr != MB1.get());

   // RequiresNullTerminator = false
   OwningBuffer MB2(MemoryBuffer::getMemBuffer(data, "one", false));
   EXPECT_TRUE(nullptr != MB2.get());

   // RequiresNullTerminator = true
   OwningBuffer MB3(MemoryBuffer::getMemBuffer(data, "two", true));
   EXPECT_TRUE(nullptr != MB3.get());

   // verify all 3 buffers point to the same address
   EXPECT_EQ(MB1->getBufferStart(), MB2->getBufferStart());
   EXPECT_EQ(MB2->getBufferStart(), MB3->getBufferStart());

   // verify the original data is unmodified after deleting the buffers
   MB1.reset();
   MB2.reset();
   MB3.reset();
   EXPECT_EQ("this is some data", data);
}

TEST_F(MemoryBufferTest, testNullTerminator4K) {
   // Test that a file with size that is a multiple of the page size can be null
   // terminated correctly by MemoryBuffer.
   int TestFD;
   SmallString<64> TestPath;
   fs::create_temporary_file("MemoryBufferTest_NullTerminator4K", "temp",
                             TestFD, TestPath);
   FileRemover Cleanup(TestPath);
   RawFdOutStream OF(TestFD, true, /*unbuffered=*/true);
   for (unsigned i = 0; i < 4096 / 16; ++i) {
      OF << "0123456789abcdef";
   }
   OF.close();

   OptionalError<OwningBuffer> memoryBuffer = MemoryBuffer::getFile(TestPath.getCStr());
   std::error_code errorCode = memoryBuffer.getError();
   ASSERT_FALSE(errorCode);

   const char *BufData = memoryBuffer.get()->getBufferStart();
   EXPECT_EQ('f', BufData[4095]);
   EXPECT_EQ('\0', BufData[4096]);
}

TEST_F(MemoryBufferTest, testCopy)
{
   // copy with no name
   OwningBuffer MBC1(MemoryBuffer::getMemBufferCopy(data));
   EXPECT_TRUE(nullptr != MBC1.get());

   // copy with a name
   OwningBuffer MBC2(MemoryBuffer::getMemBufferCopy(data, "copy"));
   EXPECT_TRUE(nullptr != MBC2.get());

   // verify the two copies do not point to the same place
   EXPECT_NE(MBC1->getBufferStart(), MBC2->getBufferStart());
}

TEST_F(MemoryBufferTest, testMakeNew)
{
   // 0-sized buffer
   OwningBuffer Zero(WritableMemoryBuffer::getNewUninitMemBuffer(0));
   EXPECT_TRUE(nullptr != Zero.get());

   // uninitialized buffer with no name
   OwningBuffer One(WritableMemoryBuffer::getNewUninitMemBuffer(321));
   EXPECT_TRUE(nullptr != One.get());

   // uninitialized buffer with name
   OwningBuffer Two(WritableMemoryBuffer::getNewUninitMemBuffer(123, "bla"));
   EXPECT_TRUE(nullptr != Two.get());

   // 0-initialized buffer with no name
   OwningBuffer Three(WritableMemoryBuffer::getNewMemBuffer(321, data));
   EXPECT_TRUE(nullptr != Three.get());
   for (size_t i = 0; i < 321; ++i)
      EXPECT_EQ(0, Three->getBufferStart()[0]);

   // 0-initialized buffer with name
   OwningBuffer Four(WritableMemoryBuffer::getNewMemBuffer(123, "zeros"));
   EXPECT_TRUE(nullptr != Four.get());
   for (size_t i = 0; i < 123; ++i) {
      EXPECT_EQ(0, Four->getBufferStart()[0]);
   }
}

void MemoryBufferTest::testGetOpenFileSlice(bool Reopen)
{
   // Test that MemoryBuffer::getOpenFile works properly when no null
   // terminator is requested and the size is large enough to trigger
   // the usage of memory mapping.
   int TestFD;
   SmallString<64> TestPath;
   // Create a temporary file and write data into it.
   fs::create_temporary_file("prefix", "temp", TestFD, TestPath);
   FileRemover Cleanup(TestPath);
   // OF is responsible for closing the file; If the file is not
   // reopened, it will be unbuffered so that the results are
   // immediately visible through the fd.
   RawFdOutStream OF(TestFD, true, !Reopen);
   for (int i = 0; i < 60000; ++i) {
      OF << "0123456789";
   }

   if (Reopen) {
      OF.close();
      EXPECT_FALSE(fs::open_file_for_read(TestPath.getCStr(), TestFD));
   }

   OptionalError<OwningBuffer> Buf =
         MemoryBuffer::getOpenFileSlice(TestFD, TestPath.getCStr(),
                                        40000, // Size
                                        80000  // Offset
                                        );

   std::error_code errorCode = Buf.getError();
   EXPECT_FALSE(errorCode);

   StringRef BufData = Buf.get()->getBuffer();
   EXPECT_EQ(BufData.size(), 40000U);
   EXPECT_EQ(BufData[0], '0');
   EXPECT_EQ(BufData[9], '9');
}

TEST_F(MemoryBufferTest, testGetOpenFileNoReopen)
{
   testGetOpenFileSlice(false);
}

TEST_F(MemoryBufferTest, testGetOpenFileReopened)
{
   testGetOpenFileSlice(true);
}

TEST_F(MemoryBufferTest, testReference)
{
   OwningBuffer memoryBuffer(MemoryBuffer::getMemBuffer(data));
   MemoryBufferRef MBR(*memoryBuffer);

   EXPECT_EQ(memoryBuffer->getBufferStart(), MBR.getBufferStart());
   EXPECT_EQ(memoryBuffer->getBufferIdentifier(), MBR.getBufferIdentifier());
}

TEST_F(MemoryBufferTest, testSlice)
{
   // Create a file that is six pages long with different data on each page.
   int fd;
   SmallString<64> TestPath;
   fs::create_temporary_file("MemoryBufferTest_Slice", "temp", fd, TestPath);
   FileRemover Cleanup(TestPath);
   RawFdOutStream OF(fd, true, /*unbuffered=*/true);
   for (unsigned i = 0; i < 0x2000 / 8; ++i) {
      OF << "12345678";
   }
   for (unsigned i = 0; i < 0x2000 / 8; ++i) {
      OF << "abcdefgh";
   }
   for (unsigned i = 0; i < 0x2000 / 8; ++i) {
      OF << "ABCDEFGH";
   }
   OF.close();

   // Try offset of one page.
   OptionalError<OwningBuffer> memoryBuffer = MemoryBuffer::getFileSlice(TestPath.getStr(),
                                                                         0x4000, 0x1000);
   std::error_code errorCode = memoryBuffer.getError();
   ASSERT_FALSE(errorCode);
   EXPECT_EQ(0x4000UL, memoryBuffer.get()->getBufferSize());

   StringRef BufData = memoryBuffer.get()->getBuffer();
   EXPECT_TRUE(BufData.substr(0x0000,8).equals("12345678"));
   EXPECT_TRUE(BufData.substr(0x0FF8,8).equals("12345678"));
   EXPECT_TRUE(BufData.substr(0x1000,8).equals("abcdefgh"));
   EXPECT_TRUE(BufData.substr(0x2FF8,8).equals("abcdefgh"));
   EXPECT_TRUE(BufData.substr(0x3000,8).equals("ABCDEFGH"));
   EXPECT_TRUE(BufData.substr(0x3FF8,8).equals("ABCDEFGH"));

   // Try non-page aligned.
   OptionalError<OwningBuffer> MB2 = MemoryBuffer::getFileSlice(TestPath.getStr(),
                                                                0x3000, 0x0800);
   errorCode = MB2.getError();
   ASSERT_FALSE(errorCode);
   EXPECT_EQ(0x3000UL, MB2.get()->getBufferSize());

   StringRef BufData2 = MB2.get()->getBuffer();
   EXPECT_TRUE(BufData2.substr(0x0000,8).equals("12345678"));
   EXPECT_TRUE(BufData2.substr(0x17F8,8).equals("12345678"));
   EXPECT_TRUE(BufData2.substr(0x1800,8).equals("abcdefgh"));
   EXPECT_TRUE(BufData2.substr(0x2FF8,8).equals("abcdefgh"));
}

TEST_F(MemoryBufferTest, testWritableSlice)
{
   // Create a file initialized with some data
   int fd;
   SmallString<64> TestPath;
   fs::create_temporary_file("MemoryBufferTest_WritableSlice", "temp", fd,
                             TestPath);
   FileRemover Cleanup(TestPath);
   RawFdOutStream OF(fd, true);
   for (unsigned i = 0; i < 0x1000; ++i)
      OF << "0123456789abcdef";
   OF.close();

   {
      auto MBOrError =
            WritableMemoryBuffer::getFileSlice(TestPath.getStr(), 0x6000, 0x2000);
      ASSERT_FALSE(MBOrError.getError());
      // Write some data.  It should be mapped private, so that upon completion
      // the original file contents are not modified.
      WritableMemoryBuffer &memoryBuffer = **MBOrError;
      ASSERT_EQ(0x6000u, memoryBuffer.getBufferSize());
      char *Start = memoryBuffer.getBufferStart();
      ASSERT_EQ(memoryBuffer.getBufferEnd(), memoryBuffer.getBufferStart() + memoryBuffer.getBufferSize());
      ::memset(Start, 'x', memoryBuffer.getBufferSize());
   }

   auto MBOrError = MemoryBuffer::getFile(TestPath);
   ASSERT_FALSE(MBOrError.getError());
   auto &memoryBuffer = **MBOrError;
   ASSERT_EQ(0x10000u, memoryBuffer.getBufferSize());
   for (size_t i = 0; i < memoryBuffer.getBufferSize(); i += 0x10) {
      EXPECT_EQ("0123456789abcdef", memoryBuffer.getBuffer().substr(i, 0x10)) << "i: " << i;
   }
}

TEST_F(MemoryBufferTest, writeThroughFile)
{
   // Create a file initialized with some data
   int fd;
   SmallString<64> TestPath;
   fs::create_temporary_file("MemoryBufferTest_WriteThrough", "temp", fd,
                                TestPath);
   FileRemover Cleanup(TestPath);
   RawFdOutStream OF(fd, true);
   OF << "0123456789abcdef";
   OF.close();
   {
      auto MBOrError = WriteThroughMemoryBuffer::getFile(TestPath);
      ASSERT_FALSE(MBOrError.getError());
      // Write some data.  It should be mapped readwrite, so that upon completion
      // the original file contents are modified.
      WriteThroughMemoryBuffer &md = **MBOrError;
      ASSERT_EQ(16u, md.getBufferSize());
      char *Start = md.getBufferStart();
      ASSERT_EQ(md.getBufferEnd(), md.getBufferStart() + md.getBufferSize());
      ::memset(Start, 'x', md.getBufferSize());
   }

   auto MBOrError = MemoryBuffer::getFile(TestPath);
   ASSERT_FALSE(MBOrError.getError());
   auto &md = **MBOrError;
   ASSERT_EQ(16u, md.getBufferSize());
   EXPECT_EQ("xxxxxxxxxxxxxxxx", md.getBuffer());
}

} // anonymous namespace
