//===--- PILGen.cpp - Implements Lowering of ASTs -> PIL ------------------===//
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

#define DEBUG_TYPE "pilgen"

#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/pil/gen/PILGenFunction.h"
#include "polarphp/pil/gen/PILGenFunctionBuilder.h"
#include "polarphp/pil/gen/Scope.h"
#include "polarphp/ast/DiagnosticsPIL.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/Initializer.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/PrettyStackTrace.h"
#include "polarphp/ast/PropertyWrappers.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/ResilienceExpansion.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/TypeCheckRequests.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/Timer.h"
#include "polarphp/clangimporter/ClangModule.h"
#include "polarphp/pil/lang/PrettyStackTrace.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILProfiler.h"
#include "polarphp/serialization/SerializedModuleLoader.h"
#include "polarphp/serialization/SerializedPILLoader.h"
#include "polarphp/global/NameStrings.h"
#include "polarphp/global/Subsystems.h"
#include "llvm/ProfileData/InstrProfReader.h"
#include "llvm/Support/Debug.h"

using namespace polar;
using namespace lowering;

//===----------------------------------------------------------------------===//
// PILGenModule Class implementation
//===----------------------------------------------------------------------===//

PILGenModule::PILGenModule(PILModule &M, ModuleDecl *SM)
   : M(M), Types(M.Types), PolarphpModule(SM), TopLevelSGF(nullptr) {
   PILOptions &Opts = M.getOptions();
   if (!Opts.UseProfile.empty()) {
      auto ReaderOrErr = llvm::IndexedInstrProfReader::create(Opts.UseProfile);
      if (auto E = ReaderOrErr.takeError()) {
         diagnose(SourceLoc(), diag::profile_read_error, Opts.UseProfile,
                  llvm::toString(std::move(E)));
         Opts.UseProfile.erase();
      }
      M.setPGOReader(std::move(ReaderOrErr.get()));
   }
}

PILGenModule::~PILGenModule() {
   assert(!TopLevelSGF && "active source file lowering!?");
   M.verify();
}

static PILDeclRef
getBridgingFn(Optional<PILDeclRef> &cacheSlot,
              PILGenModule &SGM,
              Identifier moduleName,
              StringRef functionName,
              Optional<std::initializer_list<Type>> inputTypes,
              Optional<Type> outputType) {
   // FIXME: the optionality of outputType and the presence of trustInputTypes
   // are hacks for cases where coming up with those types is complicated, i.e.,
   // when dealing with generic bridging functions.

   if (!cacheSlot) {
      AstContext &ctx = SGM.M.getAstContext();
      ModuleDecl *mod = ctx.getLoadedModule(moduleName);
      if (!mod) {
         SGM.diagnose(SourceLoc(), diag::bridging_module_missing,
                      moduleName.str(), functionName);
         llvm::report_fatal_error("unable to set up the ObjC bridge!");
      }

      SmallVector<ValueDecl *, 2> decls;
      mod->lookupValue(ctx.getIdentifier(functionName),
                       NLKind::QualifiedLookup, decls);
      if (decls.empty()) {
         SGM.diagnose(SourceLoc(), diag::bridging_function_missing,
                      moduleName.str(), functionName);
         llvm::report_fatal_error("unable to set up the ObjC bridge!");
      }
      if (decls.size() != 1) {
         SGM.diagnose(SourceLoc(), diag::bridging_function_overloaded,
                      moduleName.str(), functionName);
         llvm::report_fatal_error("unable to set up the ObjC bridge!");
      }

      auto *fd = dyn_cast<FuncDecl>(decls.front());
      if (!fd) {
         SGM.diagnose(SourceLoc(), diag::bridging_function_not_function,
                      moduleName.str(), functionName);
         llvm::report_fatal_error("unable to set up the ObjC bridge!");
      }

      // Check that the function takes the expected arguments and returns the
      // expected result type.
      PILDeclRef c(fd);
      auto funcTy =
         SGM.Types.getConstantFunctionType(TypeExpansionContext::minimal(), c);
      PILFunctionConventions fnConv(funcTy, SGM.M);

      auto toPILType = [&SGM](Type ty) {
         return SGM.Types.getLoweredType(ty, TypeExpansionContext::minimal());
      };

      if (inputTypes) {
         if (fnConv.hasIndirectPILResults()
             || funcTy->getNumParameters() != inputTypes->size()
             || !std::equal(
            fnConv.getParameterPILTypes().begin(),
            fnConv.getParameterPILTypes().end(),
            makeTransformIterator(inputTypes->begin(), toPILType))) {
            SGM.diagnose(fd->getLoc(), diag::bridging_function_not_correct_type,
                         moduleName.str(), functionName);
            llvm::report_fatal_error("unable to set up the ObjC bridge!");
         }
      }

      if (outputType
          && fnConv.getSinglePILResultType() != toPILType(*outputType)) {
         SGM.diagnose(fd->getLoc(), diag::bridging_function_not_correct_type,
                      moduleName.str(), functionName);
         llvm::report_fatal_error("unable to set up the ObjC bridge!");
      }

      cacheSlot = c;
   }

   LLVM_DEBUG(llvm::dbgs() << "bridging function "
                           << moduleName << '.' << functionName
                           << " mapped to ";
                 cacheSlot->print(llvm::dbgs()));

   return *cacheSlot;
}

#define REQUIRED(X) { Types.get##X##Type() }
#define OPTIONAL(X) { OptionalType::get(Types.get##X##Type()) }
#define GENERIC(X) None

#define GET_BRIDGING_FN(Module, FromKind, FromTy, ToKind, ToTy) \
  PILDeclRef PILGenModule::get##FromTy##To##ToTy##Fn() { \
    return getBridgingFn(FromTy##To##ToTy##Fn, *this, \
                         getAstContext().Id_##Module, \
                         "_convert" #FromTy "To" #ToTy, \
                         FromKind(FromTy), \
                         ToKind(ToTy)); \
  }

GET_BRIDGING_FN(Darwin, REQUIRED, Bool, REQUIRED, DarwinBoolean)
GET_BRIDGING_FN(Darwin, REQUIRED, DarwinBoolean, REQUIRED, Bool)
GET_BRIDGING_FN(ObjectiveC, REQUIRED, Bool, REQUIRED, ObjCBool)
GET_BRIDGING_FN(ObjectiveC, REQUIRED, ObjCBool, REQUIRED, Bool)
GET_BRIDGING_FN(Foundation, OPTIONAL, NSError, REQUIRED, Error)
GET_BRIDGING_FN(Foundation, REQUIRED, Error, REQUIRED, NSError)
GET_BRIDGING_FN(WinSDK, REQUIRED, Bool, REQUIRED, WindowsBool)
GET_BRIDGING_FN(WinSDK, REQUIRED, WindowsBool, REQUIRED, Bool)

#undef GET_BRIDGING_FN
#undef REQUIRED
#undef OPTIONAL
#undef GENERIC

static FuncDecl *diagnoseMissingIntrinsic(PILGenModule &sgm,
                                          PILLocation loc,
                                          const char *name) {
   sgm.diagnose(loc, diag::bridging_function_missing,
                sgm.getAstContext().StdlibModuleName.str(), name);
   return nullptr;
}

#define FUNC_DECL(NAME, ID)                             \
  FuncDecl *PILGenModule::get##NAME(PILLocation loc) {  \
    if (auto fn = getAstContext().get##NAME())   \
      return fn;                                        \
    return diagnoseMissingIntrinsic(*this, loc, ID);    \
  }
#include "polarphp/ast/KnownDeclsDef.h"

InterfaceDecl *PILGenModule::getObjectiveCBridgeable(PILLocation loc) {
   if (ObjectiveCBridgeable)
      return *ObjectiveCBridgeable;

   // Find the _ObjectiveCBridgeable protocol.
   auto &ctx = getAstContext();
   auto proto = ctx.getInterface(KnownInterfaceKind::ObjectiveCBridgeable);
   if (!proto)
      diagnose(loc, diag::bridging_objcbridgeable_missing);

   ObjectiveCBridgeable = proto;
   return proto;
}

FuncDecl *PILGenModule::getBridgeToObjectiveCRequirement(PILLocation loc) {
   if (BridgeToObjectiveCRequirement)
      return *BridgeToObjectiveCRequirement;

   // Find the _ObjectiveCBridgeable protocol.
   auto proto = getObjectiveCBridgeable(loc);
   if (!proto) {
      BridgeToObjectiveCRequirement = nullptr;
      return nullptr;
   }

   // Look for _bridgeToObjectiveC().
   auto &ctx = getAstContext();
   DeclName name(ctx, ctx.Id_bridgeToObjectiveC, llvm::ArrayRef<Identifier>());
   auto *found = dyn_cast_or_null<FuncDecl>(
      proto->getSingleRequirement(name));

   if (!found)
      diagnose(loc, diag::bridging_objcbridgeable_broken, name);

   BridgeToObjectiveCRequirement = found;
   return found;
}

