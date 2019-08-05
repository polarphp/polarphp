// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/19.

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodesFwd.h"

namespace polar::syntax {

class ReservedNonModifierSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      Modifier
   };

#ifdef POLAR_DEBUG_BUILD
   ///
   /// child index: Modifier
   /// token choices:
   /// T_INCLUDE | T_INCLUDE_ONCE | T_EVAL | T_REQUIRE | T_REQUIRE_ONCE | T_LOGICAL_OR | T_LOGICAL_XOR | T_LOGICAL_AND
   /// T_INSTANCEOF | T_NEW | T_CLONE | T_EXIT | T_IF | T_ELSEIF | T_ELSE | T_ECHO | T_DO | T_WHILE
   /// T_FOR | T_FOREACH | T_DECLARE | T_AS | T_TRY | T_CATCH | T_FINALLY
   /// T_THROW | T_USE | T_INSTEADOF | T_GLOBAL | T_VAR | T_UNSET | T_ISSET | T_EMPTY | T_CONTINUE | T_GOTO
   /// T_FUNCTION | T_CONST | T_RETURN | T_PRINT | T_YIELD | T_LIST | T_SWITCH | T_CASE | T_DEFAULT | T_BREAK
   /// T_ARRAY | T_CALLABLE | T_EXTENDS | T_IMPLEMENTS | T_NAMESPACE | T_TRAIT | T_INTERFACE | T_CLASS
   /// T_CLASS_CONST | T_TRAIT_CONST | T_FUNC_CONST | T_METHOD_CONST | T_LINE | T_FILE | T_DIR | T_NS_CONST | T_FN
   ///
   static const TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   ReservedNonModifierSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getModifier();
   ReservedNonModifierSyntax withModifier(std::optional<TokenSyntax> modifier);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ReservedNonModifier;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ReservedNonModifierSyntaxBuilder;
   void validate();
};

///
/// semi_reserved:
///   reserved_non_modifiers
/// | T_STATIC | T_ABSTRACT | T_FINAL | T_PRIVATE | T_PROTECTED | T_PUBLIC
///
///
class SemiReservedSytnax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// -----------------
      /// choice type: ReservedNonModifierSyntax
      /// -----------------
      /// choice type: TokenSyntax
      /// token choices: true
      Modifier,
      ModifierChoiceToken
   };

#ifdef POLAR_DEBUG_BUILD
   ///
   /// child index: ModifierChoiceToken
   /// token choices:
   /// T_STATIC | T_ABSTRACT | T_FINAL | T_PRIVATE | T_PROTECTED | T_PUBLIC
   static const TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   SemiReservedSytnax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getModifier();
   SemiReservedSytnax withModifier(std::optional<Syntax> modifier);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SemiReserved;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SemiReservedSytnaxBuilder;
   void validate();
};

///
/// identifier:
///    T_IDENTIFIER_STRING
///  | semi_reserved
///
class IdentifierSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// -----------------
      /// choice type: TokenSyntax
      /// -----------------
      /// choice type: SemiReservedSytnax
      NameItem
   };

public:
   IdentifierSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getNameItem();
   IdentifierSyntax withNameItem(std::optional<Syntax> item);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::Identifier;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class IdentifierSyntaxBuilder;
   void validate();
};

///
/// namespace_name:
///   T_IDENTIFIER_STRING
/// | namespace_name T_NS_SEPARATOR T_IDENTIFIER_STRING
///
class NamespacePartSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: true
      NsSeparator,
      /// type: TokenSyntax
      /// optional: false
      Name
   };

public:
   NamespacePartSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getNsSeparator();
   TokenSyntax getName();
   NamespacePartSyntax withNsSeparator(std::optional<TokenSyntax> separator);
   NamespacePartSyntax withName(std::optional<TokenSyntax> name);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespacePart;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespacePartSyntaxBuilder;
   void validate();
};

///
/// name:
///   namespace_name
/// | T_NAMESPACE T_NS_SEPARATOR namespace_name
/// | T_NS_SEPARATOR namespace_name
///
class NameSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: true
      NsToken,
      /// type: TokenSyntax
      /// optional: true
      NsSeparator,
      /// type: SyntaxCollection
      /// name: NamespacePartListSyntax
      /// optional: false
      Namespace
   };
public:
   NameSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   std::optional<TokenSyntax> getNsToken();
   std::optional<TokenSyntax> getNsSeparator();
   NamespacePartListSyntax getNamespace();
   NameSyntax withNsToken(std::optional<TokenSyntax> nsToken);
   NameSyntax withNsSeparator(std::optional<TokenSyntax> separatorToken);
   NameSyntax withNamespace(std::optional<NamespacePartListSyntax> ns);
   NameSyntax addNamespacePart(NamespacePartSyntax namespacePart);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::Name;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class NameSyntaxBuilder;
   void validate();
};

/// use_type:
///   T_FUNCTION
/// | T_CONST
class NamespaceUseTypeSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      TypeToken
   };
