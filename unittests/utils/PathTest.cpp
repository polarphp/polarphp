// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

// Created by polarboy on 2018/07/10.

#include "polarphp/utils/Path.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/utils/ConvertUtf.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/FileUtils.h"
#include "polarphp/utils/Host.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"
#include "gmock/gmock.h"

#ifdef _WIN32
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/utils/Chrono.h"
#include <windows.h>
#include <winerror.h>
#endif

#ifdef POLAR_ON_UNIX
#include <pwd.h>
#include <sys/stat.h>
#endif

using namespace polar::basic;
using namespace polar::utils;
using namespace polar::sys;
using namespace polar;
using polar::fs::FileRemover;

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

#define ASSERT_ERROR(x)                                                        \
   if (!x) {                                                                    \
   SmallString<128> MessageStorage;                                           \
   RawSvectorOutStream Message(MessageStorage);                               \
   Message << #x ": did not return a failure error code.\n";                  \
   GTEST_FATAL_FAILURE_(MessageStorage.getCStr());                              \
   }


namespace {

struct FileDescriptorCloser
{
   explicit FileDescriptorCloser(int FD) : FD(FD) {}
   ~FileDescriptorCloser() { ::close(FD); }
   int FD;
};

TEST(PathTest, testWorks)
{
   EXPECT_TRUE(fs::path::is_separator('/'));
   EXPECT_FALSE(fs::path::is_separator('\0'));
   EXPECT_FALSE(fs::path::is_separator('-'));
   EXPECT_FALSE(fs::path::is_separator(' '));

   EXPECT_TRUE(fs::path::is_separator('\\', fs::path::Style::windows));
   EXPECT_FALSE(fs::path::is_separator('\\', fs::path::Style::posix));

#ifdef _WIN32
   EXPECT_TRUE(fs::path::is_separator('\\'));
#else
   EXPECT_FALSE(fs::path::is_separator('\\'));
#endif
}

TEST(PathTest, testPath)
{
   SmallVector<StringRef, 40> paths;
   paths.push_back("");
   paths.push_back(".");
   paths.push_back("..");
   paths.push_back("foo");
   paths.push_back("/");
   paths.push_back("/foo");
   paths.push_back("foo/");
   paths.push_back("/foo/");
   paths.push_back("foo/bar");
   paths.push_back("/foo/bar");
   paths.push_back("//net");
   paths.push_back("//net/");
   paths.push_back("//net/foo");
   paths.push_back("///foo///");
   paths.push_back("///foo///bar");
   paths.push_back("/.");
   paths.push_back("./");
   paths.push_back("/..");
   paths.push_back("../");
   paths.push_back("foo/.");
   paths.push_back("foo/..");
   paths.push_back("foo/./");
   paths.push_back("foo/./bar");
   paths.push_back("foo/..");
   paths.push_back("foo/../");
   paths.push_back("foo/../bar");
   paths.push_back("c:");
   paths.push_back("c:/");
   paths.push_back("c:foo");
   paths.push_back("c:/foo");
   paths.push_back("c:foo/");
   paths.push_back("c:/foo/");
   paths.push_back("c:/foo/bar");
   paths.push_back("prn:");
   paths.push_back("c:\\");
   paths.push_back("c:foo");
   paths.push_back("c:\\foo");
   paths.push_back("c:foo\\");
   paths.push_back("c:\\foo\\");
   paths.push_back("c:\\foo/");
   paths.push_back("c:/foo\\bar");

   for (SmallVector<StringRef, 40>::const_iterator i = paths.begin(),
        e = paths.end();
        i != e;
        ++i) {
      SCOPED_TRACE(*i);
      SmallVector<StringRef, 5> componentStack;
      for (fs::path::ConstIterator ci = fs::path::begin(*i),
           ce = fs::path::end(*i);
           ci != ce;
           ++ci) {
         EXPECT_FALSE(ci->empty());
         componentStack.push_back(*ci);
      }
      SmallVector<StringRef, 5> reverseComponentStack;
      for (fs::path::ReverseIterator ci = fs::path::rbegin(*i),
           ce = fs::path::rend(*i);
           ci != ce;
           ++ci) {
         EXPECT_FALSE(ci->empty());
         reverseComponentStack.push_back(*ci);
      }
      std::reverse(reverseComponentStack.begin(), reverseComponentStack.end());
      EXPECT_THAT(componentStack, testing::ContainerEq(reverseComponentStack));

      // Crash test most of the API - since we're iterating over all of our paths
      // here there isn't really anything reasonable to assert on in the results.
      (void)fs::path::has_root_path(*i);
      (void)fs::path::root_path(*i);
      (void)fs::path::has_root_name(*i);
      (void)fs::path::root_name(*i);
      (void)fs::path::has_root_directory(*i);
      (void)fs::path::root_directory(*i);
      (void)fs::path::has_parent_path(*i);
      (void)fs::path::parent_path(*i);
      (void)fs::path::has_filename(*i);
      (void)fs::path::filename(*i);
      (void)fs::path::has_stem(*i);
      (void)fs::path::stem(*i);
      (void)fs::path::has_extension(*i);
      (void)fs::path::extension(*i);
      (void)fs::path::is_absolute(*i);
      (void)fs::path::is_relative(*i);

      SmallString<128> temp_store;
      temp_store = *i;
      ASSERT_NO_ERROR(fs::make_absolute(temp_store));
      temp_store = *i;
      fs::path::remove_filename(temp_store);

      temp_store = *i;
      fs::path::replace_extension(temp_store, "ext");
      StringRef filename(temp_store.begin(), temp_store.size()), stem, ext;
      stem = fs::path::stem(filename);
      ext  = fs::path::extension(filename);
      EXPECT_EQ(*fs::path::rbegin(filename), (stem + ext).getStr());

      fs::path::native(*i, temp_store);
   }

   SmallString<32> Relative("foo.cpp");
   ASSERT_NO_ERROR(fs::make_absolute("/root", Relative));
   Relative[5] = '/'; // Fix up windows paths.
   ASSERT_EQ("/root/foo.cpp", Relative);
}

TEST(PathTest, testFilenameParent)
{

   EXPECT_EQ("/", fs::path::filename("/"));
   EXPECT_EQ("", fs::path::parent_path("/"));

   EXPECT_EQ("\\", fs::path::filename("c:\\", fs::path::Style::windows));
   EXPECT_EQ("c:", fs::path::parent_path("c:\\", fs::path::Style::windows));

   EXPECT_EQ("/", fs::path::filename("///"));

   EXPECT_EQ("", fs::path::parent_path("///"));

   EXPECT_EQ("\\", fs::path::filename("c:\\\\", fs::path::Style::windows));
   EXPECT_EQ("c:", fs::path::parent_path("c:\\\\", fs::path::Style::windows));

   EXPECT_EQ("bar", fs::path::filename("/foo/bar"));
   EXPECT_EQ("/foo", fs::path::parent_path("/foo/bar"));

   EXPECT_EQ("foo", fs::path::filename("/foo"));
   EXPECT_EQ("/", fs::path::parent_path("/foo"));

   EXPECT_EQ("foo", fs::path::filename("foo"));
   EXPECT_EQ("", fs::path::parent_path("foo"));

   EXPECT_EQ(".", fs::path::filename("foo/"));
   EXPECT_EQ("foo", fs::path::parent_path("foo/"));

   EXPECT_EQ("//net", fs::path::filename("//net"));
   EXPECT_EQ("", fs::path::parent_path("//net"));

   EXPECT_EQ("/", fs::path::filename("//net/"));
   EXPECT_EQ("//net", fs::path::parent_path("//net/"));

   EXPECT_EQ("foo", fs::path::filename("//net/foo"));
   EXPECT_EQ("//net/", fs::path::parent_path("//net/foo"));

   // These checks are just to make sure we do something reasonable with the
   // paths below. They are not meant to prescribe the one true interpretation of
   // these paths. Other decompositions (e.g. "//" -> "" + "//") are also
   // possible.
   EXPECT_EQ("/", fs::path::filename("//"));
   EXPECT_EQ("", fs::path::parent_path("//"));

   EXPECT_EQ("\\", fs::path::filename("\\\\", fs::path::Style::windows));
   EXPECT_EQ("", fs::path::parent_path("\\\\", fs::path::Style::windows));

   EXPECT_EQ("\\", fs::path::filename("\\\\\\", fs::path::Style::windows));
   EXPECT_EQ("", fs::path::parent_path("\\\\\\", fs::path::Style::windows));
}

static std::vector<StringRef>
GetComponents(StringRef Path, fs::path::Style S = fs::path::Style::native)
{
   return {fs::path::begin(Path, S), fs::path::end(Path)};
}

TEST(PathTest, testPathIterator)
{
   EXPECT_THAT(GetComponents("/foo"), testing::ElementsAre("/", "foo"));
   EXPECT_THAT(GetComponents("/"), testing::ElementsAre("/"));
   EXPECT_THAT(GetComponents("//"), testing::ElementsAre("/"));
   EXPECT_THAT(GetComponents("///"), testing::ElementsAre("/"));
   EXPECT_THAT(GetComponents("c/d/e/foo.txt"),
               testing::ElementsAre("c", "d", "e", "foo.txt"));
   EXPECT_THAT(GetComponents(".c/.d/../."),
               testing::ElementsAre(".c", ".d", "..", "."));
   EXPECT_THAT(GetComponents("/c/d/e/foo.txt"),
               testing::ElementsAre("/", "c", "d", "e", "foo.txt"));
   EXPECT_THAT(GetComponents("/.c/.d/../."),
               testing::ElementsAre("/", ".c", ".d", "..", "."));
   EXPECT_THAT(GetComponents("c:\\c\\e\\foo.txt", fs::path::Style::windows),
               testing::ElementsAre("c:", "\\", "c", "e", "foo.txt"));
   EXPECT_THAT(GetComponents("//net/"), testing::ElementsAre("//net", "/"));
   EXPECT_THAT(GetComponents("//net/c/foo.txt"),
               testing::ElementsAre("//net", "/", "c", "foo.txt"));
}

TEST(PathTest, testAbsolutePathIteratorEnd)
{
   // Trailing slashes are converted to '.' unless they are part of the root path.
   SmallVector<std::pair<StringRef, fs::path::Style>, 4> Paths;
   Paths.emplace_back("/foo/", fs::path::Style::native);
   Paths.emplace_back("/foo//", fs::path::Style::native);
   Paths.emplace_back("//net/foo/", fs::path::Style::native);
   Paths.emplace_back("c:\\foo\\", fs::path::Style::windows);

   for (auto &Path : Paths) {
      SCOPED_TRACE(Path.first);
      StringRef LastComponent = *fs::path::rbegin(Path.first, Path.second);
      EXPECT_EQ(".", LastComponent);
   }

   SmallVector<std::pair<StringRef, fs::path::Style>, 3> RootPaths;
   RootPaths.emplace_back("/", fs::path::Style::native);
   RootPaths.emplace_back("//net/", fs::path::Style::native);
   RootPaths.emplace_back("c:\\", fs::path::Style::windows);
   RootPaths.emplace_back("//net//", fs::path::Style::native);
   RootPaths.emplace_back("c:\\\\", fs::path::Style::windows);

   for (auto &Path : RootPaths) {
      SCOPED_TRACE(Path.first);
      StringRef LastComponent = *fs::path::rbegin(Path.first, Path.second);
      EXPECT_EQ(1u, LastComponent.size());
      EXPECT_TRUE(fs::path::is_separator(LastComponent[0], Path.second));
   }
}

TEST(PathTest, testHomeDirectory)
{
   std::string expected;
#ifdef _WIN32
   if (wchar_t const *fs::path = ::_wgetenv(L"USERPROFILE")) {
      auto pathLen = ::wcslen(path);
      ArrayRef<char> ref{reinterpret_cast<char const *>(path),
               pathLen * sizeof(wchar_t)};
      convertUTF16ToUTF8String(ref, expected);
   }
#else
   if (char const *path = ::getenv("HOME"))
      expected = path;
#endif
   // Do not try to test it if we don't know what to expect.
   // On Windows we use something better than env vars.
   if (!expected.empty()) {
      SmallString<128> homeDir;
      auto status = fs::path::home_directory(homeDir);
      EXPECT_TRUE(status);
      EXPECT_EQ(expected, homeDir);
   }
}

#ifdef POLAR_ON_UNIX
TEST(PathTest, testHomeDirectoryWithNoEnv)
{
   std::string originalStorage;
   char const *OriginalEnv = ::getenv("HOME");
   if (OriginalEnv) {
      // We're going to unset it, so make a copy and save a pointer to the copy
      // so that we can reset it at the end of the test.
      originalStorage = OriginalEnv;
      OriginalEnv = originalStorage.c_str();
   }

   // Don't run the test if we have nothing to compare against.
   struct passwd *pw = getpwuid(getuid());
   if (!pw || !pw->pw_dir) return;

   ::unsetenv("HOME");
   EXPECT_EQ(nullptr, ::getenv("HOME"));
   std::string PwDir = pw->pw_dir;

   SmallString<128> homeDir;
   auto status = fs::path::home_directory(homeDir);
   EXPECT_TRUE(status);
   EXPECT_EQ(PwDir, homeDir);

   // Now put the environment back to its original state (meaning that if it was
   // unset before, we don't reset it).
   if (OriginalEnv) ::setenv("HOME", OriginalEnv, 1);
}
#endif

TEST(PathTest, testTempDirectory)
{
   SmallString<32> TempDir;
   fs::path::system_temp_directory(false, TempDir);
   EXPECT_TRUE(!TempDir.empty());
   TempDir.clear();
   fs::path::system_temp_directory(true, TempDir);
   EXPECT_TRUE(!TempDir.empty());
}

#ifdef _WIN32
static std::string path2regex(std::string Path) {
   size_t Pos = 0;
   while ((Pos = Path.find('\\', Pos)) != std::string::npos) {
      Path.replace(Pos, 1, "\\\\");
      Pos += 2;
   }
   return Path;
}

/// Helper for running temp dir test in separated process. See below.
#define EXPECT_TEMP_DIR(prepare, expected)                                     \
   EXPECT_EXIT(                                                                 \
{                                                                        \
   prepare;                                                               \
   SmallString<300> TempDir;                                              \
   fs::path::system_temp_directory(true, TempDir);                            \
   raw_os_ostream(std::cerr) << TempDir;                                  \
   std::exit(0);                                                          \
},                                                                       \
   ::testing::ExitedWithCode(0), path2regex(expected))

TEST(SupportDeathTest, TempDirectoryOnWindows) {
   // In this test we want to check how system_temp_directory responds to
   // different values of specific env vars. To prevent corrupting env vars of
   // the current process all checks are done in separated processes.
   EXPECT_TEMP_DIR(_wputenv_s(L"TMP", L"C:\\OtherFolder"), "C:\\OtherFolder");
   EXPECT_TEMP_DIR(_wputenv_s(L"TMP", L"C:/Unix/Path/Seperators"),
                   "C:\\Unix\\Path\\Seperators");
   EXPECT_TEMP_DIR(_wputenv_s(L"TMP", L"Local Path"), ".+\\Local Path$");
   EXPECT_TEMP_DIR(_wputenv_s(L"TMP", L"F:\\TrailingSep\\"), "F:\\TrailingSep");
   EXPECT_TEMP_DIR(
            _wputenv_s(L"TMP", L"C:\\2\x03C0r-\x00B5\x00B3\\\x2135\x2080"),
            "C:\\2\xCF\x80r-\xC2\xB5\xC2\xB3\\\xE2\x84\xB5\xE2\x82\x80");

   // Test $TMP empty, $TEMP set.
   EXPECT_TEMP_DIR(
   {
               _wputenv_s(L"TMP", L"");
               _wputenv_s(L"TEMP", L"C:\\Valid\\Path");
            },
            "C:\\Valid\\Path");

   // All related env vars empty
   EXPECT_TEMP_DIR(
   {
               _wputenv_s(L"TMP", L"");
               _wputenv_s(L"TEMP", L"");
               _wputenv_s(L"USERPROFILE", L"");
            },
            "C:\\Temp");

   // Test evn var / path with 260 chars.
   SmallString<270> Expected{"C:\\Temp\\AB\\123456789"};
   while (Expected.size() < 260)
      Expected.append("\\DirNameWith19Charss");
   ASSERT_EQ(260U, Expected.size());
   EXPECT_TEMP_DIR(_putenv_s("TMP", Expected.getStr()), Expected.getStr());
}
#endif

class FileSystemTest : public testing::Test
{
protected:
   /// Unique temporary directory in which all created filesystem entities must
   /// be placed. It is removed at the end of each test (must be empty).
   SmallString<128> TestDirectory;
   SmallString<128> NonExistantFile;

