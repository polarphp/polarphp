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

#ifndef POLAR_DEVLTOOLS_LIT_DISCOVERY_H
#define POLAR_DEVLTOOLS_LIT_DISCOVERY_H

#include <optional>
#include <string>
#include <list>
#include <map>

namespace polar {
namespace lit {

class TestSuite;

using TestSuitSearchResult = std::tuple<std::optional<TestSuite>, std::list<std::string>>;

class LitConfig;

std::optional<std::string> choose_config_file_from_dir(const std::string &dir,
                                                       const std::list<std::string> &configNames);
std::optional<std::string> dir_contains_test_suite(const std::string &path,
                                                   const LitConfig &config);
TestSuitSearchResult get_test_suite(std::string item, const LitConfig &config,
                                    std::map<std::string, TestSuitSearchResult> &cache);
void get_local_config(const std::string &ts, const std::string &pathInSuite,
                      const LitConfig &config, std::map<std::string, std::string> &cache);
void get_tests(const std::string &path, const LitConfig &config,
               std::map<std::string, std::string> &testSuiteCache,
               std::map<std::string, std::string> &localConfigCache);

void get_tests_in_suite(const std::string &ts, const std::string &pathInSuite,
                        std::map<std::string, std::string> &testSuiteCache,
                        std::map<std::string, std::string> &localConfigCache);
void find_tests_for_inputs(const LitConfig &config, const std::list<std::string> &inputs);
void load_test_suite(const std::list<std::string> &inputs);

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_DISCOVERY_H