#ifdef POLAR_DEBUG_BUILD
   ///
   /// child index: TypeToken
   /// token choices:
   /// T_FUNCTION | T_CONST
   ///
   static const TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   NamespaceUseTypeSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getTypeToken();
   NamespaceUseTypeSyntax withTypeToken(std::optional<TokenSyntax> typeToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceUseType;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class NamespaceUseTypeSyntaxBuilder;
   void validate();
};

/// unprefixed_use_declaration:
///   namespace_name
/// | namespace_name T_AS T_IDENTIFIER_STRING
///
class NamespaceUnprefixedUseDeclarationSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: SyntaxCollection
      /// optional: false
      Namespace,
      /// type: TokenSyntax
      /// opttional: true
      AsToken,
      /// type: TokenSyntax
      /// optional: true
      IdentifierToken
   };

public:
   NamespaceUnprefixedUseDeclarationSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   NamespacePartListSyntax getNamespace();
   std::optional<TokenSyntax> getAsToken();
   std::optional<TokenSyntax> getIdentifierToken();
   NamespaceUnprefixedUseDeclarationSyntax addNamespacePart(NamespacePartSyntax namespacePart);
   NamespaceUnprefixedUseDeclarationSyntax withNamespace(std::optional<NamespacePartListSyntax> ns);
   NamespaceUnprefixedUseDeclarationSyntax withAsToken(std::optional<TokenSyntax> asToken);
   NamespaceUnprefixedUseDeclarationSyntax withIdentifierToken(std::optional<TokenSyntax> identifierToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceUnprefixedUseDeclaration;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class UnprefixedUseDeclarationSyntaxBuilder;
   void validate();
};

///
/// use_declaration:
///   unprefixed_use_declaration
/// | T_NS_SEPARATOR unprefixed_use_declaration
///
class NamespaceUseDeclarationSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: true
      NsSeparator,
      /// type: NamespaceUnprefixedUseDeclarationSyntax
      /// optional: false
      UnprefixedUseDeclaration
   };

public:
   NamespaceUseDeclarationSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getNsSeparator();
   NamespaceUnprefixedUseDeclarationSyntax getUnprefixedUseDeclaration();
   NamespaceUseDeclarationSyntax withNsSeparator(std::optional<TokenSyntax> nsSeparator);
   NamespaceUseDeclarationSyntax withUnprefixedUseDeclaration(std::optional<NamespaceUnprefixedUseDeclarationSyntax> declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceUseDeclaration;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class UseDeclarationSyntaxBuilder;
   void validate();
};

///
/// inline_use_declaration:
///   unprefixed_use_declaration
/// | use_type unprefixed_use_declaration
///
class NamespaceInlineUseDeclarationSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: NamespaceUseTypeSyntax
      /// optional: true
      UseType,
      /// type: NamespaceUnprefixedUseDeclarationSyntax
      /// optional: false
      UnprefixedUseDeclaration
   };
public:
   NamespaceInlineUseDeclarationSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<NamespaceUseTypeSyntax> getUseType();
   NamespaceUnprefixedUseDeclarationSyntax getUnprefixedUseDeclaration();
   NamespaceInlineUseDeclarationSyntax withUseType(std::optional<NamespaceUseTypeSyntax> useType);
   NamespaceInlineUseDeclarationSyntax withUnprefixedUseDeclaration(std::optional<NamespaceUnprefixedUseDeclarationSyntax> declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceInlineUseDeclaration;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class InlineUseDeclarationSyntaxBuilder;
   void validate();
};

///
/// group_use_declaration:
///   namespace_name T_NS_SEPARATOR '{' unprefixed_use_declarations possible_comma '}'
/// | T_NS_SEPARATOR namespace_name T_NS_SEPARATOR '{' unprefixed_use_declarations possible_comma '}'
///
class NamespaceGroupUseDeclarationSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 7;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      FirstNsSeparator,
      ///
      /// type: NamespacePartListSyntax
      /// optional: false
      ///
      Namespace,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      SecondNsSeparator,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: NamespaceUnprefixedUseDeclarationListSyntax
      /// optional: false
      ///
      UnprefixedUseDeclarations,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      CommaToken,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      RightBrace
   };

public:
   NamespaceGroupUseDeclarationSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getFirstNsSeparator();
   NamespacePartListSyntax getNamespace();
   TokenSyntax getSecondNsSeparator();
   TokenSyntax getLeftBrace();
   NamespaceUnprefixedUseDeclarationListSyntax getUnprefixedUseDeclarations();
   std::optional<TokenSyntax> getCommaToken();
   TokenSyntax getRightBrace();

   NamespaceGroupUseDeclarationSyntax withFirstNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceGroupUseDeclarationSyntax withNamespace(std::optional<NamespacePartListSyntax> ns);
   NamespaceGroupUseDeclarationSyntax withSecondNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceGroupUseDeclarationSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   NamespaceGroupUseDeclarationSyntax withUnprefixedUseDeclarations(std::optional<NamespaceUnprefixedUseDeclarationListSyntax> declarations);
   NamespaceGroupUseDeclarationSyntax withCommaToken(std::optional<TokenSyntax> comma);
   NamespaceGroupUseDeclarationSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);
   NamespaceGroupUseDeclarationSyntax addUnprefixedUseDeclaration(NamespaceUnprefixedUseDeclarationSyntax declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceGroupUseDeclaration;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceGroupUseDeclarationSyntaxBuilder;
   void validate();
};

class NamespaceMixedGroupUseDeclarationSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 7;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      FirstNsSeparator,
      ///
      /// type: NamespacePartListSyntax
      /// optional: false
      ///
      Namespace,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      SecondNsSeparator,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: NamespaceInlineUseDeclarationListSyntax
      /// optional: false
      ///
      InlineUseDeclarations,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      CommaToken,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      RightBrace
   };

public:
   NamespaceMixedGroupUseDeclarationSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getFirstNsSeparator();
   NamespacePartListSyntax getNamespace();
   TokenSyntax getSecondNsSeparator();
   TokenSyntax getLeftBrace();
   NamespaceInlineUseDeclarationListSyntax getInlineUseDeclarations();
   std::optional<TokenSyntax> getCommaToken();
   TokenSyntax getRightBrace();

   NamespaceMixedGroupUseDeclarationSyntax withFirstNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceMixedGroupUseDeclarationSyntax withNamespace(std::optional<NamespacePartListSyntax> ns);
   NamespaceMixedGroupUseDeclarationSyntax withSecondNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceMixedGroupUseDeclarationSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   NamespaceMixedGroupUseDeclarationSyntax withInlineUseDeclarations(std::optional<NamespaceInlineUseDeclarationListSyntax> declarations);
   NamespaceMixedGroupUseDeclarationSyntax withCommaToken(std::optional<TokenSyntax> comma);
   NamespaceMixedGroupUseDeclarationSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   NamespaceMixedGroupUseDeclarationSyntax addInlineUseDeclaration(NamespaceInlineUseDeclarationSyntax declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceMixedGroupUseDeclaration;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceMixedGroupUseDeclarationSyntaxBuilder;
   void validate();
};

///
/// top_statement:
///   T_USE mixed_group_use_declaration ';'
/// |	T_USE use_type group_use_declaration ';'
/// |	T_USE use_declarations ';'
/// |	T_USE use_type use_declarations ';'
///
class NamespaceUseSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      UseToken,
      ///
      /// type: NamespaceUseTypeSyntax
      /// optional: true
      ///
      UseType,
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: false
      /// ------------------
      /// node choice: NamespaceMixedGroupUseDeclarationSyntax
      /// ------------------
      /// node choice: NamespaceGroupUseDeclarationSyntax
      /// ------------------
      /// node choice: NamespaceUseDeclarationListSyntax
      ///
      Declarations,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      SemicolonToken
   };
public:
   NamespaceUseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {}

   TokenSyntax getUseToken();
   std::optional<NamespaceUseTypeSyntax> getUseType();
   Syntax getDeclarations();
   TokenSyntax getSemicolon();

   NamespaceUseSyntax withUseToken(std::optional<TokenSyntax> useToken);
   NamespaceUseSyntax withUseType(std::optional<NamespaceUseTypeSyntax> useType);
   NamespaceUseSyntax withDeclarations(std::optional<Syntax> declarations);
   NamespaceUseSyntax withSemicolonToken(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceUse;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceUseSyntaxBuilder;
   void validate();
};

///
/// = expr
///
class InitializeClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      EqualToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ValueExpr
   };

public:
   InitializeClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {}

   TokenSyntax getEqualToken();
   ExprSyntax getValueExpr();

   InitializeClauseSyntax withEqualToken(std::optional<TokenSyntax> equalToken);
   InitializeClauseSyntax withValueExpr(std::optional<ExprSyntax> valueExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InitializeClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class InitializeClauseSyntaxBuilder;
   void validate();
};

///
/// const_decl:
/// T_IDENTIFIER_STRING '=' expr
///
class ConstDeclareItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Name,
      ///
      /// type: InitializerClauseSyntax
      /// optional: false
      ///
      InitializerClause
   };

public:
   ConstDeclareItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getName();
   InitializeClauseSyntax getInitializer();

   ConstDeclareItemSyntax withName(std::optional<TokenSyntax> name);
   ConstDeclareItemSyntax withIntializer(std::optional<InitializeClauseSyntax> initializer);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConstDeclareItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConstDeclareItemSyntaxBuilder;
   void validate();
};

///
/// top_statement:
///   T_CONST const_list ';'
///
class ConstDefinitionSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ConstToken,
      ///
      /// type: ConstDefinitionListSyntax
      /// optional: false
      ///
      Declarations,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Semicolon
   };

public:
   ConstDefinitionSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getConstToken();
   ConstDeclareItemListSyntax getDeclarations();
   TokenSyntax getSemicolon();

   ConstDefinitionSyntax withConstToken(std::optional<TokenSyntax> constToken);
   ConstDefinitionSyntax withDeclarations(std::optional<ConstDeclareItemListSyntax> declarations);
   ConstDefinitionSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConstDefinition;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConstDefinitionSyntaxBuilder;
   void validate();
};