FuncDecl *PILGenModule::getUnconditionallyBridgeFromObjectiveCRequirement(
   PILLocation loc) {
   if (UnconditionallyBridgeFromObjectiveCRequirement)
      return *UnconditionallyBridgeFromObjectiveCRequirement;

   // Find the _ObjectiveCBridgeable protocol.
   auto proto = getObjectiveCBridgeable(loc);
   if (!proto) {
      UnconditionallyBridgeFromObjectiveCRequirement = nullptr;
      return nullptr;
   }

   // Look for _bridgeToObjectiveC().
   auto &ctx = getAstContext();
   DeclName name(ctx, ctx.getIdentifier("_unconditionallyBridgeFromObjectiveC"),
                 llvm::makeArrayRef(Identifier()));
   auto *found = dyn_cast_or_null<FuncDecl>(
      proto->getSingleRequirement(name));

   if (!found)
      diagnose(loc, diag::bridging_objcbridgeable_broken, name);

   UnconditionallyBridgeFromObjectiveCRequirement = found;
   return found;
}

AssociatedTypeDecl *
PILGenModule::getBridgedObjectiveCTypeRequirement(PILLocation loc) {
   if (BridgedObjectiveCType)
      return *BridgedObjectiveCType;

   // Find the _ObjectiveCBridgeable protocol.
   auto proto = getObjectiveCBridgeable(loc);
   if (!proto) {
      BridgeToObjectiveCRequirement = nullptr;
      return nullptr;
   }

   // Look for _bridgeToObjectiveC().
   auto &ctx = getAstContext();
   auto *found = proto->getAssociatedType(ctx.Id_ObjectiveCType);
   if (!found)
      diagnose(loc, diag::bridging_objcbridgeable_broken, ctx.Id_ObjectiveCType);

   BridgedObjectiveCType = found;
   return found;
}

InterfaceConformance *
PILGenModule::getConformanceToObjectiveCBridgeable(PILLocation loc, Type type) {
   auto proto = getObjectiveCBridgeable(loc);
   if (!proto) return nullptr;

   // Find the conformance to _ObjectiveCBridgeable.
   auto result = PolarphpModule->lookupConformance(type, proto);
   if (result.isInvalid())
      return nullptr;

   return result.getConcrete();
}

InterfaceDecl *PILGenModule::getBridgedStoredNSError(PILLocation loc) {
   if (BridgedStoredNSError)
      return *BridgedStoredNSError;

   // Find the _BridgedStoredNSError protocol.
   auto &ctx = getAstContext();
   auto proto = ctx.getInterface(KnownInterfaceKind::BridgedStoredNSError);
   BridgedStoredNSError = proto;
   return proto;
}

VarDecl *PILGenModule::getNSErrorRequirement(PILLocation loc) {
   if (NSErrorRequirement)
      return *NSErrorRequirement;

   // Find the _BridgedStoredNSError protocol.
   auto proto = getBridgedStoredNSError(loc);
   if (!proto) {
      NSErrorRequirement = nullptr;
      return nullptr;
   }

   // Look for _nsError.
   auto &ctx = getAstContext();
   auto *found = dyn_cast_or_null<VarDecl>(
      proto->getSingleRequirement(ctx.Id_nsError));

   NSErrorRequirement = found;
   return found;
}

InterfaceConformanceRef
PILGenModule::getConformanceToBridgedStoredNSError(PILLocation loc, Type type) {
   auto proto = getBridgedStoredNSError(loc);
   if (!proto)
      return InterfaceConformanceRef::forInvalid();

   // Find the conformance to _BridgedStoredNSError.
   return PolarphpModule->lookupConformance(type, proto);
}
// @todo
//
//InterfaceConformance *PILGenModule::getNSErrorConformanceToError() {
//   if (NSErrorConformanceToError)
//      return *NSErrorConformanceToError;
//
//   auto &ctx = getAstContext();
//   auto nsErrorTy = ctx.getNSErrorType();
//   if (!nsErrorTy) {
//      NSErrorConformanceToError = nullptr;
//      return nullptr;
//   }
//
//   auto error = ctx.getErrorDecl();
//   if (!error) {
//      NSErrorConformanceToError = nullptr;
//      return nullptr;
//   }
//
//   auto conformance =
//      PolarphpModule->lookupConformance(nsErrorTy, cast<InterfaceDecl>(error));
//
//   if (conformance.isConcrete())
//      NSErrorConformanceToError = conformance.getConcrete();
//   else
//      NSErrorConformanceToError = nullptr;
//   return *NSErrorConformanceToError;
//}

PILFunction *
PILGenModule::getKeyPathProjectionCoroutine(bool isReadAccess,
                                            KeyPathTypeKind typeKind) {
   bool isBaseInout;
   bool isResultInout;
   StringRef functionName;
   NominalTypeDecl *keyPathDecl;
   if (isReadAccess) {
      assert(typeKind == KPTK_KeyPath ||
             typeKind == KPTK_WritableKeyPath ||
             typeKind == KPTK_ReferenceWritableKeyPath);
      functionName = "swift_readAtKeyPath";
      isBaseInout = false;
      isResultInout = false;
      keyPathDecl = getAstContext().getKeyPathDecl();
   } else if (typeKind == KPTK_WritableKeyPath) {
      functionName = "swift_modifyAtWritableKeyPath";
      isBaseInout = true;
      isResultInout = true;
      keyPathDecl = getAstContext().getWritableKeyPathDecl();
   } else if (typeKind == KPTK_ReferenceWritableKeyPath) {
      functionName = "swift_modifyAtReferenceWritableKeyPath";
      isBaseInout = false;
      isResultInout = true;
      keyPathDecl = getAstContext().getReferenceWritableKeyPathDecl();
   } else {
      llvm_unreachable("bad combination");
   }

   auto fn = M.lookUpFunction(functionName);
   if (fn) return fn;

   auto rootType = CanGenericTypeParamType::get(0, 0, getAstContext());
   auto valueType = CanGenericTypeParamType::get(0, 1, getAstContext());

   // Build the generic signature <A, B>.
   auto sig = GenericSignature::get({rootType, valueType}, {});

   auto keyPathTy = BoundGenericType::get(keyPathDecl, Type(),
                                          { rootType, valueType })
      ->getCanonicalType();

   // (@in_guaranteed/@inout Root, @guaranteed KeyPath<Root, Value>)
   PILParameterInfo params[] = {
      { rootType,
                   isBaseInout ? ParameterConvention::Indirect_Inout
                               : ParameterConvention::Indirect_In_Guaranteed },
      { keyPathTy, ParameterConvention::Direct_Guaranteed },
   };

   // -> @yields @in_guaranteed/@inout Value
   PILYieldInfo yields[] = {
      { valueType,
         isResultInout ? ParameterConvention::Indirect_Inout
                       : ParameterConvention::Indirect_In_Guaranteed },
   };

   auto extInfo = PILFunctionType::ExtInfo(
      PILFunctionTypeRepresentation::Thin,
      /*pseudogeneric*/ false,
      /*non-escaping*/ false,
      DifferentiabilityKind::NonDifferentiable,
      /*clangFunctionType*/ nullptr);

   auto functionTy = PILFunctionType::get(sig, extInfo,
                                          PILCoroutineKind::YieldOnce,
                                          ParameterConvention::Direct_Unowned,
                                          params,
                                          yields,
      /*results*/ {},
      /*error result*/ {},
                                          SubstitutionMap(), false,
                                          getAstContext());

   auto env = sig->getGenericEnvironment();

   PILGenFunctionBuilder builder(*this);
   fn = builder.createFunction(PILLinkage::PublicExternal,
                               functionName,
                               functionTy,
                               env,
      /*location*/ None,
                               IsNotBare,
                               IsNotTransparent,
                               IsNotSerialized,
                               IsNotDynamic);

   return fn;
}


