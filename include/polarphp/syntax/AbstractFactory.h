// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/14.

#ifndef POLARPHP_SYNTAX_ABSTRACT_FACTORY_H
#define POLARPHP_SYNTAX_ABSTRACT_FACTORY_H

#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/basic/adt/ArrayRef.h"

#include <vector>

namespace polar::syntax {

using polar::basic::ArrayRef;

/// The Abstract Syntax factory - the one-stop shop for making new Syntax nodes.
class AbstractFactory
{
public:
   /// Make any kind of token.
   static TokenSyntax makeToken(TokenKindType kind,
                                OwnedString text, const Trivia &leadingTrivia,
                                const Trivia &trailingTrivia,
                                SourcePresence presence,
                                RefCountPtr<SyntaxArena> arena = nullptr);

   /// Collect a list of tokens into a piece of "unknown" syntax.
   static UnknownSyntax makeUnknownSyntax(ArrayRef<TokenSyntax> tokens,
                                          RefCountPtr<SyntaxArena> arena = nullptr);

   static std::optional<Syntax> createSyntax(SyntaxKind kind,
                                             ArrayRef<Syntax> elements,
                                             RefCountPtr<SyntaxArena> arena = nullptr);

   static RefCountPtr<RawSyntax> createRaw(SyntaxKind kind,
                                           ArrayRef<RefCountPtr<RawSyntax>> elements,
                                           RefCountPtr<SyntaxArena> arena = nullptr);

   /// Count the number of children for a given syntax node kind,
   /// returning a pair of mininum and maximum count of children. The gap
   /// between these two numbers is the number of optional children.
   static std::pair<unsigned, unsigned> countChildren(SyntaxKind kind);

   /// Whether a raw node kind `memberKind` can serve as a member in a syntax
   /// collection of the given syntax collection kind.
   static bool canServeAsCollectionMemberRaw(SyntaxKind collectionKind,
                                             SyntaxKind memberKind);

   /// Whether a raw node `member` can serve as a member in a syntax collection
   /// of the given syntax collection kind.
   static bool canServeAsCollectionMemberRaw(SyntaxKind collectionKind,
                                             const RefCountPtr<RawSyntax> &member);

   /// Whether a node `member` can serve as a member in a syntax collection of
   /// the given syntax collection kind.
   static bool canServeAsCollectionMember(SyntaxKind collectionKind, Syntax member);

   /// make syntax node utils methods
   ///
   static DeclSyntax makeBlankDecl(RefCountPtr<SyntaxArena> arena = nullptr);
   static ExprSyntax makeBlankExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static StmtSyntax makeBlankStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static TypeSyntax makeBlankType(RefCountPtr<SyntaxArena> arena = nullptr);
   static TokenSyntax makeBlankToken(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnknownSyntax makeBlankUnknown(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnknownDeclSyntax makeBlankUnknownDecl(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnknownExprSyntax makeBlankUnknownExpr(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnknownStmtSyntax makeBlankUnknownStmt(RefCountPtr<SyntaxArena> arena = nullptr);
   static UnknownTypeSyntax makeBlankUnknownType(RefCountPtr<SyntaxArena> arena = nullptr);
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_ABSTRACT_FACTORY_H
