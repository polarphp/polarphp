//===- unittests/Support/VirtualFileSystem.cpp -------------- VFS tests ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/09.


#include "polarphp/utils/VirtualFileSystem.h"
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/global/Config.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/SourceMgr.h"
#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <map>
#include <string>

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;
using polar::fs::UniqueId;
using polar::utils::ErrorCode;
using testing::ElementsAre;
using testing::Pair;
using testing::UnorderedElementsAre;
using polar::utils::OptionalError;
using polar::utils::MemoryBuffer;

namespace {
struct DummyFile : public vfs::File {
   vfs::Status S;
   explicit DummyFile(vfs::Status S) : S(S) {}
   OptionalError<vfs::Status> getStatus() override { return S; }
   OptionalError<std::unique_ptr<MemoryBuffer>>
   getBuffer(const Twine &Name, int64_t FileSize, bool RequiresNullTerminator,
             bool IsVolatile) override {
      polar_unreachable("unimplemented");
   }
   std::error_code close() override { return std::error_code(); }
};

class DummyFileSystem : public vfs::FileSystem
{
   int FSID;   // used to produce UniqueIDs
   int FileID; // used to produce UniqueIDs
   std::map<std::string, vfs::Status> FilesAndDirs;

   static int getNextFsId() {
      static int Count = 0;
      return Count++;
   }

public:
   DummyFileSystem() : FSID(getNextFsId()), FileID(0) {}

   OptionalError<vfs::Status> getStatus(const Twine &path) override {
      std::map<std::string, vfs::Status>::iterator I =
            FilesAndDirs.find(path.getStr());
      if (I == FilesAndDirs.end())
         return make_error_code(ErrorCode::no_such_file_or_directory);
      return I->second;
   }
   OptionalError<std::unique_ptr<vfs::File>>
   openFileForRead(const Twine &path) override {
      auto S = getStatus(path);
      if (S)
         return std::unique_ptr<vfs::File>(new DummyFile{*S});
      return S.getError();
   }
   OptionalError<std::string> getCurrentWorkingDirectory() const override {
      return std::string();
   }
   std::error_code setCurrentWorkingDirectory(const Twine &path) override {
      return std::error_code();
   }
   // Map any symlink to "/symlink".
   std::error_code getRealPath(const Twine &path,
                               SmallVectorImpl<char> &Output) const override {
      auto I = FilesAndDirs.find(path.getStr());
      if (I == FilesAndDirs.end())
         return make_error_code(ErrorCode::no_such_file_or_directory);
      if (I->second.isSymlink()) {
         Output.clear();
         Twine("/symlink").toVector(Output);
         return std::error_code();
      }
      Output.clear();
      path.toVector(Output);
      return std::error_code();
   }

   struct DirIterImpl : public polar::vfs::internal::DirIterImpl {
      std::map<std::string, vfs::Status> &FilesAndDirs;
      std::map<std::string, vfs::Status>::iterator I;
      std::string path;
      bool isInPath(StringRef S) {
         if (path.size() < S.size() && S.find(path) == 0) {
            auto LastSep = S.findLastOf('/');
            if (LastSep == path.size() || LastSep == path.size() - 1)
               return true;
         }
         return false;
      }
      DirIterImpl(std::map<std::string, vfs::Status> &FilesAndDirs,
                  const Twine &_path)
         : FilesAndDirs(FilesAndDirs), I(FilesAndDirs.begin()),
           path(_path.getStr()) {
         for (; I != FilesAndDirs.end(); ++I) {
            if (isInPath(I->first)) {
               m_currentEntry =
                     vfs::DirectoryEntry(I->second.getName(), I->second.getType());
               break;
            }
         }
      }
      std::error_code increment() override {
         ++I;
         for (; I != FilesAndDirs.end(); ++I) {
            if (isInPath(I->first)) {
               m_currentEntry =
                     vfs::DirectoryEntry(I->second.getName(), I->second.getType());
               break;
            }
         }
         if (I == FilesAndDirs.end())
            m_currentEntry = vfs::DirectoryEntry();
         return std::error_code();
      }
   };

   vfs::DirectoryIterator dirBegin(const Twine &Dir,
                                   std::error_code &errorCode) override {
      return vfs::DirectoryIterator(
               std::make_shared<DirIterImpl>(FilesAndDirs, Dir));
   }

   void addEntry(StringRef path, const vfs::Status &Status) {
      FilesAndDirs[path] = Status;
   }

   void addRegularFile(StringRef path, polar::fs::Permission Perms = polar::fs::all_all) {
      vfs::Status S(path, UniqueId(FSID, FileID++),
                    std::chrono::system_clock::now(), 0, 0, 1024,
                    polar::fs::FileType::regular_file, Perms);
      addEntry(path, S);
   }

   void addDirectory(StringRef path, polar::fs::Permission Perms = polar::fs::all_all) {
      vfs::Status S(path, UniqueId(FSID, FileID++),
                    std::chrono::system_clock::now(), 0, 0, 0,
                    polar::fs::FileType::directory_file, Perms);
      addEntry(path, S);
   }

   void addSymlink(StringRef path) {
      vfs::Status S(path, UniqueId(FSID, FileID++),
                    std::chrono::system_clock::now(), 0, 0, 0,
                    polar::fs::FileType::symlink_file, polar::fs::all_all);
      addEntry(path, S);
   }
};

/// Replace back-slashes by front-slashes.
std::string getPosixPath(std::string S) {
   SmallString<128> Result;
   polar::fs::path::native(S, Result, polar::fs::path::Style::posix);
   return Result.getStr();
}
} // end anonymous namespace

TEST(VirtualFileSystemTest, testStatusQueries)
{
   IntrusiveRefCountPtr<DummyFileSystem> D(new DummyFileSystem());
   OptionalError<vfs::Status> Status((std::error_code()));

   D->addRegularFile("/foo");
   Status = D->getStatus("/foo");
   ASSERT_FALSE(Status.getError());
   EXPECT_TRUE(Status->isStatusKnown());
   EXPECT_FALSE(Status->isDirectory());
   EXPECT_TRUE(Status->isRegularFile());
   EXPECT_FALSE(Status->isSymlink());
   EXPECT_FALSE(Status->isOther());
   EXPECT_TRUE(Status->exists());

   D->addDirectory("/bar");
   Status = D->getStatus("/bar");
   ASSERT_FALSE(Status.getError());
   EXPECT_TRUE(Status->isStatusKnown());
   EXPECT_TRUE(Status->isDirectory());
   EXPECT_FALSE(Status->isRegularFile());
   EXPECT_FALSE(Status->isSymlink());
   EXPECT_FALSE(Status->isOther());
   EXPECT_TRUE(Status->exists());

   D->addSymlink("/baz");
   Status = D->getStatus("/baz");
   ASSERT_FALSE(Status.getError());
   EXPECT_TRUE(Status->isStatusKnown());
   EXPECT_FALSE(Status->isDirectory());
   EXPECT_FALSE(Status->isRegularFile());
   EXPECT_TRUE(Status->isSymlink());
   EXPECT_FALSE(Status->isOther());
   EXPECT_TRUE(Status->exists());

   EXPECT_TRUE(Status->equivalent(*Status));
   OptionalError<vfs::Status> Status2 = D->getStatus("/foo");
   ASSERT_FALSE(Status2.getError());
   EXPECT_FALSE(Status->equivalent(*Status2));
}

TEST(VirtualFileSystemTest, testBaseOnlyOverlay)
{
   IntrusiveRefCountPtr<DummyFileSystem> D(new DummyFileSystem());
   OptionalError<vfs::Status> Status((std::error_code()));
   EXPECT_FALSE(Status = D->getStatus("/foo"));

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(new vfs::OverlayFileSystem(D));
   EXPECT_FALSE(Status = O->getStatus("/foo"));

   D->addRegularFile("/foo");
   Status = D->getStatus("/foo");
   EXPECT_FALSE(Status.getError());

   OptionalError<vfs::Status> Status2((std::error_code()));
   Status2 = O->getStatus("/foo");
   EXPECT_FALSE(Status2.getError());
   EXPECT_TRUE(Status->equivalent(*Status2));
}

TEST(VirtualFileSystemTest, testGetRealPathInOverlay)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("/foo");
   Lower->addSymlink("/lower_link");
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Upper);

   // Regular file.
   SmallString<16> RealPath;
   EXPECT_FALSE(O->getRealPath("/foo", RealPath));
   EXPECT_EQ(RealPath.getStr(), "/foo");

   // Expect no error getting real path for symlink in lower overlay.
   EXPECT_FALSE(O->getRealPath("/lower_link", RealPath));
   EXPECT_EQ(RealPath.getStr(), "/symlink");

   // Try a non-existing link.
   EXPECT_EQ(O->getRealPath("/upper_link", RealPath),
             ErrorCode::no_such_file_or_directory);

   // Add a new symlink in upper.
   Upper->addSymlink("/upper_link");
   EXPECT_FALSE(O->getRealPath("/upper_link", RealPath));
   EXPECT_EQ(RealPath.getStr(), "/symlink");
}