PILFunction *PILGenModule::emitTopLevelFunction(PILLocation Loc) {
   AstContext &C = getAstContext();
   auto extInfo = PILFunctionType::ExtInfo()
      .withRepresentation(PILFunctionType::Representation::CFunctionPointer);

   // Use standard library types if we have them; otherwise, fall back to
   // builtins.
   CanType Int32Ty;
   if (auto Int32Decl = C.getInt32Decl()) {
      Int32Ty = Int32Decl->getDeclaredInterfaceType()->getCanonicalType();
   } else {
      Int32Ty = CanType(BuiltinIntegerType::get(32, C));
   }

   CanType PtrPtrInt8Ty = C.TheRawPointerType;
   if (auto PointerDecl = C.getUnsafeMutablePointerDecl()) {
      if (auto Int8Decl = C.getInt8Decl()) {
         Type Int8Ty = Int8Decl->getDeclaredInterfaceType();
         Type PointerInt8Ty = BoundGenericType::get(PointerDecl,
                                                    nullptr,
                                                    Int8Ty);
         Type OptPointerInt8Ty = OptionalType::get(PointerInt8Ty);
         PtrPtrInt8Ty = BoundGenericType::get(PointerDecl,
                                              nullptr,
                                              OptPointerInt8Ty)
            ->getCanonicalType();
      }
   }

   PILParameterInfo params[] = {
      PILParameterInfo(Int32Ty, ParameterConvention::Direct_Unowned),
      PILParameterInfo(PtrPtrInt8Ty, ParameterConvention::Direct_Unowned),
   };

   CanPILFunctionType topLevelType = PILFunctionType::get(nullptr, extInfo,
                                                          PILCoroutineKind::None,
                                                          ParameterConvention::Direct_Unowned,
                                                          params, /*yields*/ {},
                                                          PILResultInfo(Int32Ty,
                                                                        ResultConvention::Unowned),
                                                          None,
                                                          SubstitutionMap(), false,
                                                          C);

   PILGenFunctionBuilder builder(*this);
   return builder.createFunction(
      PILLinkage::Public, POLAR_ENTRY_POINT_FUNCTION, topLevelType, nullptr,
      Loc, IsBare, IsNotTransparent, IsNotSerialized, IsNotDynamic,
      ProfileCounter(), IsNotThunk, SubclassScope::NotApplicable);
}

PILFunction *PILGenModule::getEmittedFunction(PILDeclRef constant,
                                              ForDefinition_t forDefinition) {
   auto found = emittedFunctions.find(constant);
   if (found != emittedFunctions.end()) {
      PILFunction *F = found->second;
      if (forDefinition) {
         // In all the cases where getConstantLinkage returns something
         // different for ForDefinition, it returns an available-externally
         // linkage.
         if (is_available_externally(F->getLinkage())) {
            F->setLinkage(constant.getLinkage(ForDefinition));
         }
      }
      return F;
   }

   return nullptr;
}

static PILFunction *getFunctionToInsertAfter(PILGenModule &SGM,
                                             PILDeclRef insertAfter) {
   // If the decl ref was emitted, emit after its function.
   while (insertAfter) {
      auto found = SGM.emittedFunctions.find(insertAfter);
      if (found != SGM.emittedFunctions.end()) {
         return found->second;
      }

      // Otherwise, try to insert after the function we would be transitively
      // be inserted after.
      auto foundDelayed = SGM.delayedFunctions.find(insertAfter);
      if (foundDelayed != SGM.delayedFunctions.end()) {
         insertAfter = foundDelayed->second.insertAfter;
      } else {
         break;
      }
   }

   // If the decl ref is nil, just insert at the beginning.
   return nullptr;
}

static bool haveProfiledAssociatedFunction(PILDeclRef constant) {
   return constant.isDefaultArgGenerator() || constant.isForeign ||
          constant.isCurried;
}

/// Set up the function for profiling instrumentation.
static void setUpForProfiling(PILDeclRef constant, PILFunction *F,
                              ForDefinition_t forDefinition) {
   if (!forDefinition)
      return;

   AstNode profiledNode;
   if (!haveProfiledAssociatedFunction(constant)) {
      if (constant.hasDecl()) {
         if (auto *fd = constant.getFuncDecl()) {
            if (fd->hasBody()) {
               F->createProfiler(fd, constant, forDefinition);
               profiledNode = fd->getBody(/*canSynthesize=*/false);
            }
         }
      } else if (auto *ace = constant.getAbstractClosureExpr()) {
         F->createProfiler(ace, constant, forDefinition);
         profiledNode = ace;
      }
      // Set the function entry count for PGO.
      if (PILProfiler *SP = F->getProfiler())
         F->setEntryCount(SP->getExecutionCount(profiledNode));
   }
}

static bool isEmittedOnDemand(PILModule &M, PILDeclRef constant) {
   if (!constant.hasDecl())
      return false;

   if (constant.isCurried ||
       constant.isForeign ||
       constant.isDirectReference)
      return false;

   auto *d = constant.getDecl();
   auto *dc = d->getDeclContext()->getModuleScopeContext();

   if (isa<ClangModuleUnit>(dc))
      return true;

   if (auto *func = dyn_cast<FuncDecl>(d))
      if (func->hasForcedStaticDispatch())
         return true;

   if (auto *sf = dyn_cast<SourceFile>(dc))
      if (M.isWholeModule() || M.getAssociatedContext() == dc)
         return false;

   return false;
}

PILFunction *PILGenModule::getFunction(PILDeclRef constant,
                                       ForDefinition_t forDefinition) {
   // If we already emitted the function, return it (potentially preparing it
   // for definition).
   if (auto emitted = getEmittedFunction(constant, forDefinition)) {
      setUpForProfiling(constant, emitted, forDefinition);
      return emitted;
   }

   // Note: Do not provide any PILLocation. You can set it afterwards.
   PILGenFunctionBuilder builder(*this);
   auto *F = builder.getOrCreateFunction(constant.hasDecl() ? constant.getDecl()
                                                            : (Decl *)nullptr,
      constant, forDefinition);
   setUpForProfiling(constant, F, forDefinition);

   assert(F && "PILFunction should have been defined");

   emittedFunctions[constant] = F;

   if (isEmittedOnDemand(M, constant) &&
       !delayedFunctions.count(constant)) {
      auto *d = constant.getDecl();
      if (auto *func = dyn_cast<FuncDecl>(d)) {
         if (constant.kind == PILDeclRef::Kind::Func)
            emitFunction(func);
      } else if (auto *ctor = dyn_cast<ConstructorDecl>(d)) {
         // For factories, we don't need to emit a special thunk; the normal
         // foreign-to-native thunk is sufficient.
         if (!ctor->isFactoryInit() &&
             constant.kind == PILDeclRef::Kind::Allocator)
            emitConstructor(ctor);
      }
   }

   // If we delayed emitting this function previously, we need it now.
   auto foundDelayed = delayedFunctions.find(constant);
   if (foundDelayed != delayedFunctions.end()) {
      // Move the function to its proper place within the module.
      M.functions.remove(F);
      PILFunction *insertAfter = getFunctionToInsertAfter(*this,
                                                          foundDelayed->second.insertAfter);
      if (!insertAfter) {
         M.functions.push_front(F);
      } else {
         M.functions.insertAfter(insertAfter->getIterator(), F);
      }

      forcedFunctions.push_back(*foundDelayed);
      delayedFunctions.erase(foundDelayed);
   } else {
      // We would have registered a delayed function as "last emitted" when we
      // enqueued. If the function wasn't delayed, then we're emitting it now.
      lastEmittedFunction = constant;
   }

   return F;
}

bool PILGenModule::hasFunction(PILDeclRef constant) {
   return emittedFunctions.count(constant);
}

void PILGenModule::visitFuncDecl(FuncDecl *fd) { emitFunction(fd); }

/// Emit a function now, if it's externally usable or has been referenced in
/// the current TU, or remember how to emit it later if not.
template<typename /*void (PILFunction*)*/ Fn>
static void emitOrDelayFunction(PILGenModule &SGM,
                                PILDeclRef constant,
                                Fn &&emitter,
                                bool forceEmission = false) {
   auto emitAfter = SGM.lastEmittedFunction;

   PILFunction *f = nullptr;

   // If the function is explicit or may be externally referenced, or if we're
   // forcing emission, we must emit it.
   bool mayDelay;
   // Shared thunks and Clang-imported definitions can always be delayed.
   if (constant.isThunk() || constant.isClangImported()) {
      mayDelay = !forceEmission;
      // Implicit decls may be delayed if they can't be used externally.
   } else {
      auto linkage = constant.getLinkage(ForDefinition);
      mayDelay = !forceEmission &&
                 (constant.isImplicit() &&
                  !isPossiblyUsedExternally(linkage, SGM.M.isWholeModule()));
   }

   // Avoid emitting a delayable definition if it hasn't already been referenced.
   if (mayDelay)
      f = SGM.getEmittedFunction(constant, ForDefinition);
   else
      f = SGM.getFunction(constant, ForDefinition);

   // If we don't want to emit now, remember how for later.
   if (!f) {
      SGM.delayedFunctions.insert({constant, {emitAfter,
                                              std::forward<Fn>(emitter)}});
      // Even though we didn't emit the function now, update the
      // lastEmittedFunction so that we preserve the original ordering that
      // the symbols would have been emitted in.
      SGM.lastEmittedFunction = constant;
      return;
   }

   emitter(f);
}