///
/// type:
///   T_ARRAY
/// | T_CALLABLE
/// | name
///
class TypeClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ------------------
      /// node choice: TokenSyntax
      /// token choices: true
      /// T_ARRAY | T_CALLABLE
      /// ------------------
      /// node choice: NameSyntax
      ///
      Type
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   TypeClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getType();
   TypeClauseSyntax withType(std::optional<Syntax> type);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::TypeClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class TypeClauseSyntaxBuilder;
   void validate();
};

///
/// type_expr:
///   type
/// | '?' type
///
class TypeExprClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      QuestionToken,
      ///
      /// type: TypeClauseSyntax
      /// optional: false
      ///
      TypeClause
   };

public:
   TypeExprClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getQuestionToken();
   TypeClauseSyntax getTypeClause();
   TypeExprClauseSyntax withQuestionToken(std::optional<TokenSyntax> questionToken);
   TypeExprClauseSyntax withType(std::optional<TypeClauseSyntax> type);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::TypeExprClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class TypeExprClauseSyntaxBuilder;
   void validate();
};

///
/// return_type:
///   ':' type_expr
///
class ReturnTypeClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ColonToken,
      ///
      /// type: TypeExprClauseSyntax
      /// optional: false
      ///
      TypeExpr
   };
public:
   ReturnTypeClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getColon();
   TypeExprClauseSyntax getType();
   ReturnTypeClauseSyntax withColon(std::optional<TokenSyntax> colon);
   ReturnTypeClauseSyntax withType(std::optional<TypeExprClauseSyntax> type);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ReturnTypeClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ReturnTypeClauseSyntaxBuilder;
   void validate();
};

///
/// parameter:
///   optional_type is_reference is_variadic T_VARIABLE
/// | optional_type is_reference is_variadic T_VARIABLE '=' expr
///
class ParameterSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 5;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TypeExprClauseSyntax
      /// optional: true
      ///
      TypeHint,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      ReferenceMark,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      VariadicMark,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Variable,
      ///
      /// type: InitializeClauseSyntax
      /// optional: true
      ///
      Initializer
   };

public:
   ParameterSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TypeExprClauseSyntax> getTypeHint();
   std::optional<TokenSyntax> getReferenceMark();
   std::optional<TokenSyntax> getVariadicMark();
   TokenSyntax getVariable();
   std::optional<InitializeClauseSyntax> getInitializer();

   ParameterSyntax withTypeHint(std::optional<TypeExprClauseSyntax> typeHint);
   ParameterSyntax withReferenceMark(std::optional<TokenSyntax> referenceMark);
   ParameterSyntax withVariadicMark(std::optional<TokenSyntax> variadicMark);
   ParameterSyntax withVariable(std::optional<TokenSyntax> variable);
   ParameterSyntax withInitializer(std::optional<InitializeClauseSyntax> initializer);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ParameterItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParameterSyntaxBuilder;
   void validate();
};

///
/// parameter_clause:
///   '(' parameter_list ')'
///
class ParameterClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ParameterListSyntax
      /// optional: false
      ///
      Parameters,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      RightParen
   };

public:
   ParameterClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftParen();
   ParameterListSyntax getParameters();
   TokenSyntax getRightParen();

   ParameterClauseSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   ParameterClauseSyntax withParameters(std::optional<ParameterListSyntax> parameters);
   ParameterClauseSyntax withRightParen(std::optional<TokenSyntax> rightParen);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ParameterClauseSyntax;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class FunctionDefinitionSyntaxBuilder;
   void validate();
};

///
/// function_declaration_statement:
///   function returns_ref T_STRING backup_doc_comment '(' parameter_list ')' return_type
///   backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
///
class FunctionDefinitionSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 10;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      FuncToken,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      ReturnRefFlagToken,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      FuncName,
      ///
      /// type: ParameterClauseSyntax
      /// optional: false
      ///
      ParameterClause,
      ///
      /// type: ReturnTypeClauseSyntax
      /// optional: true
      ///
      ReturnType,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body
   };

public:
   FunctionDefinitionSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getFuncToken();
   std::optional<TokenSyntax> getReturnRefFlagToken();
   TokenSyntax getFuncName();
   ParameterClauseSyntax getParameterClause();
   std::optional<TokenSyntax> getReturnType();
   CodeBlockSyntax getBody();

   FunctionDefinitionSyntax withFuncToken(std::optional<TokenSyntax> funcToken);
   FunctionDefinitionSyntax withReturnRefFlagToken(std::optional<TokenSyntax> returnRefFlagToken);
   FunctionDefinitionSyntax withFuncName(std::optional<TokenSyntax> funcName);
   FunctionDefinitionSyntax withParameterClause(std::optional<ParameterClauseSyntax> parameterClause);
   FunctionDefinitionSyntax withReturnType(std::optional<TokenSyntax> returnType);
   FunctionDefinitionSyntax withBody(std::optional<CodeBlockSyntax> body);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::FunctionDefinition;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class FunctionDefinitionSyntaxBuilder;
   void validate();
};