TEST(VirtualFileSystemTest, testOverlayFiles)
{
   IntrusiveRefCountPtr<DummyFileSystem> Base(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Middle(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Top(new DummyFileSystem());
   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Base));
   O->pushOverlay(Middle);
   O->pushOverlay(Top);

   OptionalError<vfs::Status> Status1((std::error_code())),
         Status2((std::error_code())), Status3((std::error_code())),
         StatusB((std::error_code())), StatusM((std::error_code())),
         StatusT((std::error_code()));

   Base->addRegularFile("/foo");
   StatusB = Base->getStatus("/foo");
   ASSERT_FALSE(StatusB.getError());
   Status1 = O->getStatus("/foo");
   ASSERT_FALSE(Status1.getError());
   Middle->addRegularFile("/foo");
   StatusM = Middle->getStatus("/foo");
   ASSERT_FALSE(StatusM.getError());
   Status2 = O->getStatus("/foo");
   ASSERT_FALSE(Status2.getError());
   Top->addRegularFile("/foo");
   StatusT = Top->getStatus("/foo");
   ASSERT_FALSE(StatusT.getError());
   Status3 = O->getStatus("/foo");
   ASSERT_FALSE(Status3.getError());

   EXPECT_TRUE(Status1->equivalent(*StatusB));
   EXPECT_TRUE(Status2->equivalent(*StatusM));
   EXPECT_TRUE(Status3->equivalent(*StatusT));

   EXPECT_FALSE(Status1->equivalent(*Status2));
   EXPECT_FALSE(Status2->equivalent(*Status3));
   EXPECT_FALSE(Status1->equivalent(*Status3));
}

TEST(VirtualFileSystemTest, testOverlayDirsNonMerged)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());
   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Upper);

   Lower->addDirectory("/lower-only");
   Upper->addDirectory("/upper-only");

   // non-merged paths should be the same
   OptionalError<vfs::Status> Status1 = Lower->getStatus("/lower-only");
   ASSERT_FALSE(Status1.getError());
   OptionalError<vfs::Status> Status2 = O->getStatus("/lower-only");
   ASSERT_FALSE(Status2.getError());
   EXPECT_TRUE(Status1->equivalent(*Status2));

   Status1 = Upper->getStatus("/upper-only");
   ASSERT_FALSE(Status1.getError());
   Status2 = O->getStatus("/upper-only");
   ASSERT_FALSE(Status2.getError());
   EXPECT_TRUE(Status1->equivalent(*Status2));
}

TEST(VirtualFileSystemTest, testMergedDirPermissions)
{
   // merged directories get the permissions of the upper dir
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());
   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Upper);

   OptionalError<vfs::Status> Status((std::error_code()));
   Lower->addDirectory("/both", polar::fs::owner_read);
   Upper->addDirectory("/both", polar::fs::owner_all | polar::fs::group_read);
   Status = O->getStatus("/both");
   ASSERT_FALSE(Status.getError());
   EXPECT_EQ(0740, Status->getPermissions());

   // permissions (as usual) are not recursively applied
   Lower->addRegularFile("/both/foo", polar::fs::owner_read);
   Upper->addRegularFile("/both/bar", polar::fs::owner_write);
   Status = O->getStatus("/both/foo");
   ASSERT_FALSE(Status.getError());
   EXPECT_EQ(0400, Status->getPermissions());
   Status = O->getStatus("/both/bar");
   ASSERT_FALSE(Status.getError());
   EXPECT_EQ(0200, Status->getPermissions());
}

TEST(VirtualFileSystemTest, testOverlayIterator) {
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("/foo");
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Upper);

   OptionalError<vfs::Status> Status((std::error_code()));
   {
      auto it = O->overlaysBegin();
      auto end = O->overlaysEnd();

      EXPECT_NE(it, end);

      Status = (*it)->getStatus("/foo");
      ASSERT_TRUE(Status.getError());

      it++;
      EXPECT_NE(it, end);

      Status = (*it)->getStatus("/foo");
      ASSERT_FALSE(Status.getError());
      EXPECT_TRUE(Status->exists());

      it++;
      EXPECT_EQ(it, end);
   }

   {
      auto it = O->overlays_rbegin();
      auto end = O->overlays_rend();

      EXPECT_NE(it, end);

      Status = (*it)->getStatus("/foo");
      ASSERT_FALSE(Status.getError());
      EXPECT_TRUE(Status->exists());

      it++;
      EXPECT_NE(it, end);

      Status = (*it)->getStatus("/foo");
      ASSERT_TRUE(Status.getError());

      it++;
      EXPECT_EQ(it, end);
   }
}

namespace {
struct ScopedDir {
   SmallString<128> path;
   ScopedDir(const Twine &Name, bool Unique = false) {
      std::error_code errorCode;
      if (Unique) {
         errorCode = polar::fs::create_unique_directory(Name, path);
         if (!errorCode) {
            // Resolve any symlinks in the new directory.
            std::string unresolvedPath = path.getStr();
            errorCode = fs::real_path(unresolvedPath, path);
         }
      } else {
         path = Name.getStr();
         errorCode = polar::fs::create_directory(Twine(path));
      }
      if (errorCode){
         path = "";
      }
      EXPECT_FALSE(errorCode) << errorCode.message();
   }
   ~ScopedDir() {
      if (path != "") {
         EXPECT_FALSE(polar::fs::remove(path.getStr()));
      }
   }
   operator StringRef() { return path.getStr(); }
};

struct ScopedLink {
   SmallString<128> path;
   ScopedLink(const Twine &To, const Twine &From) {
      path = From.getStr();
      std::error_code errorCode = polar::fs::create_link(To, From);
      if (errorCode)
         path = "";
      EXPECT_FALSE(errorCode);
   }
   ~ScopedLink() {
      if (path != "") {
         EXPECT_FALSE(polar::fs::remove(path.getStr()));
      }
   }
   operator StringRef() { return path.getStr(); }
};

struct ScopedFile {
   SmallString<128> path;
   ScopedFile(const Twine &path, StringRef Contents) {
      path.toVector(this->path);
      std::error_code errorCode;
      RawFdOutStream outStream(this->path, errorCode);
      EXPECT_FALSE(errorCode);
      outStream << Contents;
      outStream.flush();
      EXPECT_FALSE(outStream.getErrorCode());
      if (errorCode || outStream.getErrorCode()) {
         this->path = "";
      }
   }
   ~ScopedFile() {
      if (path != "") {
         EXPECT_FALSE(fs::remove(path.getStr()));
      }
   }
};
} // end anonymous namespace

