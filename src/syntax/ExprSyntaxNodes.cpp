// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/17.

#include "polarphp/syntax/syntaxnode/ExprSyntaxNodes.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodes.h"

namespace polar::syntax {

///
/// ParenDecoratedExprSyntax
///

void ParenDecoratedExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ParenDecoratedExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftParenToken, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_token(raw, RightParenToken, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
#endif
}

TokenSyntax ParenDecoratedExprSyntax::getLeftParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParenToken).get()};
}

ExprSyntax ParenDecoratedExprSyntax::getExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax ParenDecoratedExprSyntax::getRightParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParenToken).get()};
}

ParenDecoratedExprSyntax
ParenDecoratedExprSyntax::withLeftParenToken(std::optional<TokenSyntax> leftParenToken)
{
   RefCountPtr<RawSyntax> leftParenTokenRaw;
   if (leftParenToken.has_value()) {
      leftParenTokenRaw = leftParenToken->getRaw();
   } else {
      leftParenTokenRaw = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<ParenDecoratedExprSyntax>(leftParenTokenRaw, Cursor::LeftParenToken);
}

ParenDecoratedExprSyntax ParenDecoratedExprSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> exprRaw;
   if (expr.has_value()) {
      exprRaw = expr->getRaw();
   } else {
      exprRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<ParenDecoratedExprSyntax>(exprRaw, Cursor::Expr);
}

ParenDecoratedExprSyntax
ParenDecoratedExprSyntax::withRightParenToken(std::optional<TokenSyntax> rightParenToken)
{
   RefCountPtr<RawSyntax> rightParenTokenRaw;
   if (rightParenToken.has_value()) {
      rightParenTokenRaw = rightParenToken->getRaw();
   } else {
      rightParenTokenRaw = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<ParenDecoratedExprSyntax>(rightParenTokenRaw, Cursor::RightParenToken);
}

///
/// NullExprSyntax
///
void NullExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NullExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax NullExprSyntax::getNullKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::NullKeyword).get()};
}

NullExprSyntax NullExprSyntax::withNullKeyword(std::optional<TokenSyntax> keyword)
{
   RefCountPtr<RawSyntax> rawKeyword;
   if (keyword.has_value()) {
      rawKeyword = keyword->getRaw();
   } else {
      rawKeyword = RawSyntax::missing(TokenKindType::T_NULL,
                                      OwnedString::makeUnowned((get_token_text(TokenKindType::T_NULL))));
   }
   return m_data->replaceChild<NullExprSyntax>(rawKeyword, Cursor::NullKeyword);
}

///
/// OptionalExprSyntax
///
void OptionalExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == OptionalExprSyntax::CHILDREN_COUNT);
#endif
}

std::optional<ExprSyntax> OptionalExprSyntax::getExpr()
{
   RefCountPtr<SyntaxData> exprData = m_data->getChild(Cursor::Expr);
   if (!exprData) {
      return std::nullopt;
   }
   return ExprSyntax {m_root, exprData.get()};
}

OptionalExprSyntax OptionalExprSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> exprRaw;
   if (expr.has_value()) {
      exprRaw = expr->getRaw();
   } else {
      exprRaw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<OptionalExprSyntax>(exprRaw, Cursor::Expr);
}

///
/// VariableExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType VariableExprSyntax::CHILD_NODE_CHOICES
{
   {
      VariableExprSyntax::Var, {
         SyntaxKind::CallableVariableExpr, SyntaxKind::StaticPropertyExpr,
               SyntaxKind::InstancePropertyExpr
      }
   }
};
#endif

void VariableExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == VariableExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Var, CHILD_NODE_CHOICES.at(Cursor::Var));
#endif
}

ExprSyntax VariableExprSyntax::getVar()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Var).get()};
}

VariableExprSyntax VariableExprSyntax::withVar(std::optional<ExprSyntax> var)
{
   RefCountPtr<RawSyntax> varRaw;
   if (var.has_value()) {
      varRaw = var->getRaw();
   } else {
      varRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<VariableExprSyntax>(varRaw, Cursor::Var);
}

///
/// ClassConstIdentifierExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ClassConstIdentifierExprSyntax::CHILD_NODE_CHOICES
{
   {
      ClassConstIdentifierExprSyntax::ClassName, {
         SyntaxKind::VariableClassNameClause, SyntaxKind::ClassNameClause
      }
   }
};
#endif

void ClassConstIdentifierExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassConstIdentifierExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, ClassName, CHILD_NODE_CHOICES.at(Cursor::ClassName));
   syntax_assert_child_token(raw, SeparatorToken, std::set{TokenKindType::T_PAAMAYIM_NEKUDOTAYIM});
   syntax_assert_child_kind(raw, Identifier, std::set{SyntaxKind::Identifier});
#endif
}

Syntax ClassConstIdentifierExprSyntax::getClassName()
{
   return Syntax {m_root, m_data->getChild(Cursor::ClassName).get()};
}

TokenSyntax ClassConstIdentifierExprSyntax::getSeparatorToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::SeparatorToken).get()};
}

IdentifierSyntax ClassConstIdentifierExprSyntax::getIdentifier()
{
   return IdentifierSyntax {m_root, m_data->getChild(Cursor::Identifier).get()};
}

ClassConstIdentifierExprSyntax ClassConstIdentifierExprSyntax::withClassName(std::optional<Syntax> className)
{
   RefCountPtr<RawSyntax> classNameRaw;
   if (className.has_value()) {
      classNameRaw = className->getRaw();
   } else {
      classNameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ClassConstIdentifierExprSyntax>(classNameRaw, Cursor::ClassName);
}

ClassConstIdentifierExprSyntax ClassConstIdentifierExprSyntax::withSeparatorToken(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = make_missing_token(T_PAAMAYIM_NEKUDOTAYIM);
   }
   return m_data->replaceChild<ClassConstIdentifierExprSyntax>(separatorRaw, Cursor::SeparatorToken);
}

ClassConstIdentifierExprSyntax ClassConstIdentifierExprSyntax::withIdentifier(std::optional<TokenSyntax> identifier)
{
   RefCountPtr<RawSyntax> identifierRaw;
   if (identifier.has_value()) {
      identifierRaw = identifier->getRaw();
   } else {
      identifierRaw = RawSyntax::missing(SyntaxKind::Identifier);
   }
   return m_data->replaceChild<ClassConstIdentifierExprSyntax>(identifierRaw, Cursor::Identifier);
}

///
/// ConstExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ConstExprSyntax::CHILD_NODE_CHOICES
{
   {
      ConstExprSyntax::Identifier, {
         SyntaxKind::Name, SyntaxKind::ClassConstIdentifierExpr
      }
   }
};
#endif

void ConstExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ConstExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Identifier, CHILD_NODE_CHOICES.at(Cursor::Identifier));
#endif
}

Syntax ConstExprSyntax::getIdentifier()
{
   return Syntax {m_root, m_data->getChild(Cursor::Identifier).get()};
}

ConstExprSyntax ConstExprSyntax::withIdentifier(std::optional<Syntax> identifier)
{
   RefCountPtr<RawSyntax> identifierRaw;
   if (identifier.has_value()) {
      identifierRaw = identifier->getRaw();
   } else {
      identifierRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ConstExprSyntax>(identifierRaw, Cursor::Identifier);
}

///
/// NewVariableClauseSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType NewVariableClauseSyntax::CHILD_NODE_CHOICES
{
   {
      NewVariableClauseSyntax::VarNode, {
         SyntaxKind::SimpleVariableExpr, SyntaxKind::ArrayAccessExpr,
               SyntaxKind::BraceDecoratedArrayAccessExpr, SyntaxKind::InstancePropertyExpr,
               SyntaxKind::StaticPropertyExpr,
      }
   }
};
#endif

void NewVariableClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == NewVariableClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, VarNode, CHILD_NODE_CHOICES.at(Cursor::VarNode));
#endif
}

ExprSyntax NewVariableClauseSyntax::getVar()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::VarNode).get()};
}

NewVariableClauseSyntax NewVariableClauseSyntax::withVar(std::optional<ExprSyntax> var)
{
   RefCountPtr<RawSyntax> varRaw;
   if (var.has_value()) {
      varRaw = var->getRaw();
   } else {
      varRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<NewVariableClauseSyntax>(varRaw, Cursor::VarNode);
}

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType CallableVariableExprSyntax::CHILD_NODE_CHOICES
{
   {
      CallableVariableExprSyntax::Var, {
         SyntaxKind::SimpleVariableExpr, SyntaxKind::ArrayAccessExpr,
               SyntaxKind::BraceDecoratedArrayAccessExpr, SyntaxKind::InstanceMethodCallExpr,
               SyntaxKind::FunctionCallExpr
      }
   }
};
#endif

void CallableVariableExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CallableVariableExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Var, CHILD_NODE_CHOICES.at(Cursor::Var));
#endif
}

ExprSyntax CallableVariableExprSyntax::getVar()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Var).get()};
}

CallableVariableExprSyntax CallableVariableExprSyntax::withVar(std::optional<ExprSyntax> var)
{
   RefCountPtr<RawSyntax> varRaw;
   if (var.has_value()) {
      varRaw = var->getRaw();
   } else {
      varRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<CallableVariableExprSyntax>(varRaw, Cursor::Var);
}

///
/// CallableFuncNameClauseSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType CallableFuncNameClauseSyntax::CHILD_NODE_CHOICES
{
   {
      CallableFuncNameClauseSyntax::FuncName, {
         SyntaxKind::CallableVariableExpr, SyntaxKind::ParenDecoratedExpr,
               SyntaxKind::DereferencableScalarExpr,
      }
   }
};
#endif

void CallableFuncNameClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == CallableFuncNameClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, FuncName, CHILD_NODE_CHOICES.at(Cursor::FuncName));
#endif
}

Syntax CallableFuncNameClauseSyntax::getFuncName()
{
   return Syntax {m_root, m_data->getChild(Cursor::FuncName).get()};
}

CallableFuncNameClauseSyntax CallableFuncNameClauseSyntax::withFuncName(std::optional<Syntax> funcName)
{
   RefCountPtr<RawSyntax> funcNameRaw;
   if (funcName.has_value()) {
      funcNameRaw = funcName->getRaw();
   } else {
      funcNameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<CallableFuncNameClauseSyntax>(funcNameRaw, Cursor::FuncName);
}

///
/// MemberNameClauseSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType MemberNameClauseSyntax::CHILD_NODE_CHOICES
{
   {
      MemberNameClauseSyntax::Name, {
         SyntaxKind::Identifier,  SyntaxKind::BraceDecoratedExprClause,
               SyntaxKind::SimpleVariableExpr
      }
   }
};
#endif

void MemberNameClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == MemberNameClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Name, CHILD_NODE_CHOICES.at(Cursor::Name));
#endif
}

Syntax MemberNameClauseSyntax::getName()
{
   return Syntax {m_root, m_data->getChild(Cursor::Name).get()};
}

MemberNameClauseSyntax MemberNameClauseSyntax::withName(std::optional<Syntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<MemberNameClauseSyntax>(nameRaw, Cursor::Name);
}

///
/// PropertyNameClauseSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType PropertyNameClauseSyntax::CHILD_NODE_CHOICES
{
   {
      PropertyNameClauseSyntax::Name, {
         SyntaxKind::BraceDecoratedExprClause, SyntaxKind::SimpleVariableExpr
      }
   }
};
#endif

void PropertyNameClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ConstExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Name, CHILD_NODE_CHOICES.at(Cursor::Name));
#endif
}

Syntax PropertyNameClauseSyntax::getName()
{
   return Syntax {m_root, m_data->getChild(Cursor::Name).get()};
}

PropertyNameClauseSyntax PropertyNameClauseSyntax::withName(std::optional<Syntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<PropertyNameClauseSyntax>(nameRaw, Cursor::Name);
}

///
/// InstancePropertyExprSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType InstancePropertyExprSyntax::CHILD_NODE_CHOICES
{
   {
      InstancePropertyExprSyntax::ObjectRef, {
         SyntaxKind::NewVariableClause, SyntaxKind::DereferencableClause
      }
   }
};
#endif

void InstancePropertyExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == InstancePropertyExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, ObjectRef, CHILD_NODE_CHOICES.at(Cursor::ObjectRef));
   syntax_assert_child_token(raw, Separator, std::set{TokenKindType::T_OBJECT_OPERATOR});
   if (const RefCountPtr<RawSyntax> &propertyChild = raw->getChild(Cursor::PropertyName)) {
      if (propertyChild->isToken()) {
         syntax_assert_child_token(raw, PropertyName, std::set{TokenKindType::T_IDENTIFIER_STRING});
      } else {
         syntax_assert_child_kind(raw, PropertyName, std::set{SyntaxKind::PropertyNameClause});
      }
   }
#endif
}

Syntax InstancePropertyExprSyntax::getObjectRef()
{
   return Syntax {m_root, m_data->getChild(Cursor::ObjectRef).get()};
}

TokenSyntax InstancePropertyExprSyntax::getSeparator()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Separator).get()};
}

Syntax InstancePropertyExprSyntax::getPropertyName()
{
   return Syntax {m_root, m_data->getChild(Cursor::PropertyName).get()};
}

InstancePropertyExprSyntax
InstancePropertyExprSyntax::withObjectRef(std::optional<Syntax> objectRef)
{
   RefCountPtr<RawSyntax> objectRefRaw;
   if (objectRef.has_value()) {
      objectRefRaw = objectRef->getRaw();
   } else {
      objectRefRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<InstancePropertyExprSyntax>(objectRefRaw, Cursor::ObjectRef);
}

InstancePropertyExprSyntax
InstancePropertyExprSyntax::withSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = make_missing_token(T_OBJECT_OPERATOR);
   }
   return m_data->replaceChild<InstancePropertyExprSyntax>(separatorRaw, Cursor::Separator);
}

InstancePropertyExprSyntax
InstancePropertyExprSyntax::withPropertyName(std::optional<Syntax> propertyName)
{
   RefCountPtr<RawSyntax> propertyNameRaw;
   if (propertyName.has_value()) {
      propertyNameRaw = propertyName->getRaw();
   } else {
      propertyNameRaw = RawSyntax::missing(SyntaxKind::PropertyNameClause);
   }
   return m_data->replaceChild<InstancePropertyExprSyntax>(propertyNameRaw, Cursor::PropertyName);
}

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType StaticPropertyExprSyntax::CHILD_NODE_CHOICES
{
   {
      StaticPropertyExprSyntax::ClassName, {
         SyntaxKind::ClassNameClause, SyntaxKind::VariableClassNameClause,
               SyntaxKind::NewVariableClause
      }
   }
};
#endif

void StaticPropertyExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StaticPropertyExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, ClassName, CHILD_NODE_CHOICES.at(Cursor::ClassName));
   syntax_assert_child_token(raw, Separator, std::set{TokenKindType::T_OBJECT_OPERATOR});
   syntax_assert_child_kind(raw, MemberName, std::set{SyntaxKind::SimpleVariableExpr});
#endif
}

Syntax StaticPropertyExprSyntax::getClassName()
{
   return Syntax {m_root, m_data->getChild(Cursor::ClassName).get()};
}

TokenSyntax StaticPropertyExprSyntax::getSeparator()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Separator).get()};
}

SimpleVariableExprSyntax StaticPropertyExprSyntax::getMemberName()
{
   return SimpleVariableExprSyntax {m_root, m_data->getChild(Cursor::MemberName).get()};
}

StaticPropertyExprSyntax StaticPropertyExprSyntax::withClassName(std::optional<Syntax> className)
{
   RefCountPtr<RawSyntax> classNameRaw;
   if (className.has_value()) {
      classNameRaw = className->getRaw();
   } else {
      classNameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<StaticPropertyExprSyntax>(classNameRaw, Cursor::ClassName);
}

StaticPropertyExprSyntax StaticPropertyExprSyntax::withSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = make_missing_token(T_PAAMAYIM_NEKUDOTAYIM);
   }
   return m_data->replaceChild<StaticPropertyExprSyntax>(separatorRaw, Cursor::Separator);
}

StaticPropertyExprSyntax
StaticPropertyExprSyntax::withMemberName(std::optional<SimpleVariableExprSyntax> memberName)
{
   RefCountPtr<RawSyntax> memberNameRaw;
   if (memberName.has_value()) {
      memberNameRaw = memberName->getRaw();
   } else {
      memberNameRaw = RawSyntax::missing(SyntaxKind::SimpleVariableExpr);
   }
   return m_data->replaceChild<StaticPropertyExprSyntax>(memberNameRaw, Cursor::MemberName);
}

///
/// ArgumentSyntax
///
void ArgumentSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArgumentSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, EllipsisToken, std::set{TokenKindType::T_ELLIPSIS});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
#endif
}

std::optional<TokenSyntax> ArgumentSyntax::getEllipsisToken()
{
   RefCountPtr<SyntaxData> ellipsisTokenData = m_data->getChild(Cursor::EllipsisToken);
   if (!ellipsisTokenData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, ellipsisTokenData.get()};
}

ExprSyntax ArgumentSyntax::getExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Expr).get()};
}

ArgumentSyntax ArgumentSyntax::withEllipsisToken(std::optional<TokenSyntax> ellipsisToken)
{
   RefCountPtr<RawSyntax> ellipsisTokenRaw;
   if (ellipsisToken.has_value()) {
      ellipsisTokenRaw = ellipsisToken->getRaw();
   } else {
      ellipsisTokenRaw = nullptr;
   }
   return m_data->replaceChild<ArgumentSyntax>(ellipsisTokenRaw, Cursor::EllipsisToken);
}

ArgumentSyntax ArgumentSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> exprRaw;
   if (expr.has_value()) {
      exprRaw = expr->getRaw();
   } else {
      exprRaw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ArgumentSyntax>(exprRaw, Cursor::Expr);
}

///
/// ArgumentListItemSyntax
///
void ArgumentListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArgumentListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Argument, std::set{SyntaxKind::Argument});
   syntax_assert_child_token(raw, TrailingComma, std::set{TokenKindType::T_COMMA});
#endif
}

ArgumentSyntax ArgumentListItemSyntax::getArgument()
{
   return ArgumentSyntax {m_root, m_data->getChild(Cursor::Argument).get()};
}

std::optional<TokenSyntax> ArgumentListItemSyntax::getTokenSyntax()
{
   RefCountPtr<SyntaxData> tokenSyntaxData = m_data->getChild(Cursor::Argument);
   if (!tokenSyntaxData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, tokenSyntaxData.get()};
}

ArgumentListItemSyntax ArgumentListItemSyntax::withArgument(std::optional<ArgumentSyntax> argument)
{
   RefCountPtr<RawSyntax> argumentRaw;
   if (argument.has_value()) {
      argumentRaw = argument->getRaw();
   } else {
      argumentRaw = RawSyntax::missing(SyntaxKind::Argument);
   }
   return m_data->replaceChild<ArgumentListItemSyntax>(argumentRaw, Cursor::Argument);
}

ArgumentListItemSyntax ArgumentListItemSyntax::withTrailingComma(std::optional<TokenSyntax> comma)
{
   RefCountPtr<RawSyntax> commaRaw;
   if (comma.has_value()) {
      commaRaw = comma->getRaw();
   } else {
      commaRaw = nullptr;
   }
   return m_data->replaceChild<ArgumentListItemSyntax>(commaRaw, Cursor::TrailingComma);
}

///
/// ArgumentListClauseSyntax
///
void ArgumentListClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArgumentListClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftParenToken, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_kind(raw, Arguments, std::set{SyntaxKind::ArgumentList});
   syntax_assert_child_token(raw, RightParenToken, std::set{TokenKindType::T_RIGHT_PAREN});
#endif
}

TokenSyntax ArgumentListClauseSyntax::getLeftParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParenToken).get()};
}

std::optional<ArgumentListSyntax> ArgumentListClauseSyntax::getArguments()
{
   RefCountPtr<SyntaxData> argumentsData = m_data->getChild(Cursor::Arguments);
   if (!argumentsData) {
      return std::nullopt;
   }
   return ArgumentListSyntax {m_root, argumentsData.get()};
}

TokenSyntax ArgumentListClauseSyntax::getRightParenToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParenToken).get()};
}

ArgumentListClauseSyntax ArgumentListClauseSyntax::withLeftParenToken(std::optional<TokenSyntax> leftParenToken)
{
   RefCountPtr<RawSyntax> leftParenTokenRaw;
   if (leftParenToken.has_value()) {
      leftParenTokenRaw = leftParenToken->getRaw();
   } else {
      leftParenTokenRaw = make_missing_token(T_LEFT_PAREN);
   }
   return m_data->replaceChild<ArgumentListClauseSyntax>(leftParenTokenRaw, Cursor::LeftParenToken);
}

ArgumentListClauseSyntax ArgumentListClauseSyntax::withArguments(std::optional<ArgumentListSyntax> arguments)
{
   RefCountPtr<RawSyntax> argumentsRaw;
   if (arguments.has_value()) {
      argumentsRaw = arguments->getRaw();
   } else {
      argumentsRaw = nullptr;
   }
   return m_data->replaceChild<ArgumentListClauseSyntax>(argumentsRaw, Cursor::Arguments);
}

ArgumentListClauseSyntax ArgumentListClauseSyntax::withRightParenToken(std::optional<TokenSyntax> rightParenToken)
{
   RefCountPtr<RawSyntax> rightParenTokenRaw;
   if (rightParenToken.has_value()) {
      rightParenTokenRaw = rightParenToken->getRaw();
   } else {
      rightParenTokenRaw = make_missing_token(T_RIGHT_PAREN);
   }
   return m_data->replaceChild<ArgumentListClauseSyntax>(rightParenTokenRaw, Cursor::RightParenToken);
}

///
/// DereferencableClauseSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType DereferencableClauseSyntax::CHILD_NODE_CHOICES
{
   {
      DereferencableClauseSyntax::DereferencableExpr, {
         SyntaxKind::VariableExpr, SyntaxKind::ParenDecoratedExpr,
               SyntaxKind::DereferencableScalarExpr
      }
   }
};
#endif

void DereferencableClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DereferencableClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, DereferencableExpr, CHILD_NODE_CHOICES.at(Cursor::DereferencableExpr));
#endif
}

ExprSyntax DereferencableClauseSyntax::getDereferencableExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::DereferencableExpr).get()};
}

DereferencableClauseSyntax
DereferencableClauseSyntax::withDereferencableExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> exprRaw;
   if (expr.has_value()) {
      exprRaw = expr->getRaw();
   } else {
      exprRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<DereferencableClauseSyntax>(exprRaw, Cursor::DereferencableExpr);
}

///
/// VariableClassNameClauseSyntax
///
void VariableClassNameClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == VariableClassNameClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, DereferencableExpr, std::set{SyntaxKind::Expr});
#endif
}

ExprSyntax VariableClassNameClauseSyntax::getDereferencableExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::DereferencableExpr).get()};
}

VariableClassNameClauseSyntax
VariableClassNameClauseSyntax::withDereferencableExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> exprRaw;
   if (expr.has_value()) {
      exprRaw = expr->getRaw();
   } else {
      exprRaw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<VariableClassNameClauseSyntax>(exprRaw, Cursor::DereferencableExpr);
}

///
/// ClassNameClauseSyntax
///
void ClassNameClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassNameClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Name, std::set{SyntaxKind::Name});
   syntax_assert_child_token(raw, Name, std::set{TokenKindType::T_STATIC});
#endif
}

Syntax ClassNameClauseSyntax::getName()
{
   return Syntax {m_root, m_data->getChild(Cursor::Name).get()};
}

ClassNameClauseSyntax ClassNameClauseSyntax::withName(std::optional<Syntax> name)
{
   RefCountPtr<RawSyntax> nameRaw;
   if (name.has_value()) {
      nameRaw = name->getRaw();
   } else {
      nameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ClassNameClauseSyntax>(nameRaw, Cursor::Name);
}

///
/// BraceDecoratedExprClauseSyntax
///
void BraceDecoratedExprClauseSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BraceDecoratedExprClauseSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftBrace, std::set{TokenKindType::T_LEFT_BRACE});
   syntax_assert_child_token(raw, RightBrace, std::set{TokenKindType::T_RIGHT_BRACE});
   if (const RefCountPtr<RawSyntax> &exprChild = raw->getChild(Cursor::Expr)) {
      assert(exprChild->kindOf(SyntaxKind::Expr));
   }
#endif
}

TokenSyntax BraceDecoratedExprClauseSyntax::getLeftBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftBrace).get()};
}

ExprSyntax BraceDecoratedExprClauseSyntax::getExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax BraceDecoratedExprClauseSyntax::getRightBrace()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightBrace).get()};
}

BraceDecoratedExprClauseSyntax BraceDecoratedExprClauseSyntax::withLeftBrace(std::optional<TokenSyntax> leftBrace)
{
   RefCountPtr<RawSyntax> leftBraceRaw;
   if (leftBrace.has_value()) {
      leftBraceRaw = leftBrace->getRaw();
   } else {
      leftBraceRaw = RawSyntax::missing(TokenKindType::T_LEFT_BRACE,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_BRACE)));
   }
   return m_data->replaceChild<BraceDecoratedExprClauseSyntax>(leftBraceRaw, Cursor::LeftBrace);
}

BraceDecoratedExprClauseSyntax BraceDecoratedExprClauseSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> exprRaw;
   if (expr.has_value()) {
      exprRaw = expr->getRaw();
   } else {
      exprRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<BraceDecoratedExprClauseSyntax>(exprRaw, Cursor::Expr);
}

BraceDecoratedExprClauseSyntax BraceDecoratedExprClauseSyntax::withRightBrace(std::optional<TokenSyntax> rightBrace)
{
   RefCountPtr<RawSyntax> rightBraceRaw;
   if (rightBrace.has_value()) {
      rightBraceRaw = rightBrace->getRaw();
   } else {
      rightBraceRaw = RawSyntax::missing(TokenKindType::T_RIGHT_BRACE,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_BRACE)));
   }
   return m_data->replaceChild<BraceDecoratedExprClauseSyntax>(rightBraceRaw, Cursor::RightBrace);
}

///
/// BraceDecoratedVariableExprSyntax
///
void BraceDecoratedVariableExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BraceDecoratedVariableExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DollarSign, std::set{TokenKindType::T_DOLLAR_SIGN});
   if (const RefCountPtr<RawSyntax> &exprChild = raw->getChild(Cursor::DecoratedExpr)) {
      assert(exprChild->kindOf(SyntaxKind::BraceDecoratedExprClause));
   }
#endif
}

TokenSyntax BraceDecoratedVariableExprSyntax::getDollarSign()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::DollarSign).get()};
}

BraceDecoratedExprClauseSyntax BraceDecoratedVariableExprSyntax::getDecoratedExpr()
{
   return BraceDecoratedExprClauseSyntax{m_root, m_data->getChild(Cursor::DecoratedExpr).get()};
}

BraceDecoratedVariableExprSyntax
BraceDecoratedVariableExprSyntax::withDollarSign(std::optional<TokenSyntax> dollarSign)
{
   RefCountPtr<RawSyntax> dollarSignRaw;
   if (dollarSign.has_value()) {
      dollarSignRaw = dollarSign->getRaw();
   } else {
      dollarSignRaw = RawSyntax::missing(TokenKindType::T_DOLLAR_SIGN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_DOLLAR_SIGN)));
   }
   return m_data->replaceChild<BraceDecoratedVariableExprSyntax>(dollarSignRaw, Cursor::DollarSign);
}

BraceDecoratedVariableExprSyntax
BraceDecoratedVariableExprSyntax::withDecoratedExpr(std::optional<BraceDecoratedExprClauseSyntax> decoratedExpr)
{
   RefCountPtr<RawSyntax> decoratedExprRaw;
   if (decoratedExpr.has_value()) {
      decoratedExprRaw = decoratedExpr->getRaw();
   } else {
      decoratedExprRaw = RawSyntax::missing(SyntaxKind::BraceDecoratedExprClause);
   }
   return m_data->replaceChild<BraceDecoratedVariableExprSyntax>(decoratedExprRaw, Cursor::DecoratedExpr);
}

///
/// ArrayKeyValuePairItemSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ArrayKeyValuePairItemSyntax::CHILD_NODE_CHOICES
{
   {
      ArrayKeyValuePairItemSyntax::Value, {
         SyntaxKind::Expr, SyntaxKind::VariableExpr
      }
   }
};
#endif

void ArrayKeyValuePairItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArrayKeyValuePairItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, KeyExpr, std::set{TokenKindType::T_DOUBLE_ARROW});
   syntax_assert_child_token(raw, DoubleArrowToken, std::set{TokenKindType::T_DOUBLE_ARROW});
   syntax_assert_child_token(raw, ReferenceToken, std::set{TokenKindType::T_AMPERSAND});
   syntax_assert_child_kind(raw, Value, CHILD_NODE_CHOICES.at(Cursor::Value));
#endif
}

std::optional<ExprSyntax> ArrayKeyValuePairItemSyntax::getKeyExpr()
{
   RefCountPtr<SyntaxData> keyExprData = m_data->getChild(Cursor::KeyExpr);
   if (!keyExprData) {
      return std::nullopt;
   }
   return ExprSyntax {m_root, keyExprData.get()};
}

std::optional<TokenSyntax> ArrayKeyValuePairItemSyntax::getDoubleArrowToken()
{
   RefCountPtr<SyntaxData> doubleArrowTokenData = m_data->getChild(Cursor::DoubleArrowToken);
   if (!doubleArrowTokenData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, doubleArrowTokenData.get()};
}

std::optional<TokenSyntax> ArrayKeyValuePairItemSyntax::getReferenceToken()
{
   RefCountPtr<SyntaxData> referenceTokenData;
   if (!referenceTokenData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, referenceTokenData.get()};
}

ExprSyntax ArrayKeyValuePairItemSyntax::getValue()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Value).get()};
}

ArrayKeyValuePairItemSyntax ArrayKeyValuePairItemSyntax::withKeyExpr(std::optional<ExprSyntax> keyExpr)
{
   RefCountPtr<RawSyntax> keyExprRaw;
   if (keyExpr.has_value()) {
      keyExprRaw = keyExpr->getRaw();
   } else {
      keyExprRaw = nullptr;
   }
   return m_data->replaceChild<ArrayKeyValuePairItemSyntax>(keyExprRaw, Cursor::KeyExpr);
}

ArrayKeyValuePairItemSyntax ArrayKeyValuePairItemSyntax::withDoubleArrowToken(std::optional<TokenSyntax> doubleArrowToken)
{
   RefCountPtr<RawSyntax> doubleArrowTokenRaw;
   if (doubleArrowToken.has_value()) {
      doubleArrowTokenRaw = doubleArrowToken->getRaw();
   } else {
      doubleArrowTokenRaw = nullptr;
   }
   return m_data->replaceChild<ArrayKeyValuePairItemSyntax>(doubleArrowTokenRaw, Cursor::DoubleArrowToken);
}

ArrayKeyValuePairItemSyntax ArrayKeyValuePairItemSyntax::withReferenceToken(std::optional<TokenSyntax> referenceToken)
{
   RefCountPtr<RawSyntax> referenceTokenRaw;
   if (referenceToken.has_value()) {
      referenceTokenRaw = referenceToken->getRaw();
   } else {
      referenceTokenRaw = nullptr;
   }
   return m_data->replaceChild<ArrayKeyValuePairItemSyntax>(referenceTokenRaw, Cursor::ReferenceToken);
}

ArrayKeyValuePairItemSyntax ArrayKeyValuePairItemSyntax::withValue(std::optional<ExprSyntax> value)
{
   RefCountPtr<RawSyntax> valueRaw;
   if (value.has_value()) {
      valueRaw = value->getRaw();
   } else {
      valueRaw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ArrayKeyValuePairItemSyntax>(valueRaw, Cursor::Value);
}

///
/// ArrayUnpackPairItemSyntax
///
void ArrayUnpackPairItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArrayUnpackPairItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, EllipsisToken, std::set{TokenKindType::T_ELLIPSIS});
   syntax_assert_child_kind(raw, UnpackExpr, std::set{SyntaxKind::Expr});
#endif
}

TokenSyntax ArrayUnpackPairItemSyntax::getEllipsisToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::EllipsisToken).get()};
}

ExprSyntax ArrayUnpackPairItemSyntax::getUnpackExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::UnpackExpr).get()};
}

ArrayUnpackPairItemSyntax ArrayUnpackPairItemSyntax::withEllipsisToken(std::optional<TokenSyntax> ellipsisToken)
{
   RefCountPtr<RawSyntax> ellipsisTokenRaw;
   if (ellipsisToken.has_value()) {
      ellipsisTokenRaw = ellipsisToken->getRaw();
   } else {
      ellipsisTokenRaw = RawSyntax::missing(TokenKindType::T_ELLIPSIS,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_ELLIPSIS)));
   }
   return m_data->replaceChild<ArrayUnpackPairItemSyntax>(ellipsisTokenRaw, Cursor::EllipsisToken);
}

ArrayUnpackPairItemSyntax ArrayUnpackPairItemSyntax::withUnpackExpr(std::optional<ExprSyntax> unpackExpr)
{
   RefCountPtr<RawSyntax> unpackExprRaw;
   if (unpackExpr.has_value()) {
      unpackExprRaw = unpackExpr->getRaw();
   } else {
      unpackExprRaw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<ArrayUnpackPairItemSyntax>(unpackExprRaw, Cursor::UnpackExpr);
}

///
/// ArrayPairItemSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ArrayPairItemSyntax::CHILD_NODE_CHOICES
{
   {
      ArrayPairItemSyntax::Item, {
         SyntaxKind::ArrayKeyValuePairItem,
               SyntaxKind::ArrayUnpackPairItem
      }
   }
};
#endif

void ArrayPairItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArrayPairItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Item, CHILD_NODE_CHOICES.at(Cursor::Item));
#endif
}

Syntax ArrayPairItemSyntax::getItem()
{
   return Syntax {m_root, m_data->getChild(Cursor::Item).get()};
}

std::optional<TokenSyntax> ArrayPairItemSyntax::getTrailingComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::TrailingComma);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

ArrayPairItemSyntax ArrayPairItemSyntax::withItem(std::optional<Syntax> item)
{
   RefCountPtr<RawSyntax> itemRaw;
   if (item.has_value()) {
      itemRaw = item->getRaw();
   } else {
      itemRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ArrayPairItemSyntax>(itemRaw, Cursor::Item);
}

ArrayPairItemSyntax ArrayPairItemSyntax::withTrailingComma(std::optional<TokenSyntax> trailingComma)
{
   RefCountPtr<RawSyntax> trailingCommaRaw;
   if (trailingComma.has_value()) {
      trailingCommaRaw = trailingComma->getRaw();
   } else {
      trailingCommaRaw = nullptr;
   }
   return m_data->replaceChild<ArrayPairItemSyntax>(trailingCommaRaw, Cursor::TrailingComma);
}

///
/// ArrayKeyValuePairItemSyntax
///
void ListRecursivePairItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ListRecursivePairItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, KeyExpr, std::set{SyntaxKind::Expr});
   syntax_assert_child_kind(raw, ListPairItemList, std::set{SyntaxKind::ListPairItemList});
   syntax_assert_child_token(raw, DoubleArrowToken, std::set{TokenKindType::T_DOUBLE_ARROW});
   syntax_assert_child_token(raw, ListToken, std::set{TokenKindType::T_LIST});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
#endif
}

std::optional<ExprSyntax> ListRecursivePairItemSyntax::getKeyExpr()
{
   RefCountPtr<SyntaxData> keyExprData = m_data->getChild(Cursor::KeyExpr);
   if (!keyExprData) {
      return std::nullopt;
   }
   return ExprSyntax {m_root, keyExprData.get()};
}

std::optional<TokenSyntax> ListRecursivePairItemSyntax::getDoubleArrowToken()
{
   RefCountPtr<SyntaxData> arrowData = m_data->getChild(Cursor::DoubleArrowToken);
   if (!arrowData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, arrowData.get()};
}

TokenSyntax ListRecursivePairItemSyntax::getListToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ListToken).get()};
}

TokenSyntax ListRecursivePairItemSyntax::getLeftParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ListPairItemListSyntax ListRecursivePairItemSyntax::getListPairItemList()
{
   return ListPairItemListSyntax {m_root, m_data->getChild(Cursor::ListPairItemList).get()};
}

TokenSyntax ListRecursivePairItemSyntax::getRightParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParen).get()};
}

ListRecursivePairItemSyntax
ListRecursivePairItemSyntax::withKeyExpr(std::optional<ExprSyntax> keyExpr)
{
   RefCountPtr<RawSyntax> keyExprRaw;
   if (keyExpr.has_value()) {
      keyExprRaw = keyExpr->getRaw();
   } else {
      keyExprRaw = nullptr;
   }
   return m_data->replaceChild<ListRecursivePairItemSyntax>(keyExprRaw, Cursor::KeyExpr);
}

ListRecursivePairItemSyntax
ListRecursivePairItemSyntax::withDoubleArrowToken(std::optional<TokenSyntax> doubleArrowToken)
{
   RefCountPtr<RawSyntax> doubleArrowTokenRaw;
   if (doubleArrowToken.has_value()) {
      doubleArrowTokenRaw = doubleArrowToken->getRaw();
   } else {
      doubleArrowTokenRaw = nullptr;
   }
   return m_data->replaceChild<ListRecursivePairItemSyntax>(doubleArrowTokenRaw, Cursor::DoubleArrowToken);
}

ListRecursivePairItemSyntax
ListRecursivePairItemSyntax::withListToken(std::optional<TokenSyntax> listToken)
{
   RefCountPtr<RawSyntax> listTokenRaw;
   if (listToken.has_value()) {
      listTokenRaw = listToken->getRaw();
   } else {
      listTokenRaw = RawSyntax::missing(TokenKindType::T_LIST,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LIST)));
   }
   return m_data->replaceChild<ListRecursivePairItemSyntax>(listTokenRaw, Cursor::ListPairItemList);
}

ListRecursivePairItemSyntax
ListRecursivePairItemSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> leftParenRaw;
   if (leftParen.has_value()) {
      leftParenRaw = leftParen->getRaw();
   } else {
      leftParenRaw = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<ListRecursivePairItemSyntax>(leftParenRaw, Cursor::LeftParen);
}

ListRecursivePairItemSyntax
ListRecursivePairItemSyntax::withListPairItemList(std::optional<ListPairItemListSyntax> pairItemList)
{
   RefCountPtr<RawSyntax> pairItemListRaw;
   if (pairItemList.has_value()) {
      pairItemListRaw = pairItemList->getRaw();
   } else {
      pairItemListRaw = RawSyntax::missing(SyntaxKind::ListPairItemList);
   }
   return m_data->replaceChild<ListRecursivePairItemSyntax>(pairItemListRaw, Cursor::ListPairItemList);
}

ListRecursivePairItemSyntax ListRecursivePairItemSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rightParenRaw;
   if (rightParen.has_value()) {
      rightParenRaw = rightParen->getRaw();
   } else {
      rightParenRaw = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   return m_data->replaceChild<ListRecursivePairItemSyntax>(rightParenRaw, Cursor::RightParen);
}

///
/// ListPairItemSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ListPairItemSyntax::CHILD_NODE_CHOICES
{
   {
      ListPairItemSyntax::Item, {
         SyntaxKind::ArrayPairItem, SyntaxKind::ListRecursivePairItem
      }
   }
};
#endif

void ListPairItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ListPairItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Item, CHILD_NODE_CHOICES.at(Cursor::Item));
   syntax_assert_child_token(raw, TrailingComma, std::set{TokenKindType::T_COMMA});
#endif
}

Syntax ListPairItemSyntax::getItem()
{
   return Syntax {m_root, m_data->getChild(Cursor::Item).get()};
}

std::optional<TokenSyntax> ListPairItemSyntax::getTrailingComma()
{
   RefCountPtr<SyntaxData> commaData = m_data->getChild(Cursor::TrailingComma);
   if (!commaData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, commaData.get()};
}

ListPairItemSyntax ListPairItemSyntax::withItem(std::optional<Syntax> item)
{
   RefCountPtr<RawSyntax> itemRaw;
   if (item.has_value()) {
      itemRaw = item->getRaw();
   } else {
      itemRaw = RawSyntax::missing(SyntaxKind::ArrayPairItem);
   }
   return m_data->replaceChild<ListPairItemSyntax>(itemRaw, Cursor::Item);
}

ListPairItemSyntax ListPairItemSyntax::withTrailingComma(std::optional<TokenSyntax> trailingComma)
{
   RefCountPtr<RawSyntax> trailingCommaRaw;
   if (trailingComma.has_value()) {
      trailingCommaRaw = trailingComma->getRaw();
   } else {
      trailingCommaRaw = nullptr;
   }
   return m_data->replaceChild<ListPairItemSyntax>(trailingCommaRaw, Cursor::TrailingComma);
}

///
/// SimpleVariableExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType SimpleVariableExprSyntax::CHILD_NODE_CHOICES
{
   {
      SimpleVariableExprSyntax::Variable, {
         SyntaxKind::BraceDecoratedVariableExpr,
               SyntaxKind::SimpleVariableExpr
      }
   }
};
#endif

void SimpleVariableExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SimpleVariableExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DollarSign, std::set{TokenKindType::T_DOLLAR_SIGN});
   if (const RefCountPtr<RawSyntax> &variableChild = raw->getChild(Cursor::Variable)) {
      if (variableChild->isToken()) {
         syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
      } else {
         syntax_assert_child_kind(raw, Variable, CHILD_NODE_CHOICES.at(Cursor::Variable));
      }
      if (variableChild->kindOf(SyntaxKind::SimpleVariableExpr)) {
         assert(raw->getChild(Cursor::DollarSign));
      }
   }
#endif
}

std::optional<TokenSyntax> SimpleVariableExprSyntax::getDollarSign()
{
   RefCountPtr<SyntaxData> dollarSignData = m_data->getChild(Cursor::DollarSign);
   if (!dollarSignData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, dollarSignData.get()};
}