   void SetUp() override {
      ASSERT_NO_ERROR(
               fs::create_unique_directory("file-system-test", TestDirectory));
      // We don't care about this specific file.
      error_stream() << "Test Directory: " << TestDirectory << '\n';
      error_stream().flush();
      NonExistantFile = TestDirectory;

      // Even though this value is hardcoded, is a 128-bit GUID, so we should be
      // guaranteed that this file will never exist.
      fs::path::append(NonExistantFile, "1B28B495C16344CB9822E588CD4C3EF0");
   }

   void TearDown() override { ASSERT_NO_ERROR(fs::remove(TestDirectory.getStr())); }
};

TEST_F(FileSystemTest, testUnique)
{
   // Create a temp file.
   int FileDescriptor;
   SmallString<64> TempPath;
   ASSERT_NO_ERROR(
            fs::create_temporary_file("prefix", "temp", FileDescriptor, TempPath));

   // The same file should return an identical unique id.
   fs::UniqueId F1, F2;
   ASSERT_NO_ERROR(fs::get_unique_id(Twine(TempPath), F1));
   ASSERT_NO_ERROR(fs::get_unique_id(Twine(TempPath), F2));
   ASSERT_EQ(F1, F2);

   // Different files should return different unique ids.
   int FileDescriptor2;
   SmallString<64> TempPath2;
   ASSERT_NO_ERROR(
            fs::create_temporary_file("prefix", "temp", FileDescriptor2, TempPath2));

   fs::UniqueId D;
   ASSERT_NO_ERROR(fs::get_unique_id(Twine(TempPath2), D));
   ASSERT_NE(D, F1);
   ::close(FileDescriptor2);

   ASSERT_NO_ERROR(fs::remove(Twine(TempPath2)));

   // Two paths representing the same file on disk should still provide the
   // same unique id.  We can test this by making a hard link.
   ASSERT_NO_ERROR(fs::create_link(Twine(TempPath), Twine(TempPath2)));
   fs::UniqueId D2;
   ASSERT_NO_ERROR(fs::get_unique_id(Twine(TempPath2), D2));
   ASSERT_EQ(D2, F1);

   ::close(FileDescriptor);

   SmallString<128> Dir1;
   ASSERT_NO_ERROR(
            fs::create_unique_directory("dir1", Dir1));
   ASSERT_NO_ERROR(fs::get_unique_id(Dir1.getStr(), F1));
   ASSERT_NO_ERROR(fs::get_unique_id(Dir1.getStr(), F2));
   ASSERT_EQ(F1, F2);

   SmallString<128> Dir2;
   ASSERT_NO_ERROR(
            fs::create_unique_directory("dir2", Dir2));
   ASSERT_NO_ERROR(fs::get_unique_id(Dir2.getStr(), F2));
   ASSERT_NE(F1, F2);
   ASSERT_NO_ERROR(fs::remove(Dir1));
   ASSERT_NO_ERROR(fs::remove(Dir2));
   ASSERT_NO_ERROR(fs::remove(TempPath2));
   ASSERT_NO_ERROR(fs::remove(TempPath));
}

TEST_F(FileSystemTest, testRealPath)
{
   ASSERT_NO_ERROR(
            fs::create_directories(Twine(TestDirectory) + "/test1/test2/test3"));
   ASSERT_TRUE(fs::exists(Twine(TestDirectory) + "/test1/test2/test3"));

   SmallString<64> RealBase;
   SmallString<64> Expected;
   SmallString<64> Actual;

   // TestDirectory itself might be under a symlink or have been specified with
   // a different case than the existing temp directory.  In such cases real_path
   // on the concatenated path will differ in the TestDirectory portion from
   // how we specified it.  Make sure to compare against the real_path of the
   // TestDirectory, and not just the value of TestDirectory.
   ASSERT_NO_ERROR(fs::real_path(TestDirectory, RealBase));
   fs::path::native(Twine(RealBase) + "/test1/test2", Expected);

   ASSERT_NO_ERROR(fs::real_path(
                      Twine(TestDirectory) + "/././test1/../test1/test2/./test3/..", Actual));

   EXPECT_EQ(Expected, Actual);

   SmallString<64> homeDir;

   // This can fail if $HOME is not set and getpwuid fails.
   bool result = fs::path::home_directory(homeDir);
   if (result) {
      ASSERT_NO_ERROR(fs::real_path(homeDir, Expected));
      ASSERT_NO_ERROR(fs::real_path("~", Actual, true));
      EXPECT_EQ(Expected, Actual);
      ASSERT_NO_ERROR(fs::real_path("~/", Actual, true));
      EXPECT_EQ(Expected, Actual);
   }

   ASSERT_NO_ERROR(fs::remove_directories(Twine(TestDirectory) + "/test1"));
}


TEST_F(FileSystemTest, testExpandTilde)
{
   SmallString<64> Expected;
   SmallString<64> Actual;
   SmallString<64> HomeDir;

   // This can fail if $HOME is not set and getpwuid fails.
   bool result = polar::fs::path::home_directory(HomeDir);
   if (result) {
      fs::expand_tilde(HomeDir, Expected);

      fs::expand_tilde("~", Actual);
      EXPECT_EQ(Expected, Actual);

#ifdef _WIN32
      Expected += "\\foo";
      fs::expand_tilde("~\\foo", Actual);
#else
      Expected += "/foo";
      fs::expand_tilde("~/foo", Actual);
#endif

      EXPECT_EQ(Expected, Actual);
   }
}

#ifdef POLAR_ON_UNIX
TEST_F(FileSystemTest, testRealPathNoReadPerm) {
   SmallString<64> Expanded;

   ASSERT_NO_ERROR(
            fs::create_directories(Twine(TestDirectory) + "/noreadperm"));
   ASSERT_TRUE(fs::exists(Twine(TestDirectory) + "/noreadperm"));

   fs::set_permissions(Twine(TestDirectory) + "/noreadperm", fs::no_perms);
   fs::set_permissions(Twine(TestDirectory) + "/noreadperm", fs::all_exe);

   ASSERT_NO_ERROR(fs::real_path(Twine(TestDirectory) + "/noreadperm", Expanded,
                                 false));

   ASSERT_NO_ERROR(fs::remove_directories(Twine(TestDirectory) + "/noreadperm"));
}
#endif


TEST_F(FileSystemTest, testTempFileKeepDiscard) {
   // We can keep then discard.
   auto TempFileOrError = fs::TempFile::create(TestDirectory + "/test-%%%%");
   ASSERT_TRUE((bool)TempFileOrError);
   fs::TempFile File = std::move(*TempFileOrError);
   ASSERT_FALSE((bool)File.keep(TestDirectory + "/keep"));
   ASSERT_FALSE((bool)File.discard());
   ASSERT_TRUE(fs::exists(TestDirectory + "/keep"));
   ASSERT_NO_ERROR(fs::remove(TestDirectory + "/keep"));
}

TEST_F(FileSystemTest, testTempFileDiscardDiscard)
{
   // We can discard twice.
   auto TempFileOrError = fs::TempFile::create(TestDirectory + "/test-%%%%");
   ASSERT_TRUE((bool)TempFileOrError);
   fs::TempFile File = std::move(*TempFileOrError);
   ASSERT_FALSE((bool)File.discard());
   ASSERT_FALSE((bool)File.discard());
   ASSERT_FALSE(fs::exists(TestDirectory + "/keep"));
}

TEST_F(FileSystemTest, testTempFiles)
{
   // Create a temp file.
   int FileDescriptor;
   SmallString<64> TempPath;
   ASSERT_NO_ERROR(
            fs::create_temporary_file("prefix", "temp", FileDescriptor, TempPath));

   // Make sure it exists.
   ASSERT_TRUE(fs::exists(Twine(TempPath)));

   // Create another temp tile.
   int FD2;
   SmallString<64> TempPath2;
   ASSERT_NO_ERROR(fs::create_temporary_file("prefix", "temp", FD2, TempPath2));
   ASSERT_TRUE(TempPath2.endsWith(".temp"));
   ASSERT_NE(TempPath.getStr(), TempPath2.getStr());

   fs::FileStatus A, B;
   ASSERT_NO_ERROR(fs::status(Twine(TempPath), A));
   ASSERT_NO_ERROR(fs::status(Twine(TempPath2), B));
   EXPECT_FALSE(fs::equivalent(A, B));

   ::close(FD2);

   // Remove Temp2.
   ASSERT_NO_ERROR(fs::remove(Twine(TempPath2)));
   ASSERT_NO_ERROR(fs::remove(Twine(TempPath2)));
   ASSERT_EQ(fs::remove(Twine(TempPath2), false),
             ErrorCode::no_such_file_or_directory);

   std::error_code errorCode = fs::status(TempPath2.getStr(), B);
   EXPECT_EQ(errorCode, ErrorCode::no_such_file_or_directory);
   EXPECT_EQ(B.getType(), fs::FileType::file_not_found);

   // Make sure Temp2 doesn't exist.
   ASSERT_EQ(fs::access(Twine(TempPath2), fs::AccessMode::Exist),
             ErrorCode::no_such_file_or_directory);

   SmallString<64> TempPath3;
   ASSERT_NO_ERROR(fs::create_temporary_file("prefix", "", TempPath3));
   ASSERT_FALSE(TempPath3.endsWith("."));
   fs::FileRemover Cleanup3(TempPath3);

   // Create a hard link to Temp1.
   ASSERT_NO_ERROR(fs::create_link(Twine(TempPath), Twine(TempPath2)));
   bool equal;
   ASSERT_NO_ERROR(fs::equivalent(Twine(TempPath), Twine(TempPath2), equal));
   EXPECT_TRUE(equal);
   ASSERT_NO_ERROR(fs::status(Twine(TempPath), A));
   ASSERT_NO_ERROR(fs::status(Twine(TempPath2), B));
   EXPECT_TRUE(fs::equivalent(A, B));

   // Remove Temp1.
   ::close(FileDescriptor);
   ASSERT_NO_ERROR(fs::remove(Twine(TempPath)));

   // Remove the hard link.
   ASSERT_NO_ERROR(fs::remove(Twine(TempPath2)));

   // Make sure Temp1 doesn't exist.
   ASSERT_EQ(fs::access(Twine(TempPath), fs::AccessMode::Exist),
             ErrorCode::no_such_file_or_directory);

#ifdef _WIN32
   // Path name > 260 chars should get an error.
   const char *fs::path270 =
         "abcdefghijklmnopqrstuvwxyz9abcdefghijklmnopqrstuvwxyz8"
         "abcdefghijklmnopqrstuvwxyz7abcdefghijklmnopqrstuvwxyz6"
         "abcdefghijklmnopqrstuvwxyz5abcdefghijklmnopqrstuvwxyz4"
         "abcdefghijklmnopqrstuvwxyz3abcdefghijklmnopqrstuvwxyz2"
         "abcdefghijklmnopqrstuvwxyz1abcdefghijklmnopqrstuvwxyz0";
   EXPECT_EQ(fs::create_unique_file(Path270, FileDescriptor, TempPath),
             errc::invalid_argument);
   // Relative path < 247 chars, no problem.
   const char *fs::path216 =
         "abcdefghijklmnopqrstuvwxyz7abcdefghijklmnopqrstuvwxyz6"
         "abcdefghijklmnopqrstuvwxyz5abcdefghijklmnopqrstuvwxyz4"
         "abcdefghijklmnopqrstuvwxyz3abcdefghijklmnopqrstuvwxyz2"
         "abcdefghijklmnopqrstuvwxyz1abcdefghijklmnopqrstuvwxyz0";
   ASSERT_NO_ERROR(fs::create_temporary_file(Path216, "", TempPath));
   ASSERT_NO_ERROR(fs::remove(Twine(TempPath)));
#endif
}

TEST_F(FileSystemTest, testTempFileCollisions)
{
   SmallString<128> TestDirectory;
   ASSERT_NO_ERROR(
            fs::create_unique_directory("CreateUniqueFileTest", TestDirectory));
   FileRemover Cleanup(TestDirectory);
   SmallString<128> Model = TestDirectory;
   polar::fs::path::append(Model, "%.tmp");
   SmallString<128> Path;
   std::vector<fs::TempFile> TempFiles;

   auto tryCreateTempFile = [&]() {
      Expected<fs::TempFile> T = fs::TempFile::create(Model);
      if (T) {
         TempFiles.push_back(std::move(*T));
         return true;
      } else {
         log_all_unhandled_errors(T.takeError(), error_stream(),
                                  "Failed to create temporary file: ");
         return false;
      }
   };

   // We should be able to create exactly 16 temporary files.
   for (int i = 0; i < 16; ++i)
      EXPECT_TRUE(tryCreateTempFile());
   EXPECT_FALSE(tryCreateTempFile());

   for (fs::TempFile &T : TempFiles)
      cant_fail(T.discard());
}

TEST_F(FileSystemTest, testCreateDir)
{
   ASSERT_NO_ERROR(fs::create_directory(Twine(TestDirectory) + "foo"));
   ASSERT_NO_ERROR(fs::create_directory(Twine(TestDirectory) + "foo"));
   ASSERT_EQ(fs::create_directory(Twine(TestDirectory) + "foo", false),
             ErrorCode::file_exists);
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "foo"));