TEST(VirtualFileSystemTest, testBasicRealFSIteration)
{
   ScopedDir TestDirectory("virtual-file-system-test", /*Unique*/ true);
   IntrusiveRefCountPtr<vfs::FileSystem> FS = vfs::get_real_file_system();

   std::error_code errorCode;
   vfs::DirectoryIterator I = FS->dirBegin(Twine(TestDirectory), errorCode);
   ASSERT_FALSE(errorCode);
   EXPECT_EQ(vfs::DirectoryIterator(), I); // empty directory is empty

   ScopedDir _a(TestDirectory + "/a");
   ScopedDir _ab(TestDirectory + "/a/b");
   ScopedDir _c(TestDirectory + "/c");
   ScopedDir _cd(TestDirectory + "/c/d");

   I = FS->dirBegin(Twine(TestDirectory), errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_NE(vfs::DirectoryIterator(), I);
   // Check either a or c, since we can't rely on the iteration order.
   EXPECT_TRUE(I->path().endsWith("a") || I->path().endsWith("c"));
   I.increment(errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_NE(vfs::DirectoryIterator(), I);
   EXPECT_TRUE(I->path().endsWith("a") || I->path().endsWith("c"));
   I.increment(errorCode);
   EXPECT_EQ(vfs::DirectoryIterator(), I);
}

#ifdef POLAR_ON_UNIX
TEST(VirtualFileSystemTest, testMultipleWorkingDirs) {
   // Our root contains a/aa, b/bb, c, where c is a link to a/.
   // Run tests both in root/b/ and root/c/ (to test "normal" and symlink dirs).
   // Interleave operations to show the working directories are independent.
   ScopedDir root("r", true), ADir(root.path + "/a"), BDir(root.path + "/b");
   ScopedLink C(ADir.path, root.path + "/c");
   ScopedFile AA(ADir.path + "/aa", "aaaa"), BB(BDir.path + "/bb", "bbbb");
   std::unique_ptr<vfs::FileSystem> bfs = vfs::create_physical_file_system(),
         cfs = vfs::create_physical_file_system();

   ASSERT_FALSE(bfs->setCurrentWorkingDirectory(BDir.path));
   ASSERT_FALSE(cfs->setCurrentWorkingDirectory(C.path));
   EXPECT_EQ(BDir.path, *bfs->getCurrentWorkingDirectory());
   EXPECT_EQ(C.path, *cfs->getCurrentWorkingDirectory());

   // openFileForRead(), indirectly.
   auto BBuf = bfs->getBufferForFile("bb");
   ASSERT_TRUE(BBuf);
   EXPECT_EQ("bbbb", (*BBuf)->getBuffer());

   auto ABuf = cfs->getBufferForFile("aa");
   ASSERT_TRUE(ABuf);
   EXPECT_EQ("aaaa", (*ABuf)->getBuffer());

   // status()
   auto BStat = bfs->getStatus("bb");
   ASSERT_TRUE(BStat);
   EXPECT_EQ("bb", BStat->getName());

   auto AStat = cfs->getStatus("aa");
   ASSERT_TRUE(AStat);
   EXPECT_EQ("aa", AStat->getName()); // unresolved name

   // getRealPath()
   SmallString<128> BPath;
   ASSERT_FALSE(bfs->getRealPath("bb", BPath));
   EXPECT_EQ(BB.path, BPath);

   SmallString<128> APath;
   ASSERT_FALSE(cfs->getRealPath("aa", APath));
   EXPECT_EQ(AA.path, APath); // Reports resolved name.

   // dir_begin
   std::error_code errorCode;
   auto biter = bfs->dirBegin(".", errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_NE(biter, vfs::DirectoryIterator());
   EXPECT_EQ((BDir.path + "/./bb").getStr(), biter->path());
   biter.increment(errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_EQ(biter, vfs::DirectoryIterator());

   auto citer = cfs->dirBegin(".", errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_NE(citer, vfs::DirectoryIterator());
   EXPECT_EQ((ADir.path + "/./aa").getStr(), citer->path()); // Partly resolved name!
   citer.increment(errorCode); // Because likely to read through this path.
   ASSERT_FALSE(errorCode);
   ASSERT_EQ(citer, vfs::DirectoryIterator());
}

TEST(VirtualFileSystemTest, testBrokenSymlinkRealFSIteration)
{
   ScopedDir TestDirectory("virtual-file-system-test", /*Unique*/ true);
   IntrusiveRefCountPtr<vfs::FileSystem> FS = vfs::get_real_file_system();

   ScopedLink _a("no_such_file", TestDirectory + "/a");
   ScopedDir _b(TestDirectory + "/b");
   ScopedLink _c("no_such_file", TestDirectory + "/c");

   // Should get no iteration error, but a stat error for the broken symlinks.
   std::map<std::string, std::error_code> StatResults;
   std::error_code errorCode;
   for (vfs::DirectoryIterator I = FS->dirBegin(Twine(TestDirectory), errorCode), E;
        I != E; I.increment(errorCode)) {
      EXPECT_FALSE(errorCode);
      StatResults[polar::fs::path::filename(I->path())] =
            FS->getStatus(I->path()).getError();
   }
   EXPECT_THAT(
            StatResults,
            ElementsAre(
               Pair("a", polar::utils::make_error_code(ErrorCode::no_such_file_or_directory)),
               Pair("b", std::error_code()),
               Pair("c",
                    polar::utils::make_error_code(ErrorCode::no_such_file_or_directory))));
}
#endif

TEST(VirtualFileSystemTest, testBasicRealFSRecursiveIteration)
{
   ScopedDir TestDirectory("virtual-file-system-test", /*Unique*/ true);
   IntrusiveRefCountPtr<vfs::FileSystem> FS = vfs::get_real_file_system();

   std::error_code errorCode;
   auto I = vfs::RecursiveDirectoryIterator(*FS, Twine(TestDirectory), errorCode);
   ASSERT_FALSE(errorCode);
   EXPECT_EQ(vfs::RecursiveDirectoryIterator(), I); // empty directory is empty

   ScopedDir _a(TestDirectory + "/a");
   ScopedDir _ab(TestDirectory + "/a/b");
   ScopedDir _c(TestDirectory + "/c");
   ScopedDir _cd(TestDirectory + "/c/d");

   I = vfs::RecursiveDirectoryIterator(*FS, Twine(TestDirectory), errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_NE(vfs::RecursiveDirectoryIterator(), I);

   std::vector<std::string> Contents;
   for (auto E = vfs::RecursiveDirectoryIterator(); !errorCode && I != E;
        I.increment(errorCode)) {
      Contents.push_back(I->path());
   }

   // Check contents, which may be in any order
   EXPECT_EQ(4U, Contents.size());
   int Counts[4] = {0, 0, 0, 0};
   for (const std::string &Name : Contents) {
      ASSERT_FALSE(Name.empty());
      int Index = Name[Name.size() - 1] - 'a';
      ASSERT_TRUE(Index >= 0 && Index < 4);
      Counts[Index]++;
   }
   EXPECT_EQ(1, Counts[0]); // a
   EXPECT_EQ(1, Counts[1]); // b
   EXPECT_EQ(1, Counts[2]); // c
   EXPECT_EQ(1, Counts[3]); // d
}

TEST(VirtualFileSystemTest, testBasicRealFSRecursiveIterationNoPush)
{
   ScopedDir TestDirectory("virtual-file-system-test", /*Unique*/ true);

   ScopedDir _a(TestDirectory + "/a");
   ScopedDir _ab(TestDirectory + "/a/b");
   ScopedDir _c(TestDirectory + "/c");
   ScopedDir _cd(TestDirectory + "/c/d");
   ScopedDir _e(TestDirectory + "/e");
   ScopedDir _ef(TestDirectory + "/e/f");
   ScopedDir _g(TestDirectory + "/g");

   IntrusiveRefCountPtr<vfs::FileSystem> FS = vfs::get_real_file_system();

   // Test that calling no_push on entries without subdirectories has no effect.
   {
      std::error_code errorCode;
      auto I = vfs::RecursiveDirectoryIterator(*FS, Twine(TestDirectory), errorCode);
      ASSERT_FALSE(errorCode);

      std::vector<std::string> Contents;
      for (auto E = vfs::RecursiveDirectoryIterator(); !errorCode && I != E;
           I.increment(errorCode)) {
         Contents.push_back(I->path());
         char last = I->path().back();
         switch (last) {
         case 'b':
         case 'd':
         case 'f':
         case 'g':
            I.noPush();
            break;
         default:
            break;
         }
      }
      EXPECT_EQ(7U, Contents.size());
   }

   // Test that calling no_push skips subdirectories.
   {
      std::error_code errorCode;
      auto I = vfs::RecursiveDirectoryIterator(*FS, Twine(TestDirectory), errorCode);
      ASSERT_FALSE(errorCode);

      std::vector<std::string> Contents;
      for (auto E = vfs::RecursiveDirectoryIterator(); !errorCode && I != E;
           I.increment(errorCode)) {
         Contents.push_back(I->path());
         char last = I->path().back();
         switch (last) {
         case 'a':
         case 'c':
         case 'e':
            I.noPush();
            break;
         default:
            break;
         }
      }

      // Check contents, which may be in any order
      EXPECT_EQ(4U, Contents.size());
      int Counts[7] = {0, 0, 0, 0, 0, 0, 0};
      for (const std::string &Name : Contents) {
         ASSERT_FALSE(Name.empty());
         int Index = Name[Name.size() - 1] - 'a';
         ASSERT_TRUE(Index >= 0 && Index < 7);
         Counts[Index]++;
      }
      EXPECT_EQ(1, Counts[0]); // a
      EXPECT_EQ(0, Counts[1]); // b
      EXPECT_EQ(1, Counts[2]); // c
      EXPECT_EQ(0, Counts[3]); // d
      EXPECT_EQ(1, Counts[4]); // e
      EXPECT_EQ(0, Counts[5]); // f
      EXPECT_EQ(1, Counts[6]); // g
   }
}

#ifdef POLAR_ON_UNIX
TEST(VirtualFileSystemTest, testBrokenSymlinkRealFSRecursiveIteration)
{
   ScopedDir TestDirectory("virtual-file-system-test", /*Unique*/ true);
   IntrusiveRefCountPtr<vfs::FileSystem> FS = vfs::get_real_file_system();

   ScopedLink _a("no_such_file", TestDirectory + "/a");
   ScopedDir _b(TestDirectory + "/b");
   ScopedLink _ba("no_such_file", TestDirectory + "/b/a");
   ScopedDir _bb(TestDirectory + "/b/b");
   ScopedLink _bc("no_such_file", TestDirectory + "/b/c");
   ScopedLink _c("no_such_file", TestDirectory + "/c");
   ScopedDir _d(TestDirectory + "/d");
   ScopedDir _dd(TestDirectory + "/d/d");
   ScopedDir _ddd(TestDirectory + "/d/d/d");
   ScopedLink _e("no_such_file", TestDirectory + "/e");

   std::vector<std::string> VisitedBrokenSymlinks;
   std::vector<std::string> VisitedNonBrokenSymlinks;
   std::error_code errorCode;
   for (vfs::RecursiveDirectoryIterator I(*FS, Twine(TestDirectory), errorCode), E;
        I != E; I.increment(errorCode)) {
      EXPECT_FALSE(errorCode);
      (FS->getStatus(I->path()) ? VisitedNonBrokenSymlinks : VisitedBrokenSymlinks)
            .push_back(I->path());
   }

   // Check visited file names.
   EXPECT_THAT(VisitedBrokenSymlinks,
               UnorderedElementsAre(StringRef(_a), StringRef(_ba),
                                    StringRef(_bc), StringRef(_c),
                                    StringRef(_e)));
   EXPECT_THAT(VisitedNonBrokenSymlinks,
               UnorderedElementsAre(StringRef(_b), StringRef(_bb), StringRef(_d),
                                    StringRef(_dd), StringRef(_ddd)));
}
#endif

template <typename DirIter>
static void checkContents(DirIter I, ArrayRef<StringRef> ExpectedOut)
{
   std::error_code errorCode;
   SmallVector<StringRef, 4> Expected(ExpectedOut.begin(), ExpectedOut.end());
   SmallVector<std::string, 4> InputToCheck;

   // Do not rely on iteration order to check for contents, sort both
   // content vectors before comparison.
   for (DirIter E; !errorCode && I != E; I.increment(errorCode))
      InputToCheck.push_back(I->path());

   polar::basic::sort(InputToCheck);
   polar::basic::sort(Expected);
   EXPECT_EQ(InputToCheck.size(), Expected.size());

   unsigned LastElt = std::min(InputToCheck.size(), Expected.size());
   for (unsigned Idx = 0; Idx != LastElt; ++Idx)
      EXPECT_EQ(StringRef(InputToCheck[Idx]), Expected[Idx]);
}

TEST(VirtualFileSystemTest, testOverlayIteration)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());
   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Upper);

   std::error_code errorCode;
   checkContents(O->dirBegin("/", errorCode), ArrayRef<StringRef>());

   Lower->addRegularFile("/file1");
   checkContents(O->dirBegin("/", errorCode), ArrayRef<StringRef>("/file1"));

   Upper->addRegularFile("/file2");
   checkContents(O->dirBegin("/", errorCode), {"/file2", "/file1"});

   Lower->addDirectory("/dir1");
   Lower->addRegularFile("/dir1/foo");
   Upper->addDirectory("/dir2");
   Upper->addRegularFile("/dir2/foo");
   checkContents(O->dirBegin("/dir2", errorCode), ArrayRef<StringRef>("/dir2/foo"));
   checkContents(O->dirBegin("/", errorCode), {"/dir2", "/file2", "/dir1", "/file1"});
}

TEST(VirtualFileSystemTest, testOverlayRecursiveIteration)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Middle(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());
   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Middle);
   O->pushOverlay(Upper);

   std::error_code errorCode;
   checkContents(vfs::RecursiveDirectoryIterator(*O, "/", errorCode),
                 ArrayRef<StringRef>());

   Lower->addRegularFile("/file1");
   checkContents(vfs::RecursiveDirectoryIterator(*O, "/", errorCode),
                 ArrayRef<StringRef>("/file1"));

   Upper->addDirectory("/dir");
   Upper->addRegularFile("/dir/file2");
   checkContents(vfs::RecursiveDirectoryIterator(*O, "/", errorCode),
   {"/dir", "/dir/file2", "/file1"});

   Lower->addDirectory("/dir1");
   Lower->addRegularFile("/dir1/foo");
   Lower->addDirectory("/dir1/a");
   Lower->addRegularFile("/dir1/a/b");
   Middle->addDirectory("/a");
   Middle->addDirectory("/a/b");
   Middle->addDirectory("/a/b/c");
   Middle->addRegularFile("/a/b/c/d");
   Middle->addRegularFile("/hiddenByUp");
   Upper->addDirectory("/dir2");
   Upper->addRegularFile("/dir2/foo");
   Upper->addRegularFile("/hiddenByUp");
   checkContents(vfs::RecursiveDirectoryIterator(*O, "/dir2", errorCode),
                 ArrayRef<StringRef>("/dir2/foo"));
   checkContents(vfs::RecursiveDirectoryIterator(*O, "/", errorCode),
   {"/dir", "/dir/file2", "/dir2", "/dir2/foo", "/hiddenByUp",
    "/a", "/a/b", "/a/b/c", "/a/b/c/d", "/dir1", "/dir1/a",
    "/dir1/a/b", "/dir1/foo", "/file1"});
}

TEST(VirtualFileSystemTest, testThreeLevelIteration)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Middle(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());
   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Middle);
   O->pushOverlay(Upper);

   std::error_code errorCode;
   checkContents(O->dirBegin("/", errorCode), ArrayRef<StringRef>());

   Middle->addRegularFile("/file2");
   checkContents(O->dirBegin("/", errorCode), ArrayRef<StringRef>("/file2"));

   Lower->addRegularFile("/file1");
   Upper->addRegularFile("/file3");
   checkContents(O->dirBegin("/", errorCode), {"/file3", "/file2", "/file1"});
}

