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

#include "Discovery.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace polar {
namespace lit {

std::optional<std::string> choose_config_file_from_dir(const std::string &dir,
                                                       const std::list<std::string> &configNames)
{
   for (const std::string &name : configNames) {
      fs::path p(dir);
      p /= name;
      if (fs::exists(p)) {
         return p;
      }
   }
   return std::nullopt;
}

} // lit
} // polar