void PILGenModule::preEmitFunction(PILDeclRef constant,
                                   llvm::PointerUnion<ValueDecl *,
                                      Expr *> astNode,
                                   PILFunction *F,
                                   PILLocation Loc) {
   // By default, use the astNode to create the location.
   if (Loc.isNull()) {
      if (auto *decl = astNode.get<ValueDecl *>())
         Loc = RegularLocation(decl);
      else
         Loc = RegularLocation(astNode.get<Expr *>());
   }

   assert(F->empty() && "already emitted function?!");

   if (F->getLoweredFunctionType()->isPolymorphic())
      F->setGenericEnvironment(Types.getConstantGenericEnvironment(constant));

   // Create a debug scope for the function using astNode as source location.
   F->setDebugScope(new (M) PILDebugScope(Loc, F));

   LLVM_DEBUG(llvm::dbgs() << "lowering ";
                 F->printName(llvm::dbgs());
                 llvm::dbgs() << " : ";
                 F->getLoweredType().print(llvm::dbgs());
                 llvm::dbgs() << '\n';
                 if (astNode) {
                    if (auto *decl = astNode.dyn_cast<ValueDecl *>()) {
                       decl->dump(llvm::dbgs());
                    } else {
                       astNode.get<Expr *>()->dump(llvm::dbgs());
                       llvm::dbgs() << "\n";
                    }
                    llvm::dbgs() << '\n';
                 });
}

void PILGenModule::postEmitFunction(PILDeclRef constant,
                                    PILFunction *F) {
   emitLazyConformancesForFunction(F);

   assert(!F->isExternalDeclaration() && "did not emit any function body?!");
   LLVM_DEBUG(llvm::dbgs() << "lowered sil:\n";
                 F->print(llvm::dbgs()));
   F->verify();
}

void PILGenModule::
emitMarkFunctionEscapeForTopLevelCodeGlobals(PILLocation loc,
                                             CaptureInfo captureInfo) {
   assert(TopLevelSGF && TopLevelSGF->B.hasValidInsertionPoint()
          && "no valid code generator for top-level function?!");

   SmallVector<PILValue, 4> Captures;

   for (auto capture : captureInfo.getCaptures()) {
      // Decls captured by value don't escape.
      auto It = TopLevelSGF->VarLocs.find(capture.getDecl());
      if (It == TopLevelSGF->VarLocs.end() ||
          !It->getSecond().value->getType().isAddress())
         continue;

      Captures.push_back(It->second.value);
   }

   if (!Captures.empty())
      TopLevelSGF->B.createMarkFunctionEscape(loc, Captures);
}

void PILGenModule::emitAbstractFuncDecl(AbstractFunctionDecl *AFD) {
   // Emit any default argument generators.
   emitDefaultArgGenerators(AFD, AFD->getParameters());

   // If this is a function at global scope, it may close over a global variable.
   // If we're emitting top-level code, then emit a "mark_function_escape" that
   // lists the captured global variables so that definite initialization can
   // reason about this escape point.
   if (!AFD->getDeclContext()->isLocalContext() &&
       TopLevelSGF && TopLevelSGF->B.hasValidInsertionPoint()) {
      emitMarkFunctionEscapeForTopLevelCodeGlobals(AFD, AFD->getCaptureInfo());
   }

   // If the declaration is exported as a C function, emit its native-to-foreign
   // thunk too, if it wasn't already forced.
   if (AFD->getAttrs().hasAttribute<CDeclAttr>()) {
      auto thunk = PILDeclRef(AFD).asForeign();
      if (!hasFunction(thunk))
         emitNativeToForeignThunk(thunk);
   }
}

void PILGenModule::emitFunction(FuncDecl *fd) {
   PILDeclRef::Loc decl = fd;

   emitAbstractFuncDecl(fd);

   if (fd->hasBody()) {
      FrontendStatsTracer Tracer(getAstContext().Stats, "PILGen-funcdecl", fd);
      PrettyStackTraceDecl stackTrace("emitting PIL for", fd);

      PILDeclRef constant(decl);

      bool ForCoverageMapping = doesAstRequireProfiling(M, fd);

      emitOrDelayFunction(*this, constant, [this,constant,fd](PILFunction *f){
         preEmitFunction(constant, fd, f, fd);
         PrettyStackTracePILFunction X("silgen emitFunction", f);
         PILGenFunction(*this, *f, fd).emitFunction(fd);
         postEmitFunction(constant, f);
      }, /*forceEmission=*/ForCoverageMapping);
   }
}

void PILGenModule::addGlobalVariable(VarDecl *global) {
   // We create PILGlobalVariable here.
   getPILGlobalVariable(global, ForDefinition);
}

void PILGenModule::emitConstructor(ConstructorDecl *decl) {
   // FIXME: Handle 'self' like any other argument here.
   // Emit any default argument getter functions.
   emitAbstractFuncDecl(decl);

   // We never emit constructors in protocols.
   if (isa<InterfaceDecl>(decl->getDeclContext()))
      return;

   // Always-unavailable imported constructors are factory methods
   // that have been imported as constructors and then hidden by an
   // imported init method.
   if (decl->hasClangNode() &&
       decl->getAttrs().isUnavailable(decl->getAstContext()))
      return;

   PILDeclRef constant(decl);
   DeclContext *declCtx = decl->getDeclContext();

   bool ForCoverageMapping = doesAstRequireProfiling(M, decl);

   if (declCtx->getSelfClassDecl()) {
      // Designated initializers for classes, as well as @objc convenience
      // initializers, have have separate entry points for allocation and
      // initialization.
      // @todo
      if (decl->isDesignatedInit() /*|| decl->isObjC()*/) {
         emitOrDelayFunction(
            *this, constant, [this, constant, decl](PILFunction *f) {
               preEmitFunction(constant, decl, f, decl);
               PrettyStackTracePILFunction X("silgen emitConstructor", f);
               PILGenFunction(*this, *f, decl).emitClassConstructorAllocator(decl);
               postEmitFunction(constant, f);
            });

         // Constructors may not have bodies if they've been imported, or if they've
         // been parsed from a module interface.
         if (decl->hasBody()) {
            PILDeclRef initConstant(decl, PILDeclRef::Kind::Initializer);
            emitOrDelayFunction(
               *this, initConstant,
               [this, initConstant, decl](PILFunction *initF) {
                  preEmitFunction(initConstant, decl, initF, decl);
                  PrettyStackTracePILFunction X("silgen constructor initializer",
                                                initF);
                  initF->createProfiler(decl, initConstant, ForDefinition);
                  PILGenFunction(*this, *initF, decl)
                     .emitClassConstructorInitializer(decl);
                  postEmitFunction(initConstant, initF);
               },
               /*forceEmission=*/ForCoverageMapping);
         }
         return;
      }
   }

   // Struct and enum constructors do everything in a single function, as do
   // non-@objc convenience initializers for classes.
   if (decl->hasBody()) {
      emitOrDelayFunction(
         *this, constant, [this, constant, decl](PILFunction *f) {
            preEmitFunction(constant, decl, f, decl);
            PrettyStackTracePILFunction X("silgen emitConstructor", f);
            f->createProfiler(decl, constant, ForDefinition);
            PILGenFunction(*this, *f, decl).emitValueConstructor(decl);
            postEmitFunction(constant, f);
         });
   }
}

void PILGenModule::emitEnumConstructor(EnumElementDecl *decl) {
   // Enum element constructors are always emitted by need, so don't need
   // delayed emission.
   PILDeclRef constant(decl);
   PILFunction *f = getFunction(constant, ForDefinition);
   preEmitFunction(constant, decl, f, decl);
   PrettyStackTracePILFunction X("silgen enum constructor", f);
   PILGenFunction(*this, *f, decl->getDeclContext()).emitEnumConstructor(decl);
   postEmitFunction(constant, f);
}

PILFunction *PILGenModule::emitClosure(AbstractClosureExpr *ce) {
   PILDeclRef constant(ce);
   PILFunction *f = getFunction(constant, ForDefinition);

   // Generate the closure function, if we haven't already.
   //
   // We may visit the same closure expr multiple times in some cases,
   // for instance, when closures appear as in-line initializers of stored
   // properties. In these cases the closure will be emitted into every
   // initializer of the containing type.
   if (!f->isExternalDeclaration())
      return f;
   preEmitFunction(constant, ce, f, ce);
   PrettyStackTracePILFunction X("silgen closureexpr", f);
   PILGenFunction(*this, *f, ce).emitClosure(ce);
   postEmitFunction(constant, f);
   return f;
}