Syntax SimpleVariableExprSyntax::getVariable()
{
   return Syntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

SimpleVariableExprSyntax SimpleVariableExprSyntax::withDollarSign(std::optional<TokenSyntax> dollarSign)
{
   RefCountPtr<RawSyntax> dollarSignRaw;
   if (dollarSign.has_value()) {
      dollarSignRaw = dollarSign->getRaw();
   } else {
      dollarSignRaw = nullptr;
   }
   return m_data->replaceChild<SimpleVariableExprSyntax>(dollarSignRaw, Cursor::DollarSign);
}

SimpleVariableExprSyntax SimpleVariableExprSyntax::withVariable(std::optional<Syntax> variable)
{
   RefCountPtr<RawSyntax> variableRaw;
   if (variable.has_value()) {
      variableRaw = variable->getRaw();
   } else {
      variableRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<SimpleVariableExprSyntax>(variableRaw, Cursor::Variable);
}

///
/// ArrayCreateExprSyntax
///
void ArrayCreateExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArrayCreateExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ArrayToken, std::set{TokenKindType::T_ARRAY});
   syntax_assert_child_token(raw, LeftParen, std::set{TokenKindType::T_LEFT_PAREN});
   syntax_assert_child_token(raw, RightParen, std::set{TokenKindType::T_RIGHT_PAREN});
   syntax_assert_child_kind(raw, PairItemList, std::set{SyntaxKind::ArrayPairItemList});
#endif
}

TokenSyntax ArrayCreateExprSyntax::getArrayToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ArrayToken).get()};
}

TokenSyntax ArrayCreateExprSyntax::getLeftParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftParen).get()};
}

ArrayPairItemListSyntax ArrayCreateExprSyntax::getPairItemList()
{
   return ArrayPairItemListSyntax {m_root, m_data->getChild(Cursor::PairItemList).get()};
}

TokenSyntax ArrayCreateExprSyntax::getRightParen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightParen).get()};
}

ArrayCreateExprSyntax ArrayCreateExprSyntax::withArrayToken(std::optional<TokenSyntax> arrayToken)
{
   RefCountPtr<RawSyntax> arrayTokenRaw;
   if (arrayToken.has_value()) {
      arrayTokenRaw = arrayToken->getRaw();
   } else {
      arrayTokenRaw = RawSyntax::missing(TokenKindType::T_ARRAY,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_ARRAY)));
   }
   return m_data->replaceChild<ArrayCreateExprSyntax>(arrayTokenRaw, Cursor::ArrayToken);
}

ArrayCreateExprSyntax ArrayCreateExprSyntax::withLeftParen(std::optional<TokenSyntax> leftParen)
{
   RefCountPtr<RawSyntax> leftParanRaw;
   if (leftParen.has_value()) {
      leftParanRaw = leftParen->getRaw();
   } else {
      leftParanRaw = RawSyntax::missing(TokenKindType::T_LEFT_PAREN,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_PAREN)));
   }
   return m_data->replaceChild<ArrayCreateExprSyntax>(leftParanRaw, Cursor::LeftParen);
}

ArrayCreateExprSyntax ArrayCreateExprSyntax::withPairItemList(std::optional<ArrayPairItemListSyntax> pairItemList)
{
   RefCountPtr<RawSyntax> pairItemListRaw;
   if (pairItemList.has_value()) {
      pairItemListRaw = pairItemList->getRaw();
   } else {
      pairItemListRaw = RawSyntax::missing(SyntaxKind::ArrayPairItemList);
   }
   return m_data->replaceChild<ArrayCreateExprSyntax>(pairItemListRaw, Cursor::PairItemList);
}

ArrayCreateExprSyntax ArrayCreateExprSyntax::withRightParen(std::optional<TokenSyntax> rightParen)
{
   RefCountPtr<RawSyntax> rightParenRaw;
   if (rightParen.has_value()) {
      rightParenRaw = rightParen->getRaw();
   } else {
      rightParenRaw = RawSyntax::missing(TokenKindType::T_RIGHT_PAREN,
                                         OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_PAREN)));
   }
   return m_data->replaceChild<ArrayCreateExprSyntax>(rightParenRaw, Cursor::RightParen);
}

///
/// SimplifiedArrayCreateExprSyntax
///
void SimplifiedArrayCreateExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SimplifiedArrayCreateExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftSquareBracket, std::set{TokenKindType::T_LEFT_SQUARE_BRACKET});
   syntax_assert_child_token(raw, RightSquareBracket, std::set{TokenKindType::T_RIGHT_SQUARE_BRACKET});
   syntax_assert_child_kind(raw, PairItemList, std::set{SyntaxKind::ArrayPairItemList});
#endif
}

TokenSyntax SimplifiedArrayCreateExprSyntax::getLeftSquareBracket()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftSquareBracket).get()};
}

ArrayPairItemListSyntax SimplifiedArrayCreateExprSyntax::getPairItemList()
{
   return ArrayPairItemListSyntax {m_root, m_data->getChild(Cursor::PairItemList).get()};
}

TokenSyntax SimplifiedArrayCreateExprSyntax::getRightSquareBracket()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightSquareBracket).get()};
}

SimplifiedArrayCreateExprSyntax
SimplifiedArrayCreateExprSyntax::withLeftSquareBracket(std::optional<TokenSyntax> leftSquareBracket)
{
   RefCountPtr<RawSyntax> leftSquareBracketRaw;
   if (leftSquareBracket.has_value()) {
      leftSquareBracketRaw = leftSquareBracket->getRaw();
   } else {
      leftSquareBracketRaw = RawSyntax::missing(TokenKindType::T_LEFT_SQUARE_BRACKET,
                                                OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_SQUARE_BRACKET)));
   }
   return m_data->replaceChild<SimplifiedArrayCreateExprSyntax>(leftSquareBracketRaw, Cursor::LeftSquareBracket);
}

SimplifiedArrayCreateExprSyntax
SimplifiedArrayCreateExprSyntax::withPairItemList(std::optional<ArrayPairItemListSyntax> pairItemList)
{
   RefCountPtr<RawSyntax> pairItemListRaw;
   if (pairItemList.has_value()) {
      pairItemListRaw = pairItemList->getRaw();
   } else {
      pairItemListRaw = RawSyntax::missing(SyntaxKind::ArrayPairItemList);
   }
   return m_data->replaceChild<SimplifiedArrayCreateExprSyntax>(pairItemListRaw, Cursor::PairItemList);
}

SimplifiedArrayCreateExprSyntax
SimplifiedArrayCreateExprSyntax::withRightSquareBracket(std::optional<TokenSyntax> rightSquareBracket)
{
   RefCountPtr<RawSyntax> rightSquareBracketRaw;
   if (rightSquareBracket.has_value()) {
      rightSquareBracketRaw = rightSquareBracket->getRaw();
   } else {
      rightSquareBracketRaw = RawSyntax::missing(TokenKindType::T_RIGHT_SQUARE_BRACKET,
                                                 OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_SQUARE_BRACKET)));
   }
   return m_data->replaceChild<SimplifiedArrayCreateExprSyntax>(rightSquareBracketRaw, Cursor::RightSquareBracket);
}

///
/// ArrayAccessExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType ArrayAccessExprSyntax::CHILD_NODE_CHOICES
{
   {
      ArrayAccessExprSyntax::ArrayRef, {
         SyntaxKind::NewVariableClause, SyntaxKind::DereferencableClause,
               SyntaxKind::ConstExpr
      }
   },
   {
      ArrayAccessExprSyntax::Offset, {
         SyntaxKind::EncapsVarOffset, SyntaxKind::OptionalExpr,
      }
   }
};
#endif

void ArrayAccessExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ArrayAccessExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftSquareBracket, std::set{TokenKindType::T_LEFT_SQUARE_BRACKET});
   syntax_assert_child_token(raw, RightSquareBracket, std::set{TokenKindType::T_RIGHT_SQUARE_BRACKET});
   syntax_assert_child_kind(raw, Offset, CHILD_NODE_CHOICES.at(Cursor::Offset));
   if (const RefCountPtr<RawSyntax> &arrayRefChild = raw->getChild(Cursor::ArrayRef)) {
      if (arrayRefChild->isToken()) {
         syntax_assert_child_token(raw, ArrayRef, std::set{TokenKindType::T_VARIABLE});
      } else {
         syntax_assert_child_kind(raw, ArrayRef, CHILD_NODE_CHOICES.at(Cursor::ArrayRef));
      }
   }
#endif
}

Syntax ArrayAccessExprSyntax::getArrayRef()
{
   return Syntax {m_root, m_data->getChild(Cursor::ArrayRef).get()};
}

TokenSyntax ArrayAccessExprSyntax::getLeftSquareBracket()
{
   return TokenSyntax {m_root, m_data->getChild(LeftSquareBracket).get()};
}

Syntax ArrayAccessExprSyntax::getOffset()
{
   return Syntax {m_root, m_data->getChild(Cursor::Offset).get()};
}

TokenSyntax ArrayAccessExprSyntax::getRightSquareBracket()
{
   return TokenSyntax {m_root, m_data->getChild(RightSquareBracket).get()};
}

ArrayAccessExprSyntax
ArrayAccessExprSyntax::withArrayRef(std::optional<Syntax> arrayRef)
{
   RefCountPtr<RawSyntax> arrayRefRaw;
   if (arrayRef.has_value()) {
      arrayRefRaw = arrayRef->getRaw();
   } else {
      arrayRefRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ArrayAccessExprSyntax>(arrayRefRaw, Cursor::ArrayRef);
}

ArrayAccessExprSyntax
ArrayAccessExprSyntax::withLeftSquareBracket(std::optional<TokenSyntax> leftSquareBracket)
{
   RefCountPtr<RawSyntax> leftSquareBracketRaw;
   if (leftSquareBracket.has_value()) {
      leftSquareBracketRaw = leftSquareBracket->getRaw();
   } else {
      leftSquareBracketRaw = make_missing_token(T_LEFT_SQUARE_BRACKET);
   }
   return m_data->replaceChild<ArrayAccessExprSyntax>(leftSquareBracketRaw, Cursor::LeftSquareBracket);
}

ArrayAccessExprSyntax ArrayAccessExprSyntax::withOffset(std::optional<Syntax> offset)
{
   RefCountPtr<RawSyntax> offsetRaw;
   if (offset.has_value()) {
      offsetRaw = offset->getRaw();
   } else {
      offsetRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ArrayAccessExprSyntax>(offsetRaw, Cursor::Offset);
}

ArrayAccessExprSyntax
ArrayAccessExprSyntax::withRightSquareBracket(std::optional<TokenSyntax> rightSquareBracket)
{
   RefCountPtr<RawSyntax> rightSquareBracketRaw;
   if (rightSquareBracket.has_value()) {
      rightSquareBracketRaw = rightSquareBracket->getRaw();
   } else {
      rightSquareBracketRaw = make_missing_token(T_RIGHT_SQUARE_BRACKET);
   }
   return m_data->replaceChild<ArrayAccessExprSyntax>(rightSquareBracketRaw, Cursor::RightSquareBracket);
}

///
/// BraceDecoratedArrayAccessExprSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType BraceDecoratedArrayAccessExprSyntax::CHILD_NODE_CHOICES
{
   {
      BraceDecoratedArrayAccessExprSyntax::ArrayRef, {
         SyntaxKind::NewVariableClause, SyntaxKind::DereferencableClause
      }
   }
};
#endif

void BraceDecoratedArrayAccessExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BraceDecoratedArrayAccessExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, ArrayRef, CHILD_NODE_CHOICES.at(Cursor::ArrayRef));
   syntax_assert_child_kind(raw, OffsetExpr, std::set{SyntaxKind::BraceDecoratedExprClause});
#endif
}

Syntax BraceDecoratedArrayAccessExprSyntax::getArrayRef()
{
   return Syntax {m_root, m_data->getChild(Cursor::ArrayRef).get()};
}

BraceDecoratedExprClauseSyntax BraceDecoratedArrayAccessExprSyntax::getOffsetExpr()
{
   return BraceDecoratedExprClauseSyntax {m_root, m_data->getChild(Cursor::OffsetExpr).get()};
}

BraceDecoratedArrayAccessExprSyntax
BraceDecoratedArrayAccessExprSyntax::withArrayRef(std::optional<Syntax> arrayRef)
{
   RefCountPtr<RawSyntax> arrayRefRaw;
   if (arrayRef.has_value()) {
      arrayRefRaw = arrayRef->getRaw();
   } else {
      arrayRefRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<BraceDecoratedArrayAccessExprSyntax>(arrayRefRaw, Cursor::ArrayRef);
}

BraceDecoratedArrayAccessExprSyntax
BraceDecoratedArrayAccessExprSyntax::withOffsetExpr(std::optional<BraceDecoratedExprClauseSyntax> offsetExpr)
{
   RefCountPtr<RawSyntax> offsetExprRaw;
   if (offsetExpr.has_value()) {
      offsetExprRaw = offsetExpr->getRaw();
   } else {
      offsetExprRaw = RawSyntax::missing(SyntaxKind::BraceDecoratedExprClause);
   }
   return m_data->replaceChild<BraceDecoratedArrayAccessExprSyntax>(offsetExprRaw, Cursor::OffsetExpr);
}

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType SimpleFunctionCallExprSyntax::CHILD_NODE_CHOICES
{
   {
      SimpleFunctionCallExprSyntax::FuncName, {
         SyntaxKind::Name, SyntaxKind::CallableFuncNameClause
      }
   }
};
#endif

void SimpleFunctionCallExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SimpleFunctionCallExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, FuncName, CHILD_NODE_CHOICES.at(Cursor::FuncName));
   syntax_assert_child_kind(raw, ArgumentsClause, std::set{SyntaxKind::ArgumentListClause});
#endif
}

Syntax SimpleFunctionCallExprSyntax::getFuncName()
{
   return Syntax {m_root, m_data->getChild(Cursor::FuncName).get()};
}

ArgumentListClauseSyntax SimpleFunctionCallExprSyntax::getArgumentsClause()
{
   return ArgumentListClauseSyntax {m_root, m_data->getChild(Cursor::ArgumentsClause).get()};
}

