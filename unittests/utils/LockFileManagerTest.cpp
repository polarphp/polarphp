// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

// Created by polarboy on 2018/07/13.

#include "polarphp/utils/LockFileMgr.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"
#include "gtest/gtest.h"
#include <memory>

using namespace polar::basic;
using namespace polar::utils;
using namespace polar;

namespace {

TEST(LockFileManagerTest, testBasic)
{
   SmallString<64> TmpDir;
   std::error_code errorCode;
   errorCode = fs::create_unique_directory("LockFileManagerTestDir", TmpDir);
   ASSERT_FALSE(errorCode);

   SmallString<64> LockedFile(TmpDir);
   fs::path::append(LockedFile, "file.lock");

   {
      // The lock file should not exist, so we should successfully acquire it.
      LockFileManager Locked1(LockedFile);
      EXPECT_EQ(LockFileManager::LFS_Owned, Locked1.getState());

      // Attempting to reacquire the lock should fail.  Waiting on it would cause
      // deadlock, so don't try that.
      LockFileManager Locked2(LockedFile);
      EXPECT_NE(LockFileManager::LFS_Owned, Locked2.getState());
   }

   // Now that the lock is out of scope, the file should be gone.
   EXPECT_FALSE(fs::exists(StringRef(LockedFile)));

   errorCode = fs::remove(StringRef(TmpDir));
   ASSERT_FALSE(errorCode);
}

TEST(LockFileManagerTest, testLinkLockExists)
{
   SmallString<64> TmpDir;
   std::error_code errorCode;
   errorCode = fs::create_unique_directory("LockFileManagerTestDir", TmpDir);
   ASSERT_FALSE(errorCode);

   SmallString<64> LockedFile(TmpDir);
   fs::path::append(LockedFile, "file");

   SmallString<64> FileLocK(TmpDir);
   fs::path::append(FileLocK, "file.lock");

   SmallString<64> TmpFileLock(TmpDir);
   fs::path::append(TmpFileLock, "file.lock-000");

   int FD;
   errorCode = fs::open_file_for_write(StringRef(TmpFileLock), FD);
   ASSERT_FALSE(errorCode);

   int Ret = close(FD);
   ASSERT_EQ(Ret, 0);

   errorCode = fs::create_link(TmpFileLock.getStr(), FileLocK.getStr());
   ASSERT_FALSE(errorCode);

   errorCode = fs::remove(StringRef(TmpFileLock));
   ASSERT_FALSE(errorCode);

   {
      // The lock file doesn't point to a real file, so we should successfully
      // acquire it.
      LockFileManager Locked(LockedFile);
      EXPECT_EQ(LockFileManager::LFS_Owned, Locked.getState());
   }

   // Now that the lock is out of scope, the file should be gone.
   EXPECT_FALSE(fs::exists(StringRef(LockedFile)));

   errorCode = fs::remove(StringRef(TmpDir));
   ASSERT_FALSE(errorCode);
}


TEST(LockFileManagerTest, testRelativePath)
{
   SmallString<64> TmpDir;
   std::error_code errorCode;
   errorCode = fs::create_unique_directory("LockFileManagerTestDir", TmpDir);
   ASSERT_FALSE(errorCode);

   char PathBuf[1024];
   const char *OrigPath = getcwd(PathBuf, 1024);
   ASSERT_FALSE(chdir(TmpDir.getCStr()));

   fs::create_directory("inner");
   SmallString<64> LockedFile("inner");
   fs::path::append(LockedFile, "file");

   SmallString<64> FileLock(LockedFile);
   FileLock += ".lock";

   {
      // The lock file should not exist, so we should successfully acquire it.
      LockFileManager Locked(LockedFile);
      EXPECT_EQ(LockFileManager::LFS_Owned, Locked.getState());
      EXPECT_TRUE(fs::exists(FileLock.getStr()));
   }

   // Now that the lock is out of scope, the file should be gone.
   EXPECT_FALSE(fs::exists(LockedFile.getStr()));
   EXPECT_FALSE(fs::exists(FileLock.getStr()));

   errorCode = fs::remove("inner");
   ASSERT_FALSE(errorCode);

   ASSERT_FALSE(chdir(OrigPath));

   errorCode = fs::remove(StringRef(TmpDir));
   ASSERT_FALSE(errorCode);
}

} // anonymous namespace
