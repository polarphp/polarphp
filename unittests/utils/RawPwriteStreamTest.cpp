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

#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/FileUtils.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;
using namespace polar::fs;

#define ASSERT_NO_ERROR(x)                                                     \
   if (std::error_code ASSERT_NO_ERROR_ec = x) {                                \
   SmallString<128> MessageStorage;                                           \
   RawSvectorOutStream Message(MessageStorage);                               \
   Message << #x ": did not return errc::success.\n"                          \
   << "error number: " << ASSERT_NO_ERROR_ec.value() << "\n"          \
   << "error message: " << ASSERT_NO_ERROR_ec.message() << "\n";      \
   GTEST_FATAL_FAILURE_(MessageStorage.getCStr());                              \
   } else {                                                                     \
   }

namespace {

TEST(RawPwriteOutStreamTest, testSVector)
{
   SmallVector<char, 0> Buffer;
   RawSvectorOutStream outstream(Buffer);
   outstream << "abcd";
   StringRef Test = "test";
   outstream.pwrite(Test.getData(), Test.size(), 0);
   EXPECT_EQ(Test, outstream.getStr());

#ifdef GTEST_HAS_DEATH_TEST
#ifndef NDEBUG
   EXPECT_DEATH(outstream.pwrite("12345", 5, 0),
                "We don't support extending the stream");
#endif
#endif
}

#ifdef _WIN32
#define setenv(name, var, ignore) _putenv_s(name, var)
#endif

TEST(RawPwriteOutStreamTest, testFD)
{
   SmallString<64> Path;
   int FD;

   // If we want to clean up from a death test, we have to remove the file from
   // the parent process. Have the parent create the file, pass it via
   // environment variable to the child, let the child crash, and then remove it
   // in the parent.
   const char *ParentPath = getenv("RAW_PWRITE_TEST_FILE");
   if (ParentPath) {
      Path = ParentPath;
      ASSERT_NO_ERROR(fs::open_file_for_read(Path, FD));
   } else {
      ASSERT_NO_ERROR(fs::create_temporary_file("foo", "bar", FD, Path));
      setenv("RAW_PWRITE_TEST_FILE", Path.getCStr(), true);
   }
   FileRemover Cleanup(Path);

   RawFdOutStream outstream(FD, true);
   outstream << "abcd";
   StringRef Test = "test";
   outstream.pwrite(Test.getData(), Test.size(), 0);
   outstream.pwrite(Test.getData(), Test.size(), 0);

#ifdef GTEST_HAS_DEATH_TEST
#ifndef NDEBUG
   EXPECT_DEATH(outstream.pwrite("12345", 5, 0),
                "We don't support extending the stream");
#endif
#endif
}

#ifdef POLAR_ON_UNIX
TEST(RawPwriteOutStreamTest, testDevNull)
{
   int FD;
   fs::open_file_for_write("/dev/null", FD, fs::CD_OpenExisting);
   RawFdOutStream outstream(FD, true);
   outstream << "abcd";
   StringRef Test = "test";
   outstream.pwrite(Test.getData(), Test.size(), 0);
   outstream.pwrite(Test.getData(), Test.size(), 0);
}
#endif

} // anonymous namespace
