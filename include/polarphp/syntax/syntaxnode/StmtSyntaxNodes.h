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
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/StmtSyntaxNodesFwd.h"

namespace polar::syntax {

///
/// empty_stmt:
///   ';'
///
class EmptyStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };

public:
   EmptyStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getSemicolon();
   EmptyStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EmptyStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EmptyStmtSyntaxBuilder;
   void validate();
};

///
/// statement:
///   '{' inner_statement_list '}'
///
class NestStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_BRACE)
      /// optional: false
      ///
      LeftBraceToken,
      ///
      /// type: InnerStmtListSyntax
      /// optional: false
      ///
      Statements,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      RightBraceToken,
   };

public:
   NestStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftBrace();
   InnerStmtListSyntax getStatements();
   TokenSyntax getRightBrace();

   NestStmtSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   NestStmtSyntax withStatements(std::optional<InnerStmtListSyntax> statements);
   NestStmtSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NestStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EmptyStmtSyntaxBuilder;
   void validate();
};

///
/// expr_stmt:
///   expr ';'
///
class ExprStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };

public:
   ExprStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getExpr();
   TokenSyntax getSemicolon();

   ExprStmtSyntax withExpr(std::optional<ExprSyntax> expr);
   ExprStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ExprStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ExprStmtBuilder;
   void validate();
};

///
/// inner_statement:
///   statement
/// | function_declaration_statement
/// | class_declaration_statement
/// | trait_declaration_statement
/// | interface_declaration_statement
/// | T_HALT_COMPILER '(' ')' ';'
///
class InnerStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 3;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: StmtSyntax
      /// optional: false
      /// node choices: true
      /// --------------------------------------------
      /// node choice: StmtSyntax
      /// --------------------------------------------
      /// node choice: ClassDefinitionStmtSyntax
      /// --------------------------------------------
      /// node choice: InterfaceDefinitionStmtSyntax
      /// --------------------------------------------
      /// node choice: TraitDefinitionStmtSyntax
      /// --------------------------------------------
      /// node choice: FunctionDefinitionStmtSyntax
      ///
      Stmt
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   InnerStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   StmtSyntax getStmt();
   InnerStmtSyntax withStmt(std::optional<StmtSyntax> stmt);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InnerStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class InnerStmtSyntaxBuilder;
   void validate();
};

///
/// inner_code_block_stmt:
///   '{' inner_statement_list '}'
///
class InnerCodeBlockStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 3;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_BRACE)
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: InnerStmtListSyntax
      /// optional: false
      ///
      Statements,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      RightBrace
   };

public:
   InnerCodeBlockStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftBrace();
   InnerStmtListSyntax getStatements();
   TokenSyntax getRightBrace();

   InnerCodeBlockStmtSyntax addCodeBlockItem(InnerStmtSyntax stmt);
   InnerCodeBlockStmtSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   InnerCodeBlockStmtSyntax withStatements(std::optional<InnerStmtSyntax> statements);
   InnerCodeBlockStmtSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::InnerCodeBlockStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class InnerCodeBlockStmtSyntaxBuilder;
   void validate();
};

///
/// top_statement:
///   statement
/// | function_declaration_statement
/// | class_declaration_statement
/// |	trait_declaration_statement
/// | interface_declaration_statement
/// | T_HALT_COMPILER '(' ')' ';'
/// | T_NAMESPACE namespace_name ';'
/// | T_NAMESPACE namespace_name '{' top_statement_list '}'
/// | T_NAMESPACE '{' top_statement_list '}'
/// | T_USE mixed_group_use_declaration ';'
/// | T_USE use_type group_use_declaration ';'
/// | T_USE use_declarations ';'
/// | T_USE use_type use_declarations ';'
/// | T_CONST const_list ';'
///
class TopStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 1;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: StmtSyntax
      /// optional: false
      ///
      Stmt
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   TopStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   StmtSyntax getStmt();
   TopStmtSyntax withStmt(std::optional<StmtSyntax> stmt);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::TopStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class TopStmtSyntaxBuilder;
   void validate();
};

///
/// top_code_block_stmt:
///   '{' top_statement_list '}'
///
class TopCodeBlockStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 3;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_BRACE)
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: TopStmtListSyntax
      /// optional: false
      ///
      Statements,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      RightBrace
   };

public:
   TopCodeBlockStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftBrace();
   TopStmtListSyntax getStatements();
   TokenSyntax getRightBrace();


   TopCodeBlockStmtSyntax addCodeBlockItem(TopStmtSyntax stmt);
   TopCodeBlockStmtSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   TopCodeBlockStmtSyntax withStatements(std::optional<TopStmtListSyntax> statements);
   TopCodeBlockStmtSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::TopCodeBlockStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class TopCodeBlockStmtSyntaxBuilder;
   void validate();
};

///
/// declare_stmr:
///   T_DECLARE '(' const_list ')' declare_statement
///
class DeclareStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 5;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 5;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DECLARE)
      /// optional: false
      ///
      DeclareToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: ConstDeclareListSyntax
      /// optional: false
      ///
      ConstList,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken,
      ///
      /// type: StmtSyntax
      /// optional: false
      ///
      Stmt
   };

public:
   DeclareStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getDeclareToken();
   TokenSyntax getLeftParenToken();
   ConstDeclareListSyntax getConstList();
   TokenSyntax getRightParenToken();
   StmtSyntax getStmt();

   DeclareStmtSyntax withDeclareToken(std::optional<TokenSyntax> declareToken);
   DeclareStmtSyntax withLeftParenToken(std::optional<TokenSyntax> leftParen);
   DeclareStmtSyntax withConstList(std::optional<ConstDeclareListSyntax> constList);
   DeclareStmtSyntax withRightParenToken(std::optional<TokenSyntax> rightParen);
   DeclareStmtSyntax withStmt(std::optional<StmtSyntax> stmt);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::DeclareStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class DeclareStmtSyntaxBuilder;
   void validate();
};

///
/// goto_stmt:
///   T_GOTO T_IDENTIFIER_STRING ';'
///
class GotoStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 3;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_GOTO)
      /// optional: false
      ///
      GotoToken,
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: false
      ///
      Target,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };

public:
   GotoStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getGotoToken();
   TokenSyntax getTarget();
   TokenSyntax getSemicolon();

   GotoStmtSyntax withGotoToken(std::optional<TokenSyntax> gotoToken);
   GotoStmtSyntax withTarget(std::optional<TokenSyntax> target);
   GotoStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::InnerCodeBlockStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class GotoStmtSyntaxBuilder;
   void validate();
};

///
/// unset_variable:
///   variable ','
///
class UnsetVariableSyntax final : public Syntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_VARIABLE)
      /// optional: false
      ///
      Variable,
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      TrailingComma,
   };

public:
   UnsetVariableSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getVariable();
   std::optional<TokenSyntax> getTrailingComma();

   UnsetVariableSyntax withVariable(std::optional<TokenSyntax> variable);
   UnsetVariableSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnsetVariable == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class UnsetVariableSyntaxBuilder;
   void validate();
};

///
/// unset_stmt:
///   T_UNSET '(' unset_variables possible_comma ')' ';'
///
class UnsetStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 5;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 5;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_UNSET)
      /// optional: false
      ///
      UnsetToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: UnsetVariableListSyntax
      /// optional: false
      ///
      UnsetVariables,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken,
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: false
      ///
      Semicolon,
   };

public:
   UnsetStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getUnsetToken();
   TokenSyntax getLeftParenToken();
   UnsetVariableListSyntax getUnsetVariables();
   TokenSyntax getRightParenToken();
   TokenSyntax getSemicolon();

   UnsetStmtSyntax withUnsetToken(std::optional<TokenSyntax> unsetToken);
   UnsetStmtSyntax withLeftParenToken(std::optional<TokenSyntax> leftParen);
   UnsetStmtSyntax withUnsetVariables(std::optional<UnsetVariableListSyntax> variables);
   UnsetStmtSyntax withRightParenToken(std::optional<TokenSyntax> rightParen);
   UnsetStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::UnsetStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class UnsetStmtSyntaxBuilder;
   void validate();
};

///
/// label_stmt:
///   T_IDENTIFIER_STRING ':'
///
class LabelStmtSyntax final : public StmtSyntax
{
public:
   constexpr static unsigned int CHILDREN_COUNT = 2;
   constexpr static unsigned int REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: false
      ///
      Name,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: false
      ///
      Colon
   };

public:
   LabelStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getName();
   TokenSyntax getColon();

   LabelStmtSyntax withName(std::optional<TokenSyntax> name);
   LabelStmtSyntax withColon(std::optional<TokenSyntax> colon);

   static bool kindOf(SyntaxKind kind)
   {
      return SyntaxKind::LabelStmt == kind;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class LabelStmtSyntaxBuilder;
   void validate();
};

///
///  condition -> expression
///
class ConditionElementSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      /// node choices: true
      /// -----------------------------------
      /// node choice: ExprSyntax
      ///
      Condition,

      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      TrailingComma,
   };

#ifdef POLAR_DEBUG_BUILD
   ///
   /// child name: Condition
   /// choices:
   /// SyntaxKind::Expr
   ///
   static const NodeChoicesType CHILD_NODE_CHOICES;
#endif
public:
   ConditionElementSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ExprSyntax getCondition();
   std::optional<TokenSyntax> getTrailingComma();

   ConditionElementSyntax withCondition(std::optional<ExprSyntax> condition);
   ConditionElementSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConditionElement;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConditionElementSyntaxBuilder;
   void validate();
};

///
/// continue_stmt:
///   T_CONTINUE optional_expr ';'
///
class ContinueStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_CONTINUE)
      /// optional: false
      ///
      ContinueKeyword,
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };
public:
   ContinueStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getContinueKeyword();
   std::optional<ExprSyntax> getExpr();
   TokenSyntax getSemicolon();

   ContinueStmtSyntax withContinueKeyword(std::optional<TokenSyntax> continueKeyword);
   ContinueStmtSyntax withExpr(std::optional<ExprSyntax> expr);
   ContinueStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

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

///
/// break_stmt:
///   T_BREAK optional_expr ';'
///
class BreakStmtSyntax final : public StmtSyntax
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
      BreakKeyword,
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };
public:
   BreakStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getBreakKeyword();
   std::optional<ExprSyntax> getExpr();
   TokenSyntax getSemicolon();

   BreakStmtSyntax withBreakKeyword(std::optional<TokenSyntax> breakKeyword);
   BreakStmtSyntax withExpr(std::optional<ExprSyntax> expr);
   BreakStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BreakStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class BreakStmtSyntaxBuilder;
   void validate();
};

class FallthroughStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_FALLTHROUGH)
      /// optional: false
      ///
      FallthroughKeyword,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };

public:
   FallthroughStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getFallthroughKeyword();
   TokenSyntax getSemicolon();

   FallthroughStmtSyntax withFallthroughStmtSyntax(std::optional<TokenSyntax> fallthroughKeyword);
   FallthroughStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

private:
   friend class FallthroughStmtSyntaxBuilder;
   void validate();
};

///
/// if_stmt_without_else:
///   T_IF '(' expr ')' statement
/// | if_stmt_without_else T_ELSEIF '(' expr ')' statement
///
class ElseIfClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 5;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ELSEIF)
      /// optional: false
      ///
      ElseIfKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Condition,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: StmtSyntax
      /// optional: false
      ///
      Body
   };

public:
   ElseIfClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getElseIfKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getCondition();
   TokenSyntax getRightParen();
   StmtSyntax getBody();

   ElseIfClauseSyntax withElseIfKeyword(std::optional<TokenSyntax> elseIfKeyword);
   ElseIfClauseSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   ElseIfClauseSyntax withCondition(std::optional<ExprSyntax> condition);
   ElseIfClauseSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   ElseIfClauseSyntax withBody(std::optional<StmtSyntax> body);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ElseIfClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ElseIfClauseSyntaxBuilder;
   void validate();
};

///
/// if_stmt:
///   if_stmt_without_else %prec T_NOELSE
/// | if_stmt_without_else T_ELSE statement
///
class IfStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 10;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_IF)
      /// optional: false
      ///
      IfKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Condition,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body,
      ///
      /// type: ElseIfListSyntax
      /// is_syntax_collection: true
      /// optional: true
      ///
      ElseIfClauses,
      ///
      /// type: TokenSyntax (T_ELSE)
      /// optional: true
      ///
      ElseKeyword,
      ///
      /// type: Syntax
      /// optional: true
      /// --------------
      /// node choices
      /// name: IfStmt kind: IfStmt
      /// name: CodeBlock kind: CodeBlock
      ///
      ElseBody
   };

