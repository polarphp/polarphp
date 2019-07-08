//===--- ParsedRawSyntaxNode.h - Parsed Raw Syntax Node ---------*- C++ -*-===//
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
// Created by polarboy on 2019/06/15.

#ifndef POLARPHP_PARSER_PARSED_RAW_SYNTAX_NODE_H
#define POLARPHP_PARSER_PARSED_RAW_SYNTAX_NODE_H

#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/ParsedTrivia.h"
#include "polarphp/parser/Token.h"
#include "polarphp/syntax/SyntaxKind.h"
#include <vector>

namespace polar::parser {

typedef void *OpaqueSyntaxNode;
class SyntaxParsingContext;
using polar::syntax::SyntaxKind;

/// Represents a raw syntax node formed by the parser.
///
/// It can be either 'recorded', in which case it encapsulates an
/// \c OpaqueSyntaxNode that was returned from a \c SyntaxParseActions
/// invocation, or 'deferred' which captures the data for a
/// \c SyntaxParseActions invocation to occur later.
///
/// An \c OpaqueSyntaxNode can represent both the result of 'recording' a token
/// as well as 'recording' a syntax layout, so there's only one
/// \c RecordedSyntaxNode structure that can represent both.
///
/// The 'deferred' form is used for when the parser is backtracking and when
/// there are instances that it's not clear what will be the final syntax node
/// in the current parsing context.
///
class ParsedRawSyntaxNode
{
   enum class DataKind: uint8_t
   {
      Null,
      Recorded,
      deferredLayout,
      deferredToken,
   };

   struct RecordedSyntaxNode
   {
      OpaqueSyntaxNode opaqueNode;
      CharSourceRange range;
   };

   struct DeferredLayoutNode
   {
      ArrayRef<ParsedRawSyntaxNode> children;
   };

   struct DeferredTokenNode
   {
      const ParsedTriviaPiece *triviaPieces;
      SourceLoc tokenLoc;
      unsigned tokenLength;
      uint16_t numLeadingTrivia;
      uint16_t numTrailingTrivia;
   };

   union
   {
      RecordedSyntaxNode recordedData;
      DeferredLayoutNode deferredLayout;
      DeferredTokenNode deferredToken;
   };
   uint16_t m_syntaxKind;
   uint16_t m_tokenKind;
   DataKind m_dataKind;
   /// Primary used for capturing a deferred missing token.
   bool m_isMissing = false;

   ParsedRawSyntaxNode(syntax::SyntaxKind k,
                       ArrayRef<ParsedRawSyntaxNode> deferredNodes)
      : deferredLayout{deferredNodes},
        m_syntaxKind(uint16_t(k)),
        m_tokenKind(uint16_t(TokenKindType::T_UNKNOWN_MARK)),
        m_dataKind(DataKind::deferredLayout)
   {
      assert(getKind() == k && "Syntax kind with too large value!");
   }

   ParsedRawSyntaxNode(TokenKindType tokenKind, SourceLoc tokenLoc, unsigned tokenLength,
                       const ParsedTriviaPiece *triviaPieces,
                       unsigned numLeadingTrivia,
                       unsigned numTrailingTrivia)
      : deferredToken{triviaPieces,
                      tokenLoc, tokenLength,
                      uint16_t(numLeadingTrivia),
                      uint16_t(numTrailingTrivia)},
        m_syntaxKind(uint16_t(SyntaxKind::Token)),
        m_tokenKind(uint16_t(tokenKind)),
        m_dataKind(DataKind::deferredToken)
   {
      assert(getTokenKind() == tokenKind && "Token kind is too large value!");
      assert(deferredToken.numLeadingTrivia == numLeadingTrivia &&
             "numLeadingTrivia is too large value!");
      assert(deferredToken.numTrailingTrivia == numTrailingTrivia &&
             "numLeadingTrivia is too large value!");
   }

public:
   ParsedRawSyntaxNode()
      : recordedData{},
        m_syntaxKind(uint16_t(syntax::SyntaxKind::Unknown)),
        m_tokenKind(uint16_t(TokenKindType::T_UNKNOWN_MARK)),
        m_dataKind(DataKind::Null)
   {
   }

