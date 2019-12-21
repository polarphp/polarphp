//===--- PILGen.h - Implements Lowering of ASTs -> PIL ----------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_GEN_PILGEN_H
#define POLARPHP_PIL_GEN_PILGEN_H

#include "polarphp/pil/gen/AstVisitor.h"
#include "polarphp/pil/gen/Cleanup.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AnyFunctionRef.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/lang/TypeLowering.h"
#include "llvm/ADT/DenseMap.h"
#include <deque>

namespace polar {
class PILBasicBlock;

namespace lowering {
class TypeConverter;
class PILGenFunction;

/// An enum to indicate whether a protocol method requirement is satisfied by
/// a free function, as for an operator requirement.
enum IsFreeFunctionWitness_t : bool {
   IsNotFreeFunctionWitness = false,
   IsFreeFunctionWitness = true,
};

/// An AstVisitor for generating PIL from top-level declarations in a module.
class LLVM_LIBRARY_VISIBILITY PILGenModule : public AstVisitor<PILGenModule> {
public:
   /// The Module being constructed.
   PILModule &M;

   /// The type converter for the module.
   TypeConverter &Types;

   /// The Swift module we are visiting.
   ModuleDecl *PolarphpModule;

   /// TopLevelSGF - The PILGenFunction used to visit top-level code, or null if
   /// the current source file is not a script source file.
   PILGenFunction /*nullable*/ *TopLevelSGF;

   /// Mapping from PILDeclRefs to emitted PILFunctions.
   llvm::DenseMap<PILDeclRef, PILFunction*> emittedFunctions;
   /// Mapping from InterfaceConformances to emitted PILWitnessTables.
   llvm::DenseMap<NormalInterfaceConformance*, PILWitnessTable*> emittedWitnessTables;

   struct DelayedFunction {
      /// Insert the entity after the given function when it's emitted.
      PILDeclRef insertAfter;
      /// Code that generates the function.
      std::function<void (PILFunction *)> emitter;
   };

   /// Mapping from PILDeclRefs to delayed PILFunction generators for
   /// non-externally-visible symbols.
   llvm::DenseMap<PILDeclRef, DelayedFunction> delayedFunctions;

   /// Queue of delayed PILFunctions that need to be forced.
   std::deque<std::pair<PILDeclRef, DelayedFunction>> forcedFunctions;

   /// The most recent declaration we considered for emission.
   PILDeclRef lastEmittedFunction;

   /// Bookkeeping to ensure that useConformancesFrom{ObjectiveC,}Type() is
   /// only called once for each unique type, as an optimization.
   llvm::DenseSet<TypeBase *> usedConformancesFromTypes;
   llvm::DenseSet<TypeBase *> usedConformancesFromObjectiveCTypes;

   /// Queue of delayed conformances that need to be emitted.
   std::deque<NormalInterfaceConformance *> pendingConformances;

   /// Set of delayed conformances that have already been forced.
   llvm::DenseSet<NormalInterfaceConformance *> forcedConformances;

   PILFunction *emitTopLevelFunction(PILLocation Loc);

   size_t anonymousSymbolCounter = 0;

   Optional<PILDeclRef> StringToNSStringFn;
   Optional<PILDeclRef> NSStringToStringFn;
   Optional<PILDeclRef> ArrayToNSArrayFn;
   Optional<PILDeclRef> NSArrayToArrayFn;
   Optional<PILDeclRef> DictionaryToNSDictionaryFn;
   Optional<PILDeclRef> NSDictionaryToDictionaryFn;
   Optional<PILDeclRef> SetToNSSetFn;
   Optional<PILDeclRef> NSSetToSetFn;
   Optional<PILDeclRef> BoolToObjCBoolFn;
   Optional<PILDeclRef> ObjCBoolToBoolFn;
   Optional<PILDeclRef> BoolToDarwinBooleanFn;
   Optional<PILDeclRef> DarwinBooleanToBoolFn;
   Optional<PILDeclRef> NSErrorToErrorFn;
   Optional<PILDeclRef> ErrorToNSErrorFn;
   Optional<PILDeclRef> BoolToWindowsBoolFn;
   Optional<PILDeclRef> WindowsBoolToBoolFn;

