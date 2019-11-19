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
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ErrorOr.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/parser/Token.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/parser/SourceMgr.h"
#include "polarphp/parser/Parser.h"

#include <memory>
#include <iostream>
#include <fstream>

using llvm::MemoryBuffer;
using llvm::ErrorOr;
using polar::kernel::LangOptions;
using polar::parser::SourceManager;
using polar::parser::Parser;

int main(int argc, char * argv[])
{
   CLI::App parser;
   std::string filePath;
   std::string outputFilePath;
   parser.name("polar-ast-dumper");
   parser.footer("\nCopyright (c) 2019-2020 polar software foundation");
   parser.add_option("sourceFilepath", filePath, "path of file to be parser, use stdin if not specified");
   parser.add_option("-o,--output", outputFilePath, "process result write into file path");
   POLAR_CLI11_PARSE(parser, argc, argv);
   return 0;
}
