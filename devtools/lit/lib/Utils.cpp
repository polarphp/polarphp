// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/08/28.

#include "Utils.h"
#include "ProcessUtils.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/OptionalError.h"
#include "LitConfig.h"
#include <cmath>
#include <iostream>
#include <thread>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <locale>

namespace polar {
namespace lit {

using polar::basic::StringRef;
using polar::basic::ArrayRef;
using polar::utils::OptionalError;

const char *sg_emptyStr = "";

std::list<std::FILE *> g_tempFiles {};

void temp_files_clear_handler()
{
   for (std::FILE *file : g_tempFiles) {
      std::fclose(file);
   }
   g_tempFiles.clear();
}

void register_temp_file(std::FILE *file)
{
   g_tempFiles.push_back(file);
}

std::list<std::string> split_string(const std::string &str, char separator, int maxSplit)
{
   std::string buff;
   std::list<std::string> parts;
   int currentSplitCycle = 0;
   size_t currentPos = 0;
   for(auto n : str) {
      ++currentPos;
      if(n != separator) {
         buff+=n;
      } else if(n == separator && buff != "") {
         parts.push_back(buff);
         ++currentSplitCycle;
         buff = "";
         if (maxSplit != -1 && currentSplitCycle >= maxSplit) {
            parts.push_back(str.substr(currentPos));
            return parts;
         }
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

   ArrayRef<StringRef> args{
      "--show-sdk-version",
      "--sdk",
      "macosx"
   };
   OptionalError xcrun = polar::sys::find_program_by_name("xcrun");
   assert(xcrun && "xcrun command not found");
   RunCmdResponse result = execute_and_wait(xcrun.get(), args);
   if (std::get<0>(result)) {
      return std::get<1>(result);
   }
   return std::nullopt;
}

namespace {

bool check_file_have_ext(const std::string &filename, const std::set<std::string> &suffixes)
{
   for (const std::string &suffix : suffixes) {
      if (string_endswith(filename, suffix)) {
         return true;
      }
   }
   return false;
}

} // anonymous namespace

std::list<std::string> listdir_files(const std::string &dirname,
                                     const std::set<std::string> &suffixes,
                                     const std::set<std::string> &excludeFilenames)
{
   stdfs::path dir(dirname);
   if (!stdfs::exists(dir)) {
      return {};
   }
   std::list<std::string> files;
   for (const stdfs::directory_entry &entry: stdfs::recursive_directory_iterator(dir)) {
      std::string filename = entry.path().string();
      if (entry.is_directory() || filename[0] == '.' ||
          excludeFilenames.find(filename) != excludeFilenames.end() ||
          !check_file_have_ext(filename, suffixes)) {
         continue;
      }
      files.push_back(filename);
   }
   return files;
}

bool check_tools_path(const stdfs::path &dir, const std::list<std::string> &tools) noexcept
{
   for (const std::string &tool : tools) {
      stdfs::path toolPath = dir / tool;
      if (!stdfs::exists(toolPath)) {
         return false;
      }
   }
   return true;
}

std::optional<std::string> which_tools(const std::list<std::string> &tools, const std::string &paths) noexcept
{
   std::list<std::string> pathList = split_string(paths, ':');
   for (const std::string &path : pathList) {
      if (check_tools_path(path, tools)) {
         return path;
      }
   }
   return std::nullopt;
}

void print_histogram(std::list<std::tuple<std::string, int>> items, const std::string &title)
{
   items.sort([](const std::tuple<std::string, int> &lhs,
              const std::tuple<std::string, int> &rhs) -> bool {
      return std::get<1>(lhs) < std::get<1>(rhs);
   });
   // Select first "nice" bar height that produces more than 10 bars.
   int maxValue = 0;
   for (auto item : items) {
      int curValue = std::get<1>(item);
      if (curValue > maxValue) {
         maxValue = curValue;
      }
   }
   int power = (int)std::ceil(std::log10(maxValue));
   int N = 0;
   double barH = 0;
   while (true) {
      std::list<float> cycle{5, 2, 2.5, 1};
      for (float inc : cycle) {
         barH = inc * std::pow(10, power);
         N = (int)std::ceil(maxValue / barH);
         if (N > 10) {
            goto end_iterator;
         } else if(inc == 1) {
            power -= 1;
         }
      }
   }
end_iterator:
   std::unique_ptr<std::set<std::string>[]> histo(new std::set<std::string>[N]);
   for (int i = 0; i < N; ++i) {
      // @TODO maybe have issue
      histo[i] = std::set<std::string>{};
   }
   for (auto &item : items) {
      int bin = std::min(int(N * std::get<1>(item) / maxValue), N - 1);
      histo[bin].insert(std::get<0>(item));
   }
   int barW = 40;
   std::string hr(barW + 34, '-');
   std::printf("\nSlowest %s:\n", title.c_str());
   std::cout << hr << std::endl;
   int pDigits = (int)std::ceil(std::log10(maxValue));
   int pfDigits = std::max(0, 3 - pDigits);
   if (pfDigits) {
      pDigits += pfDigits + 1;
   }
   int cDigits = (int)std::ceil(std::log10(items.size()));
   std::printf("[%s] :: [%s] :: [%s]\n", center_string("Range", (pDigits + 1) * 2 + 3).c_str(),
               center_string("Percentage", barW).c_str(),
               center_string("Count", cDigits * 2 + 1).c_str());
   std::cout << hr << std::endl;
   for (int i = 0; i < N; ++i){
      const std::set<std::string> &row = histo[i];
      float pct = float(row.size()) / items.size();
      int w = int(barW * pct);
      std::printf("[%*.*fs,%*.*fs) :: [%s%s] :: [%*lu/%*lu]\n",
                  pDigits, pfDigits, i * barH, pDigits, pfDigits, (i + 1) * barH,
                  std::string(w, '*').c_str(), std::string(barW - w, ' ').c_str(),
                  cDigits, row.size(), cDigits, items.size());
   }
}

std::string center_string(const std::string &text, size_t width, char fillChar)
{
   size_t textSize = text.size();
   if (width < textSize) {
      return text;
   }
   size_t halfWidth = (width - textSize) / 2;
   return std::string(halfWidth, fillChar) + text + std::string(halfWidth, fillChar);
}

bool string_startswith(const std::string &str, const std::string &searchStr) noexcept
{
   if (str.size() < searchStr.size()) {
      return false;
   } else if (searchStr.empty()) {
      return true;
   }
   return 0 == str.find(searchStr);
}

bool string_endswith(const std::string &str, const std::string &searchStr) noexcept
{
   size_t strSize = str.size();
   size_t searchStrSize = searchStr.size();
   if (strSize < searchStrSize) {
      return false;
   } else if (searchStr.empty()) {
      return true;
   }
   return (strSize - searchStrSize) == str.find(searchStr);
}

std::string join_string_list(const std::list<std::string> &list, const std::string &glue) noexcept
{
   std::string result;
   size_t listSize = list.size();
   auto iter = list.begin();
   auto endMark = list.end();
   size_t i = 0;
   while (iter != endMark) {
      if (i == listSize - 1) {
         result += *iter;
      } else {
         result += *iter + glue;
      }
      ++i;
      ++iter;
   }
   return result;
}

std::string join_string_list(const std::vector<std::string> &list, const std::string &glue) noexcept
{
   std::string result;
   size_t listSize = list.size();
   auto iter = list.begin();
   auto endMark = list.end();
   size_t i = 0;
   while (iter != endMark) {
      if (i == listSize - 1) {
         result += *iter;
      } else {
         result += *iter + glue;
      }
      ++i;
      ++iter;
   }
   return result;
}

void replace_string(const std::string &search, const std::string &replacement,
                    std::string &targetStr)
{
   size_t pos = std::string::npos;
   size_t startPos = 0;
   size_t searchSize = search.size();
   size_t targetStrSize = targetStr.size();
   size_t replacementSize = replacement.size();
   do {
      pos = targetStr.find(search, startPos);
      if (pos == std::string::npos) {
         break;
      }
      startPos = std::max(pos + replacementSize, targetStrSize);
      targetStr.replace(pos, searchSize, replacement);
   } while (true);
}

// trim from start (in place)
void ltrim_string(std::string &str)
{
   str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) {
      return !std::isspace(ch);
   }));
}

// trim from end (in place)
void rtrim_string(std::string &str)
{
   str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) {
      return !std::isspace(ch);
   }).base(), str.end());
}

// trim from both ends (in place)
void trim_string(std::string &str)
{
   ltrim_string(str);
   rtrim_string(str);
}

} // lit
} // polar