/// Determine whether the given class requires a separate instance
/// variable initialization method.
static bool requiresIVarInitialization(PILGenModule &SGM, ClassDecl *cd) {
   if (!cd->requiresStoredPropertyInits())
      return false;

   for (Decl *member : cd->getMembers()) {
      auto pbd = dyn_cast<PatternBindingDecl>(member);
      if (!pbd) continue;

      for (auto i : range(pbd->getNumPatternEntries()))
         if (pbd->getExecutableInit(i))
            return true;
   }

   return false;
}

bool PILGenModule::hasNonTrivialIVars(ClassDecl *cd) {
   for (Decl *member : cd->getMembers()) {
      auto *vd = dyn_cast<VarDecl>(member);
      if (!vd || !vd->hasStorage()) continue;

      auto &ti = Types.getTypeLowering(
         vd->getType(), TypeExpansionContext::maximalResilienceExpansionOnly());
      if (!ti.isTrivial())
         return true;
   }

   return false;
}

bool PILGenModule::requiresIVarDestroyer(ClassDecl *cd) {
   // Only needed if we have non-trivial ivars, we're not a root class, and
   // the superclass is not @objc.
   return (hasNonTrivialIVars(cd) &&
           cd->getSuperclass() &&
           !cd->getSuperclass()->getClassOrBoundGenericClass()->hasClangNode());
}

/// TODO: This needs a better name.
void PILGenModule::emitObjCAllocatorDestructor(ClassDecl *cd,
                                               DestructorDecl *dd) {
   // Emit the native deallocating destructor for -dealloc.
   // Destructors are a necessary part of class metadata, so can't be delayed.
   if (dd->hasBody()) {
      PILDeclRef dealloc(dd, PILDeclRef::Kind::Deallocator);
      PILFunction *f = getFunction(dealloc, ForDefinition);
      preEmitFunction(dealloc, dd, f, dd);
      PrettyStackTracePILFunction X("silgen emitDestructor -dealloc", f);
      f->createProfiler(dd, dealloc, ForDefinition);
      PILGenFunction(*this, *f, dd).emitObjCDestructor(dealloc);
      postEmitFunction(dealloc, f);
   }

   // Emit the Objective-C -dealloc entry point if it has
   // something to do beyond messaging the superclass's -dealloc.
   if (dd->hasBody() && !dd->getBody()->empty())
      emitObjCDestructorThunk(dd);

   // Emit the ivar initializer, if needed.
   if (requiresIVarInitialization(*this, cd)) {
      auto ivarInitializer = PILDeclRef(cd, PILDeclRef::Kind::IVarInitializer)
         .asForeign();
      PILFunction *f = getFunction(ivarInitializer, ForDefinition);
      preEmitFunction(ivarInitializer, dd, f, dd);
      PrettyStackTracePILFunction X("silgen emitDestructor ivar initializer", f);
      PILGenFunction(*this, *f, cd).emitIVarInitializer(ivarInitializer);
      postEmitFunction(ivarInitializer, f);
   }

   // Emit the ivar destroyer, if needed.
   if (hasNonTrivialIVars(cd)) {
      auto ivarDestroyer = PILDeclRef(cd, PILDeclRef::Kind::IVarDestroyer)
         .asForeign();
      PILFunction *f = getFunction(ivarDestroyer, ForDefinition);
      preEmitFunction(ivarDestroyer, dd, f, dd);
      PrettyStackTracePILFunction X("silgen emitDestructor ivar destroyer", f);
      PILGenFunction(*this, *f, cd).emitIVarDestroyer(ivarDestroyer);
      postEmitFunction(ivarDestroyer, f);
   }
}

void PILGenModule::emitDestructor(ClassDecl *cd, DestructorDecl *dd) {
   emitAbstractFuncDecl(dd);

   // Emit the ivar destroyer, if needed.
   if (requiresIVarDestroyer(cd)) {
      PILDeclRef ivarDestroyer(cd, PILDeclRef::Kind::IVarDestroyer);
      PILFunction *f = getFunction(ivarDestroyer, ForDefinition);
      preEmitFunction(ivarDestroyer, dd, f, dd);
      PrettyStackTracePILFunction X("silgen emitDestructor ivar destroyer", f);
      PILGenFunction(*this, *f, dd).emitIVarDestroyer(ivarDestroyer);
      postEmitFunction(ivarDestroyer, f);
   }

   // If the class would use the Objective-C allocator, only emit -dealloc.
   if (usesObjCAllocator(cd)) {
      emitObjCAllocatorDestructor(cd, dd);
      return;
   }

   // Emit the destroying destructor.
   // Destructors are a necessary part of class metadata, so can't be delayed.
   if (dd->hasBody()) {
      PILDeclRef destroyer(dd, PILDeclRef::Kind::Destroyer);
      PILFunction *f = getFunction(destroyer, ForDefinition);
      preEmitFunction(destroyer, dd, f, dd);
      PrettyStackTracePILFunction X("silgen emitDestroyingDestructor", f);
      PILGenFunction(*this, *f, dd).emitDestroyingDestructor(dd);
      f->setDebugScope(new (M) PILDebugScope(dd, f));
      postEmitFunction(destroyer, f);
   }

   // Emit the deallocating destructor.
   {
      PILDeclRef deallocator(dd, PILDeclRef::Kind::Deallocator);
      PILFunction *f = getFunction(deallocator, ForDefinition);
      preEmitFunction(deallocator, dd, f, dd);
      PrettyStackTracePILFunction X("silgen emitDeallocatingDestructor", f);
      f->createProfiler(dd, deallocator, ForDefinition);
      PILGenFunction(*this, *f, dd).emitDeallocatingDestructor(dd);
      f->setDebugScope(new (M) PILDebugScope(dd, f));
      postEmitFunction(deallocator, f);
   }
}

void PILGenModule::emitDefaultArgGenerator(PILDeclRef constant,
                                           ParamDecl *param) {
   auto initDC = param->getDefaultArgumentInitContext();

   switch (param->getDefaultArgumentKind()) {
      case DefaultArgumentKind::None:
         llvm_unreachable("No default argument here?");

      case DefaultArgumentKind::Normal: {
         auto arg = param->getTypeCheckedDefaultExpr();
         emitOrDelayFunction(*this, constant,
                             [this,constant,arg,initDC](PILFunction *f) {
                                preEmitFunction(constant, arg, f, arg);
                                PrettyStackTracePILFunction X("silgen emitDefaultArgGenerator ", f);
                                PILGenFunction SGF(*this, *f, initDC);
                                SGF.emitGeneratorFunction(constant, arg);
                                postEmitFunction(constant, f);
                             });
         return;
      }

      case DefaultArgumentKind::StoredProperty: {
         auto arg = param->getStoredProperty();
         emitOrDelayFunction(*this, constant,
                             [this,constant,arg,initDC](PILFunction *f) {
                                preEmitFunction(constant, arg, f, arg);
                                PrettyStackTracePILFunction X("silgen emitDefaultArgGenerator ", f);
                                PILGenFunction SGF(*this, *f, initDC);
                                SGF.emitGeneratorFunction(constant, arg);
                                postEmitFunction(constant, f);
                             });
         return;
      }

      case DefaultArgumentKind::Inherited:
      case DefaultArgumentKind::Column:
      case DefaultArgumentKind::File:
      case DefaultArgumentKind::Line:
      case DefaultArgumentKind::Function:
      case DefaultArgumentKind::DSOHandle:
      case DefaultArgumentKind::NilLiteral:
      case DefaultArgumentKind::EmptyArray:
      case DefaultArgumentKind::EmptyDictionary:
         return;
   }
}

void PILGenModule::
emitStoredPropertyInitialization(PatternBindingDecl *pbd, unsigned i) {
   auto *var = pbd->getAnchoringVarDecl(i);
   auto *init = pbd->getInit(i);
   auto *initDC = pbd->getInitContext(i);
   auto captureInfo = pbd->getCaptureInfo(i);
   assert(!pbd->isInitializerSubsumed(i));

   // If this is the backing storage for a property with an attached wrapper
   // that was initialized with `=`, use that expression as the initializer.
   if (auto originalProperty = var->getOriginalWrappedProperty()) {
      if (originalProperty
         ->isPropertyMemberwiseInitializedWithWrappedType()) {
         auto wrapperInfo =
            originalProperty->getPropertyWrapperBackingPropertyInfo();
         if (wrapperInfo.originalInitialValue)
            init = wrapperInfo.originalInitialValue;
      }
   }

   PILDeclRef constant(var, PILDeclRef::Kind::StoredPropertyInitializer);
   emitOrDelayFunction(*this, constant,
                       [this,var,captureInfo,constant,init,initDC](PILFunction *f) {
                          preEmitFunction(constant, init, f, init);
                          PrettyStackTracePILFunction X("silgen emitStoredPropertyInitialization", f);
                          f->createProfiler(init, constant, ForDefinition);
                          PILGenFunction SGF(*this, *f, initDC);

                          // If this is a stored property initializer inside a type at global scope,
                          // it may close over a global variable. If we're emitting top-level code,
                          // then emit a "mark_function_escape" that lists the captured global
                          // variables so that definite initialization can reason about this
                          // escape point.
                          if (!var->getDeclContext()->isLocalContext() &&
                              TopLevelSGF && TopLevelSGF->B.hasValidInsertionPoint()) {
                             emitMarkFunctionEscapeForTopLevelCodeGlobals(var, captureInfo);
                          }

                          SGF.emitGeneratorFunction(constant, init, /*EmitProfilerIncrement=*/true);
                          postEmitFunction(constant, f);
                       });
}

