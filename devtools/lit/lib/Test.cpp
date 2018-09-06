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

namespace polar {
namespace lit {

std::unordered_map<std::string, ResultCode *> ResultCode::sm_instances{};

Result::Result(int code, std::string output, std::optional<int> elapsed)
   : m_code(code),
     m_output(output),
     m_elapsed(elapsed)
{
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

Test::Test(const TestSuite &suit, const std::string &pathInSuite,
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

      }
   } catch (ValueError &e) {

   }
}

bool Test::isExpectedToFail()
{

}

} // lit
} // polar
