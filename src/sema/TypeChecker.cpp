//===--- TypeChecker.cpp - Type Checking ----------------------------------===//
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
// This file implements the polar::performTypeChecking entry point for
// semantic analysis.
//
//===----------------------------------------------------------------------===//

#include "polarphp/global/Subsystems.h"
#include "polarphp/sema/internal/ConstraintSystem.h"
#include "polarphp/sema/internal/TypeChecker.h"
#include "polarphp/sema/internal/TypeCheckType.h"
#include "polarphp/sema/internal/CodeSynthesis.h"
#include "polarphp/sema/internal/MiscDiagnostics.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/AstVisitor.h"
#include "polarphp/ast/Attr.h"
#include "polarphp/ast/DiagnosticSuppression.h"
#include "polarphp/ast/ExistentialLayout.h"
#include "polarphp/ast/Identifier.h"
#include "polarphp/ast/ImportCache.h"
#include "polarphp/ast/Initializer.h"
#include "polarphp/ast/ModuleLoader.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/PrettyStackTrace.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/TypeCheckRequests.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/STLExtras.h"
#include "polarphp/basic/Timer.h"
#include "polarphp/llparser/Lexer.h"
#include "polarphp/sema/IDETypeChecking.h"
#include "polarphp/global/NameStrings.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/TinyPtrVector.h"
#include "llvm/ADT/Twine.h"
#include <algorithm>

using namespace polar;
using namespace llparser;

TypeChecker &TypeChecker::createForContext(AstContext &ctx) {
   assert(!ctx.getLegacyGlobalTypeChecker() &&
          "Cannot install more than one instance of the global type checker!");
   auto *TC = new TypeChecker();
   ctx.installGlobalTypeChecker(TC);
   ctx.addCleanup([=](){ delete TC; });
   return *ctx.getLegacyGlobalTypeChecker();
}

InterfaceDecl *TypeChecker::getInterface(AstContext &Context, SourceLoc loc,
                                       KnownInterfaceKind kind) {
   auto protocol = Context.getInterface(kind);
   if (!protocol && loc.isValid()) {
      Context.Diags.diagnose(loc, diag::missing_protocol,
                             Context.getIdentifier(get_interface_name(kind)));
   }

   if (protocol && protocol->isInvalid()) {
      return nullptr;
   }

   return protocol;
}

