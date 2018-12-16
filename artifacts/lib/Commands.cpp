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

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/basic/adt/StringRef.h"
#include "Commands.h"
#include "ExecEnv.h"
#include "Defs.h"
#include "lib/PolarVersion.h"

#include <iostream>
#include <vector>

/// defined in main.cpp
extern std::vector<std::string> sg_defines;
extern bool sg_showVersion;
extern bool sg_showVersion;
extern bool sg_showNgInfo;
extern bool sg_interactive;
extern bool sg_generateExtendInfo;
extern bool sg_ignoreIni;
extern bool sg_syntaxCheck;
extern bool sg_showModulesInfo;
extern bool sg_hideExternArgs;

extern int sg_exitStatus;
extern std::string sg_errorMsg;

namespace polar {

using polar::basic::StringRef;

namespace {
const char *PARAM_MODE_CONFLICT = "Either execute direct code, process stdin or use a file.";
ExecMode sg_behavior = ExecMode::Standard;
}

void interactive_opt_setter(int)
{
   if (sg_behavior != ExecMode::Standard) {
      sg_exitStatus = 1;
      sg_errorMsg = PARAM_MODE_CONFLICT;
      throw CLI::ParseError(PARAM_MODE_CONFLICT, sg_exitStatus);
   }
}

bool everyline_exec_script_filename_setter(CLI::results_t res)
{
   return true;
}

void print_polar_version()
{
   std::cout << POLARPHP_PACKAGE_STRING << " (built: "<< BUILD_TIME <<  ") "<< std::endl
             << "Copyright (c) 2016-2018 The polarphp Foundation" << std::endl
             << get_zend_version();
}

void setup_init_entries_commands(const std::vector<std::string> defines, std::string &iniEntries)
{
   for (StringRef defineStr : defines) {
      size_t equalPos = defineStr.find('=');
      if (equalPos != StringRef::npos) {
         char val = defineStr[++equalPos];
         if (!isalnum(val) && val != '"' && val != '\'' && val != '\0') {
            iniEntries += defineStr.substr(0, equalPos).getStr();
            iniEntries += "\"";
            iniEntries += defineStr.substr(equalPos).getStr();
            iniEntries += "\n\0\"";
         } else {
            iniEntries += defineStr.getStr();
         }
      } else {
         iniEntries += "=1\n\0";
      }
   }
}

int dispatch_cli_command()
{
   ExecEnv &execEnv = retrieve_global_execenv();
   int c;
   zend_file_handle file_handle;
   char *reflection_what = nullptr;
   volatile int request_started = 0;
   volatile int exitStatus = 0;
   char *php_optarg = nullptr;
   char *orig_optarg = nullptr;
   int php_optind = 1;
   int orig_optind = 1;
   char *exec_direct = nullptr;
   char *exec_run = nullptr;
   char *exec_begin = nullptr;
   char *exec_end = nullptr;
   char *arg_free = nullptr;
   char **arg_excp = &arg_free;
   char *script_file = nullptr;
   char *translated_path = nullptr;
   int interactive = 0;
   int lineno = 0;
   const char *param_error = nullptr;
   int hide_argv = 0;
   zend_try {
      CG(in_compilation) = 0; /* not initialized but needed for several options */
      if (sg_showVersion) {
         polar::print_polar_version();
         execEnv.deactivate();
         goto out;
      }
      /// processing with priority
      ///
   } zend_end_try();
out:
   if (translated_path) {
      free(translated_path);
   }
   if (exitStatus == 0) {
      exitStatus = EG(exit_status);
   }
   return exitStatus;
err:
   execEnv.deactivate();
   zend_ini_deactivate();
   exitStatus = 1;
   goto out;
}

} // polar
