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

#include "polarphp/parser/Lexer.h"
#include "polarphp/parser/CommonDefs.h"

namespace polar::parser {

Lexer::Lexer(const PrincipalTag &, const LangOptions &langOpts,
             const SourceManager &sourceMgr, unsigned bufferId,
             DiagnosticEngine *diags, CommentRetentionMode commentRetention,
             TriviaRetentionMode triviaRetention)
   : m_langOpts(langOpts),
     m_sourceMgr(sourceMgr),
     m_bufferId(bufferId),
     m_diags(diags),
     m_commentRetention(commentRetention),
     m_triviaRetention(triviaRetention)
{}

Lexer::Lexer(const LangOptions &options, const SourceManager &sourceMgr,
             unsigned bufferId, DiagnosticEngine *diags,
             CommentRetentionMode commentRetain, TriviaRetentionMode triviaRetention)
   : Lexer(PrincipalTag(), options, sourceMgr, bufferId, diags,
           commentRetain, triviaRetention)
{
   unsigned endOffset = sourceMgr.getRangeForBuffer(bufferId).getByteLength();
   initialize(/*offset=*/0, endOffset);
}

Lexer::Lexer(const LangOptions &options, const SourceManager &sourceMgr,
             unsigned bufferId, DiagnosticEngine *diags,
             CommentRetentionMode commentRetain, TriviaRetentionMode triviaRetention,
             unsigned offset, unsigned endOffset)
   : Lexer(PrincipalTag(), options, sourceMgr, bufferId, diags, commentRetain, triviaRetention)
{
   initialize(offset, endOffset);
}

Lexer::Lexer(Lexer &parent, State beginState, State endState)
   : Lexer(PrincipalTag(), parent.m_langOpts, parent.m_sourceMgr,
           parent.m_bufferId, parent.m_diags, parent.m_commentRetention,
           parent.m_triviaRetention)
{
   assert(m_bufferId == m_sourceMgr.findBufferContainingLoc(beginState.m_loc) &&
          "state for the wrong buffer");
   assert(m_bufferId == m_sourceMgr.findBufferContainingLoc(endState.m_loc) &&
          "state for the wrong buffer");

   unsigned offset = m_sourceMgr.getLocOffsetInBuffer(beginState.m_loc, m_bufferId);
   unsigned endOffset = m_sourceMgr.getLocOffsetInBuffer(endState.m_loc, m_bufferId);
   initialize(offset, endOffset);
}

void Lexer::initialize(unsigned offset, unsigned endOffset)
{}

} // polar::parser