   Optional<InterfaceDecl*> PointerInterface;

   Optional<InterfaceDecl*> ObjectiveCBridgeable;
   Optional<FuncDecl*> BridgeToObjectiveCRequirement;
   Optional<FuncDecl*> UnconditionallyBridgeFromObjectiveCRequirement;
   Optional<AssociatedTypeDecl*> BridgedObjectiveCType;

   Optional<InterfaceDecl*> BridgedStoredNSError;
   Optional<VarDecl*> NSErrorRequirement;

   Optional<InterfaceConformance *> NSErrorConformanceToError;

public:
   PILGenModule(PILModule &M, ModuleDecl *SM);

   ~PILGenModule();

   PILGenModule(PILGenModule const &) = delete;
   void operator=(PILGenModule const &) = delete;

   AstContext &getAstContext() { return M.getAstContext(); }

   static DeclName getMagicFunctionName(PILDeclRef ref);
   static DeclName getMagicFunctionName(DeclContext *dc);

   /// Get the function for a PILDeclRef, or return nullptr if it hasn't been
   /// emitted yet.
   PILFunction *getEmittedFunction(PILDeclRef constant,
                                   ForDefinition_t forDefinition);

   /// Get the function for a PILDeclRef, creating it if necessary.
   PILFunction *getFunction(PILDeclRef constant,
                            ForDefinition_t forDefinition);

   /// Get the dynamic dispatch thunk for a PILDeclRef.
   PILFunction *getDynamicThunk(PILDeclRef constant,
                                CanPILFunctionType constantTy);

   /// Emit a vtable thunk for a derived method if its natural abstraction level
   /// diverges from the overridden base method. If no thunking is needed,
   /// returns a static reference to the derived method.
   Optional<PILVTable::Entry> emitVTableMethod(ClassDecl *theClass,
                                               PILDeclRef derived,
                                               PILDeclRef base);

   /// True if a function has been emitted for a given PILDeclRef.
   bool hasFunction(PILDeclRef constant);

   /// Get or create the declaration of a reabstraction thunk with the
   /// given signature.
   PILFunction *getOrCreateReabstractionThunk(
      CanPILFunctionType thunkType,
      CanPILFunctionType fromType,
      CanPILFunctionType toType,
      CanType dynamicSelfType);

   /// Determine whether the given class has any instance variables that
   /// need to be destroyed.
   bool hasNonTrivialIVars(ClassDecl *cd);

   /// Determine whether we need to emit an ivar destroyer for the given class.
   /// An ivar destroyer is needed if a superclass of this class may define a
   /// failing designated initializer.
   bool requiresIVarDestroyer(ClassDecl *cd);

   //===--------------------------------------------------------------------===//
   // Visitors for top-level forms
   //===--------------------------------------------------------------------===//

   // These are either not allowed at global scope or don't require
   // code emission.
   void visitImportDecl(ImportDecl *d) {}
   void visitEnumCaseDecl(EnumCaseDecl *d) {}
   void visitEnumElementDecl(EnumElementDecl *d) {}
   void visitOperatorDecl(OperatorDecl *d) {}
   void visitPrecedenceGroupDecl(PrecedenceGroupDecl *d) {}
   void visitTypeAliasDecl(TypeAliasDecl *d) {}
   void visitOpaqueTypeDecl(OpaqueTypeDecl *d) {}
   void visitAbstractTypeParamDecl(AbstractTypeParamDecl *d) {}
   void visitConstructorDecl(ConstructorDecl *d) {}
   void visitDestructorDecl(DestructorDecl *d) {}
   void visitModuleDecl(ModuleDecl *d) { }
   void visitMissingMemberDecl(MissingMemberDecl *d) {}

   // Emitted as part of its storage.
   void visitAccessorDecl(AccessorDecl *ad) {}

