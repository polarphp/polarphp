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

#ifndef POLAR_DEVLTOOLS_LIT_TEST_H
#define POLAR_DEVLTOOLS_LIT_TEST_H

#include <string>
#include <unordered_map>

namespace polar {
namespace lit {

class ResultCode
{
public:
   ResultCode &getInstance(const std::string &name, bool isFailure)
   {
      if (sm_instances.find(name) != sm_instances.end()) {
         return *sm_instances.at(name);
      }
      sm_instances[name] = new ResultCode(name, isFailure);
      return *sm_instances[name];
   }

   ~ResultCode()
   {
      if (sm_instances.find(m_name) != sm_instances.end()) {
         ResultCode *code = sm_instances.at(m_name);
         delete code;
         sm_instances.erase(m_name);
      }
   }

   operator std::string()
   {
      return std::string("polar::lit::ResultCode "+ m_name + "/" + std::to_string(m_isFailure));
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

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_TEST_H
