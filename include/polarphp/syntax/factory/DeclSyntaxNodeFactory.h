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

#ifndef POLARPHP_SYNTAX_FACTORY_DECL_SYNTAX_NODE_FACTORY_H
#define POLARPHP_SYNTAX_FACTORY_DECL_SYNTAX_NODE_FACTORY_H

#include "polarphp/syntax/AbstractFactory.h"
#include "polarphp/syntax/syntaxnode/DeclSyntaxNodesFwd.h"

namespace polar::syntax {

class DeclSyntaxNodeFactory final : public AbstractFactory
{
public:
   ///
   /// make collection nodes
   ///
   static Syntax makeBlankCollection(SyntaxKind kind);
   static NameListSyntax makeNameList(
         const std::vector<NameSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static NamespacePartListSyntax makeNamespacePartList(
         const std::vector<NamespacePartSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ParameterListSyntax makeParameterList(
         const std::vector<ParameterSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassModififerListSyntax makeClassModififerList(
         const std::vector<ClassModifierSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberDeclListSyntax makeMemberDeclList(
         const std::vector<MemberDeclListItemSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static MemberModifierListSyntax makeMemberModifierList(
         const std::vector<MemberModifierSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassPropertyListSyntax makeClassPropertyList(
         const std::vector<ClassPropertyClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassConstListSyntax makeClassConstList(
         const std::vector<ClassConstClauseSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);
   static ClassTraitAdaptationListSyntax makeClassTraitAdaptationList(
         const std::vector<ClassTraitAdaptationSyntax> &elements, RefCountPtr<SyntaxArena> arena = nullptr);

   ///
   /// make normal nodes
   ///

};

} // polar::syntax

#endif // POLARPHP_SYNTAX_FACTORY_DECL_SYNTAX_NODE_FACTORY_H