#ifdef POLAR_ON_UNIX
   // Set a 0000 umask so that we can test our directory getPermissions.
   mode_t OldUmask = ::umask(0000);

   fs::FileStatus Status;
   ASSERT_NO_ERROR(
            fs::create_directory(Twine(TestDirectory) + "baz500", false,
                                 fs::Permission::owner_read | fs::Permission::owner_exe));
   ASSERT_NO_ERROR(fs::status(Twine(TestDirectory) + "baz500", Status));
   ASSERT_EQ(Status.getPermissions() & fs::Permission::all_all,
             fs::Permission::owner_read | fs::Permission::owner_exe);
   ASSERT_NO_ERROR(fs::create_directory(Twine(TestDirectory) + "baz777", false,
                                        fs::Permission::all_all));
   ASSERT_NO_ERROR(fs::status(Twine(TestDirectory) + "baz777", Status));
   ASSERT_EQ(Status.getPermissions() & fs::Permission::all_all, fs::Permission::all_all);

   // Restore umask to be safe.
   ::umask(OldUmask);
#endif

#ifdef _WIN32
   // Prove that create_directories() can handle a pathname > 248 characters,
   // which is the documented limit for CreateDirectory().
   // (248 is MAX_PATH subtracting room for an 8.3 filename.)
   // Generate a directory path guaranteed to fall into that range.
   size_t TmpLen = TestDirectory.size();
   const char *OneDir = "\\123456789";
   size_t OneDirLen = strlen(OneDir);
   ASSERT_LT(OneDirLen, 12U);
   size_t NLevels = ((248 - TmpLen) / OneDirLen) + 1;
   SmallString<260> LongDir(TestDirectory);
   for (size_t I = 0; I < NLevels; ++I)
      LongDir.append(OneDir);
   ASSERT_NO_ERROR(fs::create_directories(Twine(LongDir)));
   ASSERT_NO_ERROR(fs::create_directories(Twine(LongDir)));
   ASSERT_EQ(fs::create_directories(Twine(LongDir), false),
             errc::file_exists);
   // Tidy up, "recursively" removing the directories.
   StringRef ThisDir(LongDir);
   for (size_t J = 0; J < NLevels; ++J) {
      ASSERT_NO_ERROR(fs::remove(ThisDir));
      ThisDir = fs::path::parent_path(ThisDir);
   }

   // Also verify that paths with Unix separators are handled correctly.
   std::string LongPathWithUnixSeparators(TestDirectory.getStr());
   // Add at least one subdirectory to TestDirectory, and replace slashes with
   // backslashes
   do {
      LongPathWithUnixSeparators.append("/DirNameWith19Charss");
   } while (LongPathWithUnixSeparators.size() < 260);
   std::replace(LongPathWithUnixSeparators.begin(),
                LongPathWithUnixSeparators.end(),
                '\\', '/');
   ASSERT_NO_ERROR(fs::create_directories(Twine(LongPathWithUnixSeparators)));
   // cleanup
   ASSERT_NO_ERROR(fs::remove_directories(Twine(TestDirectory) +
                                          "/DirNameWith19Charss"));

   // Similarly for a relative pathname.  Need to set the current directory to
   // TestDirectory so that the one we create ends up in the right place.
   char PreviousDir[260];
   size_t PreviousDirLen = ::GetCurrentDirectoryA(260, PreviousDir);
   ASSERT_GT(PreviousDirLen, 0U);
   ASSERT_LT(PreviousDirLen, 260U);
   ASSERT_NE(::SetCurrentDirectoryA(TestDirectory.getStr()), 0);
   LongDir.clear();
   // Generate a relative directory name with absolute length > 248.
   size_t LongDirLen = 249 - TestDirectory.size();
   LongDir.assign(LongDirLen, 'a');
   ASSERT_NO_ERROR(fs::create_directory(Twine(LongDir)));
   // While we're here, prove that .. and . handling works in these long paths.
   const char *DotDotDirs = "\\..\\.\\b";
   LongDir.append(DotDotDirs);
   ASSERT_NO_ERROR(fs::create_directory("b"));
   ASSERT_EQ(fs::create_directory(Twine(LongDir), false), errc::file_exists);
   // And clean up.
   ASSERT_NO_ERROR(fs::remove("b"));
   ASSERT_NO_ERROR(fs::remove(
                      Twine(LongDir.substr(0, LongDir.size() - strlen(DotDotDirs)))));
   ASSERT_NE(::SetCurrentDirectoryA(PreviousDir), 0);