#ifdef POLAR_DEBUG_BUILD
   static const NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   IfStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getIfKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getCondition();
   TokenSyntax getRightParen();
   StmtSyntax getBody();
   std::optional<ElseIfListSyntax> getElseIfClauses();
   std::optional<TokenSyntax> getElseKeyword();
   std::optional<Syntax> getElseBody();

   IfStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   IfStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   IfStmtSyntax withIfKeyword(std::optional<TokenSyntax> ifKeyword);
   IfStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   IfStmtSyntax withCondition(std::optional<ExprSyntax> condition);
   IfStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   IfStmtSyntax withBody(std::optional<StmtSyntax> body);
   IfStmtSyntax withElseIfClauses(std::optional<ElseIfListSyntax> elseIfClauses);
   IfStmtSyntax withElseKeyword(std::optional<TokenSyntax> elseKeyword);
   IfStmtSyntax withElseBody(std::optional<Syntax> elseBody);

   IfStmtSyntax addElseIfClause(ElseIfClauseSyntax elseIfClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::IfStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class IfStmtSyntaxBuilder;
   void validate();
};

///
/// while_stmt:
///   T_WHILE '(' expr ')' while_statement
///
class WhileStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 7;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_WHILE)
      /// optional: false
      ///
      WhileKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ConditionElementListSyntax
      /// optional: false
      ///
      Conditions,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: InnerCodeBlockStmtSyntax
      /// optional: false
      ///
      Body
   };

public:
   WhileStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getWhileKeyword();
   TokenSyntax getLeftParen();
   ConditionElementListSyntax getConditions();
   TokenSyntax getRightParen();
   InnerCodeBlockStmtSyntax getBody();

   WhileStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   WhileStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   WhileStmtSyntax withWhileKeyword(std::optional<TokenSyntax> whileKeyword);
   WhileStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   WhileStmtSyntax withConditions(std::optional<ConditionElementListSyntax> conditions);
   WhileStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   WhileStmtSyntax withBody(std::optional<InnerCodeBlockStmtSyntax> body);

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

class DoWhileStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 8;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_DO)
      /// optional: false
      ///
      DoKeyword,
      ///
      /// type: CodeBlockSyntax
      /// optional: false
      ///
      Body,
      ///
      /// type: TokenSyntax (T_WHILE)
      /// optional: false
      ///
      WhileKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Condition,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon,
   };

public:
   DoWhileStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getDoKeyword();
   InnerCodeBlockStmtSyntax getBody();
   TokenSyntax getWhileKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getCondition();
   TokenSyntax getRightParen();
   TokenSyntax getSemicolon();

   DoWhileStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   DoWhileStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   DoWhileStmtSyntax withDoKeyword(std::optional<TokenSyntax> doKeyword);
   DoWhileStmtSyntax withBody(std::optional<InnerCodeBlockStmtSyntax> body);
   DoWhileStmtSyntax withWhileKeyword(std::optional<TokenSyntax> whileKeyword);
   DoWhileStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   DoWhileStmtSyntax withCondition(std::optional<ExprSyntax> condition);
   DoWhileStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   DoWhileStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::DoWhileStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class DoWhileStmtSyntaxBuilder;
   void validate();
};

///
/// for_stmt:
///   T_FOR '(' for_exprs ';' for_exprs ';' for_exprs ')' for_statement
///
class ForStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 9;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_FOR)
      /// optional: false
      ///
      ForToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: ExprListSyntax
      /// optional: true
      ///
      InitializedExprs,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      InitializedSemicolonToken,
      ///
      /// type: ExprListSyntax
      /// optional: true
      ///
      ConditionalExprs,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      ConditionalSemicolonToken,
      ///
      /// type: ExprListSyntax
      /// optional: true
      ///
      OperationalExprs,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken,
      ///
      /// type: StmtSyntax
      /// optional: false
      ///
      Stmt
   };
public:
   ForStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getForToken();
   TokenSyntax getLeftParenToken();
   std::optional<ExprListSyntax> getInitializedExprs();
   TokenSyntax getInitializedSemicolonToken();
   std::optional<ExprListSyntax> getConditionalExprs();
   TokenSyntax getConditionalSemicolonToken();
   std::optional<ExprListSyntax> getOperationalExprs();
   TokenSyntax getRightParenToken();
   StmtSyntax getStmt();

   ForStmtSyntax withForToken(std::optional<TokenSyntax> forToken);
   ForStmtSyntax withLeftParenToken(std::optional<TokenSyntax> leftParen);
   ForStmtSyntax withInitializedExprs(std::optional<TokenSyntax> exprs);
   ForStmtSyntax withInitializedSemicolonToken(std::optional<TokenSyntax> semicolon);
   ForStmtSyntax withConditionalExprs(std::optional<TokenSyntax> exprs);
   ForStmtSyntax withConditionalSemicolonToken(std::optional<TokenSyntax> semicolon);
   ForStmtSyntax withOperationalExprs(std::optional<TokenSyntax> exprs);
   ForStmtSyntax withRightParenToken(std::optional<TokenSyntax> rightParen);
   ForStmtSyntax withStmt(std::optional<TokenSyntax> stmt);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ForStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ForStmtSyntaxBuilder;
   void validate();
};

///
/// foreach_variable:
///   variable
/// |	'&' variable
/// |	T_LIST '(' array_pair_list ')'
/// |	'[' array_pair_list ']'
///
class ForeachVariableSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      /// node choices: true
      /// -----------------------------------------------
      /// node choice: VariableExprSyntax
      /// -----------------------------------------------
      /// node choice: ReferencedVariableExprSyntax
      /// -----------------------------------------------
      /// node choice: ListStructureClauseSyntax
      /// -----------------------------------------------
      /// node choice: SimplifiedArrayCreateExprSyntax
      ///
      Variable
   };
#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif
public:

   ForeachVariableSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ExprSyntax getVariable();
   ForeachVariableSyntax withVariable(std::optional<ExprSyntax> variable);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ForeachVariable;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ForeachVariableSyntaxBuilder;
   void validate();
};

