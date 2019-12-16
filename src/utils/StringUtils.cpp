// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/25.

#include "polarphp/utils/StringUtils.h"
#include "llvm/ADT/StringRef.h"
#include <cstring>

namespace polar::utils {

static const char sg_regexMetachars[] = "()^$|*+?.:[]\\{}";

std::string regex_escape(StringRef str) {
   std::string regexStr;
   for (unsigned i = 0, e = str.size(); i != e; ++i) {
      if (strchr(sg_regexMetachars, str[i])) {
         regexStr += '\\';
      }
      regexStr += str[i];
   }
   return regexStr;
}

} // polar::utils
