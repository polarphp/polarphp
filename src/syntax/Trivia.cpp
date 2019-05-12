//===--- Trivia.cpp - Swift Syntax Trivia Implementation ------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
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
// Created by polarboy on 2019/05/12.

#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/Trivia.h"

namespace polar::syntax {

TriviaPiece TriviaPiece::fromText(TriviaKind kind, StringRef text)
{
   switch (kind) {
   case TriviaKind::Space:
   case TriviaKind::Tab:
   case TriviaKind::VerticalTab:
   case TriviaKind::Formfeed:
   case TriviaKind::Newline:
   case TriviaKind::CarriageReturn:
   case TriviaKind::Backtick:
   case TriviaKind::LineComment:
   case TriviaKind::BlockComment:
   case TriviaKind::DocLineComment:
   case TriviaKind::DocBlockComment:
   case TriviaKind::GarbageText:
      return TriviaPiece(kind, OwnedString::makeRefCounted(text));
   case TriviaKind::CarriageReturnLineFeed:
      assert(text.size() % 2 == 0);
      return TriviaPiece(kind, text.size() / 2);
   }
   polar_unreachable("unknown kind");
}

void TriviaPiece::dump(RawOutStream &outStream, unsigned indent) const
{
   for (decltype(m_count) i = 0; i < indent; ++i) {
      outStream << ' ';
   }
   outStream << "(trivia ";
   switch (m_kind) {
   case TriviaKind::Space:
   case TriviaKind::Tab:
   case TriviaKind::VerticalTab:
   case TriviaKind::Formfeed:
   case TriviaKind::Newline:
   case TriviaKind::CarriageReturn:
   case TriviaKind::Backtick:
   case TriviaKind::LineComment:
   case TriviaKind::BlockComment:
   case TriviaKind::DocLineComment:
   case TriviaKind::DocBlockComment:
   case TriviaKind::GarbageText:
      retrieve_trivia_kind_name(m_kind);
      outStream.writeEscaped(m_text.getStr());
      break;
   case TriviaKind::CarriageReturnLineFeed:
      retrieve_trivia_kind_name(m_kind);
      outStream << m_count;
      break;
   default:
      polar_unreachable("unknown kind");
   }
   outStream << ')';
}

bool is_comment_trivia_kind(TriviaKind kind)
{
   switch (kind) {
   case TriviaKind::Space:
   case TriviaKind::Tab:
   case TriviaKind::VerticalTab:
   case TriviaKind::Formfeed:
   case TriviaKind::Newline:
   case TriviaKind::CarriageReturn:
   case TriviaKind::Backtick:
   case TriviaKind::GarbageText:
   case TriviaKind::CarriageReturnLineFeed:
      return false;
   case TriviaKind::LineComment:
   case TriviaKind::BlockComment:
   case TriviaKind::DocLineComment:
   case TriviaKind::DocBlockComment:
      return true;
   default:
      polar_unreachable("unknown kind");
   }

}

void TriviaPiece::accumulateAbsolutePosition(AbsolutePosition &pos) const
{
   switch (m_kind) {
   case TriviaKind::Newline:
   case TriviaKind::CarriageReturn:
   case TriviaKind::Space:
   case TriviaKind::Tab:
   case TriviaKind::VerticalTab:
   case TriviaKind::Formfeed:
   case TriviaKind::Backtick:
   case TriviaKind::GarbageText:
   case TriviaKind::LineComment:
   case TriviaKind::BlockComment:
   case TriviaKind::DocLineComment:
   case TriviaKind::DocBlockComment:
      pos.addText(m_text.getStr());
      break;
   case TriviaKind::CarriageReturnLineFeed:
      pos.addNewlines(m_count, 2);
      break;
   default:
      polar_unreachable("unknown kind");
   }
}

bool TriviaPiece::trySquash(const TriviaPiece &next)
{
   if (m_kind != next.m_kind) {
      return false;
   }
   switch (m_kind) {
   case TriviaKind::Space:
   case TriviaKind::Tab:
   case TriviaKind::VerticalTab:
   case TriviaKind::Formfeed:
   case TriviaKind::Newline:
   case TriviaKind::CarriageReturn:
   case TriviaKind::Backtick:
   case TriviaKind::LineComment:
   case TriviaKind::BlockComment:
   case TriviaKind::DocLineComment:
   case TriviaKind::DocBlockComment:
   case TriviaKind::GarbageText:
      return false;
   case TriviaKind::CarriageReturnLineFeed:
      m_count += next.m_count;
      return true;
   default:
      polar_unreachable("unknown kind");
   }
}

void TriviaPiece::print(RawOutStream &outStream) const
{
   switch (m_kind) {
   case TriviaKind::Space:
   case TriviaKind::Tab:
   case TriviaKind::VerticalTab:
   case TriviaKind::Formfeed:
   case TriviaKind::Newline:
   case TriviaKind::CarriageReturn:
   case TriviaKind::Backtick:
   case TriviaKind::LineComment:
   case TriviaKind::BlockComment:
   case TriviaKind::DocLineComment:
   case TriviaKind::DocBlockComment:
   case TriviaKind::GarbageText: {
      outStream << m_text.getStr();
      break;
   }
   case TriviaKind::CarriageReturnLineFeed: {
      StringRef chars = retrieve_trivia_kind_characters(m_kind);
      for (uint i = 0; i < m_count; ++i) {
         outStream << chars;
      }
      break;
   }
   default:
      polar_unreachable("unknown kind");
   }
}

#pragma mark - Trivia collection

void Trivia::appendOrSquash(const TriviaPiece &next)
{
   if (pieces.size() > 0) {
      TriviaPiece &last = pieces.back();
      if (last.trySquash(next)) {
         return;
      }
   }
   push_back(next);
}

Trivia Trivia::appending(const Trivia &other) const
{
   auto newPieces = pieces;
   std::copy(other.begin(), other.end(), std::back_inserter(newPieces));
   return { newPieces };
}

void Trivia::dump(RawOutStream &outStream, unsigned indent) const
{
   for (const auto &piece : pieces) {
      piece.dump(outStream, indent);
   }
}

void Trivia::dump() const
{
   dump(polar::utils::error_stream());
}

void Trivia::print(RawOutStream &outStream) const
{
   for (const auto &piece : pieces) {
      piece.print(outStream);
   }
}

TriviaList::const_iterator Trivia::find(const TriviaKind desiredKind) const
{
   return std::find_if(pieces.begin(), pieces.end(),
                       [=](const TriviaPiece &piece) -> bool {
      return piece.getKind() == desiredKind;
   });
}

Trivia Trivia::operator+(const Trivia &other) const
{
   auto newPieces = pieces;
   std::copy(other.pieces.begin(), other.pieces.end(),
             std::back_inserter(newPieces));
   return { newPieces };
}

} // polar::syntax
