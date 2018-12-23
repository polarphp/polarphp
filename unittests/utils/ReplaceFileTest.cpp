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

#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Process.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;
using namespace polar::sys;

#define ASSERT_NO_ERROR(x)                                                 \
   do {                                                                     \
   if (std::error_code ASSERT_NO_ERROR_ec = x) {                          \
   error_stream() << #x ": did not return errc::success.\n"                     \
   << "error number: " << ASSERT_NO_ERROR_ec.value() << "\n"     \
   << "error message: " << ASSERT_NO_ERROR_ec.message() << "\n"; \
   }                                                                      \
   } while (false)


namespace {

std::error_code create_file_with_content(const SmallString<128> &filePath,
                                         const StringRef &content)
{
   int fd = 0;
   if (std::error_code ec = fs::open_file_for_write(filePath, fd))
      return ec;

   const bool shouldClose = true;
   RawFdOutStream OS(fd, shouldClose);
   OS << content;

   return std::error_code();
}

class ScopedFD
{
   int fd;

   ScopedFD(const ScopedFD &) = delete;
   ScopedFD &operator=(const ScopedFD &) = delete;

public:
   explicit ScopedFD(int descriptor) : fd(descriptor) {}
   ~ScopedFD() { Process::safelyCloseFileDescriptor(fd); }
};

bool fd_has_content(int fd, StringRef content)
{
   auto Buffer = MemoryBuffer::getOpenFile(fd, "", -1);
   assert(Buffer);
   return Buffer.get()->getBuffer() == content;
}

bool file_has_content(StringRef file, StringRef content)
{
   int fd = 0;
   auto EC = fs::open_file_for_read(file, fd);
   (void)EC;
   assert(!EC);
   ScopedFD EventuallyCloseIt(fd);
   return fd_has_content(fd, content);
}

TEST(ReplaceFileTest, testFileOpenedForReadingCanBeReplaced)
{
   // Create unique temporary directory for this test.
   SmallString<128> testDirectory;
   ASSERT_NO_ERROR(fs::create_unique_directory(
                      "FileOpenedForReadingCanBeReplaced-test", testDirectory));

   // Add a couple of files to the test directory.
   SmallString<128> sourceFileName(testDirectory);
   fs::path::append(sourceFileName, "source");

   SmallString<128> targetFileName(testDirectory);
   fs::path::append(targetFileName, "target");

   ASSERT_NO_ERROR(create_file_with_content(sourceFileName, "!!source!!"));
   ASSERT_NO_ERROR(create_file_with_content(targetFileName, "!!target!!"));

   {
      // Open the target file for reading.
      int readFD = 0;
      ASSERT_NO_ERROR(fs::open_file_for_read(targetFileName, readFD));
      ScopedFD EventuallyCloseIt(readFD);

      // Confirm we can replace the file while it is open.
      EXPECT_TRUE(!fs::rename(sourceFileName, targetFileName));

      // We should still be able to read the old data through the existing
      // descriptor.
      EXPECT_TRUE(fd_has_content(readFD, "!!target!!"));

      // The source file should no longer exist
      EXPECT_FALSE(fs::exists(sourceFileName));
   }

   // If we obtain a new descriptor for the target file, we should find that it
   // contains the content that was in the source file.
   EXPECT_TRUE(file_has_content(targetFileName, "!!source!!"));

   // Rename the target file back to the source file name to confirm that rename
   // still works if the destination does not already exist.
   EXPECT_TRUE(!fs::rename(targetFileName, sourceFileName));
   EXPECT_FALSE(fs::exists(targetFileName));
   ASSERT_TRUE(fs::exists(sourceFileName));

   // Clean up.
   ASSERT_NO_ERROR(fs::remove(sourceFileName));
   ASSERT_NO_ERROR(fs::remove(testDirectory.getStr()));
}

TEST(ReplaceFileTest, testExistingTemp)
{
   // Test that existing .tmpN files don't get deleted by the Windows
   // sys::fs::rename implementation.
   SmallString<128> testDirectory;
   ASSERT_NO_ERROR(
            fs::create_unique_directory("ExistingTemp-test", testDirectory));

   SmallString<128> sourceFileName(testDirectory);
   fs::path::append(sourceFileName, "source");

   SmallString<128> targetFileName(testDirectory);
   fs::path::append(targetFileName, "target");

   SmallString<128> TargetTmp0FileName(testDirectory);
   fs::path::append(TargetTmp0FileName, "target.tmp0");

   SmallString<128> TargetTmp1FileName(testDirectory);
   fs::path::append(TargetTmp1FileName, "target.tmp1");

   ASSERT_NO_ERROR(create_file_with_content(sourceFileName, "!!source!!"));
   ASSERT_NO_ERROR(create_file_with_content(targetFileName, "!!target!!"));
   ASSERT_NO_ERROR(create_file_with_content(TargetTmp0FileName, "!!target.tmp0!!"));

   {
      // Use mapped_file_region to make sure that the destination file is mmap'ed.
      // This will cause SetInformationByHandle to fail when renaming to the
      // destination, and we will follow the code path that tries to give target
      // a temporary name.
      int TargetFD;
      std::error_code EC;
      ASSERT_NO_ERROR(fs::open_file_for_read(targetFileName, TargetFD));
      ScopedFD X(TargetFD);
      fs::MappedFileRegion MFR(
               TargetFD, fs::MappedFileRegion::readonly, 10, 0, EC);
      ASSERT_FALSE(EC);

      ASSERT_NO_ERROR(fs::rename(sourceFileName, targetFileName));

#ifdef _WIN32
      // Make sure that target was temporarily renamed to target.tmp1 on Windows.
      // This is signified by a permission denied error as opposed to no such file
      // or directory when trying to open it.
      int Tmp1FD;
      EXPECT_EQ(ErrorCode::permission_denied,
                fs::open_file_for_read(TargetTmp1FileName, Tmp1FD));
#endif
   }

   EXPECT_TRUE(file_has_content(TargetTmp0FileName, "!!target.tmp0!!"));

   ASSERT_NO_ERROR(fs::remove(targetFileName));
   ASSERT_NO_ERROR(fs::remove(TargetTmp0FileName));
   ASSERT_NO_ERROR(fs::remove(testDirectory.getStr()));
}

} // anonymous namespace
