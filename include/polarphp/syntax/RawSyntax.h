//===--- RawSyntax.h - Swift raw Syntax Interface ---------------*- ch++ -*-===//
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
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
// Created by polarboy on 2019/05/08.
//===----------------------------------------------------------------------===//
// This file defines the RawSyntax type.
//
// These are the "backbone or "skeleton" of the Syntax tree, providing
// the recursive structure, child relationships, kind of node, etc.
//
// They are reference-counted and strictly immutable, so can be shared freely
// among Syntax nodes and have no specific identity. They could even in theory
// be shared for expressions like 1 + 1 + 1 + 1 - you don't need 7 syntax nodes
// to express that at this layer.
//
// These are internal implementation ONLY - do not expose anything involving
// RawSyntax publicly. Clients of lib/Syntax should not be aware that they
// exist.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_SYNTAX_RAWSYNTAX_H
#define POLARPHP_SYNTAX_RAWSYNTAX_H

#include "llvm/ADT/FoldingSet.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/TrailingObjects.h"
#include "llvm/Support/raw_ostream.h"

#include "polarphp/basic/InlineBitfield.h"
#include "polarphp/syntax/References.h"
#include "polarphp/syntax/SyntaxArena.h"
#include "polarphp/syntax/SyntaxKind.h"
#include "polarphp/syntax/TokenKinds.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/basic/OwnedString.h"

#include <vector>
#include <atomic>

#ifndef NDEBUG
#define syntax_assert_child_kind(raw, cursorName, expectedKind)              \
   do {                                                                      \
      if (auto &__child = raw->getChild(cursorName)) {                       \                                                        \
         assert(__child->getKind() == expectedKind)                          \
      }                                                                      \
   } while (false)
#else
#define syntax_assert_child_kind(raw, cursorName, expectedKind)
#endif

