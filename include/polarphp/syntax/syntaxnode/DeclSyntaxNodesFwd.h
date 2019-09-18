// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/08/01.

#ifndef POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_FWD_H
#define POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_FWD_H

#include "polarphp/syntax/SyntaxCollection.h"
#include "polarphp/syntax/SyntaxKind.h"

namespace polar::syntax {

class ReservedNonModifierSyntax;
class SemiReservedSytnax;
class IdentifierSyntax;
class NamespaceNameSyntax;
class NameSyntax;
class NameListItemSyntax;
class TypeClauseSyntax;
class TypeExprClauseSyntax;
class ReturnTypeClauseSyntax;
class InitializerClauseSyntax;
class ParameterSyntax;
class ParameterListItemSyntax;
class ParameterClauseSyntax;
class FunctionDefinitionSyntax;
class ClassModifierSyntax;
class ExtendsFromClauseSyntax;
class ImplementsClauseSyntax;
class InterfaceExtendsClauseSyntax;
class ClassPropertyClauseSyntax;
class ClassConstClauseSyntax;
class ClassConstListItemSyntax;
class MemberModifierSyntax;
class ClassPropertyDeclSyntax;
class ClassConstDeclSyntax;
class ClassMethodDeclSyntax;
class ClassTraitMethodReferenceSyntax;
class ClassAbsoluteTraitMethodReferenceSyntax;
class ClassTraitPrecedenceSyntax;
class ClassTraitAliasSyntax;
class ClassTraitAdaptationSyntax;
class ClassTraitAdaptationBlockSyntax;
class ClassTraitDeclSyntax;
class MemberDeclBlockSyntax;
class MemberDeclListItemSyntax;
class ClassDefinitionSyntax;
class InterfaceDefinitionSyntax;
class TraitDefinitionSyntax;
class SourceFileSyntax;

// stmt syntax nodes forward declares
class TopStmtSyntax;
class InnerCodeBlockStmtSyntax;
using TopStmtListSyntax = SyntaxCollection<SyntaxKind::TopStmtList, TopStmtSyntax>;

///
/// type: SyntaxCollection
/// element type: NameSyntax
///
/// name_list:
///   name
/// |	name_list ',' name
///
using NameListSyntax = SyntaxCollection<SyntaxKind::NameList, NameListItemSyntax>;

///
/// type: SyntaxCollection
/// element type: ParameterSyntax
///
/// parameter_list:
///   non_empty_parameter_list
/// |	/* empty */
/// non_empty_parameter_list:
///   parameter
/// |	non_empty_parameter_list ',' parameter
///
using ParameterListSyntax = SyntaxCollection<SyntaxKind::ParameterList, ParameterListItemSyntax>;

///
/// type: SyntaxCollection
/// element type: ClassModifierListSyntax
///
/// class_modifiers:
///   class_modifier
/// | class_modifiers class_modifier
///
using ClassModifierListSyntax = SyntaxCollection<SyntaxKind::ClassModifierList, ClassModifierSyntax>;

///
/// type: SyntaxCollection
/// element type: MemberDeclListItemSyntax
///
/// class_statement_list:
///   class_statement_list class_statement
///
using MemberDeclListSyntax = SyntaxCollection<SyntaxKind::MemberDeclList, MemberDeclListItemSyntax>;

///
/// type: SyntaxCollection
/// element type: MemberModifierSyntax
///
/// member_modifiers:
///   member_modifier
/// | member_modifiers member_modifier
///
using MemberModifierListSyntax = SyntaxCollection<SyntaxKind::MemberModifierList, MemberModifierSyntax>;

///
/// type: SyntaxCollection
/// element type: MemberModifierSyntax
///
/// property_list:
///   property_list ',' property
/// | property
///
using ClassPropertyListSyntax = SyntaxCollection<SyntaxKind::ClassPropertyList, ClassPropertyClauseSyntax>;

///
/// type: SyntaxCollection
/// element type: ClassConstListItemSyntax
///
/// class_const_list:
///   class_const_list ',' class_const_decl
///   | class_const_decl
///
using ClassConstListSyntax = SyntaxCollection<SyntaxKind::ClassConstList, ClassConstListItemSyntax>;

///
/// type: SyntaxCollection
/// element type: ClassTraitAdaptationSyntax
///
/// trait_adaptation_list:
///   trait_adaptation
/// | trait_adaptation_list trait_adaptation
///
using ClassTraitAdaptationListSyntax = SyntaxCollection<SyntaxKind::ClassTraitAdaptationList, ClassTraitAdaptationSyntax>;

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_FWD_H
