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
#include "polarphp/basic/SourceMgr.h"
#include "polarphp/parser/Parser.h"

#include <memory>
#include <iostream>
#include <fstream>

#define READ_STDIN_ERROR 1
#define OPEN_SOURCE_FILE_ERROR 2
#define OPEN_OUTPUT_FILE_ERROR 3

using llvm::MemoryBuffer;
using llvm::ErrorOr;
using polar::kernel::LangOptions;
using polar::basic::SourceManager;
using polar::parser::Parser;
using polar::syntax::RefCountPtr;

int main(int argc, char * argv[])
{
   CLI::App parserApp;
   std::string filePath;
   std::string outputFilePath;
   parserApp.name("polar-ast-dumper");
   parserApp.footer("\nCopyright (c) 2019-2020 polar software foundation");
   parserApp.add_option("sourceFilepath", filePath, "path of file to be parser, use stdin if not specified");
   parserApp.add_option("-o,--output", outputFilePath, "process result write into file path");
   POLAR_CLI11_PARSE(parserApp, argc, argv);
   std::unique_ptr<MemoryBuffer> sourceBuffer;
   if (filePath.empty()) {
      ErrorOr<std::unique_ptr<MemoryBuffer>> tempBuffer = MemoryBuffer::getSTDIN();
      if (tempBuffer) {
         sourceBuffer = std::move(tempBuffer.get());
      } else {
         std::cerr << "read stdin error: " << tempBuffer.getError() << std::endl;
         return READ_STDIN_ERROR;
      }
   } else {
      ErrorOr<std::unique_ptr<MemoryBuffer>> tempBuffer = MemoryBuffer::getFile(filePath);
      if (tempBuffer) {
         sourceBuffer = std::move(tempBuffer.get());
      } else {
         std::cerr << "read source file error: " << tempBuffer.getError() << std::endl;
         return OPEN_SOURCE_FILE_ERROR;
      }
   }

   std::ostream *output = nullptr;
   std::unique_ptr<std::ofstream> foutstream;
   if (outputFilePath.empty()) {
      output = &std::cout;
   } else {
      foutstream = std::make_unique<std::ofstream>(outputFilePath, std::ios_base::out | std::ios_base::trunc);
      if (foutstream->fail()) {
         std::cerr << "open output file error: " << strerror(errno) << std::endl;
         return OPEN_OUTPUT_FILE_ERROR;
      }
      output = foutstream.get();
   }
   LangOptions langOpts{};
   SourceManager sourceMgr;
   unsigned bufferId = sourceMgr.addNewSourceBuffer(std::move(sourceBuffer));
   Parser parser(langOpts, bufferId, sourceMgr, nullptr);
   parser.parse();
   RefCountPtr<RawSyntax> syntaxTree = parser.getSyntaxTree();
   return 0;
}
