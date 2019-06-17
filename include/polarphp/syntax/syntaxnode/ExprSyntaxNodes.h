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

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"

namespace polar::syntax {

class NullExprSyntax;
class ClassRefParentExprSyntax;
class ClassRefStaticExprSyntax;
class ClassRefSelfExprSyntax;
class IntegerLiteralExprSyntax;
class FloatLiteralExprSyntax;
class StringLiteralExprSyntax;
class BooleanLiteralExprSyntax;
class TernaryExprSyntax;
class AssignmentExprSyntax;
class SequenceExprSyntax;
class PrefixOperatorExprSyntax;
class PostfixOperatorExprSyntax;
class BinaryOperatorExprSyntax;

/// type: SyntaxCollection
/// element type: ExprSyntax
using ExprListSyntax = SyntaxCollection<SyntaxKind::ExprList, ExprSyntax>;

class NullExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      NullKeyword,
   };
public:
   NullExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getNullKeyword();
   NullExprSyntax withNullKeyword(std::optional<TokenSyntax> keyword);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NullExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class NullExprSyntaxBuilder;
   void validate();
};

class ClassRefParentExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      ParentKeyword,
   };

public:
   ClassRefParentExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getParentKeyword();
   ClassRefParentExprSyntax withParentKeyword(std::optional<TokenSyntax> parentKeyword);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassRefParentExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassRefParentExprBuilder;
   void validate();
};

class ClassRefSelfExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      SelfKeyword,
   };

public:
   ClassRefSelfExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getSelfKeyword();
   ClassRefSelfExprSyntax withSelfKeyword(std::optional<TokenSyntax> selfKeyword);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassRefSelfExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassRefSelfExprBuilder;
   void validate();
};

class ClassRefStaticExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      StaticKeyword,
   };
public:
   ClassRefStaticExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getStaticKeyword();
   ClassRefStaticExprSyntax withStaticKeyword(std::optional<TokenSyntax> staticKeyword);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassRefStaticExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassRefStaticExprBuilder;
   void validate();
};

class IntegerLiteralExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      Digits,
   };
public:
   IntegerLiteralExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getDigits();
   IntegerLiteralExprSyntax withDigits(std::optional<TokenSyntax> digits);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::IntegerLiteralExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class IntegerLiteralExprSyntaxBuilder;
   void validate();
};

class FloatLiteralExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      FloatDigits,
   };

public:
   FloatLiteralExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getFloatDigits();
   FloatLiteralExprSyntax withFloatDigits(std::optional<TokenSyntax> digits);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::FloatLiteralExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class FloatLiteralExprSyntaxBuilder;
   void validate();
};

class StringLiteralExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      String,
   };
public:
   StringLiteralExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getString();
   StringLiteralExprSyntax withString(std::optional<TokenSyntax> str);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::StringLiteralExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class StringLiteralExprSyntaxBuilder;
   void validate();
};

class BooleanLiteralExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      Boolean,
   };

#ifdef POLAR_DEBUG_BUILD
   ///
   /// Child name: Boolean
   /// Choices:
   /// TokenKindType::T_TRUE
   /// TokenKindType::T_FALSE
   ///
   static const std::map<SyntaxChildrenCountType, std::set<TokenKindType>> CHILD_TOKEN_CHOICES;
#endif

public:
   BooleanLiteralExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getBooleanValue();

   /// Returns a copy of the receiver with its `Boolean` replaced.
   /// - param newChild: The new `Boolean` to replace the node's
   ///                   current `Boolean`, if present.
   BooleanLiteralExprSyntax withBooleanValue(std::optional<TokenSyntax> booleanValue);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BooleanLiteralExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class BooleanLiteralExprSyntaxBuilder;
   void validate();
};

class TernaryExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 5;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 5;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: ExprSyntax
      /// optional: false
      ConditionExpr,
      /// type: TokenSyntax
      /// optional: false
      QuestionMark,
      /// type: ExprSyntax
      /// optional: false
      FirstChoice,
      /// type: TokenSyntax
      /// optional: false
      ColonMark,
      /// type: ExprSyntax
      /// optional: false
      SecondChoice
   };

