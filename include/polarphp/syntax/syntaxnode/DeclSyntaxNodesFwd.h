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

namespace polar::syntax {

class ReservedNonModifierSyntax;
class SemiReservedSytnax;
class IdentifierSyntax;
class NamespacePartSyntax;
class NameSyntax;
class NamespaceUseTypeSyntax;
class NamespaceUnprefixedUseDeclarationSyntax;
class NamespaceUseDeclarationSyntax;
class NamespaceInlineUseDeclarationSyntax;
class NamespaceGroupUseDeclarationSyntax;
class NamespaceMixedGroupUseDeclarationSyntax;
class NamespaceUseSyntax;
class ConstDeclareItemSyntax;
class ConstDefinitionSyntax;
class TypeClauseSyntax;
class TypeExprClauseSyntax;
class ReturnTypeClauseSyntax;
class InitializeClauseSyntax;
class ParameterSyntax;
class ParameterClauseSyntax;
class FunctionDefinitionSyntax;
class ClassModifierSyntax;
class ExtendsFromClauseSyntax;
class ImplementClauseSyntax;
class InterfaceExtendsClauseSyntax;
class ClassPropertySyntax;
class MemberModifierSyntax;
class MemberDeclBlockSyntax;
class MemberDeclListItemSyntax;
class ClassDefinitionSyntax;
class InterfaceDefinitionSyntax;
class TraitDefinitionSyntax;
class SourceFileSyntax;

///
/// type: SyntaxCollection
/// element type: NameSyntax
///
/// name_list:
///   name
/// |	name_list ',' name
///
using NameListSyntax = SyntaxCollection<SyntaxKind::NameList, NameSyntax>;

///
/// type: SyntaxCollection
/// element type: TokenSyntax
///
/// namespace_name:
///   T_IDENTIFIER_STRING
/// | namespace_name T_NS_SEPARATOR T_IDENTIFIER_STRING
///
using NamespacePartListSyntax = SyntaxCollection<SyntaxKind::NamespacePartList, NamespacePartSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceUseDeclarationSyntax
///
/// use_declarations:
///   use_declarations ',' use_declaration
/// | use_declaration
///
using NamespaceUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceUseDeclarationList, NamespaceUseDeclarationSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceInlineUseDeclarationSyntax
///
/// inline_use_declarations:
///   inline_use_declarations ',' inline_use_declaration
/// | inline_use_declaration
///
using NamespaceInlineUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceInlineUseDeclarationList, NamespaceInlineUseDeclarationSyntax>;

///
/// type: SyntaxCollection
/// element type: NamespaceUnprefixedUseDeclarationSyntax
///
/// unprefixed_use_declarations:
///   unprefixed_use_declarations ',' unprefixed_use_declaration
/// | unprefixed_use_declaration
///
using NamespaceUnprefixedUseDeclarationListSyntax = SyntaxCollection<SyntaxKind::NamespaceUnprefixedUseDeclarationList, NamespaceUnprefixedUseDeclarationSyntax>;

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
using ParameterListSyntax = SyntaxCollection<SyntaxKind::ParameterList, ParameterSyntax>;

///
/// type: SyntaxCollection
/// element type: ConstDeclareItemSyntax
///
/// const_list:
///   const_list ',' const_decl
/// | const_decl
///
using ConstDeclareItemListSyntax = SyntaxCollection<SyntaxKind::ConstDeclareItemList, ConstDeclareItemSyntax>;

///
/// type: SyntaxCollection
/// element type: ClassModififerListSyntax
///
/// class_modifiers:
///   class_modifier
/// | class_modifiers class_modifier
///
using ClassModififerListSyntax = SyntaxCollection<SyntaxKind::ClassModifierList, ClassModifierSyntax>;

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
using ClassPropertyListSyntax = SyntaxCollection<SyntaxKind::ParameterList, ClassPropertySyntax>;

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAX_NODE_DECL_NODES_FWD_H