#endif
}

TEST_F(FileSystemTest, testDirectoryIteration)
{
   std::error_code ec;
   for (fs::DirectoryIterator i(".", ec), e; i != e; i.increment(ec)) {
      ASSERT_NO_ERROR(ec);
   }
   // Create a known hierarchy to recurse over.
   ASSERT_NO_ERROR(
            fs::create_directories(Twine(TestDirectory) + "/recursive/a0/aa1"));
   ASSERT_NO_ERROR(
            fs::create_directories(Twine(TestDirectory) + "/recursive/a0/ab1"));
   ASSERT_NO_ERROR(fs::create_directories(Twine(TestDirectory) +
                                          "/recursive/dontlookhere/da1"));
   ASSERT_NO_ERROR(
            fs::create_directories(Twine(TestDirectory) + "/recursive/z0/za1"));
   ASSERT_NO_ERROR(
            fs::create_directories(Twine(TestDirectory) + "/recursive/pop/p1"));
   typedef std::vector<std::string> v_t;
   v_t visited;
   for (fs::RecursiveDirectoryIterator i(Twine(TestDirectory)
                                         + "/recursive", ec), e; i != e; i.increment(ec)){
      ASSERT_NO_ERROR(ec);
      if (fs::path::filename(i->getPath()) == "p1") {
         i.pop();
         // FIXME: RecursiveDirectoryIterator should be more robust.
         if (i == e) break;
      }
      if (fs::path::filename(i->getPath()) == "dontlookhere")
         i.noPush();
      visited.push_back(fs::path::filename(i->getPath()));
   }
   v_t::const_iterator a0 = find(visited, "a0");
   v_t::const_iterator aa1 = find(visited, "aa1");
   v_t::const_iterator ab1 = find(visited, "ab1");
   v_t::const_iterator dontlookhere = find(visited, "dontlookhere");
   v_t::const_iterator da1 = find(visited, "da1");
   v_t::const_iterator z0 = find(visited, "z0");
   v_t::const_iterator za1 = find(visited, "za1");
   v_t::const_iterator pop = find(visited, "pop");
   v_t::const_iterator p1 = find(visited, "p1");

   // Make sure that each path was visited correctly.
   ASSERT_NE(a0, visited.end());
   ASSERT_NE(aa1, visited.end());
   ASSERT_NE(ab1, visited.end());
   ASSERT_NE(dontlookhere, visited.end());
   ASSERT_EQ(da1, visited.end()); // Not visited.
   ASSERT_NE(z0, visited.end());
   ASSERT_NE(za1, visited.end());
   ASSERT_NE(pop, visited.end());
   ASSERT_EQ(p1, visited.end()); // Not visited.

   // Make sure that parents were visited before children. No other ordering
   // guarantees can be made across siblings.
   ASSERT_LT(a0, aa1);
   ASSERT_LT(a0, ab1);
   ASSERT_LT(z0, za1);

   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/a0/aa1"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/a0/ab1"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/a0"));
   ASSERT_NO_ERROR(
            fs::remove(Twine(TestDirectory) + "/recursive/dontlookhere/da1"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/dontlookhere"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/pop/p1"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/pop"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/z0/za1"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive/z0"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/recursive"));

   // Test RecursiveDirectoryIterator level()
   ASSERT_NO_ERROR(
            fs::create_directories(Twine(TestDirectory) + "/reclevel/a/b/c"));
   fs::RecursiveDirectoryIterator I(Twine(TestDirectory) + "/reclevel", ec), E;
   for (int l = 0; I != E; I.increment(ec), ++l) {
      ASSERT_NO_ERROR(ec);
      EXPECT_EQ(I.getLevel(), l);
   }
   EXPECT_EQ(I, E);
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/reclevel/a/b/c"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/reclevel/a/b"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/reclevel/a"));
   ASSERT_NO_ERROR(fs::remove(Twine(TestDirectory) + "/reclevel"));
}