///
/// class_modifier:
///   T_ABSTRACT
/// | T_FINAL
///
class ClassModifierSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// -------------------------
      /// token choice: T_ABSTRACT
      /// -------------------------
      /// token choice: T_FINAL
      ///
      Modifier
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif
public:
   ClassModifierSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getModifier();
   ClassModifierSyntax withModifier(std::optional<TokenSyntax> modifier);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassModifier;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassModifierSyntaxBuilder;
   void validate();
};

///
/// extends_from:
///   T_EXTENDS name
///
class ExtendsFromClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ExtendToken,
      ///
      /// type: NameSyntax
      /// optional: false
      ///
      Name
   };

public:
   ExtendsFromClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getExtendToken();
   NameSyntax getName();

   ExtendsFromClauseSyntax withExtendToken(std::optional<TokenSyntax> extendToken);
   ExtendsFromClauseSyntax withName(std::optional<NameSyntax> name);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ExtendsFromClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ExtendsFromClauseSyntaxBuilder;
   void validate();
};

///
/// implements_list:
///   T_IMPLEMENTS name_list
///
class ImplementClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ImplementToken,
      ///
      /// type: NameListSyntax
      /// optional: false
      ///
      Interfaces
   };

public:
   ImplementClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getImplementToken();
   NameListSyntax getInterfaces();

   ImplementClauseSyntax withImplementToken(std::optional<TokenSyntax> implementToken);
   ImplementClauseSyntax withInterfaces(std::optional<NameListSyntax> interfaces);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ImplementsClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ImplementClauseSyntaxBuilder;
   void validate();
};

///
/// interface_extends_list:
///   T_EXTENDS name_list
///
class InterfaceExtendsClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ExtendsToken,
      ///
      /// type: NameListSyntax
      /// optional: false
      ///
      Interfaces
   };

public:
   InterfaceExtendsClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getExtendsToken();
   NameListSyntax getInterfaces();

   InterfaceExtendsClauseSyntax withExtendsToken(std::optional<TokenSyntax> extendsToken);
   InterfaceExtendsClauseSyntax withInterfaces(std::optional<NameListSyntax> interfaces);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InterfaceExtendsClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ImplementClauseSyntaxBuilder;
   void validate();
};

///
/// property:
///   T_VARIABLE backup_doc_comment
/// | T_VARIABLE '=' expr backup_doc_comment
///
class ClassPropertyClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Variable,
      ///
      /// type: InitializeClauseSyntax
      /// optional: true
      ///
      Initializer
   };

public:
   ClassPropertyClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getVariable();
   std::optional<InitializeClauseSyntax> getInitializer();
   ClassPropertyClauseSyntax withVariable(std::optional<TokenSyntax> variable);
   ClassPropertyClauseSyntax withInitializer(std::optional<InitializeClauseSyntax> initializer);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassPropertyClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ClassPropertyClauseSyntaxBuilder;
   void validate();
};

///
/// class_const_decl:
///   identifier '=' expr backup_doc_comment
///
class ClassConstClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: IdentifierSyntax
      /// optional: false
      ///
      Identifier,
      ///
      /// type: InitializeClauseSyntax
      /// optional: true
      ///
      Initializer
   };

public:
   ClassConstClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   IdentifierSyntax getIdentifier();
   std::optional<InitializeClauseSyntax> getInitializer();
   ClassConstClauseSyntax withIdentifier(std::optional<IdentifierSyntax> identifier);
   ClassConstClauseSyntax withInitializer(std::optional<InitializeClauseSyntax> initializer);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassConstClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassConstClauseSyntaxBuilder;
   void validate();
};

///
/// member_modifier:
///   T_PUBLIC
/// | T_PROTECTED
/// | T_PRIVATE
/// | T_STATIC
/// | T_ABSTRACT
/// | T_FINAL
///
class MemberModifierSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// --------------------
      /// T_PUBLIC | T_PROTECTED | T_PRIVATE
      /// T_STATIC | T_ABSTRACT  | T_FINAL
      ///
      Modifier
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   MemberModifierSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getModifier();
   MemberModifierSyntax withModifier(std::optional<TokenSyntax> modifier);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::MemberModifier;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class MemberModifierSyntaxBuilder;
   void validate();
};

///
/// class_statement:
///    member_modifiers optional_type property_list
///
class ClassPropertyDeclSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: MemberModifierListSyntax
      /// optional: false
      ///
      Modifiers,
      ///
      /// type: TypeExprClauseSyntax
      /// optional: true
      ///
      TypeHint,
      ///
      /// type: ClassPropertyListSyntax
      /// optional: false
      ///
      PropertyList
   };

public:
   ClassPropertyDeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   MemberModifierListSyntax getModifiers();
   std::optional<TypeExprClauseSyntax> getTypeHint();
   ClassPropertyListSyntax getPropertyList();

   ClassPropertyDeclSyntax withModifiers(std::optional<MemberModifierListSyntax> modifiers);
   ClassPropertyDeclSyntax withTypeHint(std::optional<TypeExprClauseSyntax> typeHint);
   ClassPropertyDeclSyntax withPropertyList(std::optional<ClassPropertyListSyntax> propertyList);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassPropertyDecl;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class MemberDeclListItemSyntaxBuilder;
   void validate();
};

