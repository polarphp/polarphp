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

#ifndef POLARPHP_PARSER_SYNTAX_PARSING_CONTEXT_H
#define POLARPHP_PARSER_SYNTAX_PARSING_CONTEXT_H

#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/parser/ParsedRawSyntaxNode.h"
#include "polarphp/parser/ParsedRawSyntaxRecorder.h"

namespace polar::ast {
class DiagnosticEngine;
} // polar::ast

namespace polar::parser {

class ParsedSyntax;
class ParsedTokenSyntax;
struct ParsedTrivia;
class Token;

using polar::syntax::internal::TokenKindType;
using polar::ast::DiagnosticEngine;

enum class SyntaxContextKind
{
   Decl,
   Stmt,
   Expr,
   Type,
   Syntax,
};

enum class SyntaxNodeCreationKind
{
   /// This is for \c SyntaxParsingContext to collect the syntax data and create
   /// a 'recorded' ParsedRawSyntaxNode object, which would be a result of
   /// passing the index data to the \c SyntaxParseActions implementation.
   Recorded,
   /// This is for \c SyntaxParsingContext to collect the syntax data and create
   /// a 'deferred' ParsedRawSyntaxNode object, which captures the data for a
   /// \c SyntaxParseActions invocation to occur later.
   ///
   /// This is intended to be used for when it's not clear what will be the final
   /// syntax node in the current parsing context.
   Deferred,
};

constexpr size_t SyntaxAlignInBits = 3;

/// RAII object which receive RawSyntax parts. On destruction, this constructs
/// a specified syntax node from received parts and propagate it to the parent
/// context.
///
/// e.g.
///   parseExprParen() {
///     SyntaxParsingContext LocalCtxt(SyntaxKind::ParenExpr, SyntaxContext);
///     consumeToken(tok::l_paren) // In consumeToken(), a RawTokenSyntax is
///                                // added to the context.
///     parseExpr(); // On returning from parseExpr(), a Expr Syntax node is
///                  // created and added to the context.
///     consumeToken(tok::r_paren)
///     // Now the context holds { '(' Expr ')' }.
///     // From these parts, it creates ParenExpr node and add it to the parent.
///   }
class /*alignas(1 << SyntaxAlignInBits)*/ SyntaxParsingContext
{
public:
   /// The shared data for all syntax parsing contexts with the same root.
   /// This should be accessible from the root context only.
   struct alignas(1 << SyntaxAlignInBits) RootContextData
   {
      // Where to issue diagnostics.
      DiagnosticEngine &diags;

      SourceManager &sourceMgr;

      unsigned bufferId;

      // storage for Collected parts.
      std::vector<ParsedRawSyntaxNode> storage;

      ParsedRawSyntaxRecorder recorder;

      polar::utils::BumpPtrAllocator scratchAlloc;

      RootContextData(DiagnosticEngine &diags,
                      SourceManager &sourceMgr, unsigned bufferId,
                      std::shared_ptr<SyntaxParseActions> spActions)
         : diags(diags),
           sourceMgr(sourceMgr),
           bufferId(bufferId),
           recorder(std::move(spActions))
      {}
   };

private:
   /// Indicates what action should be performed on the destruction of
   ///  SyntaxParsingContext
   enum class AccumulationMode
   {
      // Coerece the result to one of ContextKind.
      // E.g. for ContextKind::Expr, passthroug if the result is CallExpr, whereas
      // <UnknownExpr><SomeDecl /></UnknownDecl> for non Exprs.
      CoerceKind,

      // Construct a result Syntax with specified SyntaxKind.
      CreateSyntax,

      /// Construct a deferred raw node, to be recorded later.
      DeferSyntax,

      // Pass through all parts to the parent context.
      Transparent,

      // Discard all parts in the context.
      Discard,

      // The node has been found as incremental update and all parts shall be
      // discarded.
      SkippedForIncrementalUpdate,

      // Construct SourceFile syntax to the specified SF.
      Root,

      // Invalid.
      NotSet,
   };

