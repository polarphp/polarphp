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
#include "polarphp/syntax/TokenSyntax.h"
#include "polarphp/syntax/UnknownSyntax.h"
#include "polarphp/syntax/syntaxnode/CommonSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/ExprSyntaxNodesFwd.h"

namespace polar::syntax {

///
/// paren_decorated_expr：
///   '(' expr ')'
///
class ParenDecoratedExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken
   };

public:
   ParenDecoratedExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftParenToken();
   ExprSyntax getExpr();
   TokenSyntax getRightParenToken();

   ParenDecoratedExprSyntax withLeftParenToken(std::optional<TokenSyntax> leftParenToken);
   ParenDecoratedExprSyntax withExpr(std::optional<ExprSyntax> expr);
   ParenDecoratedExprSyntax withRightParenToken(std::optional<TokenSyntax> rightParenToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ParenDecoratedExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ParenDecoratedExprSyntaxBuilder;
   void validate();
};

class NullExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax (T_NULL)
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

///
/// optional_expr:
///   %empty
/// | expr
///
class OptionalExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 0;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: ExprSyntax
      /// optional: true
      Expr,
   };

public:
   OptionalExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   std::optional<ExprSyntax> getExpr();
   OptionalExprSyntax withExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::OptionalExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class OptionalExprSyntaxBuilder;
   void validate();
};

///
/// echo_expr:
///   expr
///
class EchoExprSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr
   };

public:
   EchoExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ExprSyntax getExpr();
   EchoExprSyntax withExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EchoExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EchoExprSyntaxBuilder;
   void validate();
};

///
/// echo_expr_list_item:
///   echo_expr ','
/// | echo_expr
///
class EchoExprListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: EchoExprSyntax
      /// optional: false
      ///
      EchoExpr,
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      TrailingComma,
   };


public:
   EchoExprListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   EchoExprSyntax getEchoExpr();
   std::optional<TokenSyntax> getTrailingComma();

   EchoExprListItemSyntax withEchoExpr(std::optional<EchoExprSyntax> echoExpr);
   EchoExprListItemSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EchoExprListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EchoExprListItemSyntaxBuilder;
   void validate();
};

///
/// variable:
///   callable_variable
/// | static_member
/// | dereferencable T_OBJECT_OPERATOR property_name
///
class VariableExprSyntax final : public ExprSyntax
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
      /// -------------------------------------------
      /// node choice: CallableVariableExprSyntax
      /// -------------------------------------------
      /// node choice: StaticPropertyExprSyntax
      /// -------------------------------------------
      /// node choice: InstancePropertyExprSyntax
      ///
      Var
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   VariableExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getVar();
   VariableExprSyntax withVar(std::optional<ExprSyntax> var);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::VariableExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class VariableExprSyntaxBuilder;
   void validate();
};

///
/// class_const_identifier:
///   class_name T_PAAMAYIM_NEKUDOTAYIM identifier
/// | variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier
///
class ClassConstIdentifierExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ----------------------------------------
      /// node choice: VariableClassNameClauseSyntax
      /// ----------------------------------------
      /// node choice: ClassNameClauseSyntax
      ///
      ClassName,
      ///
      /// type: TokenSyntax (T_PAAMAYIM_NEKUDOTAYIM)
      ///
      SeparatorToken,
      ///
      /// type: IdentifierSyntax
      /// optional: false
      ///
      Identifier
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ClassConstIdentifierExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getClassName();
   TokenSyntax getSeparatorToken();
   IdentifierSyntax getIdentifier();

   ClassConstIdentifierExprSyntax withClassName(std::optional<Syntax> className);
   ClassConstIdentifierExprSyntax withSeparatorToken(std::optional<TokenSyntax> separator);
   ClassConstIdentifierExprSyntax withIdentifier(std::optional<TokenSyntax> identifier);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassConstIdentifierExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassConstIdentifierExprSyntaxBuilder;
   void validate();
};

///
/// constant:
///   name
/// | class_name T_PAAMAYIM_NEKUDOTAYIM identifier
/// | variable_class_name T_PAAMAYIM_NEKUDOTAYIM identifier
///
class ConstExprSyntax final : public ExprSyntax
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
      /// ---------------------------------------
      /// node choice: NameSyntax
      /// ---------------------------------------
      /// node choice: ClassConstIdentifierExprSyntax
      Identifier
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ConstExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getIdentifier();
   ConstExprSyntax withIdentifier(std::optional<Syntax> identifier);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ConstExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ConstExprSyntaxBuilder;
   void validate();
};

///
/// new_variable:
///   simple_variable
/// | new_variable '[' optional_expr ']'
/// | new_variable '{' expr '}'
/// | new_variable T_OBJECT_OPERATOR property_name
/// | class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
/// | new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable
///
class NewVariableClauseSyntax final : public Syntax
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
      /// --------------------------------------------------
      /// node choice: SimpleVariableExprSyntax
      /// --------------------------------------------------
      /// node choice: ArrayAccessExprSyntax
      /// --------------------------------------------------
      /// node choice: BraceDecoratedArrayAccessExprSyntax
      /// --------------------------------------------------
      /// node choice: InstancePropertyExprSyntax
      /// --------------------------------------------------
      /// node choice: StaticPropertyExprSyntax
      ///
      VarNode
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   NewVariableClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ExprSyntax getVar();
   NewVariableClauseSyntax withVar(std::optional<ExprSyntax> var);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::NewVariableClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class NewVariableClauseSyntaxBuilder;
   void validate();
};

///
/// callable_variable:
///   simple_variable
/// | dereferencable '[' optional_expr ']'
/// | constant '[' optional_expr ']'
/// | dereferencable '{' expr '}'
/// | dereferencable T_OBJECT_OPERATOR property_name argument_list
/// | function_call
///
class CallableVariableExprSyntax final : public ExprSyntax
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
      /// ----------------------------------------------------
      /// node choice: SimpleVariableExprSyntax
      /// ----------------------------------------------------
      /// node choice: ArrayAccessExprSyntax
      /// ----------------------------------------------------
      /// node choice: BraceDecoratedArrayAccessExprSyntax
      /// ----------------------------------------------------
      /// node choice: InstanceMethodCallExprSyntax
      /// ----------------------------------------------------
      /// node choice: FunctionCallExprSyntax
      ///
      Var
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   CallableVariableExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getVar();
   CallableVariableExprSyntax withVar(std::optional<ExprSyntax> var);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::CallableVariableExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CallableVariableExprSyntaxBuilder;
   void validate();
};

///
/// callable_expr:
///   callable_variable
/// | '(' expr ')'
/// | dereferencable_scalar
///
class CallableFuncNameClauseSyntax final : public Syntax
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
      /// -----------------------------------------------
      /// node choice: CallableVariableExprSyntax
      /// -----------------------------------------------
      /// node choice: ParenDecoratedExprSyntax
      /// -----------------------------------------------
      /// node choice: DereferencableScalarExprSyntax
      ///
      FuncName
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   CallableFuncNameClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getFuncName();
   CallableFuncNameClauseSyntax withFuncName(std::optional<Syntax> funcName);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::CallableFuncNameClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CallableFuncNameClauseSyntaxBuilder;
   void validate();
};

///
/// member_name:
///   identifier
/// | '{' expr '}'
/// | simple_variable
///
class MemberNameClauseSyntax final : public Syntax
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
      /// ---------------------------------------------
      /// node choice: IdentifierSyntax
      /// ---------------------------------------------
      /// node choice: BraceDecoratedExprClauseSyntax
      /// ---------------------------------------------
      /// node choice: SimpleVariableExprSyntax
      ///
      Name
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   MemberNameClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getName();
   MemberNameClauseSyntax withName(std::optional<Syntax> name);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::MemberNameClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class MemberNameClauseSyntaxBuilder;
   void validate();
};

///
/// property_name:
///   T_IDENTIFIER_STRING
/// | '{' expr '}'
/// | simple_variable
///
class PropertyNameClauseSyntax final : public Syntax
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
      /// ------------------------------------------------
      /// node choice: TokenSyntax (T_IDENTIFIER_STRING)
      /// ------------------------------------------------
      /// node choice: BraceDecoratedExprClauseSyntax
      /// ------------------------------------------------
      /// node choice: SimpleVariableExprSyntax
      ///
      Name
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   PropertyNameClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getName();
   PropertyNameClauseSyntax withName(std::optional<Syntax> name);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::PropertyNameClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class PropertyNameClauseSyntaxBuilder;
   void validate();
};

///
/// object_property_access:
///   new_variable T_OBJECT_OPERATOR property_name
/// | dereferencable T_OBJECT_OPERATOR property_name
/// | T_VARIABLE T_OBJECT_OPERATOR T_IDENTIFIER_STRING
///
class InstancePropertyExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ----------------------------------------
      /// node choice: TokenSyntax (T_VARIABLE)
      /// ----------------------------------------
      /// node choice: NewVariableSyntax
      /// ----------------------------------------
      /// node choice: DereferencableClauseSyntax
      ///
      ObjectRef,
      ///
      /// type: TokenSyntax (T_OBJECT_OPERATOR)
      /// optional: false
      ///
      Separator,
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ------------------------------------------------
      /// node choice: TokenSyntax (T_IDENTIFIER_STRING)
      /// ------------------------------------------------
      /// node choice: PropertyNameClauseSyntax
      ///
      PropertyName
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   InstancePropertyExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getObjectRef();
   TokenSyntax getSeparator();
   Syntax getPropertyName();

   InstancePropertyExprSyntax withObjectRef(std::optional<Syntax> objectRef);
   InstancePropertyExprSyntax withSeparator(std::optional<TokenSyntax> separator);
   InstancePropertyExprSyntax withPropertyName(std::optional<Syntax> propertyName);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InstancePropertyExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class InstancePropertyExprSyntaxBuilder;
   void validate();
};

