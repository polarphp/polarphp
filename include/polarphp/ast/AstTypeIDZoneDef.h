//===--- ASTTypeIDZone.def - Define the AST TypeID Zone ---------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
//  This definition file describes the types in the "AST" TypeID zone,
//  for use with the TypeID template.
//
//===----------------------------------------------------------------------===//

POLAR_TYPEID(AncestryFlags)
POLAR_TYPEID(CtorInitializerKind)
POLAR_TYPEID(FunctionBuilderClosurePreCheck)
POLAR_TYPEID(GenericSignature)
POLAR_TYPEID(ImplicitMemberAction)
POLAR_TYPEID(ParamSpecifier)
POLAR_TYPEID(PropertyWrapperBackingPropertyInfo)
POLAR_TYPEID(PropertyWrapperTypeInfo)
POLAR_TYPEID(Requirement)
POLAR_TYPEID(ResilienceExpansion)
POLAR_TYPEID(Type)
POLAR_TYPEID(TypePair)
POLAR_TYPEID(TypeWitnessAndDecl)
POLAR_TYPEID(Witness)
POLAR_TYPEID_NAMED(ClosureExpr *, ClosureExpr)
POLAR_TYPEID_NAMED(ConstructorDecl *, ConstructorDecl)
POLAR_TYPEID_NAMED(CustomAttr *, CustomAttr)
POLAR_TYPEID_NAMED(Decl *, Decl)
POLAR_TYPEID_NAMED(EnumDecl *, EnumDecl)
POLAR_TYPEID_NAMED(GenericParamList *, GenericParamList)
POLAR_TYPEID_NAMED(GenericTypeParamType *, GenericTypeParamType)
POLAR_TYPEID_NAMED(InfixOperatorDecl *, InfixOperatorDecl)
POLAR_TYPEID_NAMED(IterableDeclContext *, IterableDeclContext)
POLAR_TYPEID_NAMED(ModuleDecl *, ModuleDecl)
POLAR_TYPEID_NAMED(NamedPattern *, NamedPattern)
POLAR_TYPEID_NAMED(NominalTypeDecl *, NominalTypeDecl)
POLAR_TYPEID_NAMED(OpaqueTypeDecl *, OpaqueTypeDecl)
POLAR_TYPEID_NAMED(OperatorDecl *, OperatorDecl)
POLAR_TYPEID_NAMED(Optional<PropertyWrapperMutability>,
                   PropertyWrapperMutability)
POLAR_TYPEID_NAMED(ParamDecl *, ParamDecl)
POLAR_TYPEID_NAMED(PatternBindingEntry *, PatternBindingEntry)
POLAR_TYPEID_NAMED(PrecedenceGroupDecl *, PrecedenceGroupDecl)
POLAR_TYPEID_NAMED(InterfaceDecl *, InterfaceDecl)
POLAR_TYPEID_NAMED(SourceFile *, SourceFile)
POLAR_TYPEID_NAMED(TypeAliasDecl *, TypeAliasDecl)
POLAR_TYPEID_NAMED(ValueDecl *, ValueDecl)
POLAR_TYPEID_NAMED(VarDecl *, VarDecl)