TEST(VirtualFileSystemTest, testHiddenInIteration)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Middle(new DummyFileSystem());
   IntrusiveRefCountPtr<DummyFileSystem> Upper(new DummyFileSystem());
   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(Middle);
   O->pushOverlay(Upper);

   std::error_code errorCode;
   Lower->addRegularFile("/onlyInLow");
   Lower->addDirectory("/hiddenByMid");
   Lower->addDirectory("/hiddenByUp");
   Middle->addRegularFile("/onlyInMid");
   Middle->addRegularFile("/hiddenByMid");
   Middle->addDirectory("/hiddenByUp");
   Upper->addRegularFile("/onlyInUp");
   Upper->addRegularFile("/hiddenByUp");
   checkContents(
            O->dirBegin("/", errorCode),
   {"/hiddenByUp", "/onlyInUp", "/hiddenByMid", "/onlyInMid", "/onlyInLow"});

   // Make sure we get the top-most entry
   {
      std::error_code errorCode;
      vfs::DirectoryIterator I = O->dirBegin("/", errorCode), E;
      for (; !errorCode && I != E; I.increment(errorCode))
         if (I->path() == "/hiddenByUp")
            break;
      ASSERT_NE(E, I);
      EXPECT_EQ(polar::fs::FileType::regular_file, I->type());
   }
   {
      std::error_code errorCode;
      vfs::DirectoryIterator I = O->dirBegin("/", errorCode), E;
      for (; !errorCode && I != E; I.increment(errorCode))
         if (I->path() == "/hiddenByMid")
            break;
      ASSERT_NE(E, I);
      EXPECT_EQ(polar::fs::FileType::regular_file, I->type());
   }
}


