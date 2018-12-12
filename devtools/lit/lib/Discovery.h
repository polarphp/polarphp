// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#ifndef POLAR_DEVLTOOLS_LIT_DISCOVERY_H
#define POLAR_DEVLTOOLS_LIT_DISCOVERY_H

#include <optional>
#include <string>
#include <list>
#include <map>
#include "Test.h"
#include "ForwardDefs.h"

namespace polar {
namespace lit {

std::optional<std::string> choose_config_file_from_dir(const std::string &dir,
                                                       const std::list<std::string> &configNames);
std::optional<std::string> dir_contains_test_suite(const std::string &path,
                                                   LitConfigPointer litConfig);
TestSuitSearchResult get_test_suite(std::string item, LitConfigPointer litConfig,
                                    std::map<std::string, TestSuitSearchResult> &cache);
TestingConfigPointer get_local_config(TestSuitePointer testSuite, LitConfigPointer litConfig,
                                      const std::list<std::string> &pathInSuite);
std::tuple<TestSuitePointer, TestList> get_tests(const std::string &path, LitConfigPointer litConfig,
                                                 std::map<std::string, TestSuitSearchResult> &cache);
TestList get_tests_in_suite(TestSuitePointer testSuite, LitConfigPointer litConfig,
                            const std::list<std::string> &pathInSuite,
                            std::map<std::string, TestSuitSearchResult> &cache);
TestList find_tests_for_inputs(LitConfigPointer litConfig, const std::list<std::string> &inputs);
std::list<std::shared_ptr<LitTestCase>> load_test_suite(const std::list<std::string> &inputs);

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_DISCOVERY_H
