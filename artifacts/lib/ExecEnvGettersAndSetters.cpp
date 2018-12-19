// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/16.

#include "ExecEnv.h"

namespace polar {

ExecEnv &ExecEnv::setArgc(int argc)
{
   m_argc = argc;
   return *this;
}

ExecEnv &ExecEnv::setArgv(const std::vector<StringRef> &argv)
{
   m_argv = argv;
   return *this;
}

ExecEnv &ExecEnv::setArgv(char **argv)
{
   std::vector<StringRef> tempArgv;
   char **arg = argv;
   while (*arg != nullptr) {
      tempArgv.push_back(*arg);
      ++arg;
   }
   if (!tempArgv.empty()) {
      m_argv = tempArgv;
   }
   return *this;
}

ExecEnv &ExecEnv::setPhpIniIgnore(bool flag)
{
   m_phpIniIgnore = flag;
   return *this;
}

ExecEnv &ExecEnv::setPhpIniIgnoreCwd(bool flag)
{
   m_phpIniIgnoreCwd = flag;
   return *this;
}

ExecEnv &ExecEnv::setImplicitFlush(bool flag)
{
   m_implicitFlush = flag;
   return *this;
}

ExecEnv &ExecEnv::setEnableDl(bool flag)
{
   m_enableDl = flag;
   return *this;
}

ExecEnv &ExecEnv::setTrackErrors(bool flag)
{
   m_trackErrors = flag;
   return *this;
}

ExecEnv &ExecEnv::setDisplayStartupErrors(bool flag)
{
   m_displayStartupErrors = flag;
   return *this;
}

ExecEnv &ExecEnv::setLogErrors(bool flag)
{
   m_logErrors = flag;
   return *this;
}

ExecEnv &ExecEnv::setIgnoreRepeatedErrors(bool flag)
{
   m_ignoreRepeatedErrors = flag;
   return *this;
}

ExecEnv &ExecEnv::setIgnoreRepeatedSource(bool flag)
{
   m_ignoreRepeatedSource = flag;
   return *this;
}

ExecEnv &ExecEnv::setReportMemLeaks(bool flag)
{
   m_reportMemLeaks = flag;
   return *this;
}

ExecEnv &ExecEnv::setIgnoreUserAbort(bool flag)
{
   m_ignoreUserAbort = flag;
   return *this;
}

ExecEnv &ExecEnv::setRegisterArgcArgv(bool flag)
{
   m_registerArgcArgv = flag;
   return *this;
}

ExecEnv &ExecEnv::setAutoGlobalsJit(bool flag)
{
   m_autoGlobalsJit = flag;
   return *this;
}

ExecEnv &ExecEnv::setModulesActivated(bool flag)
{
   m_modulesActivated = flag;
   return *this;
}

ExecEnv &ExecEnv::setDuringExecEnvStartup(bool flag)
{
   m_duringExecEnvStartup = flag;
   return *this;
}

ExecEnv &ExecEnv::setAllowUrlFopen(bool flag)
{
   m_allowUrlFopen = flag;
   return *this;
}

ExecEnv &ExecEnv::setReportZendDebug(bool flag)
{
   m_reportZendDebug = flag;
   return *this;
}

ExecEnv &ExecEnv::setInErrorLog(bool flag)
{
   m_inErrorLog = flag;
   return *this;
}

ExecEnv &ExecEnv::setInUserInclude(bool flag)
{
   m_inUserInclude = flag;
   return *this;
}
#ifdef POLAR_OS_WIN32
ExecEnv &ExecEnv::setWindowsShowCrtWarning(bool flag)
{
   m_windowsShowCrtWarning = flag;
   return *this;
}

#endif
ExecEnv &ExecEnv::setHaveCalledOpenlog(bool flag)
{
   m_haveCalledOpenlog = flag;
   return *this;
}

ExecEnv &ExecEnv::setAllowUrlInclude(bool flag)
{
   m_allowUrlInclude = flag;
   return *this;
}

#ifdef POLAR_OS_WIN32
ExecEnv &ExecEnv::setComInitialized(bool flag)
{
   m_comInitialized = flag;
   return *this;
}
#endif

ExecEnv &ExecEnv::setStarted(bool flag)
{
   m_started = flag;
   return *this;
}

ExecEnv &ExecEnv::setDisplayErrors(std::uint8_t value)
{
   m_displayErrors = value;
   return *this;
}

ExecEnv &ExecEnv::setSerializePrecision(zend_long value)
{
   m_serializePrecision = value;
   return *this;
}

ExecEnv &ExecEnv::setMemoryLimit(zend_long value)
{
   m_memoryLimit = value;
   return *this;
}

ExecEnv &ExecEnv::setOutputBuffering(zend_long value)
{
   m_outputBuffering = value;
   return *this;
}

ExecEnv &ExecEnv::setLogErrorsMaxLen(zend_long value)
{
   m_logErrorsMaxLen = value;
   return *this;
}

ExecEnv &ExecEnv::setMaxInputNestingLevel(zend_long value)
{
   m_maxInputNestingLevel = value;
   return *this;
}

ExecEnv &ExecEnv::setMaxInputVars(zend_long value)
{
   m_maxInputVars = value;
   return *this;
}

ExecEnv &ExecEnv::setUserIniCacheTtl(zend_long value)
{
   m_userIniCacheTtl = value;
   return *this;
}

ExecEnv &ExecEnv::setSyslogFacility(zend_long value)
{
   m_syslogFacility = value;
   return *this;
}

ExecEnv &ExecEnv::setSyslogFilter(zend_long value)
{
   m_syslogFilter = value;
   return *this;
}

ExecEnv &ExecEnv::setDefaultSocketTimeout(zend_long value)
{
   m_defaultSocketTimeout = value;
   return *this;
}

ExecEnv &ExecEnv::setPhpIniPathOverride(const std::string &path)
{
   m_phpIniPathOverride = path;
   return *this;
}

ExecEnv &ExecEnv::setInitEntries(const std::string &entries)
{
   m_iniEntries = entries;
   return *this;
}

ExecEnv &ExecEnv::setOutputHandler(const std::string &value)
{
   m_outputHandler = value;
   return *this;
}

ExecEnv &ExecEnv::setUnserializeCallbackFunc(const std::string &value)
{
   m_unserializeCallbackFunc = value;
   return *this;
}

ExecEnv &ExecEnv::setErrorLog(const std::string &value)
{
   m_errorLog = value;
   return *this;
}

ExecEnv &ExecEnv::setDocRoot(const std::string &value)
{
   m_docRoot = value;
   return *this;
}

ExecEnv &ExecEnv::setUserDir(const std::string &value)
{
   m_userDir = value;
   return *this;
}

ExecEnv &ExecEnv::setIncludePath(const std::string &value)
{
   m_includePath = value;
   return *this;
}

ExecEnv &ExecEnv::setOpenBaseDir(const std::string &value)
{
   m_openBaseDir = value;
   return *this;
}

ExecEnv &ExecEnv::setExtensionDir(const std::string &value)
{
   m_extensionDir = value;
   return *this;
}

ExecEnv &ExecEnv::setPolarBinary(const std::string &value)
{
   m_polarBinary = value;
   return *this;
}

ExecEnv &ExecEnv::setSysTempDir(const std::string &value)
{
   m_sysTempDir = value;
   return *this;
}

ExecEnv &ExecEnv::setErrorAppendString(const std::string &value)
{
   m_errorAppendString = value;
   return *this;
}

ExecEnv &ExecEnv::setErrorPrependString(const std::string &value)
{
   m_errorPrependString = value;
   return *this;
}

ExecEnv &ExecEnv::setAutoPrependFile(const std::string &value)
{
   m_autoPrependFile = value;
   return *this;
}

ExecEnv &ExecEnv::setAutoAppendFile(const std::string &value)
{
   m_autoAppendFile = value;
   return *this;
}

ExecEnv &ExecEnv::setInputEncoding(const std::string &value)
{
   m_inputEncoding = value;
   return *this;
}

ExecEnv &ExecEnv::setInternalEncoding(const std::string &value)
{
   m_internalEncoding = value;
   return *this;
}

ExecEnv &ExecEnv::setOutputEncoding(const std::string &value)
{
   m_outputEncoding = value;
   return *this;
}

ExecEnv &ExecEnv::setVariablesOrder(const std::string &value)
{
   m_variablesOrder = value;
   return *this;
}

ExecEnv &ExecEnv::setLastErrorMessage(const std::string &value)
{
   m_lastErrorMessage = value;
   return *this;
}

ExecEnv &ExecEnv::setLastErrorFile(const std::string &value)
{
   m_lastErrorFile = value;
   return *this;
}

ExecEnv &ExecEnv::setPhpSysTempDir(const std::string &value)
{
   m_phpSysTempDir = value;
   return *this;
}

ExecEnv &ExecEnv::setDisableFunctions(const std::string &value)
{
   m_disableFunctions = value;
   return *this;
}

ExecEnv &ExecEnv::setDisableClasses(const std::string &value)
{
   m_disableClasses = value;
   return *this;
}

ExecEnv &ExecEnv::setDocrefRoot(const std::string &value)
{
   m_docrefRoot = value;
   return *this;
}

ExecEnv &ExecEnv::setDocrefExt(const std::string &value)
{
   m_docrefExt = value;
   return *this;
}

ExecEnv &ExecEnv::setUserIniFilename(const std::string &value)
{
   m_userIniFilename = value;
   return *this;
}

ExecEnv &ExecEnv::setRequestOrder(const std::string &value)
{
   m_requestOrder = value;
   return *this;
}

ExecEnv &ExecEnv::setSyslogIdent(const std::string &value)
{
   m_syslogIdent = value;
   return *this;
}

ExecEnv &ExecEnv::setEntryScriptFilename(const std::string &value)
{
   m_entryScriptFilename = value;
   return *this;
}

ExecEnv &ExecEnv::setIniDefaultsHandler(IniConfigDefaultInitFunc handler)
{
   m_iniDefaultInitHandler = handler;
   return *this;
}

ExecEnv &ExecEnv::setLastErrorType(int type)
{
   m_lastErrorType = type;
   return *this;
}

ExecEnv &ExecEnv::setLastErrorLineno(int line)
{
   m_lastErrorLineno = line;
   return *this;
}

ExecEnv &ExecEnv::setIniConfigDeaultHandler(IniConfigDefaultInitFunc func)
{
   m_iniDefaultInitHandler = func;
   return *this;
}

ExecEnv &ExecEnv::setArgSeparator(ArgSeparators seps)
{
   m_argSeparator = seps;
   return *this;
}

const std::vector<StringRef> &ExecEnv::getArgv() const
{
   return m_argv;
}

int ExecEnv::getArgc() const
{
   return m_argc;
}

bool ExecEnv::getPhpIniIgnore() const
{
   return m_phpIniIgnore;
}

bool ExecEnv::getPhpIniIgnoreCwd() const
{
   return m_phpIniIgnoreCwd;
}

bool ExecEnv::getImplicitFlush() const
{
   return m_implicitFlush;
}

bool ExecEnv::getEnableDl() const
{
   return m_enableDl;
}

bool ExecEnv::getTrackErrors() const
{
   return m_trackErrors;
}

bool ExecEnv::getDisplayStartupErrors() const
{
   return m_displayStartupErrors;
}

bool ExecEnv::getLogErrors() const
{
   return m_logErrors;
}

bool ExecEnv::getIgnoreRepeatedErrors() const
{
   return m_ignoreRepeatedErrors;
}

bool ExecEnv::getIgnoreRepeatedSource() const
{
   return m_ignoreRepeatedSource;
}

bool ExecEnv::getReportMemLeaks() const
{
   return m_reportMemLeaks;
}

bool ExecEnv::getIgnoreUserAbort() const
{
   return m_ignoreUserAbort;
}

bool ExecEnv::getRegisterArgcArgv() const
{
   return m_registerArgcArgv;
}

bool ExecEnv::getAutoGlobalsJit() const
{
   return m_autoGlobalsJit;
}

bool ExecEnv::getModulesActivated() const
{
   return m_modulesActivated;
}

bool ExecEnv::getDuringExecEnvStartup() const
{
   return m_duringExecEnvStartup;
}

bool ExecEnv::getAllowUrlFopen() const
{
   return m_allowUrlFopen;
}

bool ExecEnv::getReportZendDebug() const
{
   return m_reportZendDebug;
}

bool ExecEnv::getInErrorLog() const
{
   return m_inErrorLog;
}

bool ExecEnv::getInUserInclude() const
{
   return m_inUserInclude;
}

#ifdef POLAR_OS_WIN32
bool ExecEnv::getWindowsShowCrtWarning() const
{
   return m_inWindowShowCrtWarning;
}

#endif
bool ExecEnv::getHaveCalledOpenlog() const
{
   return m_haveCalledOpenlog;
}

bool ExecEnv::getAllowUrlInclude() const
{
   return m_allowUrlFopen;
}

#ifdef POLAR_OS_WIN32
bool ExecEnv::getComInitialized() const
{
   return m_comInitialized;
}
#endif

bool ExecEnv::getStarted() const
{
   return m_started;
}

std::uint8_t ExecEnv::getDisplayErrors() const
{
   return m_displayErrors;
}

int ExecEnv::getLastErrorType() const
{
   return m_lastErrorType;
}

int ExecEnv::getLastErrorLineno() const
{
   return m_lastErrorLineno;
}

zend_long ExecEnv::getSerializePrecision() const
{
   return m_serializePrecision;
}

zend_long ExecEnv::getMemoryLimit() const
{
   return m_memoryLimit;
}

zend_long ExecEnv::getOutputBuffering() const
{
   return m_outputBuffering;
}

zend_long ExecEnv::getLogErrorsMaxLen() const
{
   return m_logErrorsMaxLen;
}

zend_long ExecEnv::getMaxInputNestingLevel() const
{
   return m_maxInputNestingLevel;
}

zend_long ExecEnv::getMaxInputVars() const
{
   return m_maxInputVars;
}

zend_long ExecEnv::getUserIniCacheTtl() const
{
   return m_userIniCacheTtl;
}

zend_long ExecEnv::getSyslogFacility() const
{
   return m_syslogFacility;
}

zend_long ExecEnv::getSyslogFilter() const
{
   return m_syslogFilter;
}

zend_long ExecEnv::getDefaultSocketTimeout() const
{
   return m_defaultSocketTimeout;
}

StringRef ExecEnv::getPhpIniPathOverride() const
{
   return m_phpIniPathOverride;
}

StringRef ExecEnv::getIniEntries() const
{
   return m_iniEntries;
}

StringRef ExecEnv::getOutputHandler() const
{
   return m_outputHandler;
}

StringRef ExecEnv::getUnserializeCallbackFunc() const
{
   return m_unserializeCallbackFunc;
}

StringRef ExecEnv::getErrorLog() const
{
   return m_errorLog;
}

StringRef ExecEnv::getDocRoot() const
{
   return m_docrefRoot;
}

StringRef ExecEnv::getUserDir() const
{
   return m_userDir;
}

StringRef ExecEnv::getIncludePath() const
{
   return m_includePath;
}

StringRef ExecEnv::getOpenBaseDir() const
{
   return m_openBaseDir;
}

StringRef ExecEnv::getExtensionDir() const
{
   return m_extensionDir;
}

StringRef ExecEnv::getPolarBinary() const
{
   return m_polarBinary;
}

StringRef ExecEnv::getSysTempDir() const
{
   return m_sysTempDir;
}

StringRef ExecEnv::getErrorAppendString() const
{
   return m_errorAppendString;
}

StringRef ExecEnv::getErrorPrependString() const
{
   return m_errorPrependString;
}

StringRef ExecEnv::getAutoPrependFile() const
{
   return m_autoPrependFile;
}

StringRef ExecEnv::getAutoAppendFile() const
{
   return m_autoAppendFile;
}

StringRef ExecEnv::getInputEncoding() const
{
   return m_inputEncoding;
}

StringRef ExecEnv::getInternalEncoding() const
{
   return m_internalEncoding;
}

StringRef ExecEnv::getOutputEncoding() const
{
   return m_outputEncoding;
}

StringRef ExecEnv::getVariablesOrder() const
{
   return m_variablesOrder;
}

StringRef ExecEnv::getLastErrorMessage() const
{
   return m_lastErrorMessage;
}

StringRef ExecEnv::getLastErrorFile() const
{
   return m_lastErrorFile;
}

StringRef ExecEnv::getPhpSysTempDir() const
{
   return m_phpSysTempDir;
}

StringRef ExecEnv::getDisableFunctions() const
{
   return m_disableFunctions;
}

StringRef ExecEnv::getDisableClasses() const
{
   return m_disableClasses;
}

StringRef ExecEnv::getDocrefRoot() const
{
   return m_docrefRoot;
}

StringRef ExecEnv::getDocrefExt() const
{
   return m_docrefExt;
}

StringRef ExecEnv::getUserIniFilename() const
{
   return m_userIniFilename;
}

StringRef ExecEnv::getRequestOrder() const
{
   return m_requestOrder;
}

StringRef ExecEnv::getSyslogIdent() const
{
   return m_syslogIdent;
}

StringRef ExecEnv::getEntryScriptFilename() const
{
   return m_entryScriptFilename;
}

IniConfigDefaultInitFunc ExecEnv::getIniConfigDeaultHandler() const
{
   return m_iniDefaultInitHandler;
}

const ArgSeparators &ExecEnv::getArgSeparator() const
{
   return m_argSeparator;
}

const zend_llist &ExecEnv::getTickFunctions() const
{
   return m_tickFunctions;
}

zend_llist &ExecEnv::getTickFunctions()
{
   return m_tickFunctions;
}

StringRef ExecEnv::getExecutableFilepath() const
{
   assert(m_argv.size() > 0);
   return m_argv[0];
}

} // polar
