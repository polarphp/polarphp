//===--- SyntaxParseActions.h - Syntax Parsing Actions ----------*- C++ -*-===//
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
//  This file defines the interface between the parser and a receiver of
//  raw syntax nodes.
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

#ifndef POLARPHP_PARSER_SYNTAX_PARSE_ACTIONS_H
#define POLARPHP_PARSER_SYNTAX_PARSE_ACTIONS_H

#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/syntax/internal/TokenEnumDefs.h"

namespace polar::syntax {
enum class SyntaxKind : std::uint32_t;
} // polar::syntax

namespace polar::parser {

class CharSourceRange;
class ParsedTriviaPiece;
class SourceLoc;
using polar::basic::ArrayRef;
using polar::syntax::internal::TokenKindType;
using polar::syntax::SyntaxKind;
using OpaqueSyntaxNode = void *;

class SyntaxParseActions
{
public:
   virtual ~SyntaxParseActions() = default;

   virtual OpaqueSyntaxNode recordToken(TokenKindType tokenKind,
                                        ArrayRef<ParsedTriviaPiece> leadingTrivia,
                                        ArrayRef<ParsedTriviaPiece> trailingTrivia,
                                        CharSourceRange range) = 0;

   /// Record a missing token. \c loc can be invalid or an approximate location
   /// of where the token would be if not missing.
   virtual OpaqueSyntaxNode recordMissingToken(TokenKindType tokenKind, SourceLoc loc) = 0;

   /// The provided \c elements are an exact layout appropriate for the syntax
   /// \c kind. Missing optional elements are represented with a null
   /// OpaqueSyntaxNode object.
   virtual OpaqueSyntaxNode recordRawSyntax(SyntaxKind kind,
                                            ArrayRef<OpaqueSyntaxNode> elements,
                                            CharSourceRange range) = 0;

   /// Used for incremental re-parsing.
   virtual std::pair<size_t, OpaqueSyntaxNode>
   lookupNode(size_t lexerOffset, SyntaxKind kind)
   {
      return std::make_pair(0, nullptr);
   }

private:
   virtual void anchor();
};

} // polar::parser

#endif // POLARPHP_PARSER_SYNTAX_PARSE_ACTIONS_H