///
/// foreach_stmt:
///   T_FOREACH '(' expr T_AS foreach_variable ')' foreach_statement
/// | T_FOREACH '(' expr T_AS foreach_variable T_DOUBLE_ARROW foreach_variable ')' foreach_statement
///
class ForeachStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 9;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 7;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_FOREACH)
      /// optional: false
      ///
      ForeachToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      IterableExpr,
      ///
      /// type: TokenSyntax (T_AS)
      /// optional: false
      ///
      AsToken,
      ///
      /// type: ForeachVariableSyntax
      /// optional: true
      ///
      KeyVariable,
      ///
      /// type: TokenSyntax (T_DOUBLE_ARROW)
      /// optional: true
      ///
      DoubleArrowToken,
      ///
      /// type: ForeachVariableSyntax
      /// optional: false
      ///
      ValueVariable,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken,
      ///
      /// type: StmtSyntax
      /// optional: false
      ///
      Stmt
   };
public:
   ForeachStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getForeachToken();
   TokenSyntax getLeftParenToken();
   ExprSyntax getIterableExpr();
   TokenSyntax getAsToken();
   std::optional<ForeachVariableSyntax> getKeyVariable();
   std::optional<TokenSyntax> getDoubleArrowToken();
   ForeachVariableSyntax getValueVariable();
   TokenSyntax getRightParenToken();
   StmtSyntax getStmt();

   ForeachStmtSyntax withForeachToken(std::optional<TokenSyntax> foreachToken);
   ForeachStmtSyntax withLeftParenToken(std::optional<TokenSyntax> leftParen);
   ForeachStmtSyntax withIterableExpr(std::optional<ExprSyntax> iterableExpr);
   ForeachStmtSyntax withAsToken(std::optional<TokenSyntax> asToken);
   ForeachStmtSyntax withKeyVariable(std::optional<ForeachVariableSyntax> key);
   ForeachStmtSyntax withDoubleArrowToken(std::optional<TokenSyntax> doubleArrow);
   ForeachStmtSyntax withValueVariable(std::optional<ForeachVariableSyntax> value);
   ForeachStmtSyntax withRightParenToken(std::optional<TokenSyntax> rightParen);
   ForeachStmtSyntax withStmt(std::optional<StmtSyntax> stmt);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ForeachStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ForeachStmtSyntaxBuilder;
   void validate();
};

///
/// switch-default-label -> 'default' ':'
///
class SwitchDefaultLabelSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DEFAULT)
      /// optional: false
      ///
      DefaultKeyword,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: false
      ///
      Colon,
   };

public:
   SwitchDefaultLabelSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getDefaultKeyword();
   TokenSyntax getColon();

   SwitchDefaultLabelSyntax withDefaultKeyword(std::optional<TokenSyntax> defaultKeyword);
   SwitchDefaultLabelSyntax withColon(std::optional<TokenSyntax> colon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SwitchDefaultLabel;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SwitchDefaultLabelSyntaxBuilder;
   void validate();
};

///
/// switch-case-label -> 'case' case-item-list ':'
///
class SwitchCaseLabelSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_CASE)
      /// optional: false
      ///
      CaseKeyword,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: false
      ///
      Colon
   };
public:
   SwitchCaseLabelSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getCaseKeyword();
   ExprSyntax getExpr();
   TokenSyntax getColon();

   SwitchCaseLabelSyntax withCaseKeyword(std::optional<TokenSyntax> caseKeyword);
   SwitchCaseLabelSyntax withExpr(std::optional<ExprSyntax> expr);
   SwitchCaseLabelSyntax withColon(std::optional<TokenSyntax> colon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SwitchCaseLabel;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SwitchCaseLabelSyntaxBuilder;
   void validate();
};

///
/// switch-case -> switch-case-label stmt-list
///              | switch-default-label stmt-list
///
class SwitchCaseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices
      /// -------------------------------------------
      /// node choice: SwitchDefaultLabelSyntax
      /// -------------------------------------------
      /// node choice: SwitchCaseLabelSyntax
      ///
      Label,
      ///
      /// type: InnerCodeBlockStmtSyntax
      /// optional: false
      ///
      Statements
   };

#ifdef POLAR_DEBUG_BUILD
   static const NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   SwitchCaseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getLabel();
   InnerCodeBlockStmtSyntax getStatements();

   SwitchCaseSyntax withLabel(std::optional<Syntax> label);
   SwitchCaseSyntax withStatements(std::optional<InnerStmtListSyntax> statements);
   SwitchCaseSyntax addStatement(InnerStmtSyntax statement);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SwitchCase;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SwitchCaseSyntaxBuilder;
   void validate();
};

///
/// switch-stmt -> identifier? ':'? 'switch' '(' expr ')' '{'
///    switch-case-list '}'
///
class SwitchStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 9;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 7;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      LabelName,
      ///
      /// type: TokenSyntax (T_COLON)
      /// optional: true
      ///
      LabelColon,
      ///
      /// type: TokenSyntax (T_SWITCH)
      /// optional: false
      ///
      SwitchKeyword,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ConditionExpr,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen,
      ///
      /// type: TokenSyntax (T_LEFT_BRACE)
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: SwitchCaseListSyntax
      /// optional: false
      ///
      Cases,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      RightBrace
   };

public:
   SwitchStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getLabelName();
   std::optional<TokenSyntax> getLabelColon();
   TokenSyntax getSwitchKeyword();
   TokenSyntax getLeftParen();
   ExprSyntax getConditionExpr();
   TokenSyntax getRightParen();
   TokenSyntax getLeftBrace();
   SwitchCaseListSyntax getCases();
   TokenSyntax getRightBrace();

   SwitchStmtSyntax withLabelName(std::optional<TokenSyntax> labelName);
   SwitchStmtSyntax withLabelColon(std::optional<TokenSyntax> labelColon);
   SwitchStmtSyntax withSwitchKeyword(std::optional<TokenSyntax> switchKeyword);
   SwitchStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   SwitchStmtSyntax withConditionExpr(std::optional<ExprSyntax> conditionExpr);
   SwitchStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   SwitchStmtSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   SwitchStmtSyntax withCases(std::optional<SwitchCaseListSyntax> cases);
   SwitchStmtSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   SwitchStmtSyntax addCase(SwitchCaseSyntax switchCase);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SwitchStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SwitchStmtSyntaxBuilder;
   void validate();
};

///
/// defer-stmt -> 'defer' code-block
///
class DeferStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DEFER)
      /// optional: false
      ///
      DeferKeyword,
      ///
      /// type: InnerCodeBlockStmtSyntax
      /// optional: false
      ///
      Body
   };

