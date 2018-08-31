// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/30.

#include "LitConfig.h"
#include <cassert>
#include <iostream>

namespace polar {
namespace lit {

LitConfig::LitConfig(const std::string &progName, const std::list<std::string> &path,
                     bool quiet, bool useValgrind, bool valgrindLeakCheck,
                     const std::list<std::string> &valgrindArgs, bool noExecute, bool debug,
                     bool singleProcess, bool isWindows, const std::map<std::string, std::string> &params,
                     const std::optional<std::string> &configPrefix, int maxIndividualTestTime,
                     const std::optional<int> &maxFailures, const std::map<std::string, std::string> &parallelismGroups,
                     bool echoAllCommands)
   : m_progName(progName), m_path(path),
     m_quiet(quiet), m_useValgrind(useValgrind),
     m_valgrindLeakCheck(valgrindLeakCheck),
     m_valgrindUserArgs(valgrindArgs), m_noExecute(noExecute),
     m_debug(debug), m_singleProcess(singleProcess),
     m_isWindows(isWindows), m_params(params), m_bashPath(std::nullopt),
     m_configPrefix(configPrefix.has_value() ? configPrefix.value() : "lit"),
     m_suffixes({"cfg", "cfg.json"}), m_maxFailures(maxFailures),
     m_parallelismGroups(parallelismGroups), m_echoAllCommands(echoAllCommands)
{
   setMaxIndividualTestTime(maxIndividualTestTime);
   for (const std::string &suffix : m_suffixes) {
      m_configNames.push_back(m_configPrefix + "." + suffix);
      m_siteConfigNames.push_back(m_configPrefix + ".site." + suffix);
      m_localConfigNames.push_back(m_configPrefix + ".local." + suffix);
   }
   if (useValgrind) {
      m_valgrindArgs.push_back("valgrind");
      m_valgrindArgs.push_back("-q");
      m_valgrindArgs.push_back("--run-libc-freeres=no");
      m_valgrindArgs.push_back("--tool=memcheck");
      m_valgrindArgs.push_back("--trace-children=yes");
      m_valgrindArgs.push_back("--error-exitcode=123");
      if (m_valgrindLeakCheck) {
         m_valgrindArgs.push_back("--leak-check=full");
      } else {
         m_valgrindArgs.push_back("--leak-check=no");
      }
      for (const std::string &userArg : m_valgrindUserArgs) {
         m_valgrindArgs.push_back(userArg);
      }
   }
}

LitConfig &LitConfig::setMaxIndividualTestTime(int value)
{
   assert(value >= 0);
   m_maxIndividualTestTime = value;
}

void LitConfig::writeMessage(const std::string &kind, const std::string &message,
                             const std::string &file, const std::string &line)
{
   std::cerr << m_progName << ": " << file << ":" << line << " :" << kind << " :" << message
             << std::endl;
}

std::string LitConfig::getBashPath()
{
   if (m_bashPath.has_value()){
      return m_bashPath.value();
   }

}

std::string LitConfig::getToolsPath()
{

}

} // lit
} // polar