///
/// static_member:
///   class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
/// | variable_class_name T_PAAMAYIM_NEKUDOTAYIM simple_variable
/// | new_variable T_PAAMAYIM_NEKUDOTAYIM simple_variable
///
class StaticPropertyExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// --------------------------------------------
      /// node choice: ClassNameClauseSyntax
      /// --------------------------------------------
      /// node choice: VariableClassNameClauseSyntax
      /// --------------------------------------------
      /// node choice: NewVariableClauseSyntax
      ///
      ClassName,
      ///
      /// type: TokenSyntax (T_PAAMAYIM_NEKUDOTAYIM)
      /// optional: false
      ///
      Separator,
      ///
      /// type: SimpleVariableExprSyntax
      /// optional: false
      ///
      MemberName
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   StaticPropertyExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getClassName();
   TokenSyntax getSeparator();
   SimpleVariableExprSyntax getMemberName();

   StaticPropertyExprSyntax withClassName(std::optional<Syntax> className);
   StaticPropertyExprSyntax withSeparator(std::optional<TokenSyntax> separator);
   StaticPropertyExprSyntax withMemberName(std::optional<SimpleVariableExprSyntax> memberName);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::StaticPropertyExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class StaticPropertyExprSyntaxBuilder;
   void validate();
};

///
/// argument:
///   expr
/// | T_ELLIPSIS expr
///
class ArgumentSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ELLIPSIS)
      /// optional: true
      ///
      EllipsisToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr
   };

public:
   ArgumentSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getEllipsisToken();
   ExprSyntax getExpr();
   ArgumentSyntax withEllipsisToken(std::optional<TokenSyntax> ellipsisToken);
   ArgumentSyntax withExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::Argument;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ArgumentSyntaxBuilder;
   void validate();
};

///
/// argument_list_item:
///   argument ','
///
class ArgumentListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ArgumentSyntax
      /// optional: false
      ///
      Argument,
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      TrailingComma
   };

public:
   ArgumentListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ArgumentSyntax getArgument();
   std::optional<TokenSyntax> getTokenSyntax();

   ArgumentListItemSyntax withArgument(std::optional<ArgumentSyntax> argument);
   ArgumentListItemSyntax withTrailingComma(std::optional<TokenSyntax> comma);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ArgumentListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ArgumentListItemSyntaxBuilder;
   void validate();
};

///
/// argument_list:
///   '(' ')'
/// | '(' non_empty_argument_list possible_comma ')'
///
class ArgumentListClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: ArgumentListSyntax
      /// optional: true
      ///
      Arguments,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken
   };

public:
   ArgumentListClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftParenToken();
   std::optional<ArgumentListSyntax> getArguments();
   TokenSyntax getRightParenToken();

   ArgumentListClauseSyntax withLeftParenToken(std::optional<TokenSyntax> leftParenToken);
   ArgumentListClauseSyntax withArguments(std::optional<ArgumentListSyntax> arguments);
   ArgumentListClauseSyntax withRightParenToken(std::optional<TokenSyntax> rightParenToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ArgumentListClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ArgumentListClauseSyntaxBuilder;
   void validate();
};

///
/// dereferencable_clause:
///   variable
/// | '(' expr ')'
/// | dereferencable_scalar
///
class DereferencableClauseSyntax final : public Syntax
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
      /// ---------------------------------------------
      /// node choice: VariableExprSyntax
      /// ---------------------------------------------
      /// node choice: ParenDecoratedExprSyntax
      /// ---------------------------------------------
      /// node choice: DereferencableScalarExprSyntax
      ///
      DereferencableExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   DereferencableClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ExprSyntax getDereferencableExpr();
   DereferencableClauseSyntax withDereferencableExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::DereferencableClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class DereferencableClauseSyntaxBuilder;
   void validate();
};

///
/// variable_class_name:
///   dereferencable
///
class VariableClassNameClauseSyntax final : Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      DereferencableExpr
   };

public:
   VariableClassNameClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ExprSyntax getDereferencableExpr();
   VariableClassNameClauseSyntax withDereferencableExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::VariableClassNameClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class VariableClassNameClauseSyntaxBuilder;
   void validate();
};

///
/// class_name:
///   T_STATIC
/// | name
///
class ClassNameClauseSyntax final : public Syntax
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
      /// ---------------------------------------
      /// node choice: TokenSyntax (T_STATIC)
      /// ---------------------------------------
      /// node choice: NameExprSyntax
      ///
      Name
   };

public:
   ClassNameClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getName();
   ClassNameClauseSyntax withName(std::optional<Syntax> name);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassNameClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassNameClauseSyntaxBuilder;
   void validate();
};

///
/// class_name_reference:
///   class_name
/// | new_variable
///
class ClassNameRefClauseSyntax final : public Syntax
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
      /// ------------------------------------------
      /// node choice: ClassNameClauseSyntax
      /// ------------------------------------------
      /// node choice: NewVariableClauseSyntax
      ///
      Name
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ClassNameRefClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getName();
   ClassNameRefClauseSyntax withName(std::optional<Syntax> name);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassNameRefClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ClassNameRefClauseSyntaxBuilder;
   void validate();
};

///
/// brace_decorated_expr_clause:
///   '{' expr '}'
///
class BraceDecoratedExprClauseSyntax final : public Syntax
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
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      RightBrace
   };

public:
   BraceDecoratedExprClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftBrace();
   ExprSyntax getExpr();
   TokenSyntax getRightBrace();

   BraceDecoratedExprClauseSyntax withLeftBrace(std::optional<TokenSyntax> leftBrace);
   BraceDecoratedExprClauseSyntax withExpr(std::optional<ExprSyntax> expr);
   BraceDecoratedExprClauseSyntax withRightBrace(std::optional<TokenSyntax> rightBrace);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BraceDecoratedExprClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class BraceDecoratedExprClauseSyntaxBuilder;
   void validate();
};

///
/// expr:
///   '$' '{' expr '}'
///
class BraceDecoratedVariableExprSyntax final : public ExprSyntax
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
      DollarSign,
      ///
      /// type: BraceDecoratedExprClauseSyntax
      /// optional: false
      ///
      DecoratedExpr
   };

public:
   BraceDecoratedVariableExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getDollarSign();
   BraceDecoratedExprClauseSyntax getDecoratedExpr();

   BraceDecoratedVariableExprSyntax withDollarSign(std::optional<TokenSyntax> dollarSign);
   BraceDecoratedVariableExprSyntax withDecoratedExpr(std::optional<BraceDecoratedExprClauseSyntax> decoratedExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BraceDecoratedVariableExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class BraceDecoratedVariableExprSyntaxBuilder;
   void validate();
};

///
/// array_key_value_pair_item:
///   expr T_DOUBLE_ARROW expr
/// | expr
/// | expr T_DOUBLE_ARROW '&' variable
/// | '&' variable
///
class ArrayKeyValuePairItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      KeyExpr,
      ///
      /// type: TokenSyntax (T_DOUBLE_ARROW)
      /// optional: true
      ///
      DoubleArrowToken,
      ///
      /// type: TokenSyntax (T_AMPERSAND)
      /// optional: true
      ///
      ReferenceToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      /// node choices: true
      /// --------------------------------
      /// node choice: VariableExprSyntax
      /// --------------------------------
      /// node choice: ExprSyntax
      ///
      Value
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ArrayKeyValuePairItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<ExprSyntax> getKeyExpr();
   std::optional<TokenSyntax> getDoubleArrowToken();
   std::optional<TokenSyntax> getReferenceToken();
   ExprSyntax getValue();

   ArrayKeyValuePairItemSyntax withKeyExpr(std::optional<ExprSyntax> keyExpr);
   ArrayKeyValuePairItemSyntax withDoubleArrowToken(std::optional<TokenSyntax> doubleArrowToken);
   ArrayKeyValuePairItemSyntax withReferenceToken(std::optional<TokenSyntax> referenceToken);
   ArrayKeyValuePairItemSyntax withValue(std::optional<ExprSyntax> value);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ArrayKeyValuePairItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ArrayKeyValuePairItemSyntaxBuilder;
   void validate();
};

///
/// array_unpack_pair_item:
///   T_ELLIPSIS expr
///
class ArrayUnpackPairItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ELLIPSIS)
      /// optional: false
      ///
      EllipsisToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      UnpackExpr
   };

public:
   ArrayUnpackPairItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getEllipsisToken();
   ExprSyntax getUnpackExpr();

   ArrayUnpackPairItemSyntax withEllipsisToken(std::optional<TokenSyntax> ellipsisToken);
   ArrayUnpackPairItemSyntax withUnpackExpr(std::optional<ExprSyntax> unpackExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ArrayUnpackPairItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ArrayUnpackPairItemSyntaxBuilder;
   void validate();
};

///
/// array_pair_item:
///   array_key_value_pair_item
/// | array_unpack_pair_item
///
class ArrayPairItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// -----------------------------------------
      /// node choice: ArrayKeyValuePairItemSyntax
      /// -----------------------------------------
      /// node choice: ArrayUnpackPairItemSyntax
      ///
      Item,
      ///
      /// type: TokenSyntax (T_COMMA)
      ///
      TrailingComma
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ArrayPairItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getItem();
   std::optional<TokenSyntax> getTrailingComma();

   ArrayPairItemSyntax withItem(std::optional<Syntax> item);
   ArrayPairItemSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ArrayPairItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ArrayPairItemSyntaxBuilder;
   void validate();
};