InterfaceDecl *TypeChecker::getLiteralInterface(AstContext &Context, Expr *expr) {
   if (isa<ArrayExpr>(expr))
      return TypeChecker::getInterface(
         Context, expr->getLoc(), KnownInterfaceKind::ExpressibleByArrayLiteral);

   if (isa<DictionaryExpr>(expr))
      return TypeChecker::getInterface(
         Context, expr->getLoc(),
         KnownInterfaceKind::ExpressibleByDictionaryLiteral);

   if (!isa<LiteralExpr>(expr))
      return nullptr;

   if (isa<NilLiteralExpr>(expr))
      return TypeChecker::getInterface(Context, expr->getLoc(),
                                      KnownInterfaceKind::ExpressibleByNilLiteral);

   if (isa<IntegerLiteralExpr>(expr))
      return TypeChecker::getInterface(
         Context, expr->getLoc(),
         KnownInterfaceKind::ExpressibleByIntegerLiteral);

   if (isa<FloatLiteralExpr>(expr))
      return TypeChecker::getInterface(
         Context, expr->getLoc(), KnownInterfaceKind::ExpressibleByFloatLiteral);

   if (isa<BooleanLiteralExpr>(expr))
      return TypeChecker::getInterface(
         Context, expr->getLoc(),
         KnownInterfaceKind::ExpressibleByBooleanLiteral);

   if (const auto *SLE = dyn_cast<StringLiteralExpr>(expr)) {
      if (SLE->isSingleUnicodeScalar())
         return TypeChecker::getInterface(
            Context, expr->getLoc(),
            KnownInterfaceKind::ExpressibleByUnicodeScalarLiteral);

      if (SLE->isSingleExtendedGraphemeCluster())
         return getInterface(
            Context, expr->getLoc(),
            KnownInterfaceKind::ExpressibleByExtendedGraphemeClusterLiteral);

      return TypeChecker::getInterface(
         Context, expr->getLoc(), KnownInterfaceKind::ExpressibleByStringLiteral);
   }

   if (isa<InterpolatedStringLiteralExpr>(expr))
      return TypeChecker::getInterface(
         Context, expr->getLoc(),
         KnownInterfaceKind::ExpressibleByStringInterpolation);

   if (auto E = dyn_cast<MagicIdentifierLiteralExpr>(expr)) {
      switch (E->getKind()) {
         case MagicIdentifierLiteralExpr::File:
         case MagicIdentifierLiteralExpr::Function:
            return TypeChecker::getInterface(
               Context, expr->getLoc(),
               KnownInterfaceKind::ExpressibleByStringLiteral);

         case MagicIdentifierLiteralExpr::Line:
         case MagicIdentifierLiteralExpr::Column:
            return TypeChecker::getInterface(
               Context, expr->getLoc(),
               KnownInterfaceKind::ExpressibleByIntegerLiteral);

         case MagicIdentifierLiteralExpr::DSOHandle:
            return nullptr;
      }
   }

   if (auto E = dyn_cast<ObjectLiteralExpr>(expr)) {
      switch (E->getLiteralKind()) {
#define POUND_OBJECT_LITERAL(Name, Desc, Interface)                             \
  case ObjectLiteralExpr::Name:                                                \
    return TypeChecker::getInterface(Context, expr->getLoc(),                   \
                                    KnownInterfaceKind::Interface);
#include "polarphp/llparser/TokenKindsDef.h"
      }
   }

   return nullptr;
}

DeclName TypeChecker::getObjectLiteralConstructorName(AstContext &Context,
                                                      ObjectLiteralExpr *expr) {
   switch (expr->getLiteralKind()) {
      case ObjectLiteralExpr::colorLiteral: {
         return DeclName(Context, DeclBaseName::createConstructor(),
                         { Context.getIdentifier("_colorLiteralRed"),
                           Context.getIdentifier("green"),
                           Context.getIdentifier("blue"),
                           Context.getIdentifier("alpha") });
      }
      case ObjectLiteralExpr::imageLiteral: {
         return DeclName(Context, DeclBaseName::createConstructor(),
                         { Context.getIdentifier("imageLiteralResourceName") });
      }
      case ObjectLiteralExpr::fileLiteral: {
         return DeclName(Context, DeclBaseName::createConstructor(),
                         { Context.getIdentifier("fileReferenceLiteralResourceName") });
      }
   }
   llvm_unreachable("unknown literal constructor");
}

/// Return an idealized form of the parameter type of the given
/// object-literal initializer.  This removes references to the protocol
/// name from the first argument label, which would be otherwise be
/// redundant when writing out the object-literal syntax:
///
///   #fileLiteral(fileReferenceLiteralResourceName: "hello.jpg")
///
/// Doing this allows us to preserve a nicer (and source-compatible)
/// literal syntax while still giving the initializer a semantically
/// unambiguous name.
Type TypeChecker::getObjectLiteralParameterType(ObjectLiteralExpr *expr,
                                                ConstructorDecl *ctor) {
   auto params = ctor->getMethodInterfaceType()
      ->castTo<FunctionType>()->getParams();
   SmallVector<AnyFunctionType::Param, 8> newParams;
   newParams.append(params.begin(), params.end());

   auto replace = [&](StringRef replacement) -> Type {
      auto &Context = ctor->getAstContext();
      newParams[0] = AnyFunctionType::Param(newParams[0].getPlainType(),
                                            Context.getIdentifier(replacement),
                                            newParams[0].getParameterFlags());
      return AnyFunctionType::composeInput(Context, newParams,
         /*canonicalVararg=*/false);
   };

   switch (expr->getLiteralKind()) {
      case ObjectLiteralExpr::colorLiteral:
         return replace("red");
      case ObjectLiteralExpr::fileLiteral:
      case ObjectLiteralExpr::imageLiteral:
         return replace("resourceName");
   }
   llvm_unreachable("unknown literal constructor");
}