///
/// class_statement:
///   member_modifiers T_CONST class_const_list
///
class ClassConstDeclSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: MemberModifierListSyntax
      /// optional: false
      ///
      Modifiers,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ConstToken,
      ///
      /// type: ClassConstListSyntax
      /// optional: false
      ///
      ConstList
   };

public:
   ClassConstDeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   MemberModifierListSyntax getModifiers();
   std::optional<TokenSyntax> getConstToken();
   ClassPropertyListSyntax getConstList();

   ClassConstDeclSyntax withModifiers(std::optional<MemberModifierListSyntax> modifiers);
   ClassConstDeclSyntax withConstToken(std::optional<TokenSyntax> constToken);
   ClassConstDeclSyntax withConstList(std::optional<ClassConstListSyntax> constList);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassConstDecl;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassConstDeclSyntaxBuilder;
   void validate();
};

///
/// class_statement:
///   method_modifiers function returns_ref identifier backup_doc_comment '(' parameter_list ')'
///   return_type backup_fn_flags method_body backup_fn_flags
///
class ClassMethodDeclSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 7;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: MemberModifierListSyntax
      /// optional: false
      ///
      Modifiers,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      FunctionToken,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      ReturnRefToken,
      ///
      /// type: IdentifierSyntax
      /// optional: false
      ///
      FuncName,
      ///
      /// type: ParameterClauseSyntax
      /// optional: false
      ///
      ParameterClause,
      ///
      /// type: ReturnTypeClauseSyntax
      /// optional: true
      ///
      ReturnType,
      ///
      /// type: MemberDeclBlockSyntax
      /// optional: true
      ///
      Body
   };

public:
   ClassMethodDeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   MemberModifierListSyntax getModifiers();
   TokenSyntax getFunctionToken();
   std::optional<TokenSyntax> getReturnRefToken();
   IdentifierSyntax getFuncName();
   ParameterClauseSyntax getParameterClause();
   std::optional<ReturnTypeClauseSyntax> getReturnType();
   std::optional<MemberDeclBlockSyntax> getBody();

   ClassMethodDeclSyntax withModifiers(std::optional<MemberModifierListSyntax> modifiers);
   ClassMethodDeclSyntax withFunctionToken(std::optional<TokenSyntax> functionToken);
   ClassMethodDeclSyntax withReturnRefToken(std::optional<TokenSyntax> returnRefToken);
   ClassMethodDeclSyntax withFuncName(std::optional<IdentifierSyntax> funcName);
   ClassMethodDeclSyntax withParameterClause(std::optional<ParameterClauseSyntax> parameterClause);
   ClassMethodDeclSyntax withReturnType(std::optional<ReturnTypeClauseSyntax> returnType);
   ClassMethodDeclSyntax withBody(std::optional<MemberDeclBlockSyntax> body);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassMethodDecl;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassMethodDeclSyntaxBuilder;
   void validate();
};

///
/// trait_method_reference:
///   identifier
/// | absolute_trait_method_reference
///
class ClassTraitMethodReferenceSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ------------------------------
      /// node choice: IdentifierSyntax
      /// ------------------------------
      /// node choice: ClassAbsoluteTraitMethodReferenceSyntax
      ///
      Reference
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ClassTraitMethodReferenceSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getReference();
   ClassTraitMethodReferenceSyntax withReference(std::optional<Syntax> reference);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassTraitMethodReference;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassTraitMethodReferenceSyntaxBuilder;
   void validate();
};

///
/// absolute_trait_method_reference:
///   name T_PAAMAYIM_NEKUDOTAYIM identifier
///
class ClassAbsoluteTraitMethodReferenceSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: NameSyntax
      /// optional: false
      ///
      BaseName,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Separator,
      ///
      /// type: IdentifierSyntax
      /// optional: false
      ///
      MemberName
   };

public:
   ClassAbsoluteTraitMethodReferenceSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   NameSyntax getBaseName();
   TokenSyntax getSeparator();
   IdentifierSyntax getMemberName();

   ClassAbsoluteTraitMethodReferenceSyntax withBaseName(std::optional<NameSyntax> baseName);
   ClassAbsoluteTraitMethodReferenceSyntax withSeparator(std::optional<TokenSyntax> separator);
   ClassAbsoluteTraitMethodReferenceSyntax withMemberName(std::optional<IdentifierSyntax> memberName);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassAbsoluteTraitMethodReference;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassAbsoluteTraitMethodReferenceSyntaxBuilder;
   void validate();
};

///
/// trait_precedence:
///    absolute_trait_method_reference T_INSTEADOF name_list
///
class ClassTraitPrecedenceSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ClassAbsoluteTraitMethodReferenceSyntax
      /// optional: false
      ///
      MethodReference,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      InsteadOfToken,
      ///
      /// type: NameListSyntax
      /// optional: false
      ///
      Names
   };

