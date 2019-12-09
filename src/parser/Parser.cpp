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

#include "polarphp/kernel/LangOptions.h"
#include "polarphp/parser/Parser.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/syntax/Syntax.h"

namespace polar::parser {

const Trivia Parser::sm_emptyTrivia{};

Parser::Parser(const LangOptions &langOpts, unsigned bufferId,
               SourceManager &sourceMgr, std::shared_ptr<DiagnosticEngine> diags)
   : Parser(sourceMgr, diags,
            std::unique_ptr<Lexer>(new Lexer(langOpts, sourceMgr, bufferId,
                                             diags.get(), langOpts.AttachCommentsToDecls ?
                                                CommentRetentionMode::AttachToNextToken : CommentRetentionMode::None,
                                             TriviaRetentionMode::WithTrivia)))
{}

Parser::Parser(SourceManager &sourceMgr, std::shared_ptr<DiagnosticEngine> diags,
               std::unique_ptr<Lexer> lexer)
   : m_sourceMgr(sourceMgr),
     m_lexer(lexer.release()),
     m_diags(diags)
{
   m_yyParser = std::make_unique<internal::YYParser>(this, m_lexer);
   m_token.setKind(TokenKindType::T_UNKNOWN_MARK);
}

bool Parser::parse()
{
   m_inCompilation = true;
   int status = m_yyParser->parse();
   m_inCompilation = false;
   return status;
}

void Parser::setParsedAst(RefCountPtr<RawSyntax> ast)
{
   m_ast = ast;
}

RefCountPtr<RawSyntax> Parser::getSyntaxTree()
{
   assert(m_token.is(TokenKindType::END) && "not done parsing yet");
   return m_ast;
}

Parser::~Parser()
{
   delete m_lexer;
}

} // polar::parser
