// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/09/05.

#include "polarphp/syntax/SyntaxNodeFactories.h"
#include "polarphp/syntax/SyntaxKindEnumDefs.h"
#include "polarphp/syntax/AbstractFactory.h"

namespace polar::syntax {

Syntax AbstractFactory::makeBlankCollectionSyntax(SyntaxKind collectionKind)
{
   switch (collectionKind) {
   case SyntaxKind::NameList:
      return DeclSyntaxNodeFactory::makeBlankNameList();
   case SyntaxKind::ParameterList:
      return DeclSyntaxNodeFactory::makeBlankParameterList();
   case SyntaxKind::ClassModifierList:
      return DeclSyntaxNodeFactory::makeBlankClassModififerList();
   case SyntaxKind::MemberDeclList:
      return DeclSyntaxNodeFactory::makeBlankMemberDeclList();
   case SyntaxKind::MemberModifierList:
      return DeclSyntaxNodeFactory::makeBlankMemberModifierList();
   case SyntaxKind::ClassPropertyList:
      return DeclSyntaxNodeFactory::makeBlankClassPropertyList();
   case SyntaxKind::ClassConstList:
      return DeclSyntaxNodeFactory::makeBlankClassConstList();
   case SyntaxKind::ClassTraitAdaptationList:
      return DeclSyntaxNodeFactory::makeBlankClassTraitAdaptationList();

   case SyntaxKind::ExprList:
      return ExprSyntaxNodeFactory::makeBlankExprList();
   case SyntaxKind::LexicalVarList:
      return ExprSyntaxNodeFactory::makeBlankLexicalVarList();
   case SyntaxKind::ArrayPairList:
      return ExprSyntaxNodeFactory::makeBlankArrayPairList();
   case SyntaxKind::ListPairItemList:
      return ExprSyntaxNodeFactory::makeBlankListPairItemList();
   case SyntaxKind::EncapsList:
      return ExprSyntaxNodeFactory::makeBlankEncapsItemList();
   case SyntaxKind::ArgumentList:
      return ExprSyntaxNodeFactory::makeBlankArgumentList();
   case SyntaxKind::IssetVariablesList:
      return ExprSyntaxNodeFactory::makeBlankIssetVariablesList();

   case SyntaxKind::ConditionElementList:
      return StmtSyntaxNodeFactory::makeBlankConditionElementList();
   case SyntaxKind::SwitchCaseList:
      return StmtSyntaxNodeFactory::makeBlankSwitchCaseList();
   case SyntaxKind::ElseIfList:
      return StmtSyntaxNodeFactory::makeBlankElseIfList();
   case SyntaxKind::InnerStmtList:
      return StmtSyntaxNodeFactory::makeBlankInnerStmtList();
   case SyntaxKind::TopStmtList:
      return StmtSyntaxNodeFactory::makeBlankTopStmtList();
   case SyntaxKind::CatchList:
      return StmtSyntaxNodeFactory::makeBlankCatchList();
   case SyntaxKind::CatchArgTypeHintList:
      return StmtSyntaxNodeFactory::makeBlankCatchArgTypeHintList();
   case SyntaxKind::UnsetVariableList:
      return StmtSyntaxNodeFactory::makeBlankUnsetVariableList();
   case SyntaxKind::GlobalVariableList:
      return StmtSyntaxNodeFactory::makeBlankGlobalVariableList();
   case SyntaxKind::StaticVariableList:
      return StmtSyntaxNodeFactory::makeBlankStaticVariableList();
   case SyntaxKind::NamespaceUseDeclarationList:
      return StmtSyntaxNodeFactory::makeBlankNamespaceUseDeclarationList();
   case SyntaxKind::NamespaceInlineUseDeclarationList:
      return StmtSyntaxNodeFactory::makeBlankNamespaceInlineUseDeclarationList();
   case SyntaxKind::NamespaceUnprefixedUseDeclarationList:
      return StmtSyntaxNodeFactory::makeBlankNamespaceUnprefixedUseDeclarationList();
   case SyntaxKind::ConstDeclareList:
      return StmtSyntaxNodeFactory::makeBlankConstDeclareList();
   default:
      break;
   }
   polar_unreachable("not collection syntax kind.");
}

} // polar::syntax
