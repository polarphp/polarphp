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

namespace polar {

struct PhpCoreGlobals;
extern thread_local POLAR_DECL_EXPORT PhpCoreGlobals sg_coreGlobals;

struct PhpCoreGlobals
{
   bool ImplicitFlush;
   zend_long OutputBuffering;
   bool EnableDl;
   std::string OutputHandler;
   std::string UnserializeCallbackFunc;
   zend_long SerializePrecision;
   zend_long MemoryLimit;
   zend_long MaxInputTime;
   bool TrackErrors;
   bool DisplayErrors;
   bool DisplayStartupErrors;
   bool LogErrors;
   zend_long LogErrorsMaxLen;
   bool IgnoreRepeatedErrors;
   bool IgnoreRepeatedSource;
   bool ReportMemLeaks;
   std::string ErrorLog;
   std::string DocRoot;
   std::string UserDir;
   std::string IncludePath;
   std::string OpenBaseDir;
   std::string ExtensionDir;
   std::string PolarBinary;
   std::string SysTempDir;
   std::string ErrorAppendString;
   std::string ErrorPrependString;
   std::string AutoPrependFile;
   std::string AutoAppendFile;
   std::string InputEncoding;
   std::string InternalEncoding;
   std::string OutputEncoding;
   //   arg_separators arg_separator;
   std::string VariablesOrder;

   bool IgnoreUserAbort;

   //   zend_llist tick_functions;
   bool ExposePhp;

   bool RegisterArgcArgv;
   bool AutoGlobalsJit;

   std::string DocrefRoot;
   std::string DocrefExt;

   bool HtmlErrors;
   bool ModulesActivated;
   bool DuringRequestStartup;
   bool AllowUrlFopen;
   bool ReportZendDebug;

   int LastErrorType;
   std::string LastErrorMessage;
   std::string LastErrorFile;
   int  LastErrorLineno;

   std::string PhpSysTempDir;

   std::string DisableFunctions;
   std::string DisableClasses;
   bool AllowUrlInclude;
#ifdef PHP_WIN32
   bool ComInitialized;
#endif
   zend_long MaxInputNestingLevel;
   zend_long MaxInputVars;
   bool InUserInclude;

   std::string UserIniFilename;
   zend_long UserIniCacheTtl;

   std::string RequestOrder;
   bool InErrorLog;

#ifdef PHP_WIN32
   bool WindowsShowCrtWarning;
#endif

   zend_long SyslogFacility;
   std::string SyslogIdent;
   bool HaveCalledOpenlog;
   zend_long SyslogFilter;
};

POLAR_DECL_EXPORT int php_execute_script(zend_file_handle *primary_file);
POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primary_file, zval *ret);

} // polar

#endif // POLARPHP_ARTIFACTS_EXEC_ENV_H
