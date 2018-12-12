// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/07.

#include <iostream>
#include <string>
#include <list>
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char *argv[])
{
   if (argc < 3) {
      std::cerr << "Wrong number of args" << std::endl;
      return 1;
   }
   std::string type = argv[1];
   std::list<std::string> paths;
   int exitCode = 0;
   for (int i = 2; i < argc; ++i) {
      paths.push_back(argv[i]);
   }
   std::cout << std::boolalpha;
   if (type == "dir") {
      for (const std::string &path : paths) {
         std::cout << fs::is_directory(path) << std::endl;
      }
   } else if (type == "file") {
      for (const std::string &path : paths) {
         std::cout << fs::is_regular_file(path) << std::endl;
      }
   } else {
      std::cerr << "Unrecognised type " << type << std::endl;
      exitCode = 1;
   }
   return exitCode;
}