void PILGenModule::
emitPropertyWrapperBackingInitializer(VarDecl *var) {
   PILDeclRef constant(var, PILDeclRef::Kind::PropertyWrapperBackingInitializer);
   emitOrDelayFunction(*this, constant, [this, constant, var](PILFunction *f) {
      preEmitFunction(constant, var, f, var);
      PrettyStackTracePILFunction X(
         "silgen emitPropertyWrapperBackingInitializer", f);
      auto wrapperInfo = var->getPropertyWrapperBackingPropertyInfo();
      assert(wrapperInfo.initializeFromOriginal);
      f->createProfiler(wrapperInfo.initializeFromOriginal, constant,
                        ForDefinition);
      auto varDC = var->getInnermostDeclContext();
      PILGenFunction SGF(*this, *f, varDC);
      SGF.emitGeneratorFunction(constant, wrapperInfo.initializeFromOriginal);
      postEmitFunction(constant, f);
   });
}

PILFunction *PILGenModule::emitLazyGlobalInitializer(StringRef funcName,
                                                     PatternBindingDecl *binding,
                                                     unsigned pbdEntry) {
   AstContext &C = M.getAstContext();
   auto *onceBuiltin =
      cast<FuncDecl>(getBuiltinValueDecl(C, C.getIdentifier("once")));
   auto blockParam = onceBuiltin->getParameters()->get(1);
   auto *type = blockParam->getType()->castTo<FunctionType>();
   Type initType = FunctionType::get({}, TupleType::getEmpty(C),
                                     type->getExtInfo());
   auto initPILType = cast<PILFunctionType>(
      Types.getLoweredRValueType(TypeExpansionContext::minimal(), initType));

   PILGenFunctionBuilder builder(*this);
   auto *f = builder.createFunction(
      PILLinkage::Private, funcName, initPILType, nullptr, PILLocation(binding),
      IsNotBare, IsNotTransparent, IsNotSerialized, IsNotDynamic);
   f->setDebugScope(new (M) PILDebugScope(RegularLocation(binding), f));
   auto dc = binding->getDeclContext();
   PILGenFunction(*this, *f, dc).emitLazyGlobalInitializer(binding, pbdEntry);
   emitLazyConformancesForFunction(f);
   f->verify();

   return f;
}

void PILGenModule::emitGlobalAccessor(VarDecl *global,
                                      PILGlobalVariable *onceToken,
                                      PILFunction *onceFunc) {
   PILDeclRef accessor(global, PILDeclRef::Kind::GlobalAccessor);
   emitOrDelayFunction(*this, accessor,
                       [this,accessor,global,onceToken,onceFunc](PILFunction *f){
                          preEmitFunction(accessor, global, f, global);
                          PrettyStackTracePILFunction X("silgen emitGlobalAccessor", f);
                          PILGenFunction(*this, *f, global->getDeclContext())
                             .emitGlobalAccessor(global, onceToken, onceFunc);
                          postEmitFunction(accessor, f);
                       });
}

void PILGenModule::emitDefaultArgGenerators(PILDeclRef::Loc decl,
                                            ParameterList *paramList) {
   unsigned index = 0;
   for (auto param : *paramList) {
      if (param->isDefaultArgument())
         emitDefaultArgGenerator(PILDeclRef::getDefaultArgGenerator(decl, index),
                                 param);
      ++index;
   }
}

void PILGenModule::emitObjCMethodThunk(FuncDecl *method) {
   auto thunk = PILDeclRef(method).asForeign();

   // Don't emit the thunk if it already exists.
   if (hasFunction(thunk))
      return;

   // ObjC entry points are always externally usable, so can't be delay-emitted.

   PILFunction *f = getFunction(thunk, ForDefinition);
   preEmitFunction(thunk, method, f, method);
   PrettyStackTracePILFunction X("silgen emitObjCMethodThunk", f);
   f->setBare(IsBare);
   f->setThunk(IsThunk);
   PILGenFunction(*this, *f, method).emitNativeToForeignThunk(thunk);
   postEmitFunction(thunk, f);
}

void PILGenModule::emitObjCPropertyMethodThunks(AbstractStorageDecl *prop) {
   auto *getter = prop->getOpaqueAccessor(AccessorKind::Get);

   // If we don't actually need an entry point for the getter, do nothing.
   // @todo
   if (!getter /*|| !requiresObjCMethodEntryPoint(getter) */)
      return;

   auto getterRef = PILDeclRef(getter, PILDeclRef::Kind::Func).asForeign();

   // Don't emit the thunks if they already exist.
   if (hasFunction(getterRef))
      return;

   RegularLocation ThunkBodyLoc(prop);
   ThunkBodyLoc.markAutoGenerated();
   // ObjC entry points are always externally usable, so emitting can't be
   // delayed.
   {
      PILFunction *f = getFunction(getterRef, ForDefinition);
      preEmitFunction(getterRef, prop, f, ThunkBodyLoc);
      PrettyStackTracePILFunction X("silgen objc property getter thunk", f);
      f->setBare(IsBare);
      f->setThunk(IsThunk);
      PILGenFunction(*this, *f, getter).emitNativeToForeignThunk(getterRef);
      postEmitFunction(getterRef, f);
   }

   if (!prop->isSettable(prop->getDeclContext()))
      return;

   // FIXME: Add proper location.
   auto *setter = prop->getOpaqueAccessor(AccessorKind::Set);
   auto setterRef = PILDeclRef(setter, PILDeclRef::Kind::Func).asForeign();

   PILFunction *f = getFunction(setterRef, ForDefinition);
   preEmitFunction(setterRef, prop, f, ThunkBodyLoc);
   PrettyStackTracePILFunction X("silgen objc property setter thunk", f);
   f->setBare(IsBare);
   f->setThunk(IsThunk);
   PILGenFunction(*this, *f, setter).emitNativeToForeignThunk(setterRef);
   postEmitFunction(setterRef, f);
}

void PILGenModule::emitObjCConstructorThunk(ConstructorDecl *constructor) {
   auto thunk = PILDeclRef(constructor, PILDeclRef::Kind::Initializer)
      .asForeign();

   // Don't emit the thunk if it already exists.
   if (hasFunction(thunk))
      return;
   // ObjC entry points are always externally usable, so emitting can't be
   // delayed.

   PILFunction *f = getFunction(thunk, ForDefinition);
   preEmitFunction(thunk, constructor, f, constructor);
   PrettyStackTracePILFunction X("silgen objc constructor thunk", f);
   f->setBare(IsBare);
   f->setThunk(IsThunk);
   PILGenFunction(*this, *f, constructor).emitNativeToForeignThunk(thunk);
   postEmitFunction(thunk, f);
}

void PILGenModule::emitObjCDestructorThunk(DestructorDecl *destructor) {
   auto thunk = PILDeclRef(destructor, PILDeclRef::Kind::Deallocator)
      .asForeign();

   // Don't emit the thunk if it already exists.
   if (hasFunction(thunk))
      return;
   PILFunction *f = getFunction(thunk, ForDefinition);
   preEmitFunction(thunk, destructor, f, destructor);
   PrettyStackTracePILFunction X("silgen objc destructor thunk", f);
   f->setBare(IsBare);
   f->setThunk(IsThunk);
   PILGenFunction(*this, *f, destructor).emitNativeToForeignThunk(thunk);
   postEmitFunction(thunk, f);
}

void PILGenModule::visitPatternBindingDecl(PatternBindingDecl *pd) {
   assert(!TopLevelSGF && "script mode PBDs should be in TopLevelCodeDecls");
   for (auto i : range(pd->getNumPatternEntries()))
      if (pd->getExecutableInit(i))
         emitGlobalInitialization(pd, i);
}

void PILGenModule::visitVarDecl(VarDecl *vd) {
   if (vd->hasStorage())
      addGlobalVariable(vd);

   vd->visitEmittedAccessors([&](AccessorDecl *accessor) {
      emitFunction(accessor);
   });

   tryEmitPropertyDescriptor(vd);
}

