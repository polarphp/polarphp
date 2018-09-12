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

#ifndef POLAR_DEVLTOOLS_LIT_GLOBAL_H
#define POLAR_DEVLTOOLS_LIT_GLOBAL_H

#include "Config.h"
#include <filesystem>

namespace polar {
namespace lit {

namespace fs = std::filesystem;

class ValueError : public std::runtime_error
{
   using std::runtime_error::runtime_error;
};

using ShellTokenType = std::tuple<std::string, int>;

#define SUBPROCESS_FD_PIPE -9

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_GLOBAL_H
