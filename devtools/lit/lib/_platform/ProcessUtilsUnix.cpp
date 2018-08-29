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

#include "../ProcessUtils.h"
#include "../Utils.h"
#include <list>
#include <cstdlib>

namespace polar {
namespace lit {

bool find_executable(const fs::path &filepath) noexcept
{
   std::error_code errCode;
   if (fs::exists(filepath, errCode)) {
      return true;
   }
   return false;
}

std::optional<std::string> look_path(const std::string &file) noexcept
{
   if (std::string::npos != file.find_first_of('/')) {
      if (find_executable(file)) {
         return file;
      }
      return std::nullopt;
   }
   std::string path(std::getenv("PATH"));
   std::list<std::string> dirs(split_string(path, '/'));
   for (std::string dir : dirs) {
      if (dir == "") {
         dir = ".";
      }
      fs::path path(dir);
      path /= file;
      if (find_executable(path)) {
         return path.string();
      }
   }
   return std::nullopt;
}

} // lit
} // polar