TEST(VirtualFileSystemTest, testBasicProxy) {
   IntrusiveRefCountPtr<vfs::InMemoryFileSystem> Base(
            new vfs::InMemoryFileSystem());
   vfs::ProxyFileSystem PFS(Base);

   Base->addFile("/a", 0, MemoryBuffer::getMemBuffer("test"));

   auto Stat = PFS.getStatus("/a");
   ASSERT_FALSE(Stat.getError());

   auto File = PFS.openFileForRead("/a");
   ASSERT_FALSE(File.getError());
   EXPECT_EQ("test", (*(*File)->getBuffer("ignored"))->getBuffer());

   std::error_code errorCode;
   vfs::DirectoryIterator I = PFS.dirBegin("/", errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_EQ("/a", I->path());
   I.increment(errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_EQ(vfs::DirectoryIterator(), I);

   ASSERT_FALSE(PFS.setCurrentWorkingDirectory("/"));

   auto PWD = PFS.getCurrentWorkingDirectory();
   ASSERT_FALSE(PWD.getError());
   ASSERT_EQ("/", *PWD);

   SmallString<16> path;
   ASSERT_FALSE(PFS.getRealPath("a", path));
   ASSERT_EQ("/a", path);

   bool Local = true;
   ASSERT_FALSE(PFS.isLocal("/a", Local));
   EXPECT_FALSE(Local);
}

class InMemoryFileSystemTest : public ::testing::Test
{
protected:
   polar::vfs::InMemoryFileSystem FS;
   polar::vfs::InMemoryFileSystem NormalizedFS;

   InMemoryFileSystemTest()
      : FS(/*UseNormalizedPaths=*/false),
        NormalizedFS(/*UseNormalizedPaths=*/true) {}
};

MATCHER_P2(IsHardLinkTo, FS, Target, "") {
   StringRef From = arg;
   StringRef To = Target;
   auto OpenedFrom = FS->openFileForRead(From);
   auto OpenedTo = FS->openFileForRead(To);
   return !OpenedFrom.getError() && !OpenedTo.getError() &&
         (*OpenedFrom)->getStatus()->getUniqueId() ==
         (*OpenedTo)->getStatus()->getUniqueId();
}

TEST_F(InMemoryFileSystemTest, testIsEmpty)
{
   auto Stat = FS.getStatus("/a");
   ASSERT_EQ(Stat.getError(), ErrorCode::no_such_file_or_directory) << FS.toString();
   Stat = FS.getStatus("/");
   ASSERT_EQ(Stat.getError(), ErrorCode::no_such_file_or_directory) << FS.toString();
}

TEST_F(InMemoryFileSystemTest, testWindowsPath)
{
   FS.addFile("c:/windows/system128/foo.cpp", 0, MemoryBuffer::getMemBuffer(""));
   auto Stat = FS.getStatus("c:");
#if !defined(_WIN32)
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << FS.toString();
#endif
   Stat = FS.getStatus("c:/windows/system128/foo.cpp");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << FS.toString();
   FS.addFile("d:/windows/foo.cpp", 0, MemoryBuffer::getMemBuffer(""));
   Stat = FS.getStatus("d:/windows/foo.cpp");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << FS.toString();
}

TEST_F(InMemoryFileSystemTest, testOverlayFile)
{
   FS.addFile("/a", 0, MemoryBuffer::getMemBuffer("a"));
   NormalizedFS.addFile("/a", 0, MemoryBuffer::getMemBuffer("a"));
   auto Stat = FS.getStatus("/");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << FS.toString();
   Stat = FS.getStatus("/.");
   ASSERT_FALSE(Stat);
   Stat = NormalizedFS.getStatus("/.");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << FS.toString();
   Stat = FS.getStatus("/a");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_EQ("/a", Stat->getName());
}

TEST_F(InMemoryFileSystemTest, testOverlayFileNoOwn)
{
   auto Buf = MemoryBuffer::getMemBuffer("a");
   FS.addFileNoOwn("/a", 0, Buf.get());
   auto Stat = FS.getStatus("/a");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_EQ("/a", Stat->getName());
}

TEST_F(InMemoryFileSystemTest, testOpenFileForRead)
{
   FS.addFile("/a", 0, MemoryBuffer::getMemBuffer("a"));
   FS.addFile("././c", 0, MemoryBuffer::getMemBuffer("c"));
   FS.addFile("./d/../d", 0, MemoryBuffer::getMemBuffer("d"));
   NormalizedFS.addFile("/a", 0, MemoryBuffer::getMemBuffer("a"));
   NormalizedFS.addFile("././c", 0, MemoryBuffer::getMemBuffer("c"));
   NormalizedFS.addFile("./d/../d", 0, MemoryBuffer::getMemBuffer("d"));
   auto File = FS.openFileForRead("/a");
   ASSERT_EQ("a", (*(*File)->getBuffer("ignored"))->getBuffer());
   File = FS.openFileForRead("/a"); // Open again.
   ASSERT_EQ("a", (*(*File)->getBuffer("ignored"))->getBuffer());
   File = NormalizedFS.openFileForRead("/././a"); // Open again.
   ASSERT_EQ("a", (*(*File)->getBuffer("ignored"))->getBuffer());
   File = FS.openFileForRead("/");
   ASSERT_EQ(File.getError(), ErrorCode::invalid_argument) << FS.toString();
   File = FS.openFileForRead("/b");
   ASSERT_EQ(File.getError(), ErrorCode::no_such_file_or_directory) << FS.toString();
   File = FS.openFileForRead("./c");
   ASSERT_FALSE(File);
   File = FS.openFileForRead("e/../d");
   ASSERT_FALSE(File);
   File = NormalizedFS.openFileForRead("./c");
   ASSERT_EQ("c", (*(*File)->getBuffer("ignored"))->getBuffer());
   File = NormalizedFS.openFileForRead("e/../d");
   ASSERT_EQ("d", (*(*File)->getBuffer("ignored"))->getBuffer());
}

TEST_F(InMemoryFileSystemTest, testDuplicatedFile)
{
   ASSERT_TRUE(FS.addFile("/a", 0, MemoryBuffer::getMemBuffer("a")));
   ASSERT_FALSE(FS.addFile("/a/b", 0, MemoryBuffer::getMemBuffer("a")));
   ASSERT_TRUE(FS.addFile("/a", 0, MemoryBuffer::getMemBuffer("a")));
   ASSERT_FALSE(FS.addFile("/a", 0, MemoryBuffer::getMemBuffer("b")));
}

TEST_F(InMemoryFileSystemTest, testDirectoryIteration)
{
   FS.addFile("/a", 0, MemoryBuffer::getMemBuffer(""));
   FS.addFile("/b/c", 0, MemoryBuffer::getMemBuffer(""));

   std::error_code errorCode;
   vfs::DirectoryIterator I = FS.dirBegin("/", errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_EQ("/a", I->path());
   I.increment(errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_EQ("/b", I->path());
   I.increment(errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_EQ(vfs::DirectoryIterator(), I);

   I = FS.dirBegin("/b", errorCode);
   ASSERT_FALSE(errorCode);
   // When on Windows, we end up with "/b\\c" as the name.  Convert to Posix
   // path for the sake of the comparison.
   ASSERT_EQ("/b/c", getPosixPath(I->path()));
   I.increment(errorCode);
   ASSERT_FALSE(errorCode);
   ASSERT_EQ(vfs::DirectoryIterator(), I);
}

TEST_F(InMemoryFileSystemTest, testWorkingDirectory)
{
   FS.setCurrentWorkingDirectory("/b");
   FS.addFile("c", 0, MemoryBuffer::getMemBuffer(""));

   auto Stat = FS.getStatus("/b/c");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_EQ("/b/c", Stat->getName());
   ASSERT_EQ("/b", *FS.getCurrentWorkingDirectory());

   Stat = FS.getStatus("c");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();

   NormalizedFS.setCurrentWorkingDirectory("/b/c");
   NormalizedFS.setCurrentWorkingDirectory(".");
   ASSERT_EQ("/b/c",
             getPosixPath(NormalizedFS.getCurrentWorkingDirectory().get()));
   NormalizedFS.setCurrentWorkingDirectory("..");
   ASSERT_EQ("/b",
             getPosixPath(NormalizedFS.getCurrentWorkingDirectory().get()));
}

TEST_F(InMemoryFileSystemTest, testIsLocal)
{
   FS.setCurrentWorkingDirectory("/b");
   FS.addFile("c", 0, MemoryBuffer::getMemBuffer(""));

   std::error_code errorCode;
   bool IsLocal = true;
   errorCode = FS.isLocal("c", IsLocal);
   ASSERT_FALSE(errorCode);
   ASSERT_FALSE(IsLocal);
}

#if !defined(_WIN32)
TEST_F(InMemoryFileSystemTest, testGetRealPath)
{
   SmallString<16> path;
   EXPECT_EQ(FS.getRealPath("b", path), ErrorCode::operation_not_permitted);

   auto GetRealPath = [this](StringRef P) {
      SmallString<16> Output;
      auto errorCode = FS.getRealPath(P, Output);
      EXPECT_FALSE(errorCode);
      return Output.getStr().getStr();
   };

   FS.setCurrentWorkingDirectory("a");
   EXPECT_EQ(GetRealPath("b"), "a/b");
   EXPECT_EQ(GetRealPath("../b"), "b");
   EXPECT_EQ(GetRealPath("b/./c"), "a/b/c");

   FS.setCurrentWorkingDirectory("/a");
   EXPECT_EQ(GetRealPath("b"), "/a/b");
   EXPECT_EQ(GetRealPath("../b"), "/b");
   EXPECT_EQ(GetRealPath("b/./c"), "/a/b/c");
}
#endif // _WIN32

TEST_F(InMemoryFileSystemTest, testAddFileWithUser)
{
   FS.addFile("/a/b/c", 0, MemoryBuffer::getMemBuffer("abc"), 0xFEEDFACE);
   auto Stat = FS.getStatus("/a");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   ASSERT_EQ(0xFEEDFACE, Stat->getUser());
   Stat = FS.getStatus("/a/b");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   ASSERT_EQ(0xFEEDFACE, Stat->getUser());
   Stat = FS.getStatus("/a/b/c");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isRegularFile());
   ASSERT_EQ(polar::fs::Permission::all_all, Stat->getPermissions());
   ASSERT_EQ(0xFEEDFACE, Stat->getUser());
}

TEST_F(InMemoryFileSystemTest, testAddFileWithGroup)
{
   FS.addFile("/a/b/c", 0, MemoryBuffer::getMemBuffer("abc"), std::nullopt, 0xDABBAD00);
   auto Stat = FS.getStatus("/a");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   ASSERT_EQ(0xDABBAD00, Stat->getGroup());
   Stat = FS.getStatus("/a/b");
   ASSERT_TRUE(Stat->isDirectory());
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_EQ(0xDABBAD00, Stat->getGroup());
   Stat = FS.getStatus("/a/b/c");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isRegularFile());
   ASSERT_EQ(polar::fs::Permission::all_all, Stat->getPermissions());
   ASSERT_EQ(0xDABBAD00, Stat->getGroup());
}

TEST_F(InMemoryFileSystemTest, testAddFileWithFileType)
{
   FS.addFile("/a/b/c", 0, MemoryBuffer::getMemBuffer("abc"), std::nullopt, std::nullopt,
              polar::fs::FileType::socket_file);
   auto Stat = FS.getStatus("/a");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   Stat = FS.getStatus("/a/b");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   Stat = FS.getStatus("/a/b/c");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_EQ(polar::fs::FileType::socket_file, Stat->getType());
   ASSERT_EQ(polar::fs::Permission::all_all, Stat->getPermissions());
}

TEST_F(InMemoryFileSystemTest, testAddFileWithPerms)
{
   FS.addFile("/a/b/c", 0, MemoryBuffer::getMemBuffer("abc"), std::nullopt, std::nullopt, std::nullopt,
              polar::fs::Permission::owner_read | polar::fs::Permission::owner_write);
   auto Stat = FS.getStatus("/a");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   ASSERT_EQ(polar::fs::Permission::owner_read | polar::fs::Permission::owner_write |
             polar::fs::Permission::owner_exe,
             Stat->getPermissions());
   Stat = FS.getStatus("/a/b");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   ASSERT_EQ(polar::fs::Permission::owner_read | polar::fs::Permission::owner_write |
             polar::fs::Permission::owner_exe,
             Stat->getPermissions());
   Stat = FS.getStatus("/a/b/c");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isRegularFile());
   ASSERT_EQ(polar::fs::Permission::owner_read | polar::fs::Permission::owner_write,
             Stat->getPermissions());
}

TEST_F(InMemoryFileSystemTest, testAddDirectoryThenAddChild)
{
   FS.addFile("/a", 0, MemoryBuffer::getMemBuffer(""), /*User=*/std::nullopt,
              /*Group=*/std::nullopt, polar::fs::FileType::directory_file);
   FS.addFile("/a/b", 0, MemoryBuffer::getMemBuffer("abc"), /*User=*/std::nullopt,
              /*Group=*/std::nullopt, polar::fs::FileType::regular_file);
   auto Stat = FS.getStatus("/a");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isDirectory());
   Stat = FS.getStatus("/a/b");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n" << FS.toString();
   ASSERT_TRUE(Stat->isRegularFile());
}

// Test that the name returned by status() is in the same form as the path that
// was requested (to match the behavior of RealFileSystem).
TEST_F(InMemoryFileSystemTest, testStatusName)
{
   NormalizedFS.addFile("/a/b/c", 0, MemoryBuffer::getMemBuffer("abc"),
                        /*User=*/std::nullopt,
                        /*Group=*/std::nullopt, polar::fs::FileType::regular_file);
   NormalizedFS.setCurrentWorkingDirectory("/a/b");

   // Access using InMemoryFileSystem::status.
   auto Stat = NormalizedFS.getStatus("../b/c");
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n"
                                 << NormalizedFS.toString();
   ASSERT_TRUE(Stat->isRegularFile());
   ASSERT_EQ("../b/c", Stat->getName());

   // Access using InMemoryFileAdaptor::status.
   auto File = NormalizedFS.openFileForRead("../b/c");
   ASSERT_FALSE(File.getError()) << File.getError() << "\n"
                                 << NormalizedFS.toString();
   Stat = (*File)->getStatus();
   ASSERT_FALSE(Stat.getError()) << Stat.getError() << "\n"
                                 << NormalizedFS.toString();
   ASSERT_TRUE(Stat->isRegularFile());
   ASSERT_EQ("../b/c", Stat->getName());

   // Access using a directory iterator.
   std::error_code errorCode;
   polar::vfs::DirectoryIterator It = NormalizedFS.dirBegin("../b", errorCode);
   // When on Windows, we end up with "../b\\c" as the name.  Convert to Posix
   // path for the sake of the comparison.
   ASSERT_EQ("../b/c", getPosixPath(It->path()));
}

TEST_F(InMemoryFileSystemTest, testAddHardLinkToFile)
{
   StringRef FromLink = "/path/to/FROM/link";
   StringRef Target = "/path/to/TO/file";
   FS.addFile(Target, 0, MemoryBuffer::getMemBuffer("content of target"));
   EXPECT_TRUE(FS.addHardLink(FromLink, Target));
   EXPECT_THAT(FromLink, IsHardLinkTo(&FS, Target));
   EXPECT_TRUE(FS.getStatus(FromLink)->getSize() == FS.getStatus(Target)->getSize());
   EXPECT_TRUE(FS.getBufferForFile(FromLink)->get()->getBuffer() ==
               FS.getBufferForFile(Target)->get()->getBuffer());
}

TEST_F(InMemoryFileSystemTest, testAddHardLinkInChainPattern)
{
   StringRef Link0 = "/path/to/0/link";
   StringRef Link1 = "/path/to/1/link";
   StringRef Link2 = "/path/to/2/link";
   StringRef Target = "/path/to/target";
   FS.addFile(Target, 0, MemoryBuffer::getMemBuffer("content of target file"));
   EXPECT_TRUE(FS.addHardLink(Link2, Target));
   EXPECT_TRUE(FS.addHardLink(Link1, Link2));
   EXPECT_TRUE(FS.addHardLink(Link0, Link1));
   EXPECT_THAT(Link0, IsHardLinkTo(&FS, Target));
   EXPECT_THAT(Link1, IsHardLinkTo(&FS, Target));
   EXPECT_THAT(Link2, IsHardLinkTo(&FS, Target));
}

TEST_F(InMemoryFileSystemTest, testAddHardLinkToAFileThatWasNotAddedBefore)
{
   EXPECT_FALSE(FS.addHardLink("/path/to/link", "/path/to/target"));
}

TEST_F(InMemoryFileSystemTest, AddHardLinkFromAFileThatWasAddedBefore)
{
   StringRef Link = "/path/to/link";
   StringRef Target = "/path/to/target";
   FS.addFile(Target, 0, MemoryBuffer::getMemBuffer("content of target"));
   FS.addFile(Link, 0, MemoryBuffer::getMemBuffer("content of link"));
   EXPECT_FALSE(FS.addHardLink(Link, Target));
}

TEST_F(InMemoryFileSystemTest, testAddSameHardLinkMoreThanOnce)
{
   StringRef Link = "/path/to/link";
   StringRef Target = "/path/to/target";
   FS.addFile(Target, 0, MemoryBuffer::getMemBuffer("content of target"));
   EXPECT_TRUE(FS.addHardLink(Link, Target));
   EXPECT_FALSE(FS.addHardLink(Link, Target));
}

TEST_F(InMemoryFileSystemTest, testAddFileInPlaceOfAHardLinkWithSameContent)
{
   StringRef Link = "/path/to/link";
   StringRef Target = "/path/to/target";
   StringRef Content = "content of target";
   EXPECT_TRUE(FS.addFile(Target, 0, MemoryBuffer::getMemBuffer(Content)));
   EXPECT_TRUE(FS.addHardLink(Link, Target));
   EXPECT_TRUE(FS.addFile(Link, 0, MemoryBuffer::getMemBuffer(Content)));
}

TEST_F(InMemoryFileSystemTest, testAddFileInPlaceOfAHardLinkWithDifferentContent)
{
   StringRef Link = "/path/to/link";
   StringRef Target = "/path/to/target";
   StringRef Content = "content of target";
   StringRef LinkContent = "different content of link";
   EXPECT_TRUE(FS.addFile(Target, 0, MemoryBuffer::getMemBuffer(Content)));
   EXPECT_TRUE(FS.addHardLink(Link, Target));
   EXPECT_FALSE(FS.addFile(Link, 0, MemoryBuffer::getMemBuffer(LinkContent)));
}

TEST_F(InMemoryFileSystemTest, testAddHardLinkToADirectory)
{
   StringRef Dir = "path/to/dummy/dir";
   StringRef Link = "/path/to/link";
   StringRef File = "path/to/dummy/dir/target";
   StringRef Content = "content of target";
   EXPECT_TRUE(FS.addFile(File, 0, MemoryBuffer::getMemBuffer(Content)));
   EXPECT_FALSE(FS.addHardLink(Link, Dir));
}

TEST_F(InMemoryFileSystemTest, testAddHardLinkFromADirectory)
{
   StringRef Dir = "path/to/dummy/dir";
   StringRef Target = "path/to/dummy/dir/target";
   StringRef Content = "content of target";
   EXPECT_TRUE(FS.addFile(Target, 0, MemoryBuffer::getMemBuffer(Content)));
   EXPECT_FALSE(FS.addHardLink(Dir, Target));
}

TEST_F(InMemoryFileSystemTest, testAddHardLinkUnderAFile)
{
   StringRef CommonContent = "content string";
   FS.addFile("/a/b", 0, MemoryBuffer::getMemBuffer(CommonContent));
   FS.addFile("/c/d", 0, MemoryBuffer::getMemBuffer(CommonContent));
   EXPECT_FALSE(FS.addHardLink("/c/d/e", "/a/b"));
}

TEST_F(InMemoryFileSystemTest, testRecursiveIterationWithHardLink)
{
   std::error_code errorCode;
   FS.addFile("/a/b", 0, MemoryBuffer::getMemBuffer("content string"));
   EXPECT_TRUE(FS.addHardLink("/c/d", "/a/b"));
   auto I = vfs::RecursiveDirectoryIterator(FS, "/", errorCode);
   ASSERT_FALSE(errorCode);
   std::vector<std::string> Nodes;
   for (auto E = vfs::RecursiveDirectoryIterator(); !errorCode && I != E;
        I.increment(errorCode)) {
      Nodes.push_back(getPosixPath(I->path()));
   }
   EXPECT_THAT(Nodes, testing::UnorderedElementsAre("/a", "/a/b", "/c", "/c/d"));
}

// NOTE: in the tests below, we use '//root/' as our root directory, since it is
// a legal *absolute* path on Windows as well as *nix.
class VFSFromYAMLTest : public ::testing::Test {
public:
   int NumDiagnostics;

   void SetUp() override { NumDiagnostics = 0; }

   static void CountingDiagHandler(const SMDiagnostic &, void *Context) {
      VFSFromYAMLTest *Test = static_cast<VFSFromYAMLTest *>(Context);
      ++Test->NumDiagnostics;
   }

   IntrusiveRefCountPtr<vfs::FileSystem>
   getFromYAMLRawString(StringRef Content,
                        IntrusiveRefCountPtr<vfs::FileSystem> ExternalFS) {
      std::unique_ptr<MemoryBuffer> Buffer = MemoryBuffer::getMemBuffer(Content);
      return vfs::get_vfs_from_yaml(std::move(Buffer), CountingDiagHandler, "", this,
                                    ExternalFS);
   }

   IntrusiveRefCountPtr<vfs::FileSystem> getFromYAMLString(
         StringRef Content,
         IntrusiveRefCountPtr<vfs::FileSystem> ExternalFS = new DummyFileSystem()) {
      std::string VersionPlusContent("{\n  'version':0,\n");
      VersionPlusContent += Content.slice(Content.find('{') + 1, StringRef::npos);
      return getFromYAMLRawString(VersionPlusContent, ExternalFS);
   }

   // This is intended as a "XFAIL" for windows hosts.
   bool supportsSameDirMultipleYAMLEntries() {
      Triple Host(Triple::normalize(polar::sys::get_process_triple()));
      return !Host.isOSWindows();
   }
};

TEST_F(VFSFromYAMLTest, testBasicVFSFromYAML)
{
   IntrusiveRefCountPtr<vfs::FileSystem> FS;
   FS = getFromYAMLString("");
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString("[]");
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString("'string'");
   EXPECT_EQ(nullptr, FS.get());
   EXPECT_EQ(3, NumDiagnostics);
}

TEST_F(VFSFromYAMLTest, testMappedFiles)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("//root/foo/bar/a");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'file1',\n"
            "                  'external-contents': '//root/foo/bar/a'\n"
            "                },\n"
            "                {\n"
            "                  'type': 'file',\n"
            "                  'name': 'file2',\n"
            "                  'external-contents': '//root/foo/b'\n"
            "                }\n"
            "              ]\n"
            "}\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(FS);

   // file
   OptionalError<vfs::Status> S = O->getStatus("//root/file1");
   ASSERT_FALSE(S.getError());
   EXPECT_EQ("//root/foo/bar/a", S->getName());
   EXPECT_TRUE(S->isVFSMapped);

   OptionalError<vfs::Status> SLower = O->getStatus("//root/foo/bar/a");
   EXPECT_EQ("//root/foo/bar/a", SLower->getName());
   EXPECT_TRUE(S->equivalent(*SLower));
   EXPECT_FALSE(SLower->isVFSMapped);

   // file after opening
   auto OpenedF = O->openFileForRead("//root/file1");
   ASSERT_FALSE(OpenedF.getError());
   auto OpenedS = (*OpenedF)->getStatus();
   ASSERT_FALSE(OpenedS.getError());
   EXPECT_EQ("//root/foo/bar/a", OpenedS->getName());
   EXPECT_TRUE(OpenedS->isVFSMapped);

   // directory
   S = O->getStatus("//root/");
   ASSERT_FALSE(S.getError());
   EXPECT_TRUE(S->isDirectory());
   EXPECT_TRUE(S->equivalent(*O->getStatus("//root/"))); // non-volatile UniqueId

   // broken mapping
   EXPECT_EQ(O->getStatus("//root/file2").getError(),
             ErrorCode::no_such_file_or_directory);
   EXPECT_EQ(0, NumDiagnostics);
}