public:
   TernaryExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getConditionExpr();
   TokenSyntax getQuestionMark();
   ExprSyntax getFirstChoice();
   TokenSyntax getColonMark();
   ExprSyntax getSecondChoice();

   /// Returns a copy of the receiver with its `ConditionExpr` replaced.
   /// - param newChild: The new `ConditionExpr` to replace the node's
   ///                   current `ConditionExpr`, if present.
   TernaryExprSyntax withConditionExpr(std::optional<ExprSyntax> conditionExpr);

   /// Returns a copy of the receiver with its `QuestionMark` replaced.
   /// - param newChild: The new `QuestionMark` to replace the node's
   ///                   current `QuestionMark`, if present.
   TernaryExprSyntax withQuestionMark(std::optional<TokenSyntax> questionMark);

   /// Returns a copy of the receiver with its `FirstChoice` replaced.
   /// - param newChild: The new `FirstChoice` to replace the node's
   ///                   current `FirstChoice`, if present.
   TernaryExprSyntax withFirstChoice(std::optional<ExprSyntax> firstChoice);

   /// Returns a copy of the receiver with its `ColonMark` replaced.
   /// - param newChild: The new `ColonMark` to replace the node's
   ///                   current `ColonMark`, if present.
   TernaryExprSyntax withColonMark(std::optional<TokenSyntax> colonMark);

   /// Returns a copy of the receiver with its `SecondChoice` replaced.
   /// - param newChild: The new `SecondChoice` to replace the node's
   ///                   current `SecondChoice`, if present.
   TernaryExprSyntax withSecondChoice(std::optional<ExprSyntax> secondChoice);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::TernaryExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class TernaryExprSyntaxBuilder;
   void validate();
};

class AssignmentExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      AssignToken
   };
public:
   AssignmentExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getAssignToken();

   /// Returns a copy of the receiver with its `AssignToken` replaced.
   /// - param newChild: The new `AssignToken` to replace the node's
   ///                   current `AssignToken`, if present.
   AssignmentExprSyntax withAssignToken(std::optional<TokenSyntax> assignToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::AssignmentExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class AssignmentExprSyntaxBuilder;
   void validate();
};

class SequenceExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: ExprListSyntax
      /// is_syntax_collection: true
      /// optional: false
      Elements
   };

public:
   SequenceExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprListSyntax getElements();

   /// Adds the provided `Expr` to the node's `Elements`
   /// collection.
   /// - param element: The new `Expr` to add to the node's
   ///                  `Elements` collection.
   /// - returns: A copy of the receiver with the provided `Expr`
   ///            appended to its `Elements` collection.
   SequenceExprSyntax addElement(ExprSyntax expr);

   /// Returns a copy of the receiver with its `Elements` replaced.
   /// - param newChild: The new `Elements` to replace the node's
   ///                   current `Elements`, if present.
   SequenceExprSyntax withElements(std::optional<ExprListSyntax> elements);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SequenceExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SequenceExprSyntaxBuilder;
   void validate();
};

class PrefixOperatorExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: true
      OperatorToken,
      /// type: ExprSyntax
      /// optional: false
      Expr
   };

public:
   PrefixOperatorExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getOperatorToken();
   ExprSyntax getExpr();
   PrefixOperatorExprSyntax withOperatorToken(std::optional<TokenSyntax> operatorToken);
   PrefixOperatorExprSyntax withExpr(std::optional<TokenSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::PrefixOperatorExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class PrefixOperatorExprSyntaxBuilder;
   void validate();
};

class PostfixOperatorExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: Expr
      /// optional: false
      Expr,
      /// type: TokenSyntax
      /// optional: false
      OperatorToken
   };

public:
   PostfixOperatorExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getExpr();
   TokenSyntax getOperatorToken();

   PostfixOperatorExprSyntax withExpr(std::optional<ExprSyntax> expr);
   PostfixOperatorExprSyntax withOperatorToken(std::optional<TokenSyntax> operatorToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::PostfixOperatorExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class PostfixOperatorExprSyntaxBuilder;
   void validate();
};

class BinaryOperatorExprSyntax final : public ExprSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      OperatorToken
   };
public:
   BinaryOperatorExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getOperatorToken();
   BinaryOperatorExprSyntax withOperatorToken(std::optional<TokenSyntax> operatorToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BinaryOperatorExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class BinaryOperatorExprSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_H
