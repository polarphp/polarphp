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

ExecEnv &ExecEnv::setDisplayErrors(bool flag)
{
   m_displayErrors = flag;
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

ExecEnv &ExecEnv::setExposePhp(bool flag)
{
   m_exposePhp = flag;
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

ExecEnv &ExecEnv::setHtmlErrors(bool flag)
{
   m_htmlErrors = flag;
   return *this;
}

ExecEnv &ExecEnv::setModulesActivated(bool flag)
{
   m_modulesActivated = flag;
   return *this;
}

ExecEnv &ExecEnv::setDuringRequestStartup(bool flag)
{
   m_duringRequestStartup = flag;
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

ExecEnv &ExecEnv::setPolarBinary(const std::string &binary)
{
   m_polarBinary = binary;
   return *this;
}

ExecEnv &ExecEnv::setIniDefaultsHandler(IniConfigDefaultInitFunc handler)
{
   m_iniDefaultInitHandler = handler;
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

bool ExecEnv::getDisplayErrors() const
{
   return m_displayErrors;
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

bool ExecEnv::getExposePhp() const
{
   return m_exposePhp;
}

bool ExecEnv::getRegisterArgcArgv() const
{
   return m_registerArgcArgv;
}

bool ExecEnv::getAutoGlobalsJit() const
{
   return m_autoGlobalsJit;
}

bool ExecEnv::getHtmlErrors() const
{
   return m_htmlErrors;
}

bool ExecEnv::getModulesActivated() const
{
   return m_modulesActivated;
}

bool ExecEnv::getDuringRequestStartup() const
{
   return m_duringRequestStartup;
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
bool ExecEnv::getComInitialized()
{
   return m_comInitialized;
}
#endif

StringRef ExecEnv::getPhpIniPathOverride() const
{
   return m_phpIniPathOverride;
}

StringRef ExecEnv::getIniEntries() const
{
   return m_iniEntries;
}

StringRef ExecEnv::getPolarBinary() const
{
   return m_polarBinary;
}

IniConfigDefaultInitFunc ExecEnv::getIniConfigDeaultHandler() const
{
   return m_iniDefaultInitHandler;
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