///
/// list_recursive_pair_item:
///   expr T_DOUBLE_ARROW T_LIST '(' list_pair_item_list ')'
/// | T_LIST '(' list_pair_item_list ')'
///
class ListRecursivePairItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 6;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      KeyExpr,
      ///
      /// type: TokenSyntax (T_DOUBLE_ARROW)
      /// optional: true
      ///
      DoubleArrowToken,
      ///
      /// type: TokenSyntax (T_LIST)
      /// optional: false
      ///
      ListToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ListPairItemListSyntax
      /// optional: false
      ///
      ListPairItemList,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen
   };

public:
   ListRecursivePairItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<ExprSyntax> getKeyExpr();
   std::optional<TokenSyntax> getDoubleArrowToken();
   TokenSyntax getListToken();
   TokenSyntax getLeftParen();
   ListPairItemListSyntax getListPairItemList();
   TokenSyntax getRightParen();

   ListRecursivePairItemSyntax withKeyExpr(std::optional<ExprSyntax> keyExpr);
   ListRecursivePairItemSyntax withDoubleArrowToken(std::optional<TokenSyntax> doubleArrowToken);
   ListRecursivePairItemSyntax withListToken(std::optional<TokenSyntax> listToken);
   ListRecursivePairItemSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   ListRecursivePairItemSyntax withListPairItemList(std::optional<ListPairItemListSyntax> pairItemList);
   ListRecursivePairItemSyntax withRightParen(std::optional<TokenSyntax> rightParen);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ListRecursivePairItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ListRecursivePairItemSyntaxBuilder;
   void validate();
};

///
/// list_pair_item:
///   array_pair_item
/// | list_recursive_pair_item
///
class ListPairItemSyntax final : public Syntax
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
      /// ------------------------------------------
      /// node choice: ArrayPairItemSyntax
      /// ------------------------------------------
      /// node choice: ListRecursivePairItemSyntax
      ///
      Item,
      ///
      /// type: TokenSyntax (T_COMMA)
      ///
      TrailingComma
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ListPairItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getItem();
   std::optional<TokenSyntax> getTrailingComma();

   ListPairItemSyntax withItem(std::optional<Syntax> item);
   ListPairItemSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

private:
   friend class ListPairItemSyntaxBuilder;
   void validate();
};

///
/// simple_variable:
///   T_VARIABLE
/// | brace_decorated_variable_expr
/// | '$' simple_variable
///
class SimpleVariableExprSyntax final : public ExprSyntax
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
      DollarSign,
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ----------------------------------------------
      /// node choice: TokenSyntax (T_VARIABLE)
      /// ----------------------------------------------
      /// node choice: BraceDecoratedVariableExprSyntax
      /// ----------------------------------------------
      /// node choice: SimpleVariableExprSyntax
      ///
      Variable,
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   SimpleVariableExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getDollarSign();
   Syntax getVariable();

   SimpleVariableExprSyntax withDollarSign(std::optional<TokenSyntax> dollarSign);
   SimpleVariableExprSyntax withVariable(std::optional<Syntax> variable);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SimpleVariableExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SimpleVariableExprSyntaxBuilder;
   void validate();
};

///
/// array_expr:
///   T_ARRAY '(' array_pair_list ')'
///
class ArrayCreateExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ARRAY)
      /// optional: false
      ///
      ArrayToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParen,
      ///
      /// type: ArrayPairItemListSyntax
      /// optional: false
      ///
      PairItemList,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParen
   };

public:
   ArrayCreateExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getArrayToken();
   TokenSyntax getLeftParen();
   ArrayPairItemListSyntax getPairItemList();
   TokenSyntax getRightParen();

   ArrayCreateExprSyntax withArrayToken(std::optional<TokenSyntax> arrayToken);
   ArrayCreateExprSyntax withLeftParen(std::optional<TokenSyntax> leftParen);
   ArrayCreateExprSyntax withPairItemList(std::optional<ArrayPairItemListSyntax> pairItemList);
   ArrayCreateExprSyntax withRightParen(std::optional<TokenSyntax> rightParen);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ArrayCreateExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ArrayCreateExprSyntaxBuilder;
   void validate();
};

///
/// simplified_array_expr:
///   '[' array_pair_list ']'
///
class SimplifiedArrayCreateExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_SQUARE_BRACKET)
      /// optional: false
      ///
      LeftSquareBracket,
      ///
      /// type: ArrayPairItemListSyntax
      /// optional: false
      ///
      PairItemList,
      ///
      /// type: TokenSyntax (T_RIGHT_SQUARE_BRACKET)
      /// optional: false
      ///
      RightSquareBracket
   };

public:
   SimplifiedArrayCreateExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftSquareBracket();
   ArrayPairItemListSyntax getPairItemList();
   TokenSyntax getRightSquareBracket();

   SimplifiedArrayCreateExprSyntax withLeftSquareBracket(std::optional<TokenSyntax> leftSquareBracket);
   SimplifiedArrayCreateExprSyntax withPairItemList(std::optional<ArrayPairItemListSyntax> pairItemList);
   SimplifiedArrayCreateExprSyntax withRightSquareBracket(std::optional<TokenSyntax> rightSquareBracket);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SimplifiedArrayCreateExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SimplifiedArrayCreateExprSyntaxBuilder;
   void validate();
};

///
/// array_access:
///   T_VARIABLE '[' encaps_var_offset ']'
/// | new_variable '[' optional_expr ']'
/// | constant '[' optional_expr ']'
/// | dereferencable '[' optional_expr ']'
///
class ArrayAccessExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ----------------------------------------
      /// node choice: TokenSyntax (T_VARIABLE)
      /// ----------------------------------------
      /// node choice: NewVariableClauseSyntax
      /// ----------------------------------------
      /// node choice: DereferencableClauseSyntax
      /// ----------------------------------------
      /// node choice: ConstExprSyntax
      ///
      ArrayRef,
      ///
      /// type: TokenSyntax (T_LEFT_SQUARE_BRACKET)
      /// optional: false
      ///
      LeftSquareBracket,
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ----------------------------------------
      /// node choice: EncapsVarOffsetSyntax
      /// ----------------------------------------
      /// node choice: OptionalExprSyntax
      ///
      Offset,
      ///
      /// type: TokenSyntax (T_RIGHT_SQUARE_BRACKET)
      /// optional: false
      ///
      RightSquareBracket
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ArrayAccessExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getArrayRef();
   TokenSyntax getLeftSquareBracket();
   Syntax getOffset();
   TokenSyntax getRightSquareBracket();

   ArrayAccessExprSyntax withArrayRef(std::optional<Syntax> arrayRef);
   ArrayAccessExprSyntax withLeftSquareBracket(std::optional<TokenSyntax> leftSquareBracket);
   ArrayAccessExprSyntax withOffset(std::optional<Syntax> offset);
   ArrayAccessExprSyntax withRightSquareBracket(std::optional<TokenSyntax> rightSquareBracket);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ArrayAccessExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class ArrayAccessExprSyntaxBuilder;
   void validate();
};

///
/// brace_decorated_array_access:
///   new_variable '{' expr '}'
/// | dereferencable '{' expr '}'
///
class BraceDecoratedArrayAccessExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// -----------------------------------------
      /// node choice: NewVariableClauseSyntax
      /// -----------------------------------------
      /// node choice: DereferencableClauseSyntax
      ///
      ArrayRef,
      ///
      /// type: BraceDecoratedExprClauseSyntax
      /// optional: false
      ///
      OffsetExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   BraceDecoratedArrayAccessExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getArrayRef();
   BraceDecoratedExprClauseSyntax getOffsetExpr();

   BraceDecoratedArrayAccessExprSyntax withArrayRef(std::optional<Syntax> arrayRef);
   BraceDecoratedArrayAccessExprSyntax withOffsetExpr(std::optional<BraceDecoratedExprClauseSyntax> offsetExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BraceDecoratedArrayAccessExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class BraceDecoratedArrayAccessExprBuilder;
   void validate();
};

///
/// simple_function_call:
///   name argument_list
/// | callable_expr argument_list
///
class SimpleFunctionCallExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// --------------------------------------------
      /// node choice: NameSyntax
      /// --------------------------------------------
      /// node choice: CallableFuncNameClauseSyntax
      ///
      FuncName,
      ///
      /// type: ArgumentListClauseSyntax
      /// optional: false
      ///
      ArgumentsClause
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   SimpleFunctionCallExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getFuncName();
   ArgumentListClauseSyntax getArgumentsClause();

   SimpleFunctionCallExprSyntax withFuncName(std::optional<Syntax> funcName);
   SimpleFunctionCallExprSyntax withArgumentsClause(std::optional<ArgumentListClauseSyntax> argumentsClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SimpleFunctionCallExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class SimpleFunctionCallExprSyntaxBuilder;
   void validate();
};

///
/// function_call:
///   name argument_list
/// | class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
/// | variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
/// | callable_expr argument_list
///
class FunctionCallExprSyntax final : public ExprSyntax
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
      /// -------------------------------------------
      /// node choice: SimpleFunctionCallExprSyntax
      /// -------------------------------------------
      /// node choice: StaticMethodCallExprSyntax
      ///
      Callable
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   FunctionCallExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getCallable();
   FunctionCallExprSyntax withCallable(std::optional<ExprSyntax> callable);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::FunctionCallExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class FunctionCallExprSyntaxBuilder;
   void validate();
};

///
/// instance_method_call:
///   dereferencable T_OBJECT_OPERATOR property_name argument_list
///
class InstanceMethodCallExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: InstancePropertyExprSyntax
      /// optional: false
      ///
      QualifiedMethodName,
      ///
      /// type: ArgumentListClauseSyntax
      /// optional: false
      ///
      ArgumentListClause
   };

