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
#include "polarphp/syntax/TokenKinds.h"
#include "polarphp/syntax/serialization/TokenKindTypeSerialization.h"
#include "polarphp/parser/serialization/TokenJsonSerialization.h"
#include "nlohmann/json.hpp"

#include <memory>
#include <iostream>
#include <fstream>

using llvm::MemoryBuffer;
using llvm::ErrorOr;
using polar::kernel::LangOptions;
using polar::basic::SourceManager;
using polar::parser::Lexer;
using polar::parser::Token;
using polar::parser::TokenKindType;
using nlohmann::json;

#define READ_STDIN_ERROR 1
#define OPEN_SOURCE_FILE_ERROR 2
#define OPEN_OUTPUT_FILE_ERROR 3

int main(int argc, char * argv[])
{
   CLI::App tokenizer;
   std::string filePath;
   std::string outputFilePath;
   tokenizer.name("polar-tokenizer");
   tokenizer.footer("\nCopyright (c) 2019-2020 polar software foundation");
   tokenizer.add_option("sourceFilepath", filePath, "path of file to be tokenized, use stdin if not specified");
   tokenizer.add_option("-o,--output", outputFilePath, "process result write into file path");
   POLAR_CLI11_PARSE(tokenizer, argc, argv);
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
   Lexer lexer(langOpts, sourceMgr, bufferId, nullptr);
   Token currentToken;
   json tokenJsonArray = json::array();
   do {
      lexer.lex(currentToken);
      tokenJsonArray.push_back(currentToken);
   } while (currentToken.isNot(TokenKindType::END));
   *output << tokenJsonArray.dump(3);
   output->flush();
   return 0;
}
