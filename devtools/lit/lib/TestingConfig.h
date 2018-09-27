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

#ifndef POLAR_DEVLTOOLS_LIT_TESTING_CONFIG_H
#define POLAR_DEVLTOOLS_LIT_TESTING_CONFIG_H

#include <string>
#include <set>
#include <map>
#include <list>
#include <any>
#include <memory>

namespace polar {
namespace lit {

class LitConfig;
class TestFormat;
class TestingConfig;
using TestingConfigPointer = std::shared_ptr<TestingConfig>;
using LitConfigPointer = std::shared_ptr<LitConfig>;

class TestingConfig {
public:
   TestingConfig()
   {}
   TestingConfig(TestingConfig *parent, const std::string &name,
                 const std::set<std::string> &suffixes, std::shared_ptr<TestFormat> testFormat,
                 const std::map<std::string, std::string> &environment, const std::list<std::string> &substitutions,
                 bool unsupported, const std::optional<std::string> &testExecRoot,
                 const std::optional<std::string> &testSourceRoot, const std::set<std::string> &excludes,
                 const std::set<std::string> &availableFeatures, bool pipefail,
                 const std::set<std::string> &limitToFeatures = {}, bool isEarly = false,
                 const std::string &parallelismGroup = "")
      : m_parent(parent),
        m_name(name),
        m_suffixes(suffixes),
        m_testFormat(testFormat),
        m_environment(environment),
        m_substitutions(substitutions),
        m_unsupported(unsupported),
        m_testExecRoot(testExecRoot),
        m_testSourceRoot(testSourceRoot),
        m_excludes(excludes),
        m_availableFeatures(availableFeatures),
        m_pipefail(pipefail),
        m_limitToFeatures(limitToFeatures),
        m_isEarly(isEarly),
        m_parallelismGroup(parallelismGroup)
   {}
   ~TestingConfig();
public:
   static TestingConfigPointer fromDefaults(LitConfigPointer litConfig);
   TestingConfig *getParent();
   const std::string &getName();
   const std::set<std::string> &getSuffixes();
   const std::shared_ptr<TestFormat> getTestFormat();
   const std::map<std::string, std::string> &getEnvironment();
   const std::list<std::string> &getSubstitutions();
   bool isUnsupported();
   const std::optional<std::string> &getTestExecRoot();
   const std::optional<std::string> &getTestSourceRoot();
   const std::set<std::string> &getExcludes();
   const std::set<std::string> &getAvailableFeatures();
   bool isPipefail();
   const std::set<std::string> &getLimitToFeatures();
   bool isEarly() const;
   template <typename T>
   const T &getExtraConfig(const std::string &name, const T &defaultValue = T{});
   TestingConfig &setName(const std::string &name);
   TestingConfig &setSuffixes(const std::set<std::string> &suffixes);
   TestingConfig &setTestFormat(std::shared_ptr<TestFormat> testFormat);
   TestingConfig &setEnvironment(const std::map<std::string, std::string> &environment);
   TestingConfig &setSubstitutions(const std::list<std::string> &substitutions);
   TestingConfig &setIsUnsupported(bool flag);
   TestingConfig &setTestExecRoot(const std::optional<std::string> &root);
   TestingConfig &setTestSourceRoot(const std::optional<std::string> &root);
   TestingConfig &setExcludes(const std::set<std::string> &excludes);
   TestingConfig &setAvailableFeatures(const std::set<std::string> &features);
   TestingConfig &setPipeFail(bool flag);
   TestingConfig &setLimitToFeatures(const std::set<std::string> &features);
   TestingConfig &setIsEarly(bool flag);
   TestingConfig &setExtraConfig(const std::string &name, std::any value);
   void loadFromPath(const std::string &path, LitConfigPointer litConfig);
   void loadFromPath(const std::string &path, LitConfig &litConfig);
protected:
   TestingConfig *m_parent;
   std::string m_name;
   std::set<std::string> m_suffixes;
   std::shared_ptr<TestFormat> m_testFormat;
   std::map<std::string, std::string> m_environment;
   std::list<std::string> m_substitutions;
   bool m_unsupported;
   std::optional<std::string> m_testExecRoot;
   std::optional<std::string> m_testSourceRoot;
   std::set<std::string> m_excludes;
   std::set<std::string> m_availableFeatures;
   bool m_pipefail;
   std::set<std::string> m_limitToFeatures;
   bool m_isEarly;
   std::string m_parallelismGroup;
   std::map<std::string, std::any> m_extraConfig;
};

template <typename T>
const T &TestingConfig::getExtraConfig(const std::string &name, const T &defaultValue)
{
   if (m_extraConfig.find(name) != m_extraConfig.end()) {
      return std::any_cast<T &>(m_extraConfig.at(name));
   }
   return defaultValue;
}

} // lit
} // polar

#endif
