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
#include <set>
#include <cstring>
#include "Global.h"

namespace polar {
namespace lit {

extern std::list<std::FILE *> g_tempFiles;

void temp_files_clear_handler();
void register_temp_file(std::FILE *file);

std::string center_string(const std::string &text, int width, char fillChar = ' ');
std::string normal_path(const std::string &path);
inline std::string make_word_regex(const std::string &word)
{
   return "\\b"+word+"\\b";
}

inline unsigned detect_cpus()
{
   return std::thread::hardware_concurrency();
}

inline bool mkdir_p(const std::string &path, std::error_code& ec)
{
   return fs::create_directories(path, ec);
}

std::list<std::string> listdir_files(const std::string &dirname,
                                     const std::set<std::string> &suffixes = {""},
                                     const std::set<std::string> &excludeFilenames = {});
std::optional<std::string> which(const fs::path &command, const std::optional<std::string> &paths = std::nullopt) noexcept;
bool check_tools_path(const fs::path &dir, const std::list<std::string> &tools) noexcept;
std::optional<std::string> which_tools(const std::list<std::string> &tools, const std::string &paths) noexcept;
void print_histogram(std::list<std::tuple<std::string, int>> items, const std::string &title = "Items");

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

RunCmdResponse
execute_command(const std::string &command, std::optional<std::string> cwd = std::nullopt,
                std::optional<EnvVarType> env = std::nullopt, std::optional<std::string> input = std::nullopt,
                int timeout = 0);

void use_platform_sdk_on_darwin();
std::optional<std::string> find_platform_sdk_version_on_macos() noexcept;

std::list<std::string> split_string(const std::string &str, char separator, int maxSplit = -1);
void kill_process_and_children(pid_t pid) noexcept;
bool string_startswith(const std::string &str, const std::string &searchStr) noexcept;
bool string_endswith(const std::string &str, const std::string &searchStr) noexcept;

std::string join_string_list(const std::list<std::string> &list, const std::string &glue) noexcept;

template <typename... ArgTypes>
std::string format_string(const std::string &format, ArgTypes&&...args)
{
   char buffer[512];
   int size = std::snprintf(buffer, 512, format.c_str(), std::forward<ArgTypes>(args)...);
   return std::string(buffer, size);
}

void replace_string(const std::string &search, const std::string &replacement,
                    std::string &targetStr);

void ltrim_string(std::string &str);
void rtrim_string(std::string &str);
void trim_string(std::string &str);

bool stdcout_isatty();
void abort_execute_now();
} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_UTILS_H