ModuleDecl *TypeChecker::getStdlibModule(const DeclContext *dc) {
   if (auto *stdlib = dc->getAstContext().getStdlibModule()) {
      return stdlib;
   }

   return dc->getParentModule();
}

/// Bind the given extension to the given nominal type.
static void bindExtensionToNominal(ExtensionDecl *ext,
                                   NominalTypeDecl *nominal) {
   if (ext->alreadyBoundToNominal())
      return;

   nominal->addExtension(ext);
}

static void bindExtensions(SourceFile &SF) {
   // Utility function to try and resolve the extended type without diagnosing.
   // If we succeed, we go ahead and bind the extension. Otherwise, return false.
   auto tryBindExtension = [&](ExtensionDecl *ext) -> bool {
      assert(!ext->canNeverBeBound() &&
             "Only extensions that can ever be bound get here.");
      if (auto nominal = ext->computeExtendedNominal()) {
         bindExtensionToNominal(ext, nominal);
         return true;
      }

      return false;
   };

   // Phase 1 - try to bind each extension, adding those whose type cannot be
   // resolved to a worklist.
   SmallVector<ExtensionDecl *, 8> worklist;

   // FIXME: The current source file needs to be handled specially, because of
   // private extensions.
   for (auto import : namelookup::getAllImports(&SF)) {
      // FIXME: Respect the access path?
      for (auto file : import.second->getFiles()) {
         auto SF = dyn_cast<SourceFile>(file);
         if (!SF)
            continue;

         for (auto D : SF->Decls) {
            if (auto ED = dyn_cast<ExtensionDecl>(D))
               if (!tryBindExtension(ED))
                  worklist.push_back(ED);
         }
      }
   }

   // Phase 2 - repeatedly go through the worklist and attempt to bind each
   // extension there, removing it from the worklist if we succeed.
   bool changed;
   do {
      changed = false;

      auto last = std::remove_if(worklist.begin(), worklist.end(),
                                 tryBindExtension);
      if (last != worklist.end()) {
         worklist.erase(last, worklist.end());
         changed = true;
      }
   } while(changed);

   // Any remaining extensions are invalid. They will be diagnosed later by
   // typeCheckDecl().
}

static void typeCheckFunctionsAndExternalDecls(SourceFile &SF, TypeChecker &TC) {
   unsigned currentFunctionIdx = 0;
   unsigned currentSynthesizedDecl = SF.LastCheckedSynthesizedDecl;
   do {
      // Type check the body of each of the function in turn.  Note that outside
      // functions must be visited before nested functions for type-checking to
      // work correctly.
      for (unsigned n = TC.definedFunctions.size(); currentFunctionIdx != n;
           ++currentFunctionIdx) {
         auto *AFD = TC.definedFunctions[currentFunctionIdx];
         assert(!AFD->getDeclContext()->isLocalContext());

         TypeChecker::typeCheckAbstractFunctionBody(AFD);
      }

      // Type check synthesized functions and their bodies.
      for (unsigned n = SF.SynthesizedDecls.size();
           currentSynthesizedDecl != n;
           ++currentSynthesizedDecl) {
         auto decl = SF.SynthesizedDecls[currentSynthesizedDecl];
         TypeChecker::typeCheckDecl(decl);
      }

   } while (currentFunctionIdx < TC.definedFunctions.size() ||
            currentSynthesizedDecl < SF.SynthesizedDecls.size());


   for (AbstractFunctionDecl *FD : llvm::reverse(TC.definedFunctions)) {
      TypeChecker::computeCaptures(FD);
   }

   TC.definedFunctions.clear();
}