void PILGenModule::visitSubscriptDecl(SubscriptDecl *sd) {
   llvm_unreachable("top-level subscript?");
}

bool
PILGenModule::canStorageUseStoredKeyPathComponent(AbstractStorageDecl *decl,
                                                  ResilienceExpansion expansion) {
   // If the declaration is resilient, we have to treat the component as
   // computed.
   if (decl->isResilient(M.getTypePHPModule(), expansion))
      return false;

   auto strategy = decl->getAccessStrategy(AccessSemantics::Ordinary,
                                           decl->supportsMutation()
                                           ? AccessKind::ReadWrite
                                           : AccessKind::Read,
                                           M.getTypePHPModule(),
                                           expansion);
   switch (strategy.getKind()) {
      case AccessStrategy::Storage: {
         // Keypaths rely on accessors to handle the special behavior of weak or
         // unowned properties.
         if (decl->getInterfaceType()->is<ReferenceStorageType>())
            return false;
         // If the stored value would need to be reabstracted in fully opaque
         // context, then we have to treat the component as computed.
         auto componentObjTy = decl->getValueInterfaceType();
         if (auto genericEnv =
            decl->getInnermostDeclContext()->getGenericEnvironmentOfContext())
            componentObjTy = genericEnv->mapTypeIntoContext(componentObjTy);
         auto storageTy = M.Types.getSubstitutedStorageType(
            TypeExpansionContext::minimal(), decl, componentObjTy);
         auto opaqueTy = M.Types.getLoweredRValueType(
            TypeExpansionContext::noOpaqueTypeArchetypesSubstitution(expansion),
            AbstractionPattern::getOpaque(), componentObjTy);

         return storageTy.getAstType() == opaqueTy;
      }
      case AccessStrategy::DirectToAccessor:
      case AccessStrategy::DispatchToAccessor:
      case AccessStrategy::MaterializeToTemporary:
         return false;
   }
   llvm_unreachable("unhandled strategy");
}

static bool canStorageUseTrivialDescriptor(PILGenModule &SGM,
                                           AbstractStorageDecl *decl) {
   // A property can use a trivial property descriptor if the key path component
   // that an external module would form given publicly-exported information
   // about the property is never equivalent to the canonical component for the
   // key path.
   // This means that the property isn't stored (without promising to be always
   // stored) and doesn't have a setter with less-than-public visibility.
   auto expansion = ResilienceExpansion::Maximal;

   if (!SGM.M.getTypePHPModule()->isResilient()) {
      if (SGM.canStorageUseStoredKeyPathComponent(decl, expansion)) {
         // External modules can't directly access storage, unless this is a
         // property in a fixed-layout type.
         return !decl->isFormallyResilient();
      }

      // If the type is computed and doesn't have a setter that's hidden from
      // the public, then external components can form the canonical key path
      // without our help.
      auto *setter = decl->getOpaqueAccessor(AccessorKind::Set);
      if (!setter)
         return true;

      if (setter->getFormalAccessScope(nullptr, true).isPublic())
         return true;

      return false;
   }

   // A resilient module needs to handle binaries compiled against its older
   // versions. This means we have to be a bit more conservative, since in
   // earlier versions, a settable property may have withheld the setter,
   // or a fixed-layout type may not have been.
   // Without availability information, only get-only computed properties
   // can resiliently use trivial descriptors.
   return (!SGM.canStorageUseStoredKeyPathComponent(decl, expansion) &&
           !decl->supportsMutation());
}

void PILGenModule::tryEmitPropertyDescriptor(AbstractStorageDecl *decl) {
   // TODO: Key path code emission doesn't handle opaque values properly yet.
   if (!PILModuleConventions(M).useLoweredAddresses())
      return;

   if (!decl->exportsPropertyDescriptor())
      return;

   PrettyStackTraceDecl stackTrace("emitting property descriptor for", decl);

   Type baseTy;
   if (decl->getDeclContext()->isTypeContext()) {
      // TODO: Static properties should eventually be referenceable as
      // keypaths from T.Type -> Element, viz `baseTy = MetatypeType::get(baseTy)`
      assert(!decl->isStatic());

      baseTy = decl->getDeclContext()->getSelfInterfaceType()
         ->getCanonicalType(decl->getInnermostDeclContext()
                               ->getGenericSignatureOfContext());
   } else {
      // TODO: Global variables should eventually be referenceable as
      // key paths from (), viz. baseTy = TupleType::getEmpty(getAstContext());
      llvm_unreachable("should not export a property descriptor yet");
   }

   auto genericEnv = decl->getInnermostDeclContext()
      ->getGenericEnvironmentOfContext();
   unsigned baseOperand = 0;
   bool needsGenericContext = true;

   if (canStorageUseTrivialDescriptor(*this, decl)) {
      (void)PILProperty::create(M, /*serialized*/ false, decl, None);
      return;
   }

   SubstitutionMap subs;
   if (genericEnv)
      subs = genericEnv->getForwardingSubstitutionMap();

   auto component = emitKeyPathComponentForDecl(PILLocation(decl),
                                                genericEnv,
                                                ResilienceExpansion::Maximal,
                                                baseOperand, needsGenericContext,
                                                subs, decl, {},
                                                baseTy->getCanonicalType(),
      /*property descriptor*/ true);

   (void)PILProperty::create(M, /*serialized*/ false, decl, component);
}

void PILGenModule::visitIfConfigDecl(IfConfigDecl *ICD) {
   // Nothing to do for these kinds of decls - anything active has been added
   // to the enclosing declaration.
}

void PILGenModule::visitPoundDiagnosticDecl(PoundDiagnosticDecl *PDD) {
   // Nothing to do for #error/#warning; they've already been emitted.
}

void PILGenModule::visitTopLevelCodeDecl(TopLevelCodeDecl *td) {
   assert(TopLevelSGF && "top-level code in a non-main source file!");

   if (!TopLevelSGF->B.hasValidInsertionPoint())
      return;

   // A single PILFunction may be used to lower multiple top-level decls. When
   // this happens, fresh profile counters must be assigned to the new decl.
   TopLevelSGF->F.discardProfiler();
   TopLevelSGF->F.createProfiler(td, PILDeclRef(), ForDefinition);

   TopLevelSGF->emitProfilerIncrement(td->getBody());

   DebugScope DS(*TopLevelSGF, CleanupLocation(td));

   for (auto &ESD : td->getBody()->getElements()) {
      if (!TopLevelSGF->B.hasValidInsertionPoint()) {
         if (auto *S = ESD.dyn_cast<Stmt*>()) {
            if (S->isImplicit())
               continue;
         } else if (auto *E = ESD.dyn_cast<Expr*>()) {
            if (E->isImplicit())
               continue;
         }

         diagnose(ESD.getStartLoc(), diag::unreachable_code);
         // There's no point in trying to emit anything else.
         return;
      }

      if (auto *S = ESD.dyn_cast<Stmt*>()) {
         TopLevelSGF->emitStmt(S);
      } else if (auto *E = ESD.dyn_cast<Expr*>()) {
         TopLevelSGF->emitIgnoredExpr(E);
      } else {
         TopLevelSGF->visit(ESD.get<Decl*>());
      }
   }
}

namespace {

/// An RAII class to scope source file codegen.
class SourceFileScope {
   PILGenModule &sgm;
   SourceFile *sf;
   Optional<Scope> scope;
public:
   SourceFileScope(PILGenModule &sgm, SourceFile *sf) : sgm(sgm), sf(sf) {
      // If this is the script-mode file for the module, create a toplevel.
      if (sf->isScriptMode()) {
         assert(!sgm.TopLevelSGF && "already emitted toplevel?!");
         assert(!sgm.M.lookUpFunction(POLAR_ENTRY_POINT_FUNCTION)
                && "already emitted toplevel?!");

         RegularLocation TopLevelLoc = RegularLocation::getModuleLocation();
         PILFunction *toplevel = sgm.emitTopLevelFunction(TopLevelLoc);

         // Assign a debug scope pointing into the void to the top level function.
         toplevel->setDebugScope(new (sgm.M) PILDebugScope(TopLevelLoc, toplevel));

         sgm.TopLevelSGF = new PILGenFunction(sgm, *toplevel, sf);
         sgm.TopLevelSGF->MagicFunctionName = sgm.PolarphpModule->getName();
         auto moduleCleanupLoc = CleanupLocation::getModuleCleanupLocation();
         sgm.TopLevelSGF->prepareEpilog(Type(), true, moduleCleanupLoc);

         // Create the argc and argv arguments.
         auto prologueLoc = RegularLocation::getModuleLocation();
         prologueLoc.markAsPrologue();
         auto entry = sgm.TopLevelSGF->B.getInsertionBB();
         auto paramTypeIter =
            sgm.TopLevelSGF->F.getConventions().getParameterPILTypes().begin();
         entry->createFunctionArgument(*paramTypeIter);
         entry->createFunctionArgument(*std::next(paramTypeIter));

         scope.emplace(sgm.TopLevelSGF->Cleanups, moduleCleanupLoc);
      }
   }

