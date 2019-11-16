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

#include "Commands.h"
#include "Defs.h"

#include "polarphp/global/CompilerFeature.h"
#include "llvm/ADT/StringRef.h"

#include <iostream>
#include <vector>
#include <map>

/// defined in main.cpp
extern std::vector<std::string> sg_defines;
extern bool sg_showVersion;
extern bool sg_showNgInfo;
extern bool sg_interactive;
extern bool sg_generateExtendInfo;
extern bool sg_ignoreIni;
extern bool sg_syntaxCheck;
extern bool sg_showModulesInfo;
extern bool sg_hideExternArgs;
extern bool sg_showIniCfg;
extern bool sg_stripCode;
extern std::string sg_configPath;
extern std::string sg_scriptFile;
extern std::string sg_codeWithoutPhpTags;
extern std::string sg_beginCode;
extern std::string sg_everyLineExecCode;
extern std::string sg_endCode;
extern std::vector<std::string> sg_zendExtensionFilenames;
extern std::vector<std::string> sg_scriptArgs;
extern std::vector<std::string> sg_defines;
extern std::string sg_reflectWhat;
extern int sg_exitStatus;
extern std::string sg_errorMsg;

namespace polar {

using namespace runtime;

using llvm::StringRef;

namespace {
const char *PARAM_MODE_CONFLICT = "Either execute direct code, process stdin or use a file.";
ExecMode sg_behavior = ExecMode::Standard;
std::string sg_phpSelf = "";
}

void interactive_opt_setter(int)
{
   if (sg_behavior != ExecMode::Standard) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   sg_interactive = true;
}

bool everyline_exec_script_filename_opt_setter(CLI::results_t res)
{
   if (sg_behavior == ExecMode::ProcessStdin) {
      if (!sg_everyLineExecCode.empty() || !sg_scriptFile.empty()) {
         sg_exitStatus = 1;
         sg_errorMsg = "You can use -R or -F only once.";
         throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
      }
   } else if (sg_behavior != ExecMode::Standard) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   sg_behavior = ExecMode::ProcessStdin;
   return CLI::detail::lexical_cast(res[0], sg_scriptFile);
}

bool script_file_opt_setter(CLI::results_t res)
{
   if (sg_behavior == ExecMode::CliDirect || sg_behavior == ExecMode::ProcessStdin) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   } else if (!sg_scriptFile.empty()) {
      sg_exitStatus = 1;
      sg_errorMsg = "You can use -f only once.";
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   CLI::detail::lexical_cast(res[0], sg_scriptFile);
   return true;
}

void lint_opt_setter(int)
{
   if (sg_behavior == ExecMode::Standard) {
      return;
   }
   sg_syntaxCheck = true;
   sg_behavior = ExecMode::Lint;
}

bool code_without_php_tags_opt_setter(CLI::results_t res)
{
   if (sg_behavior == ExecMode::CliDirect) {
      if (!sg_codeWithoutPhpTags.empty() || !sg_scriptFile.empty()) {
         sg_exitStatus = 1;
         sg_errorMsg = "You can use -r only once.";
         throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
      }
   } else if(sg_behavior != ExecMode::Standard || sg_interactive) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   sg_behavior = ExecMode::CliDirect;
   return CLI::detail::lexical_cast(res[0], sg_codeWithoutPhpTags);
}

bool everyline_code_opt_setter(CLI::results_t res)
{
   if (sg_behavior == ExecMode::ProcessStdin) {
      if (!sg_everyLineExecCode.empty() || !sg_scriptFile.empty()) {
         sg_exitStatus = 1;
         sg_errorMsg = "You can use -R or -F only once.";
         throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
      }
   } else if (sg_behavior != ExecMode::Standard) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   sg_behavior = ExecMode::ProcessStdin;
   return CLI::detail::lexical_cast(res[0], sg_everyLineExecCode);
}

bool begin_code_opt_setter(CLI::results_t res)
{
   if (sg_behavior != ExecMode::Standard || sg_interactive) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   sg_behavior = ExecMode::ProcessStdin;
   return CLI::detail::lexical_cast(res[0], sg_beginCode);
}

bool end_code_opt_setter(CLI::results_t res)
{
   if (sg_behavior != ExecMode::Standard || sg_interactive) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   sg_behavior = ExecMode::ProcessStdin;
   return CLI::detail::lexical_cast(res[0], sg_endCode);
}

void strip_code_opt_setter(int)
{
   if (sg_behavior == ExecMode::CliDirect || sg_behavior == ExecMode::ProcessStdin) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(sg_errorMsg, sg_exitStatus);
   }
   sg_behavior = ExecMode::Strip;
   sg_stripCode = true;
}

