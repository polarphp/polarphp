// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#ifndef POLARPHP_ARTIFACTS_COMMANDS_H
#define POLARPHP_ARTIFACTS_COMMANDS_H

#include "ZendHeaders.h"
#include <string>
#include <vector>
#include <CLI/Option.hpp>

namespace CLI {
class App;
} // CLI

namespace polar {

using CLI::App;

void print_polar_version();
POLAR_DECL_EXPORT int php_lint_script(zend_file_handle *file);
void setup_init_entries_commands(const std::vector<std::string> defines, std::string &iniEntries);
int dispatch_cli_command();

void interactive_opt_setter(int count);
bool everyline_exec_script_filename_opt_setter(CLI::results_t res);
bool everyline_code_opt_setter(CLI::results_t res);
} // polar

#endif // POLARPHP_ARTIFACTS_COMMANDS_H
