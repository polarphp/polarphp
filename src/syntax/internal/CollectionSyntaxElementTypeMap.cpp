// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/09/04.

#include <unordered_map>

#include "polarphp/syntax/internal/CollectionSyntaxNodeExtraFuncs.h"
#include "polarphp/syntax/SyntaxKindEnumDefs.h"

namespace polar::syntax::internal {
namespace {

using CollectionElementTypeChoicesMap = const std::unordered_map<SyntaxKind, std::set<SyntaxKind>>;
static const std::unordered_map<SyntaxKind, std::set<SyntaxKind>> scg_collectionElementTypeChoicesMap{
    // decl syntax colloection
    { SyntaxKind::NameList, { SyntaxKind::NameListItem }},
    { SyntaxKind::NamespaceName, { SyntaxKind::NamespaceName }},
    { SyntaxKind::ParameterList, { SyntaxKind::ParameterListItem }},
    { SyntaxKind::ClassModifierList, { SyntaxKind::ClassModifier }},
    { SyntaxKind::MemberDeclList, { SyntaxKind::MemberDeclListItem }},
    { SyntaxKind::MemberModifierList, { SyntaxKind::MemberModifier }},
    { SyntaxKind::ClassPropertyList, { SyntaxKind::ClassPropertyClause }},
    { SyntaxKind::ClassConstList, { SyntaxKind::ClassConstClause }},
    { SyntaxKind::ClassTraitAdaptationList, { SyntaxKind::ClassTraitAdaptation }},

    // expr syntax collection
    { SyntaxKind::ExprList, { SyntaxKind::ExprListItem }},
    { SyntaxKind::LexicalVarList, { SyntaxKind::LexicalVariable }},
    { SyntaxKind::ArrayPairList, { SyntaxKind::ArrayPairListItem }},
    { SyntaxKind::EncapsList, { SyntaxKind::EncapsListItem }},
    { SyntaxKind::ArgumentList, { SyntaxKind::ArgumentListItem }},
    { SyntaxKind::IssetVariablesList, { SyntaxKind::IssetVariableListItem }},

    // stmt syntax collection
    { SyntaxKind::ConditionElementList, { SyntaxKind::ConditionElement }},
    { SyntaxKind::SwitchCaseList, { SyntaxKind::SwitchCase }},
    { SyntaxKind::ElseIfList, { SyntaxKind::ElseIfClause }},
    { SyntaxKind::InnerStmtList, { SyntaxKind::InnerStmt }},
    { SyntaxKind::TopStmtList, { SyntaxKind::TopStmt }},
    { SyntaxKind::CatchList, { SyntaxKind::CatchListItemClause }},
    { SyntaxKind::CatchArgTypeHintList, { SyntaxKind::CatchArgTypeHintItem }},
    { SyntaxKind::UnsetVariableList, { SyntaxKind::UnsetVariableListItem }},
    { SyntaxKind::GlobalVariableList, { SyntaxKind::GlobalVariableListItem }},
    { SyntaxKind::StaticVariableList, { SyntaxKind::StaticVariableListItem }},
    { SyntaxKind::NamespaceUseDeclarationList, { SyntaxKind::NamespaceUseDeclarationListItem }},
    { SyntaxKind::NamespaceInlineUseDeclarationList, { SyntaxKind::NamespaceInlineUseDeclarationListItem }},
    { SyntaxKind::NamespaceUnprefixedUseDeclarationList, { SyntaxKind::NamespaceUnprefixedUseDeclarationListItem }},
    { SyntaxKind::ConstDeclareList, { SyntaxKind::ConstListItem }},
};
} // anonymous namespace

std::set<SyntaxKind> retrive_collection_syntax_element_type_choices(SyntaxKind kind)
{
    CollectionElementTypeChoicesMap::const_iterator iter = scg_collectionElementTypeChoicesMap.find(kind);
    assert(iter != scg_collectionElementTypeChoicesMap.cend() && "unknown collection syntax kind");
    return iter->second;
}

} // polar::syntax::internal