SimpleFunctionCallExprSyntax
SimpleFunctionCallExprSyntax::withFuncName(std::optional<Syntax> funcName)
{
   RefCountPtr<RawSyntax> funcNameRaw;
   if (funcName.has_value()) {
      funcNameRaw = funcName->getRaw();
   } else {
      funcNameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<SimpleFunctionCallExprSyntax>(funcNameRaw, Cursor::FuncName);
}

SimpleFunctionCallExprSyntax
SimpleFunctionCallExprSyntax::withArgumentsClause(std::optional<ArgumentListClauseSyntax> argumentsClause)
{
   RefCountPtr<RawSyntax> argumentsClauseRaw;
   if (argumentsClause.has_value()) {
      argumentsClauseRaw = argumentsClause->getRaw();
   } else {
      argumentsClauseRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<SimpleFunctionCallExprSyntax>(argumentsClauseRaw, Cursor::ArgumentsClause);
}

///
/// FunctionCallExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType FunctionCallExprSyntax::CHILD_NODE_CHOICES
{
   {
      FunctionCallExprSyntax::Callable, {
         SyntaxKind::SimpleFunctionCallExpr, SyntaxKind::StaticMethodCallExpr
      }
   }
};
#endif

void FunctionCallExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FunctionCallExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, Callable, CHILD_NODE_CHOICES.at(Cursor::Callable));
#endif
}

ExprSyntax FunctionCallExprSyntax::getCallable()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::Callable).get()};
}

FunctionCallExprSyntax FunctionCallExprSyntax::withCallable(std::optional<ExprSyntax> callable)
{
   RefCountPtr<RawSyntax> callableRaw;
   if (callable.has_value()) {
      callableRaw = callable->getRaw();
   } else {
      callableRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<FunctionCallExprSyntax>(callableRaw, Cursor::Callable);
}

///
/// InstanceMethodCallExprSyntax
///
void InstanceMethodCallExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == InstanceMethodCallExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, QualifiedMethodName, std::set{SyntaxKind::InstancePropertyExpr});
   syntax_assert_child_kind(raw, ArgumentListClause, std::set{SyntaxKind::ArgumentListClause});
#endif
}

InstancePropertyExprSyntax InstanceMethodCallExprSyntax::getQualifiedMethodName()
{
   return InstancePropertyExprSyntax {m_root, m_data->getChild(Cursor::QualifiedMethodName).get()};
}

ArgumentListClauseSyntax InstanceMethodCallExprSyntax::getArgumentListClause()
{
   return ArgumentListClauseSyntax {m_root, m_data->getChild(Cursor::ArgumentListClause).get()};
}

InstanceMethodCallExprSyntax
InstanceMethodCallExprSyntax::withQualifiedMethodName(std::optional<InstancePropertyExprSyntax> methodName)
{
   RefCountPtr<RawSyntax> methodNameRaw;
   if (methodName.has_value()) {
      methodNameRaw = methodName->getRaw();
   } else {
      methodNameRaw = RawSyntax::missing(SyntaxKind::InstancePropertyExpr);
   }
   return m_data->replaceChild<InstanceMethodCallExprSyntax>(methodNameRaw, Cursor::QualifiedMethodName);
}

InstanceMethodCallExprSyntax
InstanceMethodCallExprSyntax::withArgumentListClause(std::optional<ArgumentListClauseSyntax> arguments)
{
   RefCountPtr<RawSyntax> argumentsRaw;
   if (arguments.has_value()) {
      argumentsRaw = arguments->getRaw();
   } else {
      argumentsRaw = RawSyntax::missing(SyntaxKind::ArgumentListClause);
   }
   return m_data->replaceChild<InstanceMethodCallExprSyntax>(argumentsRaw, Cursor::ArgumentListClause);
}

///
/// StaticMethodCallExprSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType StaticMethodCallExprSyntax::CHILD_NODE_CHOICES
{
   {
      StaticMethodCallExprSyntax::ClassName, {
         SyntaxKind::ClassNameClause, SyntaxKind::VariableClassNameClause,
      }
   }
};
#endif

void StaticMethodCallExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StaticMethodCallExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, ClassName, CHILD_NODE_CHOICES.at(Cursor::ClassName));
   syntax_assert_child_token(raw, Separator, std::set{TokenKindType::T_PAAMAYIM_NEKUDOTAYIM});
   syntax_assert_child_kind(raw, MethodName, std::set{SyntaxKind::MemberNameClause});
   syntax_assert_child_kind(raw, Arguments, std::set{SyntaxKind::ArgumentListClause});
#endif
}

Syntax StaticMethodCallExprSyntax::getClassName()
{
   return Syntax {m_root, m_data->getChild(Cursor::ClassName).get()};
}

TokenSyntax StaticMethodCallExprSyntax::getSeparator()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Separator).get()};
}

MemberNameClauseSyntax StaticMethodCallExprSyntax::getMethodName()
{
   return MemberNameClauseSyntax {m_root, m_data->getChild(Cursor::MethodName).get()};
}

ArgumentListClauseSyntax StaticMethodCallExprSyntax::getArguments()
{
   return ArgumentListClauseSyntax {m_root, m_data->getChild(Cursor::Arguments).get()};
}

StaticMethodCallExprSyntax
StaticMethodCallExprSyntax::withClassName(std::optional<Syntax> className)
{
   RefCountPtr<RawSyntax> classNameRaw;
   if (className.has_value()) {
      classNameRaw = className->getRaw();
   } else {
      classNameRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<StaticMethodCallExprSyntax>(classNameRaw, Cursor::ClassName);
}

StaticMethodCallExprSyntax
StaticMethodCallExprSyntax::withSeparator(std::optional<TokenSyntax> separator)
{
   RefCountPtr<RawSyntax> separatorRaw;
   if (separator.has_value()) {
      separatorRaw = separator->getRaw();
   } else {
      separatorRaw = make_missing_token(T_PAAMAYIM_NEKUDOTAYIM);
   }
   return m_data->replaceChild<StaticMethodCallExprSyntax>(separatorRaw, Cursor::Separator);
}

StaticMethodCallExprSyntax
StaticMethodCallExprSyntax::withMethodName(std::optional<MemberNameClauseSyntax> methodName)
{
   RefCountPtr<RawSyntax> methodNameRaw;
   if (methodName.has_value()) {
      methodNameRaw = methodName->getRaw();
   } else {
      methodNameRaw = RawSyntax::missing(SyntaxKind::MemberNameClause);
   }
   return m_data->replaceChild<StaticMethodCallExprSyntax>(methodNameRaw, Cursor::MethodName);
}

StaticMethodCallExprSyntax
StaticMethodCallExprSyntax::withArguments(std::optional<ArgumentListClauseSyntax> arguments)
{
   RefCountPtr<RawSyntax> argumentsRaw;
   if (arguments.has_value()) {
      argumentsRaw = arguments->getRaw();
   } else {
      argumentsRaw = RawSyntax::missing(SyntaxKind::ArgumentListClause);
   }
   return m_data->replaceChild<StaticMethodCallExprSyntax>(argumentsRaw, Cursor::Arguments);
}

///
/// DereferencableScalarExprSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType DereferencableScalarExprSyntax::CHILD_NODE_CHOICES
{
   {
      DereferencableScalarExprSyntax::ScalarValue, {
         SyntaxKind::ArrayCreateExpr, SyntaxKind::SimplifiedArrayCreateExpr,
      }
   }
};
#endif

void DereferencableScalarExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == DereferencableScalarExprSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &valueChild = raw->getChild(Cursor::ScalarValue)) {
      if (valueChild->isToken()) {
         syntax_assert_child_token(raw, ScalarValue, std::set{TokenKindType::T_CONSTANT_ENCAPSED_STRING});
      } else {
         syntax_assert_child_kind(raw, ScalarValue, CHILD_NODE_CHOICES.at(Cursor::ScalarValue));
      }
   }
#endif
}

Syntax DereferencableScalarExprSyntax::getScalarValue()
{
   return Syntax {m_root, m_data->getChild(Cursor::ScalarValue).get()};
}

DereferencableScalarExprSyntax
DereferencableScalarExprSyntax::withScalarValue(std::optional<Syntax> scalarValue)
{
   RefCountPtr<RawSyntax> scalarValueRaw;
   if (scalarValue.has_value()) {
      scalarValueRaw = scalarValue->getRaw();
   } else {
      scalarValueRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<DereferencableScalarExprSyntax>(scalarValueRaw, Cursor::ScalarValue);
}

///
/// ScalarExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType ScalarExprSyntax::CHILD_TOKEN_CHOICES
{
   {
      ScalarExprSyntax::Value, {
         TokenKindType::T_LNUMBER, TokenKindType::T_DNUMBER,
               TokenKindType::T_LINE, TokenKindType::T_FILE,
               TokenKindType::T_DIR, TokenKindType::T_TRAIT_CONST,
               TokenKindType::T_METHOD_CONST, TokenKindType::T_FUNC_CONST,
               TokenKindType::T_NS_CONST, TokenKindType::T_CLASS_CONST,
      }
   }
};
const NodeChoicesType ScalarExprSyntax::CHILD_NODE_CHOICES
{
   {
      ScalarExprSyntax::Value, {
         SyntaxKind::HeredocExpr, SyntaxKind::EncapsListStringExpr,
               SyntaxKind::DereferencableScalarExpr, SyntaxKind::ConstExpr
      }
   }
};
#endif

void ScalarExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ScalarExprSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &valueChild = raw->getChild(Cursor::Value)) {
      if (valueChild->isToken()) {
         syntax_assert_child_token(raw, Value, CHILD_TOKEN_CHOICES.at(Cursor::Value));
      } else {
         syntax_assert_child_kind(raw, Value, CHILD_NODE_CHOICES.at(Cursor::Value));
      }
   }
#endif
}

Syntax ScalarExprSyntax::getValue()
{
   return Syntax {m_root, m_data->getChild(Cursor::Value).get()};
}

ScalarExprSyntax ScalarExprSyntax::withValue(std::optional<Syntax> value)
{
   RefCountPtr<RawSyntax> valueRaw;
   if (value.has_value()) {
      valueRaw = value->getRaw();
   } else {
      valueRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<ScalarExprSyntax>(valueRaw, Cursor::Value);
}

///
/// ClassRefParentExprSyntax
///
void ClassRefParentExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefParentExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ClassRefParentExprSyntax::getParentKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ParentKeyword).get()};
}

ClassRefParentExprSyntax ClassRefParentExprSyntax::withParentKeyword(std::optional<TokenSyntax> parentKeyword)
{
   RefCountPtr<RawSyntax> rawParentKeyword;
   if (parentKeyword.has_value()) {
      rawParentKeyword = parentKeyword->getRaw();
   } else {
      rawParentKeyword = RawSyntax::missing(TokenKindType::T_CLASS_REF_PARENT,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_PARENT)));
   }
   return m_data->replaceChild<ClassRefParentExprSyntax>(rawParentKeyword, Cursor::ParentKeyword);
}

void ClassRefSelfExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefSelfExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ClassRefSelfExprSyntax::getSelfKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::SelfKeyword).get()};
}

ClassRefSelfExprSyntax ClassRefSelfExprSyntax::withSelfKeyword(std::optional<TokenSyntax> selfKeyword)
{
   RefCountPtr<RawSyntax> rawSelfKeyword;
   if (selfKeyword.has_value()) {
      rawSelfKeyword = selfKeyword->getRaw();
   } else {
      rawSelfKeyword = RawSyntax::missing(TokenKindType::T_CLASS_REF_SELF,
                                          OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_SELF)));
   }
   return m_data->replaceChild<ClassRefSelfExprSyntax>(rawSelfKeyword, Cursor::SelfKeyword);
}

void ClassRefStaticExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == ClassRefStaticExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax ClassRefStaticExprSyntax::getStaticKeyword()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::StaticKeyword).get()};
}

ClassRefStaticExprSyntax ClassRefStaticExprSyntax::withStaticKeyword(std::optional<TokenSyntax> staticKeyword)
{
   RefCountPtr<RawSyntax> rawStaticKeyword;
   if (staticKeyword.has_value()) {
      rawStaticKeyword = staticKeyword->getRaw();
   } else {
      rawStaticKeyword = RawSyntax::missing(TokenKindType::T_CLASS_REF_STATIC,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_CLASS_REF_STATIC)));
   }
   return m_data->replaceChild<ClassRefStaticExprSyntax>(rawStaticKeyword, Cursor::StaticKeyword);
}

///
/// IntegerLiteralExprSyntax
///
void IntegerLiteralExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == IntegerLiteralExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax IntegerLiteralExprSyntax::getDigits()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Digits).get()};
}

IntegerLiteralExprSyntax IntegerLiteralExprSyntax::withDigits(std::optional<TokenSyntax> digits)
{
   RefCountPtr<RawSyntax> rawDigits;
   if (digits.has_value()) {
      rawDigits = digits->getRaw();
   } else {
      rawDigits = RawSyntax::missing(TokenKindType::T_LNUMBER,
                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_LNUMBER)));
   }
   return m_data->replaceChild<IntegerLiteralExprSyntax>(rawDigits, Cursor::Digits);
}

///
/// FloatLiteralExprSyntax
///
void FloatLiteralExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == FloatLiteralExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax FloatLiteralExprSyntax::getFloatDigits()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::FloatDigits).get()};
}

FloatLiteralExprSyntax FloatLiteralExprSyntax::withFloatDigits(std::optional<TokenSyntax> digits)
{
   RefCountPtr<RawSyntax> rawDigits;
   if (digits.has_value()) {
      rawDigits = digits->getRaw();
   } else {
      rawDigits = RawSyntax::missing(TokenKindType::T_DNUMBER,
                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_DNUMBER)));
   }
   return m_data->replaceChild<FloatLiteralExprSyntax>(rawDigits, Cursor::FloatDigits);
}

///
/// StringLiteralExprSyntax
///

#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType StringLiteralExprSyntax::CHILD_TOKEN_CHOICES
{
   {
      StringLiteralExprSyntax::LeftQuote, {
         TokenKindType::T_SINGLE_QUOTE, TokenKindType::T_DOUBLE_QUOTE
      }
   },
   {
      StringLiteralExprSyntax::RightQuote, {
         TokenKindType::T_SINGLE_QUOTE, TokenKindType::T_DOUBLE_QUOTE
      }
   }
};
#endif

void StringLiteralExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == StringLiteralExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftQuote, CHILD_TOKEN_CHOICES.at(Cursor::LeftQuote));
   syntax_assert_child_token(raw, RightQuote, CHILD_TOKEN_CHOICES.at(Cursor::RightQuote));
   assert(raw->getChild(Cursor::LeftQuote)->getTokenKind() == raw->getChild(Cursor::RightQuote)->getTokenKind());
