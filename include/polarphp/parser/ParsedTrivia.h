//===--- ParsedTrivia.h - ParsedTrivia API ----------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
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

#ifndef POLARPHP_PARSER_PARSED_TRIVIA_H
#define POLARPHP_PARSER_PARSED_TRIVIA_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

namespace polar::syntax {
enum class TriviaKind : uint8_t;
struct Trivia;
} // polar::syntax

namespace polar {
class SourceLoc;
class SourceManager;
} // polar

namespace polar::parser {

using polar::syntax::Trivia;
using polar::syntax::TriviaKind;
using llvm::ArrayRef;
using llvm::SmallVector;
using polar::SourceLoc;
using polar::SourceManager;

class ParsedTriviaPiece
{
public:
public:
   ParsedTriviaPiece(syntax::TriviaKind kind, unsigned length)
      : m_kind(kind),
        m_length(length)
   {}

   syntax::TriviaKind getKind() const
   {
      return m_kind;
   }

   /// Return the text of the trivia.
   unsigned getLength() const
   {
      return m_length;
   }

   void extendLength(unsigned len)
   {
      m_length += len;
   }

   static size_t getTotalLength(ArrayRef<ParsedTriviaPiece> pieces)
   {
      size_t length = 0;
      for (auto &p : pieces) {
         length += p.getLength();
      }
      return length;
   }

   bool operator==(const ParsedTriviaPiece &other) const
   {
      return m_kind == other.m_kind && m_length == other.m_length;
   }

   bool operator!=(const ParsedTriviaPiece &other) const
   {
      return !(*this == other);
   }

   static syntax::Trivia
   convertToSyntaxTrivia(ArrayRef<ParsedTriviaPiece> pieces, SourceLoc loc,
                         const SourceManager &sourceMgr, unsigned bufferID);
private:
   syntax::TriviaKind m_kind;
   unsigned m_length;
};

using ParsedTriviaList = SmallVector<ParsedTriviaPiece, 3>;

struct ParsedTrivia
{
   ParsedTriviaList pieces;

   /// Get the begin iterator of the pieces.
   ParsedTriviaList::const_iterator begin() const
   {
      return pieces.begin();
   }

   /// Get the end iterator of the pieces.
   ParsedTriviaList::const_iterator end() const
   {
      return pieces.end();
   }

   /// Clear pieces.
   void clear()
   {
      pieces.clear();
   }

   /// Returns true if there are no pieces in this Trivia collection.
   bool empty() const
   {
      return pieces.empty();
   }

   /// Return the number of pieces in this Trivia collection.
   size_t size() const
   {
      return pieces.size();
   }

   size_t getLength() const
   {
      return ParsedTriviaPiece::getTotalLength(pieces);
   }

   void push_back(syntax::TriviaKind kind, unsigned length)
   {
      pieces.emplace_back(kind, length);
   }

   void appendOrSquash(syntax::TriviaKind kind, unsigned length)
   {
      if (empty() || pieces.back().getKind() != kind) {
         push_back(kind, length);
      } else {
         pieces.back().extendLength(length);
      }
   }

   bool operator==(const ParsedTrivia &other) const
   {
      if (pieces.size() != other.size()) {
         return false;
      }

      for (size_t i = 0; i < pieces.size(); ++i) {
         if (pieces[i] != other.pieces[i]) {
            return false;
         }
      }

      return true;
   }

   bool operator!=(const ParsedTrivia &other) const
   {
      return !(*this == other);
   }

   syntax::Trivia convertToSyntaxTrivia(SourceLoc loc, const SourceManager &sourceMgr,
                                        unsigned bufferID) const;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_TRIVIA_H
