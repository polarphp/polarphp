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

#include "llvm/ADT/PointerUnion.h"
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/syntax/TokenKinds.h"
#include "llvm/Support/Allocator.h"
#include "polarphp/syntax/SyntaxKind.h"

namespace polar::ast {
class DiagnosticEngine;
} // polar::ast

namespace polar::parser {

class ParsedSyntax;
class ParsedTokenSyntax;
struct ParsedTrivia;
class Token;

using polar::syntax::SyntaxKind;
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
class alignas(1 << SyntaxAlignInBits) SyntaxParsingContext
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

      llvm::BumpPtrAllocator scratchAlloc;

      RootContextData(DiagnosticEngine &diags,
                      SourceManager &sourceMgr, unsigned bufferId)
         : diags(diags),
           sourceMgr(sourceMgr),
           bufferId(bufferId)
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

public:
   /// Construct root context.
   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder,
                        DiagnosticEngine &diags, SourceManager &sourceMgr,
                        unsigned bufferId);

   /// Designated constructor for child context.
   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder)
      : m_isBacktracking(ctxtHolder->m_isBacktracking),
        m_enabled(ctxtHolder->isEnabled())
   {
      assert(ctxtHolder->m_mode != AccumulationMode::SkippedForIncrementalUpdate &&
            "Cannot create child context for a node loaded from the cache");
      ctxtHolder = this;
   }

   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder, SyntaxContextKind kind)
      : SyntaxParsingContext(ctxtHolder)
   {
   }

   SyntaxParsingContext(SyntaxParsingContext *&ctxtHolder, SyntaxKind kind)
      : SyntaxParsingContext(ctxtHolder)
   {
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
