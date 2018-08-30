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

#ifndef POLAR_DEVLTOOLS_LIT_LITCONFIG_H
#define POLAR_DEVLTOOLS_LIT_LITCONFIG_H

#include <string>
#include <list>
#include <map>
#include <optional>

namespace polar {
namespace lit {

class LitConfig
{
protected:
   std::string m_progName;
   std::list<std::string> m_path;
   bool m_quiet;
   bool m_useValgrind;
   bool m_useValgrindLeakCheck;
   std::list<std::string> m_valgrindUserArgs;
   bool m_noExecute;
   bool m_debug;
   bool m_singleProcess;
   bool m_isWindows;
   std::map<std::string, std::string> m_params;
   std::optional<std::string> m_bashPath;
   std::string m_configPrefix;
   std::list<std::string> m_suffixes;
   std::list<std::string> m_configNames;
   std::list<std::string> m_siteConfigNames;
   std::list<std::string> m_localConfigNames;
   int m_numErrors;
   int m_numWarnings;
   std::list<std::string> m_valgrindArgs;
   int m_maxIndividualTestTime;
   int m_maxFailures;
   std::map<std::string, std::string> m_parallelismGroups;
   bool m_echoAllCommands;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_LITCONFIG_H
