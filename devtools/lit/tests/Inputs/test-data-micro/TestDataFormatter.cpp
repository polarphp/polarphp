// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/18.

#include "TestDataFormatter.h"
#include "nlohmann/json.hpp"
#include "fstream"
#include "Test.h"
#include "polarphp/basic/adt/StringRef.h"
#include <iostream>

using nlohmann::json;

namespace polar {
namespace lit {

ResultPointer TestDataFormatter::execute(TestPointer test, LitConfigPointer litConfig)
{
   std::string cfgFilepath = (StringRef(LIT_SOURCE_DIR"/micro-metrics.tjson")).getStr();
   std::ifstream dataStream(cfgFilepath);
   if (!dataStream.is_open()) {
      return std::make_shared<Result>(UNRESOLVED, cfgFilepath + " open failed");
   }
   json jsonDoc;
   dataStream >> jsonDoc;
   assert(jsonDoc.is_object());
   json global = jsonDoc["global"];
   std::string resultCode = global["result_code"].get<std::string>();
   std::string resultOutput = global["result_output"].get<std::string>();
   ResultPointer result = std::make_shared<Result>(get_result_code_by_name(resultCode), resultOutput);
   json results = jsonDoc["results"];
   for (json::iterator iter = results.begin(); iter != results.end(); ++iter) {
      MetricValuePointer metric;
      json value = iter.value();
      if (value.is_number_integer()) {
         metric.reset(new IntMetricValue(value.get<int>()));
      } else if (value.is_number_float()) {
         metric.reset(new RealMetricValue(value.get<double>()));
      } else {
         throw std::runtime_error("unsupported result type");
      }
      result->addMetric(iter.key(), metric);
   }
   // Create micro test results
   json microTests = jsonDoc["micro-tests"];
   for (json::iterator iter = microTests.begin(); iter != microTests.end(); ++iter) {
      ResultPointer microResult = std::make_shared<Result>(get_result_code_by_name(resultCode), resultOutput);
      // Load micro test additional metrics
      json microResults = jsonDoc["micro-results"];
      for (json::iterator microResultIter = microResults.begin(); microResultIter != microResults.end(); ++microResultIter) {
         MetricValuePointer metric;
         json value = microResultIter.value();
         if (value.is_number_integer()) {
            metric.reset(new IntMetricValue(value.get<int>()));
         } else if (value.is_number_float()) {
            metric.reset(new RealMetricValue(value.get<double>()));
         } else {
            throw std::runtime_error("unsupported result type");
         }
         microResult->addMetric(microResultIter.key(), metric);
      }
      std::string microName = iter.value().get<std::string>();
      result->addMicroResult(microName, microResult);
   }
   return result;
}

} // lit
} // polar
