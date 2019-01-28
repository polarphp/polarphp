// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/25.

#ifndef POLARPHP_RUNTIME_EXECENV_H
#define POLARPHP_RUNTIME_EXECENV_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ArrayRef.h"

#include <vector>
#include <string>
#include <iostream>

#include "polarphp/runtime/internal/DepsZendVmHeaders.h"
#include "polarphp/runtime/RtDefs.h"

/* Error display modes */
#define PHP_DISPLAY_ERRORS_STDOUT	1
#define PHP_DISPLAY_ERRORS_STDERR	2

#ifdef HAVE_SYSLOG_H
#include "SysLog.h"
#define php_log_err(msg) php_log_err_with_severity(msg, LOG_NOTICE)
#else
#define php_log_err(msg) php_log_err_with_severity(msg, 5)
#endif

namespace polar {
namespace runtime {

using polar::basic::StringRef;
using IniConfigDefaultInitFunc = void (*)(HashTable *configuration_hash);
struct php_tick_function_entry;

/// TODO
///  for interactive command shell
///
struct CliShellCallbacksType
{
   size_t (*cliShellWrite)(const char *str, size_t str_length);
   size_t (*cliShellUnbufferWrite)(const char *str, size_t str_length);
   int (*cliShellRun)();
};

class ExecEnv;
struct ExecEnvInfo;

ExecEnv &retrieve_global_execenv();
ExecEnvInfo &retrieve_global_execenv_runtime_info();
ssize_t cli_single_write(const char *str, size_t strLength);
size_t cli_unbuffer_write(const char *str, size_t strLength);
void cli_flush();
CliShellCallbacksType *php_cli_get_shell_callbacks();

POLAR_DECL_EXPORT int php_execute_script(zend_file_handle *primaryFile);
POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primaryFile, zval *ret);

ZEND_COLD void php_error_callback(int type, const char *errorFilename,
                                  const uint32_t errorLineno, const char *format,
                                  va_list args);

size_t php_write(void *buf, size_t size);
size_t php_printf(const char *format, ...) PHP_ATTRIBUTE_FORMAT(printf, 1, 2);
size_t php_output_wrapper(const char *str, size_t strLength);
zval *php_get_configuration_directive_for_zend(zend_string *name);
void php_message_handler_for_zend(zend_long message, const void *data);
void php_on_timeout(int seconds);
char *bootstrap_getenv(char *name, size_t nameLen);
zend_string *php_resolve_path(const char *filename, size_t filename_len, const char *path);
zend_string *php_resolve_path_for_zend(const char *filename, size_t filenameLen);
bool seek_file_begin(zend_file_handle *fileHandle, const char *scriptFile, int *lineno);
POLAR_DECL_EXPORT bool php_hash_environment();
void cli_register_file_handles();
POLAR_DECL_EXPORT ZEND_COLD void php_log_err_with_severity(char *logMessage, int syslogTypeInt);

///
/// POD data of execute environment
///
struct ExecEnvInfo
{
   bool phpIniIgnore;
   /// don't look for php.ini in the current directory
   bool phpIniIgnoreCwd;
   bool implicitFlush;
   bool enableDl;
   bool trackErrors;
   bool displayStartupErrors;
   bool logErrors;
   bool ignoreRepeatedErrors;
   bool ignoreRepeatedSource;
   bool reportMemLeaks;
   bool ignoreUserAbort;
   bool registerArgcArgv;
   bool modulesActivated;
   bool duringExecEnvStartup;
   bool allowUrlFopen;
   bool reportZendDebug;
   bool inErrorLog;
   bool inUserInclude;
#ifdef POLAR_OS_WIN32
   bool windowsShowCrtWarning;
#endif
   bool haveCalledOpenlog;
   bool allowUrlInclude;
#ifdef POLAR_OS_WIN32
   bool comInitialized;
#endif

   std::uint8_t displayErrors;

   int lastErrorType;
   int lastErrorLineno;
   size_t scriptArgc;

   zend_long serializePrecision;
   zend_long memoryLimit;
   zend_long outputBuffering;
   zend_long logErrorsMaxLen;
   zend_long maxInputNestingLevel;
   zend_long maxInputVars;
   zend_long userIniCacheTtl;
   zend_long syslogFacility;
   zend_long syslogFilter;
   zend_long defaultSocketTimeout;

   std::string iniEntries;
   std::string phpIniPathOverride;
   std::string outputHandler;
   std::string unserializeCallbackFunc;
   std::string errorLog;
   std::string docRoot;
   std::string userDir;
   std::string includePath;
   std::string openBaseDir;
   std::string extensionDir;
   std::string polarBinary;
   std::string sysTempDir;
   std::string errorAppendString;
   std::string errorPrependString;
   std::string autoPrependFile;
   std::string autoAppendFile;
   std::string inputEncoding;
   std::string internalEncoding;
   std::string outputEncoding;

   std::string lastErrorMessage;
   std::string lastErrorFile;
   std::string phpSysTempDir;
   std::string disableFunctions;
   std::string disableClasses;
   std::string docrefRoot;
   std::string docrefExt;
   std::string userIniFilename;
   std::string syslogIdent;
   std::string entryScriptFilename;

   std::vector<std::string> scriptArgv;
   IniConfigDefaultInitFunc iniDefaultInitHandler;
   zend_llist tickFunctions;
};

class ExecEnv
{
public:
   ExecEnv();
   ~ExecEnv();

   bool bootup();
   void shutdown();
   void activate();
   void deactivate();

   ExecEnv &setCompileOptions(int opts);
   ExecEnv &setContainerArgc(int argc);
   ExecEnv &setContainerArgv(const std::vector<StringRef> &argv);
   ExecEnv &setContainerArgv(char *argv[]);
   ExecEnv &setEnvReady(bool flag);

   bool isEnvReady() const;
   ExecEnvInfo &getRuntimeInfo();
   uint32_t getCompileOptions() const;
   const std::vector<StringRef> &getContainerArgv() const;
   int getContainerArgc() const;
   StringRef getExecutableFilepath() const;
   int getVmExitStatus() const;

   bool execScript(StringRef filename, int &exitStatus);

   size_t unbufferWrite(const char *str, int len);
   void logMessage(const char *logMessage, int syslogTypeInt);
   void initDefaultConfig(HashTable *configurationHash);

private:
   bool m_moduleStarted;
   bool m_execEnvStarted;
   bool m_execEnvReady;
   bool m_execEnvDestroyed;
   int m_argc;
   std::vector<StringRef> m_argv;
   ExecEnvInfo m_runtimeInfo;
};

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_EXECENV_H