   void visitFuncDecl(FuncDecl *fd);
   void visitPatternBindingDecl(PatternBindingDecl *vd);
   void visitTopLevelCodeDecl(TopLevelCodeDecl *td);
   void visitIfConfigDecl(IfConfigDecl *icd);
   void visitPoundDiagnosticDecl(PoundDiagnosticDecl *PDD);
   void visitNominalTypeDecl(NominalTypeDecl *ntd);
   void visitExtensionDecl(ExtensionDecl *ed);
   void visitVarDecl(VarDecl *vd);
   void visitSubscriptDecl(SubscriptDecl *sd);

   void emitAbstractFuncDecl(AbstractFunctionDecl *AFD);

   /// Generate code for a source file of the module.
   void emitSourceFile(SourceFile *sf);

   /// Generates code for the given FuncDecl and adds the
   /// PILFunction to the current PILModule under the name PILDeclRef(decl). For
   /// curried functions, curried entry point Functions are also generated and
   /// added to the current PILModule.
   void emitFunction(FuncDecl *fd);

   /// Generates code for the given closure expression and adds the
   /// PILFunction to the current PILModule under the name PILDeclRef(ce).
   PILFunction *emitClosure(AbstractClosureExpr *ce);
   /// Generates code for the given ConstructorDecl and adds
   /// the PILFunction to the current PILModule under the name PILDeclRef(decl).
   void emitConstructor(ConstructorDecl *decl);

   /// Generates code for the given class's destructor and adds
   /// the PILFunction to the current PILModule under the name
   /// PILDeclRef(cd, Destructor).
   void emitDestructor(ClassDecl *cd, DestructorDecl *dd);

   /// Generates the enum constructor for the given
   /// EnumElementDecl under the name PILDeclRef(decl).
   void emitEnumConstructor(EnumElementDecl *decl);

   /// Emits the default argument generator with the given expression.
   void emitDefaultArgGenerator(PILDeclRef constant, ParamDecl *param);

   /// Emits the stored property initializer for the given pattern.
   void emitStoredPropertyInitialization(PatternBindingDecl *pd, unsigned i);

   /// Emits the backing initializer for a property with an attached wrapper.
   void emitPropertyWrapperBackingInitializer(VarDecl *var);

   /// Emits default argument generators for the given parameter list.
   void emitDefaultArgGenerators(PILDeclRef::Loc decl,
                                 ParameterList *paramList);

   /// Emits the curry thunk between two uncurry levels of a function.
   void emitCurryThunk(PILDeclRef thunk);

   /// Emits a thunk from a foreign function to the native Swift convention.
   void emitForeignToNativeThunk(PILDeclRef thunk);

   /// Emits a thunk from a Swift function to the native Swift convention.
   void emitNativeToForeignThunk(PILDeclRef thunk);

   void preEmitFunction(PILDeclRef constant,
                        llvm::PointerUnion<ValueDecl *,
                           Expr *> astNode,
                        PILFunction *F,
                        PILLocation L);
   void postEmitFunction(PILDeclRef constant, PILFunction *F);

   /// Add a global variable to the PILModule.
   void addGlobalVariable(VarDecl *global);

   /// Emit the ObjC-compatible entry point for a method.
   void emitObjCMethodThunk(FuncDecl *method);

   /// Emit the ObjC-compatible getter and setter for a property.
   void emitObjCPropertyMethodThunks(AbstractStorageDecl *prop);

   /// Emit the ObjC-compatible entry point for a constructor.
   void emitObjCConstructorThunk(ConstructorDecl *constructor);

   /// Emit the ObjC-compatible entry point for a destructor (i.e., -dealloc).
   void emitObjCDestructorThunk(DestructorDecl *destructor);

   /// Get or emit the witness table for a protocol conformance.
   PILWitnessTable *getWitnessTable(NormalInterfaceConformance *conformance);

