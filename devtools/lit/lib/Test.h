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

#ifndef POLAR_DEVLTOOLS_LIT_TEST_H
#define POLAR_DEVLTOOLS_LIT_TEST_H

#include <string>
#include <unordered_map>
#include <any>
#include <list>
#include "nlohmann/json.hpp"
#include "TestingConfig.h"
#include "Utils.h"
#include "LitGlobal.h"
#include "ForwardDefs.h"
#include <iostream>

namespace polar {
namespace lit {

class ResultCode
{
public:
   static ResultCode *getInstance(const std::string &name, bool isFailure)
   {
      if (sm_instances.find(name) != sm_instances.end()) {
         return sm_instances.at(name);
      }
      sm_instances[name] = new ResultCode(name, isFailure);
      return sm_instances[name];
   }

   static void clearupPool()
   {
      for (auto &item : sm_instances) {
         delete item.second;
      }
      sm_instances.clear();
   }

   ~ResultCode()
   {
   }

   bool operator == (const ResultCode &other) const
   {
      return m_name == other.m_name && m_isFailure == other.m_isFailure;
   }

   operator std::string()
   {
      return std::string("polar::lit::ResultCode "+ m_name + "/" + std::to_string(m_isFailure));
   }

   const std::string &getName() const
   {
      return m_name;
   }

   bool isFailure() const
   {
      return m_isFailure;
   }
protected:
   ResultCode(const std::string &name, bool isFailure)
      : m_name(name),
        m_isFailure(isFailure)
   {}
protected:
   std::string m_name;
   bool m_isFailure;
   static std::unordered_map<std::string, ResultCode *> sm_instances;
};

extern const ResultCode *PASS;
extern const ResultCode *FLAKYPASS;
extern const ResultCode *XFAIL;
extern const ResultCode *FAIL;
extern const ResultCode *XPASS;
extern const ResultCode *UNRESOLVED;
extern const ResultCode *UNSUPPORTED;
extern const ResultCode *TIMEOUT;

extern const std::map<std::string, const ResultCode *> sg_resultCodeMap;

inline const ResultCode *get_result_code_by_name(const std::string &name)
{
   return sg_resultCodeMap.at(name);
}

inline void global_resultcode_destroyer()
{
   ResultCode::clearupPool();
}

class MetricValue
{
public:
   enum class ValueType {
      Integer,
      Real,
      Json
   };
public:
   virtual std::string format() = 0;
   virtual ValueType getValueType() const = 0;
   ~MetricValue()
   {}
};

class IntMetricValue : public MetricValue
{
public:
   IntMetricValue(int value)
      : m_value(value)
   {}
   std::string format()
   {
      return std::to_string(m_value);
   }

   int toData()
   {
      return m_value;
   }

   ValueType getValueType() const
   {
      return ValueType::Integer;
   }
protected:
   int m_value;
};

class RealMetricValue : public MetricValue
{
public:
   RealMetricValue(double value)
      : m_value(value)
   {
   }

   std::string format()
   {
      char buffer[32];
      std::sprintf(buffer, "%.4f", m_value);
      return buffer;
   }

   double toData()
   {
      return m_value;
   }

   ValueType getValueType() const
   {
      return ValueType::Real;
   }
protected:
   double m_value;
};

class JSONMetricValue : public MetricValue
{
public:
   template <typename T>
   JSONMetricValue(const T &value)
      : m_value(value)
   {}
   std::string format()
   {
      return m_value.dump(2);
   }

   nlohmann::json toData()
   {
      return m_value;
   }

   ValueType getValueType() const
   {
      return ValueType::Json;
   }
protected:
   nlohmann::json m_value;
};

template <typename T, typename std::enable_if<std::is_integral<T>::value>::type>
inline IntMetricValue to_meteric_value(T value)
{
   return IntMetricValue(value);
}

template <typename T, typename std::enable_if<std::is_floating_point<T>::value>::type>
inline RealMetricValue to_meteric_value(T value)
{
   return RealMetricValue(value);
}

template <typename T, typename std::enable_if<!std::is_integral<T>::value && !std::is_floating_point<T>::value>::type>
inline JSONMetricValue to_meteric_value(T value)
{
   return JSONMetricValue(value);
}

class Result
{
public:
   Result(const ResultCode *code = nullptr, std::string output = "", std::optional<int> elapsed = std::nullopt);
   Result &addMetric(const std::string &name, MetricValuePointer value);
   Result &addMicroResult(const std::string &name, ResultPointer microResult);
   ~Result();
   const ResultCode *getCode() const;
   Result &setCode(const ResultCode *code);
   const std::string &getOutput() const;
   Result &setOutput(const std::string &output);
   const std::optional<size_t> &getElapsed() const;
   Result &setElapsed(size_t elapsed);
   const std::unordered_map<std::string, MetricValuePointer> &getMetrics() const;
   std::unordered_map<std::string, ResultPointer> &getMicroResults();
protected:
   const ResultCode *m_code;
   std::string m_output;
   std::optional<size_t> m_elapsed;
   std::unordered_map<std::string, MetricValuePointer> m_metrics;
   std::unordered_map<std::string, ResultPointer> m_microResults;
};

class TestSuite
{
public:
   TestSuite();
   TestSuite(const std::string &name, const std::string &sourceRoot,
             const std::string &execRoot, TestingConfigPointer config);
   const std::string &getName();
   int getId() const;
   std::string getSourcePath(const std::list<std::string> &components = {}) const;
   std::string getExecPath(const std::list<std::string> &components = {}) const;
   TestingConfigPointer getConfig() const;
protected:
   std::string m_name;
   int m_id;
   std::string m_sourceRoot;
   std::string m_execRoot;
   TestingConfigPointer m_config;
   static int sm_gid;
};

class Test
{
public:
   Test(TestSuitePointer testSuite, const std::list<std::string> &pathInSuite,
        TestingConfigPointer config, const std::optional<std::string> &filePath = std::nullopt);
   void setResult(ResultPointer result);
   ResultPointer getResult() const;
   std::string getFullName() const;
   std::string getFilePath();
   std::string getSourcePath();
   TestingConfigPointer getConfig();
   std::vector<std::string> &getXFails();
   std::vector<std::string> &getRequires();
   const std::string &getSelfSourcePath() const;
   std::string getExecPath();
   TestSuitePointer getTestSuite() const;
   Test &setSelfSourcePath(const std::string &sourcePath);
   bool isExpectedToFail();
   bool isWithinFeatureLimits();
   std::list<std::string> getMissingRequiredFeaturesFromList(const std::vector<std::string> &features);
   std::list<std::string> getMissingRequiredFeatures();
   std::vector<std::string> getUnSupportedFeatures();
   std::vector<std::string> &getRawUnSupported();
   bool isEarlyTest() const;
   void writeJUnitXML(std::string &xmlStr);
protected:
   TestSuitePointer m_suite;
   std::list<std::string> m_pathInSuite;
   TestingConfigPointer m_config;
   std::optional<std::string> m_filePath;
   std::vector<std::string> m_xfails;
   std::vector<std::string> m_requires;
   std::vector<std::string> m_unsupported;
   std::shared_ptr<Result> m_result;
   std::string m_selfSourcePath;
};

} // lit
} // polar

POLAR_MAKE_HASHABLE(polar::lit::ResultCode, std::hash<std::string>{}(t.getName()),
                    std::hash<bool>{}(t.isFailure()))

#endif // POLAR_DEVLTOOLS_LIT_TEST_H
