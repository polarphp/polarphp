//===----------- SyntaxParsingCache.h -================----------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PARSER_SYNTAX_PARSING_CACHE_H
#define POLARPHP_PARSER_SYNTAX_PARSING_CACHE_H

#include "polarphp/syntax/SyntaxNodes.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include <unordered_set>

namespace polar::parser {

using polar::syntax::AbsolutePosition;
using polar::syntax::Syntax;
using polar::syntax::SyntaxKind;
using polar::syntax::SourceFileSyntax;
using polar::syntax::SyntaxNodeId;
using polar::basic::SmallVector;
using polar::basic::ArrayRef;

/// A single edit to the original source file in which a continuous range of
/// characters have been replaced by a new string
struct SourceEdit
{
   /// The byte offset from which on characters were replaced.
   size_t start;

   /// The byte offset to which on characters were replaced.
   size_t end;

   /// The length of the string that replaced the range described above.
   size_t replacementLength;

   SourceEdit(size_t start, size_t end, size_t replacementLength)
      : start(start),
        end(end),
        replacementLength(replacementLength)
   {}

   /// The length of the range that has been replaced
   size_t originalLength() const
   {
      return end - start;
   }

   /// Check if the characters replaced by this edit fall into the given range
   /// or are directly adjacent to it
   bool intersectsOrTouchesRange(size_t rangeStart, size_t rangeEnd)
   {
      return end >= rangeStart && start <= rangeEnd;
   }
};

struct SyntaxReuseRegion
{
   AbsolutePosition start;
   AbsolutePosition end;
};

class SyntaxParsingCache
{
public:
   SyntaxParsingCache(SourceFileSyntax oldSyntaxTree)
      : m_oldSyntaxTree(oldSyntaxTree)
   {}

   /// Add an edit that transformed the source file which created this cache into
   /// the source file that is now being parsed incrementally. \c start must be a
   /// position from the *original* source file, and it must not overlap any
   /// other edits previously added. For instance, given:
   ///   (aaa, bbb)
   ///   0123456789
   /// When you want to turn this into:
   ///   (c, dddd)
   ///   0123456789
   /// edits should be: { 1, 4, 1 } and { 6, 9, 4 }.
   void addEdit(size_t start, size_t end, size_t replacementLength);

   /// Check if a syntax node of the given kind at the given position can be
   /// reused for a new syntax tree.
   std::optional<Syntax> lookUp(size_t newPosition, SyntaxKind kind);

   const std::unordered_set<SyntaxNodeId> &getReusedNodeIds() const
   {
      return m_reusedNodeIds;
   }

   /// Get the source regions of the new source file, represented by
   /// \p syntaxTree that have been reused as part of the incremental parse.
   std::vector<SyntaxReuseRegion>
   getReusedRegions(const SourceFileSyntax &syntaxTree) const;

   /// Translates a post-edit position to a pre-edit position by undoing the
   /// specified edits. Returns \c None if no pre-edit position exists because
   /// the post-edit position has been inserted by an edit.
   ///
   /// Should not be invoked externally. Only public for testing purposes.
   static std::optional<size_t>
   translateToPreEditPosition(size_t postEditPosition,
                              ArrayRef<SourceEdit> edits);

private:
   std::optional<Syntax> lookUpFrom(const Syntax &node, size_t nodeStart,
                                    size_t position, SyntaxKind kind);

   bool nodeCanBeReused(const Syntax &node, size_t position, size_t nodeStart,
                        SyntaxKind kind) const;
private:
   /// The syntax tree prior to the edit
   SourceFileSyntax m_oldSyntaxTree;

   /// The edits that were made from the source file that created this cache to
   /// the source file that is now parsed incrementally
   SmallVector<SourceEdit, 4> m_edits;

   /// The IDs of all syntax nodes that got reused are collected in this vector.
   std::unordered_set<SyntaxNodeId> m_reusedNodeIds;
};

} // polar::parser

#endif // POLARPHP_PARSER_SYNTAX_PARSING_CACHE_H