   /// Emit a protocol witness entry point.
   PILFunction *
   emitInterfaceWitness(InterfaceConformanceRef conformance, PILLinkage linkage,
                        IsSerialized_t isSerialized, PILDeclRef requirement,
                        PILDeclRef witnessRef, IsFreeFunctionWitness_t isFree,
                        Witness witness);

   /// Emit the default witness table for a resilient protocol.
   void emitDefaultWitnessTable(InterfaceDecl *protocol);

   /// Emit the self-conformance witness table for a protocol.
   void emitSelfConformanceWitnessTable(InterfaceDecl *protocol);

   /// Emit the lazy initializer function for a global pattern binding
   /// declaration.
   PILFunction *emitLazyGlobalInitializer(StringRef funcName,
                                          PatternBindingDecl *binding,
                                          unsigned pbdEntry);

   /// Emit the accessor for a global variable or stored static property.
   ///
   /// This ensures the lazy initializer has been run before returning the
   /// address of the variable.
   void emitGlobalAccessor(VarDecl *global,
                           PILGlobalVariable *onceToken,
                           PILFunction *onceFunc);

   // @todo
//   /// True if the given function requires an entry point for ObjC method
//   /// dispatch.
//   bool requiresObjCMethodEntryPoint(FuncDecl *method);
//
//   /// True if the given constructor requires an entry point for ObjC method
//   /// dispatch.
//   bool requiresObjCMethodEntryPoint(ConstructorDecl *constructor);

   /// Emit a global initialization.
   void emitGlobalInitialization(PatternBindingDecl *initializer, unsigned elt);

   /// Should the self argument of the given method always be emitted as
   /// an r-value (meaning that it can be borrowed only if that is not
   /// semantically detectable), or it acceptable to emit it as a borrowed
   /// storage reference?
   bool shouldEmitSelfAsRValue(FuncDecl *method, CanType selfType);

   /// Is the self method of the given nonmutating method passed indirectly?
   bool isNonMutatingSelfIndirect(PILDeclRef method);

   PILDeclRef getAccessorDeclRef(AccessorDecl *accessor);

   bool canStorageUseStoredKeyPathComponent(AbstractStorageDecl *decl,
                                            ResilienceExpansion expansion);

   KeyPathPatternComponent
   emitKeyPathComponentForDecl(PILLocation loc,
                               GenericEnvironment *genericEnv,
                               ResilienceExpansion expansion,
                               unsigned &baseOperand,
                               bool &needsGenericContext,
                               SubstitutionMap subs,
                               AbstractStorageDecl *storage,
                               ArrayRef<InterfaceConformanceRef> indexHashables,
                               CanType baseTy,
                               bool forPropertyDescriptor);

   /// Known functions for bridging.
   PILDeclRef getStringToNSStringFn();
   PILDeclRef getNSStringToStringFn();
   PILDeclRef getArrayToNSArrayFn();
   PILDeclRef getNSArrayToArrayFn();
   PILDeclRef getDictionaryToNSDictionaryFn();
   PILDeclRef getNSDictionaryToDictionaryFn();
   PILDeclRef getSetToNSSetFn();
   PILDeclRef getNSSetToSetFn();
   PILDeclRef getBoolToObjCBoolFn();
   PILDeclRef getObjCBoolToBoolFn();
   PILDeclRef getBoolToDarwinBooleanFn();
   PILDeclRef getDarwinBooleanToBoolFn();
   PILDeclRef getBoolToWindowsBoolFn();
   PILDeclRef getWindowsBoolToBoolFn();
   PILDeclRef getNSErrorToErrorFn();
   PILDeclRef getErrorToNSErrorFn();

#define FUNC_DECL(NAME, ID) \
  FuncDecl *get##NAME(PILLocation loc);
#include "polarphp/ast/KnownDeclsDef.h"

   /// Retrieve the _ObjectiveCBridgeable protocol definition.
   InterfaceDecl *getObjectiveCBridgeable(PILLocation loc);

   /// Retrieve the _ObjectiveCBridgeable._bridgeToObjectiveC requirement.
   FuncDecl *getBridgeToObjectiveCRequirement(PILLocation loc);

