// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/28.

#include "TestingConfig.h"
#include "LitConfig.h"
#include "Utils.h"
#include "formats/Base.h"
#include "nlohmann/json.hpp"
#include "CfgSetterPluginLoader.h"
#include <iostream>

#define CFG_SETTER_SITE_FILENAME "litsitecfg.cmake"
#define CFG_SETTER_LOCAL_FILENAME "litlocalcfg.cmake"
#define CFG_SETTER_NORMAL_FILENAME "litcfg.cmake"

namespace polar {
namespace lit {

using nlohmann::json;

const CfgSetterPlugin &retrieve_current_cfg_setter_plugin();

TestingConfig::~TestingConfig()
{
}

TestingConfigPointer TestingConfig::fromDefaults(LitConfigPointer litConfig)
{
   std::list<std::string> paths = litConfig->getPaths();
   paths.push_back(std::getenv("PATH"));
   std::map<std::string, std::string> environment;
   environment["PATH"] = join_string_list(paths, ":");
   environment["POLARPHP_DISABLE_CRASH_REPORT"] = "1";
   std::list<std::string> passVars = {
      "LIBRARY_PATH", "LD_LIBRARY_PATH", "SYSTEMROOT", "TERM",
      "CLANG", "LD_PRELOAD", "ASAN_OPTIONS", "UBSAN_OPTIONS",
      "LSAN_OPTIONS", "ADB", "ANDROID_SERIAL",
      "SANITIZER_IGNORE_CVE_2016_2143", "TMPDIR", "TMP", "TEMP",
      "TEMPDIR", "AVRLIT_BOARD", "AVRLIT_PORT",
      "FILECHECK_DUMP_INPUT_ON_FAILURE"
   };
   for (const std::string &envVarName : passVars) {
      const char *envStr = std::getenv(envVarName.c_str());
      std::string envVal;
      if (envStr) {
         envVal = std::string(envStr, strlen(envStr));
      }
      if (!envVal.empty()) {
         environment[envVarName] = envVal;
      }
   }
#ifdef POLAR_OS_WIN32
   m_environment["INCLUDE"] = std::getenv("INCLUDE");
   m_environment["PATHEXT"] = std::getenv("PATHEXT");
   m_environment["PYTHONUNBUFFERED"] = "1";
   m_environment["TEMP"] = std::getenv("TEMP");
   m_environment["TMP"] = std::getenv("TMP");
#endif
   // Set the default available features based on the LitConfig.
   std::set<std::string> availableFeatures;
   if (litConfig->isUseValgrind()) {
      availableFeatures.insert("valgrind");
      if (litConfig->isValgrindLeakCheck()) {
         availableFeatures.insert("vg_leak");
      }
   }
   return std::make_shared<TestingConfig>(nullptr,
                                          "<unnamed>",
                                          std::set<std::string>{},
                                          nullptr,
                                          environment,
                                          std::list<std::string>{},
                                          false,
                                          std::nullopt,
                                          std::nullopt,
                                          std::set<std::string>{},
                                          availableFeatures,
                                          true);

}

TestingConfig *TestingConfig::getParent()
{
   if (nullptr != m_parent) {
      return m_parent->getParent();
   } else {
      return this;
   }
}

const std::string &TestingConfig::getName()
{
   return m_name;
}

const std::set<std::string> &TestingConfig::getSuffixes()
{
   return m_suffixes;
}

const std::shared_ptr<TestFormat> TestingConfig::getTestFormat()
{
   return m_testFormat;
}

const std::map<std::string, std::string> &TestingConfig::getEnvironment()
{
   return m_environment;
}

const std::list<std::string> &TestingConfig::getSubstitutions()
{
   return m_substitutions;
}

bool TestingConfig::isUnsupported()
{
   return m_unsupported;
}

const std::optional<std::string> &TestingConfig::getTestExecRoot()
{
   return m_testExecRoot;
}

const std::optional<std::string> &TestingConfig::getTestSourceRoot()
{
   return m_testSourceRoot;
}

const std::set<std::string> &TestingConfig::getExcludes()
{
   return m_excludes;
}

const std::set<std::string> &TestingConfig::getAvailableFeatures()
{
   return m_availableFeatures;
}

bool TestingConfig::isPipefail()
{
   return m_pipefail;
}

const std::set<std::string> &TestingConfig::getLimitToFeatures()
{
   return m_limitToFeatures;
}

bool TestingConfig::isEarly() const
{
   return m_isEarly;
}

TestingConfig &TestingConfig::setName(const std::string &name)
{
   m_name = name;
   return *this;
}

TestingConfig &TestingConfig::setSuffixes(const std::set<std::string> &suffixes)
{
   m_suffixes = suffixes;
   return *this;
}

TestingConfig &TestingConfig::setTestformat(std::shared_ptr<TestFormat> testFormat)
{
   m_testFormat = testFormat;
   return *this;
}

TestingConfig &TestingConfig::setEnvironment(const std::map<std::string, std::string> &environment)
{
   m_environment = environment;
   return *this;
}

TestingConfig &TestingConfig::setSubstitutions(const std::list<std::string> &substitutions)
{
   m_substitutions = substitutions;
   return *this;
}

TestingConfig &TestingConfig::setIsUnsupported(bool flag)
{
   m_unsupported = flag;
   return *this;
}

TestingConfig &TestingConfig::setTestExecRoot(const std::optional<std::string> &root)
{
   m_testExecRoot = root;
   return *this;
}

TestingConfig &TestingConfig::setTestSourceRoot(const std::optional<std::string> &root)
{
   m_testSourceRoot = root;
   return *this;
}

TestingConfig &TestingConfig::setExcludes(const std::set<std::string> &excludes)
{
   m_excludes = excludes;
   return *this;
}

TestingConfig &TestingConfig::setAvailableFeatures(const std::set<std::string> &features)
{
   m_availableFeatures = features;
   return *this;
}

TestingConfig &TestingConfig::setPipeFail(bool flag)
{
   m_pipefail = flag;
   return *this;
}

TestingConfig &TestingConfig::setLimitToFeatures(const std::set<std::string> &features)
{
   m_limitToFeatures = features;
   return *this;
}

TestingConfig &TestingConfig::setIsEarly(bool flag)
{
   m_isEarly = flag;
   return *this;
}

void TestingConfig::loadFromPath(const std::string &path, LitConfigPointer litConfig)
{
   loadFromPath(path, *litConfig.get());
}

void TestingConfig::loadFromPath(const std::string &path, LitConfig &litConfig)
{
   fs::path fsPath = fs::path(path);
   std::string cfgSetterName = fsPath.parent_path();
   std::string filename = fsPath.filename();
   std::string cfgSetterSuffix;
   const CfgSetterPlugin &cfgSetterPlugin = retrieve_current_cfg_setter_plugin();
   if (filename == CFG_SETTER_NORMAL_FILENAME) {
      cfgSetterSuffix = "cfgsetter";
   } else if (filename == CFG_SETTER_LOCAL_FILENAME) {
      cfgSetterSuffix = "local_cfgsetter";
   } else {
      // CFG_SETTER_SITE_FILENAME
      cfgSetterSuffix = "site_cfgsetter";
   }
   const std::string &startupPath = cfgSetterPlugin.getStartupPath();
   if (!string_startswith(cfgSetterName, startupPath)) {
      return;
   }
   if (cfgSetterName.at(0) == fs::path::preferred_separator) {
      cfgSetterName = cfgSetterName.substr(1);
   }
   cfgSetterName = cfgSetterName.replace(0, startupPath.size(), "");
   if (cfgSetterName.empty()) {
      cfgSetterName = "root";
   }
   cfgSetterName += "_" + cfgSetterSuffix;
   replace_string(std::string(1, fs::path::preferred_separator), "_", cfgSetterName);
   // if this method invoked, symbol must exist at normation situation
   // @TODO can ignore this exception ?
   CfgSetterType cfgSetter = cfgSetterPlugin.getCfgSetter<CfgSetterType>(cfgSetterName);
   cfgSetter(this, &litConfig);
}

} // lit
} // polar