public:
   DeferStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getDeferKeyword();
   InnerCodeBlockStmtSyntax getBody();
   DeferStmtSyntax withDeferKeyword(std::optional<TokenSyntax> deferKeyword);
   DeferStmtSyntax withBody(std::optional<InnerCodeBlockStmtSyntax> body);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::DeferStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class DeferStmtSyntaxBuilder;
   void validate();
};

///
/// throw_stmt:
///   T_THROW expr ';'
///
class ThrowStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_THROW)
      /// optional: false
      ///
      ThrowKeyword,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optioal: false
      ///
      Semicolon,
   };
public:
   ThrowStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getThrowKeyword();
   ExprSyntax getExpr();
   TokenSyntax getSemicolon();

   ThrowStmtSyntax withThrowKeyword(std::optional<TokenSyntax> throwKeyword);
   ThrowStmtSyntax withExpr(std::optional<ExprSyntax> expr);
   ThrowStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

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

///
/// try_stmt:
///   T_TRY '{' inner_statement_list '}' catch_list finally_statement
///
class TryStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_TRY)
      /// optional: false
      ///
      TryToken,
      ///
      /// type: InnerCodeBlockStmtSyntax
      /// optional: false
      ///
      CodeBlock,
      ///
      /// type: CatchListSyntax
      /// optional: true
      ///
      CatchList,
      ///
      /// type: FinallyClauseSyntax
      /// optional: true
      ///
      FinallyClause
   };

public:
   TryStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getTryToken();
   InnerCodeBlockStmtSyntax getCodeBlock();
   std::optional<CatchListSyntax> getCatchList();
   std::optional<FinallyClauseSyntax> getFinallyClause();

   TryStmtSyntax withTryToken(std::optional<TokenSyntax> tryToken);
   TryStmtSyntax withCodeBlock(std::optional<InnerCodeBlockStmtSyntax> codeBlock);
   TryStmtSyntax withCatchList(std::optional<CatchListSyntax> catchList);
   TryStmtSyntax withFinallyClause(std::optional<FinallyClauseSyntax> finallyClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::TryStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class TryStmtSyntaxBuilder;
   void validate();
};

///
/// finally_statement:
///   T_FINALLY '{' inner_statement_list '}'
///
class FinallyClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_FINALLY)
      /// optional: false
      ///
      FinallyToken,
      ///
      /// type: InnerCodeBlockStmtSyntax
      /// optional: false
      ///
      CodeBlock
   };

public:
   FinallyClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getFinallyToken();
   InnerCodeBlockStmtSyntax getCodeBlock();

   FinallyClauseSyntax withFinallyToken(std::optional<TokenSyntax> finallyToken);
   FinallyClauseSyntax withCodeBlock(std::optional<InnerCodeBlockStmtSyntax> codeBlock);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::FinallyClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class FinallyClauseSyntaxBuilder;
   void validate();
};

///
/// catch_arg_type_hint_item:
///   name '|'
///
class CatchArgTypeHintItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: NameSyntax
      /// optional: false
      ///
      TypeName,
      ///
      /// type: TokenSyntax (T_VBAR)
      /// optional: true
      ///
      Separator
   };

public:
   CatchArgTypeHintItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   NameSyntax getTypeName();
   std::optional<TokenSyntax> getSeparator();

   CatchArgTypeHintItemSyntax withTypeName(std::optional<NameSyntax> typeName);
   CatchArgTypeHintItemSyntax withSeparator(std::optional<TokenSyntax> separator);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::CatchArgTypeHintItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CatchArgTypeHintItemSyntaxBuilder;
   void validate();
};

///
/// catch_list:
///   catch_list T_CATCH '(' catch_name_list T_VARIABLE ')' '{' inner_statement_list '}'
///
class CatchListItemClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 6;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_CATCH)
      /// optional: false
      ///
      CatchToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: CatchArgTypeHintListSyntax
      /// optional: false
      ///
      CatchArgTypeHintList,
      ///
      /// type: TokenSyntax (T_VARIABLE)
      /// optional: false
      ///
      Variable,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken,
      ///
      /// type: InnerCodeBlockStmtSyntax
      /// optional: false
      ///
      CodeBlock
   };

public:
   CatchListItemClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getCatchToken();
   TokenSyntax getLeftParenToken();
   CatchArgTypeHintListSyntax getCatchArgTypeHintList();
   TokenSyntax getVariable();
   TokenSyntax getRightParenToken();
   InnerCodeBlockStmtSyntax getCodeBlock();

   CatchListItemClauseSyntax withCatchToken(std::optional<TokenSyntax> catchToken);
   CatchListItemClauseSyntax withLeftParenToken(std::optional<TokenSyntax> leftParen);
   CatchListItemClauseSyntax withCatchArgTypeHintList(std::optional<CatchArgTypeHintListSyntax> typeHints);
   CatchListItemClauseSyntax withVariable(std::optional<TokenSyntax> variable);
   CatchListItemClauseSyntax withRightParenToken(std::optional<TokenSyntax> rightParen);
   CatchListItemClauseSyntax withCodeBlock(std::optional<InnerCodeBlockStmtSyntax> codeBlock);

   CatchListItemClauseSyntax addCatchArgTypeHintList(InnerCodeBlockStmtSyntax typeHint);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::CatchListItemClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CatchListItemClauseSyntaxBuilder;
   void validate();
};

///
/// return_stmt:
///   T_RETURN optional_expr ';'
///
class ReturnStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_RETURN)
      /// optional: false
      ///
      ReturnKeyword,
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optioal: false
      ///
      Semicolon,
   };

public:
   ReturnStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getReturnKeyword();
   ExprSyntax getExpr();
   TokenSyntax getSemicolon();

   ReturnStmtSyntax withReturnKeyword(std::optional<TokenSyntax> returnKeyword);
   ReturnStmtSyntax withExpr(std::optional<ExprSyntax> expr);
   ReturnStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

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

///
/// echo_stmt:
///   T_ECHO echo_expr_list ';'
///
class EchoStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ECHO)
      /// optional: false
      ///
      EchoToken,
      ///
      /// type: ExprListSyntax
      /// optional: false
      ///
      ExprListClause,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optioal: false
      ///
      Semicolon,
   };

