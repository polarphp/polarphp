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

namespace polar {
namespace lit {

class LitConfig;

std::optional<std::string> choose_config_file_from_dir(const std::string &dir,
                                                       const std::list<std::string> &configNames);
std::optional<std::string> dir_contains_test_suite(const std::string &path,
                                                   const LitConfig &config);
void get_test_suite();
void get_local_config();
void get_tests();
void get_tests_in_suite();
void find_tests_for_inputs();
void load_test_suite();

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_DISCOVERY_H