void polar::performTypeChecking(SourceFile &SF, unsigned StartElem) {
   return (void)evaluateOrDefault(SF.getAstContext().evaluator,
                                  TypeCheckSourceFileRequest{&SF, StartElem},
                                  false);
}

llvm::Expected<bool>
TypeCheckSourceFileRequest::evaluate(Evaluator &eval,
                                     SourceFile *SF, unsigned StartElem) const {
   assert(SF && "Source file cannot be null!");
   assert(SF->AstStage != SourceFile::TypeChecked &&
          "Should not be re-typechecking this file!");

   // Eagerly build the top-level scopes tree before type checking
   // because type-checking expressions mutates the Ast and that throws off the
   // scope-based lookups. Only the top-level scopes because extensions have not
   // been bound yet.
   auto &Ctx = SF->getAstContext();
   if (Ctx.LangOpts.EnableAstScopeLookup && SF->isSuitableForAstScopes())
      SF->getScope()
         .buildEnoughOfTreeForTopLevelExpressionsButDontRequestGenericsOrExtendedNominals();

   BufferIndirectlyCausingDiagnosticRAII cpr(*SF);

   // Make sure we have a type checker.
   TypeChecker &TC = createTypeChecker(Ctx);

   // Make sure that name binding has been completed before doing any type
   // checking.
   performNameBinding(*SF, StartElem);

   // Could build scope maps here because the Ast is stable now.

   {
      FrontendStatsTracer tracer(Ctx.Stats, "Type checking and Semantic analysis");

      if (Ctx.TypeCheckerOpts.SkipNonInlinableFunctionBodies)
         // Disable this optimization if we're compiling SwiftOnoneSupport, because
         // we _definitely_ need to look inside every declaration to figure out
         // what gets prespecialized.
         if (SF->getParentModule()->isOnoneSupportModule())
            Ctx.TypeCheckerOpts.SkipNonInlinableFunctionBodies = false;

      if (!Ctx.LangOpts.DisableAvailabilityChecking) {
         // Build the type refinement hierarchy for the primary
         // file before type checking.
         TypeChecker::buildTypeRefinementContextHierarchy(*SF, StartElem);
      }

      // Resolve extensions. This has to occur first during type checking,
      // because the extensions need to be wired into the Ast for name lookup
      // to work.
      ::bindExtensions(*SF);

      // Type check the top-level elements of the source file.
      for (auto D : llvm::makeArrayRef(SF->Decls).slice(StartElem)) {
         if (auto *TLCD = dyn_cast<TopLevelCodeDecl>(D)) {
            // Immediately perform global name-binding etc.
            TypeChecker::typeCheckTopLevelCodeDecl(TLCD);
            TypeChecker::contextualizeTopLevelCode(TLCD);
         } else {
            TypeChecker::typeCheckDecl(D);
         }
      }

      // If we're in REPL mode, inject temporary result variables and other stuff
      // that the REPL needs to synthesize.
      if (SF->Kind == SourceFileKind::REPL && !Ctx.hadError())
         TypeChecker::processREPLTopLevel(*SF, StartElem);

      typeCheckFunctionsAndExternalDecls(*SF, TC);
   }

   // Checking that benefits from having the whole module available.
   if (!Ctx.TypeCheckerOpts.DelayWholeModuleChecking) {
      performWholeModuleTypeChecking(*SF);
   }

   return true;
}

