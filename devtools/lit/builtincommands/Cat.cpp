// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/09/18.

#include "CLI/CLI.hpp"
#include "LitConfigDef.h"
#include <iostream>
#include <thread>
#include <assert.h>
#include <filesystem>
#include <list>

namespace fs = std::filesystem;

std::string convert_to_caret_and_mnotation(std::wstring data)
{
   std::string output;
   for (wchar_t c : data) {
      if (c == 9 || c == 10) {
         output.push_back(c);
         continue;
      }
      if (c > 127) {
         c = c - 128;
         output += "M-";
      }
      if (c < 32) {
         output.push_back('^');
         output.push_back(c + 64);
      } else if (c == 127) {
         output += "^?";
      } else {
         output.push_back(c);
      }
   }
   return output;
}

void general_exception_handler(std::exception_ptr eptr)
{
   try {
      if (eptr) {
         std::rethrow_exception(eptr);
      }
   } catch (const std::exception &exp) {
      std::cerr << exp.what() << std::endl;
      exit(1);
   }
}

int main(int argc, char *argv[])
{
   CLI::App catApp;
   bool showNonprinting = false;
   std::vector<std::string> filenames;
   catApp.add_flag("-v, --show-nonprinting", showNonprinting, "show all non printable char");
   catApp.add_option("filenames", filenames, "Filenames to been print")->required();
   CLI11_PARSE(catApp, argc, argv);
   std::exception_ptr eptr;
   try {
      wchar_t buffer[1024];
      for (const std::string &filename : filenames) {
         if (!fs::exists(filename)) {
            throw std::runtime_error(std::string("No such file or directory: ") + filename);
         }
         std::wifstream fstream(filename, std::ios_base::in | std::ios_base::binary);
         if (!fstream.is_open()) {
            throw std::runtime_error(std::string("open file ") + filename + " failure");
         }
         while (!fstream.eof()) {
            fstream.read(buffer, 1024);
            if (showNonprinting) {
               std::cout << convert_to_caret_and_mnotation(std::wstring(buffer, fstream.gcount()));
            } else {
               std::wcout << std::wstring(buffer, fstream.gcount());
            }
         }
         std::cout.flush();
      }
   } catch (...) {
      eptr = std::current_exception();
   }
   general_exception_handler(eptr);
   return 0;
}
