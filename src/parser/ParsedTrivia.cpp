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

#include "polarphp/parser/ParsedTrivia.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/basic/SourceMgr.h"

namespace polar::parser {

using polar::syntax::TriviaPiece;
using polar::CharSourceRange;
using llvm::StringRef;

Trivia
ParsedTriviaPiece::convertToSyntaxTrivia(ArrayRef<ParsedTriviaPiece> pieces,
                                         SourceLoc loc,
                                         const SourceManager &sourceMgr,
                                         unsigned bufferID)
{
   Trivia trivia;
   SourceLoc curLoc = loc;
   for (const auto &piece : pieces) {
      CharSourceRange range{curLoc, piece.getLength()};
      StringRef text = sourceMgr.extractText(range, bufferID);
      trivia.push_back(TriviaPiece::fromText(piece.getKind(), text));
      curLoc = curLoc.getAdvancedLoc(piece.getLength());
   }
   return trivia;
}

Trivia
ParsedTrivia::convertToSyntaxTrivia(SourceLoc loc, const SourceManager &sourceMgr,
                                    unsigned bufferID) const
{
   return ParsedTriviaPiece::convertToSyntaxTrivia(pieces, loc, sourceMgr, bufferID);
}

} // polar::parser