TEST_F(VFSFromYAMLTest, testCaseInsensitive)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("//root/foo/bar/a");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'case-sensitive': 'false',\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'XX',\n"
            "                  'external-contents': '//root/foo/bar/a'\n"
            "                }\n"
            "              ]\n"
            "}]}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(FS);

   OptionalError<vfs::Status> S = O->getStatus("//root/XX");
   ASSERT_FALSE(S.getError());

   OptionalError<vfs::Status> SS = O->getStatus("//root/xx");
   ASSERT_FALSE(SS.getError());
   EXPECT_TRUE(S->equivalent(*SS));
   SS = O->getStatus("//root/xX");
   EXPECT_TRUE(S->equivalent(*SS));
   SS = O->getStatus("//root/Xx");
   EXPECT_TRUE(S->equivalent(*SS));
   EXPECT_EQ(0, NumDiagnostics);
}

TEST_F(VFSFromYAMLTest, testCaseSensitive)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("//root/foo/bar/a");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'case-sensitive': 'true',\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'XX',\n"
            "                  'external-contents': '//root/foo/bar/a'\n"
            "                }\n"
            "              ]\n"
            "}]}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(FS);

   OptionalError<vfs::Status> SS = O->getStatus("//root/xx");
   EXPECT_EQ(SS.getError(), ErrorCode::no_such_file_or_directory);
   SS = O->getStatus("//root/xX");
   EXPECT_EQ(SS.getError(), ErrorCode::no_such_file_or_directory);
   SS = O->getStatus("//root/Xx");
   EXPECT_EQ(SS.getError(), ErrorCode::no_such_file_or_directory);
   EXPECT_EQ(0, NumDiagnostics);
}