   /// Retrieve the
   /// _ObjectiveCBridgeable._unconditionallyBridgeFromObjectiveC
   /// requirement.
   FuncDecl *getUnconditionallyBridgeFromObjectiveCRequirement(PILLocation loc);

   /// Retrieve the _ObjectiveCBridgeable._ObjectiveCType requirement.
   AssociatedTypeDecl *getBridgedObjectiveCTypeRequirement(PILLocation loc);

   /// Find the conformance of the given Swift type to the
   /// _ObjectiveCBridgeable protocol.
   InterfaceConformance *getConformanceToObjectiveCBridgeable(PILLocation loc,
                                                              Type type);

   /// Retrieve the _BridgedStoredNSError protocol definition.
   InterfaceDecl *getBridgedStoredNSError(PILLocation loc);

   /// Retrieve the _BridgedStoredNSError._nsError requirement.
   VarDecl *getNSErrorRequirement(PILLocation loc);

   /// Find the conformance of the given Swift type to the
   /// _BridgedStoredNSError protocol.
   InterfaceConformanceRef getConformanceToBridgedStoredNSError(PILLocation loc,
                                                                Type type);

   /// Retrieve the conformance of NSError to the Error protocol.
   InterfaceConformance *getNSErrorConformanceToError();

   PILFunction *getKeyPathProjectionCoroutine(bool isReadAccess,
                                              KeyPathTypeKind typeKind);

   /// Report a diagnostic.
   template<typename...T, typename...U>
   InFlightDiagnostic diagnose(SourceLoc loc, Diag<T...> diag,
                               U &&...args) {
      return M.getAstContext().Diags.diagnose(loc, diag, std::forward<U>(args)...);
   }

   template<typename...T, typename...U>
   InFlightDiagnostic diagnose(PILLocation loc, Diag<T...> diag,
                               U &&...args) {
      return M.getAstContext().Diags.diagnose(loc.getSourceLoc(),
                                              diag, std::forward<U>(args)...);
   }

   /// Get or create PILGlobalVariable for a given global VarDecl.
   PILGlobalVariable *getPILGlobalVariable(VarDecl *gDecl,
                                           ForDefinition_t forDef);

   /// Emit all lazy conformances referenced from this function body.
   void emitLazyConformancesForFunction(PILFunction *F);

   /// Emit all lazy conformances referenced from this type's signature and
   /// stored properties (or in the case of enums, associated values).
   void emitLazyConformancesForType(NominalTypeDecl *NTD);

   /// Mark a protocol conformance as used, so we know we need to emit it if
   /// it's in our TU.
   void useConformance(InterfaceConformanceRef conformance);

   /// Mark protocol conformances from the given type as used.
   void useConformancesFromType(CanType type);

   /// Mark protocol conformances from the given set of substitutions as used.
   void useConformancesFromSubstitutions(SubstitutionMap subs);

   /// Mark _ObjectiveCBridgeable conformances as used for any imported types
   /// mentioned by the given type.
   void useConformancesFromObjectiveCType(CanType type);

   /// Emit a `mark_function_escape` instruction for top-level code when a
   /// function or closure at top level refers to script globals.
   void emitMarkFunctionEscapeForTopLevelCodeGlobals(PILLocation loc,
                                                     CaptureInfo captureInfo);

   /// Map the substitutions for the original declaration to substitutions for
   /// the overridden declaration.
   static SubstitutionMap mapSubstitutionsForWitnessOverride(
      AbstractFunctionDecl *original,
      AbstractFunctionDecl *overridden,
      SubstitutionMap subs);

   /// Emit a property descriptor for the given storage decl if it needs one.
   void tryEmitPropertyDescriptor(AbstractStorageDecl *decl);

private:
   /// Emit the deallocator for a class that uses the objc allocator.
   void emitObjCAllocatorDestructor(ClassDecl *cd, DestructorDecl *dd);
};

} // end namespace lowering
} // end namespace polar

#endif // POLARPHP_PIL_GEN_PILGEN_H
