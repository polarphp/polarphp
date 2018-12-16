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

/* Error display modes */
#define PHP_DISPLAY_ERRORS_STDOUT	1
#define PHP_DISPLAY_ERRORS_STDERR	2

namespace polar {

using polar::basic::StringRef;

struct php_tick_function_entry;
class ExecEnv;
extern POLAR_DECL_EXPORT int sg_coreGlobalsId;

ExecEnv &retrieve_global_execenv();

struct ArgSeparators
{
   char *output;
   char *input;
};

using IniConfigDefaultInitFunc = void (*)(HashTable *configuration_hash);

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
   ExecEnv &setDisplayErrors(bool flag);
   ExecEnv &setDisplayStartupErrors(bool flag);
   ExecEnv &setLogErrors(bool flag);
   ExecEnv &setIgnoreRepeatedErrors(bool flag);
   ExecEnv &setIgnoreRepeatedSource(bool flag);
   ExecEnv &setReportMemLeaks(bool flag);
   ExecEnv &setIgnoreUserAbort(bool flag);
   ExecEnv &setExposePhp(bool flag);
   ExecEnv &setRegisterArgcArgv(bool flag);
   ExecEnv &setAutoGlobalsJit(bool flag);
   ExecEnv &setHtmlErrors(bool flag);
   ExecEnv &setModulesActivated(bool flag);
   ExecEnv &setDuringRequestStartup(bool flag);
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

   ExecEnv &setPhpIniPathOverride(const std::string &path);
   ExecEnv &setInitEntries(const std::string &entries);
   ExecEnv &setPolarBinary(const std::string &binary);
   ExecEnv &setIniDefaultsHandler(IniConfigDefaultInitFunc handler);

   const std::vector<StringRef> &getArgv() const;
   int getArgc() const;
   StringRef getExecutableFilepath() const;
   bool getPhpIniIgnore() const;
   bool getPhpIniIgnoreCwd() const;
   bool getImplicitFlush() const;
   bool getEnableDl() const;
   bool getTrackErrors() const;
   bool getDisplayErrors() const;
   bool getDisplayStartupErrors() const;
   bool getLogErrors() const;
   bool getIgnoreRepeatedErrors() const;
   bool getIgnoreRepeatedSource() const;
   bool getReportMemLeaks() const;
   bool getIgnoreUserAbort() const;
   bool getExposePhp() const;
   bool getRegisterArgcArgv() const;
   bool getAutoGlobalsJit() const;
   bool getHtmlErrors() const;
   bool getModulesActivated() const;
   bool getDuringRequestStartup() const;
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
   bool getComInitialized();
#endif

   StringRef getPhpIniPathOverride() const;
   StringRef getIniEntries() const;
   StringRef getPolarBinary() const;
   IniConfigDefaultInitFunc getIniConfigDeaultHandler() const;
   zend_llist &getTickFunctions();

   void unbufferWrite(const char *str, int len);
private:
   bool m_phpIniIgnore;
   /// don't look for php.ini in the current directory
   bool m_phpIniIgnoreCwd;
   bool m_implicitFlush;
   bool m_enableDl;
   bool m_trackErrors;
   bool m_displayErrors;
   bool m_displayStartupErrors;
   bool m_logErrors;
   bool m_ignoreRepeatedErrors;
   bool m_ignoreRepeatedSource;
   bool m_reportMemLeaks;
   bool m_ignoreUserAbort;
   bool m_exposePhp;
   bool m_registerArgcArgv;
   bool m_autoGlobalsJit;
   bool m_htmlErrors;
   bool m_modulesActivated;
   bool m_duringRequestStartup;
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

   int m_argc;
   int lastErrorType;
   int lastErrorLineno;

   zend_long serializePrecision;
   zend_long memoryLimit;
   zend_long maxInputTime;
   zend_long m_outputBuffering;

   zend_long logErrorsMaxLen;
   zend_long maxInputNestingLevel;
   zend_long maxInputVars;
   zend_long userIniCacheTtl;
   zend_long syslogFacility;
   zend_long syslogFilter;
   std::string m_iniEntries;
   std::string m_phpIniPathOverride;
   std::string outputHandler;
   std::string unserializeCallbackFunc;
   std::string errorLog;
   std::string docRoot;
   std::string userDir;
   std::string includePath;
   std::string openBaseDir;
   std::string extensionDir;
   std::string m_polarBinary;
   std::string sysTempDir;
   std::string errorAppendString;
   std::string errorPrependString;
   std::string autoPrependFile;
   std::string autoAppendFile;
   std::string inputEncoding;
   std::string internalEncoding;
   std::string outputEncoding;

   std::string variablesOrder;
   std::string lastErrorMessage;
   std::string lastErrorFile;
   std::string phpSysTempDir;
   std::string disableFunctions;
   std::string disableClasses;
   std::string docrefRoot;
   std::string docrefExt;
   std::string userIniFilename;
   std::string requestOrder;
   std::string syslogIdent;
   IniConfigDefaultInitFunc m_iniDefaultInitHandler;
   ArgSeparators argSeparator;
   std::vector<StringRef> m_argv;
   zend_llist m_tickFunctions;
};

POLAR_DECL_EXPORT int php_execute_script(zend_file_handle *primaryFile);
POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primaryFile, zval *ret);

ZEND_COLD void php_error_callback(int type, const char *errorFilename,
                                  const uint32_t errorLineno, const char *format,
                                  va_list args);

POLAR_DECL_EXPORT size_t php_printf(const char *format, ...);
size_t php_output_wrapper(const char *str, size_t strLength);
FILE *php_fopen_wrapper_for_zend(const char *filename, zend_string **openedPath);
zval *php_get_configuration_directive_for_zend(zend_string *name);
POLAR_DECL_EXPORT void php_message_handler_for_zend(zend_long message, const void *data);
void php_on_timeout(int seconds);
int php_stream_open_for_zend(const char *filename, zend_file_handle *handle);
POLAR_DECL_EXPORT void php_printf_to_smart_string(smart_string *buf, const char *format, va_list ap);
POLAR_DECL_EXPORT void php_printf_to_smart_str(smart_str *buf, const char *format, va_list ap);
POLAR_DECL_EXPORT char *bootstrap_getenv(char *name, size_t nameLen);
zend_string *php_resolve_path_for_zend(const char *filename, size_t filenameLen);
bool seek_file_begin(zend_file_handle *fileHandle, const char *scriptFile, int *lineno);

} // polar

#endif // POLARPHP_ARTIFACTS_EXEC_ENV_H
