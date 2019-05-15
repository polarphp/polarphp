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
#include "polarphp/syntax/internal/ListSyntaxNodeExtraFuncs.h"

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
   std::vector<RefCountPtr<RawSyntax>> layout;
   layout.reserve(elements.size());
   for (auto &e : elements) {
      layout.emplace_back(e.getRaw());
   }
   if (auto raw = createRaw(kind, layout, arena)) {
      return make<Syntax>(raw);
   } else {
      return std::nullopt;
   }
}

RefCountPtr<RawSyntax> AbstractFactory::createRaw(SyntaxKind kind,
                                                  ArrayRef<RefCountPtr<RawSyntax>> elements,
                                                  RefCountPtr<SyntaxArena> arena)
{
   using namespace internal::abstractfactorycreateraw;
   switch (kind) {
   case SyntaxKind::Decl:
      return create_decl_raw(elements, arena);
   default:
      return nullptr;
   }
}

/// Count the number of children for a given syntax node kind,
/// returning a pair of mininum and maximum count of children. The gap
/// between these two numbers is the number of optional children.
std::pair<unsigned, unsigned> AbstractFactory::countChildren(SyntaxKind kind)
{
   bool exist = false;
   auto countPair = retrieve_syntax_kind_child_count(kind, exist);
   if (!exist) {
      polar_unreachable("bad syntax kind.");
   }
   return countPair;
}

Syntax AbstractFactory::makeBlankCollectionSyntax(SyntaxKind kind)
{
   switch(kind) {
   //   case SyntaxKind::CodeBlockItemList: return makeBlankCodeBlockItemList();
   default: break;
   }
   polar_unreachable("not collection kind.");
}

/// Whether a raw node kind `memberKind` can serve as a member in a syntax
/// collection of the given syntax collection kind.
bool AbstractFactory::canServeAsCollectionMemberRaw(SyntaxKind collectionKind,
                                                    SyntaxKind memberKind)
{
   using namespace internal::canserveascollectionmemberraw;
   switch (collectionKind) {
   case SyntaxKind::CodeBlockItemList:
      return check_code_block_item_list(memberKind);
   case SyntaxKind::TokenList:
      return check_token_list(memberKind);
   case SyntaxKind::NonEmptyTokenList:
      return check_non_empty_token_list(memberKind);
   default:
      polar_unreachable("Not collection kind.");
   }
}

/// Whether a raw node `member` can serve as a member in a syntax collection
/// of the given syntax collection kind.
bool AbstractFactory::canServeAsCollectionMemberRaw(SyntaxKind collectionKind,
                                                    const RefCountPtr<RawSyntax> &member)
{
   return canServeAsCollectionMemberRaw(collectionKind, member->getKind());
}

/// Whether a node `member` can serve as a member in a syntax collection of
/// the given syntax collection kind.
bool AbstractFactory::canServeAsCollectionMember(SyntaxKind collectionKind, Syntax member)
{
   return canServeAsCollectionMemberRaw(collectionKind, member.getRaw());
}

} // polar::syntax