public:
   InstanceMethodCallExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   InstancePropertyExprSyntax getQualifiedMethodName();
   ArgumentListClauseSyntax getArgumentListClause();

   InstanceMethodCallExprSyntax withQualifiedMethodName(std::optional<InstancePropertyExprSyntax> methodName);
   InstanceMethodCallExprSyntax withArgumentListClause(std::optional<ArgumentListClauseSyntax> arguments);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InstanceMethodCallExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class InstanceMethodCallExperSyntaxBuilder;
   void validate();
};

///
/// static_method_call:
///   class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
/// | variable_class_name T_PAAMAYIM_NEKUDOTAYIM member_name argument_list
///
class StaticMethodCallExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// --------------------------------------------
      /// node choice: ClassNameClauseSyntax
      /// --------------------------------------------
      /// node choice: VariableClassNameClauseSyntax
      ///
      ClassName,
      ///
      /// type: TokenSyntax (T_PAAMAYIM_NEKUDOTAYIM)
      /// optional: false
      ///
      Separator,
      ///
      /// type: MemberNameClauseSyntax
      /// optional: false
      ///
      MethodName,
      ///
      /// type: ArgumentListClauseSyntax
      /// optional: false
      ///
      Arguments
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   StaticMethodCallExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getClassName();
   TokenSyntax getSeparator();
   MemberNameClauseSyntax getMethodName();
   ArgumentListClauseSyntax getArguments();

   StaticMethodCallExprSyntax withClassName(std::optional<Syntax> className);
   StaticMethodCallExprSyntax withSeparator(std::optional<TokenSyntax> separator);
   StaticMethodCallExprSyntax withMethodName(std::optional<MemberNameClauseSyntax> methodName);
   StaticMethodCallExprSyntax withArguments(std::optional<ArgumentListClauseSyntax> arguments);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::StaticMethodCallExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class StaticMethodCallExprSyntaxBuilder;
   void validate();
};

///
/// dereferencable_scalar:
///   T_ARRAY '(' array_pair_list ')'
/// | '[' array_pair_list ']'
/// | T_CONSTANT_ENCAPSED_STRING
///
class DereferencableScalarExprSyntax final : public ExprSyntax
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
      /// ---------------------------------------------
      /// node choice: ArrayCreateExprSyntax
      /// ---------------------------------------------
      /// node choice: SimplifiedArrayCreateExprSyntax
      /// ---------------------------------------------
      /// node choice: TokenSyntax (T_CONSTANT_ENCAPSED_STRING)
      ///
      ScalarValue
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   DereferencableScalarExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getScalarValue();
   DereferencableScalarExprSyntax withScalarValue(std::optional<Syntax> scalarValue);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::DereferencableScalarExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class DereferencableScalarExprSyntaxBuilder;
   void validate();
};

///
/// anonymous_class:
///   T_CLASS ctor_arguments
///   extends_from implements_list backup_doc_comment '{' class_statement_list '}'
///
class AnonymousClassDefinitionClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_CLASS)
      /// optional: false
      ///
      ClassToken,
      ///
      /// type: ArgumentListClauseSyntax
      /// optional: true
      ///
      CtorArguments,
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
   AnonymousClassDefinitionClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getClassToken();
   std::optional<ArgumentListClauseSyntax> getCtorArguments();
   ExtendsFromClauseSyntax getExtendsFrom();
   ImplementClauseSyntax getImplementsList();
   MemberDeclBlockSyntax getMembers();

   AnonymousClassDefinitionClauseSyntax withClassToken(std::optional<TokenSyntax> classToken);
   AnonymousClassDefinitionClauseSyntax withCtorArguments(std::optional<ArgumentListClauseSyntax> ctorArguments);
   AnonymousClassDefinitionClauseSyntax withExtendsFrom(std::optional<ExtendsFromClauseSyntax> extends);
   AnonymousClassDefinitionClauseSyntax withImplementsList(std::optional<ImplementClauseSyntax> implements);
   AnonymousClassDefinitionClauseSyntax withMembers(std::optional<MemberDeclBlockSyntax> members);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::AnonymousClassDefinitionClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class AnonymousClassDefinitionSyntaxBuilder;
   void validate();
};

///
/// simple_instance_create_expr:
///   T_NEW class_name_reference ctor_arguments
///
class SimpleInstanceCreateExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_NEW)
      /// optional: false
      ///
      NewToken,
      ///
      /// type: ClassNameRefClauseSyntax
      /// optional: false
      ///
      ClassName,
      ///
      /// type: ArgumentListClauseSyntax
      /// optional: true
      ///
      CtorArgsClause
   };

public:
   SimpleInstanceCreateExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getNewToken();
   ClassNameRefClauseSyntax getClassName();
   std::optional<ArgumentListClauseSyntax> getCtorArgsClause();

   SimpleInstanceCreateExprSyntax withNewToken(std::optional<TokenSyntax> newToken);
   SimpleInstanceCreateExprSyntax withClassName(std::optional<ClassNameRefClauseSyntax> className);
   SimpleInstanceCreateExprSyntax withCtorArgsClause(std::optional<ArgumentListClauseSyntax> argsClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SimpleInstanceCreateExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SimpleInstanceCreateExprSyntaxBuilder;
   void validate();
};

///
/// anonymous_instance_create:
///   T_NEW anonymous_class
///
class AnonymousInstanceCreateExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_NEW)
      /// optional: false
      ///
      NewToken,
      ///
      /// type: AnonymousClassDefinitionClauseSyntax
      /// optional: false
      ///
      AnonymousClassDef
   };

public:
   AnonymousInstanceCreateExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getNewToken();
   AnonymousClassDefinitionClauseSyntax getAnonymousClassDef();

   AnonymousInstanceCreateExprSyntax withNewToken(std::optional<TokenSyntax> newToken);
   AnonymousInstanceCreateExprSyntax withAnonymousClassDef(std::optional<AnonymousClassDefinitionClauseSyntax> classDef);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::AnonymousInstanceCreateExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class AnonymousInstanceCreateExprSyntaxBuilder;
   void validate();
};

///
/// inline_function:
///   function returns_ref backup_doc_comment '(' parameter_list ')' lexical_vars return_type
///   backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
///
class ClassicLambdaExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 6;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_FUNCTION)
      /// optional: false
      ///
      FuncToken,
      ///
      /// type: TokenSyntax (T_AMPERSAND)
      /// optional: true
      ///
      ReturnRefToken,
      ///
      /// type: ParameterClauseSyntax
      /// optional: false
      ///
      ParameterListClause,
      ///
      /// type: UseLexicalVarClauseSyntax
      /// optional: true
      ///
      LexicalVarsClause,
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
   ClassicLambdaExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getFuncToken();
   std::optional<TokenSyntax> getReturnRefToken();
   ParameterClauseSyntax getParameterListClause();
   std::optional<UseLexicalVarClauseSyntax> getLexicalVarsClause();
   std::optional<ReturnTypeClauseSyntax> getReturnType();
   CodeBlockSyntax getBody();

   ClassicLambdaExprSyntax withFuncToken(std::optional<TokenSyntax> funcToken);
   ClassicLambdaExprSyntax withReturnRefToken(std::optional<TokenSyntax> returnRefToken);
   ClassicLambdaExprSyntax withParameterListClause(std::optional<ParameterClauseSyntax> parameterListClause);
   ClassicLambdaExprSyntax withLexicalVarsClause(std::optional<UseLexicalVarClauseSyntax> lexicalVars);
   ClassicLambdaExprSyntax withReturnType(std::optional<ReturnTypeClauseSyntax> returnType);
   ClassicLambdaExprSyntax withBody(std::optional<CodeBlockSyntax> body);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ClassicLambdaExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ClassicLambdaExprSyntaxBuilder;
   void validate();
};


///
/// inline_function:
///   fn returns_ref '(' parameter_list ')' return_type backup_doc_comment T_DOUBLE_ARROW backup_fn_flags backup_lex_pos expr backup_fn_flags
///
class SimplifiedLambdaExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 6;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_AMPERSAND)
      /// optional: false
      ///
      FnToken,
      ///
      /// type: TokenSyntax (T_AMPERSAND)
      /// optional: true
      ///
      ReturnRefToken,
      ///
      /// type: ParameterClauseSyntax
      /// optional: false
      ///
      ParameterListClause,
      ///
      /// type: ReturnTypeClauseSyntax
      /// optional: true
      ///
      ReturnType,
      ///
      /// type: TokenSyntax (T_DOUBLE_ARROW)
      /// optional: false
      ///
      DoubleArrowToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Body
   };

public:
   SimplifiedLambdaExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getFnToken();
   std::optional<TokenSyntax> getReturnRefToken();
   ParameterClauseSyntax getParameterListClause();
   std::optional<ReturnTypeClauseSyntax> getReturnType();
   TokenSyntax getDoubleArrowToken();
   ExprSyntax getBody();

   SimplifiedLambdaExprSyntax withFnToken(std::optional<TokenSyntax> fnToken);
   SimplifiedLambdaExprSyntax withReturnRefToken(std::optional<TokenSyntax> returnRefToken);
   SimplifiedLambdaExprSyntax withParameterListClause(std::optional<ParameterClauseSyntax> parameterListClause);
   SimplifiedLambdaExprSyntax withReturnType(std::optional<ReturnTypeClauseSyntax> returnType);
   SimplifiedLambdaExprSyntax withDoubleArrowToken(std::optional<TokenSyntax> doubleArrow);
   SimplifiedLambdaExprSyntax withBody(std::optional<ExprSyntax> body);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::SimplifiedLambdaExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class SimplifiedLambdaExprSyntaxBuilder;
   void validate();
};

