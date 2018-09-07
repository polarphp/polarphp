// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#include "Test.h"
#include "Global.h"
#include "BooleanExpression.h"
#include <set>

namespace polar {
namespace lit {

std::unordered_map<std::string, ResultCode *> ResultCode::sm_instances{};

const ResultCode &PASS = ResultCode::getInstance("PASS", false);
const ResultCode &FLAKYPASS = ResultCode::getInstance("FLAKYPASS", false);
const ResultCode &XFAIL = ResultCode::getInstance("XFAIL", false);
const ResultCode &FAIL = ResultCode::getInstance("FAIL", true);
const ResultCode &XPASS = ResultCode::getInstance("XPASS", true);
const ResultCode &UNRESOLVED = ResultCode::getInstance("UNRESOLVED", true);
const ResultCode &UNSUPPORTED = ResultCode::getInstance("UNSUPPORTED", false);
const ResultCode &TIMEOUT = ResultCode::getInstance("TIMEOUT", true);

Result::Result(const ResultCode &code, std::string output, std::optional<int> elapsed)
   : m_code(code),
     m_output(output),
     m_elapsed(elapsed)
{
}

const ResultCode &Result::getCode()
{
   return m_code;
}

Result &Result::setCode(const ResultCode &code)
{
   m_code = code;
   return *this;
}

const std::string &Result::getOutput()
{
   return m_output;
}

Result &Result::setOutput(const std::string &output)
{
   m_output = output;
   return *this;
}

const std::optional<int> &Result::getElapsed()
{
   return m_elapsed;
}

const std::unordered_map<std::string, MetricValue *> &Result::getMetrics()
{
   return m_metrics;
}

std::unordered_map<std::string, std::shared_ptr<Result>> &Result::getMicroResults()
{
   return m_microResults;
}

Result::~Result()
{
   auto iter = m_metrics.begin();
   while (iter != m_metrics.end()) {
      delete iter->second;
   }
}

Result &Result::addMetric(const std::string &name, MetricValue *value)
{
   if (m_metrics.find(name) != m_metrics.end()) {
      throw ValueError(format_string("result already includes metrics for %s", name.c_str()));
   }
   m_metrics[name] = value;
   return *this;
}

Result &Result::addMicroResult(const std::string &name, std::shared_ptr<Result> microResult)
{
   if (m_metrics.find(name) != m_metrics.end()) {
      throw ValueError(format_string("esult already includes microResult for %s", name.c_str()));
   }
   m_microResults[name] = microResult;
   return *this;
}

TestSuite::TestSuite(const std::string &name, const std::string &sourceRoot,
                     const std::string &execRoot, const TestingConfig &config)
   : m_name(name),
     m_sourceRoot(sourceRoot),
     m_execRoot(execRoot),
     m_config(config)
{
   // m_config is the test suite configuration.
}

const std::string &TestSuite::getName()
{
   return m_name;
}

std::string TestSuite::getSourcePath(const std::list<std::string> &components)
{
   fs::path base(m_sourceRoot);
   for (const std::string &item : components) {
      base /= item;
   }
   return base.string();
}

std::string TestSuite::getExecPath(const std::list<std::string> &components)
{
   fs::path base(m_execRoot);
   for (const std::string &item : components) {
      base /= item;
   }
   return base.string();
}

TestingConfig &TestSuite::getConfig()
{
   return m_config;
}

Test::Test(const TestSuite &suit, const std::list<std::string> &pathInSuite,
           const TestingConfig &config, std::optional<std::string> &filePath)
   : m_suite(suit),
     m_pathInSuite(pathInSuite),
     m_config(config),
     m_filePath(filePath),
     m_result(std::nullopt)
{
}

void Test::setResult(const Result &result)
{
   if (m_result.has_value()) {
      throw ValueError("test result already set");
   }
   m_result = result;
   // Apply the XFAIL handling to resolve the result exit code.
   try {
      if (isExpectedToFail()) {
         if (m_result.has_value()) {
            Result &selfResult = m_result.value();
            const ResultCode &code = selfResult.getCode();
            if (code == PASS) {
               selfResult.setCode(XPASS);
            } else if (code == FAIL) {
               selfResult.setCode(XFAIL);
            }
         }
      }
   } catch (ValueError &e) {
      // Syntax error in an XFAIL line.
      if (m_result.has_value()) {
         Result &selfResult = m_result.value();
         selfResult.setCode(UNRESOLVED);
         selfResult.setOutput(e.what());
      }
   }
}

std::string Test::getFullName()
{
   return m_config.getName() + " :: " + join_string_list(m_pathInSuite, "/");
}

std::string Test::getFilePath()
{
   if (m_filePath.has_value()) {
      return m_filePath.value();
   }
   return getSourcePath();
}

std::string Test::getSourcePath()
{
   return m_suite.getSourcePath(m_pathInSuite);
}

std::string Test::getExecPath()
{
   return m_suite.getExecPath(m_pathInSuite);
}

bool Test::isExpectedToFail()
{
   const std::set<std::string> &features = m_config.getAvailableFeatures();
   const std::string &triple = m_config.getExtraConfig<std::string>("target_triple", std::string(""));
   // Check if any of the xfails match an available feature or the target.
   for (const std::string &item : m_xfails) {
      // If this is the wildcard, it always fails.
      if (item == "*") {
         return true;
      }
      // If this is a True expression of features and target triple parts,
      // it fails.
      try {
         std::optional<bool> evalResult = BooleanExpression::evaluate(item, features, triple);
         if (evalResult.has_value() && evalResult.value()) {
            return true;
         }
      } catch (ValueError &error) {
         throw ValueError(format_string("Error in XFAIL list:\n%s", error.what()));
      }
   }
   return false;
}

bool Test::isWithinFeatureLimits()
{
   if (m_config.getLimitToFeatures().empty()) {
      return true; // No limits. Run it.
   }
   // Check the requirements as-is (#1)
   if (!getMissingRequiredFeatures().empty()) {
      return false;
   }
   // Check the requirements after removing the limiting features (#2)
   std::set<std::string> featuresMinusLimits;
   const std::set<std::string> &availableFeatures = m_config.getAvailableFeatures();
   const std::set<std::string> &limitToFeatures = m_config.getLimitToFeatures();
   auto iter = availableFeatures.begin();
   auto endMark = availableFeatures.end();
   while (iter != endMark) {
      if (limitToFeatures.find(*iter) == limitToFeatures.end()) {
         featuresMinusLimits.insert(*iter);
      }
      ++iter;
   }
   if (getMissingRequiredFeaturesFromList(featuresMinusLimits).empty()) {
      return false;
   }
   return true;
}

std::list<std::string> Test::getMissingRequiredFeatures()
{
   const std::set<std::string> &features = m_config.getAvailableFeatures();
   return getMissingRequiredFeaturesFromList(features);
}

std::list<std::string> Test::getMissingRequiredFeaturesFromList(const std::set<std::string> &features)
{
   std::list<std::string> ret;
   try {
      for (const std::string &item : m_requires) {
         if (!BooleanExpression::evaluate(item, m_requires)) {
            ret.push_back(item);
         }
      }
      return ret;
   } catch (ValueError &error) {
      throw ValueError(format_string("Error in REQUIRES list:\n%s", error.what()));
   }
}

std::list<std::string> Test::getUnsupportedFeatures()
{
   std::list<std::string> ret;
   const std::set<std::string> &features = m_config.getAvailableFeatures();
   const std::string &triple = m_config.getExtraConfig<std::string>("target_triple", std::string(""));
   try {
      for (const std::string &item : m_unsupported) {
         if (BooleanExpression::evaluate(item, features, triple)) {
            ret.push_back(item);
         }
      }
      return ret;
   } catch (ValueError &error) {
      throw ValueError(format_string("Error in UNSUPPORTED list:\n%s", error.what()));
   }
}

bool Test::isEarlyTest()
{
   return m_suite.getConfig().isEarly();
}

void Test::writeJUnitXML(std::string &xmlStr)
{
   std::string &testName = *m_pathInSuite.rbegin();
   std::list<std::string> safeTestPath;
   auto piter = m_pathInSuite.begin();
   for (int i = 0; i < m_pathInSuite.size() - 1; ++i) {

   }
}

} // lit
} // polar
