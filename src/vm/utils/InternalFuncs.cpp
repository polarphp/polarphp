// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/27.

#include "polarphp/vm/utils/InternalFuncs.h"

namespace polar {
namespace vmapi {
namespace internal {

bool parse_namespaces(const std::string &ns, std::list<std::string> &parts)
{
   std::string::size_type markPos = 0;
   int cycleSepCount = 0;
   std::string::value_type curChar;
   std::string::size_type len = ns.size();
   if (0 == len) {
      return true;
   }
   if (ns[len - 1] == ':') {
      return false;
   }
   std::string::size_type i = 0;
   for (; i < ns.size(); i++) {
      curChar = ns[i];
      if (':' == curChar) {
         if (cycleSepCount == 0 && (i - markPos > 0)) {
            parts.push_back(ns.substr(markPos, i - markPos));
         }
         cycleSepCount++;
      } else {
         if (cycleSepCount == 1 || cycleSepCount > 2) {
            parts.clear();
            return false;
         } else if (cycleSepCount == 2) {
            markPos = i;
            cycleSepCount = 0;
         }
      }
   }
   if (i > markPos) {
      parts.push_back(ns.substr(markPos, i - markPos));
   }
   return true;
}

} // internal
} // vmapi
} // polar
