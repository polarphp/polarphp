//===--- ParsedRawSyntaxRecorder.h - Raw Syntax Parsing Recorder ----------===//
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
//
// This file defines the ParsedRawSyntaxRecorder, which is the interface the
// parser is using to pass parsed syntactic elements to a SyntaxParseActions
// receiver and get a ParsedRawSyntaxNode object back.
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
// Created by polarboy on 2019/06/15.

#ifndef POLARPHP_PARSER_PARSED_RAW_SYNTAX_RECORDEDR_H
#define POLARPHP_PARSER_PARSED_RAW_SYNTAX_RECORDEDR_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/syntax/internal/TokenEnumDefs.h"
#include <memory>

namespace polar::syntax {
enum class SyntaxKind : std::uint32_t;
} // polar::syntax

namespace polar::parser {

class CharSourceRange;
class ParsedRawSyntaxNode;
struct ParsedTrivia;
class ParsedTriviaPiece;
class SyntaxParseActions;
class SourceLoc;
class Token;

using polar::syntax::SyntaxKind;
using polar::syntax::internal::TokenKindType;
using polar::basic::ArrayRef;

class ParsedRawSyntaxRecorder {
public:
   explicit ParsedRawSyntaxRecorder(std::shared_ptr<SyntaxParseActions> actions)
      : m_actions(std::move(actions))
   {}

   ParsedRawSyntaxNode recordToken(const Token &token,
                                   const ParsedTrivia &leadingTrivia,
                                   const ParsedTrivia &trailingTrivia);

   ParsedRawSyntaxNode recordToken(TokenKindType tokenKind, CharSourceRange tokenRange,
                                   ArrayRef<ParsedTriviaPiece> leadingTrivia,
                                   ArrayRef<ParsedTriviaPiece> trailingTrivia);

   /// Record a missing token. \p loc can be invalid or an approximate location
   /// of where the token would be if not missing.
   ParsedRawSyntaxNode recordMissingToken(TokenKindType tokenKind, SourceLoc loc);

   /// The provided \p elements are an exact layout appropriate for the syntax
   /// \p kind. Missing optional elements are represented with a null
   /// ParsedRawSyntaxNode object.
   ParsedRawSyntaxNode recordRawSyntax(syntax::SyntaxKind kind,
                                       ArrayRef<ParsedRawSyntaxNode> elements);

   /// Record a raw syntax collecton without eny elements. \p loc can be invalid
   /// or an approximate location of where an element of the collection would be
   /// if not missing.
   ParsedRawSyntaxNode recordEmptyRawSyntaxCollection(syntax::SyntaxKind kind,
                                                      SourceLoc loc);

   /// Used for incremental re-parsing.
   ParsedRawSyntaxNode lookupNode(size_t lexerOffset, SourceLoc loc,
                                  syntax::SyntaxKind kind);
private:
   std::shared_ptr<SyntaxParseActions> m_actions;
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_RAW_SYNTAX_RECORDEDR_H
