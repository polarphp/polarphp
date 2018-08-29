// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#ifndef POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H
#define POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H

#include <string>
#include <optional>
#include "Global.h"

namespace polar {
namespace lit {

inline bool find_executable(const std::string &file) noexcept
{
   return find_executable(fs::path(file));
}

bool find_executable(const fs::path &filepath) noexcept;
std::optional<std::string> look_path(const std::string &file) noexcept;

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_PROCESS_UTILS_H
