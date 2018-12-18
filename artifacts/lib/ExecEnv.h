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

#ifndef POLARPHP_ARTIFACTS_EXEC_ENV_H
#define POLARPHP_ARTIFACTS_EXEC_ENV_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/ArrayRef.h"

#include <vector>
#include <string>
#include <iostream>

#include "ZendHeaders.h"
#include "Defs.h"

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

using polar::basic::StringRef;
using IniConfigDefaultInitFunc = void (*)(HashTable *configuration_hash);
struct php_tick_function_entry;

struct ArgSeparators
{
   char *output;
   char *input;
};

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

ExecEnv &retrieve_global_execenv();

ssize_t cli_single_write(const char *str, size_t strLength);
size_t cli_unbuffer_write(const char *str, size_t strLength);
void cli_flush();
CliShellCallbacksType *php_cli_get_shell_callbacks();

POLAR_DECL_EXPORT int php_execute_script(zend_file_handle *primaryFile);
POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primaryFile, zval *ret);

ZEND_COLD void php_error_callback(int type, const char *errorFilename,
                                  const uint32_t errorLineno, const char *format,
                                  va_list args);

POLAR_DECL_EXPORT size_t php_write(void *buf, size_t size);
POLAR_DECL_EXPORT  size_t php_printf(const char *format, ...) PHP_ATTRIBUTE_FORMAT(printf, 1, 2);
size_t php_output_wrapper(const char *str, size_t strLength);
FILE *php_fopen_wrapper_for_zend(const char *filename, zend_string **openedPath);
zval *php_get_configuration_directive_for_zend(zend_string *name);
POLAR_DECL_EXPORT void php_message_handler_for_zend(zend_long message, const void *data);
void php_on_timeout(int seconds);
int php_stream_open_for_zend(const char *filename, zend_file_handle *handle);
//POLAR_DECL_EXPORT void php_printf_to_smart_string(smart_string *buf, const char *format, va_list ap);
//POLAR_DECL_EXPORT void php_printf_to_smart_str(smart_str *buf, const char *format, va_list ap);
POLAR_DECL_EXPORT char *bootstrap_getenv(char *name, size_t nameLen);
zend_string *php_resolve_path_for_zend(const char *filename, size_t filenameLen);
bool seek_file_begin(zend_file_handle *fileHandle, const char *scriptFile, int *lineno);
POLAR_DECL_EXPORT bool php_hash_environment();
void cli_register_file_handles();
POLAR_DECL_EXPORT ZEND_COLD void php_log_err_with_severity(char *logMessage, int syslogTypeInt);

class ExecEnv
{
public:
   ExecEnv();
   ~ExecEnv();
   void activate();
   void deactivate();

   ExecEnv &setArgc(int argc);
   ExecEnv &setArgv(const std::vector<StringRef> &argv);
   ExecEnv &setArgv(char *argv[]);
   ExecEnv &setPhpIniIgnore(bool flag);
   ExecEnv &setPhpIniIgnoreCwd(bool flag);
   ExecEnv &setImplicitFlush(bool flag);
   ExecEnv &setEnableDl(bool flag);
   ExecEnv &setTrackErrors(bool flag);
   ExecEnv &setDisplayStartupErrors(bool flag);
   ExecEnv &setLogErrors(bool flag);
   ExecEnv &setIgnoreRepeatedErrors(bool flag);
   ExecEnv &setIgnoreRepeatedSource(bool flag);
   ExecEnv &setReportMemLeaks(bool flag);
   ExecEnv &setIgnoreUserAbort(bool flag);
   ExecEnv &setRegisterArgcArgv(bool flag);
   ExecEnv &setAutoGlobalsJit(bool flag);
   ExecEnv &setModulesActivated(bool flag);
   ExecEnv &setDuringExecEnvStartup(bool flag);
   ExecEnv &setAllowUrlFopen(bool flag);
   ExecEnv &setReportZendDebug(bool flag);
   ExecEnv &setInErrorLog(bool flag);
   ExecEnv &setInUserInclude(bool flag);
#ifdef POLAR_OS_WIN32
   ExecEnv &setWindowsShowCrtWarning(bool flag);
#endif
   ExecEnv &setHaveCalledOpenlog(bool flag);
   ExecEnv &setAllowUrlInclude(bool flag);
#ifdef POLAR_OS_WIN32
   ExecEnv &setComInitialized(bool flag);
#endif
   ExecEnv &setStarted(bool flag);

   ExecEnv &setIniDefaultsHandler(IniConfigDefaultInitFunc handler);

   ExecEnv &setDisplayErrors(std::uint8_t value);

