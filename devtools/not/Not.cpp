// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
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
#include "llvm/ADT/StringRef.h"
#include "polarphp/utils/InitPolar.h"
#include "llvm/Support/Program.h"
#include "llvm/Support/raw_ostream.h"

using llvm::StringRef;
using namespace llvm;

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
   ErrorOr<std::string> finded = sys::findProgramByName(argv[0]);
   if (!finded) {
      errs() << "Error: Unable to find `" << argv[0]
             << "' in PATH: " << finded.getError().message() << "\n";
   }
   std::vector<StringRef> subArgv;
   subArgv.reserve(argc);
   for (int i = 0; i < argc; ++i) {
      subArgv.push_back(argv[i]);
   }
   std::string errMsg;
   int result = sys::ExecuteAndWait(*finded, subArgv, None, {}, 0, 0, &errMsg);
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
      errs() << "Error: " << errMsg << "\n";
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
