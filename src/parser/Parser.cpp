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

#include "polarphp/parser/Parser.h"
#include "polarphp/kernel/LangOptions.h"
#include "polarphp/parser/Lexer.h"

namespace polar::parser {

Parser::Parser(const LangOptions &langOpts, unsigned bufferId,
               SourceManager &sourceMgr, DiagnosticEngine &diags)
   : Parser(sourceMgr, diags,
            std::unique_ptr<Lexer>(new Lexer(langOpts, sourceMgr, bufferId,
                                             &diags, langOpts.attachCommentsToDecls ?
                                                CommentRetentionMode::AttachToNextToken : CommentRetentionMode::None,
                                             TriviaRetentionMode::WithTrivia)))
{}

Parser::Parser(SourceManager &sourceMgr, DiagnosticEngine &diags,
               std::unique_ptr<Lexer> lexer)
   : m_sourceMgr(sourceMgr),
     m_diags(diags),
     m_lexer(lexer.release())
{
   m_token.setKind(TokenKindType::T_UNKOWN_MARK);
}

Parser::~Parser()
{
   delete m_lexer;
}

} // polar::parser
