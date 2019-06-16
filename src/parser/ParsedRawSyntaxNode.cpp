//===--- ParsedRawSyntaxNode.cpp - Parsed Raw Syntax Node -----------------===//
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

#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/parser/SyntaxParsingContext.h"

namespace polar::parser {

using polar::basic::make_array_ref;
using polar::syntax::dump_syntax_kind;
using polar::syntax::dump_token_kind;

ParsedRawSyntaxNode
ParsedRawSyntaxNode::makeDeferred(SyntaxKind k,
                                  ArrayRef<ParsedRawSyntaxNode> deferredNodes,
                                  SyntaxParsingContext &ctx)
{
   if (deferredNodes.empty()) {
      return ParsedRawSyntaxNode(k, {});
   }
   ParsedRawSyntaxNode *newPtr =
         ctx.getScratchAlloc().allocate<ParsedRawSyntaxNode>(deferredNodes.size());
   std::uninitialized_copy(deferredNodes.begin(), deferredNodes.end(), newPtr);
   return ParsedRawSyntaxNode(k, make_array_ref(newPtr, deferredNodes.size()));
}

ParsedRawSyntaxNode
ParsedRawSyntaxNode::makeDeferred(Token tok,
                                  const ParsedTrivia &leadingTrivia,
                                  const ParsedTrivia &trailingTrivia,
                                  SyntaxParsingContext &ctx)
{
   CharSourceRange tokRange = tok.getRangeWithoutBackticks();
   size_t piecesCount = leadingTrivia.size() + trailingTrivia.size();
   ParsedTriviaPiece *piecesPtr = nullptr;
   if (piecesCount > 0) {
      piecesPtr = ctx.getScratchAlloc().allocate<ParsedTriviaPiece>(piecesCount);
      std::uninitialized_copy(leadingTrivia.begin(), leadingTrivia.end(),
                              piecesPtr);
      std::uninitialized_copy(trailingTrivia.begin(), trailingTrivia.end(),
                              piecesPtr + leadingTrivia.size());
   }
   return ParsedRawSyntaxNode(tok.getKind(), tokRange.getStart(),
                              tokRange.getByteLength(), piecesPtr,
                              leadingTrivia.size(), trailingTrivia.size());
}

void ParsedRawSyntaxNode::dump() const
{
   dump(polar::utils::error_stream(), /*indent*/ 0);
   polar::utils::error_stream() << '\n';
}

void ParsedRawSyntaxNode::dump(RawOutStream &outStream, unsigned indent) const
{
   auto indentFunc = [&](unsigned amount) {
      for (decltype(amount) i = 0; i < amount; ++i) {
         outStream << ' ';
      }
   };

   indentFunc(indent);

   if (isNull()) {
      outStream << "(<NULL>)";
      return;
   }

   outStream << '(';
   dump_syntax_kind(outStream, getKind());

   if (isToken()) {
      dump_token_kind(outStream, getTokenKind());

   } else {
      if (isRecorded()) {
         outStream << " [recorded]";
      } else if (isDeferredLayout()) {
         for (const auto &child : getDeferredChildren()) {
            outStream << "\n";
            child.dump(outStream, indent + 1);
         }
      } else {
         assert(isDeferredToken());
         outStream << " [deferred token]";
      }
   }
   outStream << ')';
}

} // polar::parser
