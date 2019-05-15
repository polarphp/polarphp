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

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"

namespace polar::syntax {

/// common syntex nodes
/// normal node
class DeclSyntax;
class ExprSyntax;
class StmtSyntax;
class TypeSyntax;
class UnknownDeclSyntax;
class UnknownExprSyntax;
class UnknownStmtSyntax;
class UnknowTypeSyntax;
class CodeBlockItemSyntax;
class CodeBlockSyntax;

/// type: SyntaxCollection
/// element type: CodeBlockItem
using CodeBlockItemListSyntax = SyntaxCollection<SyntaxKind::CodeBlockItemList, CodeBlockItemSyntax>;
/// type: SyntaxCollection
/// element type: Token
using TokenListSyntax = SyntaxCollection<SyntaxKind::TokenList, TokenSyntax>;
/// type: SyntaxCollection
/// element type: Token
using NonEmptyTokenListSyntax = SyntaxCollection<SyntaxKind::NonEmptyTokenList, TokenSyntax>;

class DeclSyntax : public Syntax
{
public:
   DeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return is_decl_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class StmtSyntax : public Syntax
{
public:
   StmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return is_stmt_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class ExprSyntax : public Syntax
{
public:
   ExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return is_expr_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class TypeSyntax : public Syntax
{
public:
   TypeSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return is_type_kind(kind);
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class UnknownDeclSyntax final : public DeclSyntax
{
public:
   UnknownDeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownDecl == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class UnknownExprSyntax final : public ExprSyntax
{
public:
   UnknownExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownExpr == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class UnknownStmtSyntax final : public StmtSyntax
{
public:
   UnknownStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

class UnknownTypeSyntax final : public TypeSyntax
{
public:
   UnknownTypeSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : TypeSyntax(root, data)
   {}

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnknownType == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
};

/// A CodeBlockItem is any Syntax node that appears on its own line inside
/// a CodeBlock.
class CodeBlockItemSyntax final : public Syntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 3;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 2;
public:
   enum Cursor : uint32_t
   {
      /// type: Syntax
      /// optional: false
      /// ------------
      /// node choices
      /// name: Decl kind: Decl
      /// name: Stmt kind: Stmt
      /// name: TokenList kind: TokenList
      /// name: NonEmptyTokenList kind: NonEmptyTokenList
      Item,
      /// type: TokenSyntax
      /// optional: false
      Semicolon,
      /// type: Syntax
      /// optional: true
      ErrorTokens
   };

#ifdef POLAR_DEBUG_BUILD
   static const std::set<SyntaxKind> CHILD_NODE_CHOICES;
#endif

public:
   CodeBlockItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      this->validate();
   }

   /// The underlying node inside the code block.
   Syntax getItem();
   TokenSyntax getSemicolon();
   std::optional<Syntax> getErrorTokens();

   /// Returns a copy of the receiver with its `${child.name}` replaced.
   /// - param newChild: The new `Item` to replace the node's
   ///                   current `Item`, if present.
   CodeBlockItemSyntax withItem(std::optional<Syntax> item);

   /// the trailing semicolon at the end of the item.
   CodeBlockItemSyntax withSemicolon(std::optional<TokenSyntax> semicolon);
   CodeBlockItemSyntax withErrorTokens(std::optional<Syntax> errorTokens);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::CodeBlockItem == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CodeBlockItemSyntaxBuilder;
   void validate();
};

class CodeBlockSyntax final : public Syntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 3;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 3;

public:
   enum Cursor : uint32_t
   {
      /// type: TokenSyntax
      /// optional: false
      LeftBrace,
      /// type: CodeBlockItemListSyntax
      /// optional: false
      Statements,
      /// type: TokenSyntax
      /// optional: false
      RightBrace
   };

public:
   CodeBlockSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      this->validate();
   }

   TokenSyntax getLeftBrace();
   TokenSyntax getRightBrace();
   CodeBlockItemListSyntax getStatements();

   /// Adds the provided `CodeBlockItem` to the node's `Statements`
   /// collection.
   /// - param element: The new `CodeBlockItem` to add to the node's
   ///                  `Statements` collection.
   /// - returns: A copy of the receiver with the provided `CodeBlockItem
   ///            appended to its `Statements` collection.
   CodeBlockSyntax addCodeBlockItem(CodeBlockItemSyntax codeBlockItem);
   CodeBlockSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   CodeBlockSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);
   CodeBlockSyntax withStatements(std::optional<CodeBlockItemListSyntax> statements);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::CodeBlock == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CodeBlockSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_COMMON_NODES_H
