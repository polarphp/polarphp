// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/19.

//===----------------------------------------------------------------------===//
// Usage:
//   not cmd
//     Will return true if cmd doesn't crash and returns false.
//   not --crash cmd
//     Will return true if cmd crashes (e.g. for testing crash reporting).

#include "CLI/CLI.hpp"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/InitPolar.h"
#include "polarphp/utils/Program.h"
#include "polarphp/utils/RawOutStream.h"

using polar::basic::StringRef;
using namespace polar::utils;
using namespace polar::basic;

int main(int argc, char *argv[])
{
   bool expectCrash = false;
   ++argv;
   --argc;
   if (argc > 0 && StringRef(argv[0]) == "--crash") {
      ++argv;
      --argc;
      expectCrash = true;
   }

   if (argc == 0) {
      return 1;
   }
   OptionalError<std::string> finded = polar::sys::find_program_by_name(argv[0]);
   if (!finded) {
      polar::utils::error_stream() << "Error: Unable to find `" << argv[0]
                                   << "' in PATH: " << finded.getError().message() << "\n";
   }
   std::vector<StringRef> subArgv;
   subArgv.reserve(argc);
   for (int i = 0; i < argc; ++i) {
      subArgv.push_back(argv[i]);
   }
   std::string errMsg;
   int result = polar::sys::execute_and_wait(*finded, subArgv, std::nullopt, {}, {}, 0, 0, &errMsg);
#ifdef _WIN32
   // Handle abort() in msvcrt -- It has exit code as 3.  abort(), aka
   // unreachable, should be recognized as a crash.  However, some binaries use
   // exit code 3 on non-crash failure paths, so only do this if we expect a
   // crash.
   if (expectCrash && result == 3) {
       result = -3;
   }
#endif
   if (result < 0) {
      polar::utils::error_stream() << "Error: " << errMsg << "\n";
      if (expectCrash) {
         return 0;
      }
      return 1;
   }
   if (expectCrash) {
      return 1;
   }
   return result == 0;
}