#ifdef POLAR_ON_UNIX
TEST_F(FileSystemTest, testBrokenSymlinkDirectoryIteration)
{
   // Create a known hierarchy to recurse over.
     ASSERT_NO_ERROR(fs::create_directories(Twine(TestDirectory) + "/symlink"));
     ASSERT_NO_ERROR(
         fs::create_link("no_such_file", Twine(TestDirectory) + "/symlink/a"));
     ASSERT_NO_ERROR(
         fs::create_directories(Twine(TestDirectory) + "/symlink/b/bb"));
     ASSERT_NO_ERROR(
         fs::create_link("no_such_file", Twine(TestDirectory) + "/symlink/b/ba"));
     ASSERT_NO_ERROR(
         fs::create_link("no_such_file", Twine(TestDirectory) + "/symlink/b/bc"));
     ASSERT_NO_ERROR(
         fs::create_link("no_such_file", Twine(TestDirectory) + "/symlink/c"));
     ASSERT_NO_ERROR(
         fs::create_directories(Twine(TestDirectory) + "/symlink/d/dd/ddd"));
     ASSERT_NO_ERROR(fs::create_link(Twine(TestDirectory) + "/symlink/d/dd",
                                     Twine(TestDirectory) + "/symlink/d/da"));
     ASSERT_NO_ERROR(
         fs::create_link("no_such_file", Twine(TestDirectory) + "/symlink/e"));

     typedef std::vector<std::string> v_t;
     v_t VisitedNonBrokenSymlinks;
     v_t VisitedBrokenSymlinks;
     std::error_code ec;
     using testing::UnorderedElementsAre;
     using testing::UnorderedElementsAreArray;

     // Broken symbol links are expected to throw an error.
     for (fs::DirectoryIterator i(Twine(TestDirectory) + "/symlink", ec), e;
          i != e; i.increment(ec)) {
       ASSERT_NO_ERROR(ec);
       if (i->getStatus().getError() ==
           std::make_error_code(std::errc::no_such_file_or_directory)) {
         VisitedBrokenSymlinks.push_back(polar::fs::path::filename(i->getPath()));
         continue;
       }
       VisitedNonBrokenSymlinks.push_back(polar::fs::path::filename(i->getPath()));
     }
     EXPECT_THAT(VisitedNonBrokenSymlinks, UnorderedElementsAre("b", "d"));
     VisitedNonBrokenSymlinks.clear();

     EXPECT_THAT(VisitedBrokenSymlinks, UnorderedElementsAre("a", "c", "e"));
     VisitedBrokenSymlinks.clear();

     // Broken symbol links are expected to throw an error.
     for (fs::RecursiveDirectoryIterator i(
         Twine(TestDirectory) + "/symlink", ec), e; i != e; i.increment(ec)) {
       ASSERT_NO_ERROR(ec);
       if (i->getStatus().getError() ==
           std::make_error_code(std::errc::no_such_file_or_directory)) {
         VisitedBrokenSymlinks.push_back(polar::fs::path::filename(i->getPath()));
         continue;
       }
       VisitedNonBrokenSymlinks.push_back(polar::fs::path::filename(i->getPath()));
     }
     EXPECT_THAT(VisitedNonBrokenSymlinks,
                 UnorderedElementsAre("b", "bb", "d", "da", "dd", "ddd", "ddd"));
     VisitedNonBrokenSymlinks.clear();

     EXPECT_THAT(VisitedBrokenSymlinks,
                 UnorderedElementsAre("a", "ba", "bc", "c", "e"));
     VisitedBrokenSymlinks.clear();

     for (fs::RecursiveDirectoryIterator i(
         Twine(TestDirectory) + "/symlink", ec, /*follow_symlinks=*/false), e;
          i != e; i.increment(ec)) {
       ASSERT_NO_ERROR(ec);
       if (i->getStatus().getError() ==
           std::make_error_code(std::errc::no_such_file_or_directory)) {
         VisitedBrokenSymlinks.push_back(polar::fs::path::filename(i->getPath()));
         continue;
       }
       VisitedNonBrokenSymlinks.push_back(polar::fs::path::filename(i->getPath()));
     }
     EXPECT_THAT(VisitedNonBrokenSymlinks,
                 UnorderedElementsAreArray({"a", "b", "ba", "bb", "bc", "c", "d",
                                            "da", "dd", "ddd", "e"}));
     VisitedNonBrokenSymlinks.clear();

     EXPECT_THAT(VisitedBrokenSymlinks, UnorderedElementsAre());
     VisitedBrokenSymlinks.clear();

     ASSERT_NO_ERROR(fs::remove_directories(Twine(TestDirectory) + "/symlink"));
}
#endif

