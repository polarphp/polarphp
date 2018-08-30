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

#include "Utils.h"
#include "ProcessUtils.h"
#include "Config.h"

namespace polar {
namespace lit {

std::list<std::string> split_string(const std::string &str, char separator)
{
   std::string buff;
   std::list<std::string> parts;
   for(auto n : str) {
      if(n != separator) {
         buff+=n;
      } else if(n == separator && buff != "") {
         parts.push_back(buff);
         buff = "";
      }
   }
   if(buff != "") {
      parts.push_back(buff);
   }
   return parts;
}

std::optional<std::string> find_platform_sdk_version_on_macos() noexcept
{
   if (std::strcmp(POLAR_OS, "Darwin") != 0) {
      return std::nullopt;
   }
   RunCmdResponse result = run_program("xcrun", "--show-sdk-version", "--sdk", "macosx");
   if (std::get<0>(result)) {
      return std::get<1>(result);
   }
   return std::nullopt;
}

} // lit
} // polar