   // When this context is a root, this points to an instance of RootContextData;
   // When this context isn't a root, this points to the parent context.
   const polar::utils::PointerUnion<RootContextData *, SyntaxParsingContext *>
   m_rootDataOrParent;

   // Reference to the
   SyntaxParsingContext *&m_ctxtHolder;

   RootContextData *m_rootData;

   // Offet for 'storage' this context owns from.
   const size_t m_offset;

   // Operation on destruction.
   AccumulationMode m_mode = AccumulationMode::NotSet;

   // Additional info depending on \c m_mode.
   union {
      // For AccumulationMode::CreateSyntax; desired syntax node kind.
      SyntaxKind m_synKind;
      // For AccumulationMode::CoerceKind; desired syntax node category.
      SyntaxContextKind m_ctxtKind;
   };

   /// true if it's in backtracking context.
   bool m_isBacktracking = false;

   // If false, context does nothing.
   bool m_enabled;

   /// Create a syntax node using the tail \c N elements of collected parts and
   /// replace those parts with the single result.
   void createNodeInPlace(SyntaxKind kind, size_t N,
                          SyntaxNodeCreationKind nodeCreateK);

   ArrayRef<ParsedRawSyntaxNode> getParts() const
   {
      return polar::basic::make_array_ref(getStorage()).drop_front(m_offset);
   }

   ParsedRawSyntaxNode makeUnknownSyntax(SyntaxKind kind,
                                         ArrayRef<ParsedRawSyntaxNode> parts);
   ParsedRawSyntaxNode createSyntaxAs(SyntaxKind kind,
                                      ArrayRef<ParsedRawSyntaxNode> parts,
                                      SyntaxNodeCreationKind nodeCreateK);
   std::optional<ParsedRawSyntaxNode> bridgeAs(SyntaxContextKind kind,
                                          ArrayRef<ParsedRawSyntaxNode> parts);

public:
   /// Construct root context.
   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder,
                        unsigned bufferId,
                        std::shared_ptr<SyntaxParseActions> actions);

