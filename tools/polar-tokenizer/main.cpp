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
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/OptionalError.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/parser/Token.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/parser/SourceMgr.h"
#include "polarphp/syntax/TokenKinds.h"

#include <memory>
#include <iostream>
#include <fstream>

using polar::utils::MemoryBuffer;
using polar::utils::OptionalError;
using polar::kernel::LangOptions;
using polar::parser::SourceManager;
using polar::parser::Lexer;
using polar::parser::Token;
using polar::parser::TokenKindType;

#define READ_STDIN_ERROR 1
#define OPEN_SOURCE_FILE_ERROR 2
#define OPEN_OUTPUT_FILE_ERROR 3

int main(int argc, char * argv[])
{
   CLI::App tokenizer;
   std::string filePath;
   std::string outputFilePath;
   tokenizer.add_option("sourceFilepath", filePath, "path of file to be tokenized");
   tokenizer.add_option("-o,--output", outputFilePath, "process result write into file path");
   POLAR_CLI11_PARSE(tokenizer, argc, argv);
   std::unique_ptr<MemoryBuffer> sourceBuffer;
   if (filePath.empty()) {
      OptionalError<std::unique_ptr<MemoryBuffer>> tempBuffer = MemoryBuffer::getStdIn();
      if (tempBuffer) {
         sourceBuffer = std::move(tempBuffer.get());
      } else {
         std::cerr << "read stdin error: " << tempBuffer.getError() << std::endl;
         return READ_STDIN_ERROR;
      }
   } else {
      OptionalError<std::unique_ptr<MemoryBuffer>> tempBuffer = MemoryBuffer::getFile(filePath);
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
   }
   LangOptions langOpts{};
   SourceManager sourceMgr;
   unsigned bufferId = sourceMgr.addNewSourceBuffer(std::move(sourceBuffer));
   Lexer lexer(langOpts, sourceMgr, bufferId, nullptr);
   Token currentToken;
   do {
      lexer.lex(currentToken);
   } while (currentToken.isNot(TokenKindType::END));
   return 0;
}