TEST_F(FileSystemTest, testRemove)
{
   SmallString<64> BaseDir;
   SmallString<64> Paths[4];
   int fds[4];
   ASSERT_NO_ERROR(fs::create_unique_directory("fs_remove", BaseDir));

   ASSERT_NO_ERROR(fs::create_directories(Twine(BaseDir) + "/foo/bar/baz"));
   ASSERT_NO_ERROR(fs::create_directories(Twine(BaseDir) + "/foo/bar/buzz"));
   ASSERT_NO_ERROR(fs::create_unique_file(
                      Twine(BaseDir) + "/foo/bar/baz/%%%%%%.tmp", fds[0], Paths[0]));
   ASSERT_NO_ERROR(fs::create_unique_file(
                      Twine(BaseDir) + "/foo/bar/baz/%%%%%%.tmp", fds[1], Paths[1]));
   ASSERT_NO_ERROR(fs::create_unique_file(
                      Twine(BaseDir) + "/foo/bar/buzz/%%%%%%.tmp", fds[2], Paths[2]));
   ASSERT_NO_ERROR(fs::create_unique_file(
                      Twine(BaseDir) + "/foo/bar/buzz/%%%%%%.tmp", fds[3], Paths[3]));

   for (int fd : fds)
      ::close(fd);

   EXPECT_TRUE(fs::exists(Twine(BaseDir) + "/foo/bar/baz"));
   EXPECT_TRUE(fs::exists(Twine(BaseDir) + "/foo/bar/buzz"));
   EXPECT_TRUE(fs::exists(Paths[0]));
   EXPECT_TRUE(fs::exists(Paths[1]));
   EXPECT_TRUE(fs::exists(Paths[2]));
   EXPECT_TRUE(fs::exists(Paths[3]));

   ASSERT_NO_ERROR(fs::remove_directories("D:/footest"));

   ASSERT_NO_ERROR(fs::remove_directories(BaseDir));
   ASSERT_FALSE(fs::exists(BaseDir));
}

#ifdef _WIN32
TEST_F(FileSystemTest, CarriageReturn) {
   SmallString<128> FilePathname(TestDirectory);
   std::error_code errorCode;
   fs::path::append(FilePathname, "test");

   {
      raw_fd_ostream File(FilePathname, errorCode, fs::F_Text);
      ASSERT_NO_ERROR(errorCode);
      File << '\n';
   }
   {
      auto Buf = MemoryBuffer::getFile(FilePathname.getStr());
      EXPECT_TRUE((bool)Buf);
      EXPECT_EQ(Buf.get()->getBuffer(), "\r\n");
   }

   {
      raw_fd_ostream File(FilePathname, errorCode, fs::F_None);
      ASSERT_NO_ERROR(errorCode);
      File << '\n';
   }
   {
      auto Buf = MemoryBuffer::getFile(FilePathname.getStr());
      EXPECT_TRUE((bool)Buf);
      EXPECT_EQ(Buf.get()->getBuffer(), "\n");
   }
   ASSERT_NO_ERROR(fs::remove(Twine(FilePathname)));
}
#endif

TEST_F(FileSystemTest, testResize)
{
   int FD;
   SmallString<64> TempPath;
   ASSERT_NO_ERROR(fs::create_temporary_file("prefix", "temp", FD, TempPath));
   ASSERT_NO_ERROR(fs::resize_file(FD, 123));
   fs::FileStatus Status;
   ASSERT_NO_ERROR(fs::status(FD, Status));
   ASSERT_EQ(Status.getSize(), 123U);
   ::close(FD);
   ASSERT_NO_ERROR(fs::remove(TempPath));
}

TEST_F(FileSystemTest, testMD5)
{
   int FD;
   SmallString<64> TempPath;
   ASSERT_NO_ERROR(fs::create_temporary_file("prefix", "temp", FD, TempPath));
   StringRef Data("abcdefghijklmnopqrstuvwxyz");
   ASSERT_EQ(write(FD, Data.getData(), Data.size()), static_cast<ssize_t>(Data.size()));
   lseek(FD, 0, SEEK_SET);
   auto Hash = fs::md5_contents(FD);
   ::close(FD);
   ASSERT_NO_ERROR(Hash.getError());

   EXPECT_STREQ("c3fcd3d76192e4007dfb496cca67e13b", Hash->getDigest().getCStr());
}

TEST_F(FileSystemTest, testFileMapping)
{
   // Create a temp file.
   int FileDescriptor;
   SmallString<64> TempPath;
   ASSERT_NO_ERROR(
            fs::create_temporary_file("prefix", "temp", FileDescriptor, TempPath));
   unsigned Size = 4096;
   ASSERT_NO_ERROR(fs::resize_file(FileDescriptor, Size));

   // Map in temp file and add some content
   std::error_code errorCode;
   StringRef Val("hello there");
   {
      fs::MappedFileRegion mfr(FileDescriptor,
                               fs::MappedFileRegion::readwrite, Size, 0, errorCode);
      ASSERT_NO_ERROR(errorCode);
      std::copy(Val.begin(), Val.end(), mfr.getData());
      // Explicitly add a 0.
      mfr.getData()[Val.size()] = 0;
      // Unmap temp file
   }
   ASSERT_EQ(close(FileDescriptor), 0);

   // Map it back in read-only
   {
      int FD;
      errorCode = fs::open_file_for_read(Twine(TempPath), FD);
      ASSERT_NO_ERROR(errorCode);
      fs::MappedFileRegion mfr(FD, fs::MappedFileRegion::readonly, Size, 0, errorCode);
      ASSERT_NO_ERROR(errorCode);

      // Verify content
      EXPECT_EQ(StringRef(mfr.getConstData()), Val);

      // Unmap temp file
      fs::MappedFileRegion m(FD, fs::MappedFileRegion::readonly, Size, 0, errorCode);
      ASSERT_NO_ERROR(errorCode);
      ASSERT_EQ(close(FD), 0);
   }
   ASSERT_NO_ERROR(fs::remove(TempPath));
}

TEST(PathTest, testNormalizePath)
{
   using TestTuple = std::tuple<const char *, const char *, const char *>;
   std::vector<TestTuple> Tests;
   Tests.emplace_back("a", "a", "a");
   Tests.emplace_back("a/b", "a\\b", "a/b");
   Tests.emplace_back("a\\b", "a\\b", "a/b");
   Tests.emplace_back("a\\\\b", "a\\\\b", "a\\\\b");
   Tests.emplace_back("\\a", "\\a", "/a");
   Tests.emplace_back("a\\", "a\\", "a/");

   for (auto &T : Tests) {
      SmallString<64> Win(std::get<0>(T));
      SmallString<64> Posix(Win);
      fs::path::native(Win, fs::path::Style::windows);
      fs::path::native(Posix, fs::path::Style::posix);
      EXPECT_EQ(std::get<1>(T), Win);
      EXPECT_EQ(std::get<2>(T), Posix);
   }

#if defined(_WIN32)
   SmallString<64> PathHome;
   fs::path::home_directory(PathHome);

   const char *fs::path7a = "~/aaa";
   SmallString<64> Path7(Path7a);
   fs::path::native(Path7);
   EXPECT_TRUE(Path7.endsWith("\\aaa"));
   EXPECT_TRUE(Path7.startswith(PathHome));
   EXPECT_EQ(Path7.size(), PathHome.size() + strlen(Path7a + 1));

   const char *fs::path8a = "~";
   SmallString<64> Path8(Path8a);
   fs::path::native(Path8);
   EXPECT_EQ(Path8, PathHome);

   const char *fs::path9a = "~aaa";
   SmallString<64> Path9(Path9a);
   fs::path::native(Path9);
   EXPECT_EQ(Path9, "~aaa");

   const char *fs::path10a = "aaa/~/b";
   SmallString<64> Path10(Path10a);
   fs::path::native(Path10);
   EXPECT_EQ(Path10, "aaa\\~\\b");
#endif
}

TEST(PathTest, testRemoveLeadingDotSlash)
{
   StringRef Path1("././/foolz/wat");
   StringRef Path2("./////");

   Path1 = fs::path::remove_leading_dotslash(Path1);
   EXPECT_EQ(Path1, "foolz/wat");
   Path2 = fs::path::remove_leading_dotslash(Path2);
   EXPECT_EQ(Path2, "");
}

static std::string remove_dots(StringRef path, bool remove_dot_dot,
                               fs::path::Style style) {
   SmallString<256> buffer(path);
   fs::path::remove_dots(buffer, remove_dot_dot, style);
   return buffer.getStr();
}

