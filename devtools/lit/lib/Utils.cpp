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

#include "Utils.h"
#include "ProcessUtils.h"
#include "Config.h"
#include <cmath>
#include <set>
#include <iostream>

namespace polar {
namespace lit {

std::list<std::string> split_string(const std::string &str, char separator)
{
   std::string buff;
   std::list<std::string> parts;
   for(auto n : str) {
      if(n != separator) {
         buff+=n;
      } else if(n == separator && buff != "") {
         parts.push_back(buff);
         buff = "";
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
   RunCmdResponse result = run_program("xcrun", "--show-sdk-version", "--sdk", "macosx");
   if (std::get<0>(result)) {
      return std::get<1>(result);
   }
   return std::nullopt;
}

std::optional<std::string> which(const std::string &command, const std::optional<std::string> &paths) noexcept
{
   fs::path commandPath(command);
   if (commandPath.is_absolute() && fs::exists(commandPath)) {
      return fs::canonical(commandPath).string();
   }
   std::string cmdStrPaths;
   // current only support posix os
   if (!paths.has_value()) {
      cmdStrPaths = std::getenv("PATH");
   } else {
      cmdStrPaths = paths.value();
   }
   std::list<std::string> dirs(split_string(cmdStrPaths, ':'));
   for (std::string dir : dirs) {
      if (dir == "") {
         dir = ".";
      }
      fs::path path(dir);
      path /= command;
      if (find_executable(path)) {
         return path.string();
      }
   }
   return std::nullopt;
}

bool check_tools_path(const std::string &dir, const std::list<std::string> &tools) noexcept
{
   fs::path basePath(dir);
   for (const std::string &tool : tools) {
      fs::path toolPath = basePath / tool;
      if (!fs::exists(toolPath)) {
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
   std::printf("\nSlowest %s:", title);
   std::cout << hr << std::endl;
   int pDigits = (int)std::ceil(std::log10(maxValue));
   int pfDigits = std::max(0, 3 - pDigits);
   if (pfDigits) {
      pDigits += pfDigits + 1;
   }
   int cDigits = (int)std::ceil(std::log10(items.size()));
   std::printf("[%s] :: [%s] :: [%s]", center_string("Range", (pDigits + 1) * 2 + 3).c_str(),
               center_string("Percentage", barW).c_str(),
               center_string("Count", cDigits * 2 + 1).c_str());
   std::cout << hr << std::endl;
   for (int i = 0; i < N; ++i){
      const std::set<std::string> &row = histo[i];
      float pct = float(row.size()) / items.size();
      int w = int(barW * pct);
      std::printf("[%*.*fs,%*.*fs) :: [%s%s] :: [%*d/%*d]",
                     pDigits, pfDigits, i * barH, pDigits, pfDigits, (i + 1) * barH,
                     std::string(w, '*').c_str(), std::string(barW - w, ' ').c_str(),
                  cDigits, row.size(), cDigits, items.size());
   }
}

std::string center_string(const std::string &text, int width, char fillChar)
{
   size_t textSize = text.size();
   if (width < textSize) {
      return text;
   }
   size_t halfWidth = (width - textSize) / 2;
   return std::string(halfWidth, fillChar) + text + std::string(halfWidth, fillChar);
}

} // lit
} // polar
