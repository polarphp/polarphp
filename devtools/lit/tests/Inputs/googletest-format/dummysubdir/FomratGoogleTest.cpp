// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/11/15.

#include "CLI/CLI.hpp"
#include <string>
#include <iostream>
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/SmallVector.h"

using polar::basic::StringRef;
using polar::basic::SmallVector;

int main(int argc, char *argv[])
{
   if (argc != 2) {
      std::cerr << "unexpected number of args" << std::endl;
      return -1;
   }
   if (StringRef(argv[1]) == "--gtest_list_tests") {
      std::string output = R"(
FirstTest.
  subTestA
  subTestB
ParameterizedTest/0.
  subTest
ParameterizedTest/1.
  subTest
)";
      std::cout << output << std::endl;
      return 0;
   } else if (!StringRef(argv[1]).startsWith("--gtest_filter=")) {
      std::cerr << "unexpected argument: " << argv[1] << std::endl;
      return -1;
   }
   SmallVector<StringRef, 2> parts;
   StringRef(argv[1]).split(parts, '=');
   StringRef testName = parts[1];
   if (testName == "FirstTest.subTestA") {
      std::cout << "I am subTest A, I PASS" << std::endl;
      std::cout << "[  PASSED  ] 1 test." << std::endl;
      return 0;
   } else if (testName == "FirstTest.subTestB") {
      std::cerr << "I am subTest B, I FAIL" << std::endl;
      std::cerr << "And I have two lines of output" << std::endl;
      return 1;
   } else if (testName == "ParameterizedTest/0.subTest" ||
              testName == "ParameterizedTest/1.subTest") {
      std::cout << "I am a parameterized test, I also PASS" << std::endl;
      std::cout << "[  PASSED  ] 1 test.'" << std::endl;
      return 0;
   } else {
      std::cerr << "error: invalid test name: " << testName.getStr() << std::endl;
      return -1;
   }
}
