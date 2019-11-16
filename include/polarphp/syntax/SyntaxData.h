//===--- SyntaxData.h - Swift Syntax Data Interface -------------*- C++ -*-===//
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
//
// This file defines the SyntaxData interface, the type for the instance
// data for Syntax nodes.
//
// Effectively, these provide two main things to a Syntax node - parental
// relationships and caching for its children.
//
// A SyntaxData contains at least a strong reference to the RawSyntax,
// from which most information comes, and additionally a weak reference to
// its m_parent and the "index" at which it occurs in its m_parent. These were
// originally intended to have the important public APIs for structured
// editing but now contain no significant or public API; for those, see the
// Syntax type. These are purely to contain data, hence the name.
//
// Conceptually, SyntaxData add the characteristic of specific identity in a
// piece of php source code. While the RawSyntax for the integer literal
// token '1' can be reused anywhere a '1' occurs and has identical formatting,
// a SyntaxData represents *a* specific '1' at a particular location in
// Swift source.
//
// These are effectively internal implementation. For all public APIs, look
// for the type without "Data" in its name. For example, a StructDeclSyntaxData
// expresses its API through the wrapping StructDeclSyntax type.
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
// Created by polarboy on 2019/05/08.

#ifndef POLARPHP_SYNTAX_SYNTAXDATA_H
#define POLARPHP_SYNTAX_SYNTAXDATA_H

#include "polarphp/syntax/AtomicCache.h"
#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/References.h"
#include "llvm/ADT/DenseMap.h"

#include <atomic>

