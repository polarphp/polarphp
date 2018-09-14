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

namespace polar {
namespace lit {

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
      std::string envVal = std::getenv(envVarName.c_str());
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
                                          std::nullopt,
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

const std::optional<std::shared_ptr<TestFormat>> &TestingConfig::getTestFormat()
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

void TestingConfig::loadFromPath(const std::string &path, const LitConfig &litConfig)
{

}

} // lit
} // polar