public:
   ClassTraitPrecedenceSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ClassAbsoluteTraitMethodReferenceSyntax getMethodReference();
   TokenSyntax getInsteadOfToken();
   NameListSyntax getNames();

   ClassTraitPrecedenceSyntax withMethodReference(std::optional<ClassAbsoluteTraitMethodReferenceSyntax> methodReference);
   ClassTraitPrecedenceSyntax withInsteadOfToken(std::optional<TokenSyntax> insteadOfToken);
   ClassTraitPrecedenceSyntax withNames(std::optional<NameListSyntax> names);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassTraitPrecedence;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassTraitPrecedenceSyntaxBuilder;
   void validate();
};

///
/// trait_alias:
///   trait_method_reference T_AS T_IDENTIFIER_STRING
/// | trait_method_reference T_AS reserved_non_modifiers
/// | trait_method_reference T_AS member_modifier identifier
/// | trait_method_reference T_AS member_modifier
///
class ClassTraitAliasSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ClassTraitMethodReferenceSyntax
      /// optional: false
      ///
      MethodReference,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      AsToken,
      ///
      /// type: Syntax
      /// optional: true
      /// node choices: true
      /// --------------------------------------
      /// node choice: ReservedNonModifierSyntax
      /// --------------------------------------
      /// node choice: MemberModifierSyntax
      ///
      Modifier,
      ///
      /// type: Syntax
      /// optional: true
      /// node choices: true
      /// ------------------------------
      /// node choice: TokenSyntax
      /// ------------------------------
      /// node choice: IdentifierSyntax
      ///
      AliasName
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ClassTraitAliasSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ClassTraitMethodReferenceSyntax getMethodReference();
   TokenSyntax getAsToken();
   std::optional<Syntax> getModifier();
   std::optional<Syntax> getAliasName();

   ClassTraitAliasSyntax withMethodReference(std::optional<ClassTraitMethodReferenceSyntax> methodReference);
   ClassTraitAliasSyntax withAsToken(std::optional<TokenSyntax> asToken);
   ClassTraitAliasSyntax withModifier(std::optional<Syntax> modifier);
   ClassTraitAliasSyntax withAliasName(std::optional<Syntax> aliasName);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassTraitAlias;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassTraitAliasSyntaxBuilder;
   void validate();
};

///
/// trait_adaptation:
///   trait_precedence ';'
/// | trait_alias ';'
///
class ClassTraitAdaptationSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optiona: false
      /// node choices: true
      /// -----------------------------------------
      /// node choice: ClassTraitPrecedenceSyntax
      /// -----------------------------------------
      /// node choice: ClassTraitAliasSyntax
      ///
      Adaptation,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Semicolon
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif
public:
   ClassTraitAdaptationSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getAdaptation();
   TokenSyntax getSemicolon();

   ClassTraitAdaptationSyntax withAdaptation(std::optional<Syntax> adaptation);
   ClassTraitAdaptationSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassTraitAdaptation;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassTraitAdaptationSyntaxBuilder;
   void validate();
};

///
/// trait_adaptations:
/// ';'
/// | '{' '}'
/// | '{' trait_adaptation_list '}'
///
class ClassTraitAdaptationBlockSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: ClassTraitAdaptationListSyntax
      /// optional: false
      ///
      AdaptationList,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      RightBrace
   };

public:
   ClassTraitAdaptationBlockSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftBrace();
   ClassTraitAdaptationListSyntax getAdaptaionList();
   TokenSyntax getRightBrace();

   ClassTraitAdaptationBlockSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   ClassTraitAdaptationBlockSyntax withAdaptationList(std::optional<ClassTraitAdaptationListSyntax> adaptaionList);
   ClassTraitAdaptationBlockSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassTraitAdaptationBlock;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassTraitAdaptationBlockSyntaxBuilder;
   void validate();
};

///
/// class_statement:
///   T_USE name_list trait_adaptations
///
class ClassTraitDeclSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      UseToken,
      ///
      /// type: NameListSyntax
      /// optional: false
      ///
      NameList,
      ///
      /// type: ClassTraitAdaptationBlockSyntax
      /// optional: true
      ///
      AdaptationBlock
   };

public:
   ClassTraitDeclSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getUseToken();
   NameListSyntax getNameList();
   std::optional<ClassTraitAdaptationBlockSyntax> getAdaptationBlock();

   ClassTraitDeclSyntax withUseToken(std::optional<TokenSyntax> useToken);
   ClassTraitDeclSyntax withNameList(std::optional<NameListSyntax> nameList);
   ClassTraitDeclSyntax withAdaptationBlock(std::optional<ClassTraitAdaptationBlockSyntax> adaptationBlock);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassTraitDecl;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassTraitDeclSyntaxBuilder;
   void validate();
};

///
/// member-decl:
///   decl ';'?
///
class MemberDeclListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: DeclSyntax
      /// optional: false
      ///
      Decl,
      ///
      /// type: TokenSyntax
      /// optional: true
      ///
      Semicolon
   };

public:
   MemberDeclListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   DeclSyntax getDecl();
   TokenSyntax getSemicolon();
   MemberDeclListItemSyntax withDecl(std::optional<DeclSyntax> decl);
   MemberDeclListItemSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::MemberDeclListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class MemberDeclListItemSyntaxBuilder;
   void validate();
};

