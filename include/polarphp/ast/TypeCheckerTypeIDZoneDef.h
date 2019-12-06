//===--- TypeCheckerTypeIDZone.def ------------------------------*- C++ -*-===//
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
//  This definition file describes the types in the type checker's
//  TypeID zone, for use with the TypeID template.
//
//===----------------------------------------------------------------------===//

POLAR_REQUEST(TypeChecker, AbstractGenericSignatureRequest,
              GenericSignature (GenericSignatureImpl *,
                                SmallVector<GenericTypeParamType *, 2>,
                                SmallVector<Requirement, 2>),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, AttachedFunctionBuilderRequest,
              CustomAttr *(ValueDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, AttachedPropertyWrapperTypeRequest,
              Type(VarDecl *, unsigned), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, AttachedPropertyWrappersRequest,
              llvm::TinyPtrVector<CustomAttr *>(VarDecl *), Cached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, CallerSideDefaultArgExprRequest,
              Expr *(DefaultArgumentExpr *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ClassAncestryFlagsRequest,
              AncestryFlags(ClassDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, CompareDeclSpecializationRequest,
              bool (DeclContext *, ValueDecl *, ValueDecl *, bool), Cached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, DefaultArgumentExprRequest,
              Expr *(ParamDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, DefaultArgumentInitContextRequest,
              Initializer *(ParamDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, DefaultDefinitionTypeRequest,
              Type(AssociatedTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, DefaultTypeRequest,
              Type(KnownInterfaceKind, const DeclContext *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, DynamicallyReplacedDeclRequest,
              ValueDecl *(ValueDecl *),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, EmittedMembersRequest, DeclRange(ClassDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, EnumRawValuesRequest,
              bool (EnumDecl *, TypeResolutionStage), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, EnumRawTypeRequest,
              Type(EnumDecl *, TypeResolutionStage), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, ExistentialConformsToSelfRequest,
              bool(InterfaceDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ExistentialTypeSupportedRequest,
              bool(InterfaceDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ExtendedTypeRequest, Type(ExtensionDecl *), Cached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, FunctionBuilderTypeRequest, Type(ValueDecl *),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, FunctionOperatorRequest, OperatorDecl *(FuncDecl *),
              Cached, NoLocationInfo)
POLAR_REQUEST(NameLookup, GenericSignatureRequest,
              GenericSignature (GenericContext *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, HasCircularInheritanceRequest,
              bool(ClassDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, HasCircularInheritedInterfacesRequest,
              bool(InterfaceDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, HasCircularRawValueRequest,
              bool(EnumDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, InferredGenericSignatureRequest,
              GenericSignature (ModuleDecl *, GenericSignatureImpl *,
                                GenericParamList *,
                                SmallVector<Requirement, 2>,
                                SmallVector<TypeLoc, 2>, bool),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, InheritedTypeRequest,
              Type(llvm::PointerUnion<TypeDecl *, ExtensionDecl *>, unsigned,
                   TypeResolutionStage),
              SeparatelyCached, HasNearestLocation)
POLAR_REQUEST(TypeChecker, InheritsSuperclassInitializersRequest,
              bool(ClassDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, InitKindRequest,
              CtorInitializerKind(ConstructorDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, InterfaceTypeRequest,
              Type(ValueDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsAccessorTransparentRequest, bool(AccessorDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsCallableNominalTypeRequest,
              bool(CanType, DeclContext *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsDynamicRequest, bool(ValueDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsFinalRequest, bool(ValueDecl *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsGetterMutatingRequest, bool(AbstractStorageDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsImplicitlyUnwrappedOptionalRequest,
              bool(ValueDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsObjCRequest, bool(ValueDecl *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsSetterMutatingRequest, bool(AbstractStorageDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, LazyStoragePropertyRequest, VarDecl *(VarDecl *),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, LookupPrecedenceGroupRequest,
              PrecedenceGroupDecl *(DeclContext *, Identifier, SourceLoc),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, MangleLocalTypeDeclRequest,
              std::string(const TypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, NamingPatternRequest,
              NamedPattern *(VarDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, OpaqueReadOwnershipRequest,
              OpaqueReadOwnership(AbstractStorageDecl *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, OpaqueResultTypeRequest,
              OpaqueTypeDecl *(ValueDecl *),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, OperatorPrecedenceGroupRequest,
              PrecedenceGroupDecl *(PrecedenceGroupDecl *),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, OverriddenDeclsRequest,
              llvm::TinyPtrVector<ValueDecl *>(ValueDecl *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, PatternBindingEntryRequest,
              const PatternBindingEntry *(PatternBindingDecl *, unsigned),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, PropertyWrapperBackingPropertyInfoRequest,
              PropertyWrapperBackingPropertyInfo(VarDecl *), Cached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, PropertyWrapperBackingPropertyTypeRequest,
              Type(VarDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, PropertyWrapperMutabilityRequest,
              Optional<PropertyWrapperMutability>(VarDecl *), Cached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, PropertyWrapperTypeInfoRequest,
              PropertyWrapperTypeInfo(NominalTypeDecl *), Cached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, InterfaceRequiresClassRequest, bool(InterfaceDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, RequirementRequest,
              Requirement(WhereClauseOwner, unsigned, TypeResolutionStage),
              SeparatelyCached, HasNearestLocation)
POLAR_REQUEST(TypeChecker, RequirementSignatureRequest,
              ArrayRef<Requirement>(InterfaceDecl *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, RequiresOpaqueAccessorsRequest, bool(VarDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, RequiresOpaqueModifyCoroutineRequest,
              bool(AbstractStorageDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ResilienceExpansionRequest,
              ResilienceExpansion(DeclContext *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, SelfAccessKindRequest, SelfAccessKind(FuncDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, StorageImplInfoRequest,
              StorageImplInfo(AbstractStorageDecl *), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, StoredPropertiesAndMissingMembersRequest,
              ArrayRef<Decl *>(NominalTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, StoredPropertiesRequest,
              ArrayRef<VarDecl *>(NominalTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, StructuralTypeRequest, Type(TypeAliasDecl *), Cached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, SuperclassTypeRequest,
              Type(NominalTypeDecl *, TypeResolutionStage), SeparatelyCached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, SynthesizeAccessorRequest,
              AccessorDecl *(AbstractStorageDecl *, AccessorKind),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, TypeCheckFunctionBodyUntilRequest,
              bool(AbstractFunctionDecl *, SourceLoc), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, UnderlyingTypeRequest, Type(TypeAliasDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, USRGenerationRequest, std::string(const ValueDecl *),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsABICompatibleOverrideRequest,
              bool(ValueDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, IsStaticRequest,
              bool(FuncDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, NeedsNewVTableEntryRequest,
              bool(AbstractFunctionDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ParamSpecifierRequest,
              ParamDecl::Specifier(ParamDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ResultTypeRequest,
              Type(ValueDecl *), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, AreAllStoredPropertiesDefaultInitableRequest,
              bool(NominalTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, HasUserDefinedDesignatedInitRequest,
              bool(NominalTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, HasMemberwiseInitRequest,
              bool(StructDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, PreCheckFunctionBuilderRequest,
              FunctionBuilderClosurePreCheck(ClosureExpr *),
              Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ResolveImplicitMemberRequest,
              bool(NominalTypeDecl *, ImplicitMemberAction), Uncached,
              NoLocationInfo)
POLAR_REQUEST(TypeChecker, SynthesizeMemberwiseInitRequest,
              ConstructorDecl *(NominalTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, HasDefaultInitRequest,
              bool(NominalTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, SynthesizeDefaultInitRequest,
              ConstructorDecl *(NominalTypeDecl *), Cached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, TypeCheckSourceFileRequest,
              bool(SouceFile *, unsigned), SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, TypeWitnessRequest,
              TypeWitnessAndDecl(NormalInterfaceConformance *,
                                 AssociatedTypeDecl *),
              SeparatelyCached, NoLocationInfo)
POLAR_REQUEST(TypeChecker, ValueWitnessRequest,
              Witness(NormalInterfaceConformance *, ValueDecl *),
              SeparatelyCached, NoLocationInfo)