namespace polar::syntax {

using polar::basic::ThreadSafeRefCountedBase;
using polar::basic::TrailingObjects;

/// The class for holding parented syntax.
///
/// This structure should not contain significant public
/// API or internal modification API.
///
/// This is only for holding a strong reference to the RawSyntax, a weak
/// reference to the m_parent, and, in subclasses, lazily created strong
/// references to non-terminal child nodes.
class SyntaxData final
      : public ThreadSafeRefCountedBase<SyntaxData>,
      private TrailingObjects<SyntaxData, AtomicCache<SyntaxData>>
{
public:
   /// Get the node immediately before this current node that does contain a
   /// non-missing token. Return nullptr if we cannot find such node.
   RefCountPtr<SyntaxData> getPreviousNode() const;

   /// Get the node immediately after this current node that does contain a
   /// non-missing token. Return nullptr if we cannot find such node.
   RefCountPtr<SyntaxData> getNextNode() const;

   /// Get the first non-missing token node in this tree. Return nullptr if this
   /// node does not contain non-missing tokens.
   RefCountPtr<SyntaxData> getFirstToken() const;

   ~SyntaxData()
   {
      for (auto &iter : getChildren()) {
         iter.~AtomicCache<SyntaxData>();
      }
   }

   /// Constructs a SyntaxNode by replacing `self` and recursively building
   /// the m_parent chain up to the root.
   template <typename SyntaxNode>
   SyntaxNode replaceSelf(const RefCountPtr<RawSyntax> newRaw) const
   {
      auto newRootAndData = replaceSelf(newRaw);
      return { newRootAndData.first, newRootAndData.second.get() };
   }

   /// Replace a child in the m_raw syntax and recursively rebuild the
   /// parental chain up to the root.
   ///
   /// DO NOT expose this as public API.
   template <typename SyntaxNode, typename CursorType>
   SyntaxNode replaceChild(const RefCountPtr<RawSyntax> rawChild,
                           CursorType childCursor) const
   {
      auto  newRootAndParent = replaceChild(rawChild, childCursor);
      return SyntaxNode {
         newRootAndParent.first,
               newRootAndParent.second.get()
      };
   }

   static RefCountPtr<SyntaxData> make(RefCountPtr<RawSyntax> m_raw,
                                       const SyntaxData *m_parent = nullptr,
                                       CursorIndex m_indexInParent = 0);

   /// Returns the m_raw syntax node for this syntax node.
   const RefCountPtr<RawSyntax> getRaw() const
   {
      return m_raw;
   }

   /// Returns the kind of syntax node this is.
   SyntaxKind getKind() const
   {
      return m_raw->getKind();
   }

   /// Return the m_parent syntax if there is one.
   const SyntaxData * getParent() const
   {
      return m_parent;
   }

   /// Returns true if this syntax node has a m_parent.
   bool hasParent() const
   {
      return m_parent != nullptr;
   }

   /// Returns the child index of this node in its m_parent, if it has a m_parent,
   /// otherwise 0.
   size_t getIndexInParent() const
   {
      return m_indexInParent;
   }

   /// Returns the number of children this SyntaxData represents.
   size_t getNumChildren() const
   {
      return m_raw->getLayout().size();
   }

   /// Gets the child at the index specified by the provided cursor,
   /// lazily creating it if necessary.
   template <typename CursorType>
   RefCountPtr<SyntaxData> getChild(CursorType cursor) const
   {
      return getChild(static_cast<size_t>(cursor_index(cursor)));
   }

   /// Gets the child at the specified index in this data's children array.
   /// Why do we need this?
   /// - SyntaxData nodes should have pointer identity.
   /// - We only want to construct parented, realized child nodes as
   ///   SyntaxData when asked.
   ///
   /// For example, if we have a ReturnStmtSyntax, and ask for its returned
   /// expression for the first time with getExpression(), two nodes can race
   /// to create and set the cached expression.
   ///
   /// Looking at an example - say we have a SyntaxData.
   ///
   /// SyntaxData = {
   ///   RefCountPtr<RawSyntax> m_raw = {
   ///     RefCountPtr<RawTokenSyntax> { SyntaxKind::Token, tok::return_kw, "return" },
   ///     RefCountPtr<RawSyntax> { SyntaxKind::SomeExpression, ... }
   ///   }
   ///   llvm::SmallVector<AtomicCache<SyntaxData>, 10> Children {
   ///     AtomicCache<SyntaxData> { RefCountPtr<SyntaxData> = nullptr; },
   ///     AtomicCache<SyntaxData> { RefCountPtr<SyntaxData> = nullptr; },
   ///   }
   /// }
   ///
   /// If we wanted to safely create the 0th child, an instance of TokenSyntax,
   /// then we ask the AtomicCache in that position to realize its value and
   /// cache it. This is safe because AtomicCache only ever mutates its cache
   /// one time -- the first initialization that wins a compare_exchange_strong.
   RefCountPtr<SyntaxData> getChild(size_t index) const
   {
      if (!getRaw()->getChild(index)) {
         return nullptr;
      }
      return getChildren()[index].getOrCreate([&]() {
         return realizeSyntaxNode(index);
      });
   }

   /// Calculate the absolute position of this node, use cache if the cache
   /// is populated.
   AbsolutePosition getAbsolutePosition() const;

   /// Calculate the absolute end position of this node, use cache of the immediate
   /// next node if populated.
   AbsolutePosition getAbsoluteEndPositionAfterTrailingTrivia() const;

   /// Get the absolute position without skipping the leading trivia of this
   /// node.
   AbsolutePosition getAbsolutePositionBeforeLeadingTrivia() const;

   /// Returns true if the data node represents statement syntax.
   bool isStmt() const;

   /// Returns true if the data node represents declaration syntax.
   bool isDecl() const;

   /// Returns true if the data node represents expression syntax.
   bool isExpr() const;

   /// Returns true if this syntax is of some "unknown" kind.
   bool isUnknown() const;

   /// Dump a debug description of the syntax data for debugging to
   /// standard error.
   void dump(raw_ostream &outStream) const;

   POLAR_ATTRIBUTE_DEPRECATED(void dump() const POLAR_ATTRIBUTE_USED,
                              "Only meant for use in the debugger");
private:
   friend class TrailingObjects;

   using RootDataPair = std::pair<RefCountPtr<SyntaxData>, RefCountPtr<SyntaxData>>;

   /// The shared m_raw syntax representing this syntax data node.
   const RefCountPtr<RawSyntax> m_raw;

   /// The m_parent of this syntax.
   ///
   /// WARNING! Do not access this directly. Use getParent(),
   /// which enforces nullptr checking.
   const SyntaxData *m_parent;

   /// The index into the m_parent's child layout.
   ///
   /// If there is no m_parent, this is 0.
   const CursorIndex m_indexInParent;

   /// Cache the absolute position of this node.
   std::optional<AbsolutePosition> m_positionCache;

   size_t numTrailingObjects(OverloadToken<AtomicCache<SyntaxData>>) const
   {
      return m_raw->getNumChildren();
   }

   SyntaxData(RefCountPtr<RawSyntax> raw, const SyntaxData *parent = nullptr,
              CursorIndex indexInParent = 0)
      : m_raw(raw),
        m_parent(parent),
        m_indexInParent(indexInParent)
   {
      auto *iter = getTrailingObjects<AtomicCache<SyntaxData>>();
      for (auto *end = iter + getNumChildren(); iter != end; ++iter) {
         ::new (static_cast<void *>(iter)) AtomicCache<SyntaxData>();
      }
   }

   /// With a new RawSyntax node, create a new node from this one and
   /// recursively rebuild the parental chain up to the root.
   ///
   /// DO NOT expose this as public API.
   RootDataPair replaceSelf(const RefCountPtr<RawSyntax> newRaw) const
   {
      if (hasParent()) {
         auto  newRootAndParent = m_parent->replaceChild(newRaw, m_indexInParent);
         auto newMe =  newRootAndParent.second->getChild(m_indexInParent);
         return {  newRootAndParent.first, newMe.get() };
      } else {
         auto newMe = make(newRaw, nullptr, m_indexInParent);
         return { newMe, newMe.get() };
      }
   }

   /// Create the data for a child node with the m_raw syntax in our layout
   /// at the provided index.
   /// DO NOT expose this as public API.
   RefCountPtr<SyntaxData> realizeSyntaxNode(CursorIndex index) const
   {
      if (auto &rawChild = m_raw->getChild(index)) {
         return SyntaxData::make(rawChild, this, index);
      }
      return nullptr;
   }

   /// Replace a child in the m_raw syntax and recursively rebuild the
   /// parental chain up to the root.
   ///
   /// DO NOT expose this as public API.
   template <typename CursorType>
   RootDataPair replaceChild(const RefCountPtr<RawSyntax> rawChild,
                             CursorType childCursor) const
   {
      auto newRaw = m_raw->replaceChild(childCursor, rawChild);
      return replaceSelf(newRaw);
   }

   ArrayRef<AtomicCache<SyntaxData>> getChildren() const
   {
      return {getTrailingObjects<AtomicCache<SyntaxData>>(), getNumChildren()};
   }
};

} // polar::syntax

namespace llvm
{

using SD = polar::syntax::SyntaxData;
using RCSD = polar::syntax::RefCountPtr<SD>;
template <>
struct DenseMapInfo<RCSD>
{
   static inline RCSD getEmptyKey()
   {
      return SD::make(nullptr, nullptr, 0);
   }

   static inline RCSD getTombstoneKey()
   {
      return SD::make(nullptr, nullptr, 0);
   }

   static unsigned getHashValue(const RCSD value)
   {
      unsigned H = 0;
      H ^= DenseMapInfo<uintptr_t>::getHashValue(reinterpret_cast<uintptr_t>(value->getRaw().get()));
      H ^= DenseMapInfo<uintptr_t>::getHashValue(reinterpret_cast<uintptr_t>(value->getParent()));
      H ^= DenseMapInfo<polar::syntax::CursorIndex>::getHashValue(value->getIndexInParent());
      return H;
   }

   static bool isEqual(const RCSD lhs, const RCSD rhs)
   {
      return lhs->getRaw().get() == rhs->getRaw().get() &&
            lhs->getParent() == rhs->getParent() &&
            lhs->getIndexInParent() == rhs->getIndexInParent();
   }
};

} // llvm

#endif // POLARPHP_SYNTAX_SYNTAXDATA_H
