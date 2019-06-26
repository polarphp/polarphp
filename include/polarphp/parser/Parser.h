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

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/parser/CommonDefs.h"
#include "polarphp/parser/Token.h"

namespace polar::syntax {
class SourceFile;
} // polar::syntax

namespace polar::parser {

using polar::basic::StringRef;
using polar::syntax::SourceFile;

class SourceManager;
class Lexer;
class DiagnosticEngine;

class AstNode
{};

void parse_error(StringRef msg);

class Parser
{
public:
   Parser(const Parser &) = delete;
   Parser &operator =(const Parser &) = delete;

   void incLineNumber(int count = 1)
   {
      m_lineNumber += count;
   }

private:
   /// info properties
   bool m_incrementLineNumber;
   int m_lineNumber;
   uint32_t m_startLineNumber;

   SourceManager &m_sourceMgr;
   DiagnosticEngine &m_diags;
   SourceFile &m_sourceFile;
   Lexer *m_lexer;

   std::string m_docComment;
   std::list<std::string> m_openFiles;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSER_H
