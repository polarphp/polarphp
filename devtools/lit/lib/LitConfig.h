// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/30.

#ifndef POLAR_DEVLTOOLS_LIT_LITCONFIG_H
#define POLAR_DEVLTOOLS_LIT_LITCONFIG_H

#include <string>
#include <list>
#include <map>
#include <optional>
#include <any>
#include <memory>

namespace polar {
namespace lit {

class TestingConfig;
class LitConfig;
using LitConfigPointer = std::shared_ptr<LitConfig>;
using TestingConfigPointer = std::shared_ptr<TestingConfig>;

class LitConfig
{
public:
   LitConfig(const std::string &progName, const std::list<std::string> &path,
             bool quiet, bool useValgrind,
             bool valgrindLeakCheck, const std::list<std::string> &valgrindArgs,
             bool noExecute, bool singleProcess, bool debug, bool isWindows,
             const std::map<std::string, std::string> &params,
             const std::string &cfgSetterPluginDir,
             const std::optional<std::string> &configPrefix = std::nullopt,
             int maxIndividualTestTime = 0, const std::optional<int> &maxFailures = std::nullopt,
             const std::map<std::string, int> &parallelismGroups = std::map<std::string, int>{},
             bool echoAllCommands = false);

   int getMaxIndividualTestTime()
   {
      return m_maxIndividualTestTime;
   }

   LitConfig &setMaxIndividualTestTime(int value);

   void note(const std::string &message,
             const std::string &file = "", int line = -1) const
   {
      writeMessage("note", message, file, line);
   }

   void warning(const std::string &message,
                const std::string &file = "", int line = -1) const
   {
      writeMessage("warning", message, file, line);
      m_numWarnings += 1;
   }
   void error(const std::string &message,
              const std::string &file = "", int line = -1)
   {
      writeMessage("error", message, file, line);
      m_numErrors += 1;
   }

   void fatal(const std::string &message,
              const std::string &file = "", int line = -1)
   {
      writeMessage("fatal", message, file, line);
      exit(2);
   }

   TestingConfigPointer loadConfig(TestingConfigPointer config, const std::string &path);
   std::string getBashPath();
   std::optional<std::string> getToolsPath(std::optional<std::string> dir, const std::string &paths,
                            const std::list<std::string> &tools);

   const std::string &getProgName() const;
   const std::list<std::string> &getPaths() const;
   bool isQuiet() const;
   bool isUseValgrind() const;
   bool isValgrindLeakCheck() const;
   const std::list<std::string> &getValgrindUserArgs() const;
   bool isNoExecute() const;
   bool isDebug() const;
   bool isSingleProcess() const;
   bool isWindows() const;
   const std::map<std::string, std::string> &getParams() const;
   bool hasParam(const std::string &name) const;
   const std::string &getParam(const std::string &name, const std::string &defaultValue = "") const;
   const std::string &getCfgSetterPluginDir() const;
   const std::optional<std::string> &getBashPath() const;
   const std::string &getConfigPrefix() const;
   const std::list<std::string> &getSuffixes() const;
   const std::list<std::string> &getConfigNames() const;
   const std::list<std::string> &getSiteConfigNames() const;
   const std::list<std::string> &getLocalConfigNames() const;
   int getNumErrors() const;
   int getNumWarnings() const;
   const std::list<std::string> &getValgrindArgs() const;
   int getMaxIndividualTestTime() const;
   const std::optional<int> &getMaxFailures() const;
   const std::map<std::string, int> &getParallelismGroups() const;
   bool isEchoAllCommands() const;

private:
   void writeMessage(const std::string &kind, const std::string &message,
                     const std::string &file = "", int line = -1) const;
protected:
   std::string m_progName;
   std::list<std::string> m_path;
   bool m_quiet;
   bool m_useValgrind;
   bool m_valgrindLeakCheck;
   std::list<std::string> m_valgrindUserArgs;
   bool m_noExecute;
   bool m_singleProcess;
   bool m_debug;
   bool m_isWindows;
   std::map<std::string, std::string> m_params;
   std::string m_cfgSetterPluginDir;
   std::optional<std::string> m_bashPath;
   std::string m_configPrefix;
   std::list<std::string> m_suffixes;
   std::list<std::string> m_configNames;
   std::list<std::string> m_siteConfigNames;
   std::list<std::string> m_localConfigNames;
   int m_numErrors;
   mutable int m_numWarnings;
   std::list<std::string> m_valgrindArgs;
   int m_maxIndividualTestTime;
   std::optional<int> m_maxFailures;
   std::map<std::string, int> m_parallelismGroups;
   bool m_echoAllCommands;
};

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_LITCONFIG_H