   ExecEnv &setLastErrorType(int type);
   ExecEnv &setLastErrorLineno(int line);

   ExecEnv &setSerializePrecision(zend_long value);
   ExecEnv &setMemoryLimit(zend_long value);
   ExecEnv &setOutputBuffering(zend_long value);
   ExecEnv &setLogErrorsMaxLen(zend_long value);
   ExecEnv &setMaxInputNestingLevel(zend_long value);
   ExecEnv &setMaxInputVars(zend_long value);
   ExecEnv &setUserIniCacheTtl(zend_long value);
   ExecEnv &setSyslogFacility(zend_long value);
   ExecEnv &setSyslogFilter(zend_long value);
   ExecEnv &setDefaultSocketTimeout(zend_long value);

   ExecEnv &setPhpIniPathOverride(const std::string &path);
   ExecEnv &setInitEntries(const std::string &entries);
   ExecEnv &setOutputHandler(const std::string &value);
   ExecEnv &setUnserializeCallbackFunc(const std::string &value);
   ExecEnv &setErrorLog(const std::string &value);
   ExecEnv &setDocRoot(const std::string &value);
   ExecEnv &setUserDir(const std::string &value);
   ExecEnv &setIncludePath(const std::string &value);
   ExecEnv &setOpenBaseDir(const std::string &value);
   ExecEnv &setExtensionDir(const std::string &value);
   ExecEnv &setPolarBinary(const std::string &value);
   ExecEnv &setSysTempDir(const std::string &value);
   ExecEnv &setErrorAppendString(const std::string &value);
   ExecEnv &setErrorPrependString(const std::string &value);
   ExecEnv &setAutoPrependFile(const std::string &value);
   ExecEnv &setAutoAppendFile(const std::string &value);
   ExecEnv &setInputEncoding(const std::string &value);
   ExecEnv &setInternalEncoding(const std::string &value);
   ExecEnv &setOutputEncoding(const std::string &value);

   ExecEnv &setVariablesOrder(const std::string &value);
   ExecEnv &setLastErrorMessage(const std::string &value);
   ExecEnv &setLastErrorFile(const std::string &value);
   ExecEnv &setPhpSysTempDir(const std::string &value);
   ExecEnv &setDisableFunctions(const std::string &value);
   ExecEnv &setDisableClasses(const std::string &value);
   ExecEnv &setDocrefRoot(const std::string &value);
   ExecEnv &setDocrefExt(const std::string &value);
   ExecEnv &setUserIniFilename(const std::string &value);
   ExecEnv &setRequestOrder(const std::string &value);
   ExecEnv &setSyslogIdent(const std::string &value);

   ExecEnv &setIniConfigDeaultHandler(IniConfigDefaultInitFunc func);
   ExecEnv &setArgSeparator(ArgSeparators seps);

   const std::vector<StringRef> &getArgv() const;
   int getArgc() const;
   StringRef getExecutableFilepath() const;

   bool getPhpIniIgnore() const;
   bool getPhpIniIgnoreCwd() const;
   bool getImplicitFlush() const;
   bool getEnableDl() const;
   bool getTrackErrors() const;
   bool getDisplayStartupErrors() const;
   bool getLogErrors() const;
   bool getIgnoreRepeatedErrors() const;
   bool getIgnoreRepeatedSource() const;
   bool getReportMemLeaks() const;
   bool getIgnoreUserAbort() const;
   bool getRegisterArgcArgv() const;
   bool getAutoGlobalsJit() const;
   bool getModulesActivated() const;
   bool getDuringExecEnvStartup() const;
   bool getAllowUrlFopen() const;
   bool getReportZendDebug() const;
   bool getInErrorLog() const;
   bool getInUserInclude() const;
#ifdef POLAR_OS_WIN32
   bool getWindowsShowCrtWarning() const;
#endif
   bool getHaveCalledOpenlog() const;
   bool getAllowUrlInclude() const;
#ifdef POLAR_OS_WIN32
   bool getComInitialized() const;
#endif
   bool getStarted() const;

   std::uint8_t getDisplayErrors() const;

   int getLastErrorType() const;
   int getLastErrorLineno() const;

   zend_long getSerializePrecision() const;
   zend_long getMemoryLimit() const;
   zend_long getOutputBuffering() const;
   zend_long getLogErrorsMaxLen() const;
   zend_long getMaxInputNestingLevel() const;
   zend_long getMaxInputVars() const;
   zend_long getUserIniCacheTtl() const;
   zend_long getSyslogFacility() const;
   zend_long getSyslogFilter() const;
   zend_long getDefaultSocketTimeout() const;