#endif
}

TokenSyntax StringLiteralExprSyntax::getLeftQuote()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftQuote).get()};
}

TokenSyntax StringLiteralExprSyntax::getText()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Text).get()};
}

TokenSyntax StringLiteralExprSyntax::getRightQuote()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightQuote).get()};
}

StringLiteralExprSyntax StringLiteralExprSyntax::withLeftQuote(std::optional<TokenSyntax> leftQuote)
{
   RefCountPtr<RawSyntax> leftQuoteRaw;
   if (leftQuote.has_value()) {
      leftQuoteRaw = leftQuote->getRaw();
   } else {
      leftQuoteRaw = RawSyntax::missing(TokenKindType::T_DOUBLE_QUOTE,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_DOUBLE_QUOTE)));
   }
   return m_data->replaceChild<StringLiteralExprSyntax>(leftQuoteRaw, Cursor::LeftQuote);
}

StringLiteralExprSyntax StringLiteralExprSyntax::withText(std::optional<TokenSyntax> text)
{
   RefCountPtr<RawSyntax> textRaw;
   if (text.has_value()) {
      textRaw = text->getRaw();
   } else {
      textRaw = RawSyntax::missing(TokenKindType::T_CONSTANT_ENCAPSED_STRING,
                                   OwnedString::makeUnowned(get_token_text(TokenKindType::T_CONSTANT_ENCAPSED_STRING)));
   }
   return m_data->replaceChild<StringLiteralExprSyntax>(textRaw, Cursor::Text);
}

StringLiteralExprSyntax StringLiteralExprSyntax::withRightQuote(std::optional<TokenSyntax> rightQuote)
{
   RefCountPtr<RawSyntax> rightQuoteRaw;
   if (rightQuote.has_value()) {
      rightQuoteRaw = rightQuote->getRaw();
   } else {
      RefCountPtr<RawSyntax> leftQuote = getRaw()->getChild(Cursor::LeftQuote);
      if (leftQuote) {
         TokenKindType leftQuoteKind = leftQuote->getTokenKind();
         rightQuoteRaw = RawSyntax::missing(leftQuoteKind,
                                            OwnedString::makeUnowned(get_token_text(leftQuoteKind)));
      } else {
         rightQuoteRaw = RawSyntax::missing(TokenKindType::T_DOUBLE_QUOTE,
                                            OwnedString::makeUnowned(get_token_text(TokenKindType::T_DOUBLE_QUOTE)));
      }
   }
   return m_data->replaceChild<StringLiteralExprSyntax>(rightQuoteRaw, Cursor::LeftQuote);
}

#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType EncapsVarOffsetSyntax::CHILD_TOKEN_CHOICES
{
   {
      EncapsVarOffsetSyntax::Offset, {
         TokenKindType::T_IDENTIFIER_STRING, TokenKindType::T_NUM_STRING,
               TokenKindType::T_VARIABLE
      }
   }
};
#endif

void EncapsVarOffsetSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsVarOffsetSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, MinusSign, std::set{TokenKindType::T_MINUS_SIGN});
   syntax_assert_child_token(raw, Offset, CHILD_TOKEN_CHOICES.at(Cursor::Offset));
#endif
}

std::optional<TokenSyntax> EncapsVarOffsetSyntax::getMinusSign()
{
   RefCountPtr<SyntaxData> minusSignData = m_data->getChild(Cursor::MinusSign);
   if (!minusSignData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, minusSignData.get()};
}

TokenSyntax EncapsVarOffsetSyntax::getOffset()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Offset).get()};
}

EncapsVarOffsetSyntax EncapsVarOffsetSyntax::withMinusSign(std::optional<TokenSyntax> minusSign)
{
   RefCountPtr<RawSyntax> minusSignRaw;
   if (minusSign.has_value()) {
      minusSignRaw = minusSign->getRaw();
   } else {
      minusSignRaw = nullptr;
   }
   return m_data->replaceChild<EncapsVarOffsetSyntax>(minusSignRaw, Cursor::MinusSign);
}

EncapsVarOffsetSyntax EncapsVarOffsetSyntax::withOffset(std::optional<TokenSyntax> offset)
{
   RefCountPtr<RawSyntax> offsetRaw;
   if (offset.has_value()) {
      offsetRaw = offset->getRaw();
   } else {
      offsetRaw = RawSyntax::missing(TokenKindType::T_IDENTIFIER_STRING,
                                     OwnedString::makeUnowned(get_token_text(TokenKindType::T_IDENTIFIER_STRING)));
   }
   return m_data->replaceChild<EncapsVarOffsetSyntax>(offsetRaw, Cursor::Offset);
}

///
/// EncapsArrayVarSyntax
///
void EncapsArrayVarSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsArrayVarSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, VarToken, std::set{TokenKindType::T_VARIABLE});
   syntax_assert_child_token(raw, LeftSquareBracket, std::set{TokenKindType::T_LEFT_SQUARE_BRACKET});
   syntax_assert_child_token(raw, RightSquareBracket, std::set{TokenKindType::T_RIGHT_SQUARE_BRACKET});
   syntax_assert_child_kind(raw, Offset, std::set{SyntaxKind::EncapsVarOffset});
#endif
}

TokenSyntax EncapsArrayVarSyntax::getVarToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::VarToken).get()};
}

TokenSyntax EncapsArrayVarSyntax::getLeftSquareBracket()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftSquareBracket).get()};
}

EncapsVarOffsetSyntax EncapsArrayVarSyntax::getOffset()
{
   return EncapsVarOffsetSyntax {m_root, m_data->getChild(Cursor::Offset).get()};
}

TokenSyntax EncapsArrayVarSyntax::getRightSquareBracket()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightSquareBracket).get()};
}

EncapsArrayVarSyntax EncapsArrayVarSyntax::withVarToken(std::optional<TokenSyntax> varToken)
{
   RefCountPtr<RawSyntax> varTokenRaw;
   if (varToken.has_value()) {
      varTokenRaw = varToken->getRaw();
   } else {
      varTokenRaw = RawSyntax::missing(TokenKindType::T_VARIABLE,
                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_VARIABLE)));
   }
   return m_data->replaceChild<EncapsArrayVarSyntax>(varTokenRaw, Cursor::VarToken);
}

EncapsArrayVarSyntax EncapsArrayVarSyntax::withLeftSquareBracket(std::optional<TokenSyntax> leftSquareBracket)
{
   RefCountPtr<RawSyntax> leftSquareBracketRaw;
   if (leftSquareBracket.has_value()) {
      leftSquareBracketRaw = leftSquareBracket->getRaw();
   } else {
      leftSquareBracketRaw = RawSyntax::missing(TokenKindType::T_LEFT_SQUARE_BRACKET,
                                                OwnedString::makeUnowned(get_token_text(TokenKindType::T_LEFT_SQUARE_BRACKET)));
   }
   return m_data->replaceChild<EncapsArrayVarSyntax>(leftSquareBracketRaw, Cursor::LeftSquareBracket);
}

EncapsArrayVarSyntax EncapsArrayVarSyntax::withOffset(std::optional<EncapsVarOffsetSyntax> offset)
{
   RefCountPtr<RawSyntax> offsetRaw;
   if (offset.has_value()) {
      offsetRaw = offset->getRaw();
   } else {
      offsetRaw = RawSyntax::missing(SyntaxKind::EncapsVarOffset);
   }
   return m_data->replaceChild<EncapsArrayVarSyntax>(offsetRaw, Cursor::Offset);
}

EncapsArrayVarSyntax EncapsArrayVarSyntax::withRightSquareBracket(std::optional<TokenSyntax> rightSquareBracket)
{
   RefCountPtr<RawSyntax> rightSquareBracketRaw;
   if (rightSquareBracket.has_value()) {
      rightSquareBracketRaw = rightSquareBracket->getRaw();
   } else {
      rightSquareBracketRaw = RawSyntax::missing(TokenKindType::T_RIGHT_SQUARE_BRACKET,
                                                 OwnedString::makeUnowned(get_token_text(TokenKindType::T_RIGHT_SQUARE_BRACKET)));
   }
   return m_data->replaceChild<EncapsArrayVarSyntax>(rightSquareBracketRaw, Cursor::RightSquareBracket);
}

///
/// EncapsObjPropSyntax
///
void EncapsObjPropSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsObjPropSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, VarToken, std::set{TokenKindType::T_VARIABLE});
   syntax_assert_child_token(raw, ObjOperatorToken, std::set{TokenKindType::T_OBJECT_OPERATOR});
   syntax_assert_child_token(raw, IdentifierToken, std::set{TokenKindType::T_IDENTIFIER_STRING});
#endif
}

TokenSyntax EncapsObjPropSyntax::getVarToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::VarToken).get()};
}

TokenSyntax EncapsObjPropSyntax::getObjOperatorToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::ObjOperatorToken).get()};
}

TokenSyntax EncapsObjPropSyntax::getIdentifierToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::IdentifierToken).get()};
}

EncapsObjPropSyntax EncapsObjPropSyntax::withVarToken(std::optional<TokenSyntax> varToken)
{
   RefCountPtr<RawSyntax> varTokenRaw;
   if (varToken.has_value()) {
      varTokenRaw = varToken->getRaw();
   } else {
      varTokenRaw = RawSyntax::missing(TokenKindType::T_VARIABLE,
                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_VARIABLE)));
   }
   return m_data->replaceChild<EncapsObjPropSyntax>(varTokenRaw, Cursor::VarToken);
}

EncapsObjPropSyntax EncapsObjPropSyntax::withObjOperatorToken(std::optional<TokenSyntax> objOperatorToken)
{
   RefCountPtr<RawSyntax> objOperatorTokenRaw;
   if (objOperatorToken.has_value()) {
      objOperatorTokenRaw = objOperatorToken->getRaw();
   } else {
      objOperatorTokenRaw = RawSyntax::missing(TokenKindType::T_OBJECT_OPERATOR,
                                               OwnedString::makeUnowned(get_token_text(TokenKindType::T_OBJECT_OPERATOR)));
   }
   return m_data->replaceChild<EncapsObjPropSyntax>(objOperatorTokenRaw, Cursor::ObjOperatorToken);
}

EncapsObjPropSyntax EncapsObjPropSyntax::withIdentifierToken(std::optional<TokenSyntax> identifierToken)
{
   RefCountPtr<RawSyntax> identifierTokenRaw;
   if (identifierToken.has_value()) {
      identifierTokenRaw = identifierToken->getRaw();
   } else {
      identifierTokenRaw = RawSyntax::missing(TokenKindType::T_IDENTIFIER_STRING,
                                              OwnedString::makeUnowned(get_token_text(TokenKindType::T_IDENTIFIER_STRING)));
   }
   return m_data->replaceChild<EncapsObjPropSyntax>(identifierTokenRaw, Cursor::IdentifierToken);
}

///
/// EncapsDollarCurlyExprSyntax
///
void EncapsDollarCurlyExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsDollarCurlyExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DollarOpenCurlyToken, std::set{TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES});
   syntax_assert_child_token(raw, CloseCurlyToken, std::set{TokenKindType::T_RIGHT_BRACE});
   syntax_assert_child_kind(raw, Expr, std::set{SyntaxKind::Expr});
#endif
}

TokenSyntax EncapsDollarCurlyExprSyntax::getDollarOpenCurlyToken()
{
   return TokenSyntax {m_root, m_data->getChild(DollarOpenCurlyToken).get()};
}

ExprSyntax EncapsDollarCurlyExprSyntax::getExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Expr).get()};
}

TokenSyntax EncapsDollarCurlyExprSyntax::getCloseCurlyToken()
{
   return TokenSyntax {m_root, m_data->getChild(CloseCurlyToken).get()};
}

EncapsDollarCurlyExprSyntax
EncapsDollarCurlyExprSyntax::withDollarOpenCurlyToken(std::optional<TokenSyntax> dollarOpenCurlyToken)
{
   RefCountPtr<RawSyntax> dollarOpenCurlyTokenRaw;
   if (dollarOpenCurlyToken.has_value()) {
      dollarOpenCurlyTokenRaw = dollarOpenCurlyToken->getRaw();
   } else {
      dollarOpenCurlyTokenRaw = make_missing_token(T_DOLLAR_OPEN_CURLY_BRACES);
   }
   return m_data->replaceChild<EncapsDollarCurlyExprSyntax>(dollarOpenCurlyTokenRaw, Cursor::DollarOpenCurlyToken);
}

EncapsDollarCurlyExprSyntax EncapsDollarCurlyExprSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> exprRaw;
   if (expr.has_value()) {
      exprRaw = expr->getRaw();
   } else {
      exprRaw = RawSyntax::missing(SyntaxKind::UnknownExpr);
   }
   return m_data->replaceChild<EncapsDollarCurlyExprSyntax>(exprRaw, Cursor::Expr);
}

EncapsDollarCurlyExprSyntax EncapsDollarCurlyExprSyntax::withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken)
{
   RefCountPtr<RawSyntax> closeCurlyTokenRaw;
   if (closeCurlyToken.has_value()) {
      closeCurlyTokenRaw = closeCurlyToken->getRaw();
   } else {
      closeCurlyTokenRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<EncapsDollarCurlyExprSyntax>(closeCurlyTokenRaw, Cursor::CloseCurlyToken);
}

///
/// EncapsDollarCurlyExprSyntax
///
void EncapsDollarCurlyVarSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsDollarCurlyVarSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DollarOpenCurlyToken, std::set{TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES});
   syntax_assert_child_token(raw, Varname, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, CloseCurlyToken, std::set{TokenKindType::T_RIGHT_BRACE});
#endif
}

TokenSyntax EncapsDollarCurlyVarSyntax::getDollarOpenCurlyToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::DollarOpenCurlyToken).get()};
}

TokenSyntax EncapsDollarCurlyVarSyntax::getVarname()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Varname).get()};
}

TokenSyntax EncapsDollarCurlyVarSyntax::getCloseCurlyToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::CloseCurlyToken).get()};
}

