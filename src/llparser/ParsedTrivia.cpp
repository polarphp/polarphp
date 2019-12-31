//===--- ParsedTrivia.cpp - ParsedTrivia API ----------------------------*-===//
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

#include "polarphp/llparser/ParsedTrivia.h"

namespace polar::llparser {

bool isCommentTriviaKind(TriviaKind kind) {
   switch (kind) {
      case TriviaKind::Space:
      case TriviaKind::Tab:
      case TriviaKind::VerticalTab:
      case TriviaKind::Formfeed:
      case TriviaKind::Newline:
      case TriviaKind::CarriageReturn:
      case TriviaKind::CarriageReturnLineFeed:
      case TriviaKind::GarbageText:
         return false;
      case TriviaKind::LineComment:
      case TriviaKind::BlockComment:
      case TriviaKind::DocLineComment:
      case TriviaKind::DocBlockComment:
         return true;
   }
   llvm_unreachable("unknown kind");
}

} // polar::llparser