   StringRef getPhpIniPathOverride() const;
   StringRef getIniEntries() const;
   StringRef getOutputHandler() const;
   StringRef getUnserializeCallbackFunc() const;
   StringRef getErrorLog() const;
   StringRef getDocRoot() const;
   StringRef getUserDir() const;
   StringRef getIncludePath() const;
   StringRef getOpenBaseDir() const;
   StringRef getExtensionDir() const;
   StringRef getPolarBinary() const;
   StringRef getSysTempDir() const;
   StringRef getErrorAppendString() const;
   StringRef getErrorPrependString() const;
   StringRef getAutoPrependFile() const;
   StringRef getAutoAppendFile() const;
   StringRef getInputEncoding() const;
   StringRef getInternalEncoding() const;
   StringRef getOutputEncoding() const;

   StringRef getVariablesOrder() const;
   StringRef getLastErrorMessage() const;
   StringRef getLastErrorFile() const;
   StringRef getPhpSysTempDir() const;
   StringRef getDisableFunctions() const;
   StringRef getDisableClasses() const;
   StringRef getDocrefRoot() const;
   StringRef getDocrefExt() const;
   StringRef getUserIniFilename() const;
   StringRef getRequestOrder() const;
   StringRef getSyslogIdent() const;

   IniConfigDefaultInitFunc getIniConfigDeaultHandler() const;
   const ArgSeparators &getArgSeparator() const;
   const zend_llist &getTickFunctions() const;
   zend_llist &getTickFunctions();

   void unbufferWrite(const char *str, int len);
   void logMessage(const char *logMessage, int syslogTypeInt);
private:
   bool m_phpIniIgnore;
   /// don't look for php.ini in the current directory
   bool m_phpIniIgnoreCwd;
   bool m_implicitFlush;
   bool m_enableDl;
   bool m_trackErrors;
   bool m_displayStartupErrors;
   bool m_logErrors;
   bool m_ignoreRepeatedErrors;
   bool m_ignoreRepeatedSource;
   bool m_reportMemLeaks;
   bool m_ignoreUserAbort;
   bool m_registerArgcArgv;
   bool m_autoGlobalsJit;
   bool m_modulesActivated;
   bool m_duringExecEnvStartup;
   bool m_allowUrlFopen;
   bool m_reportZendDebug;
   bool m_inErrorLog;
   bool m_inUserInclude;
#ifdef POLAR_OS_WIN32
   bool m_windowsShowCrtWarning;
#endif
   bool m_haveCalledOpenlog;
   bool m_allowUrlInclude;
#ifdef POLAR_OS_WIN32
   bool m_comInitialized;
#endif
   bool m_started;

   std::uint8_t m_displayErrors;

   int m_argc;
   int m_lastErrorType;
   int m_lastErrorLineno;

   zend_long m_serializePrecision;
   zend_long m_memoryLimit;
   zend_long m_outputBuffering;
   zend_long m_logErrorsMaxLen;
   zend_long m_maxInputNestingLevel;
   zend_long m_maxInputVars;
   zend_long m_userIniCacheTtl;
   zend_long m_syslogFacility;
   zend_long m_syslogFilter;
   zend_long m_defaultSocketTimeout;

   std::string m_iniEntries;
   std::string m_phpIniPathOverride;
   std::string m_outputHandler;
   std::string m_unserializeCallbackFunc;
   std::string m_errorLog;
   std::string m_docRoot;
   std::string m_userDir;
   std::string m_includePath;
   std::string m_openBaseDir;
   std::string m_extensionDir;
   std::string m_polarBinary;
   std::string m_sysTempDir;
   std::string m_errorAppendString;
   std::string m_errorPrependString;
   std::string m_autoPrependFile;
   std::string m_autoAppendFile;
   std::string m_inputEncoding;
   std::string m_internalEncoding;
   std::string m_outputEncoding;

   std::string m_variablesOrder;
   std::string m_lastErrorMessage;
   std::string m_lastErrorFile;
   std::string m_phpSysTempDir;
   std::string m_disableFunctions;
   std::string m_disableClasses;
   std::string m_docrefRoot;
   std::string m_docrefExt;
   std::string m_userIniFilename;
   std::string m_requestOrder;
   std::string m_syslogIdent;

   IniConfigDefaultInitFunc m_iniDefaultInitHandler;
   ArgSeparators m_argSeparator;
   std::vector<StringRef> m_argv;
   zend_llist m_tickFunctions;
};

} // polar

#endif // POLARPHP_ARTIFACTS_EXEC_ENV_H