public:
   EchoStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getEchoToken();
   ExprListSyntax getExprListClause();
   TokenSyntax getSemicolon();

   EchoStmtSyntax withEchoToken(std::optional<TokenSyntax> echoToken);
   EchoStmtSyntax withExprListClause(std::optional<ExprListSyntax> exprListClause);
   EchoStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EchoStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EchoStmtSyntaxBuilder;
   void validate();
};

///
/// halt_compiler_stmt:
///   T_HALT_COMPILER '(' ')' ';'
///
class HaltCompilerStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_HALT_COMPILER)
      /// optional: false
      ///
      HaltCompilerToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optioal: false
      ///
      LeftParen,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optioal: false
      ///
      RightParen,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optioal: false
      ///
      Semicolon,
   };

public:
   HaltCompilerStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getHaltCompilerToken();
   TokenSyntax getLeftParen();
   TokenSyntax getRightParen();
   TokenSyntax getSemicolon();

   HaltCompilerStmtSyntax withHaltCompilerToken(std::optional<TokenSyntax> haltCompilerToken);
   HaltCompilerStmtSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   HaltCompilerStmtSyntax withRightParen(std::optional<TokenSyntax> rightParen);
   HaltCompilerStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::HaltCompilerStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class HaltCompilerStmtSyntaxBuilder;
   void validate();
};

///
/// global_var:
///   simple_variable
///
class GlobalVariableSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: SimpleVariableExprSyntax
      /// optional: false
      ///
      Variable
   };
public:
   GlobalVariableSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   SimpleVariableExprSyntax getVariable();
   GlobalVariableSyntax withVariable(std::optional<TokenSyntax> variable);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::GlobalVariable;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class GlobalVariableSyntaxBuilder;
   void validate();
};

///
/// global_var:
///   simple_variable ','
///
class GlobalVariableListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      Comma,
      ///
      /// type: GlobalVariableSyntax
      /// optional: false
      ///
      Variable
   };

public:
   GlobalVariableListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getComma();
   GlobalVariableSyntax getVariable();

   GlobalVariableListItemSyntax withComma(std::optional<TokenSyntax> comma);
   GlobalVariableListItemSyntax withVariable(std::optional<GlobalVariableSyntax> variable);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::GlobalVariableListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class GlobalVariableListItemSyntaxBuilder;
   void validate();
};

///
/// global_variable_declarations_stmt:
///   T_GLOBAL global_var_list ';'
///
class GlobalVariableDeclarationsStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_GLOBAL)
      /// optional: false
      ///
      GlobalToken,
      ///
      /// type: GlobalVariableListSyntax
      /// optional: false
      ///
      Variables,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optioal: false
      ///
      Semicolon,
   };

public:
   GlobalVariableDeclarationsStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getGlobalToken();
   GlobalVariableListSyntax getVariables();
   TokenSyntax getSemicolon();

   GlobalVariableDeclarationsStmtSyntax withGlobalToken(std::optional<TokenSyntax> globalToken);
   GlobalVariableDeclarationsStmtSyntax withVariables(std::optional<GlobalVariableListSyntax> variables);
   GlobalVariableDeclarationsStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::GlobalVariableDeclarationsStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class GlobalVariableDeclarationsStmtSyntaxBuilder;
   void validate();
};

///
/// static_var:
///   T_VARIABLE
/// | T_VARIABLE '=' expr
///
class StaticVariableDeclareSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_VARIABLE)
      /// optional: false
      ///
      Variable,
      ///
      /// type: TokenSyntax (T_EQUAL)
      /// optional: true
      ///
      EqualToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ValueExpr
   };

public:
   StaticVariableDeclareSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getVariable();
   std::optional<TokenSyntax> getEqualToken();
   std::optional<ExprSyntax> getValueExpr();

   StaticVariableDeclareSyntax withVariable(std::optional<TokenSyntax> variable);
   StaticVariableDeclareSyntax withEqualToken(std::optional<TokenSyntax> equalToken);
   StaticVariableDeclareSyntax withValueExpr(std::optional<ExprSyntax> valueExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::StaticVariableDeclare;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class StaticVariableDeclareSyntaxBuilder;
   void validate();
};

///
/// static_variable_list_item:
///   ',' static_var
///
class StaticVariableListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      Comma,
      ///
      /// type: StaticVariableDeclareSyntax
      /// optional: false
      ///
      Declaration,
   };

public:
   StaticVariableListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getComma();
   StaticVariableDeclareSyntax getDeclaration();

   StaticVariableListItemSyntax withComma(std::optional<TokenSyntax> comma);
   StaticVariableListItemSyntax withDeclaration(std::optional<TokenSyntax> declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::StaticVariableListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class StaticVariableListItemSyntaxBuilder;
   void validate();
};

///
/// static_variable_declarations_stmt:
///   T_STATIC static_var_list ';'
///
class StaticVariableDeclarationsStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_STATIC)
      /// optional: false
      ///
      StaticToken,
      ///
      /// type: StaticVariableListSyntax
      /// optional: false
      ///
      Variables,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optioal: false
      ///
      Semicolon,
   };

public:
   StaticVariableDeclarationsStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getStaticToken();
   GlobalVariableListSyntax getVariables();
   TokenSyntax getSemicolon();

   StaticVariableDeclarationsStmtSyntax withStaticToken(std::optional<TokenSyntax> staticToken);
   StaticVariableDeclarationsStmtSyntax withVariables(std::optional<GlobalVariableListSyntax> variables);
   StaticVariableDeclarationsStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::StaticVariableDeclarationsStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class StaticVariableDeclarationsStmtSyntaxBuilder;
   void validate();
};

///
/// use_type:
///   T_FUNCTION
/// | T_CONST
///
class NamespaceUseTypeSyntax final : public Syntax
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
      /// -------------------------------------
      /// T_FUNCTION
      /// T_CONST
      ///
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
      ///
      /// type: NamespaceNameSyntax
      /// optional: false
      ///
      Namespace,
      ///
      /// type: TokenSyntax (T_AS)
      /// optional: true
      ///
      AsToken,
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: true
      ///
      IdentifierToken
   };

public:
   NamespaceUnprefixedUseDeclarationSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   NamespaceNameSyntax getNamespace();
   std::optional<TokenSyntax> getAsToken();
   std::optional<TokenSyntax> getIdentifierToken();

   NamespaceUnprefixedUseDeclarationSyntax withNamespace(std::optional<NamespaceNameSyntax> ns);
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
/// namespace_unprefixed_use_declaration_list_item:
/// ',' unprefixed_use_declaration
///
class NamespaceUnprefixedUseDeclarationListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      CommaToken,
      ///
      /// type: NamespaceUnprefixedUseDeclarationSyntax
      /// optional: false
      ///
      NamespaceUseDeclaration
   };

