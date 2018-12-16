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
struct PhpCoreGlobals;
# define PG(v) ZEND_TSRMG(sg_coreGlobalsId, PhpCoreGlobals *, v)
extern POLAR_DECL_EXPORT int sg_coreGlobalsId;
//extern thread_local POLAR_DECL_EXPORT PhpCoreGlobals sg_coreGlobals;

ExecEnv &retrieve_global_execenv();

struct ArgSeparators
{
   char *output;
   char *input;
};

struct PhpCoreGlobals
{
   bool implicitFlush;
   zend_long outputBuffering;
   bool enableDl;
   std::string outputHandler;
   std::string unserializeCallbackFunc;
   zend_long serializePrecision;
   zend_long memoryLimit;
   zend_long maxInputTime;
   bool trackErrors;
   bool displayErrors;
   bool displayStartupErrors;
   bool logErrors;
   zend_long logErrorsMaxLen;
   bool ignoreRepeatedErrors;
   bool ignoreRepeatedSource;
   bool reportMemLeaks;
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
   ArgSeparators argSeparator;
   std::string variablesOrder;
   bool ignoreUserAbort;
   zend_llist tickFunctions;
   bool exposePhp;
   bool registerArgcArgv;
   bool autoGlobalsJit;
   std::string docrefRoot;
   std::string docrefExt;
   bool htmlErrors;
   bool modulesActivated;
   bool duringRequestStartup;
   bool allowUrlFopen;
   bool reportZendDebug;
   int lastErrorType;
   std::string lastErrorMessage;
   std::string lastErrorFile;
   int lastErrorLineno;
   std::string phpSysTempDir;
   std::string disableFunctions;
   std::string disableClasses;
   bool allowUrlInclude;
#ifdef PHP_WIN32
   bool comInitialized;
#endif
   zend_long maxInputNestingLevel;
   zend_long maxInputVars;
   bool inUserInclude;
   std::string userIniFilename;
   zend_long userIniCacheTtl;
   std::string requestOrder;
   bool inErrorLog;
#ifdef PHP_WIN32
   bool windowsShowCrtWarning;
#endif
   zend_long syslogFacility;
   std::string syslogIdent;
   bool haveCalledOpenlog;
   zend_long syslogFilter;
};

using IniConfigDefaultInitFunc = void (*)(HashTable *configuration_hash);

class ExecEnv
{
public:
   void activate();
   void deactivate();

   ExecEnv &setArgc(int argc);
   ExecEnv &setArgv(const std::vector<StringRef> &argv);
   ExecEnv &setArgv(char *argv[]);
   ExecEnv &setPhpIniIgnore(bool flag);
   ExecEnv &setPhpIniIgnoreCwd(bool flag);
   ExecEnv &setPhpIniPathOverride(const std::string &path);
   ExecEnv &setInitEntries(const std::string &entries);
   ExecEnv &setIniDefaultsHandler(IniConfigDefaultInitFunc handler);

   const std::vector<StringRef> &getArgv() const;
   int getArgc() const;
   StringRef getExecutableFilepath() const;
   bool getPhpIniIgnore() const;
   bool getPhpIniIgnoreCwd() const;
   StringRef getPhpIniPathOverride() const;
   StringRef getIniEntries() const;
   IniConfigDefaultInitFunc getIniConfigDeaultHandler() const;

   void unbufferWrite(const char *str, int len);
private:
   bool m_phpIniIgnore;
   /// don't look for php.ini in the current directory
   bool m_phpIniIgnoreCwd;
   int m_argc;
   std::string m_iniEntries;
   std::string m_phpIniPathOverride;
   IniConfigDefaultInitFunc m_iniDefaultInitHandler;
   std::vector<StringRef> m_argv;
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

} // polar

#endif // POLARPHP_ARTIFACTS_EXEC_ENV_H
