// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/06/05.

#ifndef POLARPHP_PARSER_LEXER_STATE_H
#define POLARPHP_PARSER_LEXER_STATE_H

#include "polarphp/basic/SourceLoc.h"
#include "polarphp/parser/ParsedTrivia.h"
#include "polarphp/parser/LexerFlags.h"
#include "polarphp/parser/internal/YYLexerDefs.h"

#include <stack>
#include <optional>

namespace polar::parser {

class Lexer;

/// Lexer state can be saved/restored to/from objects of this class.

class LexerState
{
public:
   LexerState()
   {}

   bool isValid() const
   {
      return m_loc.isValid();
   }

   LexerState advance(unsigned offset) const
   {
      assert(isValid());
      return LexerState(m_loc.getAdvancedLoc(offset));
   }

   LexerState &setYYLength(unsigned int length)
   {
      m_yyLength = length;
      return *this;
   }

   unsigned int getYYLength() const
   {
      return m_yyLength;
   }

   LexerState &setLineNumber(unsigned int number)
   {
      m_lineNumber = number;
      return *this;
   }

   unsigned int getLineNumber() const
   {
      return m_lineNumber;
   }

   LexerState &setBufferStart(const unsigned char *start)
   {
      m_bufferStart = start;
      return *this;
   }

   const unsigned char *getBufferStart() const
   {
      return m_bufferStart;
   }

   LexerState &setBufferEnd(const unsigned char *end)
   {
      m_bufferEnd = end;
      return *this;
   }

   const unsigned char *getBufferEnd() const
   {
      return m_bufferEnd;
   }

   LexerState &setYYLimit(const unsigned char *limit)
   {
      m_artificialEof = limit;
      return *this;
   }

   const unsigned char *getYYLimit() const
   {
      return m_artificialEof;
   }

   LexerState &setCodeCompletionPtr(const unsigned char *ptr)
   {
      m_codeCompletionPtr = ptr;
      return *this;
   }

   const unsigned char *getCodeCompletionPtr() const
   {
      return m_codeCompletionPtr;
   }

   LexerState &setContentStart(const unsigned char *start)
   {
      m_contentStart = start;
      return *this;
   }

   const unsigned char *getContentStart() const
   {
      return m_contentStart;
   }

   LexerState &setYYText(const unsigned char *text)
   {
      m_yyText = text;
      return *this;
   }

   const unsigned char *getYYText() const
   {
      return m_yyText;
   }

   LexerState &setYYCursor(const unsigned char *cursor)
   {
      m_yyCursor = cursor;
      return *this;
   }

   const unsigned char *getYYCursor() const
   {
      return m_yyCursor;
   }

   LexerState &setYYMarker(const unsigned char *marker)
   {
      m_yyMarker = marker;
      return *this;
   }

   const unsigned char *getYYMarker() const
   {
      return m_yyMarker;
   }

   LexerState &setCondition(YYLexerCondType cond)
   {
      m_yyCondition = cond;
      return *this;
   }

   YYLexerCondType getCondition() const
   {
      return m_yyCondition;
   }

   LexerState &setLexerFlags(LexerFlags flags)
   {
      m_flags = flags;
      return *this;
   }

   LexerFlags getLexerFlags() const
   {
      return m_flags;
   }

   LexerState &setLexicalEventHandler(LexicalEventHandler handler)
   {
      m_eventHandler = handler;
      return *this;
   }

   LexicalEventHandler getLexicalEventHandler() const
   {
      return m_eventHandler;
   }

   LexerState &setLexicalExceptionHandler(LexicalExceptionHandler handler)
   {
      m_lexicalExceptionHandler = handler;
      return *this;
   }

   LexicalExceptionHandler getLexicalExceptionHandler() const
   {
      return m_lexicalExceptionHandler;
   }

   LexerState &setConditionStack(std::stack<YYLexerCondType> &&stack)
   {
      m_yyConditionStack = std::move(stack);
      return *this;
   }

   std::stack<YYLexerCondType> &getConditionStack()
   {
      return m_yyConditionStack;
   }

   LexerState &setHeredocLabelStack(const std::stack<std::shared_ptr<HereDocLabel>> &stack)
   {
      m_heredocLabelStack = stack;
      return *this;
   }

   LexerState &setHeredocLabelStack(std::stack<std::shared_ptr<HereDocLabel>> &&stack)
   {
      m_heredocLabelStack = std::move(stack);
      return *this;
   }

   std::stack<std::shared_ptr<HereDocLabel>> &getHeredocLabelStack()
   {
      return m_heredocLabelStack;
   }
private:
   explicit LexerState(SourceLoc loc)
      : m_loc(loc)
   {}

   unsigned int m_yyLength;
   unsigned int m_lineNumber;
   const unsigned char *m_bufferStart;
   const unsigned char *m_bufferEnd;
   const unsigned char *m_artificialEof;
   const unsigned char *m_codeCompletionPtr;
   const unsigned char *m_contentStart;
   const unsigned char *m_yyText;
   const unsigned char *m_yyCursor;
   const unsigned char *m_yyMarker;
   YYLexerCondType m_yyCondition;
   LexerFlags m_flags;
   SourceLoc m_loc;
   std::optional<ParsedTrivia> m_leadingTrivia;
   LexicalEventHandler m_eventHandler;
   LexicalExceptionHandler m_lexicalExceptionHandler;

   std::stack<YYLexerCondType> m_yyConditionStack;
   std::stack<std::shared_ptr<HereDocLabel>> m_heredocLabelStack;

   friend class Lexer;
};

} // polar::parser

#endif // POLARPHP_PARSER_LEXER_STATE_H
