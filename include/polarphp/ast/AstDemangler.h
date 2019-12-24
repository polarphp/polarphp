//===--- ASTDemangler.h - Swift AST symbol demangling -----------*- C++ -*-===//
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
// Defines a builder concept for the TypeDecoder and MetadataReader which builds
// AST Types, and a utility function wrapper which takes a mangled string and
// feeds it through the TypeDecoder instance.
//
// The RemoteAST library defines a MetadataReader instance that uses this
// concept, together with some additional utilities.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_ASTDEMANGLER_H
#define POLARPHP_AST_ASTDEMANGLER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "polarphp/ast/Types.h"
#include "polarphp/demangling/Demangler.h"
#include "polarphp/demangling/TypeDecoder.h"

namespace polar {

class TypeDecl;

namespace demangling {

Type getTypeForMangling(AstContext &ctx,
                        llvm::StringRef mangling);

TypeDecl *getTypeDeclForMangling(AstContext &ctx,
                                 llvm::StringRef mangling);

TypeDecl *getTypeDeclForUSR(AstContext &ctx,
                            llvm::StringRef usr);

/// An implementation of MetadataReader's BuilderType concept that
/// just finds and builds things in the AST.
class AstBuilder {
   AstContext &Ctx;
   demangling::NodeFactory Factory;

   /// The notional context in which we're writing and type-checking code.
   /// Created lazily.
   DeclContext *NotionalDC = nullptr;

public:
   using BuiltType = polar::Type;
   using BuiltTypeDecl = polar::GenericTypeDecl *; // nominal or type alias
   using BuiltInterfaceDecl = polar::InterfaceDecl *;
   explicit AstBuilder(AstContext &ctx) : Ctx(ctx) {}

   AstContext &getAstContext() { return Ctx; }
   DeclContext *getNotionalDC();

   demangling::NodeFactory &getNodeFactory() { return Factory; }

   Type createBuiltinType(StringRef builtinName, StringRef mangledName);

   TypeDecl *createTypeDecl(NodePointer node);

   GenericTypeDecl *createTypeDecl(StringRef mangledName, bool &typeAlias);

   GenericTypeDecl *createTypeDecl(NodePointer node,
                                   bool &typeAlias);

   InterfaceDecl *createInterfaceDecl(NodePointer node);

   Type createNominalType(GenericTypeDecl *decl);

   Type createNominalType(GenericTypeDecl *decl, Type parent);

   Type createTypeAliasType(GenericTypeDecl *decl, Type parent);

   Type createBoundGenericType(GenericTypeDecl *decl, ArrayRef<Type> args);

   Type resolveOpaqueType(NodePointer opaqueDescriptor,
                          ArrayRef<ArrayRef<Type>> args,
                          unsigned ordinal);

   Type createBoundGenericType(GenericTypeDecl *decl, ArrayRef<Type> args,
                               Type parent);

   Type createTupleType(ArrayRef<Type> eltTypes, StringRef labels,
                        bool isVariadic);

   Type createFunctionType(ArrayRef<demangling::FunctionParam<Type>> params,
                           Type output, FunctionTypeFlags flags);

   Type createImplFunctionType(
      demangling::ImplParameterConvention calleeConvention,
      ArrayRef<demangling::ImplFunctionParam<Type>> params,
      ArrayRef<demangling::ImplFunctionResult<Type>> results,
      Optional<demangling::ImplFunctionResult<Type>> errorResult,
      ImplFunctionTypeFlags flags);

   Type createInterfaceCompositionType(ArrayRef<InterfaceDecl *> protocols,
                                       Type superclass,
                                       bool isClassBound);

   Type createExistentialMetatypeType(Type instance,
                                      Optional<demangling::ImplMetatypeRepresentation> repr=None);

   Type createMetatypeType(Type instance,
                           Optional<demangling::ImplMetatypeRepresentation> repr=None);

   Type createGenericTypeParameterType(unsigned depth, unsigned index);

   Type createDependentMemberType(StringRef member, Type base);

   Type createDependentMemberType(StringRef member, Type base,
                                  InterfaceDecl *protocol);

#define REF_STORAGE(Name, ...) \
  Type create##Name##StorageType(Type base);
#include "polarphp/ast/ReferenceStorageDef.h"

   Type createPILBoxType(Type base);

   Type createObjCClassType(StringRef name);

   Type createBoundGenericObjCClassType(StringRef name, ArrayRef<Type> args);

   InterfaceDecl *createObjCInterfaceDecl(StringRef name);

   Type createDynamicSelfType(Type selfType);

   Type createForeignClassType(StringRef mangledName);

   Type getUnnamedForeignClassType();

   Type getOpaqueType();

   Type createOptionalType(Type base);

   Type createArrayType(Type base);

   Type createDictionaryType(Type key, Type value);

   Type createParenType(Type base);

private:
   bool validateParentType(TypeDecl *decl, Type parent);
   CanGenericSignature demangleGenericSignature(
      NominalTypeDecl *nominalDecl,
      NodePointer node);
   DeclContext *findDeclContext(NodePointer node);
   ModuleDecl *findModule(NodePointer node);
   demangling::NodePointer findModuleNode(NodePointer node);

   enum class ForeignModuleKind {
      Imported,
      SynthesizedByImporter
   };

   Optional<ForeignModuleKind>
   getForeignModuleKind(NodePointer node);

   GenericTypeDecl *findTypeDecl(DeclContext *dc,
                                 Identifier name,
                                 Identifier privateDiscriminator,
                                 demangling::Node::Kind kind);
   GenericTypeDecl *findForeignTypeDecl(StringRef name,
                                        StringRef relatedEntityKind,
                                        ForeignModuleKind lookupKind,
                                        demangling::Node::Kind kind);

   static GenericTypeDecl *getAcceptableTypeDeclCandidate(ValueDecl *decl,
                                                          demangling::Node::Kind kind);
};

}  // namespace demangling
}  // namespace polar

#endif  // POLARPHP_AST_ASTDEMANGLER_H
