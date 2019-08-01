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
   if (!need_invoke_create_raw_func(kind)) {
      return nullptr;
   }
   switch (kind) {
   case SyntaxKind::CodeBlockItemList:
      return create_code_block_item_list_raw(elements, arena);
   case SyntaxKind::TokenList:
      return create_token_list_raw(elements, arena);
   case SyntaxKind::NonEmptyTokenList:
      return create_non_empty_token_list_raw(elements, arena);
   case SyntaxKind::CodeBlockItem:
      return create_code_block_item_raw(elements, arena);
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

/// make syntax node utils methods
DeclSyntax AbstractFactory::makeBlankDecl(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::Decl, {}, SourcePresence::Present, arena);
   return make<DeclSyntax>(raw);
}

ExprSyntax AbstractFactory::makeBlankExpr(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::Expr, {}, SourcePresence::Present, arena);
   return make<ExprSyntax>(raw);
}

StmtSyntax AbstractFactory::makeBlankStmt(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::Stmt, {}, SourcePresence::Present, arena);
   return make<StmtSyntax>(raw);
}

TypeSyntax AbstractFactory::makeBlankType(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::Type, {}, SourcePresence::Present, arena);
   return make<TypeSyntax>(raw);
}

TokenSyntax AbstractFactory::makeBlankToken(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::Token, {}, SourcePresence::Present, arena);
   return make<TokenSyntax>(raw);
}

UnknownSyntax AbstractFactory::makeBlankUnknown(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::Unknown, {}, SourcePresence::Present, arena);
   return make<UnknownSyntax>(raw);
}

UnknownDeclSyntax AbstractFactory::makeBlankUnknownDecl(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::UnknownDecl, {}, SourcePresence::Present, arena);
   return make<UnknownDeclSyntax>(raw);
}

UnknownExprSyntax AbstractFactory::makeBlankUnknownExpr(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::UnknownExpr, {}, SourcePresence::Present, arena);
   return make<UnknownExprSyntax>(raw);
}

UnknownStmtSyntax AbstractFactory::makeBlankUnknownStmt(RefCountPtr<SyntaxArena> arena)
{
   auto raw = RawSyntax::make(SyntaxKind::UnknownStmt, {}, SourcePresence::Present, arena);
   return make<UnknownStmtSyntax>(raw);
}

} // polar::syntax