void polar::performWholeModuleTypeChecking(SourceFile &SF) {
   auto &Ctx = SF.getAstContext();
   FrontendStatsTracer tracer(Ctx.Stats, "perform-whole-module-type-checking");
//   diagnoseAttrsRequiringFoundation(SF);
//   diagnoseUnintendedObjCMethodOverrides(SF);

   // In whole-module mode, import verification is deferred until all files have
   // been type checked. This avoids caching imported declarations when a valid
   // type checker is not present. The same declaration may need to be fully
   // imported by subsequent files.
   //
   // FIXME: some playgrounds tests (playground_lvalues.swift) fail with
   // verification enabled.
#if 0
   if (SF.Kind != SourceFileKind::REPL &&
      SF.Kind != SourceFileKind::PIL &&
      !Ctx.LangOpts.DebuggerSupport) {
    Ctx.verifyAllLoadedModules();
  }
#endif
}

void polar::checkInconsistentImplementationOnlyImports(ModuleDecl *MainModule) {
   bool hasAnyImplementationOnlyImports =
      llvm::any_of(MainModule->getFiles(), [](const FileUnit *F) -> bool {
         auto *SF = dyn_cast<SourceFile>(F);
         return SF && SF->hasImplementationOnlyImports();
      });
   if (!hasAnyImplementationOnlyImports)
      return;

   auto diagnose = [MainModule](const ImportDecl *normalImport,
                                const ImportDecl *implementationOnlyImport) {
      auto &diags = MainModule->getDiags();
      {
         InFlightDiagnostic warning =
            diags.diagnose(normalImport, diag::warn_implementation_only_conflict,
                           normalImport->getModule()->getName());
         if (normalImport->getAttrs().isEmpty()) {
            // Only try to add a fix-it if there's no other annotations on the
            // import to avoid creating things likeConstantPropagation.cpp
            // `@_implementationOnly @_exported import Foo`. The developer can
            // resolve those manually.
            warning.fixItInsert(normalImport->getStartLoc(),
                                "@_implementationOnly ");
         }
      }
      diags.diagnose(implementationOnlyImport,
                     diag::implementation_only_conflict_here);
   };

   llvm::DenseMap<ModuleDecl *, std::vector<const ImportDecl *>> normalImports;
   llvm::DenseMap<ModuleDecl *, const ImportDecl *> implementationOnlyImports;

   for (const FileUnit *file : MainModule->getFiles()) {
      auto *SF = dyn_cast<SourceFile>(file);
      if (!SF)
         continue;

      for (auto *topLevelDecl : SF->Decls) {
         auto *nextImport = dyn_cast<ImportDecl>(topLevelDecl);
         if (!nextImport)
            continue;

         ModuleDecl *module = nextImport->getModule();
         if (!module)
            continue;

         if (nextImport->getAttrs().hasAttribute<ImplementationOnlyAttr>()) {
            // We saw an implementation-only import.
            bool isNew =
               implementationOnlyImports.insert({module, nextImport}).second;
            if (!isNew)
               continue;

            auto seenNormalImportPosition = normalImports.find(module);
            if (seenNormalImportPosition != normalImports.end()) {
               for (auto *seenNormalImport : seenNormalImportPosition->getSecond())
                  diagnose(seenNormalImport, nextImport);

               // We're done with these; keep the map small if possible.
               normalImports.erase(seenNormalImportPosition);
            }
            continue;
         }

         // We saw a non-implementation-only import. Is that in conflict with what
         // we've seen?
         if (auto *seenImplementationOnlyImport =
            implementationOnlyImports.lookup(module)) {
            diagnose(nextImport, seenImplementationOnlyImport);
            continue;
         }

         // Otherwise, record it for later.
         normalImports[module].push_back(nextImport);
      }
   }
}

bool polar::performTypeLocChecking(AstContext &Ctx, TypeLoc &T,
                                   DeclContext *DC,
                                   bool ProduceDiagnostics) {
   return performTypeLocChecking(
      Ctx, T,
      /*isPILMode=*/false,
      /*isPILType=*/false,
      /*GenericEnv=*/DC->getGenericEnvironmentOfContext(),
      DC, ProduceDiagnostics);
}