TEST(PathTest, testRemoveDots)
{
   EXPECT_EQ("foolz\\wat",
             remove_dots(".\\.\\\\foolz\\wat", false, fs::path::Style::windows));
   EXPECT_EQ("", remove_dots(".\\\\\\\\\\", false, fs::path::Style::windows));

   EXPECT_EQ("a\\..\\b\\c",
             remove_dots(".\\a\\..\\b\\c", false, fs::path::Style::windows));
   EXPECT_EQ("b\\c", remove_dots(".\\a\\..\\b\\c", true, fs::path::Style::windows));
   EXPECT_EQ("c", remove_dots(".\\.\\c", true, fs::path::Style::windows));
   EXPECT_EQ("..\\a\\c",
             remove_dots("..\\a\\b\\..\\c", true, fs::path::Style::windows));
   EXPECT_EQ("..\\..\\a\\c",
             remove_dots("..\\..\\a\\b\\..\\c", true, fs::path::Style::windows));

   SmallString<64> Path1(".\\.\\c");
   EXPECT_TRUE(fs::path::remove_dots(Path1, true, fs::path::Style::windows));
   EXPECT_EQ("c", Path1);

   EXPECT_EQ("foolz/wat",
             remove_dots("././/foolz/wat", false, fs::path::Style::posix));
   EXPECT_EQ("", remove_dots("./////", false, fs::path::Style::posix));

   EXPECT_EQ("a/../b/c", remove_dots("./a/../b/c", false, fs::path::Style::posix));
   EXPECT_EQ("b/c", remove_dots("./a/../b/c", true, fs::path::Style::posix));
   EXPECT_EQ("c", remove_dots("././c", true, fs::path::Style::posix));
   EXPECT_EQ("../a/c", remove_dots("../a/b/../c", true, fs::path::Style::posix));
   EXPECT_EQ("../../a/c",
             remove_dots("../../a/b/../c", true, fs::path::Style::posix));
   EXPECT_EQ("/a/c", remove_dots("/../../a/c", true, fs::path::Style::posix));
   EXPECT_EQ("/a/c",
             remove_dots("/../a/b//../././/c", true, fs::path::Style::posix));

   SmallString<64> Path2("././c");
   EXPECT_TRUE(fs::path::remove_dots(Path2, true, fs::path::Style::posix));
   EXPECT_EQ("c", Path2);
}

TEST(PathTest, testReplacePathPrefix)
{
   SmallString<64> Path1("/foo");
   SmallString<64> Path2("/old/foo");
   SmallString<64> OldPrefix("/old");
   SmallString<64> NewPrefix("/new");
   SmallString<64> NewPrefix2("/longernew");
   SmallString<64> EmptyPrefix("");

   SmallString<64> Path = Path1;
   fs::path::replace_path_prefix(Path, OldPrefix, NewPrefix);
   EXPECT_EQ(Path, "/foo");
   Path = Path2;
   fs::path::replace_path_prefix(Path, OldPrefix, NewPrefix);
   EXPECT_EQ(Path, "/new/foo");
   Path = Path2;
   fs::path::replace_path_prefix(Path, OldPrefix, NewPrefix2);
   EXPECT_EQ(Path, "/longernew/foo");
   Path = Path1;
   fs::path::replace_path_prefix(Path, EmptyPrefix, NewPrefix);
   EXPECT_EQ(Path, "/new/foo");
   Path = Path2;
   fs::path::replace_path_prefix(Path, OldPrefix, EmptyPrefix);
   EXPECT_EQ(Path, "/foo");
}

TEST_F(FileSystemTest, testOpenFileForRead)
{
   // Create a temp file.
   int FileDescriptor;
   SmallString<64> TempPath;
   ASSERT_NO_ERROR(
            fs::create_temporary_file("prefix", "temp", FileDescriptor, TempPath));
   fs::FileRemover Cleanup(TempPath);

   // Make sure it exists.
   ASSERT_TRUE(fs::exists(Twine(TempPath)));

   // Open the file for read
   int FileDescriptor2;
   SmallString<64> ResultPath;
   ASSERT_NO_ERROR(
            fs::open_file_for_read(Twine(TempPath), FileDescriptor2, fs::OF_None,
                                   &ResultPath))

         // If we succeeded, check that the paths are the same (modulo case):
         if (!ResultPath.empty()) {
      // The paths returned by create_temporary_file and getPathFromOpenFD
      // should reference the same file on disk.
      fs::UniqueId D1, D2;
      ASSERT_NO_ERROR(fs::get_unique_id(Twine(TempPath), D1));
      ASSERT_NO_ERROR(fs::get_unique_id(Twine(ResultPath), D2));
      ASSERT_EQ(D1, D2);
   }

   ::close(FileDescriptor);
   ::close(FileDescriptor2);

#ifdef _WIN32
   // Since Windows Vista, file access time is not updated by default.
   // This is instead updated manually by open_file_for_read.
   // https://blogs.technet.microsoft.com/filecab/2006/11/07/disabling-last-access-time-in-windows-vista-to-improve-ntfs-performance/
   // This part of the unit test is Windows specific as the updating of
   // access times can be disabled on Linux using /etc/fstab.

   // Set access time to UNIX epoch.
   ASSERT_NO_ERROR(fs::open_file_for_write(Twine(TempPath), FileDescriptor,
                                           fs::CD_OpenExisting));
   TimePoint<> Epoch(std::chrono::milliseconds(0));
   ASSERT_NO_ERROR(fs::set_last_access_and_modification_time(FileDescriptor, Epoch));
   ::close(FileDescriptor);

   // Open the file and ensure access time is updated, when forced.
   ASSERT_NO_ERROR(fs::open_file_for_read(Twine(TempPath), FileDescriptor,
                                          fs::OF_UpdateAtime, &ResultPath));

   fs::FileStatus Status;
   ASSERT_NO_ERROR(fs::status(FileDescriptor, Status));
   auto FileAccessTime = Status.getLastAccessedTime();

   ASSERT_NE(Epoch, FileAccessTime);
   ::close(FileDescriptor);

   // Ideally this test would include a case when ATime is not forced to update,
   // however the expected behaviour will differ depending on the configuration
   // of the Windows file system.
#endif
}

static void createFileWithData(const Twine &Path, bool ShouldExistBefore,
                               fs::CreationDisposition Disp, StringRef Data)
{
   int FD;
   ASSERT_EQ(ShouldExistBefore, fs::exists(Path));
   ASSERT_NO_ERROR(fs::open_file_for_write(Path, FD, Disp));
   FileDescriptorCloser Closer(FD);
   ASSERT_TRUE(fs::exists(Path));

   ASSERT_EQ(Data.size(), (size_t)write(FD, Data.getData(), Data.size()));
}

static void verifyFileContents(const Twine &Path, StringRef Contents)
{
   auto Buffer = MemoryBuffer::getFile(Path);
   ASSERT_TRUE((bool)Buffer);
   StringRef Data = Buffer.get()->getBuffer();
   ASSERT_EQ(Data, Contents);
}

TEST_F(FileSystemTest, testCreateNew)
{
   int FD;
   std::optional<FileDescriptorCloser> Closer;

   // Succeeds if the file does not exist.
   ASSERT_FALSE(fs::exists(NonExistantFile));
   ASSERT_NO_ERROR(fs::open_file_for_write(NonExistantFile, FD, fs::CD_CreateNew));
   ASSERT_TRUE(fs::exists(NonExistantFile));

   fs::FileRemover Cleanup(NonExistantFile);
   Closer.emplace(FD);

   // And creates a file of size 0.
   fs::FileStatus Status;
   ASSERT_NO_ERROR(fs::status(FD, Status));
   EXPECT_EQ(0ULL, Status.getSize());

   // Close this first, before trying to re-open the file.
   Closer.reset();

   // But fails if the file does exist.
   ASSERT_ERROR(fs::open_file_for_write(NonExistantFile, FD, fs::CD_CreateNew));
}

TEST_F(FileSystemTest, testCreateAlways)
{
   int FD;
   std::optional<FileDescriptorCloser> Closer;

   // Succeeds if the file does not exist.
   ASSERT_FALSE(fs::exists(NonExistantFile));
   ASSERT_NO_ERROR(
            fs::open_file_for_write(NonExistantFile, FD, fs::CD_CreateAlways));

   Closer.emplace(FD);

   ASSERT_TRUE(fs::exists(NonExistantFile));

   fs::FileRemover Cleanup(NonExistantFile);

   // And creates a file of size 0.
   uint64_t FileSize;
   ASSERT_NO_ERROR(fs::file_size(NonExistantFile, FileSize));
   ASSERT_EQ(0ULL, FileSize);

   // If we write some data to it re-create it with CreateAlways, it succeeds and
   // truncates to 0 bytes.
   ASSERT_EQ(4, write(FD, "Test", 4));

   Closer.reset();

   ASSERT_NO_ERROR(fs::file_size(NonExistantFile, FileSize));
   ASSERT_EQ(4ULL, FileSize);

   ASSERT_NO_ERROR(
            fs::open_file_for_write(NonExistantFile, FD, fs::CD_CreateAlways));
   Closer.emplace(FD);
   ASSERT_NO_ERROR(fs::file_size(NonExistantFile, FileSize));
   ASSERT_EQ(0ULL, FileSize);
}