///
/// member_decl_block:
/// '{' class_statement_list '}'
///
class MemberDeclBlockSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional；false
      ///
      LeftBrace,
      ///
      /// type: MemberDeclListSyntax
      /// optional: false
      ///
      Members,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      RightBrace
   };

public:
   MemberDeclBlockSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftBrace();
   MemberDeclListSyntax getMembers();
   TokenSyntax getRightBrace();

   MemberDeclBlockSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   MemberDeclBlockSyntax withMembers(std::optional<MemberDeclListSyntax> members);
   MemberDeclBlockSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::MemberDeclBlock;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class MemberDeclListItemSyntaxBuilder;
   void validate();
};

///
/// class_declaration_statement:
///   class_modifiers T_CLASS T_STRING extends_from implements_list backup_doc_comment '{' class_statement_list '}'
/// | T_CLASS T_STRING extends_from implements_list backup_doc_comment '{' class_statement_list '}'
///
class ClassDefinitionSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 9;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 8;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ClassModififerListSyntax
      /// optional: true
      ///
      Modififers,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      ClassToken,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Name,
      ///
      /// type: ExtendsFromClauseSyntax
      /// optional: false
      ///
      ExtendsFrom,
      ///
      /// type: ImplementClauseSyntax
      /// optional: false
      ///
      ImplementsList,
      ///
      /// type: MemberDeclBlockSyntax
      /// optional: false
      ///
      Members
   };

public:
   ClassDefinitionSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   std::optional<ClassModififerListSyntax> getModififers();
   TokenSyntax getClassToken();
   TokenSyntax getName();
   ExtendsFromClauseSyntax getExtendsFrom();
   ImplementClauseSyntax getImplementsList();
   MemberDeclBlockSyntax getMembers();

   ClassDefinitionSyntax withModifiers(std::optional<ClassModififerListSyntax> modifiers);
   ClassDefinitionSyntax withClassToken(std::optional<TokenSyntax> classToken);
   ClassDefinitionSyntax withName(std::optional<TokenSyntax> name);
   ClassDefinitionSyntax withExtendsFrom(std::optional<ExtendsFromClauseSyntax> extends);
   ClassDefinitionSyntax withImplementsList(std::optional<ImplementClauseSyntax> implements);
   ClassDefinitionSyntax withMembers(std::optional<MemberDeclBlockSyntax> members);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassDefinition;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassDefinitionSyntaxBuilder;
   void validate();
};

///
/// interface_declaration_statement:
///   T_INTERFACE T_IDENTIFIER_STRING interface_extends_list backup_doc_comment '{' class_statement_list '}'
///
class InterfaceDefinitionSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 6;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      InterfaceToken,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Name,
      ///
      /// type: InterfaceExtendsClauseSyntax
      /// optional: false
      ///
      ExtendsFrom,
      ///
      /// type: MemberDeclBlockSyntax
      /// optional: false
      ///
      Members
   };

public:
   InterfaceDefinitionSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getInterfaceToken();
   TokenSyntax getName();
   InterfaceExtendsClauseSyntax getExtendsFrom();
   MemberDeclBlockSyntax getMembers();

   InterfaceDefinitionSyntax withInterfaceToken(std::optional<TokenSyntax> interfaceToken);
   InterfaceDefinitionSyntax withName(std::optional<TokenSyntax> name);
   InterfaceDefinitionSyntax withExtendsFrom(std::optional<InterfaceExtendsClauseSyntax> extendsFrom);
   InterfaceDefinitionSyntax withMembers(std::optional<MemberDeclBlockSyntax> members);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InterfaceDefinition;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class InterfaceDefinitionSyntaxBuilder;
   void validate();
};

///
/// trait_declaration_statement:
/// T_TRAIT T_IDENTIFIER_STRING backup_doc_comment '{' class_statement_list '}'
///
class TraitDefinitionSyntax final : public DeclSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 5;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      TraitToken,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Name,
      ///
      /// type: MemberDeclBlockSyntax
      /// optional: false
      ///
      Members
   };

public:
   TraitDefinitionSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : DeclSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getTraitToken();
   TokenSyntax getName();
   MemberDeclBlockSyntax getMembers();

   TraitDefinitionSyntax withTraitToken(std::optional<TokenSyntax> traitToken);
   TraitDefinitionSyntax withName(std::optional<TokenSyntax> name);
   TraitDefinitionSyntax withMembers(std::optional<MemberDeclBlockSyntax> members);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::TraitDefinition;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class TraitDefinitionSyntaxBuilder;
   void validate();
};

class SourceFileSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: CodeBlockItemListSyntax
      /// optional: false
      Statements,
      /// type: TokenSyntax
      /// optional: false
      EOFToken
   };

public:
   SourceFileSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getEofToken();
   CodeBlockItemListSyntax getStatements();
   SourceFileSyntax withStatements(std::optional<CodeBlockItemListSyntax> statements);
   SourceFileSyntax addStatement(CodeBlockItemSyntax statement);
   SourceFileSyntax withEofToken(std::optional<TokenSyntax> eofToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SourceFile;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SourceFileSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_H
