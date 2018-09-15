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
         std::string base = pathInfo.parent_path();
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

} // lit
} // polar