TEST_F(FileSystemTest, testOpenExisting)
{
   int FD;

   // Fails if the file does not exist.
   ASSERT_FALSE(fs::exists(NonExistantFile));
   ASSERT_ERROR(fs::open_file_for_write(NonExistantFile, FD, fs::CD_OpenExisting));
   ASSERT_FALSE(fs::exists(NonExistantFile));

   // Make a dummy file now so that we can try again when the file does exist.
   createFileWithData(NonExistantFile, false, fs::CD_CreateNew, "Fizz");
   fs::FileRemover Cleanup(NonExistantFile);
   uint64_t FileSize;
   ASSERT_NO_ERROR(fs::file_size(NonExistantFile, FileSize));
   ASSERT_EQ(4ULL, FileSize);

   // If we re-create it with different data, it overwrites rather than
   // appending.
   createFileWithData(NonExistantFile, true, fs::CD_OpenExisting, "Buzz");
   verifyFileContents(NonExistantFile, "Buzz");
}

TEST_F(FileSystemTest, testOpenAlways)
{
   // Succeeds if the file does not exist.
   createFileWithData(NonExistantFile, false, fs::CD_OpenAlways, "Fizz");
   fs::FileRemover Cleanup(NonExistantFile);
   uint64_t FileSize;
   ASSERT_NO_ERROR(fs::file_size(NonExistantFile, FileSize));
   ASSERT_EQ(4ULL, FileSize);

   // Now re-open it and write again, verifying the contents get over-written.
   createFileWithData(NonExistantFile, true, fs::CD_OpenAlways, "Bu");
   verifyFileContents(NonExistantFile, "Buzz");
}

TEST_F(FileSystemTest, testAppendSetsCorrectFileOffset)
{
   fs::CreationDisposition Disps[] = {fs::CD_CreateAlways, fs::CD_OpenAlways,
                                      fs::CD_OpenExisting};

   // Write some data and re-open it with every possible disposition (this is a
   // hack that shouldn't work, but is left for compatibility.  F_Append
   // overrides
   // the specified disposition.
   for (fs::CreationDisposition Disp : Disps) {
      int FD;
      std::optional<FileDescriptorCloser> Closer;

      createFileWithData(NonExistantFile, false, fs::CD_CreateNew, "Fizz");

      fs::FileRemover Cleanup(NonExistantFile);

      uint64_t FileSize;
      ASSERT_NO_ERROR(fs::file_size(NonExistantFile, FileSize));
      ASSERT_EQ(4ULL, FileSize);
      ASSERT_NO_ERROR(
               fs::open_file_for_write(NonExistantFile, FD, Disp, fs::OF_Append));
      Closer.emplace(FD);
      ASSERT_NO_ERROR(fs::file_size(NonExistantFile, FileSize));
      ASSERT_EQ(4ULL, FileSize);

      ASSERT_EQ(4, write(FD, "Buzz", 4));
      Closer.reset();

      verifyFileContents(NonExistantFile, "FizzBuzz");
   }
}

static void verifyRead(int FD, StringRef Data, bool ShouldSucceed)
{
   std::vector<char> Buffer;
   Buffer.resize(Data.size());
   int result = ::read(FD, Buffer.data(), Buffer.size());
   if (ShouldSucceed) {
      ASSERT_EQ((size_t)result, Data.size());
      ASSERT_EQ(Data, StringRef(Buffer.data(), Buffer.size()));
   } else {
      ASSERT_EQ(-1, result);
      ASSERT_EQ(EBADF, errno);
   }
}

static void verifyWrite(int FD, StringRef Data, bool ShouldSucceed)
{
   int result = ::write(FD, Data.getData(), Data.size());
   if (ShouldSucceed)
      ASSERT_EQ((size_t)result, Data.size());
   else {
      ASSERT_EQ(-1, result);
      ASSERT_EQ(EBADF, errno);
   }
}

TEST_F(FileSystemTest, testReadOnlyFileCantWrite)
{
   createFileWithData(NonExistantFile, false, fs::CD_CreateNew, "Fizz");
   fs::FileRemover Cleanup(NonExistantFile);

   int FD;
   ASSERT_NO_ERROR(fs::open_file_for_read(NonExistantFile, FD));
   FileDescriptorCloser Closer(FD);

   verifyWrite(FD, "Buzz", false);
   verifyRead(FD, "Fizz", true);
}

TEST_F(FileSystemTest, testWriteOnlyFileCantRead)
{
   createFileWithData(NonExistantFile, false, fs::CD_CreateNew, "Fizz");
   fs::FileRemover Cleanup(NonExistantFile);

   int FD;
   ASSERT_NO_ERROR(
            fs::open_file_for_write(NonExistantFile, FD, fs::CD_OpenExisting));
   FileDescriptorCloser Closer(FD);
   verifyRead(FD, "Fizz", false);
   verifyWrite(FD, "Buzz", true);
}

TEST_F(FileSystemTest, testReadWriteFileCanReadOrWrite)
{
   createFileWithData(NonExistantFile, false, fs::CD_CreateNew, "Fizz");
   fs::FileRemover Cleanup(NonExistantFile);

   int FD;
   ASSERT_NO_ERROR(fs::open_file_for_read_write(NonExistantFile, FD,
                                                fs::CD_OpenExisting, fs::OF_None));
   FileDescriptorCloser Closer(FD);
   verifyRead(FD, "Fizz", true);
   verifyWrite(FD, "Buzz", true);
}

TEST_F(FileSystemTest, testSetCurrentPath)
{
   SmallString<128> path;

   ASSERT_NO_ERROR(fs::current_path(path));
   ASSERT_NE(TestDirectory, path);

   struct RestorePath {
      SmallString<128> path;
      RestorePath(const SmallString<128> &path) : path(path) {}
      ~RestorePath() { fs::set_current_path(path); }
   } restore_path(path);

   ASSERT_NO_ERROR(fs::set_current_path(TestDirectory));

   ASSERT_NO_ERROR(fs::current_path(path));

   fs::UniqueId D1, D2;
   ASSERT_NO_ERROR(fs::get_unique_id(TestDirectory, D1));
   ASSERT_NO_ERROR(fs::get_unique_id(path, D2));
   ASSERT_EQ(D1, D2) << "D1: " << TestDirectory.getCStr() << "\nD2: " << path.getCStr();
}

TEST_F(FileSystemTest, testGetPermissions)
{
   int FD;
   SmallString<64> TempPath;
   ASSERT_NO_ERROR(fs::create_temporary_file("prefix", "temp", FD, TempPath));
   fs::FileRemover Cleanup(TempPath);

   // Make sure it exists.
   ASSERT_TRUE(fs::exists(Twine(TempPath)));

   auto CheckgetPermissions = [&](fs::Permission Expected) {
      OptionalError<fs::Permission> Actual = fs::get_permissions(TempPath);
      return Actual && *Actual == Expected;
   };

   std::error_code NoError;
   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_all), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_read | fs::all_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_read | fs::all_exe));

#if defined(_WIN32)
   fs::Permission ReadOnly = fs::all_read | fs::all_exe;
   EXPECT_EQ(fs::set_permissions(TempPath, fs::no_Permission), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_all), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_all), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_all), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::set_uid_on_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::set_gid_on_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::sticky_bit), NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::set_uid_on_exe |
                                 fs::set_gid_on_exe |
                                 fs::sticky_bit),
             NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, ReadOnly | fs::set_uid_on_exe |
                                 fs::set_gid_on_exe |
                                 fs::sticky_bit),
             NoError);
   EXPECT_TRUE(CheckgetPermissions(ReadOnly));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::set_perms), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_all));
#else
   EXPECT_EQ(fs::set_permissions(TempPath, fs::no_perms), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::no_perms));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::owner_read));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::owner_write));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::owner_exe));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::owner_all), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::owner_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::group_read));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::group_write));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::group_exe));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::group_all), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::group_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::others_read));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::others_write));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::others_exe));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::others_all), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::others_all));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_read), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_read));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_write), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_write));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_exe));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::set_uid_on_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::set_uid_on_exe));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::set_gid_on_exe), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::set_gid_on_exe));

   // Modern BSDs require root to set the sticky bit on files.
#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__)
   EXPECT_EQ(fs::set_permissions(TempPath, fs::sticky_bit), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::sticky_bit));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::set_uid_on_exe |
                                 fs::set_gid_on_exe |
                                 fs::sticky_bit),
             NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::set_uid_on_exe | fs::set_gid_on_exe |
                                   fs::sticky_bit));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_read | fs::set_uid_on_exe |
                                 fs::set_gid_on_exe |
                                 fs::sticky_bit),
             NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_read | fs::set_uid_on_exe |
                                   fs::set_gid_on_exe | fs::sticky_bit));

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_perms), NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_perms));
#endif // !FreeBSD && !NetBSD && !OpenBSD

   EXPECT_EQ(fs::set_permissions(TempPath, fs::all_perms & ~fs::sticky_bit),
             NoError);
   EXPECT_TRUE(CheckgetPermissions(fs::all_perms & ~fs::sticky_bit));
#endif
}

} // anonymous namespace