///
/// lambda_expr:
///   function returns_ref backup_doc_comment '(' parameter_list ')' lexical_vars return_type
///   backup_fn_flags '{' inner_statement_list '}' backup_fn_flags
/// | fn returns_ref '(' parameter_list ')' return_type backup_doc_comment T_DOUBLE_ARROW backup_fn_flags backup_lex_pos expr backup_fn_flags
///
class LambdaExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_STATIC)
      /// optional: true
      ///
      StaticToken,
      ///
      /// type: ExprSyntax
      /// optional: true
      /// node choices: true
      /// ------------------------------------------
      /// node choice: ClassicLambdaExprSyntax
      /// ------------------------------------------
      /// node choice: SimplifiedLambdaExprSyntax
      ///
      LambdaExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   LambdaExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getStaticToken();
   ExprSyntax getLambdaExpr();

   LambdaExprSyntax withStaticToken(std::optional<TokenSyntax> staticToken);
   LambdaExprSyntax withLambdaExpr(std::optional<ExprSyntax> lambdaExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::LambdaExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class LambdaExprSyntaxBuilder;
   void validate();
};

///
/// new_expr:
///   T_NEW class_name_reference ctor_arguments
/// | T_NEW anonymous_class
///
class InstanceCreateExprSyntax final : public ExprSyntax
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
      /// --------------------------------------------------------
      /// node choice: AnonymousClassDefinitionClauseSyntax
      /// --------------------------------------------------------
      /// node choice: SimpleInstanceCreateExprSyntax
      ///
      CreateExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   InstanceCreateExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getCreateExpr();
   InstanceCreateExprSyntax withCreateExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::InstanceCreateExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class InstanceCreateExprSyntaxBuilder;
   void validate();
};

///
/// scalar:
///   T_LNUMBER
/// | T_DNUMBER
/// | T_LINE
/// | T_FILE
/// | T_DIR
/// | T_TRAIT_CONST
/// | T_METHOD_CONST
/// | T_FUNC_CONST
/// | T_NS_CONST
/// | T_CLASS_CONST
/// | T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC
/// | T_START_HEREDOC encaps_list T_END_HEREDOC
/// | T_START_HEREDOC T_END_HEREDOC
/// | '"' encaps_list '"'
/// | dereferencable_scalar
/// | constant
///
class ScalarExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ScalarExpr;
   }

   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// --------------------------------------------
      /// node choice: TokenSyntax
      /// token choices:
      /// T_LNUMBER | T_DNUMBER | T_LINE
      /// T_FILE | T_DIR | T_TRAIT_CONST
      /// T_METHOD_CONST | T_FUNC_CONST | T_NS_CONST
      /// T_CLASS_CONST
      /// --------------------------------------------
      /// node choice: HeredocExprSyntax
      /// --------------------------------------------
      /// node choice: EncapsListStringExprSyntax
      /// --------------------------------------------
      /// node choice: DereferencableScalarExprSyntax
      /// --------------------------------------------
      /// node choice: ConstExprSyntax
      ///
      Value
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   ScalarExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   Syntax getValue();
   ScalarExprSyntax withValue(std::optional<Syntax> value);

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ScalarExprSyntaxBuilder;
   void validate();
};

class ClassRefParentExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax (T_CLASS_REF_PARENT)
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax (T_CLASS_REF_SELF)
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax (T_CLASS_REF_STATIC)
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax (T_LNUMBER)
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax (T_DNUMBER)
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

///
/// string_literal:
///   '"' T_CONSTANT_ENCAPSED_STRING '"'
/// | '\'' T_CONSTANT_ENCAPSED_STRING '\''
///
class StringLiteralExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_SINGLE_QUOTE|T_DOUBLE_QUOTE)
      /// optional: false
      ///
      LeftQuote,
      ///
      /// type: TokenSyntax (T_CONSTANT_ENCAPSED_STRING)
      /// optional: false
      ///
      Text,
      ///
      /// type: TokenSyntax (T_SINGLE_QUOTE|T_DOUBLE_QUOTE)
      /// optional: false
      ///
      RightQuote
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   StringLiteralExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftQuote();
   TokenSyntax getText();
   TokenSyntax getRightQuote();

   StringLiteralExprSyntax withLeftQuote(std::optional<TokenSyntax> leftQuote);
   StringLiteralExprSyntax withText(std::optional<TokenSyntax> text);
   StringLiteralExprSyntax withRightQuote(std::optional<TokenSyntax> rightQuote);

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

///
/// isset_var_item:
///   expr ','
///
class IsSetVarItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_COMMA)
      /// optional: true
      ///
      TrailingComma,
   };

public:
   IsSetVarItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   ExprSyntax getExpr();
   std::optional<TokenSyntax> getTrailingComma();

   IsSetVarItemSyntax withExpr(std::optional<ExprSyntax> expr);
   IsSetVarItemSyntax withTrailingComma(std::optional<TokenSyntax> comma);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::IsSetVarItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class IsSetVarItemSyntaxBuilder;
   void validate();
};

///
/// isset_vars_clause:
///   '(' isset_variables_list ')'
///
class IsSetVariablesClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: IssetVariablesListSyntax
      /// optional: false
      ///
      IsSetVariablesList,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken
   };

public:
   IsSetVariablesClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftParenToken();
   IssetVariablesListSyntax getIsSetVariablesList();
   TokenSyntax getRightParenToken();

   IsSetVariablesClauseSyntax withLeftParenToken(std::optional<TokenSyntax> leftParenToken);
   IsSetVariablesClauseSyntax withIsSetVariablesList(std::optional<IssetVariablesListSyntax> issetVariablesList);
   IsSetVariablesClauseSyntax withRightParenToken(std::optional<TokenSyntax> rightParenToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::IsSetVariablesClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class IsSetVariablesClauseSyntaxBuilder;
   void validate();
};

///
/// isset_expr:
///   T_ISSET '(' isset_variables possible_comma ')'
///
class IsSetFuncExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ISSET)
      /// optional: false
      ///
      IsSetToken,
      ///
      /// type: IsSetVariablesClauseSyntax
      /// optional: false
      ///
      IsSetVariablesClause,
   };

public:
   IsSetFuncExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getIsSetToken();
   IsSetVariablesClauseSyntax getIsSetVariablesClause();

   IsSetFuncExprSyntax withIsSetToken(std::optional<TokenSyntax> issetToken);
   IsSetFuncExprSyntax withIsSetVariablesClause(std::optional<IsSetVariablesClauseSyntax> isSetVariablesClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::IsSetFuncExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class IsSetExprSyntaxBuilder;
   void validate();
};

///
/// empty_expr:
///   T_EMPTY '(' expr ')'
///
class EmptyFuncExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_EMPTY)
      /// optional: false
      ///
      EmptyToken,
      ///
      /// type: ParenDecoratedExprSyntax
      /// optional: false
      ///
      ArgumentsClause
   };

public:
   EmptyFuncExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getEmptyToken();
   ParenDecoratedExprSyntax getArgumentsClause();

   EmptyFuncExprSyntax withEmptyToken(std::optional<TokenSyntax> emptyToken);
   EmptyFuncExprSyntax withArgumentsClause(std::optional<ParenDecoratedExprSyntax> argumentsClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EmptyFuncExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EmptyExprSyntaxBuilder;
   void validate();
};

///
/// inlcude_expr:
///   T_INCLUDE expr
/// | T_INCLUDE_ONCE expr
///
class IncludeExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// ---------------------------------------
      /// token choice: T_INCLUDE
      /// ---------------------------------------
      /// token choice: T_INCLUDE_ONCE
      ///
      IncludeToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ArgExpr,
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   IncludeExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getIncludeToken();
   ExprSyntax getArgExpr();

   IncludeExprSyntax withIncludeToken(std::optional<TokenSyntax> includeToken);
   IncludeExprSyntax withArgExpr(std::optional<ExprSyntax> argExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::IncludeExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class IncludeExprSyntaxBuilder;
   void validate();
};

///
/// require_expr:
///   T_REQUIRE expr
/// | T_REQUIRE_ONCE expr
///
class RequireExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// ---------------------------------------
      /// token choice: T_REQUIRE
      /// ---------------------------------------
      /// token choice: T_REQUIRE_ONCE
      ///
      RequireToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ArgExpr,
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   RequireExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getRequireToken();
   ExprSyntax getArgExpr();

   RequireExprSyntax withRequireToken(std::optional<TokenSyntax> requireToken);
   RequireExprSyntax withArgExpr(std::optional<ExprSyntax> argExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::RequireExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class RequireExprSyntaxBuilder;
   void validate();
};

///
/// eval_expr:
///   T_EVAL '(' expr ')'
///
class EvalFuncExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_EVAL)
      /// optional: false
      ///
      EvalToken,
      ///
      /// type: ParenDecoratedExprSyntax
      /// optional: false
      ///
      ArgumentsClause
   };

public:
   EvalFuncExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getEvalToken();
   ParenDecoratedExprSyntax getArgumentsClause();

   EvalFuncExprSyntax withEvalToken(std::optional<TokenSyntax> evalToken);
   EvalFuncExprSyntax withArgumentsClause(std::optional<ParenDecoratedExprSyntax> argumentsClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EvalFuncExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EvalExprSyntaxBuilder;
   void validate();
};

///
/// echo_func:
///   T_ECHO expr
///
class EchoFuncExprSyntax final : public ExprSyntax
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
      /// type: ExprSyntax
      /// optional: false
      ///
      ArgsExpr
   };

