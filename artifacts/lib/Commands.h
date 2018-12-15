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
#include <iostream>

namespace CLI {
class App;
} // CLI


namespace polar {

using CLI::App;

void print_polar_version();
POLAR_DECL_EXPORT int php_lint_script(zend_file_handle *file);
void setup_init_entries_commands(std::string &iniEntries);
int dispatch_cli_command(App &cmdParser, int argc, char *argv[]);

} // polar

#endif // POLARPHP_ARTIFACTS_COMMANDS_H