   /// Designated constructor for child context.
   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder)
      : m_rootDataOrParent(ctxtHolder),
        m_ctxtHolder(ctxtHolder),
        m_rootData(ctxtHolder->m_rootData),
        m_offset(m_rootData->storage.size()),
        m_isBacktracking(ctxtHolder->m_isBacktracking),
        m_enabled(ctxtHolder->isEnabled())
   {
      assert(ctxtHolder->isTopOfContextStack() &&
             "SyntaxParsingContext cannot have multiple children");
      assert(ctxtHolder->m_mode != AccumulationMode::SkippedForIncrementalUpdate &&
            "Cannot create child context for a node loaded from the cache");
      ctxtHolder = this;
   }

   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder, SyntaxContextKind kind)
      : SyntaxParsingContext(ctxtHolder)
   {
      setCoerceKind(kind);
   }

   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder, SyntaxKind kind)
      : SyntaxParsingContext(ctxtHolder)
   {
      setCreateSyntax(kind);
   }

   ~SyntaxParsingContext();

   /// Try looking up if an unmodified node exists at \p lexerOffset of the same
   /// kind. If a node is found, replace the node that is currently being
   /// constructed by the parsing context with the found node and return the
   /// number of bytes the found node took up in the original source. The lexer
   /// should pretend it has read these bytes and continue from the advanced
   /// offset. If nothing is found \c 0 is returned.
   size_t lookupNode(size_t lexerOffset, SourceLoc loc);

   void disable()
   {
      m_enabled = false;
   }

   bool isEnabled() const
   {
      return m_enabled;
   }

   bool isRoot() const
   {
      return m_rootDataOrParent.is<RootContextData*>();
   }

   bool isTopOfContextStack() const
   {
      return this == m_ctxtHolder;
   }

   SyntaxParsingContext *getParent() const
   {
      return m_rootDataOrParent.get<SyntaxParsingContext*>();
   }

   RootContextData *getRootData()
   {
      return m_rootData;
   }

   const RootContextData *getRootData() const
   {
      return m_rootData;
   }

   std::vector<ParsedRawSyntaxNode> &getStorage()
   {
      return getRootData()->storage;
   }

   const std::vector<ParsedRawSyntaxNode> &getStorage() const
   {
      return getRootData()->storage;
   }

   const SyntaxParsingContext *getRoot() const;

   ParsedRawSyntaxRecorder &getRecorder()
   {
      return getRootData()->recorder;
   }

   polar::utils::BumpPtrAllocator &getScratchAlloc()
   {
      return getRootData()->scratchAlloc;
   }

   /// Add RawSyntax to the parts.
   void addRawSyntax(ParsedRawSyntaxNode raw);

   /// Add Token with Trivia to the parts.
   void addToken(Token &token, const ParsedTrivia &leadingTrivia,
                 const ParsedTrivia &trailingTrivia);

   /// Add Syntax to the parts.
   void addSyntax(ParsedSyntax node);


   template<typename SyntaxNode>
   std::optional<SyntaxNode> popIf()
   {
      auto &storage = getStorage();
      assert(storage.size() > m_offset);
      if (SyntaxNode::kindof(storage.back().getKind())) {
         auto rawNode = std::move(storage.back());
         storage.pop_back();
         return SyntaxNode(rawNode);
      }
      return std::nullopt;
   }

   ParsedTokenSyntax popToken();

   /// Create a node using the tail of the collected parts. The number of parts
   /// is automatically determined from \c kind. Node: limited number of \c kind
   /// are supported. See the implementation.
   void createNodeInPlace(SyntaxKind kind,
                          SyntaxNodeCreationKind nodeCreateK = SyntaxNodeCreationKind::Recorded);

   /// Squashing nodes from the back of the pending syntax list to a given syntax
   /// collection kind. If there're no nodes that can fit into the collection kind,
   /// this function does nothing. Otherwise, it creates a collection node in place
   /// to contain all sequential suitable nodes from back.
   void collectNodesInPlace(SyntaxKind colletionKind,
                            SyntaxNodeCreationKind nodeCreateK = SyntaxNodeCreationKind::Recorded);

   /// On destruction, construct a specified kind of syntax node consuming the
   /// collected parts, then append it to the parent context.
   void setCreateSyntax(SyntaxKind kind)
   {
      m_mode = AccumulationMode::CreateSyntax;
      m_synKind = kind;
   }

   /// Same as \c setCreateSyntax but create a deferred node instead of a
   /// recorded one.
   void setDeferSyntax(SyntaxKind kind)
   {
      m_mode = AccumulationMode::DeferSyntax;
      m_synKind = kind;
   }

   /// On destruction, if the parts size is 1 and it's kind of \c kind, just
   /// append it to the parent context. Otherwise, create Unknown{kind} node from
   /// the collected parts.
   void setCoerceKind(SyntaxContextKind kind)
   {
      m_mode = AccumulationMode::CoerceKind;
      m_ctxtKind = kind;
   }

   /// Move the collected parts to the tail of parent context.
   void setTransparent()
   {
      m_mode = AccumulationMode::Transparent;
   }

   /// This context is a back tracking context, so we should discard collected
   /// parts on this context.
   void setBackTracking() {
      m_mode = AccumulationMode::Discard;
      m_isBacktracking = true;
   }

   bool isBacktracking() const
   {
      return m_isBacktracking;
   }

   /// Explicitly finalizing syntax tree creation.
   /// This function will be called during the destroying of a root syntax
   /// parsing context. However, we can explicitly call this function to get
   /// the syntax tree before closing the root context.
   ParsedRawSyntaxNode finalizeRoot();

   /// Make a missing node corresponding to the given token kind and
   /// push this node into the context. The synthesized node can help
   /// the creation of valid syntax nodes.
   void synthesize(TokenKindType kind, SourceLoc loc);

   /// Dump the nodes that are in the storage stack of the SyntaxParsingContext
   POLAR_ATTRIBUTE_DEPRECATED(void dumpStorage() const POLAR_ATTRIBUTE_USED,
                              "Only meant for use in the debugger");
};

} // polar::parser

#endif // POLARPHP_PARSER_SYNTAX_PARSING_CONTEXT_H