bool polar::performTypeLocChecking(AstContext &Ctx, TypeLoc &T,
                                   bool isPILMode,
                                   bool isPILType,
                                   GenericEnvironment *GenericEnv,
                                   DeclContext *DC,
                                   bool ProduceDiagnostics) {
   TypeResolutionOptions options = None;

   // Fine to have unbound generic types.
   options |= TypeResolutionFlags::AllowUnboundGenerics;
   if (isPILMode) {
      options |= TypeResolutionFlags::PILMode;
   }
   if (isPILType)
      options |= TypeResolutionFlags::PILType;

   auto resolution = TypeResolution::forContextual(DC, GenericEnv);
   Optional<DiagnosticSuppression> suppression;
   if (!ProduceDiagnostics)
      suppression.emplace(Ctx.Diags);
   assert(Ctx.areSemanticQueriesEnabled());
   return TypeChecker::validateType(Ctx, T, resolution, options);
}

/// Expose TypeChecker's handling of GenericParamList to PIL parsing.
GenericEnvironment *
polar::handlePILGenericParams(GenericParamList *genericParams,
                              DeclContext *DC) {
   if (genericParams == nullptr)
      return nullptr;

   SmallVector<GenericParamList *, 2> nestedList;
   for (; genericParams; genericParams = genericParams->getOuterParameters()) {
      nestedList.push_back(genericParams);
   }

   std::reverse(nestedList.begin(), nestedList.end());

   for (unsigned i = 0, e = nestedList.size(); i < e; ++i) {
      auto genericParams = nestedList[i];
      genericParams->setDepth(i);
   }

   auto sig =
      TypeChecker::checkGenericSignature(nestedList.back(), DC,
         /*parentSig=*/nullptr,
         /*allowConcreteGenericParams=*/true);
   return (sig ? sig->getGenericEnvironment() : nullptr);
}

void polar::typeCheckPatternBinding(PatternBindingDecl *PBD,
                                    unsigned bindingIndex) {
   assert(!PBD->isInitializerChecked(bindingIndex) &&
          PBD->getInit(bindingIndex));

   auto &Ctx = PBD->getAstContext();
   DiagnosticSuppression suppression(Ctx.Diags);
   (void)createTypeChecker(Ctx);
   TypeChecker::typeCheckPatternBinding(PBD, bindingIndex);
}

static Optional<Type> getTypeOfCompletionContextExpr(
   DeclContext *DC,
   CompletionTypeCheckKind kind,
   Expr *&parsedExpr,
   ConcreteDeclRef &referencedDecl) {
   if (constraints::ConstraintSystem::preCheckExpression(parsedExpr, DC))
      return None;

   switch (kind) {
      case CompletionTypeCheckKind::Normal:
         // Handle below.
         break;

      case CompletionTypeCheckKind::KeyPath:
         /// TODO
//         referencedDecl = nullptr;
//         if (auto keyPath = dyn_cast<KeyPathExpr>(parsedExpr))
//            return TypeChecker::checkObjCKeyPathExpr(DC, keyPath,
//               /*requireResultType=*/true);

         return None;
   }

   Type originalType = parsedExpr->getType();
   if (auto T = TypeChecker::getTypeOfExpressionWithoutApplying(parsedExpr, DC,
                                                                referencedDecl, FreeTypeVariableBinding::UnresolvedType))
      return T;

   // Try to recover if we've made any progress.
   if (parsedExpr &&
       !isa<ErrorExpr>(parsedExpr) &&
       parsedExpr->getType() &&
       !parsedExpr->getType()->hasError() &&
       (originalType.isNull() ||
        !parsedExpr->getType()->isEqual(originalType))) {
      return parsedExpr->getType();
   }

   return None;
}

