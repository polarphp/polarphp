// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/09.

#ifndef POLARPHP_PARSER_PARSER_H
#define POLARPHP_PARSER_PARSER_H

#include <list>
#include <string>
#include <memory>

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/parser/CommonDefs.h"
#include "polarphp/parser/Token.h"

namespace polar::ast {
class DiagnosticEngine;
} // polar::ast

namespace polar::kernel {
class LangOptions;
} // polar::kernel

namespace polar::parser {

using polar::basic::StringRef;
using polar::ast::DiagnosticEngine;
using polar::kernel::LangOptions;

class SourceManager;
class Lexer;

class AstNode
{};

void parse_error(StringRef msg);

class Parser
{
public:
   Parser(const LangOptions &langOpts, unsigned bufferId, SourceManager &sourceMgr,
          DiagnosticEngine &diags);
   Parser(SourceManager &sourceMgr, DiagnosticEngine &diags, std::unique_ptr<Lexer> lexer);
   Parser(const Parser &) = delete;
   Parser &operator =(const Parser &) = delete;

   void incLineNumber(int count = 1)
   {
      m_lineNumber += count;
   }

   void parse();
   void getSyntaxTree();

   ~Parser();

private:
   /// info properties
   bool m_parserError = false;
   bool m_inCompilation = false;
   bool m_incrementLineNumber = false;
   int m_lineNumber;
   uint32_t m_startLineNumber;

   SourceManager &m_sourceMgr;
   DiagnosticEngine &m_diags;
   Lexer *m_lexer;

   Token m_token;
   std::string m_docComment;
   std::list<std::string> m_openFiles;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSER_H
