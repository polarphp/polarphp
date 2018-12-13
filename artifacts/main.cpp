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

#include "CLI/CLI.hpp"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/Config.h"
#include "lib/Defs.h"
#include "lib/Commands.h"
#include "lib/ProcessTitle.h"

#include <vector>
#include <string>

using polar::basic::StringRef;

void setup_command_opts(CLI::App &parser);

namespace {
static bool sg_showVersion;
static bool sg_showNgInfo;
static bool sg_interactive;
static bool sg_generateExtendInfo;
static bool sg_syntaxCheck;
static bool sg_showModulesInfo;
static bool sg_hideExternArgs;
static std::string sg_configPath;
static std::string sg_scriptFilepath;
static std::string sg_codeWithoutPhpTags;
static std::string sg_beginCode;
static std::string sg_everyLineExecCode;
static std::string sg_everyLineExecScriptFilename;
static std::string sg_endCode;
static std::string sg_stripCodeFilename;
static std::string sg_zendExtensionFilename;
static std::vector<std::string> sg_scriptArgs;
static std::string sg_reflectFunc;
static std::string sg_reflectClass;
static std::string sg_reflectModule;
static std::string sg_reflectZendExt;
static std::string sg_reflectConfig;
} // anonymous namespace

int main(int argc, char *argv[])
{
   polar::InitPolar polarInitializer(argc, argv);
   CLI::App cmdParser;
   polarInitializer.initNgOpts(cmdParser);
   setup_command_opts(cmdParser);
   CLI11_PARSE(cmdParser, argc, argv);

#if defined(POLAR_OS_WIN32)
# ifdef PHP_CLI_WIN32_NO_CONSOLE
   int argc = __argc;
   char **argv = __argv;
# endif
   int num_args;
   wchar_t **argv_wide;
   char **argv_save = argv;
   BOOL using_wide_argv = 0;
#endif

//   int c;
//   int exit_status = SUCCESS;
//   int module_started = 0, sapi_started = 0;
//   char *php_optarg = NULL;
//   int php_optind = 1, use_extended_info = 0;
//   char *ini_path_override = NULL;
//   char *ini_entries = NULL;
//   size_t ini_entries_len = 0;
//   int ini_ignore = 0;

//   /*
//    * Do not move this initialization. It needs to happen before argv is used
//    * in any way.
//    */
//   argv = polar::save_ps_args(argc, argv);

//#if defined(POLAR_OS_WIN32) && !defined(POLAR_CLI_WIN32_NO_CONSOLE)
//   php_win32_console_fileno_set_vt100(STDOUT_FILENO, TRUE);
//   php_win32_console_fileno_set_vt100(STDERR_FILENO, TRUE);
//#endif

//#if defined(POLAR_OS_WIN32) && defined(_DEBUG) && defined(POLAR_WIN32_DEBUG_HEAP)
//   {
//      int tmp_flag;
//      _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
//      _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
//      _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
//      _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
//      _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
//      _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
//      tmp_flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
//      tmp_flag |= _CRTDBG_DELAY_FREE_MEM_DF;
//      tmp_flag |= _CRTDBG_LEAK_CHECK_DF;
//      _CrtSetDbgFlag(tmp_flag);
//   }
//#endif

//#ifdef HAVE_SIGNAL_H
//#if defined(SIGPIPE) && defined(SIG_IGN)
//   signal(SIGPIPE, SIG_IGN);
//   /// ignore SIGPIPE in standalone mode so
//   /// that sockets created via fsockopen()
//   /// don't kill PHP if the remote site
//   /// closes it.  in apache|apxs mode apache
//   /// does that for us!  thies@thieso.net
//   /// 20000419
//#endif
//#endif

//   tsrm_startup(1, 1, 0, nullptr);
//   (void)ts_resource(0);
//   ZEND_TSRMLS_CACHE_UPDATE();

//   zend_signal_startup();
//#ifdef POLAR_OS_WIN32
//   _fmode = _O_BINARY;			/*sets default for file streams to binary */
//   setmode(_fileno(stdin), O_BINARY);		/* make the stdio mode be binary */
//   setmode(_fileno(stdout), O_BINARY);		/* make the stdio mode be binary */
//   setmode(_fileno(stderr), O_BINARY);		/* make the stdio mode be binary */
//#endif

//   /* startup after we get the above ini override se we get things right */
//   if (sapi_module->startup(sapi_module) == FAILURE) {
//      /* there is no way to see if we must call zend_ini_deactivate()
//          * since we cannot check if EG(ini_directives) has been initialised
//          * because the executor's constructor does not set initialize it.
//          * Apart from that there seems no need for zend_ini_deactivate() yet.
//          * So we goto out_err.*/
//      exit_status = 1;
//      goto out;
//   }
//   module_started = 1;
//   if (sg_showVersion) {
//      polar::print_polar_version();
//      return 0;
//   }
//   return 0;
}

void setup_command_opts(CLI::App &parser)
{
   parser.add_flag("-v, --version", sg_showVersion, "Show polarphp version info.");
   parser.add_flag("-i, --ng-info", sg_showNgInfo, "Show polarphp info.");
   parser.add_flag("-a, --interactive", sg_interactive, "Run interactively PHP shell.");
   parser.add_flag("-e, --generate-extend-info", sg_generateExtendInfo, "Generate extended information for debugger/profiler.");
   parser.add_flag("-l, --lint", sg_syntaxCheck, "Syntax check only (lint)");
   parser.add_flag("-m, --modules-info", sg_showModulesInfo, "Show compiled in modules.");
   parser.add_option("-c, --config", sg_configPath, "Look for php.yaml file in this directory.")->type_name("<path>|<file>");
   parser.add_option("-f", sg_scriptFilepath, "Parse and execute <file>.")->type_name("<file>");
   parser.add_option("-r", sg_codeWithoutPhpTags, "Run PHP <code> without using script tags <?..?>.")->type_name("<code>");
   parser.add_option("-B", sg_beginCode, "Run PHP <begin_code> before processing input lines.")->type_name("<begin_code>");
   parser.add_option("-R", sg_everyLineExecCode, "Run PHP <code> for every input line.")->type_name("<code>");
   parser.add_option("-F", sg_everyLineExecScriptFilename, "Parse and execute <file> for every input line.")->type_name("<file>");
   parser.add_option("-E", sg_endCode, "Run PHP <end_code> after processing all input lines.")->type_name("<end_code>");
   parser.add_flag("-H", sg_hideExternArgs, "Hide any passed arguments from external tools.");
   parser.add_option("-w", sg_stripCodeFilename, "Output source with stripped comments and whitespace.")->type_name("<file>");
   parser.add_option("-z", sg_zendExtensionFilename, "Load Zend extension <file>.")->type_name("<file>");
   parser.add_option("args", sg_scriptArgs, "Arguments passed to script. Use -- args when first argument.")->type_name("string");
   parser.add_option("--rf", sg_reflectFunc, "Show information about function <name>.")->type_name("<name>");
   parser.add_option("--rc", sg_reflectClass, "Show information about class <name>.")->type_name("<name>");
   parser.add_option("--rm", sg_reflectModule, "Show information about extension <name>.")->type_name("<name>");
   parser.add_option("--rz", sg_reflectZendExt, "Show information about Zend extension <name>.")->type_name("<name>");
   parser.add_option("--ri", sg_reflectConfig, "Show configuration for extension <name>.")->type_name("<name>");
}