bool reflection_func_opt_setter(CLI::results_t res)
{
   sg_behavior = ExecMode::ReflectionFunction;
   return CLI::detail::lexical_cast(res[0], sg_reflectWhat);
}

bool reflection_class_opt_setter(CLI::results_t res)
{
   sg_behavior = ExecMode::ReflectionClass;
   return CLI::detail::lexical_cast(res[0], sg_reflectWhat);
}

bool reflection_extension_opt_setter(CLI::results_t res)
{
   sg_behavior = ExecMode::ReflectionExtension;
   return CLI::detail::lexical_cast(res[0], sg_reflectWhat);
}

bool reflection_zend_extension_opt_setter(CLI::results_t res)
{
   sg_behavior = ExecMode::ReflectionZendExtension;
   return CLI::detail::lexical_cast(res[0], sg_reflectWhat);
}

bool reflection_ext_info_opt_setter(CLI::results_t res)
{
   sg_behavior = ExecMode::ReflectionExtInfo;
   return CLI::detail::lexical_cast(res[0], sg_reflectWhat);
}

void reflection_show_ini_cfg_opt_setter(int)
{
   sg_behavior = ExecMode::ReflectionExtInfo;
   sg_showIniCfg = true;
}

void print_polar_version()
{
//   std::cout << POLARPHP_PACKAGE_STRING << " (built: "<< BUILD_TIME <<  ") "<< std::endl
//             << "Copyright (c) 2016-2018 The polarphp foundation (https://polar.foundation)" << std::endl
//             << get_zend_version();
}

void setup_init_entries_commands(const std::vector<std::string> defines, std::string &iniEntries)
{
   for (StringRef defineStr : defines) {
      size_t equalPos = defineStr.find('=');
      if (equalPos != StringRef::npos) {
         char val = defineStr[++equalPos];
         if (!isalnum(val) && val != '"' && val != '\'' && val != '\0') {
            iniEntries += defineStr.substr(0, equalPos).str();
            iniEntries += "\"";
            iniEntries += defineStr.substr(equalPos).str();
            iniEntries += "\n\0\"";
         } else {
            iniEntries += defineStr.str();
         }
      } else {
         iniEntries += "=1\n\0";
      }
   }
}

std::string PhpOptFormatter::make_usage(const CLI::App *, std::string name) const
{
   std::stringstream out;
   name = "polar";
   out << get_label("Usage") << ":" << (name.empty() ? "" : " ") << name << " [options] [-f] <file> [--] [args...]" << std::endl;
   out << "   " << name << " [options] -r <code> [--] [args...]" << std::endl;
   out << "   " << name << " [options] [-B <begin_code>] -R <code> [-E <end_code>] [--] [args...]" << std::endl;
   out << "   " << name << " [options] [-B <begin_code>] -F <file> [-E <end_code>] [--] [args...]" << std::endl;
   out << "   " << name << " [options] -- [args...]" << std::endl;
   out << "   " << name << " [options] -a" << std::endl;
   return out.str();
}

std::string PhpOptFormatter::make_group(std::string group, bool is_positional, std::vector<const CLI::Option *> opts) const
{
   std::stringstream out;

   out << "\n" << group << ":\n";
   if (group == "Positionals") {
      for(const CLI::Option *opt : opts) {
         out << make_option(opt, is_positional);
      }
   } else if (group == "Options") {
      std::map<std::string, std::string> optMap;
      for(const CLI::Option *opt : opts) {
         optMap[opt->get_name()] = make_option(opt, is_positional);
      }
      for (std::string &optname: sm_opsNames) {
         out << optMap.at(optname);
      }
   }
   return out.str();
}

std::vector<std::string> PhpOptFormatter::sm_opsNames{
   "--interactive",
   "--config",
   "-n",
   "-d",
   "-f",
   "--help",
   "--ng-info",
   "--lint",
   "--modules-info",
   "-r",
   "-B",
   "-R",
   "-F",
   "-E",
   "-H",
   "--version",
   "-w",
   "-z",
   "--ini",
   "--rf",
   "--rc",
   "--rm",
   "--rz",
   "--ri"
};

} // polar
