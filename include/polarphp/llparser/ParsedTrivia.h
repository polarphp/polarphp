// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/12.

#ifndef POLARPHP_LLPARSER_PARSEDTRIVIA_H
#define POLARPHP_LLPARSER_PARSEDTRIVIA_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

namespace polar {
class SourceLoc;
class SourceManager;
} // polar

namespace polar::llparser {

using llvm::SmallVector;
using llvm::ArrayRef;

enum class TriviaKind {
   Space, // 'A space ' ' character.
   Tab, // A tab '\t' character.
   VerticalTab, // A vertical tab '\v' character.
   Formfeed, // A form-feed 'f' character.
   Newline, // A newline '\n' character.
   CarriageReturn, // A newline '\r' character.
   CarriageReturnLineFeed, // A newline consists of contiguous '\r' and '\n' characters.
   LineComment, // A developer line comment, starting with '//'
   BlockComment, // A developer block comment, starting with \'/*\' and ending with  \'*/\'.
   DocLineComment, // A documentation line comment, starting with '///'.
   DocBlockComment, // A documentation block comment, starting with '/**' and ending with '*/'.
   GarbageText, // Any skipped garbage text.
};


class ParsedTriviaPiece {
   TriviaKind Kind;
   unsigned Length;

public:
   ParsedTriviaPiece(TriviaKind kind, unsigned length)
      : Kind(kind), Length(length) {}

   TriviaKind getKind() const { return Kind; }

   /// Return the text of the trivia.
   unsigned getLength() const { return Length; }

   void extendLength(unsigned len) {
      Length += len;
   }

   static size_t getTotalLength(ArrayRef<ParsedTriviaPiece> pieces) {
      size_t Len = 0;
      for (auto &p : pieces)
         Len += p.getLength();
      return Len;
   }

   bool operator==(const ParsedTriviaPiece &Other) const {
      return Kind == Other.Kind && Length == Other.Length;
   }

   bool operator!=(const ParsedTriviaPiece &Other) const {
      return !(*this == Other);
   }
};

using ParsedTriviaList = SmallVector<ParsedTriviaPiece, 3>;

struct ParsedTrivia {
   ParsedTriviaList Pieces;

   /// Get the begin iterator of the pieces.
   ParsedTriviaList::const_iterator begin() const {
      return Pieces.begin();
   }

   /// Get the end iterator of the pieces.
   ParsedTriviaList::const_iterator end() const {
      return Pieces.end();
   }

   /// Clear pieces.
   void clear() {
      Pieces.clear();
   }

   /// Returns true if there are no pieces in this Trivia collection.
   bool empty() const {
      return Pieces.empty();
   }

   /// Return the number of pieces in this Trivia collection.
   size_t size() const {
      return Pieces.size();
   }

   size_t getLength() const {
      return ParsedTriviaPiece::getTotalLength(Pieces);
   }

   void push_back(TriviaKind kind, unsigned length) {
      Pieces.emplace_back(kind, length);
   }

   void appendOrSquash(TriviaKind kind, unsigned length) {
      if (empty() || Pieces.back().getKind() != kind) {
         push_back(kind, length);
      } else {
         Pieces.back().extendLength(length);
      }
   }

   bool operator==(const ParsedTrivia &Other) const {
      if (Pieces.size() != Other.size()) {
         return false;
      }

      for (size_t i = 0; i < Pieces.size(); ++i) {
         if (Pieces[i] != Other.Pieces[i]) {
            return false;
         }
      }

      return true;
   }

   bool operator!=(const ParsedTrivia &Other) const {
      return !(*this == Other);
   }

};

bool isCommentTriviaKind(TriviaKind kind);

} // polar::llparser

#endif //POLARPHP_LLPARSER_PARSEDTRIVIA_H
