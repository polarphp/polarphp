// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/19.

#ifndef POLARPHP_ARTIFACTS_COMMANDS_H
#define POLARPHP_ARTIFACTS_COMMANDS_H

#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

#include <string>
#include <vector>
#include <CLI/Option.hpp>
#include <CLI/Formatter.hpp>

namespace CLI {
class App;
} // CLI

namespace polar {

using CLI::App;

class PhpOptFormatter : public CLI::Formatter
{
public:
   std::string make_usage(const App *app, std::string name) const override;
   std::string make_group(std::string group, bool is_positional, std::vector<const CLI::Option *> opts) const;
private:
   static std::vector<std::string> sm_opsNames;
};

void print_polar_version();
POLAR_DECL_EXPORT int php_lint_script(zend_file_handle *file);
void setup_init_entries_commands(const std::vector<std::string> defines, std::string &iniEntries);
int dispatch_cli_command();

void interactive_opt_setter(int count);
bool everyline_exec_script_filename_opt_setter(CLI::results_t res);
bool script_file_opt_setter(CLI::results_t res);
void lint_opt_setter(int count);
bool code_without_php_tags_opt_setter(CLI::results_t res);
bool everyline_code_opt_setter(CLI::results_t res);
bool begin_code_opt_setter(CLI::results_t res);
bool end_code_opt_setter(CLI::results_t res);
void strip_code_opt_setter(int count);
bool reflection_func_opt_setter(CLI::results_t res);
bool reflection_class_opt_setter(CLI::results_t res);
bool reflection_extension_opt_setter(CLI::results_t res);
bool reflection_zend_extension_opt_setter(CLI::results_t res);
bool reflection_ext_info_opt_setter(CLI::results_t res);
void reflection_show_ini_cfg_opt_setter(int count);

} // polar

#endif // POLARPHP_ARTIFACTS_COMMANDS_H
