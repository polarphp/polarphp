//===--- NameLookupTypeIDZone.def -------------------------------*- C++ -*-===//
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
//  This definition file describes the types in the name-lookup
//  TypeID zone, for use with the TypeID template.
//
//===----------------------------------------------------------------------===//

POLAR_REQUEST(NameLookup, CustomAttrNominalRequest,
              NominalTypeDecl *(CustomAttr *, DeclContext *), Cached,
              NoLocationInfo)
POLAR_REQUEST(NameLookup, ExpandAstScopeRequest,
              ast_scope::ASTScopeImpl* (ast_scope::ASTScopeImpl*, ast_scope::ScopeCreator*),
              SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(NameLookup, ExtendedNominalRequest,
              NominalTypeDecl *(ExtensionDecl *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(NameLookup, GenericParamListRequest,
              GenericParamList *(GenericContext *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(NameLookup, GetDestructorRequest, DestructorDecl *(ClassDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(NameLookup, InheritedDeclsReferencedRequest,
              DirectlyReferencedTypeDecls(
                  llvm::PointerUnion<TypeDecl *, ExtensionDecl *>, unsigned),
              Uncached, HasNearestLocation)
POLAR_REQUEST(NameLookup, SelfBoundsFromWhereClauseRequest,
              SelfBounds(llvm::PointerUnion<TypeDecl *, ExtensionDecl *>),
              Uncached, NoLocationInfo)
POLAR_REQUEST(NameLookup, SuperclassDeclRequest, ClassDecl *(NominalTypeDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(NameLookup, TypeDeclsFromWhereClauseRequest,
              DirectlyReferencedTypeDecls(ExtensionDecl *), Uncached,
              NoLocationInfo)
POLAR_REQUEST(NameLookup, UnderlyingTypeDeclsReferencedRequest,
              DirectlyReferencedTypeDecls(TypeAliasDecl *), Uncached,
              NoLocationInfo)
POLAR_REQUEST(NameLookup, UnqualifiedLookupRequest,
              LookupResult(UnqualifiedLookupDescriptor), Uncached,
              NoLocationInfo)