public:

   EchoFuncExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getEchoToken();
   ExprSyntax getArgsExpr();

   EchoFuncExprSyntax withEchoToken(std::optional<TokenSyntax> echoToken);
   EchoFuncExprSyntax withArgsExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EchoFuncExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EvalExprSyntaxBuilder;
   void validate();
};

///
/// print_func:
///   T_PRINT expr
///
class PrintFuncExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_PRINT)
      /// optional: false
      ///
      PrintToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ArgsExpr
   };

public:
   PrintFuncExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getPrintToken();
   ExprSyntax getArgsExpr();

   PrintFuncExprSyntax withPrintToken(std::optional<TokenSyntax> printToken);
   PrintFuncExprSyntax withArgsExpr(std::optional<ExprSyntax> expr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::PrintFuncExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EvalExprSyntaxBuilder;
   void validate();
};

///
/// func_like_expr:
///   T_ISSET '(' isset_variables possible_comma ')'
/// | T_EMPTY '(' expr ')'
/// | T_INCLUDE expr
/// | T_INCLUDE_ONCE expr
/// | T_EVAL '(' expr ')'
/// | T_REQUIRE expr
/// | _REQUIRE_ONCE expr
///
class FuncLikeExprSyntax final : public ExprSyntax
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
      /// -------------------------------------------
      /// node choice: IsSetFuncExprSyntax
      /// -------------------------------------------
      /// node choice: EmptyFuncExprSyntax
      /// -------------------------------------------
      /// node choice: IncludeExprSyntax
      /// -------------------------------------------
      /// node choice: RequireExprSyntax
      /// -------------------------------------------
      /// node choice: EvalFuncExprSyntax
      ///
      FuncLikeExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   FuncLikeExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getFuncLikeExpr();
   FuncLikeExprSyntax withFuncLikeExpr(std::optional<ExprSyntax> funcLikeExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::FuncLikeExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class FuncLikeExprSyntaxBuilder;
   void validate();
};

///
/// assign_expr:
///   variable '=' expr
/// | variable '=' '&' variable
///
class AssignmentExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: VariableExprSyntax
      /// optional: false
      ///
      Target,
      ///
      /// type: TokenSyntax (T_EQUAL)
      /// optional: false
      ///
      AssignToken,
      ///
      /// type: TokenSyntax (T_AMPERSAND)
      /// optional: true
      ///
      RefToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      /// node choices: true
      /// -----------------------------
      /// node choice: ExprSyntax
      /// -----------------------------
      /// node choice: VariableExprSyntax
      ///
      ValueExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   AssignmentExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   VariableExprSyntax getTarget();
   TokenSyntax getAssignToken();
   std::optional<TokenSyntax> getRefToken();
   ExprSyntax getValueExpr();

   AssignmentExprSyntax withTarget(std::optional<VariableExprSyntax> target);
   AssignmentExprSyntax withAssignToken(std::optional<TokenSyntax> assignToken);
   AssignmentExprSyntax withRefToken(std::optional<TokenSyntax> refToken);
   AssignmentExprSyntax withValueExpr(std::optional<ExprSyntax> valueExpr);

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

///
/// compound_assign_expr:
///   variable T_PLUS_EQUAL expr
/// | variable T_MINUS_EQUAL expr
/// | variable T_MUL_EQUAL expr
/// | variable T_POW_EQUAL expr
/// | variable T_DIV_EQUAL expr
/// | variable T_CONCAT_EQUAL expr
/// | variable T_MOD_EQUAL expr
/// | variable T_AND_EQUAL expr
/// | variable T_OR_EQUAL expr
/// | variable T_XOR_EQUAL expr
/// | variable T_SL_EQUAL expr
/// | variable T_SR_EQUAL expr
/// | variable T_COALESCE_EQUAL expr
///
class CompoundAssignmentExprSyntax : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: VariableExprSyntax
      /// optional: false
      ///
      Target,
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// -----------------------------------------------
      /// T_PLUS_EQUAL | T_MINUS_EQUAL | T_MUL_EQUAL
      /// -----------------------------------------------
      /// T_POW_EQUAL | T_DIV_EQUAL | T_CONCAT_EQUAL
      /// -----------------------------------------------
      /// T_MOD_EQUAL | T_AND_EQUAL | T_OR_EQUAL
      /// -----------------------------------------------
      /// T_XOR_EQUAL | T_SL_EQUAL | T_SR_EQUAL
      /// -----------------------------------------------
      /// T_COALESCE_EQUAL
      ///
      CompoundAssignToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ValueExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   CompoundAssignmentExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   VariableExprSyntax getTarget();
   TokenSyntax getCompoundAssignToken();
   ExprSyntax getValueExpr();

   CompoundAssignmentExprSyntax withTarget(std::optional<VariableExprSyntax> target);
   CompoundAssignmentExprSyntax withCompoundAssignToken(std::optional<TokenSyntax> compoundAssignToken);
   CompoundAssignmentExprSyntax withValueExpr(std::optional<ExprSyntax> valueExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::CompoundAssignmentExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CompoundAssignmentExprSyntaxBuilder;
   void validate();
};

///
/// logical_expr:
///   expr T_BOOLEAN_OR expr
/// | expr T_BOOLEAN_AND expr
/// | expr T_LOGICAL_OR expr
/// | expr T_LOGICAL_AND expr
/// | expr T_LOGICAL_XOR expr
///
class LogicalExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Lhs,
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// -----------------------------------------------
      /// T_BOOLEAN_OR | T_BOOLEAN_AND | T_LOGICAL_OR
      /// T_LOGICAL_AND | T_LOGICAL_XOR
      ///
      LogicalOperator,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Rhs
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   LogicalExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getLhs();
   TokenSyntax getLogicalOperator();
   ExprSyntax getRhs();

   LogicalExprSyntax withLhs(std::optional<ExprSyntax> lhs);
   LogicalExprSyntax withLogicalOperator(std::optional<TokenSyntax> logicalOperator);
   LogicalExprSyntax withRhs(std::optional<ExprSyntax> rhs);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::LogicalExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class LogicalExprSyntaxBuilder;
   void validate();
};

///
/// bit_logical_expr:
///   expr '|' expr
/// | expr '&' expr
/// | expr '^' expr
///
///
class BitLogicalExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Lhs,
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// -----------------------------------------------
      /// T_VBAR | T_AMPERSAND | T_CARET
      ///
      BitLogicalOperator,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Rhs
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   BitLogicalExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getLhs();
   TokenSyntax getBitLogicalOperator();
   ExprSyntax getRhs();

   BitLogicalExprSyntax withLhs(std::optional<ExprSyntax> lhs);
   BitLogicalExprSyntax withBitLogicalOperator(std::optional<TokenSyntax> bitLogicalOperator);
   BitLogicalExprSyntax withRhs(std::optional<ExprSyntax> rhs);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BitLogicalExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class LogicalExprSyntaxBuilder;
   void validate();
};

///
/// relation_expr:
///   expr T_IS_IDENTICAL expr
/// | expr T_IS_NOT_IDENTICAL expr
/// | expr T_IS_EQUAL expr
/// | expr T_IS_NOT_EQUAL expr
/// | expr '<' expr
/// | expr T_IS_SMALLER_OR_EQUAL expr
/// | expr '>' expr
/// | expr T_IS_GREATER_OR_EQUAL expr
/// | expr T_SPACESHIP expr
///
class RelationExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Lhs,
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// -----------------------------------------------
      /// T_IS_IDENTICAL | T_IS_NOT_IDENTICAL | T_IS_EQUAL
      /// T_IS_NOT_EQUAL | T_LEFT_ANGLE | T_IS_SMALLER_OR_EQUAL
      /// T_RIGHT_ANGLE | T_IS_GREATER_OR_EQUAL | T_SPACESHIP
      ///
      RelationOperator,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Rhs
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   RelationExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getLhs();
   TokenSyntax getRelationOperator();
   ExprSyntax getRhs();

   RelationExprSyntax withLhs(std::optional<ExprSyntax> lhs);
   RelationExprSyntax withRelationOperator(std::optional<TokenSyntax> relationOperator);
   RelationExprSyntax withRhs(std::optional<ExprSyntax> rhs);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::RelationExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class RelationExprSyntaxBuilder;
   void validate();
};

///
/// cast_expr:
/// | T_INT_CAST expr
/// | T_DOUBLE_CAST expr
/// | T_STRING_CAST expr
/// | T_ARRAY_CAST expr
/// | T_OBJECT_CAST expr
/// | T_BOOL_CAST expr
/// | T_UNSET_CAST expr
///
class CastExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// ---------------------------------------------
      /// T_INT_CAST | T_DOUBLE_CAST | T_STRING_CAST
      /// T_ARRAY_CAST | T_OBJECT_CAST | T_BOOL_CAST
      /// T_UNSET_CAST
      ///
      CastOperator,

      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      ValueExpr
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   CastExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getCastOperator();
   ExprSyntax getValueExpr();

   CastExprSyntax withCastOperator(std::optional<TokenSyntax> castOperator);
   CastExprSyntax withValueExpr(std::optional<ExprSyntax> valueExpr);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::CastExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class CastExprSyntaxBuilder;
   void validate();
};

///
/// exit_expr:
///   '(' optional_expr ')'
///
class ExitExprArgClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: ExprSyntax
      /// optional: true
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken
   };

public:
   ExitExprArgClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftParenToken();
   std::optional<ExprSyntax> getExpr();
   TokenSyntax getRightParenToken();

   ExitExprArgClauseSyntax getLeftParenToken(std::optional<TokenSyntax> leftParentToken);
   ExitExprArgClauseSyntax getExpr(std::optional<ExprSyntax> expr);
   ExitExprArgClauseSyntax getRightParenToken(std::optional<TokenSyntax> rightParentToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ExitExprArgClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ExitExprArgClauseSyntaxBuilder;
   void validate();
};