#ifndef NDEBUG
#define syntax_assert_child_token(raw, cursorName, ...)                        \
   do {                                                                        \
      bool __found = false;                                                    \
      std::string errorMsg;                                                    \
      if (auto &__token = raw->getChild(Cursor::cursorName)) {                 \
         assert(__token->isToken());                                           \
         if (__token->isPresent()) {                                           \
            for (auto tokenKind : {__VA_ARGS__}) {                             \
               if (__token->getTokenKind() == tokenKind) {                     \
                  __found = true;                                              \
                  break;                                                       \
               }                                                               \
            }                                                                  \
            assert(__found && "invalid token supplied for " #cursorName        \
                              ", expected one of {" #__VA_ARGS__ "}");         \
         }                                                                     \
      }                                                                        \
   } while (false)
#else
#define syntax_assert_child_token(raw, cursorName, ...)
#endif

#ifndef NDEBUG
#define syntax_assert_child_token_text(raw, cursorName, tokenKind, ...)          \
   do {                                                                          \
      bool __found = false;                                                      \
      if (auto &__child = raw->getChild(cursor::cursorName)) {                   \
         assert(__child->isToken());                                             \
         if (__child->isPresent()) {                                             \
            assert(__child->getTokenKind() == tokenKind);                        \
         for (auto __text : {__VA_ARGS__}) {                                     \
            if (__child->getTokenText() == __text) {                             \
               __found = true;                                                   \
               break;                                                            \
            }                                                                    \
         }                                                                       \
         assert(__found && "invalid text supplied for " #cursorName              \
         ", expected one of {" #__VA_ARGS__ "}");                                \
         }                                                                       \
      }                                                                          \
   } while (false)
#else
#define syntax_assert_child_token_text(raw, cursorName, tokenKind, ...)
#endif

#ifndef NDEBUG
#define syntax_assert_token_is(token, kind, text)                               \
   do {                                                                         \
      assert(token.getTokenKind() == kind);                                     \
      assert(token.getText() == text);                                          \
   } while (false)
#else
#define syntax_assert_token_is(token, kind, text)
#endif

namespace polar::syntax {

using llvm::raw_ostream;
using llvm::TrailingObjects;
using llvm::StringRef;
using llvm::ArrayRef;
using llvm::FoldingSetNodeID;
using polar::basic::OwnedString;

class SyntaxArena;
using CursorIndex = size_t;

/// Get a numeric index suitable for array/vector indexing
/// from a syntax node's Cursor enum value.
template <typename CursorType>
constexpr CursorIndex cursor_index(CursorType cursor)
{
   return static_cast<CursorIndex>(cursor);
}

/// An absolute position in a source file as text - the absolute offset from
/// the start, line, and column.
class AbsolutePosition
{
public:
   /// Add some number of columns to the position.
   void addColumns(uint32_t columns)
   {
      m_column += columns;
      m_offset += columns;
   }

   /// Add some number of newlines to the position, resetting the column.
   /// size is byte size of newline char.
   /// '\n' and '\r' are 1, '\r\n' is 2.
   void addNewlines(uint32_t newLines, uint32_t size)
   {
      m_line += newLines;
      m_column = 1;
      m_offset += newLines * size;
   }

   /// Use some text as a reference for adding to the absolute position,
   /// taking note of newlines, etc.
   void addText(StringRef str)
   {
      const char * ch = str.begin();
      while (ch != str.end()) {
         switch (*ch++) {
         case '\n':
            addNewlines(1, 1);
            break;
         case '\r':
            if (ch != str.end() && *ch == '\n') {
               addNewlines(1, 2);
               ++ch;
            } else {
               addNewlines(1, 1);
            }
            break;
         default:
            addColumns(1);
            break;
         }
      }
   }

   /// Get the line number of this position.
   uint32_t getLine() const
   {
      return m_line;
   }

   /// Get the column number of this position.
   uint32_t getColumn() const
   {
      return m_column;
   }

   /// Get the line and column number of this position.
   std::pair<uint32_t, uint32_t> getLineAndColumn() const
   {
      return {m_line, m_column};
   }

   /// Get the absolute offset of this position, suitable for indexing into a
   /// buffer if applicable.
   uintptr_t getOffset() const
   {
      return m_offset;
   }

   /// Print the line and column as "l:c" to the given output stream.
   void printLineAndColumn(raw_ostream &outStream) const;

   /// Dump a description of this position to the given output stream
   /// for debugging.
   void dump(raw_ostream &outStream = llvm::errs()) const;

private:
   uintptr_t m_offset = 0;
   uint32_t m_line = 1;
   uint32_t m_column = 1;
};

/// An indicator of whether a Syntax node was found or written in the source.
///
/// This is not an 'implicit' bit.
enum class SourcePresence
{
   /// The syntax was authored by a human and found, or was generated.
   Present,

   /// The syntax was expected or optional, but not found in the source.
   Missing,
};

/// The print option to specify when printing a raw syntax node.
struct SyntaxPrintOptions
{
   bool visual = false;
   bool printSyntaxKind = false;
   bool printTrivialNodeKind = false;
};

using SyntaxNodeId = unsigned;

/// RawSyntax - the strictly immutable, shared backing nodes for all syntax.
///
/// This is implementation detail - do not expose it in public API.
class RawSyntax final
      : private TrailingObjects<RawSyntax, RefCountPtr<RawSyntax>, OwnedString, std::int64_t, double,
                                TriviaPiece>
{
public:
   ~RawSyntax();

   // This is a copy-pased implementation of llvm::ThreadSafeRefCountedBase with
   // the difference that we do not delete the RawSyntax node's memory if the
   // node was allocated within a SyntaxArena and thus doesn't own its memory.
   void retain() const
   {
      m_refCount.fetch_add(1, std::memory_order_relaxed);
   }

   void Retain() const
   {
      retain();
   }

   void release() const
   {
      int newRefCount = m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
      assert(newRefCount >= 0 && "Reference count was already zero.");
      if (newRefCount == 0) {
         if (arena) {
            // The node was allocated inside a SyntaxArena and thus doesn't own its
            // own memory region. Hence we cannot free it. It will be deleted once
            // the last RawSyntax node allocated with it will release its reference
            // to the arena.
            this->~RawSyntax();
         } else {
            delete this;
         }
      }
   }

   void Release() const
   {
      release();
   }

   /// \name Factory methods.
   /// @{

   /// Make a raw "layout" syntax node.
   static RefCountPtr<RawSyntax> make(SyntaxKind kind, ArrayRef<RefCountPtr<RawSyntax>> layout,
                                      SourcePresence presence,
                                      std::optional<SyntaxNodeId> nodeId = std::nullopt)
   {
      RefCountPtr<SyntaxArena> arena = nullptr;
      return make(kind, layout, presence, arena, nodeId);
   }

   /// Make a raw "layout" syntax node that was allocated in \p arena.
   static RefCountPtr<RawSyntax> make(SyntaxKind kind, ArrayRef<RefCountPtr<RawSyntax>> layout,
                                      SourcePresence presence,
                                      const RefCountPtr<SyntaxArena> &arena,
                                      std::optional<SyntaxNodeId> nodeId = std::nullopt);

   /// Make a raw "token" syntax node.
   static RefCountPtr<RawSyntax> make(TokenKindType tokenKind, OwnedString text,
                                      ArrayRef<TriviaPiece> leadingTrivia,
                                      ArrayRef<TriviaPiece> trailingTrivia,
                                      SourcePresence presence,
                                      std::optional<SyntaxNodeId> nodeId = std::nullopt)
   {
      RefCountPtr<SyntaxArena> arena = nullptr;
      return make(tokenKind, text, leadingTrivia, trailingTrivia, presence, arena,
                  nodeId);
   }

   /// Make a raw "token" syntax node that was allocated in \p arena.
   static RefCountPtr<RawSyntax> make(TokenKindType tokenKind, OwnedString text,
                                      ArrayRef<TriviaPiece> leadingTrivia,
                                      ArrayRef<TriviaPiece> trailingTrivia,
                                      SourcePresence presence,
                                      const RefCountPtr<SyntaxArena> &arena,
                                      std::optional<SyntaxNodeId> nodeId = std::nullopt);

   static RefCountPtr<RawSyntax> make(TokenKindType tokenKind, OwnedString text,
                                      std::int64_t value,
                                      ArrayRef<TriviaPiece> leadingTrivia,
                                      ArrayRef<TriviaPiece> trailingTrivia,
                                      SourcePresence presence,
                                      const RefCountPtr<SyntaxArena> &arena,
                                      std::optional<SyntaxNodeId> nodeId = std::nullopt);

   static RefCountPtr<RawSyntax> make(TokenKindType tokenKind, OwnedString text,
                                      double value,
                                      ArrayRef<TriviaPiece> leadingTrivia,
                                      ArrayRef<TriviaPiece> trailingTrivia,
                                      SourcePresence presence,
                                      const RefCountPtr<SyntaxArena> &arena,
                                      std::optional<SyntaxNodeId> nodeId = std::nullopt);

   /// Make a missing raw "layout" syntax node.
   static RefCountPtr<RawSyntax> missing(SyntaxKind kind,
                                         RefCountPtr<SyntaxArena> arena = nullptr)
   {
      return make(kind, {}, SourcePresence::Missing, arena);
   }

   /// Make a missing raw "token" syntax node.
   static RefCountPtr<RawSyntax> missing(TokenKindType tokenKind, OwnedString text,
                                         RefCountPtr<SyntaxArena> arena = nullptr)
   {
      return make(tokenKind, text, {}, {}, SourcePresence::Missing, arena);
   }

   /// @}

   SourcePresence getPresence() const
   {
      return static_cast<SourcePresence>(m_bits.common.presence);
   }

   SyntaxKind getKind() const
   {
      return static_cast<SyntaxKind>(m_bits.common.kind);
   }

   bool kindOf(SyntaxKind kind) const
   {
      return getKind() == kind;
   }

   /// Get an id for this node that is stable across incremental parses
   SyntaxNodeId getId() const
   {
      return m_nodeId;
   }

   /// Returns true if the node is "missing" in the source (i.e. it was
   /// expected (or optional) but not written.
   bool isMissing() const
   {
      return getPresence() == SourcePresence::Missing;
   }

   /// Returns true if the node is "present" in the source.
   bool isPresent() const
   {
      return getPresence() == SourcePresence::Present;
   }

   /// Returns true if this raw syntax node is some kind of declaration.
   bool isDecl() const
   {
      return is_decl_kind(getKind());
   }

   /// Returns true if this raw syntax node is some kind of statement.
   bool isStmt() const
   {
      return is_stmt_kind(getKind());
   }

   /// Returns true if this raw syntax node is some kind of expression.
   bool isExpr() const
   {
      return is_expr_kind(getKind());
   }

   /// Return true is this raw syntax node is a unknown node.
   bool isUnknown() const
   {
      return is_unknown_kind(getKind());
   }

   /// Return true if this raw syntax node is a token.
   bool isToken() const
   {
      return is_token_kind(getKind());
   }

   /// \name Getter routines for SyntaxKind::token.
   /// @{

   /// Get the kind of the token.
   TokenKindType getTokenKind() const
   {
      assert(isToken());
      return static_cast<TokenKindType>(m_bits.token.tokenKind);
   }

   /// Return the text of the token as an \c OwnedString. Keeping a reference to
   /// this string will keep it alive even if the syntax node gets freed.
   OwnedString getOwnedTokenText() const
   {
      assert(isToken());
      return *getTrailingObjects<OwnedString>();
   }

   /// Return the text of the token as a reference. The referenced buffer may
   /// disappear when the syntax node gets freed.
   StringRef getTokenText() const
   {
      return getOwnedTokenText().str();
   }

   /// Return the leading trivia list of the token.
   ArrayRef<TriviaPiece> getLeadingTrivia() const
   {
      assert(isToken());
      return {getTrailingObjects<TriviaPiece>(), m_bits.token.numLeadingTrivia};
   }

   /// Return the trailing trivia list of the token.
   ArrayRef<TriviaPiece> getTrailingTrivia() const
   {
      assert(isToken());
      return {getTrailingObjects<TriviaPiece>() + m_bits.token.numLeadingTrivia,
               m_bits.token.numTrailingTrivia};
   }

   /// Return \c true if this is the given kind of token.
   bool isToken(TokenKindType K) const
   {
      return isToken() && getTokenKind() == K;
   }

   /// @}

   /// \name Transform routines for "token" nodes.
   /// @{

   /// Return a new token like this one, but with the given leading
   /// trivia instead.
   RefCountPtr<RawSyntax>
   withLeadingTrivia(ArrayRef<TriviaPiece> newLeadingTrivia) const
   {
      return make(getTokenKind(), getOwnedTokenText(), newLeadingTrivia,
                  getTrailingTrivia(), getPresence());
   }

   RefCountPtr<RawSyntax> withLeadingTrivia(Trivia newLeadingTrivia) const
   {
      return withLeadingTrivia(newLeadingTrivia.pieces);
   }

   /// Return a new token like this one, but with the given trailing
   /// trivia instead.
   RefCountPtr<RawSyntax>
   withTrailingTrivia(ArrayRef<TriviaPiece> newTrailingTrivia) const
   {
      return make(getTokenKind(), getOwnedTokenText(), getLeadingTrivia(),
                  newTrailingTrivia, getPresence());
   }

   RefCountPtr<RawSyntax> withTrailingTrivia(Trivia newTrailingTrivia) const
   {
      return withTrailingTrivia(newTrailingTrivia.pieces);
   }

   /// @}

   /// \name Getter routines for "layout" nodes.
   /// @{

   /// Get the child nodes.
   ArrayRef<RefCountPtr<RawSyntax>> getLayout() const
   {
      if (isToken()) {
         return {};
      }
      return {getTrailingObjects<RefCountPtr<RawSyntax>>(), m_bits.layout.numChildren};
   }

   size_t getNumChildren() const
   {
      if (isToken()) {
         return 0;
      }
      return m_bits.layout.numChildren;
   }

   /// Get a child based on a particular node's "Cursor", indicating
   /// the position of the terms in the production of the Swift grammar.
   const RefCountPtr<RawSyntax> &getChild(CursorIndex index) const
   {
      return getLayout()[index];
   }

   /// Return the number of bytes this node takes when spelled out in the source
   size_t getTextLength()
   {
      // For tokens the computation of the length is fast enough to justify the
      // space for caching it. For layout nodes, we cache the length to avoid
      // traversing the tree

      // FIXME: Or would it be sensible to cache the size of token nodes as well?
      if (isToken()) {
         AbsolutePosition pos;
         accumulateAbsolutePosition(pos);
         return pos.getOffset();
      } else {
         if (m_bits.layout.textLength == UINT32_MAX) {
            m_bits.layout.textLength = computeTextLength();
         }
         return m_bits.layout.textLength;
      }
   }

   /// @}

   /// \name Transform routines for "layout" nodes.
   /// @{

   /// Return a new raw syntax node with the given new layout element appended
   /// to the end of the node's layout.
   RefCountPtr<RawSyntax> append(RefCountPtr<RawSyntax> newLayoutElement) const;

   /// Return a new raw syntax node with the given new layout element replacing
   /// another at some cursor position.
   RefCountPtr<RawSyntax>
   replaceChild(CursorIndex index, RefCountPtr<RawSyntax> newLayoutElement) const;

   /// @}

   /// Advance the provided AbsolutePosition by the full width of this node.
   ///
   /// If this is token node, returns the AbsolutePosition of the start of the
   /// token's nontrivial text. Otherwise, return the position of the first
   /// token. If this contains no tokens, return None.
   std::optional<AbsolutePosition>
   accumulateAbsolutePosition(AbsolutePosition &pos) const;

   /// Advance the provided AbsolutePosition by the first trivia of this node.
   /// Return true if we found this trivia; otherwise false.
   bool accumulateLeadingTrivia(AbsolutePosition &pos) const;

   /// Print this piece of syntax recursively.
   void print(raw_ostream &outStream, SyntaxPrintOptions opts) const;

   /// Dump this piece of syntax recursively for debugging or testing.
   void dump() const;

   /// Dump this piece of syntax recursively.
   void dump(raw_ostream &outStream, unsigned indent = 0) const;

   static void profile(FoldingSetNodeID &id, TokenKindType tokenKind, OwnedString text,
                       ArrayRef<TriviaPiece> leadingTrivia,
                       ArrayRef<TriviaPiece> trailingTrivia);
private:
   friend class TrailingObjects;

   /// The id that shall be used for the next node that is created and does not
   /// have a manually specified id
   static SyntaxNodeId sm_nextFreeNodeId;

   /// An id of this node that is stable across incremental parses
   SyntaxNodeId m_nodeId;

   /// If this node was allocated using a \c SyntaxArena's bump allocator, a
   /// reference to the arena to keep the underlying memory buffer of this node
   /// alive. If this is a \c nullptr, the node owns its own memory buffer.
   RefCountPtr<SyntaxArena> arena;

   union {
      uint64_t opaqueBits;
      struct {
         /// The kind of syntax this node represents.
         unsigned kind : polar::basic::bitmax(NumSyntaxKindBits, 8);
         /// Whether this piece of syntax was actually present in the source.
         unsigned presence : 1;
      } common;
      enum { NumRawSyntaxBits = polar::basic::bitmax(NumSyntaxKindBits, 8) + 1 };

      // For "layout" nodes.
      struct {
         static_assert(NumRawSyntaxBits <= 32,
                       "Only 32 bits reserved for standard syntax bits");
         uint64_t : polar::basic::bitmax(NumRawSyntaxBits, 32); // align to 32 bits
         /// Number of children this "layout" node has.
         unsigned numChildren : 32;
         /// Number of bytes this node takes up spelled out in the source code
         /// A value of UINT32_MAX indicates that the text length has not been
         /// computed yet.
         unsigned textLength : 32;
      } layout;

      // For "token" nodes.
      struct {
         static_assert(NumRawSyntaxBits <= 16,
                       "Only 16 bits reserved for standard syntax bits");
         uint64_t : polar::basic::bitmax(NumRawSyntaxBits, 16); // align to 16 bits
         /// The kind of token this "token" node represents.
         unsigned tokenKind : 16;
         /// Number of leading  trivia pieces.
         unsigned numLeadingTrivia : 16;
         /// Number of trailing trivia pieces.
         unsigned numTrailingTrivia : 16;
      } token;
   } m_bits;

   size_t getNumTrailingObjects(OverloadToken<RefCountPtr<RawSyntax>>) const
   {
      return isToken() ? 0 : m_bits.layout.numChildren;
   }

   size_t numTrailingObjects(OverloadToken<RefCountPtr<RawSyntax>> token) const
   {
      return getNumTrailingObjects(token);
   }

   size_t getNumTrailingObjects(OverloadToken<OwnedString>) const
   {
      return isToken() ? 1 : 0;
   }

   size_t numTrailingObjects(OverloadToken<OwnedString> token) const
   {
      return getNumTrailingObjects(token);
   }

   size_t getNumTrailingObjects(OverloadToken<double>) const
   {
      return isToken() ? 1 : 0;
   }

   size_t numTrailingObjects(OverloadToken<double> token) const
   {
      return getNumTrailingObjects(token);
   }

   size_t getNumTrailingObjects(OverloadToken<std::int64_t>) const
   {
      return isToken() ? 1 : 0;
   }

   size_t numTrailingObjects(OverloadToken<std::int64_t> token) const
   {
      return getNumTrailingObjects(token);
   }

   size_t getNumTrailingObjects(OverloadToken<TriviaPiece>) const
   {
      return isToken()
            ? m_bits.token.numLeadingTrivia + m_bits.token.numTrailingTrivia
            : 0;
   }

   size_t numTrailingObjects(OverloadToken<TriviaPiece> token) const
   {
      return getNumTrailingObjects(token);
   }

   /// Constructor for creating layout nodes.
   /// If the node has been allocated inside the bump allocator of a
   /// \c SyntaxArena, that arena must be passed as \p arena to retain the node's
   /// underlying storage.
   /// If \p nodeId is \c None, the next free nodeId is used, if it is passed,
   /// the caller needs to assure that the node id has not been used yet.
   RawSyntax(SyntaxKind kind, ArrayRef<RefCountPtr<RawSyntax>> layout,
             SourcePresence presence, const RefCountPtr<SyntaxArena> &arena,
             std::optional<SyntaxNodeId> nodeId);

   /// Constructor for creating token nodes
   /// \c SyntaxArena, that arena must be passed as \p arena to retain the node's
   /// underlying storage.
   /// If \p nodeId is \c None, the next free nodeId is used, if it is passed,
   /// the caller needs to assure that the nodeId has not been used yet.
   RawSyntax(TokenKindType tokenKind, OwnedString text, ArrayRef<TriviaPiece> leadingTrivia,
             ArrayRef<TriviaPiece> trailingTrivia, SourcePresence presence,
             const RefCountPtr<SyntaxArena> &arena, std::optional<SyntaxNodeId> nodeId);

   RawSyntax(TokenKindType tokenKind, OwnedString text, std::int64_t value, ArrayRef<TriviaPiece> leadingTrivia,
             ArrayRef<TriviaPiece> trailingTrivia, SourcePresence presence,
             const RefCountPtr<SyntaxArena> &arena, std::optional<SyntaxNodeId> nodeId);

   RawSyntax(TokenKindType tokenKind, OwnedString text, double value, ArrayRef<TriviaPiece> leadingTrivia,
             ArrayRef<TriviaPiece> trailingTrivia, SourcePresence presence,
             const RefCountPtr<SyntaxArena> &arena, std::optional<SyntaxNodeId> nodeId);

   /// Compute the node's text length by summing up the length of its childern
   size_t computeTextLength()
   {
      size_t textLength = 0;
      for (size_t index = 0, numChildren = getNumChildren(); index < numChildren; ++index) {
         auto &childNode = getChild(index);
         if (childNode && !childNode->isMissing()) {
            textLength += childNode->getTextLength();
         }
      }
      return textLength;
   }

   mutable std::atomic<int> m_refCount;
};

} // polar::syntax

namespace polar::utils {
raw_ostream &operator<<(raw_ostream &outStream, polar::syntax::AbsolutePosition pos);
} // polar::utils

#endif // POLARPHP_SYNTAX_RAWSYNTAX_H