   ~SourceFileScope() {
      if (sgm.TopLevelSGF) {
         scope.reset();

         // Unregister the top-level function emitter.
         auto &SGF = *sgm.TopLevelSGF;
         sgm.TopLevelSGF = nullptr;

         // Write out the epilog.
         auto moduleLoc = RegularLocation::getModuleLocation();
         moduleLoc.markAutoGenerated();
         auto returnInfo = SGF.emitEpilogBB(moduleLoc);
         auto returnLoc = returnInfo.second;
         returnLoc.markAutoGenerated();

         PILType returnType = SGF.F.getConventions().getSinglePILResultType();
         auto emitTopLevelReturnValue = [&](unsigned value) -> PILValue {
            // Create an integer literal for the value.
            auto litType = PILType::getBuiltinIntegerType(32, sgm.getAstContext());
            PILValue retValue =
               SGF.B.createIntegerLiteral(moduleLoc, litType, value);

            // Wrap that in a struct if necessary.
            if (litType != returnType) {
               retValue = SGF.B.createStruct(moduleLoc, returnType, retValue);
            }
            return retValue;
         };

         // Fallthrough should signal a normal exit by returning 0.
         PILValue returnValue;
         if (SGF.B.hasValidInsertionPoint())
            returnValue = emitTopLevelReturnValue(0);

         // Handle the implicit rethrow block.
         auto rethrowBB = SGF.ThrowDest.getBlock();
         SGF.ThrowDest = JumpDest::invalid();

         // If the rethrow block wasn't actually used, just remove it.
         if (rethrowBB->pred_empty()) {
            SGF.eraseBasicBlock(rethrowBB);

            // Otherwise, we need to produce a unified return block.
         } else {
            auto returnBB = SGF.createBasicBlock();
            if (SGF.B.hasValidInsertionPoint())
               SGF.B.createBranch(returnLoc, returnBB, returnValue);
            returnValue =
               returnBB->createPhiArgument(returnType, ValueOwnershipKind::Owned);
            SGF.B.emitBlock(returnBB);

            // Emit the rethrow block.
            PILGenSavedInsertionPoint savedIP(SGF, rethrowBB,
                                              FunctionSection::Postmatter);

            // Log the error.
            PILValue error = rethrowBB->getArgument(0);
            SGF.B.createBuiltin(moduleLoc,
                                sgm.getAstContext().getIdentifier("errorInMain"),
                                sgm.Types.getEmptyTupleType(), {}, {error});

            // Then end the lifetime of the error.
            //
            // We do this to appease the ownership verifier. We do not care about
            // actually destroying the value since we are going to immediately exit,
            // so this saves us a slight bit of code-size since end_lifetime is
            // stripped out after ownership is removed.
            SGF.B.createEndLifetime(moduleLoc, error);

            // Signal an abnormal exit by returning 1.
            SGF.Cleanups.emitCleanupsForReturn(CleanupLocation::get(moduleLoc),
                                               IsForUnwind);
            SGF.B.createBranch(returnLoc, returnBB, emitTopLevelReturnValue(1));
         }

         // Return.
         if (SGF.B.hasValidInsertionPoint())
            SGF.B.createReturn(returnLoc, returnValue);

         // Okay, we're done emitting the top-level function; destroy the
         // emitter and verify the result.
         PILFunction *toplevel = &SGF.getFunction();
         delete &SGF;

         LLVM_DEBUG(llvm::dbgs() << "lowered toplevel sil:\n";
                       toplevel->print(llvm::dbgs()));
         toplevel->verify();
         sgm.emitLazyConformancesForFunction(toplevel);
      }

      // If the source file contains an artificial main, emit the implicit
      // toplevel code.
      if (auto mainClass = sf->getMainClass()) {
         assert(!sgm.M.lookUpFunction(POLAR_ENTRY_POINT_FUNCTION)
                && "already emitted toplevel before main class?!");

         RegularLocation TopLevelLoc = RegularLocation::getModuleLocation();
         PILFunction *toplevel = sgm.emitTopLevelFunction(TopLevelLoc);

         // Assign a debug scope pointing into the void to the top level function.
         toplevel->setDebugScope(new (sgm.M) PILDebugScope(TopLevelLoc, toplevel));

         // Create the argc and argv arguments.
         PILGenFunction SGF(sgm, *toplevel, sf);
         auto entry = SGF.B.getInsertionBB();
         auto paramTypeIter =
            SGF.F.getConventions().getParameterPILTypes().begin();
         entry->createFunctionArgument(*paramTypeIter);
         entry->createFunctionArgument(*std::next(paramTypeIter));
         SGF.emitArtificialTopLevel(mainClass);
      }
   }
};

} // end anonymous namespace

void PILGenModule::emitSourceFile(SourceFile *sf) {
   SourceFileScope scope(*this, sf);
   FrontendStatsTracer StatsTracer(getAstContext().Stats, "PILgen-file", sf);
   for (Decl *D : sf->Decls) {
      FrontendStatsTracer StatsTracer(getAstContext().Stats, "PILgen-decl", D);
      visit(D);
   }

   for (TypeDecl *TD : sf->LocalTypeDecls) {
      FrontendStatsTracer StatsTracer(getAstContext().Stats, "PILgen-tydecl", TD);
      // FIXME: Delayed parsing would prevent these types from being added to the
      //        module in the first place.
      if (TD->getDeclContext()->getInnermostSkippedFunctionContext())
         continue;
      visit(TD);
   }
}

//===----------------------------------------------------------------------===//
// PILModule::constructPIL method implementation
//===----------------------------------------------------------------------===//

std::unique_ptr<PILModule>
PILModule::constructPIL(ModuleDecl *mod, TypeConverter &tc,
                        PILOptions &options, FileUnit *SF) {
   FrontendStatsTracer tracer(mod->getAstContext().Stats, "PILGen");
   const DeclContext *DC;
   if (SF) {
      DC = SF;
   } else {
      DC = mod;
   }

   std::unique_ptr<PILModule> M(
      new PILModule(mod, tc, options, DC, /*wholeModule*/ SF == nullptr));
   PILGenModule SGM(*M, mod);

   if (SF) {
      if (auto *file = dyn_cast<SourceFile>(SF)) {
         SGM.emitSourceFile(file);
      } else if (auto *file = dyn_cast<SerializedAstFile>(SF)) {
         /// TODO
//         if (file->isPIB())
//            M->getPILLoader()->getAllForModule(mod->getName(), file);
      }
   } else {
      for (auto file : mod->getFiles()) {
         auto nextSF = dyn_cast<SourceFile>(file);
         if (!nextSF || nextSF->AstStage != SourceFile::TypeChecked)
            continue;
         SGM.emitSourceFile(nextSF);
      }

      // Also make sure to process any intermediate files that may contain PIL
      bool hasPIB = std::any_of(mod->getFiles().begin(),
                                mod->getFiles().end(),
                                [](const FileUnit *File) -> bool {
                                   auto *SASTF = dyn_cast<SerializedAstFile>(File);
                                   return SASTF && SASTF->isPIB();
                                });
      //
//      if (hasPIB)
//         M->getPILLoader()->getAllForModule(mod->getName(), nullptr);
   }

   // Emit any delayed definitions that were forced.
   // Emitting these may in turn force more definitions, so we have to take care
   // to keep pumping the queues.
   while (!SGM.forcedFunctions.empty()
          || !SGM.pendingConformances.empty()) {
      while (!SGM.forcedFunctions.empty()) {
         auto &front = SGM.forcedFunctions.front();
         front.second.emitter(SGM.getFunction(front.first, ForDefinition));
         SGM.forcedFunctions.pop_front();
      }
      while (!SGM.pendingConformances.empty()) {
         SGM.getWitnessTable(SGM.pendingConformances.front());
         SGM.pendingConformances.pop_front();
      }
   }

   return M;
}

std::unique_ptr<PILModule>
polar::performPILGeneration(ModuleDecl *mod, lowering::TypeConverter &tc,
                            PILOptions &options) {
   return PILModule::constructPIL(mod, tc, options, nullptr);
}

std::unique_ptr<PILModule>
polar::performPILGeneration(FileUnit &sf, lowering::TypeConverter &tc,
                            PILOptions &options) {
   return PILModule::constructPIL(sf.getParentModule(), tc, options, &sf);
}
