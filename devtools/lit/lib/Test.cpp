// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/05.

#include "Test.h"
#include "LitGlobal.h"
#include "BooleanExpression.h"
#include "polarphp/basic/adt/StringRef.h"
#include <map>
#include <iostream>

namespace polar {
namespace lit {

std::unordered_map<std::string, ResultCode *> ResultCode::sm_instances{};

const ResultCode *PASS = ResultCode::getInstance("PASS", false);
const ResultCode *FLAKYPASS = ResultCode::getInstance("FLAKYPASS", false);
const ResultCode *XFAIL = ResultCode::getInstance("XFAIL", false);
const ResultCode *FAIL = ResultCode::getInstance("FAIL", true);
const ResultCode *XPASS = ResultCode::getInstance("XPASS", true);
const ResultCode *UNRESOLVED = ResultCode::getInstance("UNRESOLVED", true);
const ResultCode *UNSUPPORTED = ResultCode::getInstance("UNSUPPORTED", false);
const ResultCode *TIMEOUT = ResultCode::getInstance("TIMEOUT", true);

const std::map<std::string, const ResultCode *> sg_resultCodeMap{
   {"PASS", PASS},
   {"FLAKYPASS", FLAKYPASS},
   {"XFAIL", XFAIL},
   {"FAIL", FAIL},
   {"XPASS", XPASS},
   {"UNRESOLVED", UNRESOLVED},
   {"UNSUPPORTED", UNSUPPORTED},
   {"TIMEOUT", TIMEOUT}
};

Result::Result(const ResultCode *code, std::string output, std::optional<int> elapsed)
   : m_code(code),
     m_output(output),
     m_elapsed(elapsed)
{
}

const ResultCode *Result::getCode() const
{
   return m_code;
}

Result &Result::setCode(const ResultCode *code)
{
   m_code = code;
   return *this;
}

const std::string &Result::getOutput() const
{
   return m_output;
}

Result &Result::setOutput(const std::string &output)
{
   m_output = output;
   return *this;
}

const std::optional<size_t> &Result::getElapsed() const
{
   return m_elapsed;
}

Result &Result::setElapsed(size_t elapsed)
{
   m_elapsed = elapsed;
   return *this;
}

const std::unordered_map<std::string, MetricValuePointer> &Result::getMetrics() const
{
   return m_metrics;
}

std::unordered_map<std::string, ResultPointer> &Result::getMicroResults()
{
   return m_microResults;
}

Result::~Result()
{
}

Result &Result::addMetric(const std::string &name, MetricValuePointer value)
{
   if (m_metrics.find(name) != m_metrics.end()) {
      throw ValueError(format_string("result already includes metrics for %s", name.c_str()));
   }
   m_metrics[name] = value;
   return *this;
}

Result &Result::addMicroResult(const std::string &name, ResultPointer microResult)
{
   if (m_metrics.find(name) != m_metrics.end()) {
      throw ValueError(format_string("esult already includes microResult for %s", name.c_str()));
   }
   m_microResults[name] = microResult;
   return *this;
}

int TestSuite::sm_gid = 0;

TestSuite::TestSuite()
{}

TestSuite::TestSuite(const std::string &name, const std::string &sourceRoot,
                     const std::string &execRoot, TestingConfigPointer config)
   : m_name(name),
     m_sourceRoot(sourceRoot),
     m_execRoot(execRoot),
     m_config(config)
{
   // only one thread running when collect tests
   m_id = ++sm_gid;
}

const std::string &TestSuite::getName()
{
   return m_name;
}

int TestSuite::getId() const
{
   return m_id;
}

std::string TestSuite::getSourcePath(const std::list<std::string> &components) const
{
   stdfs::path base(m_sourceRoot);
   for (const std::string &item : components) {
      base /= item;
   }
   return base.string();
}

std::string TestSuite::getExecPath(const std::list<std::string> &components) const
{
   stdfs::path base(m_execRoot);
   for (const std::string &item : components) {
      base /= item;
   }
   return base.string();
}

TestingConfigPointer TestSuite::getConfig() const
{
   return m_config;
}

Test::Test(TestSuitePointer suite, const std::list<std::string> &pathInSuite,
           TestingConfigPointer config, const std::optional<std::string> &filePath)
   : m_suite(suite),
     m_pathInSuite(pathInSuite),
     m_config(config),
     m_filePath(filePath),
     m_result(nullptr)
{
}

void Test::setResult(ResultPointer result)
{
   if (m_result) {
      throw ValueError("test result already set");
   }
   m_result = result;
   // Apply the XFAIL handling to resolve the result exit code.
   try {
      if (isExpectedToFail()) {
         if (m_result) {
            const ResultCode *code = m_result->getCode();
            if (code == PASS) {
               m_result->setCode(XPASS);
            } else if (code == FAIL) {
               m_result->setCode(XFAIL);
            }
         }
      }
   } catch (ValueError &e) {
      // Syntax error in an XFAIL line.
      if (m_result) {
         m_result->setCode(UNRESOLVED);
         m_result->setOutput(e.what());
      }
   }
}

ResultPointer Test::getResult() const
{
   return m_result;
}

std::string Test::getFullName() const
{
   return m_config->getName() + " :: " + join_string_list(m_pathInSuite, "/");
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
   return m_suite->getSourcePath(m_pathInSuite);
}

TestingConfigPointer Test::getConfig()
{
   return m_config;
}

std::vector<std::string> &Test::getXFails()
{
   return m_xfails;
}

std::vector<std::string> &Test::getRequires()
{
   return m_requires;
}

const std::string &Test::getSelfSourcePath() const
{
   return m_selfSourcePath;
}

Test &Test::setSelfSourcePath(const std::string &sourcePath)
{
   m_selfSourcePath = sourcePath;
   return *this;
}

TestSuitePointer Test::getTestSuite() const
{
   return m_suite;
}

std::string Test::getExecPath()
{
   return m_suite->getExecPath(m_pathInSuite);
}

bool Test::isExpectedToFail()
{
   const std::vector<std::string> &features = m_config->getAvailableFeatures();
   const std::string &triple = m_config->getExtraConfig<std::string>("target_triple", "");
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
   if (m_config->getLimitToFeatures().empty()) {
      return true; // No limits. Run it.
   }
   // Check the requirements as-is (#1)
   if (!getMissingRequiredFeatures().empty()) {
      return false;
   }
   // Check the requirements after removing the limiting features (#2)
   std::vector<std::string> featuresMinusLimits;
   const std::vector<std::string> &availableFeatures = m_config->getAvailableFeatures();
   const std::vector<std::string> &limitToFeatures = m_config->getLimitToFeatures();
   auto iter = availableFeatures.begin();
   auto endMark = availableFeatures.end();
   while (iter != endMark) {
      if (std::find(limitToFeatures.begin(), limitToFeatures.end(), *iter) == limitToFeatures.end()) {
         featuresMinusLimits.push_back(*iter);
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
   const std::vector<std::string> &features = m_config->getAvailableFeatures();
   return getMissingRequiredFeaturesFromList(features);
}

std::list<std::string> Test::getMissingRequiredFeaturesFromList(const std::vector<std::string> &features)
{
   std::list<std::string> ret;
   try {
      for (const std::string &item : m_requires) {
         std::optional<bool> evalResult = BooleanExpression::evaluate(item, features);
         if (evalResult.has_value() && !evalResult.value()) {
            ret.push_back(item);
         }
      }
      return ret;
   } catch (ValueError &error) {
      throw ValueError(format_string("Error in REQUIRES list:\n%s", error.what()));
   }
}

std::vector<std::string> Test::getUnSupportedFeatures()
{
   std::vector<std::string> ret;
   const std::vector<std::string> &features = m_config->getAvailableFeatures();
   const std::string &triple = m_config->getExtraConfig<std::string>("target_triple", "");
   try {
      for (const std::string &item : m_unsupported) {
         std::optional<bool> evalResult = BooleanExpression::evaluate(item, features, triple);
         if (evalResult.has_value() && evalResult.value()) {
            ret.push_back(item);
         }
      }
      return ret;
   } catch (ValueError &error) {
      throw ValueError(format_string("Error in UNSUPPORTED list:\n%s", error.what()));
   }
}

std::vector<std::string> &Test::getRawUnSupported()
{
   return m_unsupported;
}

bool Test::isEarlyTest() const
{
   return m_suite->getConfig()->isEarly();
}

void Test::writeJUnitXML(std::string &xmlStr)
{
   std::string &testName = *m_pathInSuite.rbegin();
   std::list<std::string> safeTestPath;
   auto piter = m_pathInSuite.begin();
   for (size_t i = 0; i < m_pathInSuite.size() - 1; ++i) {
      std::string pathItem = *piter;
      replace_string(".", "_", pathItem);
      safeTestPath.push_back(pathItem);
      ++piter;
   }
   std::string safeName = m_suite->getName();
   std::string className;
   replace_string(".", "_", safeName);
   if (!safeTestPath.empty()) {
      className = safeName + "." + join_string_list(safeTestPath, "/");
   } else {
      className = safeName;
   }

   std::string testcaseTemplate = "<testcase classname=\"%s\" name=\"%s\" time=\"%.2f\"";
   float elapsedTime = 0.0;
   if (m_result && m_result->getElapsed().has_value()){
      elapsedTime = m_result->getElapsed().value();
   }
   std::string testcaseXml = format_string(testcaseTemplate, className.c_str(), testName.c_str(), elapsedTime);
   xmlStr = testcaseXml;
   if (m_result && m_result->getCode()->isFailure()) {
      xmlStr += ">\n\t<failure ><![CDATA[";
      std::string output = m_result->getOutput();
      replace_string("]]>", "]]]]><![CDATA[>", output);
      xmlStr += output;
      xmlStr += "]]></failure>\n</testcase>";
   } else if (m_result && m_result->getCode() == UNSUPPORTED) {
      std::list<std::string> unsupportedFeatures = getMissingRequiredFeatures();
      std::string skipMessage;
      if (!unsupportedFeatures.empty()) {
         skipMessage = "Skipping because of: " + join_string_list(unsupportedFeatures, ", ");
      } else {
         skipMessage = "Skipping because of configuration.";
      }
      xmlStr += format_string(">\n\t<skipped message=\"%s\" />\n</testcase>\n", skipMessage.c_str());
   } else {
      xmlStr += "/>";
   }
}

} // lit
} // polar
