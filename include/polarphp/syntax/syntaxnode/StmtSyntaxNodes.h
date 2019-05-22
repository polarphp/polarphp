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

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H

#include "polarphp/syntax/Syntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"

namespace polar::syntax {

class ConditionElementSyntax;
class ContinueStmtSyntax;
class WhileStmtSyntax;
class DeferStmtSyntax;
class ExpressionStmtSyntax;
class ThrowStmtSyntax;
class ReturnStmtSyntax;
class CodeBlockSyntax;

// condition-list -> condition
//                 | condition-list, condition ','?
using ConditionElementListSyntax = SyntaxCollection<SyntaxKind::ConditionElementList, ConditionElementSyntax>;

///
/// \brief The ConditionElementSyntax class
///
///  condition -> expression
///
class ConditionElementSyntax : public Syntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: Syntax
      /// optional: false
      /// ------------
      /// node choices
      /// name: Expr kind: Expr
      Condition,
      /// type: TokenSyntax
      /// optional: true
      TrailingComma,
   };

#ifdef POLAR_DEBUG_BUILD
   ///
   /// child name: Condition
   /// choices:
   /// SyntaxKind::Expr
   ///
   static const std::map<SyntaxChildrenCountType, std::set<SyntaxKind>> CHILD_NODE_CHOICES;
#endif
public:
   ConditionElementSyntax(const RefCountPtr<SyntaxData> parent, const SyntaxData *data)
      : Syntax(parent, data)
   {
      validate();
   }

   Syntax getCondition();
   std::optional<TokenSyntax> getTrailingComma();

   ConditionElementSyntax withCondition(std::optional<Syntax> condition);
   ConditionElementSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

private:
   friend class ConditionElementSyntaxBuilder;
   void validate();
};

class ContinueStmtSyntax : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      ContinueKeyword,
      /// type: TokenSyntax
      /// optional: true
      LNumberToken,
   };
public:
   ContinueStmtSyntax(const RefCountPtr<SyntaxData> parent, const SyntaxData *data)
      : StmtSyntax(parent, data)
   {
      validate();
   }

   TokenSyntax getContinueKeyword();
   std::optional<TokenSyntax> getLNumberToken();

   ContinueStmtSyntax withContinueKeyword(std::optional<TokenSyntax> continueKeyword);
   ContinueStmtSyntax withLNumberToken(std::optional<TokenSyntax> numberToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ContinueStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ContinueStmtSyntaxBuilder;
   void validate();
};

class WhileStmtSyntax : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 5;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: true
      LabelName,
      /// type: TokenSyntax
      /// optional: true
      LabelColon,
      /// type: TokenSyntax
      /// optional: false
      WhileKeyword,
      /// type: ConditionElementListSyntax
      /// optional: false
      Conditions,
      /// type: CodeBlockSyntax
      /// optional: false
      Body
   };

public:
   WhileStmtSyntax(const RefCountPtr<SyntaxData> parent, const SyntaxData *data)
      : StmtSyntax(parent, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getWhileKeyword();
   ConditionElementListSyntax getConditions();
   CodeBlockSyntax getBody();

   WhileStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   WhileStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   WhileStmtSyntax withWhileKeyword(std::optional<TokenSyntax> whileKeyword);
   WhileStmtSyntax withConditions(std::optional<ConditionElementListSyntax> conditions);
   WhileStmtSyntax withBody(std::optional<CodeBlockSyntax> body);

   /// Adds the provided `condition` to the node's `Conditions`
   /// collection.
   /// - param element: The new `condition` to add to the node's
   ///                  `Conditions` collection.
   /// - returns: A copy of the receiver with the provided `condition`
   ///            appended to its `Conditions` collection.
   WhileStmtSyntax addCondition(ConditionElementSyntax condition);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::WhileStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class WhileStmtSyntaxBuilder;
   void validate();
};

class ThrowStmtSyntax : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 2;

   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      ThrowKeyword,
      /// type: ExprSyntax
      /// optional: false
      Expr
   };
public:
   ThrowStmtSyntax(const RefCountPtr<SyntaxData> parent, const SyntaxData *data)
      : StmtSyntax(parent, data)
   {
      this->validate();
   }

   TokenSyntax getThrowKeyword();
   ExprSyntax getExpr();

   ThrowStmtSyntax withThrowKeyword(std::optional<TokenSyntax> throwKeyword);
   ThrowStmtSyntax withExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ThrowStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ThrowStmtSyntaxBuilder;
   void validate();
};

class ReturnStmtSyntax : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: false
      ReturnKeyword,
      /// type: ExprSyntax
      /// optional: true
      Expr
   };
public:
   ReturnStmtSyntax(const RefCountPtr<SyntaxData> parent, const SyntaxData *data)
      : StmtSyntax(parent, data)
   {}

   TokenSyntax getReturnKeyword();
   ExprSyntax getExpr();
   ReturnStmtSyntax withReturnKeyword(std::optional<TokenSyntax> returnKeyword);
   ReturnStmtSyntax withExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ReturnStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ReturnStmtSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H
