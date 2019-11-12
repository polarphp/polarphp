// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/12.

#include "CLI/CLI.hpp"
#include "polarphp/global/Global.h"

int main(int argc, char * argv[])
{
   CLI::App tokenizer;
   std::string filePath;
   tokenizer.add_option("sourceFilepath", filePath, "path of file to be tokenized");
   tokenizer.add_option("-o,--output", filePath, "process result write into file path");
   POLAR_CLI11_PARSE(tokenizer, argc, argv);
   return 0;
}
