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
   std::string cfgFilepath = (StringRef(LIT_SOURCE_DIR"/metrics.tjson")).getStr();
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
      json value = iter.value();
      if (value.is_number_integer()) {
         result->addMetric(iter.key(), std::make_shared<IntMetricValue>(value.get<int>()));
      } else if (value.is_number_float()) {
         result->addMetric(iter.key(), std::make_shared<RealMetricValue>(value.get<double>()));
      } else {
         throw std::runtime_error("unsupported result type");
      }
   }
   return result;
}

} // lit
} // polar
