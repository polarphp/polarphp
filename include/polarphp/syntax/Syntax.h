//===--- Syntax.h - Swift Syntax Interface ----------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/05/07.
//
//===----------------------------------------------------------------------===//
//
// This file defines the Syntax type, the main public-facing classes and
// subclasses for dealing with polarphp Syntax.
//
// Syntax types contain a strong reference to the root of the tree to keep
// the subtree above alive, and a weak reference to the data representing
// the syntax node (weak to prevent retain cycles). All significant public API
// are contained in Syntax and its subclasses.

#ifndef POLARPHP_SYNTAX_SYNTAX_H
#define POLARPHP_SYNTAX_SYNTAX_H

#include "polarphp/syntax/SyntaxData.h"
#include "polarphp/syntax/References.h"
#include "polarphp/syntax/RawSyntax.h"
#include "polarphp/syntax/Trivia.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/Support/raw_ostream.h"

#ifdef POLAR_DEBUG_BUILD
#  include <set>
#  include <map>
#endif

#include <optional>

namespace polar::syntax {

class SyntaxNodeVisitor;

template <typename SyntaxNode>
SyntaxNode make(RefCountPtr<RawSyntax> raw)
{
   auto data = SyntaxData::make(raw);
   return { data, data.get() };
}

/// The main handle for syntax nodes - subclasses contain all public
/// structured editing APIs.
///
/// This opaque structure holds two pieces of data: a strong reference to a
/// root node and a weak reference to the node itself. The node of interest can
/// be weakly held because the data nodes contain strong references to
/// their children.
class Syntax
{
public:
   Syntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : m_root(root),
        m_data(data)
   {
      assert(m_data != nullptr);
   }

   virtual ~Syntax();

   /// Get the kind of syntax.
   SyntaxKind getKind() const;

   /// Get the shared raw syntax.
   RefCountPtr<RawSyntax> getRaw() const;

   /// Get an ID for this node that is stable across incremental parses
   SyntaxNodeId getId() const
   {
      return getRaw()->getId();
   }

   /// Get the number of child nodes in this piece of syntax, not including
   /// tokens.
   size_t getNumChildren() const;

   /// Get the Nth child of this piece of syntax.
   std::optional<Syntax> getChild(const size_t index) const;

   /// Returns true if the syntax node is of the given type.
   template <typename T>
   bool is() const
   {
      return T::classOf(this);
   }

   /// Get the m_data for this Syntax node.
   const SyntaxData &getData() const
   {
      return *m_data;
   }

   const SyntaxData *getDataPointer() const
   {
      return m_data;
   }

   /// Cast this Syntax node to a more specific type, asserting it's of the
   /// right kind.
   template <typename T>
   T castTo() const
   {
      assert(is<T>() && "castTo<T>() node of incompatible type!");
      return T { m_root, m_data };
   }

   /// If this Syntax node is of the right kind, cast and return it,
   /// otherwise return None.
   template <typename T>
   std::optional<T> getAs() const
   {
      if (is<T>()) {
         return castTo<T>();
      }
      return std::nullopt;
   }

   /// Return the parent of this node, if it has one.
   std::optional<Syntax> getParent() const;

   /// Return the root syntax of this node.
   Syntax getRoot() const;

   /// Returns the child index of this node in its parent,
   /// if it has one, otherwise 0.
   CursorIndex getIndexInParent() const
   {
      return getData().getIndexInParent();
   }

   /// Return the number of bytes this node takes when spelled out in the source
   size_t getTextLength() const
   {
      return getRaw()->getTextLength();
   }

   /// Returns true if this syntax node represents a token.
   bool isToken() const;

   /// Returns true if this syntax node represents a statement.
   bool isStmt() const;

   /// Returns true if this syntax node represents a declaration.
   bool isDecl() const;

   /// Returns true if this syntax node represents an expression.
   bool isExpr() const;

   /// Returns true if this syntax is of some "unknown" kind.
   bool isUnknown() const;

   /// Returns true if the node is "missing" in the source (i.e. it was
   /// expected (or optional) but not written.
   bool isMissing() const;

   /// Returns true if the node is "present" in the source.
   bool isPresent() const;

   /// Print the syntax node with full fidelity to the given output stream.
   void print(raw_ostream &outStream, SyntaxPrintOptions opts = SyntaxPrintOptions()) const;

   /// Print a debug representation of the syntax node to the given output stream
   /// and indentation level.
   void dump(raw_ostream &outStream, unsigned indent = 0) const;

   /// Print a debug representation of the syntax node to standard error.
   void dump() const;

   bool hasSameIdentityAs(const Syntax &other) const
   {
      return m_root == other.m_root && m_data == other.m_data;
   }

   static bool kindOf([[maybe_unused]] SyntaxKind kind)
   {
      return true;
   }

   static bool classOf([[maybe_unused]] const Syntax *syntax)
   {
      // Trivially true.
      return true;
   }

   /// Recursively visit this node.
   void accept(SyntaxNodeVisitor &visitor);

   /// Get the absolute position of this raw syntax: its offset, line,
   /// and column.
   AbsolutePosition getAbsolutePosition() const
   {
      return m_data->getAbsolutePosition();
   }

   /// Get the absolute end position (exclusively) where the trailing trivia of
   /// this node ends.
   AbsolutePosition getAbsoluteEndPositionAfterTrailingTrivia() const
   {
      return m_data->getAbsoluteEndPositionAfterTrailingTrivia();
   }

   /// Get the absolute position at which the leading trivia of this node starts.
   AbsolutePosition getAbsolutePositionBeforeLeadingTrivia() const
   {
      return m_data->getAbsolutePositionBeforeLeadingTrivia();
   }

protected:
   /// A strong reference to the root node of the tree in which this piece of
   /// syntax resides.
   const RefCountPtr<SyntaxData> m_root;

   /// A raw pointer to the data representing this syntax node.
   ///
   /// This is mutable for being able to set cached child members, which are
   /// lazily created.
   mutable const SyntaxData *m_data;
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_H