public:
   NamespaceUnprefixedUseDeclarationListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getCommaToken();
   NamespaceUnprefixedUseDeclarationSyntax getNamespaceUseDeclaration();

   NamespaceUnprefixedUseDeclarationListItemSyntax withCommaToken(std::optional<TokenSyntax> comma);
   NamespaceUnprefixedUseDeclarationListItemSyntax withNamespaceUseDeclaration(std::optional<NamespaceUnprefixedUseDeclarationSyntax> declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceUnprefixedUseDeclarationListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceUnprefixedUseDeclarationListItemSyntaxBuilder;
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
      ///
      /// type: TokenSyntax (T_NS_SEPARATOR)
      /// optional: true
      ///
      NsSeparator,
      ///
      /// type: NamespaceUnprefixedUseDeclarationSyntax
      /// optional: false
      ///
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
/// namespace_use_declaration_list_item:
///   ',' use_declaration
///
class NamespaceUseDeclarationListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      CommaToken,
      ///
      /// type: NamespaceUseDeclarationSyntax
      /// optional: false
      ///
      NamespaceUseDeclaration
   };


public:
   NamespaceUseDeclarationListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getComma();
   NamespaceUseDeclarationSyntax getNamespaceUseDeclaration();

   NamespaceUseDeclarationListItemSyntax withComma(std::optional<TokenSyntax> comma);
   NamespaceUseDeclarationListItemSyntax withNamespaceUseDeclaration(std::optional<NamespaceUseDeclarationSyntax> declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceUseDeclarationListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class NamespaceUseDeclarationListItemSyntaxBuilder;
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
      ///
      /// type: NamespaceUseTypeSyntax
      /// optional: true
      ///
      UseType,
      ///
      /// type: NamespaceUnprefixedUseDeclarationSyntax
      /// optional: false
      ///
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
   friend class NamespaceInlineUseDeclarationSyntaxBuiler;
   void validate();
};

///
/// namespace_inline_use_declaration_list_item:
/// ',' inline_use_declaration
///
class NamespaceInlineUseDeclarationListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      CommaToken,
      ///
      /// type: NamespaceInlineUseDeclarationSyntax
      /// optional: false
      ///
      NamespaceUseDeclaration
   };

public:
   NamespaceInlineUseDeclarationListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getCommaToken();
   NamespaceInlineUseDeclarationSyntax getNamespaceUseDeclaration();

   NamespaceInlineUseDeclarationListItemSyntax withCommaToken(std::optional<TokenSyntax> comma);
   NamespaceInlineUseDeclarationListItemSyntax withNamespaceUseDeclaration(std::optional<NamespaceInlineUseDeclarationSyntax> declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceInlineUseDeclarationListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceInlineUseDeclarationListItemSyntaxBuilder;
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
      /// type: TokenSyntax (T_NS_SEPARATOR)
      /// optional: true
      ///
      FirstNsSeparator,
      ///
      /// type: NamespaceNameSyntax
      /// optional: false
      ///
      Namespace,
      ///
      /// type: TokenSyntax (T_NS_SEPARATOR)
      /// optional: false
      ///
      SecondNsSeparator,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: NamespaceUnprefixedUseDeclarationListSyntax
      /// optional: false
      ///
      UnprefixedUseDeclarations,
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      CommaToken,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
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
   NamespaceNameSyntax getNamespace();
   TokenSyntax getSecondNsSeparator();
   TokenSyntax getLeftBrace();
   NamespaceUnprefixedUseDeclarationListSyntax getUnprefixedUseDeclarations();
   std::optional<TokenSyntax> getCommaToken();
   TokenSyntax getRightBrace();

   NamespaceGroupUseDeclarationSyntax withFirstNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceGroupUseDeclarationSyntax withNamespace(std::optional<NamespaceNameSyntax> ns);
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

///
/// mixed_group_use_declaration:
///   namespace_name T_NS_SEPARATOR '{' inline_use_declarations possible_comma '}'
/// | T_NS_SEPARATOR namespace_name T_NS_SEPARATOR '{' inline_use_declarations possible_comma '}'
///
class NamespaceMixedGroupUseDeclarationSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 7;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_NS_SEPARATOR)
      /// optional: true
      ///
      FirstNsSeparator,
      ///
      /// type: NamespacePartListSyntax
      /// optional: false
      ///
      Namespace,
      ///
      /// type: TokenSyntax (T_NS_SEPARATOR)
      /// optional: false
      ///
      SecondNsSeparator,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftBrace,
      ///
      /// type: NamespaceInlineUseDeclarationListSyntax
      /// optional: false
      ///
      InlineUseDeclarations,
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      CommaToken,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
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
   NamespaceNameSyntax getNamespace();
   TokenSyntax getSecondNsSeparator();
   TokenSyntax getLeftBrace();
   NamespaceInlineUseDeclarationListSyntax getInlineUseDeclarations();
   std::optional<TokenSyntax> getCommaToken();
   TokenSyntax getRightBrace();

   NamespaceMixedGroupUseDeclarationSyntax withFirstNsSeparator(std::optional<TokenSyntax> separator);
   NamespaceMixedGroupUseDeclarationSyntax withNamespace(std::optional<NamespaceNameSyntax> ns);
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
class NamespaceUseStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_USE)
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
      /// ------------------------------------------------------
      /// node choice: NamespaceMixedGroupUseDeclarationSyntax
      /// ------------------------------------------------------
      /// node choice: NamespaceGroupUseDeclarationSyntax
      /// ------------------------------------------------------
      /// node choice: NamespaceUseDeclarationListSyntax
      ///
      Declarations,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      SemicolonToken
   };

public:
   NamespaceUseStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getUseToken();
   std::optional<NamespaceUseTypeSyntax> getUseType();
   Syntax getDeclarations();
   TokenSyntax getSemicolon();

   NamespaceUseStmtSyntax withUseToken(std::optional<TokenSyntax> useToken);
   NamespaceUseStmtSyntax withUseType(std::optional<NamespaceUseTypeSyntax> useType);
   NamespaceUseStmtSyntax withDeclarations(std::optional<Syntax> declarations);
   NamespaceUseStmtSyntax withSemicolonToken(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceUseStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceUseStmtSyntaxBuilder;
   void validate();
};

