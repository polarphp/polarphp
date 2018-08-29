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

#ifndef POLAR_DEVLTOOLS_LIT_UTILS_H
#define POLAR_DEVLTOOLS_LIT_UTILS_H

#include <string>
#include <thread>
#include <optional>
#include <list>
#include <map>
#include <cstring>
#include "Config.h"

namespace polar {
namespace lit {

std::string normal_path(const std::string &path);
inline std::string make_word_regex(const std::string &word)
{
   return "\\b"+word+"\\b";
}

inline unsigned detect_cpus()
{
   return std::thread::hardware_concurrency();
}

inline void mkdir_p()
{

}

void show_version();

void listdir_files();
void which(const std::string &command, std::optional<std::string> paths = std::nullopt);
bool check_tools_path(const std::string &dir, const std::list<std::string> &paths);
std::optional<std::string> which_tools(const std::list<std::string> &list, const std::string &paths);
void print_histogram(const std::list<std::string> &items, const std::string &title = "Items");

class ExecuteCommandTimeoutException : public std::runtime_error
{
public:
   ExecuteCommandTimeoutException(const std::string &msg, const std::string &out,
                                  const std::string &error, int code)
      : std::runtime_error(msg), m_out(out),
        m_error(error), m_code(code)
   {}
protected:
   std::string m_out;
   std::string m_error;
   int m_code;
};

inline bool kuse_close_fds()
{
   std::strcmp(POLAR_OS, "Window") != 0;
}

using EnvVarType = std::map<std::string, std::string>;

std::tuple<std::string, std::string, int>
execute_command(const std::string &command, std::optional<std::string> cwd = std::nullopt,
                std::optional<EnvVarType> env = std::nullopt, std::optional<std::string> input = std::nullopt,
                int timeout = 0);

void use_platform_sdk_on_darwin();
std::optional<std::string> find_platform_sdk_version_on_macos();

std::list<std::string> split_string(const std::string &str, char separator);

} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_UTILS_H
