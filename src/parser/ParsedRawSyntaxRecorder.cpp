//===--- ParsedRawSyntaxRecorder.cpp - Raw Syntax Parsing Recorder --------===//
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
// Created by polarboy on 2019/06/16.
//===----------------------------------------------------------------------===//
// This file defines the ParsedRawSyntaxRecorder, which is the interface the
// parser is using to pass parsed syntactic elements to a SyntaxParseActions
// receiver and get a ParsedRawSyntaxNode object back.
//
//===----------------------------------------------------------------------===//

#include "polarphp/parser/ParsedRawSyntaxRecorder.h"
#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/parser/ParsedTrivia.h"
#include "polarphp/parser/SyntaxParseActions.h"
#include "polarphp/parser/Token.h"
#include "polarphp/syntax/SyntaxKind.h"

namespace polar::parser {

ParsedRawSyntaxNode
ParsedRawSyntaxRecorder::recordToken(const Token &token,
                                     const ParsedTrivia &leadingTrivia,
                                     const ParsedTrivia &trailingTrivia)
{
   return recordToken(token.getKind(), token.getRangeWithoutBackticks(),
                      leadingTrivia.pieces, trailingTrivia.pieces);
}

ParsedRawSyntaxNode
ParsedRawSyntaxRecorder::recordToken(TokenKindType tokenKind, CharSourceRange tokRange,
                                     ArrayRef<ParsedTriviaPiece> leadingTrivia,
                                     ArrayRef<ParsedTriviaPiece> trailingTrivia)
{
   size_t leadingTriviaLen = ParsedTriviaPiece::getTotalLength(leadingTrivia);
   size_t trailingTriviaLen = ParsedTriviaPiece::getTotalLength(trailingTrivia);
   SourceLoc offset = tokRange.getStart().getAdvancedLoc(-leadingTriviaLen);
   unsigned length = leadingTriviaLen + tokRange.getByteLength() +
         trailingTriviaLen;
   CharSourceRange range{offset, length};
   OpaqueSyntaxNode n = m_actions->recordToken(tokenKind, leadingTrivia,
                                               trailingTrivia, range);
   return ParsedRawSyntaxNode{SyntaxKind::Token, tokenKind, range, n};
}

ParsedRawSyntaxNode
ParsedRawSyntaxRecorder::recordMissingToken(TokenKindType tokenKind, SourceLoc loc)
{
   CharSourceRange range{loc, 0};
   OpaqueSyntaxNode n = m_actions->recordMissingToken(tokenKind, loc);
   return ParsedRawSyntaxNode{SyntaxKind::Token, tokenKind, range, n};
}

static ParsedRawSyntaxNode
getRecordedNode(const ParsedRawSyntaxNode &node, ParsedRawSyntaxRecorder &rec)
{
   if (node.isNull() || node.isRecorded()) {
      return node;
   }
   if (node.isDeferredLayout()) {
      return rec.recordRawSyntax(node.getKind(), node.getDeferredChildren());
   }
   assert(node.isDeferredToken());
   CharSourceRange tokRange = node.getDeferredTokenRangeWithoutBackticks();
   TokenKindType tokenKind = node.getTokenKind();
   if (node.isMissing()) {
      return rec.recordMissingToken(tokenKind, tokRange.getStart());
   }

   return rec.recordToken(tokenKind,tokRange,
                          node.getDeferredLeadingTriviaPieces(),
                          node.getDeferredTrailingTriviaPieces());
}

ParsedRawSyntaxNode
ParsedRawSyntaxRecorder::recordRawSyntax(SyntaxKind kind,
                                         ArrayRef<ParsedRawSyntaxNode> elements)
{
   CharSourceRange range;
   SmallVector<OpaqueSyntaxNode, 16> subnodes;
   if (!elements.empty()) {
      SourceLoc offset;
      unsigned length = 0;
      for (const auto &elem : elements) {
         auto subnode = getRecordedNode(elem, *this);
         if (subnode.isNull()) {
            subnodes.push_back(nullptr);
         } else {
            subnodes.push_back(subnode.getOpaqueNode());
            auto range = subnode.getRange();
            if (range.isValid()) {
               if (offset.isInvalid()) {
                  offset = range.getStart();
               }
               length += subnode.getRange().getByteLength();
            }
         }
      }
      range = CharSourceRange{offset, length};
   }
   OpaqueSyntaxNode n = m_actions->recordRawSyntax(kind, subnodes, range);
   return ParsedRawSyntaxNode{kind, TokenKindType::T_UNKNOWN_MARK, range, n};
}

ParsedRawSyntaxNode
ParsedRawSyntaxRecorder::recordEmptyRawSyntaxCollection(SyntaxKind kind,
                                                        SourceLoc loc)
{
   CharSourceRange range{loc, 0};
   OpaqueSyntaxNode n = m_actions->recordRawSyntax(kind, {}, range);
   return ParsedRawSyntaxNode{kind, TokenKindType::T_UNKNOWN_MARK, range, n};
}

ParsedRawSyntaxNode
ParsedRawSyntaxRecorder::lookupNode(size_t lexerOffset, SourceLoc loc,
                                    SyntaxKind kind)
{
   size_t length;
   OpaqueSyntaxNode n;
   std::tie(length, n) = m_actions->lookupNode(lexerOffset, kind);
   if (length == 0) {
      return ParsedRawSyntaxNode::null();
   }
   CharSourceRange range{loc, unsigned(length)};
   return ParsedRawSyntaxNode{kind, TokenKindType::T_UNKNOWN_MARK, range, n};
}

} // polar::parser