///
/// namespace_definition_stmt:
///   T_NAMESPACE namespace_name ';'
///
class NamespaceDefinitionStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_NAMESPACE)
      /// optional: false
      ///
      NamespaceToken,
      ///
      /// type: NamespacePartListSyntax
      /// optional: false
      ///
      NamespaceName,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      SemicolonToken
   };

public:
   NamespaceDefinitionStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getNamespaceToken();
   NamespaceNameSyntax getNamespaceName();
   TokenSyntax getSemicolonToken();

   NamespaceDefinitionStmtSyntax withNamespaceToken(std::optional<TokenSyntax> namespaceToken);
   NamespaceDefinitionStmtSyntax withNamespaceName(std::optional<NamespaceNameSyntax> name);
   NamespaceDefinitionStmtSyntax withSemicolonToken(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceDefinitionStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceUseStmtSyntaxBuilder;
   void validate();
};

///
/// namespace_block_stmt:
///   T_NAMESPACE namespace_name '{' top_statement_list '}'
/// | T_NAMESPACE '{' top_statement_list '}'
///
class NamespaceBlockStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_NAMESPACE)
      /// optional: false
      ///
      NamespaceToken,
      ///
      /// type: NamespaceNameSyntax
      /// optional: true
      ///
      NamespaceName,
      ///
      /// type: TopCodeBlockStmtSyntax
      /// optional: false
      ///
      CodeBlock
   };

public:
   NamespaceBlockStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getNamespaceToken();
   std::optional<NamespaceNameSyntax> getNamespaceName();
   TopCodeBlockStmtSyntax getCodeBlock();

   NamespaceBlockStmtSyntax withNamespaceToken(std::optional<TokenSyntax> namespaceToken);
   NamespaceBlockStmtSyntax withNamespaceName(std::optional<NamespaceNameSyntax> name);
   NamespaceBlockStmtSyntax withCodeBlock(std::optional<TopCodeBlockStmtSyntax> codeBlock);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NamespaceDefinitionStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class NamespaceUseStmtSyntaxBuilder;
   void validate();
};

///
/// const_decl:
///   T_IDENTIFIER_STRING '=' expr
///
class ConstDeclareSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
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
   ConstDeclareSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getName();
   InitializerClauseSyntax getInitializer();

   ConstDeclareSyntax withName(std::optional<TokenSyntax> name);
   ConstDeclareSyntax withIntializer(std::optional<InitializerClauseSyntax> initializer);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConstDeclare;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConstDeclareSyntaxBuilder;
   void validate();
};

///
/// const_list_item:
///   ',' const_decl
///
class ConstListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: false
      ///
      CommaToken,
      ///
      /// type: ConstDeclareSyntax
      /// optional: false
      ///
      Declaration
   };


public:
   ConstListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getComma();
   ConstDeclareSyntax getDeclaration();

   ConstListItemSyntax withComma(std::optional<TokenSyntax> comma);
   ConstListItemSyntax withDeclaration(std::optional<ConstDeclareSyntax> declaration);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConstListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConstListItemSyntaxBuilder;
   void validate();
};

///
/// top_statement:
///   T_CONST const_list ';'
///
class ConstDefinitionStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_CONST)
      /// optional: false
      ///
      ConstToken,
      ///
      /// type: ConstDeclareListSyntax
      /// optional: false
      ///
      Declarations,
      ///
      /// type: TokenSyntax (T_SEMICOLON)
      /// optional: false
      ///
      Semicolon
   };

public:
   ConstDefinitionStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getConstToken();
   ConstDeclareListSyntax getDeclarations();
   TokenSyntax getSemicolon();

   ConstDefinitionStmtSyntax withConstToken(std::optional<TokenSyntax> constToken);
   ConstDefinitionStmtSyntax withDeclarations(std::optional<ConstDeclareListSyntax> declarations);
   ConstDefinitionStmtSyntax withSemicolon(std::optional<TokenSyntax> semicolon);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConstDefinitionStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConstDefinitionStmtSyntaxBuilder;
   void validate();
};

///
/// class_definition_stmt:
///   class_definition_decl
///
class ClassDefinitionStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ClassDefinitionSyntax
      /// optional: false
      ///
      ClassDefinition
   };

public:
   ClassDefinitionStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   ClassDefinitionSyntax getClassDefinition();
   ClassDefinitionStmtSyntax withClassDefinition(std::optional<ClassDefinitionSyntax> classDefinition);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassDefinitionStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassDefinitionStmtSyntaxBuilder;
   void validate();
};

///
/// interface_definition_stmt:
///   interface_definition_decl
///
class InterfaceDefinitionStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: InterfaceDefinitionSyntax
      /// optional: false
      ///
      InterfaceDefinition
   };

public:
   InterfaceDefinitionStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   InterfaceDefinitionSyntax getInterfaceDefinition();
   InterfaceDefinitionStmtSyntax withInterfaceDefinition(std::optional<InterfaceDefinitionSyntax> interfaceDefinition);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InterfaceDefinitionStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class InterfaceDefinitionStmtSyntaxBuilder;
   void validate();
};

///
/// trait_definition_stmt:
///   trait_definition_decl
///
class TraitDefinitionStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TraitDefinitionSyntax
      /// optional: false
      ///
      TraitDefinition
   };

public:
   TraitDefinitionStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   TraitDefinitionSyntax getTraitDefinition();
   TraitDefinitionStmtSyntax withTraitDefinition(std::optional<TraitDefinitionSyntax> traitDefinition);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::TraitDefinitionStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class TraitDefinitionStmtSyntaxBuilder;
   void validate();
};

///
/// function_definition_stmt:
///   function_definition_decl
///
class FunctionDefinitionStmtSyntax final : public StmtSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: FunctionDefinitionSyntax
      /// optional: false
      ///
      FunctionDefinition
   };

public:
   FunctionDefinitionStmtSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : StmtSyntax(root, data)
   {
      validate();
   }

   FunctionDefinitionSyntax getFunctionDefinition();
   FunctionDefinitionStmtSyntax withFunctionDefinition(std::optional<FunctionDefinitionSyntax> functionDefinition);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::FunctionDefinitionStmt;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class FunctionDefinitionStmtSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_STMT_NODES_H
