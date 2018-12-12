// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/28.

#include "TestingConfig.h"
#include "LitConfig.h"
#include "Utils.h"
#include "formats/Base.h"
#include "nlohmann/json.hpp"
#include "CfgSetterPluginLoader.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/StringRef.h"

#define CFG_SETTER_SITE_FILENAME "litsitecfg.cmake"
#define CFG_SETTER_LOCAL_FILENAME "litlocalcfg.cmake"
#define CFG_SETTER_NORMAL_FILENAME "litcfg.cmake"

namespace polar {
namespace lit {

using nlohmann::json;
using polar::basic::Twine;

const CfgSetterPlugin &retrieve_current_cfg_setter_plugin();

TestingConfig::~TestingConfig()
{
}

TestingConfigPointer TestingConfig::fromDefaults(LitConfigPointer litConfig)
{
   std::list<std::string> paths = litConfig->getPaths();
   paths.push_back(std::getenv("PATH"));
   std::list<std::string> environment;
   environment.push_back(Twine("PATH").concat("=").concat(join_string_list(paths, ":")).getStr());
   environment.push_back("POLARPHP_DISABLE_CRASH_REPORT=1");
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
         environment.push_back(Twine(envVarName).concat("=").concat(envVal).getStr());
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
   std::vector<std::string> availableFeatures;
   if (litConfig->isUseValgrind()) {
      availableFeatures.push_back("valgrind");
      if (litConfig->isValgrindLeakCheck()) {
         availableFeatures.push_back("vg_leak");
      }
   }
   return std::make_shared<TestingConfig>(nullptr,
                                          "<unnamed>",
                                          std::set<std::string>{},
                                          nullptr,
                                          environment,
                                          SubstitutionList{},
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

std::list<std::string> &TestingConfig::getEnvironment()
{
   return m_environment;
}

const SubstitutionList &TestingConfig::getSubstitutions()
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

const std::vector<std::string> &TestingConfig::getAvailableFeatures()
{
   return m_availableFeatures;
}

bool TestingConfig::isPipefail()
{
   return m_pipefail;
}

const std::vector<std::string> &TestingConfig::getLimitToFeatures()
{
   return m_limitToFeatures;
}

bool TestingConfig::hasExtraConfig(const std::string &name)
{
   return m_extraConfig.find(name) != m_extraConfig.end();
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

TestingConfig &TestingConfig::setTestFormat(std::shared_ptr<TestFormat> testFormat)
{
   m_testFormat = testFormat;
   return *this;
}

TestingConfig &TestingConfig::setEnvironment(const std::list<std::string> &environment)
{
   m_environment = environment;
   return *this;
}

TestingConfig &TestingConfig::addEnvironment(StringRef name, StringRef value)
{
   // first search
   auto iter = std::find_if(m_environment.begin(), m_environment.end(), [name](const std::string &item) {
      return StringRef(item).startsWith(name.trim());
   });
   Twine envItem = Twine(name.trim()).concat("=").concat(value.trim());
   if (iter != m_environment.end()) {
      *iter = envItem.getStr();
   } else {
      m_environment.push_back(envItem.getStr());
   }
   return *this;
}

TestingConfig &TestingConfig::setSubstitutions(const SubstitutionList &substitutions)
{
   m_substitutions = substitutions;
   return *this;
}

TestingConfig &TestingConfig::addSubstitution(StringRef name, const std::string &replacement)
{
   m_substitutions.emplace_back(name, replacement);
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

TestingConfig &TestingConfig::setAvailableFeatures(const std::vector<std::string> &features)
{
   m_availableFeatures = features;
   return *this;
}

TestingConfig &TestingConfig::addAvailableFeature(const std::string &feature)
{
   m_availableFeatures.push_back(feature);
   return *this;
}

TestingConfig &TestingConfig::setPipeFail(bool flag)
{
   m_pipefail = flag;
   return *this;
}

TestingConfig &TestingConfig::setLimitToFeatures(const std::vector<std::string> &features)
{
   m_limitToFeatures = features;
   return *this;
}

TestingConfig &TestingConfig::setParallelismGroup(const std::string &pgroup)
{
   m_parallelismGroup = pgroup;
   return *this;
}

TestingConfig &TestingConfig::setParallelismGroup(ParallelismGroupSetter handle)
{
   m_parallelismGroup = handle;
   return *this;
}

const std::any &TestingConfig::getParallelismGroup() const
{
   return m_parallelismGroup;
}

std::any &TestingConfig::getParallelismGroup()
{
   return const_cast<std::any &>(static_cast<const TestingConfig *>(this)->getParallelismGroup());
}

TestingConfig &TestingConfig::setIsEarly(bool flag)
{
   m_isEarly = flag;
   return *this;
}

TestingConfig &TestingConfig::setExtraConfig(const std::string &name, const char *value)
{
   m_extraConfig[name] = std::string(value);
   return *this;
}

TestingConfig &TestingConfig::setExtraConfig(const std::string &name, StringRef value)
{
   m_extraConfig[name] = value.getStr();
   return *this;
}

TestingConfig &TestingConfig::setExtraConfig(const std::string &name, int value)
{
   m_extraConfig[name] = value;
   return *this;
}

TestingConfig &TestingConfig::setExtraConfig(const std::string &name, bool value)
{
   m_extraConfig[name] = value;
   return *this;
}

void TestingConfig::loadFromPath(const std::string &path, LitConfigPointer litConfig)
{
   loadFromPath(path, *litConfig.get());
}

void TestingConfig::loadFromPath(const std::string &path, LitConfig &litConfig)
{
   stdfs::path fsPath = stdfs::path(path);
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
   if (cfgSetterName.at(0) == stdfs::path::preferred_separator) {
      cfgSetterName = cfgSetterName.substr(1);
   }
   cfgSetterName = cfgSetterName.replace(0, startupPath.size(), "");
   if (cfgSetterName.empty()) {
      cfgSetterName = "root";
   }
   cfgSetterName += "_" + cfgSetterSuffix;
   replace_string(std::string(1, stdfs::path::preferred_separator), "_", cfgSetterName);
   replace_string("-", "_", cfgSetterName);
   // if this method invoked, symbol must exist at normation situation
   // @TODO can ignore this exception ?
   CfgSetterType cfgSetter = cfgSetterPlugin.getCfgSetter<CfgSetterType>(cfgSetterName);
   cfgSetter(this, &litConfig);
}

} // lit
} // polar
