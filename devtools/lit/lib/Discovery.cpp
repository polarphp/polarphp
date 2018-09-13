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

#include "Discovery.h"
#include "LitConfig.h"
#include "Test.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace polar {
namespace lit {

std::optional<std::string> choose_config_file_from_dir(const std::string &dir,
                                                       const std::list<std::string> &configNames)
{
   for (const std::string &name : configNames) {
      fs::path p(dir);
      p /= name;
      if (fs::exists(p)) {
         return p;
      }
   }
   return std::nullopt;
}

std::optional<std::string> dir_contains_test_suite(const std::string &path,
                                                   const LitConfig &config)
{
   std::optional<std::string> cfgPath = choose_config_file_from_dir(path, config.getSiteConfigNames());
   if (!cfgPath.has_value()) {
      cfgPath = choose_config_file_from_dir(path, config.getConfigNames());
   }
   return cfgPath;
}

using TestSuitSearchResult = std::tuple<TestSuite, std::list<std::string>>;

namespace {

TestSuitSearchResult do_search_testsuit(const std::string &path,
                                        std::map<std::string, std::string> &cache)
{

}

TestSuitSearchResult search_testsuit(const std::string &path,
                                     std::map<std::string, TestSuitSearchResult> &cache)
{
   // Check for an already instantiated test suite.
   std::string realPath = fs::canonical(path);
   if (cache.find(realPath) == cache.end()) {
      cache[realPath] = do_search_testsuit(path, cache);
   }
   return cache.at(realPath);
}

} // anonymous namespace

void get_test_suite(const std::string &item, const LitConfig &config,
                    std::map<std::string, std::string> &cache)
{

}

} // lit
} // polar
