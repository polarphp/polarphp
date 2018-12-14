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

#include <string>

#include "ZendHeaders.h"

/* Error display modes */
#define PHP_DISPLAY_ERRORS_STDOUT	1
#define PHP_DISPLAY_ERRORS_STDERR	2

namespace polar {

struct php_tick_function_entry;

struct PhpCoreGlobals;
# define PG(v) ZEND_TSRMG(sg_coreGlobalsId, PhpCoreGlobals *, v)
extern POLAR_DECL_EXPORT int sg_coreGlobalsId;
//extern thread_local POLAR_DECL_EXPORT PhpCoreGlobals sg_coreGlobals;

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

POLAR_DECL_EXPORT int php_execute_script(zend_file_handle *primaryFile);
POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primaryFile, zval *ret);

class ExecEnv
{
public:
   void unbufferWrite(const char *str, int len);
};

} // polar

#endif // POLARPHP_ARTIFACTS_EXEC_ENV_H
