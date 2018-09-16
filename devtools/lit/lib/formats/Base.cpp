// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#include "Base.h"
#include "../Test.h"
#include "../Global.h"
#include "../Utils.h"
#include <filesystem>

namespace polar {
namespace lit {

namespace fs = std::filesystem;

std::list<std::shared_ptr<Test>>
FileBasedTest::getTestsInDirectory(TestSuitePointer testSuite,
                                   const std::list<std::string> &pathInSuite,
                                   LitConfigPointer litConfig,
                                   TestingConfigPointer localConfig)
{
   std::string sourcePath = testSuite->getSourcePath(pathInSuite);
   std::list<std::shared_ptr<Test>> tests;
   for(auto& entry: fs::directory_iterator(sourcePath)) {
      fs::path pathInfo = entry.path();
      // Ignore dot files and excluded tests.
      std::string filename = pathInfo.filename();
      const std::set<std::string> &excludes = localConfig->getExcludes();
      if (string_startswith(filename, ".") ||
          excludes.find(filename) != excludes.end()) {
         continue;
      }
      if (!fs::is_directory(pathInfo)) {
         std::string ext = pathInfo.extension();
         const std::set<std::string> &suffixes = localConfig->getSuffixes();
         if (suffixes.find(ext) != suffixes.end()) {
            std::list<std::string> curPaths = pathInSuite;
            curPaths.push_back(filename);
            tests.push_back(std::make_shared<Test>(testSuite, curPaths, localConfig));
         }
      }
   }
   return tests;
}

OneCommandPerFileTest::OneCommandPerFileTest(const std::string &command, const std::string &dir,
                                             bool recursive, const std::string &pattern,
                                             bool useTempInput)
   : m_command(command),
     m_dir(dir),
     m_recursive(recursive),
     m_pattern(pattern),
     m_useTempInput(useTempInput)
{

}

TestList
OneCommandPerFileTest::getTestsInDirectory(std::shared_ptr<TestSuite> testSuite,
                                           const std::list<std::string> &pathInSuite,
                                           LitConfigPointer litConfig,
                                           std::shared_ptr<TestingConfig> localConfig)
{
   TestList tests;
   std::string dir = m_dir;
   const std::set<std::string> &excludes = localConfig->getExcludes();
   if (dir.empty()) {
      dir = testSuite->getSourcePath(pathInSuite);
   }
   std::list<fs::path> fileInfos;
   if (!m_recursive) {
      for(auto &entry : fs::directory_iterator(dir)) {
         if (!fs::is_directory(entry.path())) {
            fileInfos.push_back(entry.path());
         }
      }
   } else {
      for(auto iter = fs::recursive_directory_iterator(dir);
          iter != fs::recursive_directory_iterator();
          ++iter) {

         const fs::path &fileInfo = iter->path();
         std::string filename = fileInfo.filename();
         if (filename == ".svn" ||
             filename == ".git" ||
             excludes.find(filename) != excludes.end()) {
            iter.disable_recursion_pending();
         }
         if (!fs::is_directory(fileInfo)) {
            fileInfos.push_back(fileInfo);
         }
      }
   }
   for (const fs::path &filePath : fileInfos) {
      const std::string &filename = filePath.filename();
      const std::string &fullPath = filePath.string();
      if (string_startswith(filePath.string(), ".") ||
          !std::regex_match(filename, m_pattern) ||
          excludes.find(filename) != excludes.end()) {
         continue;
      }
      std::string suffix = fullPath.substr(dir.size());
      // @TODO
      // need think about Windows
      if (suffix.at(0) == '/') {
         suffix = suffix.substr(1);
      }
      std::list<std::string> curPaths = pathInSuite;
      std::list<std::string> parts = split_string(suffix, '/');
      for (auto &item : parts) {
         curPaths.push_back(item);
      }
      TestPointer test = std::make_shared<Test>(
               testSuite, curPaths,
               localConfig);
      // FIXME: Hack?
      test->setSelfSourcePath(fullPath);
      tests.push_back(test);
   }
   return tests;
}

void OneCommandPerFileTest::createTempInput(std::FILE *temp, std::shared_ptr<Test> test)
{
   throw NotImplementedError("This is an abstract method.");
}

namespace {

std::string generate_tempfilename()
{
   std::string temp = std::tmpnam(nullptr);
   temp += ".cpp";
   return temp;
}

} // anonymous namespace



std::tuple<const ResultCode &, std::string>
OneCommandPerFileTest::execute(TestPointer test, LitConfigPointer litConfig)
{
   if (test->getConfig()->isUnsupported()) {
      return ExecResultTuple{UNSUPPORTED, "Test is unsupported"};
   }
   std::string cmd = m_command;
   // If using temp input, create a temporary file and hand it to the
   // subclass.
   std::string tempFilename;
   std::FILE * tempFile;
   if (m_useTempInput) {
      tempFilename = generate_tempfilename();
      tempFile = std::fopen(tempFilename.c_str(), "wb+");
      if (NULL == tempFile) {
         throw std::runtime_error(format_string("create temp file error, %s", "OneCommandPerFileTest::execute"));
      }
      register_temp_file(tempFile);
      createTempInput(tempFile, test);
      std::fflush(tempFile);
      cmd += " " + tempFilename;
   } else if (!test->getSelfSourcePath().empty()) {
      cmd += " " + test->getSelfSourcePath();
   } else {
      cmd += " " + test->getSourcePath();
   }
   std::tuple<int, std::string, std::string> ret = execute_command(cmd);
   int exitCode = std::get<0>(ret);
   std::string out = std::get<1>(ret);
   std::string err = std::get<2>(ret);
   std::string diags = out + err;
   trim_string(diags);
   if (0 == exitCode && diags.empty()) {
      ExecResultTuple{PASS, ""};
   }
   // Try to include some useful information.
   std::string report = format_string("Command : %s\n", cmd.c_str());
   if (m_useTempInput) {
      fseek(tempFile , 0, SEEK_SET);
      report += "Temporary File: " + tempFilename + "\n";
      char buffer[256];
      std::string tempFileContent;
      while (std::fread(buffer, 256, 256, tempFile) > 0) {
         tempFileContent += std::string(buffer);
      }
      report += "--\n" + tempFileContent + "--\n";
   }
   report += "Output:\n--\n" + diags + "--";
   return ExecResultTuple{FAIL, report};
}

} // lit
} // polar
