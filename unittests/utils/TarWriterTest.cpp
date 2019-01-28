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

#include "polarphp/utils/TarWriter.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "gtest/gtest.h"
#include <vector>

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;

namespace {

struct UstarHeader {
   char Name[100];
   char Mode[8];
   char Uid[8];
   char Gid[8];
   char Size[12];
   char Mtime[12];
   char Checksum[8];
   char TypeFlag;
   char Linkname[100];
   char Magic[6];
   char Version[2];
   char Uname[32];
   char Gname[32];
   char DevMajor[8];
   char DevMinor[8];
   char Prefix[155];
   char Pad[12];
};

class TarWriterTest : public ::testing::Test {};

static std::vector<uint8_t> createTar(StringRef Base, StringRef filename)
{
   // Create a temporary file.
   SmallString<128> path;
   std::error_code errorCode =
         fs::create_temporary_file("TarWriterTest", "tar", path);
   EXPECT_FALSE((bool)errorCode);

   // Create a tar file.
   Expected<std::unique_ptr<TarWriter>> TarOrErr = TarWriter::create(path, Base);
   EXPECT_TRUE((bool)TarOrErr);
   std::unique_ptr<TarWriter> Tar = std::move(*TarOrErr);
   Tar->append(filename, "contents");
   Tar.reset();

   // Read the tar file.
   OptionalError<std::unique_ptr<MemoryBuffer>> MBOrErr = MemoryBuffer::getFile(path);
   EXPECT_TRUE((bool)MBOrErr);
   std::unique_ptr<MemoryBuffer> MB = std::move(*MBOrErr);
   std::vector<uint8_t> buffer((const uint8_t *)MB->getBufferStart(),
                            (const uint8_t *)MB->getBufferEnd());

   // Windows does not allow us to remove a mmap'ed files, so
   // unmap first and then remove the temporary file.
   MB = nullptr;
   fs::remove(path);

   return buffer;
}

static UstarHeader createUstar(StringRef Base, StringRef filename) {
   std::vector<uint8_t> buffer = createTar(Base, filename);
   EXPECT_TRUE(buffer.size() >= sizeof(UstarHeader));
   return *reinterpret_cast<const UstarHeader *>(buffer.data());
}

TEST_F(TarWriterTest, testBasics)
{
   UstarHeader header = createUstar("base", "file");
   EXPECT_EQ("ustar", StringRef(header.Magic));
   EXPECT_EQ("00", StringRef(header.Version, 2));
   EXPECT_EQ("base/file", StringRef(header.Name));
   EXPECT_EQ("00000000010", StringRef(header.Size));
}

TEST_F(TarWriterTest, testLongFilename)
{
   std::string x154(154, 'x');
   std::string x155(155, 'x');
   std::string y99(99, 'y');
   std::string y100(100, 'y');

   UstarHeader Hdr1 = createUstar("", x154 + "/" + y99);
   EXPECT_EQ("/" + x154, StringRef(Hdr1.Prefix));
   EXPECT_EQ(y99, StringRef(Hdr1.Name));

   UstarHeader Hdr2 = createUstar("", x155 + "/" + y99);
   EXPECT_EQ("", StringRef(Hdr2.Prefix));
   EXPECT_EQ("", StringRef(Hdr2.Name));

   UstarHeader Hdr3 = createUstar("", x154 + "/" + y100);
   EXPECT_EQ("", StringRef(Hdr3.Prefix));
   EXPECT_EQ("", StringRef(Hdr3.Name));

   UstarHeader Hdr4 = createUstar("", x155 + "/" + y100);
   EXPECT_EQ("", StringRef(Hdr4.Prefix));
   EXPECT_EQ("", StringRef(Hdr4.Name));

   std::string yz = "yyyyyyyyyyyyyyyyyyyy/zzzzzzzzzzzzzzzzzzzz";
   UstarHeader Hdr5 = createUstar("", x154 + "/" + yz);
   EXPECT_EQ("/" + x154, StringRef(Hdr5.Prefix));
   EXPECT_EQ(yz, StringRef(Hdr5.Name));
}

TEST_F(TarWriterTest, testPax)
{
   std::vector<uint8_t> buffer = createTar("", std::string(200, 'x'));
   EXPECT_TRUE(buffer.size() >= 1024);

   auto *header = reinterpret_cast<const UstarHeader *>(buffer.data());
   EXPECT_EQ("", StringRef(header->Prefix));
   EXPECT_EQ("", StringRef(header->Name));

   StringRef pax = StringRef((char *)(buffer.data() + 512), 512);
   EXPECT_TRUE(pax.startsWith("211 path=/" + std::string(200, 'x')));
}

TEST_F(TarWriterTest, testSingleFile)
{
   SmallString<128> path;
   std::error_code errorCode =
         fs::create_temporary_file("TarWriterTest", "tar", path);
   EXPECT_FALSE((bool)errorCode);

   Expected<std::unique_ptr<TarWriter>> TarOrErr = TarWriter::create(path, "");
   EXPECT_TRUE((bool)TarOrErr);
   std::unique_ptr<TarWriter> Tar = std::move(*TarOrErr);
   Tar->append("FooPath", "foo");
   Tar.reset();

   uint64_t TarSize;
   errorCode = fs::file_size(path, TarSize);
   EXPECT_FALSE((bool)errorCode);
   EXPECT_EQ(TarSize, 2048ULL);
}

TEST_F(TarWriterTest, testNoDuplicate)
{
   SmallString<128> path;
   std::error_code errorCode =
         fs::create_temporary_file("TarWriterTest", "tar", path);
   EXPECT_FALSE((bool)errorCode);

   Expected<std::unique_ptr<TarWriter>> TarOrErr = TarWriter::create(path, "");
   EXPECT_TRUE((bool)TarOrErr);
   std::unique_ptr<TarWriter> Tar = std::move(*TarOrErr);
   Tar->append("FooPath", "foo");
   Tar->append("BarPath", "bar");
   Tar.reset();

   uint64_t TarSize;
   errorCode = fs::file_size(path, TarSize);
   EXPECT_FALSE((bool)errorCode);
   EXPECT_EQ(TarSize, 3072ULL);
}

TEST_F(TarWriterTest, testDuplicate)
{
   SmallString<128> path;
   std::error_code errorCode =
         fs::create_temporary_file("TarWriterTest", "tar", path);
   EXPECT_FALSE((bool)errorCode);

   Expected<std::unique_ptr<TarWriter>> TarOrErr = TarWriter::create(path, "");
   EXPECT_TRUE((bool)TarOrErr);
   std::unique_ptr<TarWriter> Tar = std::move(*TarOrErr);
   Tar->append("FooPath", "foo");
   Tar->append("FooPath", "bar");
   Tar.reset();

   uint64_t TarSize;
   errorCode = fs::file_size(path, TarSize);
   EXPECT_FALSE((bool)errorCode);
   EXPECT_EQ(TarSize, 2048ULL);
}

} // anonymous namespace