TEST_F(VFSFromYAMLTest, testIllegalVFSFile)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());

   // invalid YAML at top-level
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString("{]", Lower);
   EXPECT_EQ(nullptr, FS.get());
   // invalid YAML in roots
   FS = getFromYAMLString("{ 'roots':[}", Lower);
   // invalid YAML in directory
   FS = getFromYAMLString(
            "{ 'roots':[ { 'name': 'foo', 'type': 'directory', 'contents': [}",
            Lower);
   EXPECT_EQ(nullptr, FS.get());

   // invalid configuration
   FS = getFromYAMLString("{ 'knobular': 'true', 'roots':[] }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString("{ 'case-sensitive': 'maybe', 'roots':[] }", Lower);
   EXPECT_EQ(nullptr, FS.get());

   // invalid roots
   FS = getFromYAMLString("{ 'roots':'' }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString("{ 'roots':{} }", Lower);
   EXPECT_EQ(nullptr, FS.get());

   // invalid entries
   FS = getFromYAMLString(
            "{ 'roots':[ { 'type': 'other', 'name': 'me', 'contents': '' }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString("{ 'roots':[ { 'type': 'file', 'name': [], "
                          "'external-contents': 'other' }",
                          Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString(
            "{ 'roots':[ { 'type': 'file', 'name': 'me', 'external-contents': [] }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString(
            "{ 'roots':[ { 'type': 'file', 'name': 'me', 'external-contents': {} }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString(
            "{ 'roots':[ { 'type': 'directory', 'name': 'me', 'contents': {} }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString(
            "{ 'roots':[ { 'type': 'directory', 'name': 'me', 'contents': '' }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString(
            "{ 'roots':[ { 'thingy': 'directory', 'name': 'me', 'contents': [] }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());

   // missing mandatory fields
   FS = getFromYAMLString("{ 'roots':[ { 'type': 'file', 'name': 'me' }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString(
            "{ 'roots':[ { 'type': 'file', 'external-contents': 'other' }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString("{ 'roots':[ { 'name': 'me', 'contents': [] }", Lower);
   EXPECT_EQ(nullptr, FS.get());

   // duplicate keys
   FS = getFromYAMLString("{ 'roots':[], 'roots':[] }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLString(
            "{ 'case-sensitive':'true', 'case-sensitive':'true', 'roots':[] }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS =
         getFromYAMLString("{ 'roots':[{'name':'me', 'name':'you', 'type':'file', "
                           "'external-contents':'blah' } ] }",
                           Lower);
   EXPECT_EQ(nullptr, FS.get());

   // missing version
   FS = getFromYAMLRawString("{ 'roots':[] }", Lower);
   EXPECT_EQ(nullptr, FS.get());

   // bad version number
   FS = getFromYAMLRawString("{ 'version':'foo', 'roots':[] }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLRawString("{ 'version':-1, 'roots':[] }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   FS = getFromYAMLRawString("{ 'version':100000, 'roots':[] }", Lower);
   EXPECT_EQ(nullptr, FS.get());
   EXPECT_EQ(24, NumDiagnostics);
}

TEST_F(VFSFromYAMLTest, testUseExternalName)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("//root/external/file");

   IntrusiveRefCountPtr<vfs::FileSystem> FS =
         getFromYAMLString("{ 'roots': [\n"
                           "  { 'type': 'file', 'name': '//root/A',\n"
                           "    'external-contents': '//root/external/file'\n"
                           "  },\n"
                           "  { 'type': 'file', 'name': '//root/B',\n"
                           "    'use-external-name': true,\n"
                           "    'external-contents': '//root/external/file'\n"
                           "  },\n"
                           "  { 'type': 'file', 'name': '//root/C',\n"
                           "    'use-external-name': false,\n"
                           "    'external-contents': '//root/external/file'\n"
                           "  }\n"
                           "] }",
                           Lower);
   ASSERT_TRUE(nullptr != FS.get());

   // default true
   EXPECT_EQ("//root/external/file", FS->getStatus("//root/A")->getName());
   // explicit
   EXPECT_EQ("//root/external/file", FS->getStatus("//root/B")->getName());
   EXPECT_EQ("//root/C", FS->getStatus("//root/C")->getName());

   // global configuration
   FS = getFromYAMLString("{ 'use-external-names': false,\n"
                          "  'roots': [\n"
                          "  { 'type': 'file', 'name': '//root/A',\n"
                          "    'external-contents': '//root/external/file'\n"
                          "  },\n"
                          "  { 'type': 'file', 'name': '//root/B',\n"
                          "    'use-external-name': true,\n"
                          "    'external-contents': '//root/external/file'\n"
                          "  },\n"
                          "  { 'type': 'file', 'name': '//root/C',\n"
                          "    'use-external-name': false,\n"
                          "    'external-contents': '//root/external/file'\n"
                          "  }\n"
                          "] }",
                          Lower);
   ASSERT_TRUE(nullptr != FS.get());

   // default
   EXPECT_EQ("//root/A", FS->getStatus("//root/A")->getName());
   // explicit
   EXPECT_EQ("//root/external/file", FS->getStatus("//root/B")->getName());
   EXPECT_EQ("//root/C", FS->getStatus("//root/C")->getName());
}

TEST_F(VFSFromYAMLTest, testMultiComponentPath)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("//root/other");

   // file in roots
   IntrusiveRefCountPtr<vfs::FileSystem> FS =
         getFromYAMLString("{ 'roots': [\n"
                           "  { 'type': 'file', 'name': '//root/path/to/file',\n"
                           "    'external-contents': '//root/other' }]\n"
                           "}",
                           Lower);
   ASSERT_TRUE(nullptr != FS.get());
   EXPECT_FALSE(FS->getStatus("//root/path/to/file").getError());
   EXPECT_FALSE(FS->getStatus("//root/path/to").getError());
   EXPECT_FALSE(FS->getStatus("//root/path").getError());
   EXPECT_FALSE(FS->getStatus("//root/").getError());

   // at the start
   FS = getFromYAMLString(
            "{ 'roots': [\n"
            "  { 'type': 'directory', 'name': '//root/path/to',\n"
            "    'contents': [ { 'type': 'file', 'name': 'file',\n"
            "                    'external-contents': '//root/other' }]}]\n"
            "}",
            Lower);
   ASSERT_TRUE(nullptr != FS.get());
   EXPECT_FALSE(FS->getStatus("//root/path/to/file").getError());
   EXPECT_FALSE(FS->getStatus("//root/path/to").getError());
   EXPECT_FALSE(FS->getStatus("//root/path").getError());
   EXPECT_FALSE(FS->getStatus("//root/").getError());

   // at the end
   FS = getFromYAMLString(
            "{ 'roots': [\n"
            "  { 'type': 'directory', 'name': '//root/',\n"
            "    'contents': [ { 'type': 'file', 'name': 'path/to/file',\n"
            "                    'external-contents': '//root/other' }]}]\n"
            "}",
            Lower);
   ASSERT_TRUE(nullptr != FS.get());
   EXPECT_FALSE(FS->getStatus("//root/path/to/file").getError());
   EXPECT_FALSE(FS->getStatus("//root/path/to").getError());
   EXPECT_FALSE(FS->getStatus("//root/path").getError());
   EXPECT_FALSE(FS->getStatus("//root/").getError());
}

TEST_F(VFSFromYAMLTest, testTrailingSlashes)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addRegularFile("//root/other");

   // file in roots
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'roots': [\n"
            "  { 'type': 'directory', 'name': '//root/path/to////',\n"
            "    'contents': [ { 'type': 'file', 'name': 'file',\n"
            "                    'external-contents': '//root/other' }]}]\n"
            "}",
            Lower);
   ASSERT_TRUE(nullptr != FS.get());
   EXPECT_FALSE(FS->getStatus("//root/path/to/file").getError());
   EXPECT_FALSE(FS->getStatus("//root/path/to").getError());
   EXPECT_FALSE(FS->getStatus("//root/path").getError());
   EXPECT_FALSE(FS->getStatus("//root/").getError());
}

TEST_F(VFSFromYAMLTest, testDirectoryIteration)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addDirectory("//root/");
   Lower->addDirectory("//root/foo");
   Lower->addDirectory("//root/foo/bar");
   Lower->addRegularFile("//root/foo/bar/a");
   Lower->addRegularFile("//root/foo/bar/b");
   Lower->addRegularFile("//root/file3");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'use-external-names': false,\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'file1',\n"
            "                  'external-contents': '//root/foo/bar/a'\n"
            "                },\n"
            "                {\n"
            "                  'type': 'file',\n"
            "                  'name': 'file2',\n"
            "                  'external-contents': '//root/foo/bar/b'\n"
            "                }\n"
            "              ]\n"
            "}\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(FS);

   std::error_code errorCode;
   checkContents(O->dirBegin("//root/", errorCode),
   {"//root/file1", "//root/file2", "//root/file3", "//root/foo"});

   checkContents(O->dirBegin("//root/foo/bar", errorCode),
   {"//root/foo/bar/a", "//root/foo/bar/b"});
}

TEST_F(VFSFromYAMLTest, testDirectoryIterationSameDirMultipleEntries)
{
   // https://llvm.org/bugs/show_bug.cgi?id=27725
   if (!supportsSameDirMultipleYAMLEntries())
      return;

   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addDirectory("//root/zab");
   Lower->addDirectory("//root/baz");
   Lower->addRegularFile("//root/zab/a");
   Lower->addRegularFile("//root/zab/b");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'use-external-names': false,\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/baz/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'x',\n"
            "                  'external-contents': '//root/zab/a'\n"
            "                }\n"
            "              ]\n"
            "},\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/baz/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'y',\n"
            "                  'external-contents': '//root/zab/b'\n"
            "                }\n"
            "              ]\n"
            "}\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(FS);

   std::error_code errorCode;

   checkContents(O->dirBegin("//root/baz/", errorCode),
   {"//root/baz/x", "//root/baz/y"});
}

TEST_F(VFSFromYAMLTest, testRecursiveDirectoryIterationLevel)
{

   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addDirectory("//root/a");
   Lower->addDirectory("//root/a/b");
   Lower->addDirectory("//root/a/b/c");
   Lower->addRegularFile("//root/a/b/c/file");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'use-external-names': false,\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/a/b/c/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'file',\n"
            "                  'external-contents': '//root/a/b/c/file'\n"
            "                }\n"
            "              ]\n"
            "},\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   IntrusiveRefCountPtr<vfs::OverlayFileSystem> O(
            new vfs::OverlayFileSystem(Lower));
   O->pushOverlay(FS);

   std::error_code errorCode;

   // Test RecursiveDirectoryIterator level()
   vfs::RecursiveDirectoryIterator I = vfs::RecursiveDirectoryIterator(
            *O, "//root", errorCode),
         E;
   ASSERT_FALSE(errorCode);
   for (int l = 0; I != E; I.increment(errorCode), ++l) {
      ASSERT_FALSE(errorCode);
      EXPECT_EQ(I.level(), l);
   }
   EXPECT_EQ(I, E);
}

TEST_F(VFSFromYAMLTest, testRelativePaths)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   // Filename at root level without a parent directory.
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'roots': [\n"
            "  { 'type': 'file', 'name': 'file-not-in-directory.h',\n"
            "    'external-contents': '//root/external/file'\n"
            "  }\n"
            "] }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());

   // Relative file path.
   FS = getFromYAMLString("{ 'roots': [\n"
                          "  { 'type': 'file', 'name': 'relative/file/path.h',\n"
                          "    'external-contents': '//root/external/file'\n"
                          "  }\n"
                          "] }",
                          Lower);
   EXPECT_EQ(nullptr, FS.get());

   // Relative directory path.
   FS = getFromYAMLString(
            "{ 'roots': [\n"
            "  { 'type': 'directory', 'name': 'relative/directory/path.h',\n"
            "    'contents': []\n"
            "  }\n"
            "] }",
            Lower);
   EXPECT_EQ(nullptr, FS.get());

   EXPECT_EQ(3, NumDiagnostics);
}

TEST_F(VFSFromYAMLTest, testNonFallthroughDirectoryIteration)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addDirectory("//root/");
   Lower->addRegularFile("//root/a");
   Lower->addRegularFile("//root/b");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'use-external-names': false,\n"
            "  'fallthrough': false,\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'c',\n"
            "                  'external-contents': '//root/a'\n"
            "                }\n"
            "              ]\n"
            "}\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   std::error_code errorCode;
   checkContents(FS->dirBegin("//root/", errorCode),
   {"//root/c"});
}

TEST_F(VFSFromYAMLTest, testDirectoryIterationWithDuplicates)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addDirectory("//root/");
   Lower->addRegularFile("//root/a");
   Lower->addRegularFile("//root/b");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'use-external-names': false,\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'a',\n"
            "                  'external-contents': '//root/a'\n"
            "                }\n"
            "              ]\n"
            "}\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   std::error_code errorCode;
   checkContents(FS->dirBegin("//root/", errorCode),
   {"//root/a", "//root/b"});
}

TEST_F(VFSFromYAMLTest, testDirectoryIterationErrorInVFSLayer)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addDirectory("//root/");
   Lower->addDirectory("//root/foo");
   Lower->addRegularFile("//root/foo/a");
   Lower->addRegularFile("//root/foo/b");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'use-external-names': false,\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'bar/a',\n"
            "                  'external-contents': '//root/foo/a'\n"
            "                }\n"
            "              ]\n"
            "}\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   std::error_code errorCode;
   checkContents(FS->dirBegin("//root/foo", errorCode),
   {"//root/foo/a", "//root/foo/b"});
}

TEST_F(VFSFromYAMLTest, testGetRealPath)
{
   IntrusiveRefCountPtr<DummyFileSystem> Lower(new DummyFileSystem());
   Lower->addDirectory("//dir/");
   Lower->addRegularFile("/foo");
   Lower->addSymlink("/link");
   IntrusiveRefCountPtr<vfs::FileSystem> FS = getFromYAMLString(
            "{ 'use-external-names': false,\n"
            "  'roots': [\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//root/',\n"
            "  'contents': [ {\n"
            "                  'type': 'file',\n"
            "                  'name': 'bar',\n"
            "                  'external-contents': '/link'\n"
            "                }\n"
            "              ]\n"
            "},\n"
            "{\n"
            "  'type': 'directory',\n"
            "  'name': '//dir/',\n"
            "  'contents': []\n"
            "}\n"
            "]\n"
            "}",
            Lower);
   ASSERT_TRUE(FS.get() != nullptr);

   // Regular file present in underlying file system.
   SmallString<16> RealPath;
   EXPECT_FALSE(FS->getRealPath("/foo", RealPath));
   EXPECT_EQ(RealPath.getStr(), "/foo");

   // File present in YAML pointing to symlink in underlying file system.
   EXPECT_FALSE(FS->getRealPath("//root/bar", RealPath));
   EXPECT_EQ(RealPath.getStr(), "/symlink");

   // Directories should fall back to the underlying file system is possible.
   EXPECT_FALSE(FS->getRealPath("//dir/", RealPath));
   EXPECT_EQ(RealPath.getStr(), "//dir/");

   // Try a non-existing file.
   EXPECT_EQ(FS->getRealPath("/non_existing", RealPath),
             ErrorCode::no_such_file_or_directory);
}
