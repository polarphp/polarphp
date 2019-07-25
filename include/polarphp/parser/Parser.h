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
#include "polarphp/parser/ParsedTrivia.h"

namespace polar::ast {
class DiagnosticEngine;
} // polar::ast

namespace polar::kernel {
class LangOptions;
} // polar::kernel

namespace polar::syntax {
class Syntax;
} // polar::syntax

namespace polar::parser {

using polar::basic::StringRef;
using polar::ast::DiagnosticEngine;
using polar::kernel::LangOptions;
using polar::syntax::Syntax;

class SourceManager;
class Lexer;

void parse_error(StringRef msg);

class Parser
{
public:
   Parser(const LangOptions &langOpts, unsigned bufferId, SourceManager &sourceMgr,
          DiagnosticEngine &diags);
   Parser(SourceManager &sourceMgr, DiagnosticEngine &diags, std::unique_ptr<Lexer> lexer);
   Parser(const Parser &) = delete;
   Parser &operator =(const Parser &) = delete;

   void parse();
   std::shared_ptr<Syntax> getSyntaxTree();
   const Token &peekToken();
   SourceLoc getEndOfPreviousLoc();

   ///
   /// TODO
   /// state manage methods
   ///

   ~Parser();

private:
   /// info properties
   bool m_parserError = false;
   bool m_inCompilation = false;

   SourceManager &m_sourceMgr;
   DiagnosticEngine &m_diags;
   Lexer *m_lexer;

   /// The location of the previous token.
   SourceLoc m_previousLoc;

   Token m_token;

   /// leading trivias for \c Token.
   /// Always empty if not shouldBuildSyntaxTree.
   ParsedTrivia m_leadingTrivia;

   /// trailing trivias for \c Token.
   /// Always empty if not shouldBuildSyntaxTree.
   ParsedTrivia m_trailingTrivia;

   std::string m_docComment;
   std::list<std::string> m_openFiles;
   std::shared_ptr<Syntax> m_ast;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSER_H
