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
#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"

namespace polar::syntax {

class ReservedNonModifierSyntax;
class SemiReservedSytnax;
class IdentifierSyntax;
class NamespacePartSyntax;
class NameSyntax;
class NamespaceUseTypeSyntax;
class NamespaceUnprefixedUseDeclarationSyntax;
class NamespaceUseDeclarationSyntax;
class NamespaceInlineUseDeclarationSyntax;
class SourceFileSyntax;

///
/// type: SyntaxCollection
/// element type: TokenSyntax
///
/// namespace_name:
///   T_IDENTIFIER_STRING
/// | namespace_name T_NS_SEPARATOR T_IDENTIFIER_STRING
///
using NamespacePartListSyntax = SyntaxCollection<SyntaxKind::NamespacePartList, NamespacePartSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceUseDeclarationSyntax
///
/// use_declarations:
///   use_declarations ',' use_declaration
/// | use_declaration
///
using NamespaceUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceUseDeclarationList, NamespaceUseDeclarationSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceInlineUseDeclarationSyntax
///
/// inline_use_declarations:
///   inline_use_declarations ',' inline_use_declaration
/// | inline_use_declaration
///
using NamespaceInlineUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceInlineUseDeclarationList, NamespaceInlineUseDeclarationSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceInlineUseDeclarationSyntax
///
/// unprefixed_use_declarations:
///   unprefixed_use_declarations ',' unprefixed_use_declaration
/// | unprefixed_use_declaration
///
using NamespaceUnprefixedUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceUnprefixedUseDeclarationList, NamespaceUnprefixedUseDeclarationSyntax>;

class ReservedNonModifierSyntax : public Syntax
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
   static const std::map<SyntaxChildrenCountType, std::set<TokenKindType>> CHILD_TOKEN_CHOICES;
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
class SemiReservedSytnax : public Syntax
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
   static const std::map<SyntaxChildrenCountType, std::set<TokenKindType>> CHILD_TOKEN_CHOICES;
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
class IdentifierSyntax : public Syntax
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
class NamespacePartSyntax : public Syntax
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
class NameSyntax : public Syntax
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
class NamespaceUseTypeSyntax : public Syntax
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
   static const std::map<SyntaxChildrenCountType, std::set<TokenKindType>> CHILD_TOKEN_CHOICES;
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

   static bool classOf(const Syntax *synax)
   {
      return kindOf(synax->getKind());
   }

private:
   friend class NamespaceUseTypeSyntaxBuilder;
   void validate();
};

/// unprefixed_use_declaration:
///   namespace_name
/// | namespace_name T_AS T_IDENTIFIER_STRING
///
class NamespaceUnprefixedUseDeclarationSyntax : public Syntax
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
      return kind == SyntaxKind::NamespaceUseType;
   }

   static bool classOf(const Syntax *synax)
   {
      return kindOf(synax->getKind());
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
class NamespaceUseDeclarationSyntax : public Syntax
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
      return kind == SyntaxKind::NamespaceUseType;
   }

   static bool classOf(const Syntax *synax)
   {
      return kindOf(synax->getKind());
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
class NamespaceInlineUseDeclarationSyntax : public Syntax
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
      return kind == SyntaxKind::NamespaceUseType;
   }

   static bool classOf(const Syntax *synax)
   {
      return kindOf(synax->getKind());
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
class NamespaceGroupUseDeclarationSyntax : public Syntax
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

private:
   friend class NamespaceGroupUseDeclarationSyntaxBuilder;
   void validate();
};

class NamespaceMixedGroupUseDeclarationSyntax : public Syntax
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
   NamespaceUnprefixedUseDeclarationListSyntax getInlineUseDeclarations();
   std::optional<TokenSyntax> getCommaToken();
   TokenSyntax getRightBrace();

   NamespaceGroupUseDeclarationSyntax withFirstNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceGroupUseDeclarationSyntax withNamespace(std::optional<NamespacePartListSyntax> ns);
   NamespaceGroupUseDeclarationSyntax withSecondNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceGroupUseDeclarationSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   NamespaceGroupUseDeclarationSyntax withInlineUseDeclarations(std::optional<NamespaceInlineUseDeclarationListSyntax> declarations);
   NamespaceGroupUseDeclarationSyntax withCommaToken(std::optional<TokenSyntax> comma);
   NamespaceGroupUseDeclarationSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   NamespaceGroupUseDeclarationSyntax addInlineUseDeclaration(NamespaceInlineUseDeclarationSyntax declaration);

private:
   friend class NamespaceMixedGroupUseDeclarationSyntaxBuilder;
   void validate();
};

class SourceFileSyntax : public Syntax
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

private:
   friend class SourceFileSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_H