/// Return the type of an expression parsed during code completion, or
/// a null \c Type on error.
Optional<Type> polar::getTypeOfCompletionContextExpr(
   AstContext &Ctx,
   DeclContext *DC,
   CompletionTypeCheckKind kind,
   Expr *&parsedExpr,
   ConcreteDeclRef &referencedDecl) {
   DiagnosticSuppression suppression(Ctx.Diags);
   (void)createTypeChecker(Ctx);

   // Try to solve for the actual type of the expression.
   return ::getTypeOfCompletionContextExpr(DC, kind, parsedExpr,
                                           referencedDecl);
}

/// Return the type of operator function for specified LHS, or a null
/// \c Type on error.
FunctionType *
polar::getTypeOfCompletionOperator(DeclContext *DC, Expr *LHS,
                                   Identifier opName, DeclRefKind refKind,
                                   ConcreteDeclRef &referencedDecl) {
   auto &ctx = DC->getAstContext();
   DiagnosticSuppression suppression(ctx.Diags);
   (void)createTypeChecker(ctx);
   return TypeChecker::getTypeOfCompletionOperator(DC, LHS, opName, refKind,
                                                   referencedDecl);
}

bool polar::typeCheckExpression(DeclContext *DC, Expr *&parsedExpr) {
   auto &ctx = DC->getAstContext();
   DiagnosticSuppression suppression(ctx.Diags);
   (void)createTypeChecker(ctx);
   auto resultTy = TypeChecker::typeCheckExpression(parsedExpr, DC, TypeLoc(),
                                                    CTP_Unused);
   return !resultTy;
}

bool polar::typeCheckAbstractFunctionBodyUntil(AbstractFunctionDecl *AFD,
                                               SourceLoc EndTypeCheckLoc) {
   auto &Ctx = AFD->getAstContext();
   DiagnosticSuppression suppression(Ctx.Diags);

   (void)createTypeChecker(Ctx);
   return !TypeChecker::typeCheckAbstractFunctionBodyUntil(AFD, EndTypeCheckLoc);
}

bool polar::typeCheckTopLevelCodeDecl(TopLevelCodeDecl *TLCD) {
   auto &Ctx = static_cast<Decl *>(TLCD)->getAstContext();
   DiagnosticSuppression suppression(Ctx.Diags);
   (void)createTypeChecker(Ctx);
   TypeChecker::typeCheckTopLevelCodeDecl(TLCD);
   return true;
}

TypeChecker &polar::createTypeChecker(AstContext &Ctx) {
   if (auto *TC = Ctx.getLegacyGlobalTypeChecker())
      return *TC;
   return TypeChecker::createForContext(Ctx);
}

void TypeChecker::checkForForbiddenPrefix(AstContext &C, DeclBaseName Name) {
   if (C.TypeCheckerOpts.DebugForbidTypecheckPrefix.empty())
      return;

   // Don't touch special names or empty names.
   if (Name.isSpecial() || Name.empty())
      return;

   StringRef Str = Name.getIdentifier().str();
   if (Str.startswith(C.TypeCheckerOpts.DebugForbidTypecheckPrefix)) {
      std::string Msg = "forbidden typecheck occurred: ";
      Msg += Str;
      llvm::report_fatal_error(Msg);
   }
}

DeclTypeCheckingSemantics
TypeChecker::getDeclTypeCheckingSemantics(ValueDecl *decl) {
   // Check for a @_semantics attribute.
   if (auto semantics = decl->getAttrs().getAttribute<SemanticsAttr>()) {
      if (semantics->Value.equals("typechecker.type(of:)"))
         return DeclTypeCheckingSemantics::TypeOf;
      if (semantics->Value.equals("typechecker.withoutActuallyEscaping(_:do:)"))
         return DeclTypeCheckingSemantics::WithoutActuallyEscaping;
      if (semantics->Value.equals("typechecker._openExistential(_:do:)"))
         return DeclTypeCheckingSemantics::OpenExistential;
   }
   return DeclTypeCheckingSemantics::Normal;
}

void polar::bindExtensions(SourceFile &SF) {
   ::bindExtensions(SF);
}
