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

#define SUBPROCESS_FD_PIPE -9
/// we describe all toke by ShellTokenType, and we need to distinguish
/// normal token and redirects token, so we define the token type code
#define SHELL_CMD_NORMAL_TOKEN -1
#define SHELL_CMD_REDIRECT_TOKEN -2

namespace fs = std::filesystem;

class ValueError : public std::runtime_error
{
   using std::runtime_error::runtime_error;
};

class NotImplementedError : public std::runtime_error
{
   using std::runtime_error::runtime_error;
};

using ShellTokenType = std::tuple<std::string, int>;
using RunCmdResponse = std::tuple<int, std::string, std::string>;

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_GLOBAL_H
