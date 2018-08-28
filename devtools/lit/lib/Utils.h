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

namespace polar {
namespace lit {

std::string normal_path(const std::string &path);
std::string make_word_regex(const std::string &word)
{
   return "\\b"+word+"\\b";
}

unsigned detect_cpus()
{
   return std::thread::hardware_concurrency();
}

void mkdir_p()
{

}

void listdir_files();
void which(const std::string &command, std::optional<std::string> paths = std::nullopt);
bool check_tools_path(const std::string &dir, const std::list<std::string> &paths);
std::optional<std::string> which_tools(const std::list<std::string> &list, const std::string &paths);
void print_histogram(const std::list<std::string> &items, const std::string &title = "Items");


} // lit
} // polar

#endif // POLAR_DEVLTOOLS_LIT_UTILS_H