EncapsDollarCurlyVarSyntax
EncapsDollarCurlyVarSyntax::withDollarOpenCurlyToken(std::optional<TokenSyntax> dollarOpenCurlyToken)
{
   RefCountPtr<RawSyntax> dollarOpenCurlyTokenRaw;
   if (dollarOpenCurlyToken.has_value()) {
      dollarOpenCurlyTokenRaw = dollarOpenCurlyToken->getRaw();
   } else {
      dollarOpenCurlyTokenRaw = make_missing_token(T_DOLLAR_OPEN_CURLY_BRACES);
   }
   return m_data->replaceChild<EncapsDollarCurlyVarSyntax>(dollarOpenCurlyTokenRaw, DollarOpenCurlyToken);
}

EncapsDollarCurlyVarSyntax
EncapsDollarCurlyVarSyntax::withVarname(std::optional<ExprSyntax> varname)
{
   RefCountPtr<RawSyntax> varnameRaw;
   if (varname.has_value()) {
      varnameRaw = varname->getRaw();
   } else {
      varnameRaw = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<EncapsDollarCurlyVarSyntax>(varnameRaw, Varname);
}

EncapsDollarCurlyVarSyntax
EncapsDollarCurlyVarSyntax::withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken)
{
   RefCountPtr<RawSyntax> closeCurlyTokenRaw;
   if (closeCurlyToken.has_value()) {
      closeCurlyTokenRaw = closeCurlyToken->getRaw();
   } else {
      closeCurlyTokenRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<EncapsDollarCurlyVarSyntax>(closeCurlyTokenRaw, CloseCurlyToken);
}

///
/// EncapsDollarCurlyArraySyntax
///
void EncapsDollarCurlyArraySyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsDollarCurlyArraySyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, DollarOpenCurlyToken, std::set{TokenKindType::T_DOLLAR_OPEN_CURLY_BRACES});
   syntax_assert_child_token(raw, Varname, std::set{TokenKindType::T_IDENTIFIER_STRING});
   syntax_assert_child_token(raw, LeftSquareBracketToken, std::set{TokenKindType::T_LEFT_SQUARE_BRACKET});
   syntax_assert_child_token(raw, RightSquareBracketToken, std::set{TokenKindType::T_RIGHT_SQUARE_BRACKET});
   syntax_assert_child_token(raw, CloseCurlyToken, std::set{TokenKindType::T_RIGHT_BRACE});
#endif
}

TokenSyntax EncapsDollarCurlyArraySyntax::getDollarOpenCurlyToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::DollarOpenCurlyToken).get()};
}

TokenSyntax EncapsDollarCurlyArraySyntax::getVarname()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Varname).get()};
}

TokenSyntax EncapsDollarCurlyArraySyntax::getLeftSquareBracketToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftSquareBracketToken).get()};
}

ExprSyntax EncapsDollarCurlyArraySyntax::getIndexExpr()
{
   return ExprSyntax {m_root, m_data->getChild(Cursor::IndexExpr).get()};
}

TokenSyntax EncapsDollarCurlyArraySyntax::getRightSquareBracketToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightSquareBracketToken).get()};
}

TokenSyntax EncapsDollarCurlyArraySyntax::getCloseCurlyToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::CloseCurlyToken).get()};
}

EncapsDollarCurlyArraySyntax
EncapsDollarCurlyArraySyntax::withDollarOpenCurlyToken(std::optional<TokenSyntax> dollarOpenCurlyToken)
{
   RefCountPtr<RawSyntax> dollarOpenCurlyTokenRaw;
   if (dollarOpenCurlyToken.has_value()) {
      dollarOpenCurlyTokenRaw = dollarOpenCurlyToken->getRaw();
   } else {
      dollarOpenCurlyTokenRaw = make_missing_token(T_DOLLAR_OPEN_CURLY_BRACES);
   }
   return m_data->replaceChild<EncapsDollarCurlyArraySyntax>(dollarOpenCurlyTokenRaw, DollarOpenCurlyToken);
}

EncapsDollarCurlyArraySyntax
EncapsDollarCurlyArraySyntax::withVarname(std::optional<TokenSyntax> varname)
{
   RefCountPtr<RawSyntax> varnameRaw;
   if (varname.has_value()) {
      varnameRaw = varname->getRaw();
   } else {
      varnameRaw = make_missing_token(T_IDENTIFIER_STRING);
   }
   return m_data->replaceChild<EncapsDollarCurlyArraySyntax>(varnameRaw, Varname);
}

EncapsDollarCurlyArraySyntax
EncapsDollarCurlyArraySyntax::withLeftSquareBracketToken(std::optional<TokenSyntax> leftSquareBracketToken)
{
   RefCountPtr<RawSyntax> leftSquareBracketTokenRaw;
   if (leftSquareBracketToken.has_value()) {
      leftSquareBracketTokenRaw = leftSquareBracketToken->getRaw();
   } else {
      leftSquareBracketTokenRaw = make_missing_token(T_LEFT_SQUARE_BRACKET);
   }
   return m_data->replaceChild<EncapsDollarCurlyArraySyntax>(leftSquareBracketTokenRaw, LeftSquareBracketToken);
}

EncapsDollarCurlyArraySyntax
EncapsDollarCurlyArraySyntax::withIndexExpr(std::optional<ExprSyntax> indexExpr)
{
   RefCountPtr<RawSyntax> indexExprRaw;
   if (indexExpr.has_value()) {
      indexExprRaw = indexExpr->getRaw();
   } else {
      indexExprRaw = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<EncapsDollarCurlyArraySyntax>(indexExprRaw, IndexExpr);
}

EncapsDollarCurlyArraySyntax
EncapsDollarCurlyArraySyntax::withRightSquareBracketToken(std::optional<TokenSyntax> rightSquareBracketToken)
{
   RefCountPtr<RawSyntax> rightSquareBracketTokenRaw;
   if (rightSquareBracketToken.has_value()) {
      rightSquareBracketTokenRaw = rightSquareBracketToken->getRaw();
   } else {
      rightSquareBracketTokenRaw = make_missing_token(T_RIGHT_SQUARE_BRACKET);
   }
   return m_data->replaceChild<EncapsDollarCurlyArraySyntax>(rightSquareBracketTokenRaw, RightSquareBracketToken);
}

EncapsDollarCurlyArraySyntax
EncapsDollarCurlyArraySyntax::withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken)
{
   RefCountPtr<RawSyntax> closeCurlyTokenRaw;
   if (closeCurlyToken.has_value()) {
      closeCurlyTokenRaw = closeCurlyToken->getRaw();
   } else {
      closeCurlyTokenRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<EncapsDollarCurlyArraySyntax>(closeCurlyTokenRaw, CloseCurlyToken);
}

///
/// EncapsCurlyVarSyntax
///
void EncapsCurlyVarSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsCurlyVarSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, CurlyOpen, std::set{TokenKindType::T_CURLY_OPEN});
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
   syntax_assert_child_token(raw, CloseCurlyToken, std::set{TokenKindType::T_RIGHT_BRACE});
#endif
}

TokenSyntax EncapsCurlyVarSyntax::getCurlyOpen()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::CurlyOpen).get()};
}

TokenSyntax EncapsCurlyVarSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

TokenSyntax EncapsCurlyVarSyntax::getCloseCurlyToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::CloseCurlyToken).get()};
}

EncapsCurlyVarSyntax
EncapsCurlyVarSyntax::withCurlyOpen(std::optional<TokenSyntax> curlyOpen)
{
   RefCountPtr<RawSyntax> curlyOpenRaw;
   if (curlyOpen.has_value()) {
      curlyOpenRaw = curlyOpen->getRaw();
   } else {
      curlyOpenRaw = make_missing_token(T_CURLY_OPEN);
   }
   return m_data->replaceChild<EncapsCurlyVarSyntax>(curlyOpenRaw, CurlyOpen);
}

EncapsCurlyVarSyntax
EncapsCurlyVarSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> variableRaw;
   if (variable.has_value()) {
      variableRaw = variable->getRaw();
   } else {
      variableRaw = make_missing_token(T_VARIABLE);
   }
   return m_data->replaceChild<EncapsCurlyVarSyntax>(variableRaw, Variable);
}

EncapsCurlyVarSyntax
EncapsCurlyVarSyntax::withCloseCurlyToken(std::optional<TokenSyntax> closeCurlyToken)
{
   RefCountPtr<RawSyntax> closeCurlyTokenRaw;
   if (closeCurlyToken.has_value()) {
      closeCurlyTokenRaw = closeCurlyToken->getRaw();
   } else {
      closeCurlyTokenRaw = make_missing_token(T_RIGHT_BRACE);
   }
   return m_data->replaceChild<EncapsCurlyVarSyntax>(closeCurlyTokenRaw, CloseCurlyToken);
}

///
/// EncapsVarSyntax
///

#ifdef POLAR_DEBUG_BUILD
const NodeChoicesType EncapsVarSyntax::CHILD_NODE_CHOICES
{
   {
      EncapsVarSyntax::Var, {
         SyntaxKind::EncapsArrayVar, SyntaxKind::EncapsObjProp,
               SyntaxKind::EncapsDollarCurlyExpr, SyntaxKind::EncapsDollarCurlyVar,
               SyntaxKind::EncapsDollarCurlyArray, SyntaxKind::EncapsCurlyVar

      }
   }
};
#endif

void EncapsVarSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsVarSyntax::CHILDREN_COUNT);
   if (const RefCountPtr<RawSyntax> &varChild = raw->getChild(Cursor::Var)) {
      if (varChild->isToken()) {
         syntax_assert_child_token(raw, Var, std::set{TokenKindType::T_IDENTIFIER_STRING});
      } else {
         syntax_assert_child_kind(raw, Var, CHILD_NODE_CHOICES.at(Cursor::Var));
      }
   }
#endif
}

Syntax EncapsVarSyntax::getVar()
{
   return Syntax {m_root, m_data->getChild(Cursor::Var).get()};
}

EncapsVarSyntax EncapsVarSyntax::withVar(std::optional<Syntax> var)
{
   RefCountPtr<RawSyntax> varRaw;
   if (var.has_value()) {
      varRaw = var->getRaw();
   } else {
      varRaw = RawSyntax::missing(SyntaxKind::Unknown);
   }
   return m_data->replaceChild<EncapsVarSyntax>(varRaw, Cursor::Var);
}

///
/// EncapsListItemSyntax
///
void EncapsListItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsListItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_kind(raw, EncapsVar, std::set{SyntaxKind::EncapsVar});
   syntax_assert_child_token(raw, StrLiteral, std::set{TokenKindType::T_ENCAPSED_AND_WHITESPACE});
   assert(raw->getChild(Cursor::EncapsVar) || raw->getChild(Cursor::StrLiteral));
#endif
}

std::optional<TokenSyntax> EncapsListItemSyntax::getStrLiteral()
{
   RefCountPtr<SyntaxData> strLiteralData = m_data->getChild(Cursor::StrLiteral);
   if (!strLiteralData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, strLiteralData.get()};
}

std::optional<EncapsVarSyntax> EncapsListItemSyntax::getEncapsVar()
{
   RefCountPtr<SyntaxData> encapsVarData = m_data->getChild(Cursor::EncapsVar);
   if (!encapsVarData) {
      return std::nullopt;
   }
   return EncapsVarSyntax {m_root, encapsVarData.get()};
}

EncapsListItemSyntax EncapsListItemSyntax::withEncapsListItemSyntax(std::optional<TokenSyntax> strLiteral)
{
   RefCountPtr<RawSyntax> strLiteralRaw;
   if (strLiteral.has_value()) {
      strLiteralRaw = strLiteral->getRaw();
   } else {
      strLiteralRaw = make_missing_token(T_ENCAPSED_AND_WHITESPACE);
   }
   return m_data->replaceChild<EncapsListItemSyntax>(strLiteralRaw, Cursor::StrLiteral);
}

EncapsListItemSyntax EncapsListItemSyntax::withEncapsVar(std::optional<EncapsVarSyntax> encapsVar)
{
   RefCountPtr<RawSyntax> encapsVarRaw;
   if (encapsVar.has_value()) {
      encapsVarRaw = encapsVar->getRaw();
   } else {
      encapsVarRaw = RawSyntax::missing(SyntaxKind::EncapsVar);
   }
   return m_data->replaceChild<EncapsListItemSyntax>(encapsVarRaw, Cursor::EncapsVar);
}

///
/// HeredocExprSyntax
///
void HeredocExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == HeredocExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, StartHeredocToken, std::set{TokenKindType::T_START_HEREDOC});
   syntax_assert_child_token(raw, EndHeredocToken, std::set{TokenKindType::T_END_HEREDOC});
   if (const RefCountPtr<RawSyntax> &textClauseChild = raw->getChild(Cursor::TextClause)) {
      if (textClauseChild->isToken()) {
         syntax_assert_child_token(raw, TextClause, std::set{TokenKindType::T_ENCAPSED_AND_WHITESPACE});
      } else {
         syntax_assert_child_kind(raw, TextClause, std::set{SyntaxKind::EncapsList});
      }
   }
#endif
}

TokenSyntax HeredocExprSyntax::getStartHeredocToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::StartHeredocToken).get()};
}

std::optional<Syntax> HeredocExprSyntax::getTextClause()
{
   RefCountPtr<SyntaxData> textClauseData = m_data->getChild(Cursor::TextClause);
   if (!textClauseData) {
      return std::nullopt;
   }
   return Syntax {m_root, textClauseData.get()};
}

TokenSyntax HeredocExprSyntax::getEndHeredocToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::EndHeredocToken).get()};
}

HeredocExprSyntax HeredocExprSyntax::withStartHeredocToken(std::optional<TokenSyntax> startHeredocToken)
{
   RefCountPtr<RawSyntax> startHeredocTokenRaw;
   if (startHeredocToken.has_value()) {
      startHeredocTokenRaw = startHeredocToken->getRaw();
   } else {
      startHeredocTokenRaw = make_missing_token(T_START_HEREDOC);
   }
   return m_data->replaceChild<HeredocExprSyntax>(startHeredocTokenRaw, Cursor::StartHeredocToken);
}

HeredocExprSyntax HeredocExprSyntax::withTextClause(std::optional<Syntax> textClause)
{
   RefCountPtr<RawSyntax> textClauseRaw;
   if (textClause.has_value()) {
      textClauseRaw = textClause->getRaw();
   } else {
      textClauseRaw = nullptr;
   }
   return m_data->replaceChild<HeredocExprSyntax>(textClauseRaw, Cursor::TextClause);
}