   ParsedRawSyntaxNode(syntax::SyntaxKind k, TokenKindType tokenKind,
                       CharSourceRange r, OpaqueSyntaxNode n)
      : recordedData{n, r},
        m_syntaxKind(uint16_t(k)),
        m_tokenKind(uint16_t(tokenKind)),
        m_dataKind(DataKind::Recorded)
   {
      assert(getKind() == k && "Syntax kind with too large value!");
      assert(getTokenKind() == tokenKind && "Token kind with too large value!");
   }

   syntax::SyntaxKind getKind() const
   {
      return syntax::SyntaxKind(m_syntaxKind);
   }

   TokenKindType getTokenKind() const
   {
      return TokenKindType(m_tokenKind);
   }

   bool isToken() const
   {
      return getKind() == syntax::SyntaxKind::Token;
   }

   bool isToken(TokenKindType tokenKind) const
   {
      return getTokenKind() == tokenKind;
   }

   bool isNull() const
   {
      return m_dataKind == DataKind::Null;
   }

   bool isRecorded() const
   {
      return m_dataKind == DataKind::Recorded;
   }

   bool isDeferredLayout() const
   {
      return m_dataKind == DataKind::deferredLayout;
   }

   bool isDeferredToken() const
   {
      return m_dataKind == DataKind::deferredToken;
   }

   /// Primary used for a deferred missing token.
   bool isMissing() const
   {
      return m_isMissing;
   }

   // Recorded Data ===========================================================//

   CharSourceRange getRange() const
   {
      assert(isRecorded());
      return recordedData.range;
   }

   OpaqueSyntaxNode getOpaqueNode() const
   {
      assert(isRecorded());
      return recordedData.opaqueNode;
   }

   // Deferred Layout Data ====================================================//

   ArrayRef<ParsedRawSyntaxNode> getDeferredChildren() const
   {
      assert(m_dataKind == DataKind::deferredLayout);
      return deferredLayout.children;
   }

   // Deferred Token Data =====================================================//

   CharSourceRange getDeferredTokenRangeWithoutBackticks() const
   {
      assert(m_dataKind == DataKind::deferredToken);
      return CharSourceRange{deferredToken.tokenLoc, deferredToken.tokenLength};
   }

   ArrayRef<ParsedTriviaPiece> getDeferredLeadingTriviaPieces() const
   {
      assert(m_dataKind == DataKind::deferredToken);
      return ArrayRef<ParsedTriviaPiece>(deferredToken.triviaPieces,
                                         deferredToken.numLeadingTrivia);
   }

   ArrayRef<ParsedTriviaPiece> getDeferredTrailingTriviaPieces() const
   {
      assert(m_dataKind == DataKind::deferredToken);
      return ArrayRef<ParsedTriviaPiece>(
               deferredToken.triviaPieces + deferredToken.numLeadingTrivia,
               deferredToken.numTrailingTrivia);
   }

   //==========================================================================//

   /// Form a deferred syntax layout node.
   static ParsedRawSyntaxNode makeDeferred(SyntaxKind k,
                                           ArrayRef<ParsedRawSyntaxNode> deferredNodes,
                                           SyntaxParsingContext &ctx);

   /// Form a deferred token node.
   static ParsedRawSyntaxNode makeDeferred(Token token,
                                           const ParsedTrivia &leadingTrivia,
                                           const ParsedTrivia &trailingTrivia,
                                           SyntaxParsingContext &ctx);

   /// Form a deferred missing token node.
   static ParsedRawSyntaxNode makeDeferredMissing(TokenKindType tokenKind, SourceLoc loc)
   {
      auto raw = ParsedRawSyntaxNode(tokenKind, loc, 0, nullptr, 0, 0);
      raw.m_isMissing = true;
      return raw;
   }

   /// Dump this piece of syntax recursively for debugging or testing.
   POLAR_ATTRIBUTE_DEPRECATED(
         void dump() const POLAR_ATTRIBUTE_USED,
         "only for use within the debugger");

   /// Dump this piece of syntax recursively.
   void dump(RawOutStream &outStream, unsigned indent = 0) const;

   static ParsedRawSyntaxNode null()
   {
      return ParsedRawSyntaxNode{};
   }
};

} // polar::parser

#endif // POLARPHP_PARSER_PARSED_RAW_SYNTAX_NODE_H
