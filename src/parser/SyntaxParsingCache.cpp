//===--- SyntaxParsingCache.cpp - Incremental syntax parsing lookup--------===//
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

#include "polarphp/parser/SyntaxParsingCache.h"
#include "polarphp/syntax/SyntaxNodeVisitor.h"

namespace polar::parser {

using polar::syntax::SyntaxNodeVisitor;

void SyntaxParsingCache::addEdit(size_t start, size_t end,
                                 size_t replacementLength)
{
   assert((m_edits.empty() || m_edits.back().end <= start) &&
          "'start' must be greater than or equal to 'end' of the previous edit");
   m_edits.emplace_back(start, end, replacementLength);
}

bool SyntaxParsingCache::nodeCanBeReused(const Syntax &node, size_t nodeStart,
                                         size_t position,
                                         SyntaxKind kind) const
{
   // Computing the value of nodeStart on the fly is faster than determining a
   // node's absolute position, but make sure the values match in an assertion
   // build
   assert(nodeStart == node.getAbsolutePositionBeforeLeadingTrivia().getOffset());

   if (nodeStart != position) {
      return false;
   }

   if (node.getKind() != kind) {
      return false;
   }

   // node can also not be reused if an edit has been made in the next token's
   // text, e.g. because `private struct Foo {}` parses as a CodeBlockItem with a
   // StructDecl inside and `private struc Foo {}` parses as two CodeBlockItems
   // one for `private` and one for `struc Foo {}`
   size_t nextLeafNodeLength = 0;
   if (auto nextNode = node.getData().getNextNode()) {
      auto nextLeafNode = nextNode->getFirstToken();
      auto nextRawNode = nextLeafNode->getRaw();
      assert(nextRawNode->isPresent());
      nextLeafNodeLength += nextRawNode->getTokenText().size();
      for (auto triviaPiece : nextRawNode->getLeadingTrivia()) {
         nextLeafNodeLength += triviaPiece.getTextLength();
      }
   }

   auto nodeEnd = nodeStart + node.getTextLength();
   for (auto edit : m_edits) {
      // Check if this node or the trivia of the next node has been edited. If it
      // has, we cannot reuse it.
      if (edit.intersectsOrTouchesRange(nodeStart, nodeEnd + nextLeafNodeLength)) {
         return false;
      }
   }
   return true;
}

std::optional<Syntax> SyntaxParsingCache::lookUpFrom(const Syntax &node,
                                                     size_t nodeStart,
                                                     size_t position,
                                                     SyntaxKind kind)
{
   if (nodeCanBeReused(node, nodeStart, position, kind)) {
      return node;
   }

   // Compute the child's position on the fly
   size_t childStart = nodeStart;
   for (size_t I = 0, E = node.getNumChildren(); I < E; ++I) {
      std::optional<Syntax> child = node.getChild(I);
      if (!child.has_value() || child->isMissing()) {
         continue;
      }
      auto childEnd = childStart + child->getTextLength();
      if (childStart <= position && position < childEnd) {
         return lookUpFrom(child.value(), childStart, position, kind);
      }
      // The next child starts where the previous child ended
      childStart = childEnd;
   }
   return std::nullopt;
}

std::optional<size_t>
SyntaxParsingCache::translateToPreEditPosition(size_t postEditPosition,
                                               ArrayRef<SourceEdit> edits) {
   size_t position = postEditPosition;
   for (auto &edit : edits) {
      if (edit.start > position) {
         // Remaining edits doesn't affect the position. (m_edits are sorted)
         break;
      }

      if (edit.start + edit.replacementLength > position) {
         // This is a position inserted by the edit, and thus doesn't exist in the
         // pre-edit version of the file.
         return std::nullopt;
      }
      position = position - edit.replacementLength + edit.originalLength();
   }
   return position;
}

std::optional<Syntax> SyntaxParsingCache::lookUp(size_t newPosition,
                                                 SyntaxKind kind)
{
   std::optional<size_t> oldPosition = translateToPreEditPosition(newPosition, m_edits);
   if (!oldPosition.has_value()) {
      return std::nullopt;
   }
   auto node = lookUpFrom(m_oldSyntaxTree, /*nodeStart=*/0, *oldPosition, kind);
   if (node.has_value()) {
      m_reusedNodeIds.insert(node->getId());
   }
   return node;
}

std::vector<SyntaxReuseRegion>
SyntaxParsingCache::getReusedRegions(const SourceFileSyntax &SyntaxTree) const
{
   /// Determines the reused source regions from reused syntax node IDs
   class ReusedRegionsCollector : public SyntaxNodeVisitor
   {
      std::unordered_set<SyntaxNodeId> m_reusedNodeIds;
      std::vector<SyntaxReuseRegion> m_reusedRegions;

      bool didReuseNode(SyntaxNodeId nodeId)
      {
         return m_reusedNodeIds.count(nodeId) > 0;
      }

   public:
      ReusedRegionsCollector(std::unordered_set<SyntaxNodeId> reusedNodeIds)
         : m_reusedNodeIds(reusedNodeIds)
      {}

      const std::vector<SyntaxReuseRegion> &getReusedRegions()
      {
         std::sort(m_reusedRegions.begin(), m_reusedRegions.end(),
                   [](const SyntaxReuseRegion &lhs,
                   const SyntaxReuseRegion &rhs) -> bool
         {
            return lhs.start.getOffset() < rhs.start.getOffset();
         });
         return m_reusedRegions;
      }

      void visit(Syntax node) override
      {
         if (didReuseNode(node.getId())) {
            // node has been reused, add it to the list
            auto start = node.getAbsolutePositionBeforeLeadingTrivia();
            auto end = node.getAbsoluteEndPositionAfterTrailingTrivia();
            m_reusedRegions.push_back({start, end});
         } else {
            SyntaxNodeVisitor::visit(node);
         }
      }

      void collectReusedRegions(SourceFileSyntax node)
      {
         assert(m_reusedRegions.empty() &&
                "ReusedRegionsCollector cannot be reused");
         node.accept(*this);
      }
   };

   ReusedRegionsCollector reuseRegionsCollector(getReusedNodeIds());
   reuseRegionsCollector.collectReusedRegions(SyntaxTree);
   return reuseRegionsCollector.getReusedRegions();
}


} // polar::parser
