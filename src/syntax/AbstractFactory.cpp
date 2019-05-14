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

#include "polarphp/syntax/AbstractFactory.h"

namespace polar::syntax {

/// Make any kind of token.
TokenSyntax AbstractFactory::makeToken(TokenKindType kind,
                                       OwnedString text, const Trivia &leadingTrivia,
                                       const Trivia &trailingTrivia,
                                       SourcePresence presence,
                                       RefCountPtr<SyntaxArena> arena)
{
   return make<TokenSyntax>(RawSyntax::make(kind, text, leadingTrivia.pieces, trailingTrivia.pieces,
                                            presence, arena));
}

/// Collect a list of tokens into a piece of "unknown" syntax.
UnknownSyntax AbstractFactory::makeUnknownSyntax(ArrayRef<TokenSyntax> tokens,
                                                 RefCountPtr<SyntaxArena> arena)
{
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(tokens.size());
   for (auto &token : tokens) {
      layout.push_back(token.getRaw());
   }
   auto raw = RawSyntax::make(SyntaxKind::Unknown, layout,
                              SourcePresence::Present, arena);
   return make<UnknownSyntax>(raw);
}

std::optional<Syntax> AbstractFactory::createSyntax(SyntaxKind kind,
                                                    ArrayRef<Syntax> elements,
                                                    RefCountPtr<SyntaxArena> arena)
{

}

RefCountPtr<RawSyntax> AbstractFactory::createRaw(SyntaxKind kind,
                                                  ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                  RefCountPtr<SyntaxArena> arena)
{

}

/// Count the number of children for a given syntax node kind,
/// returning a pair of mininum and maximum count of children. The gap
/// between these two numbers is the number of optional children.
std::pair<unsigned, unsigned> AbstractFactory::countChildren(SyntaxKind kind)
{
   switch(kind) {
//   case SyntaxKind:::
   default:
   polar_unreachable("bad syntax kind.");
   }
}

Syntax AbstractFactory::makeBlankCollectionSyntax(SyntaxKind kind)
{
   switch(kind) {
//   case SyntaxKind::CodeBlockItemList: return makeBlankCodeBlockItemList();
   default: break;
   }
   polar_unreachable("not collection kind.");
}

} // polar::syntax
