// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#include "llvm/ADT/StringRef.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/Config.h"

#include "CLI/CLI.hpp"
#include "lib/Defs.h"
#include "lib/Commands.h"

#include <vector>
#include <string>

using llvm::StringRef;

void setup_command_opts(CLI::App &parser);

int sg_exitStatus = 0;
std::string sg_errorMsg;
bool sg_showVersion;
bool sg_showNgInfo;
bool sg_interactive;
bool sg_generateExtendInfo;
bool sg_ignoreIni;
bool sg_syntaxCheck;
bool sg_showModulesInfo;
bool sg_hideExternArgs;
bool sg_showIniCfg;
bool sg_stripCode;
std::string sg_configPath{};
std::string sg_scriptFile{};
std::string sg_codeWithoutPhpTags{};
std::string sg_beginCode{};
std::string sg_everyLineExecCode{};
std::string sg_endCode{};
std::vector<std::string> sg_zendExtensionFilenames{};
std::vector<std::string> sg_scriptArgs{};
std::vector<std::string> sg_defines{};
std::string sg_reflectWhat{};

int main(int argc, char *argv[])
{
   CLI::App cmdParser;
   cmdParser.formatter(std::make_shared<polar::PhpOptFormatter>());
   setup_command_opts(cmdParser);
   CLI11_PARSE(cmdParser, argc, argv);
   /// check command semantic error
   if (sg_exitStatus != 0) {
      std::cerr << sg_errorMsg << std::endl;
      exit(sg_exitStatus);
   }
   return 0;
}

void setup_command_opts(CLI::App &parser)
{
   /// order sensitive
   parser.add_option("-c, --config", sg_configPath, "Look for php.yaml file in this directory.")->type_name("<path>|<file>");
   parser.add_flag("-n", sg_ignoreIni, "No configuration (ini) files will be used");
   parser.add_option("-d", sg_defines, "Define INI entry foo with value 'bar'.")->type_name("foo[=bar]");
   parser.add_flag("-e, --generate-extend-info", sg_generateExtendInfo, "Generate extended information for debugger/profiler.");
   parser.add_flag("-m, --modules-info", sg_showModulesInfo, "Show compiled in modules.");

   parser.add_flag("-i, --ng-info", sg_showNgInfo, "Show polarphp info.");
   parser.add_flag("-v, --version", sg_showVersion, "Show polarphp version info.");
   parser.add_flag("-a, --interactive", polar::interactive_opt_setter, "Run interactively PHP shell.");
   parser.add_option("-F", CLI::callback_t(polar::everyline_exec_script_filename_opt_setter), "Parse and execute <file> for every input line.")->type_name("<file>");
   parser.add_option("-f", CLI::callback_t(polar::script_file_opt_setter), "Parse and execute <file>.")->type_name("<file>");
   parser.add_flag("-l, --lint", polar::lint_opt_setter, "Syntax check only (lint)");
   parser.add_option("-r",CLI::callback_t(polar::code_without_php_tags_opt_setter), "Run PHP <code> without using script tags <?..?>.")->type_name("<code>");
   parser.add_option("-R", CLI::callback_t(polar::everyline_code_opt_setter), "Run PHP <code> for every input line.")->type_name("<code>");
   parser.add_option("-B", CLI::callback_t(polar::begin_code_opt_setter), "Run PHP <begin_code> before processing input lines.")->type_name("<begin_code>");
   parser.add_option("-E", CLI::callback_t(polar::end_code_opt_setter), "Run PHP <end_code> after processing all input lines.")->type_name("<end_code>");
   parser.add_flag("-w",  polar::strip_code_opt_setter, "Output source with stripped comments and whitespace.");
   parser.add_option("-z", sg_zendExtensionFilenames, "Load Zend extension <file>.")->type_name("<file>");
   parser.add_flag("-H", sg_hideExternArgs, "Hide any passed arguments from external tools.");

   parser.add_option("--rf", CLI::callback_t(polar::reflection_func_opt_setter), "Show information about function <name>.")->type_name("<name>");
   parser.add_option("--rc", CLI::callback_t(polar::reflection_class_opt_setter), "Show information about class <name>.")->type_name("<name>");
   parser.add_option("--rm", CLI::callback_t(polar::reflection_extension_opt_setter), "Show information about extension <name>.")->type_name("<name>");
   parser.add_option("--rz", CLI::callback_t(polar::reflection_zend_extension_opt_setter), "Show information about Zend extension <name>.")->type_name("<name>");
   parser.add_option("--ri", CLI::callback_t(polar::reflection_ext_info_opt_setter), "Show configuration for extension <name>.")->type_name("<name>");
   parser.add_flag("--ini", polar::reflection_show_ini_cfg_opt_setter, "Show configuration file names.")->type_name("");

   parser.add_option("args", sg_scriptArgs, "Arguments passed to script. Use -- args when first argument.")->type_name("string");
}

int polar::EnableABIBreakingChecks = 1;
