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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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

class PrefixOperatorExprSyntax final : public ExprSyntax
{
public:
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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
   constexpr static std::uint8_t CHILDREN_COUNT = 2;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 2;
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
   constexpr static std::uint8_t CHILDREN_COUNT = 1;
   constexpr static std::uint8_t REQUIRED_CHILDREN_COUNT = 1;
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
      /// type: TokenSyntax
      /// optional: true
      ///
      ReferenceToken,
      ///
      /// type: TokenSyntax
      /// optional: false
      ///
      Variable
   };
public:
   LexicalVarItemSyntax(const RefCountPtr<SyntaxData> root, const SyntaxData *data)
      : Syntax(root, data)
   {
      validate();
   }

   std::optional<TokenSyntax> getReferenceToken();
   TokenSyntax getVariable();
   LexicalVarItemSyntax withReferenceToken(std::optional<TokenSyntax> referenceToken);
   LexicalVarItemSyntax withVariable(std::optional<TokenSyntax> variable);

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