///
/// exit_expr:
///   T_EXIT exit_expr
///
class ExitExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_EXIT)
      /// optional: false
      ///
      ExitToken,
      ///
      /// type: ExitExprArgClauseSyntax
      /// optional: false
      ///
      ArgClause
   };

public:
   ExitExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getExitToken();
   std::optional<ExitExprArgClauseSyntax> getArgClause();

   ExitExprSyntax withExitToken(std::optional<TokenSyntax> exitToken);
   ExitExprSyntax withArgClause(std::optional<ExitExprArgClauseSyntax> argClause);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::ExitExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class ExitExprSyntaxBuilder;
   void validate();
};

///
/// encaps_var_offset:
///   T_IDENTIFIER_STRING
/// | T_NUM_STRING
/// | '-' T_NUM_STRING
/// | T_VARIABLE
///
class EncapsVarOffsetSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_MINUS_SIGN)
      /// optional: true
      ///
      MinusSign,
      ///
      /// type: TokenSynax
      /// optional: false
      /// token choices: true
      /// ---------------------------------
      /// T_IDENTIFIER_STRING
      /// T_NUM_STRING
      /// T_VARIABLE
      ///
      Offset
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   EncapsVarOffsetSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getMinusSign();
   TokenSyntax getOffset();

   EncapsVarOffsetSyntax withMinusSign(std::optional<TokenSyntax> minusSign);
   EncapsVarOffsetSyntax withOffset(std::optional<TokenSyntax> offset);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsVarOffset;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsVarOffsetSyntaxBuilder;
   void validate();
};

///
/// encaps_var:
///   T_VARIABLE '[' encaps_var_offset ']'
///
class EncapsArrayVarSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_VARIABLE)
      /// optional: false
      ///
      VarToken,
      ///
      /// type: TokenSyntax (T_LEFT_SQUARE_BRACKET)
      /// optional: false
      ///
      LeftSquareBracket,
      ///
      /// type: EncapsVarOffsetSyntax
      /// optional: false
      ///
      Offset,
      ///
      /// type: TokenSyntax (T_RIGHT_SQUARE_BRACKET)
      /// optional: false
      ///
      RightSquareBracket
   };

public:
   EncapsArrayVarSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getVarToken();
   TokenSyntax getLeftSquareBracket();
   EncapsVarOffsetSyntax getOffset();
   TokenSyntax getRightSquareBracket();

   EncapsArrayVarSyntax withVarToken(std::optional<TokenSyntax> varToken);
   EncapsArrayVarSyntax withLeftSquareBracket(std::optional<TokenSyntax> leftSquareBracket);
   EncapsArrayVarSyntax withOffset(std::optional<EncapsVarOffsetSyntax> offset);
   EncapsArrayVarSyntax withRightSquareBracket(std::optional<TokenSyntax> rightSquareBracket);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsArrayVar;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsArrayVarSyntaxBuilder;
   void validate();
};

///
/// encaps_var:
///   T_VARIABLE T_OBJECT_OPERATOR T_IDENTIFIER_STRING
///
class EncapsObjPropSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_VARIABLE)
      /// optional: false
      ///
      VarToken,
      ///
      /// type: TokenSyntax (T_OBJECT_OPERATOR)
      /// optional: false
      ///
      ObjOperatorToken,
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: false
      ///
      IdentifierToken
   };

public:
   EncapsObjPropSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getVarToken();
   TokenSyntax getObjOperatorToken();
   TokenSyntax getIdentifierToken();

   EncapsObjPropSyntax withVarToken(std::optional<TokenSyntax> varToken);
   EncapsObjPropSyntax withObjOperatorToken(std::optional<TokenSyntax> objOperatorToken);
   EncapsObjPropSyntax withIdentifierToken(std::optional<TokenSyntax> identifierToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsObjProp;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsObjPropSyntaxBuilder;
   void validate();
};

///
/// encaps_var:
///   T_DOLLAR_OPEN_CURLY_BRACES expr '}'
///
class EncapsDollarCurlyExprSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DOLLAR_OPEN_CURLY_BRACES)
      /// optional: false
      ///
      DollarOpenCurlyToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Expr,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      CloseCurlyToken
   };

public:
   EncapsDollarCurlyExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getDollarOpenCurlyToken();
   ExprSyntax getExpr();
   TokenSyntax getCloseCurlyToken();

   EncapsDollarCurlyExprSyntax withDollarOpenCurlyToken(std::optional<TokenSyntax> dollarOpenCurlyToken);
   EncapsDollarCurlyExprSyntax withExpr(std::optional<ExprSyntax> expr);
   EncapsDollarCurlyExprSyntax withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsDollarCurlyExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsDollarCurlyExprSyntaxBuilder;
   void validate();
};

///
/// encaps_var:
///   T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '}'
///
class EncapsDollarCurlyVarSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DOLLAR_OPEN_CURLY_BRACES)
      /// optional: false
      ///
      DollarOpenCurlyToken,
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: false
      ///
      Varname,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      CloseCurlyToken
   };

public:
   EncapsDollarCurlyVarSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getDollarOpenCurlyToken();
   TokenSyntax getVarname();
   TokenSyntax getCloseCurlyToken();

   EncapsDollarCurlyVarSyntax withDollarOpenCurlyToken(std::optional<TokenSyntax> dollarOpenCurlyToken);
   EncapsDollarCurlyVarSyntax withVarname(std::optional<ExprSyntax> varname);
   EncapsDollarCurlyVarSyntax withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsDollarCurlyVar;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsDollarCurlyVarSyntaxBuilder;
   void validate();
};

///
/// encaps_var:
///   T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '[' expr ']' '}'
///
class EncapsDollarCurlyArraySyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 6;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 6;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DOLLAR_OPEN_CURLY_BRACES)
      /// optional: false
      ///
      DollarOpenCurlyToken,
      ///
      /// type: TokenSyntax (T_IDENTIFIER_STRING)
      /// optional: false
      ///
      Varname,
      ///
      /// type: TokenSyntax (T_LEFT_SQUARE_BRACKET)
      /// optional: false
      ///
      LeftSquareBracketToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      IndexExpr,
      ///
      /// type: TokenSyntax (T_RIGHT_SQUARE_BRACKET)
      /// optional: false
      ///
      RightSquareBracketToken,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      CloseCurlyToken
   };

public:
   EncapsDollarCurlyArraySyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getDollarOpenCurlyToken();
   TokenSyntax getVarname();
   TokenSyntax getLeftSquareBracketToken();
   ExprSyntax getIndexExpr();
   TokenSyntax getRightSquareBracketToken();
   TokenSyntax getCloseCurlyToken();

   EncapsDollarCurlyArraySyntax withDollarOpenCurlyToken(std::optional<TokenSyntax> dollarOpenCurlyToken);
   EncapsDollarCurlyArraySyntax withVarname(std::optional<TokenSyntax> varname);
   EncapsDollarCurlyArraySyntax withLeftSquareBracketToken(std::optional<TokenSyntax> leftSquareBracketToken);
   EncapsDollarCurlyArraySyntax withIndexExpr(std::optional<ExprSyntax> indexExpr);
   EncapsDollarCurlyArraySyntax withRightSquareBracketToken(std::optional<TokenSyntax> rightSquareBracketToken);
   EncapsDollarCurlyArraySyntax withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsDollarCurlyArray;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsDollarCurlyArraySyntaxBuilder;
   void validate();
};

///
/// encaps_var:
///   T_CURLY_OPEN variable '}'
///
class EncapsCurlyVarSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_CURLY_OPEN)
      /// optional: false
      ///
      CurlyOpen,
      ///
      /// type: TokenSyntax (T_VARIABLE)
      /// optional: false
      ///
      Variable,
      ///
      /// type: TokenSyntax (T_RIGHT_BRACE)
      /// optional: false
      ///
      CloseCurlyToken
   };

public:
   EncapsCurlyVarSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getCurlyOpen();
   TokenSyntax getVariable();
   TokenSyntax getCloseCurlyToken();

   EncapsCurlyVarSyntax withCurlyOpen(std::optional<TokenSyntax> curlyOpen);
   EncapsCurlyVarSyntax withVariable(std::optional<TokenSyntax> variable);
   EncapsCurlyVarSyntax withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsCurlyVar;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsCurlyVarSyntaxBuilder;
   void validate();
};

///
/// encaps_var:
///   T_VARIABLE
/// | T_VARIABLE '[' encaps_var_offset ']'
/// | T_VARIABLE T_OBJECT_OPERATOR T_STRING
/// | T_DOLLAR_OPEN_CURLY_BRACES expr '}'
/// | T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '}'
/// | T_DOLLAR_OPEN_CURLY_BRACES T_STRING_VARNAME '[' expr ']' '}'
/// | T_CURLY_OPEN variable '}'
///
class EncapsVarSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: Syntax
      /// optional: false
      /// node choices: true
      /// ------------------------------------------
      /// node choice: TokenSyntax (T_VARIABLE)
      /// ------------------------------------------
      /// node choice: EncapsArrayVarSyntax
      /// ------------------------------------------
      /// node choice: EncapsObjPropSyntax
      /// ------------------------------------------
      /// node choice: EncapsDollarCurlyExprSyntax
      /// ------------------------------------------
      /// node choice: EncapsDollarCurlyVarSyntax
      /// ------------------------------------------
      /// node choice: EncapsDollarCurlyArraySyntax
      /// ------------------------------------------
      /// node choice: EncapsCurlyVarSyntax
      ///
      Var,
   };

#ifdef POLAR_DEBUG_BUILD
   const static NodeChoicesType CHILD_NODE_CHOICES;
