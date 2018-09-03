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

namespace polar {
namespace lit {

class LitConfig;

class TestingConfig {
public:
   TestingConfig(TestingConfig *parent, const std::string &name,
                 const std::set<std::string> &suffixes, const std::optional<std::string> &testFormat,
                 const std::map<std::string, std::string> &environment, const std::list<std::string> &substitutions,
                 bool unsupported, const std::optional<std::string> &testExecRoot,
                 const std::optional<std::string> &testSourceRoot, const std::set<std::string> &excludes,
                 const std::set<std::string> &availableFeatures, bool pipefai,
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
        m_pipefail(pipefai),
        m_limitToFeatures(limitToFeatures),
        m_isEarly(isEarly),
        m_parallelismGroup(parallelismGroup)
   {}
public:
   TestingConfig fromDefaults(const LitConfig &litConfig);

protected:
   TestingConfig *m_parent;
   std::string m_name;
   std::set<std::string> m_suffixes;
   std::optional<std::string> m_testFormat;
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
};

} // lit
} // polar

#endif