HeredocExprSyntax HeredocExprSyntax::withEndHeredocToken(std::optional<TokenSyntax> endHeredocToken)
{
   RefCountPtr<RawSyntax> endHeredocTokenRaw;
   if (endHeredocToken.has_value()) {
      endHeredocTokenRaw = endHeredocToken->getRaw();
   } else {
      endHeredocTokenRaw = make_missing_token(T_END_HEREDOC);
   }
   return m_data->replaceChild<HeredocExprSyntax>(endHeredocTokenRaw, Cursor::EndHeredocToken);
}

///
/// EncapsListStringExprSyntax
///
void EncapsListStringExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == EncapsListStringExprSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, LeftQuoteToken, std::set{TokenKindType::T_DOUBLE_QUOTE});
   syntax_assert_child_token(raw, RightQuoteToken, std::set{TokenKindType::T_DOUBLE_QUOTE});
   syntax_assert_child_kind(raw, EncapsList, std::set{SyntaxKind::EncapsList});
#endif
}

TokenSyntax EncapsListStringExprSyntax::getLeftQuoteToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::LeftQuoteToken).get()};
}

EncapsItemListSyntax EncapsListStringExprSyntax::getEncapsList()
{
   return EncapsItemListSyntax {m_root, m_data->getChild(Cursor::EncapsList).get()};
}

TokenSyntax EncapsListStringExprSyntax::getRightQuoteToken()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::RightQuoteToken).get()};
}

EncapsListStringExprSyntax
EncapsListStringExprSyntax::withLeftQuoteToken(std::optional<TokenSyntax> leftQuoteToken)
{
   RefCountPtr<RawSyntax> leftQuoteTokenRaw;
   if (leftQuoteToken.has_value()) {
      leftQuoteTokenRaw = leftQuoteToken->getRaw();
   } else {
      leftQuoteTokenRaw = make_missing_token(T_DOUBLE_QUOTE);
   }
   return m_data->replaceChild<EncapsListStringExprSyntax>(leftQuoteTokenRaw, Cursor::LeftQuoteToken);
}

EncapsListStringExprSyntax
EncapsListStringExprSyntax::withEncapsList(std::optional<EncapsItemListSyntax> encapsList)
{
   RefCountPtr<RawSyntax> encapsListRaw;
   if (encapsList.has_value()) {
      encapsListRaw = encapsList->getRaw();
   } else {
      encapsListRaw = RawSyntax::missing(SyntaxKind::EncapsList);
   }
   return m_data->replaceChild<EncapsListStringExprSyntax>(encapsListRaw, Cursor::EncapsList);
}

EncapsListStringExprSyntax
EncapsListStringExprSyntax::withRightQuoteToken(std::optional<TokenSyntax> rightQuoteToken)
{
   RefCountPtr<RawSyntax> rightQuoteTokenRaw;
   if (rightQuoteToken.has_value()) {
      rightQuoteTokenRaw = rightQuoteToken->getRaw();
   } else {
      rightQuoteTokenRaw = make_missing_token(T_DOUBLE_QUOTE);
   }
   return m_data->replaceChild<EncapsListStringExprSyntax>(rightQuoteTokenRaw, Cursor::RightQuoteToken);
}

///
/// BooleanLiteralExprSyntax
///
#ifdef POLAR_DEBUG_BUILD
const TokenChoicesType BooleanLiteralExprSyntax::CHILD_TOKEN_CHOICES
{
   {
      BooleanLiteralExprSyntax::Boolean, {
         TokenKindType::T_FALSE,
               TokenKindType::T_TRUE
      }
   }
};
#endif

void BooleanLiteralExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   ///
   /// check Boolean token choice
   ///
   syntax_assert_child_token(raw, Boolean, CHILD_TOKEN_CHOICES.at(Boolean));
   assert(raw->getLayout().size() == BooleanLiteralExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax BooleanLiteralExprSyntax::getBooleanValue()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::Boolean).get()};
}

BooleanLiteralExprSyntax BooleanLiteralExprSyntax::withBooleanValue(std::optional<TokenSyntax> booleanValue)
{
   RefCountPtr<RawSyntax> rawBooleanValue;
   if (booleanValue.has_value()) {
      rawBooleanValue = booleanValue->getRaw();
   } else {
      rawBooleanValue = RawSyntax::missing(TokenKindType::T_TRUE,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_TRUE)));
   }
   return m_data->replaceChild<BooleanLiteralExprSyntax>(rawBooleanValue, Cursor::Boolean);
}

void TernaryExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == TernaryExprSyntax::CHILDREN_COUNT);
#endif
}

ExprSyntax TernaryExprSyntax::getConditionExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::ConditionExpr).get()};
}

TokenSyntax TernaryExprSyntax::getQuestionMark()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::QuestionMark).get()};
}

ExprSyntax TernaryExprSyntax::getFirstChoice()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::FirstChoice).get()};
}

TokenSyntax TernaryExprSyntax::getColonMark()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::ColonMark).get()};
}

ExprSyntax TernaryExprSyntax::getSecondChoice()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::SecondChoice).get()};
}

TernaryExprSyntax TernaryExprSyntax::withConditionExpr(std::optional<ExprSyntax> conditionExpr)
{
   RefCountPtr<RawSyntax> rawConditionExpr;
   if (conditionExpr.has_value()) {
      rawConditionExpr = conditionExpr->getRaw();
   } else {
      rawConditionExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawConditionExpr, Cursor::ConditionExpr);
}

TernaryExprSyntax TernaryExprSyntax::withQuestionMark(std::optional<TokenSyntax> questionMark)
{
   RefCountPtr<RawSyntax> rawQuestionMark;
   if (questionMark.has_value()) {
      rawQuestionMark = questionMark->getRaw();
   } else {
      rawQuestionMark = RawSyntax::missing(TokenKindType::T_QUESTION_MARK,
                                           OwnedString::makeUnowned(get_token_text(TokenKindType::T_QUESTION_MARK)));
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawQuestionMark, Cursor::QuestionMark);
}

TernaryExprSyntax TernaryExprSyntax::withFirstChoice(std::optional<ExprSyntax> firstChoice)
{
   RefCountPtr<RawSyntax> rawFirstChoice;
   if (firstChoice.has_value()) {
      rawFirstChoice = firstChoice->getRaw();
   } else {
      rawFirstChoice = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawFirstChoice, Cursor::FirstChoice);
}

TernaryExprSyntax TernaryExprSyntax::withColonMark(std::optional<TokenSyntax> colonMark)
{
   RefCountPtr<RawSyntax> rawColonMark;
   if (colonMark.has_value()) {
      rawColonMark = colonMark->getRaw();
   } else {
      rawColonMark = RawSyntax::missing(TokenKindType::T_COLON,
                                        OwnedString::makeUnowned(get_token_text(TokenKindType::T_COLON)));
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawColonMark, Cursor::ColonMark);
}

TernaryExprSyntax TernaryExprSyntax::withSecondChoice(std::optional<ExprSyntax> secondChoice)
{
   RefCountPtr<RawSyntax> rawSecondChoice;
   if (secondChoice.has_value()) {
      rawSecondChoice = secondChoice->getRaw();
   } else {
      rawSecondChoice = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<TernaryExprSyntax>(rawSecondChoice, Cursor::SecondChoice);
}

void AssignmentExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == AssignmentExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax AssignmentExprSyntax::getAssignToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::AssignToken).get()};
}

AssignmentExprSyntax AssignmentExprSyntax::withAssignToken(std::optional<TokenSyntax> assignToken)
{
   RefCountPtr<RawSyntax> rawAssignToken;
   if (assignToken.has_value()) {
      rawAssignToken = assignToken->getRaw();
   } else {
      rawAssignToken = RawSyntax::missing(TokenKindType::T_EQUAL,
                                          OwnedString::makeUnowned(get_token_text(TokenKindType::T_EQUAL)));
   }
   return m_data->replaceChild<AssignmentExprSyntax>(rawAssignToken, Cursor::AssignToken);
}

void SequenceExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == SequenceExprSyntax::CHILDREN_COUNT);
#endif
}

ExprListSyntax SequenceExprSyntax::getElements()
{
   return ExprListSyntax{m_root, m_data->getChild(Cursor::Elements).get()};
}

SequenceExprSyntax SequenceExprSyntax::withElements(std::optional<ExprListSyntax> elements)
{
   RefCountPtr<RawSyntax> rawElements;
   if (elements.has_value()) {
      rawElements = elements->getRaw();
   } else {
      rawElements = RawSyntax::missing(SyntaxKind::ExprList);
   }
   return m_data->replaceChild<SequenceExprSyntax>(rawElements, Cursor::Elements);
}

SequenceExprSyntax SequenceExprSyntax::addElement(ExprSyntax expr)
{
   RefCountPtr<RawSyntax> elements = getRaw()->getChild(Cursor::Elements);
   if (elements) {
      elements = elements->append(expr.getRaw());
   } else {
      elements = RawSyntax::make(SyntaxKind::ExprList, {expr.getRaw()}, SourcePresence::Present);
   }
   return m_data->replaceChild<SequenceExprSyntax>(elements, Cursor::Elements);
}

void PrefixOperatorExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == PrefixOperatorExprSyntax::CHILDREN_COUNT);
#endif
}

std::optional<TokenSyntax> PrefixOperatorExprSyntax::getOperatorToken()
{
   RefCountPtr<SyntaxData> operatorTokenData = m_data->getChild(Cursor::OperatorToken);
   if (!operatorTokenData) {
      return std::nullopt;
   }
   return TokenSyntax{m_root, operatorTokenData.get()};
}

ExprSyntax PrefixOperatorExprSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

PrefixOperatorExprSyntax PrefixOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> rawOperatorToken;
   if (operatorToken.has_value()) {
      rawOperatorToken = operatorToken->getRaw();
   } else {
      rawOperatorToken = nullptr;
   }
   return m_data->replaceChild<PrefixOperatorExprSyntax>(rawOperatorToken, Cursor::OperatorToken);
}

PrefixOperatorExprSyntax PrefixOperatorExprSyntax::withExpr(std::optional<TokenSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<PrefixOperatorExprSyntax>(rawExpr, Cursor::Expr);
}

void PostfixOperatorExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == PostfixOperatorExprSyntax::CHILDREN_COUNT);
#endif
}

ExprSyntax PostfixOperatorExprSyntax::getExpr()
{
   return ExprSyntax{m_root, m_data->getChild(Cursor::Expr).get()};
}

TokenSyntax PostfixOperatorExprSyntax::getOperatorToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::OperatorToken).get()};
}

PostfixOperatorExprSyntax PostfixOperatorExprSyntax::withExpr(std::optional<ExprSyntax> expr)
{
   RefCountPtr<RawSyntax> rawExpr;
   if (expr.has_value()) {
      rawExpr = expr->getRaw();
   } else {
      rawExpr = RawSyntax::missing(SyntaxKind::Expr);
   }
   return m_data->replaceChild<PostfixOperatorExprSyntax>(rawExpr, Cursor::Expr);
}

PostfixOperatorExprSyntax PostfixOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> rawOperatorToken;
   if (operatorToken.has_value()) {
      rawOperatorToken = operatorToken->getRaw();
   } else {
      rawOperatorToken = RawSyntax::missing(TokenKindType::T_POSTFIX_OPERATOR,
                                            OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<PostfixOperatorExprSyntax>(rawOperatorToken, Cursor::OperatorToken);
}

void BinaryOperatorExprSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().size() == BinaryOperatorExprSyntax::CHILDREN_COUNT);
#endif
}

TokenSyntax BinaryOperatorExprSyntax::getOperatorToken()
{
   return TokenSyntax{m_root, m_data->getChild(Cursor::OperatorToken).get()};
}

BinaryOperatorExprSyntax BinaryOperatorExprSyntax::withOperatorToken(std::optional<TokenSyntax> operatorToken)
{
   RefCountPtr<RawSyntax> rawOperatorToken;
   if (operatorToken.has_value()) {
      rawOperatorToken = operatorToken->getRaw();
   } else {
      rawOperatorToken = RawSyntax::missing(TokenKindType::T_BINARY_OPERATOR,
                                            OwnedString::makeUnowned(""));
   }
   return m_data->replaceChild<BinaryOperatorExprSyntax>(rawOperatorToken, TokenKindType::T_BINARY_OPERATOR);
}

///
/// LexicalVarItemSyntax
///
void LexicalVarItemSyntax::validate()
{
#ifdef POLAR_DEBUG_BUILD
   RefCountPtr<RawSyntax> raw = m_data->getRaw();
   if (isMissing()) {
      return;
   }
   assert(raw->getLayout().getSize() == LexicalVarItemSyntax::CHILDREN_COUNT);
   syntax_assert_child_token(raw, ReferenceToken, std::set{TokenKindType::T_AMPERSAND});
   syntax_assert_child_token(raw, Variable, std::set{TokenKindType::T_VARIABLE});
#endif
}

std::optional<TokenSyntax> LexicalVarItemSyntax::getReferenceToken()
{
   RefCountPtr<SyntaxData> refData = m_data->getChild(Cursor::ReferenceToken);
   if (!refData) {
      return std::nullopt;
   }
   return TokenSyntax {m_root, refData.get()};
}

TokenSyntax LexicalVarItemSyntax::getVariable()
{
   return TokenSyntax {m_root, m_data->getChild(Cursor::Variable).get()};
}

LexicalVarItemSyntax LexicalVarItemSyntax::withReferenceToken(std::optional<TokenSyntax> referenceToken)
{
   RefCountPtr<RawSyntax> referenceTokenRaw;
   if (referenceToken.has_value()) {
      referenceTokenRaw = referenceToken->getRaw();
   } else {
      referenceTokenRaw = nullptr;
   }
   return m_data->replaceChild<LexicalVarItemSyntax>(referenceTokenRaw, Cursor::ReferenceToken);
}

LexicalVarItemSyntax LexicalVarItemSyntax::withVariable(std::optional<TokenSyntax> variable)
{
   RefCountPtr<RawSyntax> variableRaw;
   if (variable.has_value()) {
      variableRaw = variable->getRaw();
   } else {
      variableRaw = RawSyntax::missing(TokenKindType::T_VARIABLE,
                                       OwnedString::makeUnowned(get_token_text(TokenKindType::T_VARIABLE)));
   }
   return m_data->replaceChild<LexicalVarItemSyntax>(variableRaw, Cursor::Variable);
}

} // polar::syntax