#endif

public:
   EncapsVarSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getVar();
   EncapsVarSyntax withVar(std::optional<Syntax> var);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsVar;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsVarSyntaxBuilder;
   void validate();
};

///
/// TODO
/// encaps_list_item:
///   encaps_var
/// | T_ENCAPSED_AND_WHITESPACE
/// | T_ENCAPSED_AND_WHITESPACE encaps_var
///
class EncapsListItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_ENCAPSED_AND_WHITESPACE)
      /// optional: true
      ///
      StrLiteral,
      ///
      /// type: EncapsVarSyntax
      /// optional: true
      ///
      EncapsVar
   };

public:
   EncapsListItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getStrLiteral();
   std::optional<EncapsVarSyntax> getEncapsVar();

   EncapsListItemSyntax withEncapsListItemSyntax(std::optional<TokenSyntax> strLiteral);
   EncapsListItemSyntax withEncapsVar(std::optional<EncapsVarSyntax> encapsVar);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsListItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsListItemSyntaxBuilder;
   void validate();
};

///
/// backticks_expr:
///   T_ENCAPSED_AND_WHITESPACE
/// | encaps_list
///
class BackticksClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: Syntax
      /// optional: true
      /// node choices: true
      /// --------------------------------------------------------
      /// node choice: TokenSyntax (T_ENCAPSED_AND_WHITESPACE)
      /// --------------------------------------------------------
      /// node choice: EncapsItemListSyntax
      ///
      Backticks,
   };

public:
   BackticksClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   Syntax getBackticks();
   BackticksClauseSyntax withBackticks(std::optional<Syntax> backticks);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::BackticksClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class BackticksClauseSyntaxBuilder;
   void validate();
};

///
/// heredoc_expr:
///   T_START_HEREDOC T_ENCAPSED_AND_WHITESPACE T_END_HEREDOC
/// | T_START_HEREDOC encaps_list T_END_HEREDOC
/// | T_START_HEREDOC T_END_HEREDOC
///
class HeredocExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_START_HEREDOC)
      /// optional: false
      ///
      StartHeredocToken,
      ///
      /// type: Syntax
      /// node choices: true
      /// optional: true
      /// ----------------------------------------------------------
      /// node choice: TokenSyntax (T_ENCAPSED_AND_WHITESPACE)
      /// ----------------------------------------------------------
      /// node choice: EncapsItemListSyntax
      ///
      TextClause,
      ///
      /// type: TokenSyntax (T_START_HEREDOC)
      /// optional: false
      ///
      EndHeredocToken
   };

public:
   HeredocExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getStartHeredocToken();
   std::optional<Syntax> getTextClause();
   TokenSyntax getEndHeredocToken();

   HeredocExprSyntax withStartHeredocToken(std::optional<TokenSyntax> startHeredocToken);
   HeredocExprSyntax withTextClause(std::optional<Syntax> textClause);
   HeredocExprSyntax withEndHeredocToken(std::optional<TokenSyntax> endHeredocToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::HeredocExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class HeredocExprSyntaxBuilder;
   void validate();
};

///
/// encaps_list_str:
///   '"' encaps_list '"'
///
class EncapsListStringExprSyntax final : ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 3;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 3;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_DOUBLE_QUOTE)
      /// optional: false
      ///
      LeftQuoteToken,
      ///
      /// type: EncapsItemListSyntax
      /// optional: false
      ///
      EncapsList,
      ///
      /// type: TokenSyntax (T_DOUBLE_QUOTE)
      /// optional: false
      ///
      RightQuoteToken
   };
public:
   EncapsListStringExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   TokenSyntax getLeftQuoteToken();
   EncapsItemListSyntax getEncapsList();
   TokenSyntax getRightQuoteToken();

   EncapsListStringExprSyntax withLeftQuoteToken(std::optional<TokenSyntax> leftQuoteToken);
   EncapsListStringExprSyntax withEncapsList(std::optional<EncapsItemListSyntax> encapsList);
   EncapsListStringExprSyntax withRightQuoteToken(std::optional<TokenSyntax> rightQuoteToken);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::EncapsListStringExpr;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }

private:
   friend class EncapsListStringExprSyntaxBuilder;
   void validate();
};

class BooleanLiteralExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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
   static const TokenChoicesType CHILD_TOKEN_CHOICES;
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

///
/// ternary_expr:
/// 	expr '?' expr ':' expr
/// | expr '?' ':' expr
///
class TernaryExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 5;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 5;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: ExprSyntax
      /// optional: false
      ConditionExpr,
      /// type: TokenSyntax (T_QUESTION_MARK)
      /// optional: false
      QuestionMark,
      /// type: ExprSyntax
      /// optional: true
      FirstChoice,
      /// type: TokenSyntax (T_COLON)
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
   std::optional<ExprSyntax> getFirstChoice();
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

class SequenceExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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

///
/// prefix_operator_expr:
///   '+' expr %prec T_INC
/// | '-' expr %prec T_INC
/// | '!' expr
/// | '~' expr
/// | '@' expr
/// | T_INC variable
/// | T_DEC variable
///
class PrefixOperatorExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: TokenSyntax
      /// optional: true
      /// token choices: true
      /// ---------------------------------------------------
      /// T_PLUS_SIGN | T_MINUS_SIGN | T_EXCLAMATION_MARK
      /// T_TILDE     | T_ERROR_SUPPRESS_SIGN | T_INC
      /// T_DEC
      ///
      OperatorToken,
      /// type: ExprSyntax
      /// optional: false
      Expr
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

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

///
/// postfix_operator_expr:
///   variable T_INC
/// | variable T_DEC
///
class PostfixOperatorExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
   enum Cursor : SyntaxChildrenCountType
   {
      /// type: Expr
      /// optional: false
      Expr,
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// ----------------------------------
      /// T_INC | T_DEC
      ///
      OperatorToken
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

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

///
/// binary_expr:
/// | expr '.' expr
/// |	expr '+' expr
/// | expr '-' expr
/// | expr '*' expr
/// | expr T_POW expr
/// | expr '/' expr
/// | expr '%' expr
/// | expr T_SL expr
/// | expr T_SR expr
///
class BinaryOperatorExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Lhs,
      ///
      /// type: TokenSyntax
      /// optional: false
      /// token choices: true
      /// --------------------------------
      /// T_STR_CONCAT | T_PLUS_SIGN | T_MINUS_SIGN
      /// T_MUL_SIGN | T_DIV_SIGN | T_POW
      /// T_MOD_SIGN | T_SL | T_SR
      ///
      OperatorToken,
      ///
      /// type: ExprSyntax
      /// optional: false
      ///
      Rhs
   };

#ifdef POLAR_DEBUG_BUILD
   const static TokenChoicesType CHILD_TOKEN_CHOICES;
#endif

public:
   BinaryOperatorExprSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : ExprSyntax(root, data)
   {
      validate();
   }

   ExprSyntax getLhs();
   TokenSyntax getOperatorToken();
   ExprSyntax getRhs();

   BinaryOperatorExprSyntax withLhs(std::optional<ExprSyntax> lhs);
   BinaryOperatorExprSyntax withOperatorToken(std::optional<TokenSyntax> operatorToken);
   BinaryOperatorExprSyntax withRhs(std::optional<ExprSyntax> rhs);

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

///
///
/// lexical_vars:
///   /* empty */
/// | T_USE '(' lexical_var_list ')'
///
class UseLexicalVarClauseSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 4;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 4;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_USE)
      /// optional: false
      ///
      UseToken,
      ///
      /// type: TokenSyntax (T_LEFT_PAREN)
      /// optional: false
      ///
      LeftParenToken,
      ///
      /// type: LexicalVarListSyntax
      /// optional: false
      ///
      LexicalVars,
      ///
      /// type: TokenSyntax (T_RIGHT_PAREN)
      /// optional: false
      ///
      RightParenToken
   };

public:
   UseLexicalVarClauseSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   TokenSyntax getUseToken();
   TokenSyntax getLeftParenToken();
   LexicalVarListSyntax getLexicalVars();
   TokenSyntax getRightParenToken();

   UseLexicalVarClauseSyntax withUseToken(std::optional<TokenSyntax> useToken);
   UseLexicalVarClauseSyntax withLeftParenToken(std::optional<TokenSyntax> leftParen);
   UseLexicalVarClauseSyntax withLexicalVars(std::optional<LexicalVarListSyntax> lexicalVars);
   UseLexicalVarClauseSyntax withRightParenToken(std::optional<TokenSyntax> rightParen);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::UseLexicalVarClause;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class LexicalVarItemSyntaxBuilder;
   void validate();
};

///
/// lexical_var:
///   T_VARIABLE
/// | '&' T_VARIABLE
///
class LexicalVarItemSyntax final : public Syntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
   enum Cursor : SyntaxChildrenCountType
   {
      ///
      /// type: TokenSyntax (T_AMPERSAND)
      /// optional: true
      ///
      ReferenceToken,
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
   LexicalVarItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getReferenceToken();
   TokenSyntax getVariable();
   std::optional<TokenSyntax> getTrailingComma();

   LexicalVarItemSyntax withReferenceToken(std::optional<TokenSyntax> referenceToken);
   LexicalVarItemSyntax withVariable(std::optional<TokenSyntax> variable);
   LexicalVarItemSyntax withTrailingComma(std::optional<TokenSyntax> trailingComma);

   static bool kindOf(SyntaxKind kind)
   {
      return kind == SyntaxKind::LexicalVarItem;
   }

   static bool classOf(const Syntax *syntax)
   {
      return kindOf(syntax->getKind());
   }
private:
   friend class LexicalVarItemSyntaxBuilder;
   void validate();
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_EXPR_NODES_H
