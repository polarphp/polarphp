// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/08.

#ifndef POLARPHP_SYNTAX_FACTORIES_H
#define POLARPHP_SYNTAX_FACTORIES_H

#include "polarphp/syntax/SyntaxNodes.h"
#include "polarphp/syntax/SyntaxKind.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/Trivia.h"
#include "polarphp/syntax/TokenIdDefs.h"
#include "polarphp/basic/adt/ArrayRef.h"

#include <vector>

namespace polar::syntax {

using polar::basic::ArrayRef;

class SyntaxArena;

/// The Syntax factory - the one-stop shop for making new Syntax nodes.
class SyntaxFactory
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

   static Syntax
   makeBlankCollectionSyntax(SyntaxKind kind);

#pragma mark - Convenience APIs

   static TupleTypeSyntax makeVoidTupleType(RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates an labelled TupleTypeElementSyntax with the provided label,
   /// colon, type and optional trailing comma.
   static TupleTypeElementSyntax makeTupleTypeElement(
         std::optional<TokenSyntax> label,
         std::optional<TokenSyntax> colon, TypeSyntax type,
         std::optional<TokenSyntax> trailingComma = std::nullopt,
         RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates an unlabelled TupleTypeElementSyntax with the provided type and
   /// optional trailing comma.
   static TupleTypeElementSyntax
   makeTupleTypeElement(TypeSyntax type,
                        std::optional<TokenSyntax> trailingComma = std::nullopt,
                        RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates a TypeIdentifierSyntax with the provided name and leading/trailing
   /// trivia.
   static TypeSyntax makeTypeIdentifier(OwnedString typeName,
                                        const Trivia &leadingTrivia = {},
                                        const Trivia &trailingTrivia = {},
                                        RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates a GenericParameterSyntax with no inheritance clause and an
   /// optional trailing comma.
   static GenericParameterSyntax
   makeGenericParameter(TokenSyntax name,
                        std::optional<TokenSyntax> trailingComma,
                        RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates a TypeIdentifierSyntax for the `Any` type.
   static TypeSyntax makeAnyTypeIdentifier(const Trivia &leadingTrivia = {},
                                           const Trivia &trailingTrivia = {},
                                           RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates a TypeIdentifierSyntax for the `Self` type.
   static TypeSyntax makeSelfTypeIdentifier(const Trivia &leadingTrivia = {},
                                            const Trivia &trailingTrivia = {},
                                            RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates a TokenSyntax for the `type` identifier.
   static TokenSyntax makeTypeToken(const Trivia &leadingTrivia = {},
                                    const Trivia &trailingTrivia = {},
                                    RefCountPtr<SyntaxArena> arena = nullptr);

   /// Creates an `==` operator token.
   static TokenSyntax makeEqualityOperator(const Trivia &leadingTrivia = {},
                                           const Trivia &trailingTrivia = {},
                                           RefCountPtr<SyntaxArena> arena = nullptr);

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
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORIES_H
