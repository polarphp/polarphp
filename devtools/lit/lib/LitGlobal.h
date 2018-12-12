// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/29.

#ifndef POLAR_DEVLTOOLS_LIT_GLOBAL_H
#define POLAR_DEVLTOOLS_LIT_GLOBAL_H

#include "LitConfigDef.h"
#include <filesystem>

namespace polar {
namespace lit {

#define SUBPROCESS_FD_PIPE "______littest_subprocess_fd_pipe_filemark______"
#define SUBPROCESS_FD_STDOUT "______littest_subprocess_fd_stdout_filemark______"
#define TESTRUNNER_TEMP_PREFIX "polarphp-lit-"
/// we describe all toke by ShellTokenType, and we need to distinguish
/// normal token and redirects token, so we define the token type code
#define SHELL_CMD_NORMAL_TOKEN -1
#define SHELL_CMD_REDIRECT_TOKEN -2
#define CFG_SETTER_KEY "CfgSetterPlugin"

namespace stdfs = std::filesystem;

class ValueError : public std::runtime_error
{
   using std::runtime_error::runtime_error;
};

class NotImplementedError : public std::runtime_error
{
   using std::runtime_error::runtime_error;
};

class TestingConfig;
class LitConfig;

using ShellTokenType = std::tuple<std::string, int>;
using RunCmdResponse = std::tuple<int, std::string, std::string>;// exitCode, stdout, stderr
using CfgSetterType = void (*)(TestingConfig *config, LitConfig *litConfig);

// https://stackoverflow.com/questions/17016175/c-unordered-map-using-a-custom-class-type-as-the-key
#define POLAR_MAKE_HASHABLE(type, ...) \
    namespace std {\
        template<> struct hash<type> {\
            std::size_t operator()(const type &t) const {\
                std::size_t ret = 0;\
                ::polar::lit::hash_combine(ret, __VA_ARGS__);\
                return ret;\
            }\
        };\
    }

#define POLAR_ATTR_UNUSED [[maybe_unused]]
class LitConfig;
extern const char *sg_emptyStr;
extern LitConfig *sg_litCfg;

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_GLOBAL_H
