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

#ifndef POLAR_DEVLTOOLS_LIT_TESTING_CONFIG_H
#define POLAR_DEVLTOOLS_LIT_TESTING_CONFIG_H

#include "ForwardDefs.h"

#include <string>
#include <map>
#include <list>
#include <any>
#include <memory>
#include <set>
#include <vector>

namespace polar {
namespace basic {
class StringRef;
} // lit
} // polar

namespace polar {
namespace lit {

using polar::basic::StringRef;

class TestingConfig {
public:
   TestingConfig()
   {}
   TestingConfig(TestingConfig *parent, const std::string &name,
                 const std::set<std::string> &suffixes, std::shared_ptr<TestFormat> testFormat,
                 const std::list<std::string> &environment, const SubstitutionList &substitutions,
                 bool unsupported, const std::optional<std::string> &testExecRoot,
                 const std::optional<std::string> &testSourceRoot, const std::set<std::string> &excludes,
                 const std::vector<std::string> &availableFeatures, bool pipefail,
                 const std::vector<std::string> &limitToFeatures = {}, bool isEarly = false,
                 const std::any &parallelismGroup = std::any{})
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
   std::list<std::string> &getEnvironment();
   const SubstitutionList &getSubstitutions();
   bool isUnsupported();
   const std::optional<std::string> &getTestExecRoot();
   const std::optional<std::string> &getTestSourceRoot();
   const std::set<std::string> &getExcludes();
   const std::vector<std::string> &getAvailableFeatures();
   bool isPipefail();
   const std::vector<std::string> &getLimitToFeatures();
   bool isEarly() const;
   bool hasExtraConfig(const std::string &name);
   template <typename T>
   const T &getExtraConfig(const std::string &name, const T &defaultValue = T{});
   TestingConfig &setName(const std::string &name);
   TestingConfig &setSuffixes(const std::set<std::string> &suffixes);
   TestingConfig &setTestFormat(std::shared_ptr<TestFormat> testFormat);
   TestingConfig &setEnvironment(const std::list<std::string> &environment);
   TestingConfig &addEnvironment(StringRef name, StringRef value);
   TestingConfig &setSubstitutions(const SubstitutionList &substitutions);
   TestingConfig &addSubstitution(StringRef name, const std::string &replacement);
   TestingConfig &setIsUnsupported(bool flag);
   TestingConfig &setTestExecRoot(const std::optional<std::string> &root);
   TestingConfig &setTestSourceRoot(const std::optional<std::string> &root);
   TestingConfig &setExcludes(const std::set<std::string> &excludes);
   TestingConfig &setAvailableFeatures(const std::vector<std::string> &features);
   TestingConfig &addAvailableFeature(const std::string &feature);
   TestingConfig &setPipeFail(bool flag);
   TestingConfig &setLimitToFeatures(const std::vector<std::string> &features);
   TestingConfig &setParallelismGroup(const std::string &pgroup);
   TestingConfig &setParallelismGroup(ParallelismGroupSetter handle);
   const std::any &getParallelismGroup() const;
   std::any &getParallelismGroup();
   TestingConfig &setIsEarly(bool flag);

   TestingConfig &setExtraConfig(const std::string &name, const char *value);
   TestingConfig &setExtraConfig(const std::string &name, StringRef value);
   TestingConfig &setExtraConfig(const std::string &name, int value);
   TestingConfig &setExtraConfig(const std::string &name, bool value);
   void loadFromPath(const std::string &path, LitConfigPointer litConfig);
   void loadFromPath(const std::string &path, LitConfig &litConfig);
protected:
   TestingConfig *m_parent;
   std::string m_name;
   std::set<std::string> m_suffixes;
   std::shared_ptr<TestFormat> m_testFormat;
   std::list<std::string> m_environment;
   SubstitutionList m_substitutions;
   bool m_unsupported;
   std::optional<std::string> m_testExecRoot;
   std::optional<std::string> m_testSourceRoot;
   std::set<std::string> m_excludes;
   std::vector<std::string> m_availableFeatures;
   bool m_pipefail;
   std::vector<std::string> m_limitToFeatures;
   bool m_isEarly;
   std::any m_parallelismGroup;
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
