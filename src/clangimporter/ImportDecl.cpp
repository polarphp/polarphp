//===--- ImportDecl.cpp - Import Clang Declarations -----------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file implements support for importing Clang declarations into Swift.
//
//===----------------------------------------------------------------------===//

#include "polarphp/clangimporter/internal/CFTypeInfo.h"
#include "polarphp/clangimporter/internal/ImporterImpl.h"
#include "polarphp/global/NameStrings.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/Attr.h"
#include "polarphp/ast/BuiltinTypes.h"
#include "polarphp/ast/ClangModuleLoader.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/DiagnosticsClangImporter.h"
#include "polarphp/ast/ExistentialLayout.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/GenericEnvironment.h"
#include "polarphp/ast/GenericSignature.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/NameLookup.h"
#include "polarphp/ast/NameLookupRequests.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/PrettyStackTrace.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/Types.h"
#include "polarphp/ast/TypeCheckRequests.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/PrettyStackTrace.h"
#include "polarphp/clangimporter/ClangModule.h"
#include "polarphp/llparser/Lexer.h"
#include "clang/AST/AstContext.h"
#include "clang/AST/Attr.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Sema/Lookup.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Path.h"

#include <algorithm>

#define DEBUG_TYPE "Clang module importer"

STATISTIC(NumTotalImportedEntities, "# of imported clang entities");
STATISTIC(NumFactoryMethodsAsInitializers,
          "# of factory methods mapped to initializers");

using namespace polar;
using namespace importer;

namespace polar {
namespace inferredattributes {
enum {
   requires_stored_property_inits = 0x01
};
} // end namespace inferredattributes
} // end namespace polar

namespace {

struct AccessorInfo {
   AbstractStorageDecl *Storage;
   AccessorKind Kind;
};

enum class MakeStructRawValuedFlags {
   /// whether to also create an unlabeled init
      MakeUnlabeledValueInit = 0x01,

   /// whether the raw value should be a let
      IsLet = 0x02,

   /// whether to mark the rawValue as implicit
      IsImplicit = 0x04,
};
using MakeStructRawValuedOptions = OptionSet<MakeStructRawValuedFlags>;
} // end anonymous namespace

static MakeStructRawValuedOptions
getDefaultMakeStructRawValuedOptions() {
   MakeStructRawValuedOptions opts;
   opts -= MakeStructRawValuedFlags::MakeUnlabeledValueInit; // default off
   opts |= MakeStructRawValuedFlags::IsLet;                  // default on
   opts |= MakeStructRawValuedFlags::IsImplicit;             // default on
   return opts;
}

static bool isInSystemModule(DeclContext *D) {
   return cast<ClangModuleUnit>(D->getModuleScopeContext())->isSystemModule();
}

static AccessLevel getOverridableAccessLevel(const DeclContext *dc) {
   return (dc->getSelfClassDecl() ? AccessLevel::Open : AccessLevel::Public);
}

/// Create a typedpattern(namedpattern(decl))
static Pattern *createTypedNamedPattern(VarDecl *decl) {
   AstContext &Ctx = decl->getAstContext();
   Type ty = decl->getType();

   Pattern *P = new (Ctx) NamedPattern(decl);
   P->setType(ty);
   P->setImplicit();
   return TypedPattern::createImplicit(Ctx, P, ty);
}

/// Create a var member for this struct, along with its pattern binding, and add
/// it as a member
static std::pair<VarDecl *, PatternBindingDecl *>
createVarWithPattern(AstContext &ctx, DeclContext *dc, Identifier name, Type ty,
                     VarDecl::Introducer introducer, bool isImplicit, AccessLevel access,
                     AccessLevel setterAccess) {
   // Create a variable to store the underlying value.
   auto var = new (ctx) VarDecl(
      /*IsStatic*/false, introducer,
      /*IsCaptureList*/false,
                  SourceLoc(), name, dc);
   if (isImplicit)
      var->setImplicit();
   var->setInterfaceType(ty);
   var->setAccess(access);
   var->setSetterAccess(setterAccess);

   // Create a pattern binding to describe the variable.
   Pattern *varPattern = createTypedNamedPattern(var);
   auto *patternBinding = PatternBindingDecl::create(
      ctx, /*StaticLoc*/ SourceLoc(), StaticSpellingKind::None,
      /*VarLoc*/ SourceLoc(), varPattern, /*EqualLoc*/ SourceLoc(),
      /*InitExpr*/ nullptr, dc);
   if (isImplicit)
      patternBinding->setImplicit();

   return {var, patternBinding};
}

static FuncDecl *createFuncOrAccessor(AstContext &ctx, SourceLoc funcLoc,
                                      Optional<AccessorInfo> accessorInfo,
                                      DeclName name, SourceLoc nameLoc,
                                      ParameterList *bodyParams,
                                      Type resultTy,
                                      bool throws,
                                      DeclContext *dc,
                                      ClangNode clangNode) {
   TypeLoc resultTypeLoc = resultTy ? TypeLoc::withoutLoc(resultTy) : TypeLoc();
   if (accessorInfo) {
      return AccessorDecl::create(ctx, funcLoc,
         /*accessorKeywordLoc*/ SourceLoc(),
                                  accessorInfo->Kind,
                                  accessorInfo->Storage,
         /*StaticLoc*/SourceLoc(),
                                  StaticSpellingKind::None,
                                  throws,
         /*ThrowsLoc=*/SourceLoc(),
         /*GenericParams=*/nullptr,
                                  bodyParams,
                                  resultTypeLoc, dc, clangNode);
   } else {
      return FuncDecl::create(ctx, /*StaticLoc=*/SourceLoc(),
                              StaticSpellingKind::None,
                              funcLoc, name, nameLoc,
                              throws, /*ThrowsLoc=*/SourceLoc(),
         /*GenericParams=*/nullptr,
                              bodyParams,
                              resultTypeLoc, dc, clangNode);
   }
}

static void makeComputed(AbstractStorageDecl *storage,
                         AccessorDecl *getter, AccessorDecl *setter) {
   assert(getter);
   if (setter) {
      storage->setImplInfo(StorageImplInfo::getMutableComputed());
      storage->setAccessors(SourceLoc(), {getter, setter}, SourceLoc());
   } else {
      storage->setImplInfo(StorageImplInfo::getImmutableComputed());
      storage->setAccessors(SourceLoc(), {getter}, SourceLoc());
   }
}

#ifndef NDEBUG
static bool verifyNameMapping(MappedTypeNameKind NameMapping,
                              StringRef left, StringRef right) {
   return NameMapping == MappedTypeNameKind::DoNothing || left != right;
}
#endif

/// Map a well-known C type to a swift type from the standard library.
///
/// \param IsError set to true when we know the corresponding swift type name,
/// but we could not find it.  (For example, the type was not defined in the
/// standard library or the required standard library module was not imported.)
/// This should be a hard error, we don't want to map the type only sometimes.
///
/// \returns A pair of a swift type and its name that corresponds to a given
/// C type.
static std::pair<Type, StringRef>
getPolarphpStdlibType(const clang::TypedefNameDecl *D,
                      Identifier Name,
                      ClangImporter::Implementation &Impl,
                      bool *IsError, MappedTypeNameKind &NameMapping) {
   *IsError = false;

   MappedCTypeKind CTypeKind;
   unsigned Bitwidth;
   StringRef PolarphpModuleName;
   bool IsPolarphpModule; // True if PolarphpModuleName == STDLIB_NAME.
   StringRef TypePHPTypeName;
   bool CanBeMissing;


   do {
#define MAP_TYPE(C_TYPE_NAME, C_TYPE_KIND, C_TYPE_BITWIDTH,        \
                 POLAR_MODULE_NAME, POLAR_TYPE_NAME,               \
                 CAN_BE_MISSING, C_NAME_MAPPING)                   \
    if (Name.str() == C_TYPE_NAME) {                               \
      CTypeKind = MappedCTypeKind::C_TYPE_KIND;                    \
      Bitwidth = C_TYPE_BITWIDTH;                                  \
      PolarphpModuleName = POLAR_MODULE_NAME;                         \
      IsPolarphpModule = PolarphpModuleName == STDLIB_NAME;              \
      TypePHPTypeName = POLAR_TYPE_NAME;                             \
      CanBeMissing = CAN_BE_MISSING;                               \
      NameMapping = MappedTypeNameKind::C_NAME_MAPPING;            \
      assert(verifyNameMapping(MappedTypeNameKind::C_NAME_MAPPING, \
                               C_TYPE_NAME, POLAR_TYPE_NAME) &&    \
             "MappedTypes.def: Identical names must use DoNothing"); \
      break;                                                       \
    }
#include "polarphp/clangimporter/internal/MappedTypesDef.h"

      // We handle `BOOL` as a special case because the selection here is more
      // complicated as the type alias exists on multiple platforms as different
      // types.  It appears in an Objective-C context where it is a `signed char`
      // and appears in Windows as an `int`.  Furthermore, you can actually have
      // the two interoperate, which requires a further bit of logic to
      // disambiguate the type aliasing behaviour.  To complicate things, the two
      // aliases bridge to different types - `ObjCBool` for Objective-C and
      // `WindowsBool` for Windows's `BOOL` type.
      if (Name.str() == "BOOL") {
         auto &CAstContext = Impl.getClangAstContext();
         auto &PolarphpAstContext = Impl.TypePHPContext;

         // Default to Objective-C `BOOL`
         CTypeKind = MappedCTypeKind::ObjCBool;
         if (CAstContext.getTargetInfo().getTriple().isOSWindows()) {
            // On Windows fall back to Windows `BOOL`
            CTypeKind = MappedCTypeKind::SignedInt;
            // If Objective-C interop is enabled, and we match the Objective-C
            // `BOOL` type, then switch back to `ObjCBool`.
            if (PolarphpAstContext.LangOpts.EnableObjCInterop &&
                CAstContext.hasSameType(D->getUnderlyingType(),
                                        CAstContext.ObjCBuiltinBoolTy))
               CTypeKind = MappedCTypeKind::ObjCBool;
         }

         if (CTypeKind == MappedCTypeKind::ObjCBool) {
            Bitwidth = 8;
            PolarphpModuleName = StringRef("ObjectiveC");
            IsPolarphpModule = false;
            TypePHPTypeName = "ObjCBool";
            NameMapping = MappedTypeNameKind::DoNothing;
            CanBeMissing = false;
            assert(verifyNameMapping(MappedTypeNameKind::DoNothing,
                                     "BOOL", "ObjCBool") &&
                   "MappedTypesDef.h: Identical names must use DoNothing");
         } else {
            assert(CTypeKind == MappedCTypeKind::SignedInt &&
                   "expected Windows `BOOL` desugared to `int`");
            Bitwidth = 32;
            PolarphpModuleName = StringRef("WinSDK");
            IsPolarphpModule = false;
            TypePHPTypeName = "WindowsBool";
            NameMapping = MappedTypeNameKind::DoNothing;
            CanBeMissing = true;
            assert(verifyNameMapping(MappedTypeNameKind::DoNothing,
                                     "BOOL", "WindowsBool") &&
                   "MappedTypesDef.h: Identical names must use DoNothing");
         }

         break;
      }

      // We did not find this type, thus it is not mapped.
      return std::make_pair(Type(), "");
   } while (0);

   clang::ASTContext &ClangCtx = Impl.getClangAstContext();

   auto ClangType = D->getUnderlyingType();

   // If the C type does not have the expected size, don't import it as a stdlib
   // type.
   unsigned ClangTypeSize = ClangCtx.getTypeSize(ClangType);
   if (Bitwidth != 0 && Bitwidth != ClangTypeSize)
      return std::make_pair(Type(), "");

   // Check other expected properties of the C type.
   switch(CTypeKind) {
      case MappedCTypeKind::UnsignedInt:
         if (!ClangType->isUnsignedIntegerType())
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::SignedInt:
         if (!ClangType->isSignedIntegerType())
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::UnsignedWord:
         if (ClangTypeSize != 64 && ClangTypeSize != 32)
            return std::make_pair(Type(), "");
         if (!ClangType->isUnsignedIntegerType())
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::SignedWord:
         if (ClangTypeSize != 64 && ClangTypeSize != 32)
            return std::make_pair(Type(), "");
         if (!ClangType->isSignedIntegerType())
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::FloatIEEEsingle:
      case MappedCTypeKind::FloatIEEEdouble:
      case MappedCTypeKind::FloatX87DoubleExtended: {
         if (!ClangType->isFloatingType())
            return std::make_pair(Type(), "");

         const llvm::fltSemantics &Sem = ClangCtx.getFloatTypeSemantics(ClangType);
         switch(CTypeKind) {
            case MappedCTypeKind::FloatIEEEsingle:
               assert(Bitwidth == 32 && "FloatIEEEsingle should be 32 bits wide");
               if (&Sem != &APFloat::IEEEsingle())
                  return std::make_pair(Type(), "");
               break;

            case MappedCTypeKind::FloatIEEEdouble:
               assert(Bitwidth == 64 && "FloatIEEEdouble should be 64 bits wide");
               if (&Sem != &APFloat::IEEEdouble())
                  return std::make_pair(Type(), "");
               break;

            case MappedCTypeKind::FloatX87DoubleExtended:
               assert(Bitwidth == 80 && "FloatX87DoubleExtended should be 80 bits wide");
               if (&Sem != &APFloat::x87DoubleExtended())
                  return std::make_pair(Type(), "");
               break;

            default:
               llvm_unreachable("should see only floating point types here");
         }
      }
         break;

      case MappedCTypeKind::VaList:
         switch (ClangCtx.getTargetInfo().getBuiltinVaListKind()) {
            case clang::TargetInfo::CharPtrBuiltinVaList:
            case clang::TargetInfo::VoidPtrBuiltinVaList:
            case clang::TargetInfo::PowerABIBuiltinVaList:
            case clang::TargetInfo::AAPCSABIBuiltinVaList:
               assert(ClangCtx.getTypeSize(ClangCtx.VoidPtrTy) == ClangTypeSize &&
                      "expected va_list type to be sizeof(void *)");
               break;
            case clang::TargetInfo::AArch64ABIBuiltinVaList:
               break;
            case clang::TargetInfo::PNaClABIBuiltinVaList:
            case clang::TargetInfo::SystemZBuiltinVaList:
            case clang::TargetInfo::X86_64ABIBuiltinVaList:
               return std::make_pair(Type(), "");
         }
         break;

      case MappedCTypeKind::ObjCBool:
         if (!ClangCtx.hasSameType(ClangType, ClangCtx.ObjCBuiltinBoolTy) &&
             !(ClangCtx.getBOOLDecl() &&
               ClangCtx.hasSameType(ClangType, ClangCtx.getBOOLType())))
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::ObjCSel:
         if (!ClangCtx.hasSameType(ClangType, ClangCtx.getObjCSelType()) &&
             !ClangCtx.hasSameType(ClangType,
                                   ClangCtx.getObjCSelRedefinitionType()))
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::ObjCId:
         if (!ClangCtx.hasSameType(ClangType, ClangCtx.getObjCIdType()) &&
             !ClangCtx.hasSameType(ClangType,
                                   ClangCtx.getObjCIdRedefinitionType()))
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::ObjCClass:
         if (!ClangCtx.hasSameType(ClangType, ClangCtx.getObjCClassType()) &&
             !ClangCtx.hasSameType(ClangType,
                                   ClangCtx.getObjCClassRedefinitionType()))
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::CGFloat:
         if (!ClangType->isFloatingType())
            return std::make_pair(Type(), "");
         break;

      case MappedCTypeKind::Block:
         if (!ClangType->isBlockPointerType())
            return std::make_pair(Type(), "");
         break;
   }

   ModuleDecl *M;
   if (IsPolarphpModule)
      M = Impl.getStdlibModule();
   else
      M = Impl.getNamedModule(PolarphpModuleName);
   if (!M) {
      // User did not import the library module that contains the type we want to
      // substitute.
      *IsError = true;
      return std::make_pair(Type(), "");
   }

   Type TypePHPType = Impl.getNamedTypePHPType(M, TypePHPTypeName);
   if (!TypePHPType && !CanBeMissing) {
      // The required type is not defined in the standard library.
      *IsError = true;
      return std::make_pair(Type(), "");
   }
   return std::make_pair(TypePHPType, TypePHPTypeName);
}

static bool isNSDictionaryMethod(const clang::ObjCMethodDecl *MD,
                                 clang::Selector cmd) {
   if (MD->getSelector() != cmd)
      return false;
   if (isa<clang::ObjCInterfaceDecl>(MD->getDeclContext()))
      return false;
   if (MD->getClassInterface()->getName() != "NSDictionary")
      return false;
   return true;
}

/// Synthesize the body of \c init?(rawValue:RawType) for an imported enum.
static std::pair<BraceStmt *, bool>
synthesizeEnumRawValueConstructorBody(AbstractFunctionDecl *afd,
                                      void *context) {
   AstContext &ctx = afd->getAstContext();
   auto ctorDecl = cast<ConstructorDecl>(afd);
   auto enumDecl = static_cast<EnumDecl *>(context);
   auto selfDecl = ctorDecl->getImplicitSelfDecl();
   auto selfRef = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
      /*implicit*/true);
   selfRef->setType(LValueType::get(selfDecl->getType()));

   auto param = ctorDecl->getParameters()->get(0);
   auto paramRef = new (ctx) DeclRefExpr(param, DeclNameLoc(),
      /*implicit*/ true);
   paramRef->setType(param->getType());

   auto reinterpretCast
      = cast<FuncDecl>(
         getBuiltinValueDecl(ctx, ctx.getIdentifier("reinterpretCast")));
   auto rawTy = enumDecl->getRawType();
   auto enumTy = enumDecl->getDeclaredInterfaceType();
   SubstitutionMap subMap =
      SubstitutionMap::get(reinterpretCast->getGenericSignature(),
                           { rawTy, enumTy }, { });
   ConcreteDeclRef concreteDeclRef(reinterpretCast, subMap);
   auto reinterpretCastRef
      = new (ctx) DeclRefExpr(concreteDeclRef, DeclNameLoc(), /*implicit*/ true);
   reinterpretCastRef->setType(FunctionType::get({FunctionType::Param(rawTy)},
                                                 enumTy));

   auto reinterpreted = CallExpr::createImplicit(ctx, reinterpretCastRef,
                                                 { paramRef }, { Identifier() });
   reinterpreted->setType(enumTy);
   reinterpreted->setThrows(false);

   auto assign = new (ctx) AssignExpr(selfRef, SourceLoc(), reinterpreted,
      /*implicit*/ true);
   assign->setType(TupleType::getEmpty(ctx));

   auto result = TupleExpr::createEmpty(ctx, SourceLoc(), SourceLoc(),
      /*Implicit=*/true);
   auto ret = new (ctx) ReturnStmt(SourceLoc(), result, /*Implicit=*/true);

   auto body = BraceStmt::create(ctx, SourceLoc(), {assign, ret}, SourceLoc(),
      /*implicit*/ true);
   return { body, /*isTypeChecked=*/true };
}

// Build the init(rawValue:) initializer for an imported NS_ENUM.
//   enum NSSomeEnum: RawType {
//     init?(rawValue: RawType) {
//       self = Builtin.reinterpretCast(rawValue)
//     }
//   }
// Unlike a standard init(rawValue:) enum initializer, this does a reinterpret
// cast in order to preserve unknown or future cases from C.
static ConstructorDecl *
makeEnumRawValueConstructor(ClangImporter::Implementation &Impl,
                            EnumDecl *enumDecl) {
   AstContext &C = Impl.TypePHPContext;
   auto rawTy = enumDecl->getRawType();

   auto param = new (C) ParamDecl(SourceLoc(),
                                  SourceLoc(), C.Id_rawValue,
                                  SourceLoc(), C.Id_rawValue,
                                  enumDecl);
   param->setSpecifier(ParamSpecifier::Default);
   param->setInterfaceType(rawTy);

   auto paramPL = ParameterList::createWithoutLoc(param);

   DeclName name(C, DeclBaseName::createConstructor(), paramPL);
   auto *ctorDecl =
      new (C) ConstructorDecl(name, enumDecl->getLoc(),
         /*Failable=*/true, /*FailabilityLoc=*/SourceLoc(),
         /*Throws=*/false, /*ThrowsLoc=*/SourceLoc(),
                              paramPL,
         /*GenericParams=*/nullptr, enumDecl);
   ctorDecl->setImplicit();
   ctorDecl->setAccess(AccessLevel::Public);
   ctorDecl->setBodySynthesizer(synthesizeEnumRawValueConstructorBody, enumDecl);
   return ctorDecl;
}

/// Synthesizer callback for an enum's rawValue getter.
static std::pair<BraceStmt *, bool>
synthesizeEnumRawValueGetterBody(AbstractFunctionDecl *afd, void *context) {
   auto getterDecl = cast<AccessorDecl>(afd);
   auto enumDecl = static_cast<EnumDecl *>(context);
   auto rawTy = enumDecl->getRawType();
   auto enumTy = enumDecl->getDeclaredType();

   AstContext &ctx = getterDecl->getAstContext();
   auto *selfDecl = getterDecl->getImplicitSelfDecl();
   auto selfRef = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
      /*implicit*/true);
   selfRef->setType(selfDecl->getType());

   auto reinterpretCast
      = cast<FuncDecl>(
         getBuiltinValueDecl(ctx, ctx.getIdentifier("reinterpretCast")));
   SubstitutionMap subMap =
      SubstitutionMap::get(reinterpretCast->getGenericSignature(),
                           { enumTy, rawTy }, { });
   ConcreteDeclRef concreteDeclRef(reinterpretCast, subMap);

   auto reinterpretCastRef
      = new (ctx) DeclRefExpr(concreteDeclRef, DeclNameLoc(), /*implicit*/ true);
   reinterpretCastRef->setType(FunctionType::get({FunctionType::Param(enumTy)},
                                                 rawTy));

   auto reinterpreted = CallExpr::createImplicit(ctx, reinterpretCastRef,
                                                 { selfRef }, { Identifier() });
   reinterpreted->setType(rawTy);
   reinterpreted->setThrows(false);

   auto ret = new (ctx) ReturnStmt(SourceLoc(), reinterpreted);
   auto body = BraceStmt::create(ctx, SourceLoc(), AstNode(ret), SourceLoc(),
      /*implicit*/ true);
   return { body, /*isTypeChecked=*/true };
}

// Build the rawValue getter for an imported NS_ENUM.
//   enum NSSomeEnum: RawType {
//     var rawValue: RawType {
//       return Builtin.reinterpretCast(self)
//     }
//   }
// Unlike a standard init(rawValue:) enum initializer, this does a reinterpret
// cast in order to preserve unknown or future cases from C.
static void makeEnumRawValueGetter(ClangImporter::Implementation &Impl,
                                   EnumDecl *enumDecl,
                                   VarDecl *rawValueDecl) {
   AstContext &C = Impl.TypePHPContext;

   auto rawTy = enumDecl->getRawType();

   auto *params = ParameterList::createEmpty(C);

   auto getterDecl = AccessorDecl::create(C,
      /*FuncLoc=*/SourceLoc(),
      /*AccessorKeywordLoc=*/SourceLoc(),
                                          AccessorKind::Get,
                                          rawValueDecl,
      /*StaticLoc=*/SourceLoc(),
                                          StaticSpellingKind::None,
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr, params,
                                          TypeLoc::withoutLoc(rawTy), enumDecl);
   getterDecl->setImplicit();
   // @todo
// getterDecl->setIsObjC(false);
   getterDecl->setIsDynamic(false);
   getterDecl->setIsTransparent(false);

   getterDecl->setAccess(AccessLevel::Public);
   getterDecl->setBodySynthesizer(synthesizeEnumRawValueGetterBody, enumDecl);
   makeComputed(rawValueDecl, getterDecl, nullptr);
}

/// Synthesizer for the rawValue getter for an imported struct.
static std::pair<BraceStmt *, bool>
synthesizeStructRawValueGetterBody(AbstractFunctionDecl *afd, void *context) {
   auto getterDecl = cast<AccessorDecl>(afd);
   VarDecl *storedVar = static_cast<VarDecl *>(context);

   AstContext &ctx = getterDecl->getAstContext();
   auto *selfDecl = getterDecl->getImplicitSelfDecl();
   auto selfRef = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
      /*implicit*/true);
   selfRef->setType(selfDecl->getType());

   auto storedType = storedVar->getInterfaceType();
   auto storedRef = new (ctx) MemberRefExpr(selfRef, SourceLoc(), storedVar,
                                            DeclNameLoc(), /*Implicit=*/true,
                                            AccessSemantics::DirectToStorage);
   storedRef->setType(storedType);

   Expr *result = storedRef;

   Type computedType = getterDecl->getResultInterfaceType();
   // @todo
//   if (!computedType->isEqual(storedType)) {
//      auto bridge = new (ctx) BridgeFromObjCExpr(storedRef, computedType);
//      bridge->setType(computedType);
//
//      auto coerce = new (ctx) CoerceExpr(bridge, {}, {nullptr, computedType});
//      coerce->setType(computedType);
//
//      result = coerce;
//   }

   auto ret = new (ctx) ReturnStmt(SourceLoc(), result);
   auto body = BraceStmt::create(ctx, SourceLoc(), AstNode(ret), SourceLoc(),
      /*implicit*/ true);
   return { body, /*isTypeChecked=*/true };
}

// Build the rawValue getter for a struct type.
//
//   struct SomeType: RawRepresentable {
//     private var _rawValue: ObjCType
//     var rawValue: TypePHPType {
//       return _rawValue as TypePHPType
//     }
//   }
static AccessorDecl *makeStructRawValueGetter(
   ClangImporter::Implementation &Impl,
   StructDecl *structDecl,
   VarDecl *computedVar,
   VarDecl *storedVar) {
   assert(storedVar->hasStorage());

   AstContext &C = Impl.TypePHPContext;

   auto *params = ParameterList::createEmpty(C);

   auto computedType = computedVar->getInterfaceType();

   auto getterDecl = AccessorDecl::create(C,
      /*FuncLoc=*/SourceLoc(),
      /*AccessorKeywordLoc=*/SourceLoc(),
                                          AccessorKind::Get,
                                          computedVar,
      /*StaticLoc=*/SourceLoc(),
                                          StaticSpellingKind::None,
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr, params,
                                          TypeLoc::withoutLoc(computedType), structDecl);
   getterDecl->setImplicit();
   // @todo
//   getterDecl->setIsObjC(false);
   getterDecl->setIsDynamic(false);
   getterDecl->setIsTransparent(false);

   getterDecl->setAccess(AccessLevel::Public);
   getterDecl->setBodySynthesizer(synthesizeStructRawValueGetterBody, storedVar);
   return getterDecl;
}

static AccessorDecl *makeFieldGetterDecl(ClangImporter::Implementation &Impl,
                                         StructDecl *importedDecl,
                                         VarDecl *importedFieldDecl,
                                         ClangNode clangNode = ClangNode()) {
   auto &C = Impl.TypePHPContext;

   auto *params = ParameterList::createEmpty(C);

   auto getterType = importedFieldDecl->getInterfaceType();
   auto getterDecl = AccessorDecl::create(C,
      /*FuncLoc=*/importedFieldDecl->getLoc(),
      /*AccessorKeywordLoc=*/SourceLoc(),
                                          AccessorKind::Get,
                                          importedFieldDecl,
      /*StaticLoc=*/SourceLoc(),
                                          StaticSpellingKind::None,
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr, params,
                                          TypeLoc::withoutLoc(getterType), importedDecl, clangNode);
   getterDecl->setAccess(AccessLevel::Public);
   // @todo
//   getterDecl->setIsObjC(false);
   getterDecl->setIsDynamic(false);

   return getterDecl;
}

static AccessorDecl *makeFieldSetterDecl(ClangImporter::Implementation &Impl,
                                         StructDecl *importedDecl,
                                         VarDecl *importedFieldDecl,
                                         ClangNode clangNode = ClangNode()) {
   auto &C = Impl.TypePHPContext;
   auto newValueDecl = new (C) ParamDecl(SourceLoc(), SourceLoc(),
                                         Identifier(), SourceLoc(), C.Id_value,
                                         importedDecl);
   newValueDecl->setSpecifier(ParamSpecifier::Default);
   newValueDecl->setInterfaceType(importedFieldDecl->getInterfaceType());

   auto *params = ParameterList::createWithoutLoc(newValueDecl);

   auto voidTy = TupleType::getEmpty(C);

   auto setterDecl = AccessorDecl::create(C,
      /*FuncLoc=*/SourceLoc(),
      /*AccessorKeywordLoc=*/SourceLoc(),
                                          AccessorKind::Set,
                                          importedFieldDecl,
      /*StaticLoc=*/SourceLoc(),
                                          StaticSpellingKind::None,
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr, params,
                                          TypeLoc::withoutLoc(voidTy), importedDecl, clangNode);
   // @todo
//   setterDecl->setIsObjC(false);
   setterDecl->setIsDynamic(false);
   setterDecl->setSelfAccessKind(SelfAccessKind::Mutating);
   setterDecl->setAccess(AccessLevel::Public);

   return setterDecl;
}

/// Find the anonymous inner field declaration for the given anonymous field.
static VarDecl *findAnonymousInnerFieldDecl(VarDecl *importedFieldDecl,
                                            VarDecl *anonymousFieldDecl) {
   auto anonymousFieldType = anonymousFieldDecl->getInterfaceType();
   auto anonymousFieldTypeDecl
      = anonymousFieldType->getStructOrBoundGenericStruct();

   for (auto decl : anonymousFieldTypeDecl->lookupDirect(
      importedFieldDecl->getName())) {
      if (isa<VarDecl>(decl)) {
         return cast<VarDecl>(decl);
      }
   }

   llvm_unreachable("couldn't find anonymous inner field decl");
}

/// Synthesize the getter body for an indirect field.
static std::pair<BraceStmt *, bool>
synthesizeIndirectFieldGetterBody(AbstractFunctionDecl *afd, void *context) {
   auto getterDecl = cast<AccessorDecl>(afd);
   auto anonymousFieldDecl = static_cast<VarDecl *>(context);

   AstContext &ctx = getterDecl->getAstContext();
   auto selfDecl = getterDecl->getImplicitSelfDecl();
   Expr *expr = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
      /*implicit*/true);
   expr->setType(selfDecl->getInterfaceType());

   expr = new (ctx) MemberRefExpr(expr, SourceLoc(), anonymousFieldDecl,
                                  DeclNameLoc(), /*implicit*/true);
   expr->setType(anonymousFieldDecl->getInterfaceType());

   auto importedFieldDecl = cast<VarDecl>(getterDecl->getStorage());
   auto anonymousInnerFieldDecl =
      findAnonymousInnerFieldDecl(importedFieldDecl, anonymousFieldDecl);
   expr = new (ctx) MemberRefExpr(expr, SourceLoc(), anonymousInnerFieldDecl,
                                  DeclNameLoc(), /*implicit*/true);
   expr->setType(anonymousInnerFieldDecl->getInterfaceType());

   auto ret = new (ctx) ReturnStmt(SourceLoc(), expr);
   auto body = BraceStmt::create(ctx, SourceLoc(), AstNode(ret), SourceLoc(),
      /*implicit*/ true);
   return { body, /*isTypeChecked=*/true };
}

/// Synthesize the setter body for an indirect field.
static std::pair<BraceStmt *, bool>
synthesizeIndirectFieldSetterBody(AbstractFunctionDecl *afd, void *context) {
   auto setterDecl = cast<AccessorDecl>(afd);
   auto anonymousFieldDecl = static_cast<VarDecl *>(context);

   AstContext &ctx = setterDecl->getAstContext();
   auto selfDecl = setterDecl->getImplicitSelfDecl();
   Expr *lhs = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
      /*implicit*/true);
   lhs->setType(LValueType::get(selfDecl->getInterfaceType()));

   lhs = new (ctx) MemberRefExpr(lhs, SourceLoc(), anonymousFieldDecl,
                                 DeclNameLoc(), /*implicit*/true);
   lhs->setType(LValueType::get(anonymousFieldDecl->getInterfaceType()));

   auto importedFieldDecl = cast<VarDecl>(setterDecl->getStorage());
   auto anonymousInnerFieldDecl =
      findAnonymousInnerFieldDecl(importedFieldDecl, anonymousFieldDecl);

   lhs = new (ctx) MemberRefExpr(lhs, SourceLoc(), anonymousInnerFieldDecl,
                                 DeclNameLoc(), /*implicit*/true);
   lhs->setType(LValueType::get(anonymousInnerFieldDecl->getInterfaceType()));

   auto newValueDecl = setterDecl->getParameters()->get(0);

   auto rhs = new (ctx) DeclRefExpr(newValueDecl, DeclNameLoc(),
      /*implicit*/ true);
   rhs->setType(newValueDecl->getInterfaceType());

   auto assign = new (ctx) AssignExpr(lhs, SourceLoc(), rhs, /*implicit*/true);
   assign->setType(TupleType::getEmpty(ctx));

   auto body = BraceStmt::create(ctx, SourceLoc(), { assign }, SourceLoc(),
      /*implicit*/ true);
   return { body, /*isTypeChecked=*/true };
}

/// Build the indirect field getter and setter.
///
/// \code
/// struct SomeImportedIndirectField {
///   struct __Unnamed_struct___Anonymous_field_1 {
///     var myField : Int
///   }
///   var __Anonymous_field_1 : __Unnamed_struct___Anonymous_field_1
///   var myField : Int {
///     get {
///       __Anonymous_field_1.myField
///     }
///     set(newValue) {
///       __Anonymous_field_1.myField = newValue
///     }
///   }
/// }
/// \endcode
///
/// \returns a pair of getter and setter function decls.
static std::pair<AccessorDecl *, AccessorDecl *>
makeIndirectFieldAccessors(ClangImporter::Implementation &Impl,
                           const clang::IndirectFieldDecl *indirectField,
                           ArrayRef<VarDecl *> members,
                           StructDecl *importedStructDecl,
                           VarDecl *importedFieldDecl) {
   auto &C = Impl.TypePHPContext;

   auto getterDecl = makeFieldGetterDecl(Impl,
                                         importedStructDecl,
                                         importedFieldDecl);
   getterDecl->getAttrs().add(new (C) TransparentAttr(/*implicit*/ true));

   auto setterDecl = makeFieldSetterDecl(Impl,
                                         importedStructDecl,
                                         importedFieldDecl);
   setterDecl->getAttrs().add(new (C) TransparentAttr(/*implicit*/ true));

   makeComputed(importedFieldDecl, getterDecl, setterDecl);

   auto containingField = indirectField->chain().front();
   VarDecl *anonymousFieldDecl = nullptr;

   // Reverse scan of the members because indirect field are generated just
   // after the corresponding anonymous type, so a reverse scan allows
   // switching from O(n) to O(1) here.
   for (auto decl : reverse(members)) {
      if (decl->getClangDecl() == containingField) {
         anonymousFieldDecl = cast<VarDecl>(decl);
         break;
      }
   }
   assert (anonymousFieldDecl && "anonymous field not generated");
   getterDecl->setBodySynthesizer(synthesizeIndirectFieldGetterBody,
                                  anonymousFieldDecl);
   setterDecl->setBodySynthesizer(synthesizeIndirectFieldSetterBody,
                                  anonymousFieldDecl);

   return { getterDecl, setterDecl };
}

/// Synthesizer for the body of a union field getter.
static std::pair<BraceStmt *, bool>
synthesizeUnionFieldGetterBody(AbstractFunctionDecl *afd, void *context) {
   auto getterDecl = cast<AccessorDecl>(afd);
   AstContext &ctx = getterDecl->getAstContext();
   auto importedFieldDecl = static_cast<VarDecl *>(context);

   auto selfDecl = getterDecl->getImplicitSelfDecl();

   auto selfRef = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
      /*implicit*/ true);
   selfRef->setType(selfDecl->getInterfaceType());

   auto reinterpretCast = cast<FuncDecl>(getBuiltinValueDecl(
      ctx, ctx.getIdentifier("reinterpretCast")));

   ConcreteDeclRef reinterpretCastRef(
      reinterpretCast,
      SubstitutionMap::get(reinterpretCast->getGenericSignature(),
                           {selfDecl->getInterfaceType(),
                            importedFieldDecl->getInterfaceType()},
                           ArrayRef<InterfaceConformanceRef>()));
   auto reinterpretCastRefExpr
      = new (ctx) DeclRefExpr(reinterpretCastRef, DeclNameLoc(),
         /*implicit*/ true);
   reinterpretCastRefExpr->setType(
      FunctionType::get(
         AnyFunctionType::Param(selfDecl->getInterfaceType()),
         importedFieldDecl->getInterfaceType()));

   auto reinterpreted = CallExpr::createImplicit(ctx, reinterpretCastRefExpr,
                                                 { selfRef },
                                                 { Identifier() });
   reinterpreted->setType(importedFieldDecl->getInterfaceType());
   reinterpreted->setThrows(false);
   auto ret = new (ctx) ReturnStmt(SourceLoc(), reinterpreted);
   auto body = BraceStmt::create(ctx, SourceLoc(), AstNode(ret), SourceLoc(),
      /*implicit*/ true);
   return { body, /*isTypeChecked=*/true };
}

/// Synthesizer for the body of a union field setter.
static std::pair<BraceStmt *, bool>
synthesizeUnionFieldSetterBody(AbstractFunctionDecl *afd, void *context) {
   auto setterDecl = cast<AccessorDecl>(afd);
   AstContext &ctx = setterDecl->getAstContext();

   auto inoutSelfDecl = setterDecl->getImplicitSelfDecl();

   auto inoutSelfRef = new (ctx) DeclRefExpr(inoutSelfDecl, DeclNameLoc(),
      /*implicit*/ true);
   inoutSelfRef->setType(LValueType::get(inoutSelfDecl->getInterfaceType()));
   auto inoutSelf = new (ctx) InOutExpr(SourceLoc(), inoutSelfRef,
                                        setterDecl->mapTypeIntoContext(inoutSelfDecl->getValueInterfaceType()),
      /*implicit*/ true);
   inoutSelf->setType(InOutType::get(inoutSelfDecl->getInterfaceType()));

   auto newValueDecl = setterDecl->getParameters()->get(0);

   auto newValueRef = new (ctx) DeclRefExpr(newValueDecl, DeclNameLoc(),
      /*implicit*/ true);
   newValueRef->setType(newValueDecl->getInterfaceType());

   auto addressofFn = cast<FuncDecl>(getBuiltinValueDecl(
      ctx, ctx.getIdentifier("addressof")));
   ConcreteDeclRef addressofFnRef(addressofFn,
                                  SubstitutionMap::get(addressofFn->getGenericSignature(),
                                                       {inoutSelfDecl->getInterfaceType()},
                                                       ArrayRef<InterfaceConformanceRef>()));
   auto addressofFnRefExpr
      = new (ctx) DeclRefExpr(addressofFnRef, DeclNameLoc(), /*implicit*/ true);
   addressofFnRefExpr->setType(
      FunctionType::get(
         AnyFunctionType::Param(inoutSelfDecl->getInterfaceType(),
                                Identifier(),
                                ParameterTypeFlags().withInOut(true)),
         ctx.TheRawPointerType));
   auto selfPointer = CallExpr::createImplicit(ctx, addressofFnRefExpr,
                                               { inoutSelf },
                                               { Identifier() });
   selfPointer->setType(ctx.TheRawPointerType);
   selfPointer->setThrows(false);

   auto initializeFn = cast<FuncDecl>(getBuiltinValueDecl(
      ctx, ctx.getIdentifier("initialize")));
   ConcreteDeclRef initializeFnRef(initializeFn,
                                   SubstitutionMap::get(initializeFn->getGenericSignature(),
                                                        {newValueDecl->getInterfaceType()},
                                                        ArrayRef<InterfaceConformanceRef>()));
   auto initializeFnRefExpr
      = new (ctx) DeclRefExpr(initializeFnRef, DeclNameLoc(), /*implicit*/ true);
   initializeFnRefExpr->setType(
      FunctionType::get({AnyFunctionType::Param(newValueDecl->getInterfaceType()),
                         AnyFunctionType::Param(ctx.TheRawPointerType)},
                        TupleType::getEmpty(ctx)));
   auto initialize = CallExpr::createImplicit(ctx, initializeFnRefExpr,
                                              { newValueRef, selfPointer },
                                              { Identifier(), Identifier() });
   initialize->setType(TupleType::getEmpty(ctx));
   initialize->setThrows(false);

   auto body = BraceStmt::create(ctx, SourceLoc(), { initialize }, SourceLoc(),
      /*implicit*/ true);
   return { body, /*isTypeChecked=*/true };
}

/// Build the union field getter and setter.
///
/// \code
/// struct SomeImportedUnion {
///   var myField: Int {
///     get {
///       return Builtin.reinterpretCast(self)
///     }
///     set(newValue) {
///       Builtin.initialize(Builtin.addressof(self), newValue))
///     }
///   }
/// }
/// \endcode
///
/// \returns a pair of the getter and setter function decls.
static std::pair<AccessorDecl *, AccessorDecl *>
makeUnionFieldAccessors(ClangImporter::Implementation &Impl,
                        StructDecl *importedUnionDecl,
                        VarDecl *importedFieldDecl) {
   auto &C = Impl.TypePHPContext;

   auto getterDecl = makeFieldGetterDecl(Impl,
                                         importedUnionDecl,
                                         importedFieldDecl);
   getterDecl->setBodySynthesizer(synthesizeUnionFieldGetterBody,
                                  importedFieldDecl);
   getterDecl->getAttrs().add(new (C) TransparentAttr(/*implicit*/ true));

   auto setterDecl = makeFieldSetterDecl(Impl,
                                         importedUnionDecl,
                                         importedFieldDecl);
   setterDecl->setBodySynthesizer(synthesizeUnionFieldSetterBody,
                                  importedFieldDecl);
   setterDecl->getAttrs().add(new (C) TransparentAttr(/*implicit*/ true));

   makeComputed(importedFieldDecl, getterDecl, setterDecl);
   return { getterDecl, setterDecl };
}

static clang::DeclarationName
getAccessorDeclarationName(clang::ASTContext &Ctx,
                           StructDecl *structDecl,
                           VarDecl *fieldDecl,
                           const char *suffix) {
   std::string id;
   llvm::raw_string_ostream IdStream(id);
   mangle::AstMangler mangler;
   IdStream << "$" << mangler.mangleDeclAsUSR(structDecl, "")
            << "$" << fieldDecl->getName()
            << "$" << suffix;

   return clang::DeclarationName(&Ctx.Idents.get(IdStream.str()));
}

/// Build the bitfield getter and setter using Clang.
///
/// \code
/// static inline int get(RecordType self) {
///   return self.field;
/// }
/// static inline void set(int newValue, RecordType *self) {
///   self->field = newValue;
/// }
/// \endcode
///
/// \returns a pair of the getter and setter function decls.
static std::pair<FuncDecl *, FuncDecl *>
makeBitFieldAccessors(ClangImporter::Implementation &Impl,
                      clang::RecordDecl *structDecl,
                      StructDecl *importedStructDecl,
                      clang::FieldDecl *fieldDecl,
                      VarDecl *importedFieldDecl) {
   clang::ASTContext &Ctx = Impl.getClangAstContext();

   // Getter: static inline FieldType get(RecordType self);
   auto recordType = Ctx.getRecordType(structDecl);
   auto recordPointerType = Ctx.getPointerType(recordType);
   auto fieldType = fieldDecl->getType();

   auto cGetterName = getAccessorDeclarationName(Ctx, importedStructDecl,
                                                 importedFieldDecl, "getter");
   auto cGetterType = Ctx.getFunctionType(fieldDecl->getType(),
                                          recordType,
                                          clang::FunctionProtoType::ExtProtoInfo());
   auto cGetterTypeInfo = Ctx.getTrivialTypeSourceInfo(cGetterType);
   auto cGetterDecl = clang::FunctionDecl::Create(Ctx,
                                                  structDecl->getDeclContext(),
                                                  clang::SourceLocation(),
                                                  clang::SourceLocation(),
                                                  cGetterName,
                                                  cGetterType,
                                                  cGetterTypeInfo,
                                                  clang::SC_Static);
   cGetterDecl->setImplicitlyInline();
   assert(!cGetterDecl->isExternallyVisible());

   auto getterDecl = makeFieldGetterDecl(Impl,
                                         importedStructDecl,
                                         importedFieldDecl,
                                         cGetterDecl);

   // Setter: static inline void set(FieldType newValue, RecordType *self);
   SmallVector<clang::QualType, 8> cSetterParamTypes;
   cSetterParamTypes.push_back(fieldType);
   cSetterParamTypes.push_back(recordPointerType);

   auto cSetterName = getAccessorDeclarationName(Ctx, importedStructDecl,
                                                 importedFieldDecl, "setter");
   auto cSetterType = Ctx.getFunctionType(Ctx.VoidTy,
                                          cSetterParamTypes,
                                          clang::FunctionProtoType::ExtProtoInfo());
   auto cSetterTypeInfo = Ctx.getTrivialTypeSourceInfo(cSetterType);

   auto cSetterDecl = clang::FunctionDecl::Create(Ctx,
                                                  structDecl->getDeclContext(),
                                                  clang::SourceLocation(),
                                                  clang::SourceLocation(),
                                                  cSetterName,
                                                  cSetterType,
                                                  cSetterTypeInfo,
                                                  clang::SC_Static);
   cSetterDecl->setImplicitlyInline();
   assert(!cSetterDecl->isExternallyVisible());

   auto setterDecl = makeFieldSetterDecl(Impl,
                                         importedStructDecl,
                                         importedFieldDecl,
                                         cSetterDecl);

   makeComputed(importedFieldDecl, getterDecl, setterDecl);

   // Synthesize the getter body
   {
      auto cGetterSelfId = nullptr;
      auto recordTypeInfo = Ctx.getTrivialTypeSourceInfo(recordType);
      auto cGetterSelf = clang::ParmVarDecl::Create(Ctx, cGetterDecl,
                                                    clang::SourceLocation(),
                                                    clang::SourceLocation(),
                                                    cGetterSelfId,
                                                    recordType,
                                                    recordTypeInfo,
                                                    clang::SC_None,
                                                    nullptr);
      cGetterDecl->setParams(cGetterSelf);

      auto cGetterSelfExpr = new (Ctx) clang::DeclRefExpr(Ctx, cGetterSelf, false,
                                                          recordType,
                                                          clang::VK_RValue,
                                                          clang::SourceLocation());
      auto cGetterExpr = clang::MemberExpr::CreateImplicit(Ctx,
                                                           cGetterSelfExpr,
         /*isarrow=*/ false,
                                                           fieldDecl,
                                                           fieldType,
                                                           clang::VK_RValue,
                                                           clang::OK_BitField);


      auto cGetterBody = clang::ReturnStmt::Create(Ctx, clang::SourceLocation(),
                                                   cGetterExpr,
                                                   nullptr);
      cGetterDecl->setBody(cGetterBody);
   }

   // Synthesize the setter body
   {
      SmallVector<clang::ParmVarDecl *, 2> cSetterParams;
      auto fieldTypeInfo = Ctx.getTrivialTypeSourceInfo(fieldType);
      auto cSetterValue = clang::ParmVarDecl::Create(Ctx, cSetterDecl,
                                                     clang::SourceLocation(),
                                                     clang::SourceLocation(),
         /* nameID? */ nullptr,
                                                     fieldType,
                                                     fieldTypeInfo,
                                                     clang::SC_None,
                                                     nullptr);
      cSetterParams.push_back(cSetterValue);
      auto recordPointerTypeInfo = Ctx.getTrivialTypeSourceInfo(recordPointerType);
      auto cSetterSelf = clang::ParmVarDecl::Create(Ctx, cSetterDecl,
                                                    clang::SourceLocation(),
                                                    clang::SourceLocation(),
         /* nameID? */ nullptr,
                                                    recordPointerType,
                                                    recordPointerTypeInfo,
                                                    clang::SC_None,
                                                    nullptr);
      cSetterParams.push_back(cSetterSelf);
      cSetterDecl->setParams(cSetterParams);

      auto cSetterSelfExpr = new (Ctx) clang::DeclRefExpr(Ctx, cSetterSelf, false,
                                                          recordPointerType,
                                                          clang::VK_RValue,
                                                          clang::SourceLocation());

      auto cSetterMemberExpr = clang::MemberExpr::CreateImplicit(Ctx,
                                                                 cSetterSelfExpr,
         /*isarrow=*/true,
                                                                 fieldDecl,
                                                                 fieldType,
                                                                 clang::VK_LValue,
                                                                 clang::OK_BitField);

      auto cSetterValueExpr = new (Ctx) clang::DeclRefExpr(Ctx, cSetterValue, false,
                                                           fieldType,
                                                           clang::VK_RValue,
                                                           clang::SourceLocation());

      auto cSetterExpr = new (Ctx) clang::BinaryOperator(cSetterMemberExpr,
                                                         cSetterValueExpr,
                                                         clang::BO_Assign,
                                                         fieldType,
                                                         clang::VK_RValue,
                                                         clang::OK_Ordinary,
                                                         clang::SourceLocation(),
                                                         clang::FPOptions());

      cSetterDecl->setBody(cSetterExpr);
   }

   return { getterDecl, setterDecl };
}

/// Synthesize the body for an struct default initializer.
static std::pair<BraceStmt *, bool>
synthesizeStructDefaultConstructorBody(AbstractFunctionDecl *afd,
                                       void *context) {
   auto constructor = cast<ConstructorDecl>(afd);
   AstContext &ctx = constructor->getAstContext();
   auto structDecl = static_cast<StructDecl *>(context);

   // Use a builtin to produce a zero initializer, and assign it to self.

   // Construct the left-hand reference to self.
   auto *selfDecl = constructor->getImplicitSelfDecl();
   Expr *lhs = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(), /*Implicit=*/true);
   auto selfType = structDecl->getDeclaredInterfaceType();
   lhs->setType(LValueType::get(selfType));

   auto emptyTuple = TupleType::getEmpty(ctx);

   // Construct the right-hand call to Builtin.zeroInitializer.
   Identifier zeroInitID = ctx.getIdentifier("zeroInitializer");
   auto zeroInitializerFunc =
      cast<FuncDecl>(getBuiltinValueDecl(ctx, zeroInitID));
   SubstitutionMap subMap =
      SubstitutionMap::get(zeroInitializerFunc->getGenericSignature(),
                           llvm::makeArrayRef(selfType), { });
   ConcreteDeclRef concreteDeclRef(zeroInitializerFunc, subMap);
   auto zeroInitializerRef =
      new (ctx) DeclRefExpr(concreteDeclRef, DeclNameLoc(), /*implicit*/ true);
   zeroInitializerRef->setType(FunctionType::get({}, selfType));

   auto call = CallExpr::createImplicit(ctx, zeroInitializerRef, {}, {});
   call->setType(selfType);
   call->setThrows(false);

   auto assign = new (ctx) AssignExpr(lhs, SourceLoc(), call, /*implicit*/ true);
   assign->setType(emptyTuple);

   auto result = TupleExpr::createEmpty(ctx, SourceLoc(), SourceLoc(),
      /*Implicit=*/true);
   result->setType(emptyTuple);

   auto ret = new (ctx) ReturnStmt(SourceLoc(), result, /*Implicit=*/true);

   // Create the function body.
   auto body = BraceStmt::create(ctx, SourceLoc(), {assign, ret}, SourceLoc());
   return { body, /*isTypeChecked=*/true };
}

/// Create a default constructor that initializes a struct to zero.
static ConstructorDecl *
createDefaultConstructor(ClangImporter::Implementation &Impl,
                         StructDecl *structDecl) {
   auto &context = Impl.TypePHPContext;

   auto emptyPL = ParameterList::createEmpty(context);

   // Create the constructor.
   DeclName name(context, DeclBaseName::createConstructor(), emptyPL);
   auto constructor = new (context) ConstructorDecl(
      name, structDecl->getLoc(),
      /*Failable=*/false, /*FailabilityLoc=*/SourceLoc(),
      /*Throws=*/false, /*ThrowsLoc=*/SourceLoc(), emptyPL,
      /*GenericParams=*/nullptr, structDecl);

   constructor->setAccess(AccessLevel::Public);

   // Mark the constructor transparent so that we inline it away completely.
   constructor->getAttrs().add(new (context) TransparentAttr(/*implicit*/ true));

   constructor->setBodySynthesizer(synthesizeStructDefaultConstructorBody,
                                   structDecl);

   // We're done.
   return constructor;
}

/// Synthesizer callback for the body of a struct value constructor.
static std::pair<BraceStmt *, bool>
synthesizeValueConstructorBody(AbstractFunctionDecl *afd, void *context) {
   auto constructor = cast<ConstructorDecl>(afd);
   ArrayRef<VarDecl *> members(static_cast<VarDecl **>(context) + 1,
                               static_cast<uintptr_t*>(context)[0]);

   AstContext &ctx = constructor->getAstContext();

   // Assign all of the member variables appropriately.
   SmallVector<AstNode, 4> stmts;

   auto *selfDecl = constructor->getImplicitSelfDecl();

   // To keep DI happy, initialize stored properties before computed.
   auto parameters = constructor->getParameters();
   for (unsigned pass = 0; pass < 2; pass++) {
      unsigned paramPos = 0;

      for (unsigned i = 0, e = members.size(); i < e; i++) {
         auto var = members[i];

         if (var->hasClangNode() &&
             isa<clang::IndirectFieldDecl>(var->getClangDecl()))
            continue;

         if (var->hasStorage() == (pass != 0)) {
            paramPos++;
            continue;
         }

         // Construct left-hand side.
         Expr *lhs = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
            /*Implicit=*/true);
         lhs->setType(LValueType::get(selfDecl->getType()));

         auto semantics = (var->hasStorage()
                           ? AccessSemantics::DirectToStorage
                           : AccessSemantics::Ordinary);

         lhs = new (ctx) MemberRefExpr(lhs, SourceLoc(), var, DeclNameLoc(),
            /*Implicit=*/true, semantics);
         lhs->setType(LValueType::get(var->getType()));

         // Construct right-hand side.
         auto rhs = new (ctx) DeclRefExpr(parameters->get(paramPos),
                                          DeclNameLoc(), /*Implicit=*/true);
         rhs->setType(parameters->get(paramPos)->getType());

         // Add assignment.
         auto assign = new (ctx) AssignExpr(lhs, SourceLoc(), rhs,
            /*Implicit=*/true);
         assign->setType(TupleType::getEmpty(ctx));

         stmts.push_back(assign);
         paramPos++;
      }
   }

   auto result = TupleExpr::createEmpty(ctx, SourceLoc(), SourceLoc(),
      /*Implicit=*/true);
   result->setType(TupleType::getEmpty(ctx));

   auto ret = new (ctx) ReturnStmt(SourceLoc(), result, /*Implicit=*/true);
   stmts.push_back(ret);

   // Create the function body.
   auto body = BraceStmt::create(ctx, SourceLoc(), stmts, SourceLoc());
   return { body, /*isTypeChecked=*/true };
}

/// Create a constructor that initializes a struct from its members.
static ConstructorDecl *
createValueConstructor(ClangImporter::Implementation &Impl,
                       StructDecl *structDecl, ArrayRef<VarDecl *> members,
                       bool wantCtorParamNames, bool wantBody) {
   auto &context = Impl.TypePHPContext;

   // Construct the set of parameters from the list of members.
   SmallVector<ParamDecl *, 8> valueParameters;
   for (auto var : members) {
      bool generateParamName = wantCtorParamNames;

      if (var->hasClangNode()) {
         // TODO create value constructor with indirect fields instead of the
         // generated __Anonymous_field.
         if (isa<clang::IndirectFieldDecl>(var->getClangDecl()))
            continue;

         if (auto clangField = dyn_cast<clang::FieldDecl>(var->getClangDecl()))
            if (clangField->isAnonymousStructOrUnion())
               generateParamName = false;
      }

      Identifier argName = generateParamName ? var->getName() : Identifier();
      auto param = new (context)
         ParamDecl(SourceLoc(), SourceLoc(), argName,
                   SourceLoc(), var->getName(), structDecl);
      param->setSpecifier(ParamSpecifier::Default);
      param->setInterfaceType(var->getInterfaceType());
      Impl.recordImplicitUnwrapForDecl(param, var->isImplicitlyUnwrappedOptional());

      // Don't allow the parameter to accept temporary pointer conversions.
      param->setNonEphemeralIfPossible();

      valueParameters.push_back(param);
   }

   auto *paramList = ParameterList::create(context, valueParameters);

   // Create the constructor
   DeclName name(context, DeclBaseName::createConstructor(), paramList);
   auto constructor = new (context) ConstructorDecl(
      name, structDecl->getLoc(),
      /*Failable=*/false, /*FailabilityLoc=*/SourceLoc(),
      /*Throws=*/false, /*ThrowsLoc=*/SourceLoc(), paramList,
      /*GenericParams=*/nullptr, structDecl);

   constructor->setAccess(AccessLevel::Public);

   // Make the constructor transparent so we inline it away completely.
   constructor->getAttrs().add(new (context) TransparentAttr(/*implicit*/ true));

   if (wantBody) {
      auto memberMemory =
         context.AllocateUninitialized<uintptr_t>(members.size() + 1);
      memberMemory[0] = members.size();
      for (unsigned i : indices(members)) {
         memberMemory[i+1] = reinterpret_cast<uintptr_t>(members[i]);
      }
      constructor->setBodySynthesizer(synthesizeValueConstructorBody,
                                      memberMemory.data());
   }

   // We're done.
   return constructor;
}

static void addSynthesizedInterfaceAttrs(
   ClangImporter::Implementation &Impl,
   NominalTypeDecl *nominal,
   ArrayRef<KnownInterfaceKind> synthesizedInterfaceAttrs) {
   for (auto kind : synthesizedInterfaceAttrs) {
      nominal->getAttrs().add(new (Impl.TypePHPContext)
                                 SynthesizedInterfaceAttr(kind, &Impl));
   }
}

/// Add a synthesized typealias to the given nominal type.
static void addSynthesizedTypealias(NominalTypeDecl *nominal, Identifier name,
                                    Type underlyingType) {
   auto &ctx = nominal->getAstContext();

   auto typealias = new (ctx) TypeAliasDecl(SourceLoc(), SourceLoc(),
                                            name, SourceLoc(),
                                            nullptr, nominal);
   typealias->setUnderlyingType(underlyingType);
   typealias->setAccess(AccessLevel::Public);
   typealias->setImplicit();

   nominal->addMember(typealias);
}

/// Make a struct declaration into a raw-value-backed struct
///
/// \param structDecl the struct to make a raw value for
/// \param underlyingType the type of the raw value
/// \param synthesizedInterfaceAttrs synthesized protocol attributes to add
/// \param setterAccess the access level of the raw value's setter
///
/// This will perform most of the work involved in making a new Swift struct
/// be backed by a raw value. This will populated derived protocols and
/// synthesized protocols, add the new variable and pattern bindings, and
/// create the inits parameterized over a raw value
///
static void makeStructRawValued(
   ClangImporter::Implementation &Impl, StructDecl *structDecl,
   Type underlyingType, ArrayRef<KnownInterfaceKind> synthesizedInterfaceAttrs,
   MakeStructRawValuedOptions options = getDefaultMakeStructRawValuedOptions(),
   AccessLevel setterAccess = AccessLevel::Private) {
   auto &ctx = Impl.TypePHPContext;

   addSynthesizedInterfaceAttrs(Impl, structDecl, synthesizedInterfaceAttrs);

   // Create a variable to store the underlying value.
   VarDecl *var;
   PatternBindingDecl *patternBinding;
   auto introducer = (options.contains(MakeStructRawValuedFlags::IsLet)
                      ? VarDecl::Introducer::Let
                      : VarDecl::Introducer::Var);
   std::tie(var, patternBinding) = createVarWithPattern(
      ctx, structDecl, ctx.Id_rawValue, underlyingType, introducer,
      options.contains(MakeStructRawValuedFlags::IsImplicit),
      AccessLevel::Public,
      setterAccess);

   assert(var->hasStorage());

   // Create constructors to initialize that value from a value of the
   // underlying type.
   if (options.contains(MakeStructRawValuedFlags::MakeUnlabeledValueInit))
      structDecl->addMember(
         createValueConstructor(Impl, structDecl, var,
            /*wantCtorParamNames=*/false,
            /*wantBody=*/true));

   auto *initRawValue =
      createValueConstructor(Impl, structDecl, var,
         /*wantCtorParamNames=*/true,
         /*wantBody=*/true);
   structDecl->addMember(initRawValue);
   structDecl->addMember(patternBinding);
   structDecl->addMember(var);

   addSynthesizedTypealias(structDecl, ctx.Id_RawValue, underlyingType);
   Impl.RawTypes[structDecl] = underlyingType;
}

/// Synthesizer callback for a raw value bridging constructor body.
static std::pair<BraceStmt *, bool>
synthesizeRawValueBridgingConstructorBody(AbstractFunctionDecl *afd,
                                          void *context) {
   auto init = cast<ConstructorDecl>(afd);
   VarDecl *storedRawValue = static_cast<VarDecl *>(context);

   AstContext &ctx = init->getAstContext();

   auto selfDecl = init->getImplicitSelfDecl();
   auto storedType = storedRawValue->getInterfaceType();

   // Construct left-hand side.
   Expr *lhs = new (ctx) DeclRefExpr(selfDecl, DeclNameLoc(),
      /*Implicit=*/true);
   lhs->setType(LValueType::get(selfDecl->getType()));

   lhs = new (ctx) MemberRefExpr(lhs, SourceLoc(), storedRawValue,
                                 DeclNameLoc(), /*Implicit=*/true,
                                 AccessSemantics::DirectToStorage);
   lhs->setType(LValueType::get(storedType));

   // Construct right-hand side.
   // FIXME: get the parameter from the init, and plug it in here.
   auto *paramDecl = init->getParameters()->get(0);
   auto *paramRef = new (ctx) DeclRefExpr(
      paramDecl, DeclNameLoc(), /*Implicit=*/true);
   paramRef->setType(paramDecl->getType());

   Expr *rhs = paramRef;
   // @todo
//   if (!storedRawValue->getInterfaceType()->isEqual(paramDecl->getType())) {
//      auto bridge = new (ctx) BridgeToObjCExpr(paramRef, storedType);
//      bridge->setType(storedType);
//
//      auto coerce = new (ctx) CoerceExpr(bridge, SourceLoc(),
//                                         {nullptr, storedType});
//      coerce->setType(storedType);
//
//      rhs = coerce;
//   }

   // Add assignment.
   auto assign = new (ctx) AssignExpr(lhs, SourceLoc(), rhs,
      /*Implicit=*/true);
   assign->setType(TupleType::getEmpty(ctx));

   auto result = TupleExpr::createEmpty(ctx, SourceLoc(), SourceLoc(),
      /*Implicit=*/true);
   auto ret = new (ctx) ReturnStmt(SourceLoc(), result, /*Implicit=*/true);

   auto body = BraceStmt::create(ctx, SourceLoc(), {assign, ret}, SourceLoc());
   return { body, /*isTypeChecked=*/true };
}

/// Create a rawValue-ed constructor that bridges to its underlying storage.
static ConstructorDecl *createRawValueBridgingConstructor(
   ClangImporter::Implementation &Impl, StructDecl *structDecl,
   VarDecl *computedRawValue, VarDecl *storedRawValue, bool wantLabel,
   bool wantBody) {
   auto init = createValueConstructor(Impl, structDecl, computedRawValue,
      /*wantCtorParamNames=*/wantLabel,
      /*wantBody=*/false);
   // Insert our custom init body
   if (wantBody) {
      init->setBodySynthesizer(synthesizeRawValueBridgingConstructorBody,
                               storedRawValue);
   }

   return init;
}

/// Make a struct declaration into a raw-value-backed struct, with
/// bridged computed rawValue property which differs from stored backing
///
/// \param structDecl the struct to make a raw value for
/// \param storedUnderlyingType the type of the stored raw value
/// \param bridgedType the type of the 'rawValue' computed property bridge
/// \param synthesizedInterfaceAttrs synthesized protocol attributes to add
///
/// This will perform most of the work involved in making a new Swift struct
/// be backed by a stored raw value and computed raw value of bridged type.
/// This will populated derived protocols and synthesized protocols, add the
/// new variable and pattern bindings, and create the inits parameterized
/// over a bridged type that will cast to the stored type, as appropriate.
///
static void makeStructRawValuedWithBridge(
   ClangImporter::Implementation &Impl, StructDecl *structDecl,
   Type storedUnderlyingType, Type bridgedType,
   ArrayRef<KnownInterfaceKind> synthesizedInterfaceAttrs,
   bool makeUnlabeledValueInit = false) {
   auto &ctx = Impl.TypePHPContext;

   addSynthesizedInterfaceAttrs(Impl, structDecl, synthesizedInterfaceAttrs);

   auto storedVarName = ctx.getIdentifier("_rawValue");
   auto computedVarName = ctx.Id_rawValue;

   // Create a variable to store the underlying value.
   VarDecl *storedVar;
   PatternBindingDecl *storedPatternBinding;
   std::tie(storedVar, storedPatternBinding) = createVarWithPattern(
      ctx, structDecl, storedVarName, storedUnderlyingType,
      VarDecl::Introducer::Var, /*isImplicit=*/true,
      AccessLevel::Private,
      AccessLevel::Private);

   // Create a computed value variable.
   auto computedVar = new (ctx) VarDecl(
      /*IsStatic*/false, VarDecl::Introducer::Var, /*IsCaptureList*/false,
                  SourceLoc(), computedVarName, structDecl);
   computedVar->setInterfaceType(bridgedType);
   computedVar->setImplicit();
   computedVar->setAccess(AccessLevel::Public);
   computedVar->setSetterAccess(AccessLevel::Private);

   // Create the getter for the computed value variable.
   auto computedVarGetter = makeStructRawValueGetter(
      Impl, structDecl, computedVar, storedVar);
   makeComputed(computedVar, computedVarGetter, nullptr);

   // Create a pattern binding to describe the variable.
   Pattern *computedVarPattern = createTypedNamedPattern(computedVar);
   auto *computedPatternBinding = PatternBindingDecl::createImplicit(
      ctx, StaticSpellingKind::None, computedVarPattern, /*InitExpr*/ nullptr,
      structDecl);

   auto init = createRawValueBridgingConstructor(Impl, structDecl, computedVar,
                                                 storedVar,
      /*wantLabel*/ true,
      /*wantBody*/ true);

   ConstructorDecl *unlabeledCtor = nullptr;
   if (makeUnlabeledValueInit)
      unlabeledCtor = createRawValueBridgingConstructor(
         Impl, structDecl, computedVar, storedVar,
         /*wantLabel*/ false, /*wantBody*/true);

   if (unlabeledCtor)
      structDecl->addMember(unlabeledCtor);
   structDecl->addMember(init);
   structDecl->addMember(storedPatternBinding);
   structDecl->addMember(storedVar);
   structDecl->addMember(computedPatternBinding);
   structDecl->addMember(computedVar);

   addSynthesizedTypealias(structDecl, ctx.Id_RawValue, bridgedType);
   Impl.RawTypes[structDecl] = bridgedType;
}

/// Build a declaration for an Objective-C subscript getter.
static AccessorDecl *
buildSubscriptGetterDecl(ClangImporter::Implementation &Impl,
                         SubscriptDecl *subscript, const FuncDecl *getter,
                         Type elementTy, DeclContext *dc, ParamDecl *index) {
   auto &C = Impl.TypePHPContext;
   auto loc = getter->getLoc();

   auto *params = ParameterList::create(C, index);

   // Create the getter thunk.
   auto thunk = AccessorDecl::create(C,
      /*FuncLoc=*/loc,
      /*AccessorKeywordLoc=*/SourceLoc(),
                                     AccessorKind::Get,
                                     subscript,
      /*StaticLoc=*/SourceLoc(),
                                     subscript->getStaticSpelling(),
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr,
                                     params,
                                     TypeLoc::withoutLoc(elementTy), dc,
                                     getter->getClangNode());

   thunk->setAccess(getOverridableAccessLevel(dc));
   // @todo
//   if (auto objcAttr = getter->getAttrs().getAttribute<ObjCAttr>())
//      thunk->getAttrs().add(objcAttr->clone(C));
//   thunk->setIsObjC(getter->isObjC());
   thunk->setIsDynamic(getter->isDynamic());
   // FIXME: Should we record thunks?

   return thunk;
}

/// Build a declaration for an Objective-C subscript setter.
static AccessorDecl *
buildSubscriptSetterDecl(ClangImporter::Implementation &Impl,
                         SubscriptDecl *subscript, const FuncDecl *setter,
                         Type elementInterfaceTy,
                         DeclContext *dc, ParamDecl *index) {
   auto &C = Impl.TypePHPContext;
   auto loc = setter->getLoc();

   // Objective-C subscript setters are imported with a function type
   // such as:
   //
   //   (self) -> (value, index) -> ()
   //
   // Build a setter thunk with the latter signature that maps to the
   // former.
   auto valueIndex = setter->getParameters();

   auto paramVarDecl =
      new (C) ParamDecl(SourceLoc(), SourceLoc(),
                        Identifier(), loc, valueIndex->get(0)->getName(), dc);
   paramVarDecl->setSpecifier(ParamSpecifier::Default);
   paramVarDecl->setInterfaceType(elementInterfaceTy);

   auto valueIndicesPL = ParameterList::create(C, {paramVarDecl, index});

   // Create the setter thunk.
   auto thunk = AccessorDecl::create(C,
      /*FuncLoc=*/setter->getLoc(),
      /*AccessorKeywordLoc=*/SourceLoc(),
                                     AccessorKind::Set,
                                     subscript,
      /*StaticLoc=*/SourceLoc(),
                                     subscript->getStaticSpelling(),
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr,
                                     valueIndicesPL,
                                     TypeLoc::withoutLoc(TupleType::getEmpty(C)), dc,
                                     setter->getClangNode());

   thunk->setAccess(getOverridableAccessLevel(dc));
   // @todo
//   if (auto objcAttr = setter->getAttrs().getAttribute<ObjCAttr>())
//      thunk->getAttrs().add(objcAttr->clone(C));
//   thunk->setIsObjC(setter->isObjC());
   thunk->setIsDynamic(setter->isDynamic());

   return thunk;
}

/// Retrieve the element interface type and key param decl of a subscript
/// setter.
static std::pair<Type, ParamDecl *> decomposeSubscriptSetter(FuncDecl *setter) {
   auto *PL = setter->getParameters();
   if (PL->size() != 2)
      return {nullptr, nullptr};

   // Setter type is (self) -> (elem_type, key_type) -> ()
   Type elementType = setter->getInterfaceType()
      ->castTo<AnyFunctionType>()
      ->getResult()
      ->castTo<AnyFunctionType>()
      ->getParams().front().getParameterType();
   ParamDecl *keyDecl = PL->get(1);

   return {elementType, keyDecl};
}

/// Rectify the (possibly different) types determined by the
/// getter and setter for a subscript.
///
/// \param canUpdateType whether the type of subscript can be
/// changed from the getter type to something compatible with both
/// the getter and the setter.
///
/// \returns the type to be used for the subscript, or a null type
/// if the types cannot be rectified.
static ImportedType rectifySubscriptTypes(Type getterType, bool getterIsIUO,
                                          Type setterType, bool canUpdateType) {
   // If the caller couldn't provide a setter type, there is
   // nothing to rectify.
   if (!setterType)
      return {nullptr, false};

   // Trivial case: same type in both cases.
   if (getterType->isEqual(setterType))
      return {getterType, getterIsIUO};

   // The getter/setter types are different. If we cannot update
   // the type, we have to fail.
   if (!canUpdateType)
      return {nullptr, false};

   // Unwrap one level of optionality from each.
   if (Type getterObjectType = getterType->getOptionalObjectType())
      getterType = getterObjectType;
   if (Type setterObjectType = setterType->getOptionalObjectType())
      setterType = setterObjectType;

   // If they are still different, fail.
   // FIXME: We could produce the greatest common supertype of the
   // two types.
   if (!getterType->isEqual(setterType))
      return {nullptr, false};

   // Create an optional of the object type that can be implicitly
   // unwrapped which subsumes both behaviors.
   return {OptionalType::get(setterType), true};
}

/// Add an AvailableAttr to the declaration for the given
/// version range.
static void applyAvailableAttribute(Decl *decl, AvailabilityContext &info,
                                    AstContext &C) {
   // If the range is "all", this is the same as not having an available
   // attribute.
   if (info.isAlwaysAvailable())
      return;

   llvm::VersionTuple noVersion;
   auto AvAttr = new (C) AvailableAttr(SourceLoc(), SourceRange(),
                                       targetPlatform(C.LangOpts),
      /*Message=*/StringRef(),
      /*Rename=*/StringRef(),
                                       info.getOSVersion().getLowerEndpoint(),
      /*IntroducedRange*/SourceRange(),
      /*Deprecated=*/noVersion,
      /*DeprecatedRange*/SourceRange(),
      /*Obsoleted=*/noVersion,
      /*ObsoletedRange*/SourceRange(),
                                       PlatformAgnosticAvailabilityKind::None,
      /*Implicit=*/false);

   decl->getAttrs().add(AvAttr);
}

/// Synthesize availability attributes for protocol requirements
/// based on availability of the types mentioned in the requirements.
static void inferInterfaceMemberAvailability(ClangImporter::Implementation &impl,
                                             DeclContext *dc, Decl *member) {
   // Don't synthesize attributes if there is already an
   // availability annotation.
   if (member->getAttrs().hasAttribute<AvailableAttr>())
      return;

   auto *valueDecl = dyn_cast<ValueDecl>(member);
   if (!valueDecl)
      return;

   AvailabilityContext requiredRange =
      AvailabilityInference::inferForType(valueDecl->getInterfaceType());

   AstContext &C = impl.TypePHPContext;

   const Decl *innermostDecl = dc->getInnermostDeclarationDeclContext();
   AvailabilityContext containingDeclRange =
      AvailabilityInference::availableRange(innermostDecl, C);

   requiredRange.intersectWith(containingDeclRange);

   applyAvailableAttribute(valueDecl, requiredRange, C);
}

/// Synthesizer callback for the error domain property getter.
static std::pair<BraceStmt *, bool>
synthesizeErrorDomainGetterBody(AbstractFunctionDecl *afd, void *context) {
   auto getterDecl = cast<AccessorDecl>(afd);
   AstContext &ctx = getterDecl->getAstContext();

   auto contextData =
      llvm::PointerIntPair<ValueDecl *, 1, bool>::getFromOpaqueValue(context);
   auto polarValueDecl = contextData.getPointer();
   bool isImplicit = contextData.getInt();
   DeclRefExpr *domainDeclRef = new (ctx)
      DeclRefExpr(ConcreteDeclRef(polarValueDecl), {}, isImplicit);
   domainDeclRef->setType(
      getterDecl->mapTypeIntoContext(polarValueDecl->getInterfaceType()));

   auto ret = new (ctx) ReturnStmt(SourceLoc(), domainDeclRef);
   return { BraceStmt::create(ctx, SourceLoc(), {ret}, SourceLoc(), isImplicit),
      /*isTypeChecked=*/true };
}

/// Add a domain error member, as required by conformance to
/// _BridgedStoredNSError.
/// \returns true on success, false on failure
static bool addErrorDomain(NominalTypeDecl *polarphpDecl,
                           clang::NamedDecl *errorDomainDecl,
                           ClangImporter::Implementation &importer) {
   auto &C = importer.TypePHPContext;
   auto polarValueDecl = dyn_cast_or_null<ValueDecl>(
      importer.importDecl(errorDomainDecl, importer.CurrentVersion));
   auto stringTy = C.getStringDecl()->getDeclaredType();
   assert(stringTy && "no string type available");
   if (!polarValueDecl || !polarValueDecl->getInterfaceType()->isEqual(stringTy)) {
      // Couldn't actually import it as an error enum, fall back to enum
      return false;
   }

   bool isStatic = true;
   bool isImplicit = true;

   // Make the property decl
   auto errorDomainPropertyDecl = new (C) VarDecl(
      /*IsStatic*/isStatic, VarDecl::Introducer::Var, /*IsCaptureList*/false,
                  SourceLoc(), C.Id_errorDomain, polarphpDecl);
   errorDomainPropertyDecl->setInterfaceType(stringTy);
   errorDomainPropertyDecl->setAccess(AccessLevel::Public);

   auto *params = ParameterList::createEmpty(C);

   auto getterDecl = AccessorDecl::create(C,
      /*FuncLoc=*/SourceLoc(),
      /*AccessorKeywordLoc=*/SourceLoc(),
                                          AccessorKind::Get,
                                          errorDomainPropertyDecl,
      /*StaticLoc=*/SourceLoc(),
                                          StaticSpellingKind::None,
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr,
                                          params,
                                          TypeLoc::withoutLoc(stringTy), polarphpDecl);
   // @todo
//   getterDecl->setIsObjC(false);
   getterDecl->setIsDynamic(false);
   getterDecl->setIsTransparent(false);

   polarphpDecl->addMember(errorDomainPropertyDecl);
   makeComputed(errorDomainPropertyDecl, getterDecl, nullptr);

   getterDecl->setImplicit();
   getterDecl->setAccess(AccessLevel::Public);

   llvm::PointerIntPair<ValueDecl *, 1, bool> contextData(polarValueDecl,
                                                          isImplicit);
   getterDecl->setBodySynthesizer(synthesizeErrorDomainGetterBody,
                                  contextData.getOpaqueValue());

   return true;
}

/// As addErrorDomain above, but performs a lookup
static bool addErrorDomain(NominalTypeDecl *polarphpDecl,
                           StringRef errorDomainName,
                           ClangImporter::Implementation &importer) {
   auto &clangSema = importer.getClangSema();
   clang::IdentifierInfo *errorDomainDeclName =
      &clangSema.getASTContext().Idents.get(errorDomainName);
   clang::LookupResult lookupResult(
      clangSema, clang::DeclarationName(errorDomainDeclName),
      clang::SourceLocation(), clang::Sema::LookupNameKind::LookupOrdinaryName);

   if (!clangSema.LookupName(lookupResult, clangSema.TUScope)) {
      // Couldn't actually import it as an error enum, fall back to enum
      return false;
   }

   auto clangNamedDecl = lookupResult.getAsSingle<clang::NamedDecl>();
   if (!clangNamedDecl) {
      // Couldn't actually import it as an error enum, fall back to enum
      return false;
   }

   return addErrorDomain(polarphpDecl, clangNamedDecl, importer);
}

/// Retrieve the property type as determined by the given accessor.
static clang::QualType
getAccessorPropertyType(const clang::FunctionDecl *accessor, bool isSetter,
                        Optional<unsigned> selfIndex) {
   // Simple case: the property type of the getter is in the return
   // type.
   if (!isSetter) return accessor->getReturnType();

   // For the setter, first check that we have the right number of
   // parameters.
   unsigned numExpectedParams = selfIndex ? 2 : 1;
   if (accessor->getNumParams() != numExpectedParams)
      return clang::QualType();

   // Dig out the parameter for the value.
   unsigned valueIdx = selfIndex ? (1 - *selfIndex) : 0;
   auto param = accessor->getParamDecl(valueIdx);
   return param->getType();
}

/// Whether we should suppress importing the Objective-C generic type params
/// of this class as Swift generic type params.
static bool
shouldSuppressGenericParamsImport(const LangOptions &langOpts,
                                  const clang::ObjCInterfaceDecl *decl) {
   // @todo
//   if (decl->hasAttr<clang::SwiftImportAsNonGenericAttr>())
//      return true;

   // FIXME: This check is only necessary to keep things working even without
   // the SwiftImportAsNonGeneric API note. Once we can guarantee that that
   // attribute is present in all contexts, we can remove this check.
   auto isFromFoundationModule = [](const clang::Decl *decl) -> bool {
      clang::Module *module = getClangSubmoduleForDecl(decl).getValue();
      if (!module)
         return false;
      return module->getTopLevelModuleName() == "Foundation";
   };

   if (isFromFoundationModule(decl)) {
      // In Swift 3 we used a hardcoded list of declarations, and made all of
      // their subclasses drop their generic parameters when imported.
      while (decl) {
         StringRef name = decl->getName();
         if (name == "NSArray" || name == "NSDictionary" || name == "NSSet" ||
             name == "NSOrderedSet" || name == "NSEnumerator" ||
             name == "NSMeasurement") {
            return true;
         }
         decl = decl->getSuperClass();
      }
   }

   return false;
}

/// Determine if the given Objective-C instance method should also
/// be imported as a class method.
///
/// Objective-C root class instance methods are also reflected as
/// class methods.
static bool shouldAlsoImportAsClassMethod(FuncDecl *method) {
   // Only instance methods.
   if (!method->isInstanceMember())
      return false;

   // Must be a method within a class or extension thereof.
   auto classDecl = method->getDeclContext()->getSelfClassDecl();
   if (!classDecl)
      return false;

   // The class must not have a superclass.
   if (classDecl->hasSuperclass())
      return false;

   // There must not already be a class method with the same
   // selector.
   auto objcClass =
      cast_or_null<clang::ObjCInterfaceDecl>(classDecl->getClangDecl());
   if (!objcClass)
      return false;

   auto objcMethod = cast_or_null<clang::ObjCMethodDecl>(method->getClangDecl());
   if (!objcMethod)
      return false;
   return !objcClass->getClassMethod(objcMethod->getSelector(),
      /*AllowHidden=*/true);
}

// @todo
//static bool
//classImplementsInterface(const clang::ObjCInterfaceDecl *constInterface,
//                         const clang::ObjCInterfaceDecl *constProto,
//                         bool checkCategories) {
//   auto interface = const_cast<clang::ObjCInterfaceDecl *>(constInterface);
//   auto proto = const_cast<clang::ObjCInterfaceDecl *>(constProto);
//   return interface->ClassImplementsProtocol(proto, checkCategories);
//}

static void
applyPropertyOwnership(VarDecl *prop,
                       clang::ObjCPropertyDecl::PropertyAttributeKind attrs) {
   Type ty = prop->getInterfaceType();
   if (auto innerTy = ty->getOptionalObjectType())
      ty = innerTy;
   if (!ty->is<GenericTypeParamType>() && !ty->isAnyClassReferenceType())
      return;

   AstContext &ctx = prop->getAstContext();
   if (attrs & clang::ObjCPropertyDecl::OBJC_PR_copy) {
      prop->getAttrs().add(new (ctx) NSCopyingAttr(false));
      return;
   }
   if (attrs & clang::ObjCPropertyDecl::OBJC_PR_weak) {
      prop->getAttrs().add(new (ctx)
                              ReferenceOwnershipAttr(ReferenceOwnership::Weak));
      prop->setInterfaceType(WeakStorageType::get(
         prop->getInterfaceType(), ctx));
      return;
   }
   if ((attrs & clang::ObjCPropertyDecl::OBJC_PR_assign) ||
       (attrs & clang::ObjCPropertyDecl::OBJC_PR_unsafe_unretained)) {
      prop->getAttrs().add(
         new (ctx) ReferenceOwnershipAttr(ReferenceOwnership::Unmanaged));
      prop->setInterfaceType(UnmanagedStorageType::get(
         prop->getInterfaceType(), ctx));
      return;
   }
}

/// Does this name refer to a method that might shadow Swift.print?
///
/// As a heuristic, methods that have a base name of 'print' but more than
/// one argument are left alone. These can still shadow Swift.print but are
/// less likely to be confused for it, at least.
static bool isPrintLikeMethod(DeclName name, const DeclContext *dc) {
   if (!name || name.isSpecial() || name.isSimpleName())
      return false;
   if (name.getBaseIdentifier().str() != "print")
      return false;
   if (!dc->isTypeContext())
      return false;
   if (name.getArgumentNames().size() > 1)
      return false;
   return true;
}

using MirroredMethodEntry =
std::pair<const clang::ObjCMethodDecl*, InterfaceDecl*>;

namespace {
/// Customized llvm::DenseMapInfo for storing borrowed APSInts.
struct APSIntRefDenseMapInfo {
   static inline const llvm::APSInt *getEmptyKey() {
      return llvm::DenseMapInfo<const llvm::APSInt *>::getEmptyKey();
   }
   static inline const llvm::APSInt *getTombstoneKey() {
      return llvm::DenseMapInfo<const llvm::APSInt *>::getTombstoneKey();
   }
   static unsigned getHashValue(const llvm::APSInt *ptrVal) {
      assert(ptrVal != getEmptyKey() && ptrVal != getTombstoneKey());
      return llvm::hash_value(*ptrVal);
   }
   static bool isEqual(const llvm::APSInt *lhs, const llvm::APSInt *rhs) {
      if (lhs == rhs) return true;
      if (lhs == getEmptyKey() || rhs == getEmptyKey()) return false;
      if (lhs == getTombstoneKey() || rhs == getTombstoneKey()) return false;
      return *lhs == *rhs;
   }
};

/// Convert Clang declarations into the corresponding Swift
/// declarations.
class TypePHPDeclConverter
   : public clang::ConstDeclVisitor<TypePHPDeclConverter, Decl *>
{
   ClangImporter::Implementation &Impl;
   bool forwardDeclaration = false;
   ImportNameVersion version;

   /// The version that we're being asked to import for. May not be the version
   /// the user requested, as we may be forming an alternate for diagnostic
   /// purposes.
   ImportNameVersion getVersion() const { return version; }

   /// The actual language version the user requested we compile for.
   ImportNameVersion getActivePolarphpVersion() const {
      return Impl.CurrentVersion;
   }

   /// Whether the names we're importing are from the language version the user
   /// requested, or if these are decls from another version
   bool isActivePolarphpVersion() const {
      return getVersion() == getActivePolarphpVersion();
   }

   /// Import the name of the given entity.
   ///
   /// This version of importFullName introduces any context-specific
   /// name importing options (e.g., if we're importing the Swift 2 version).
   ///
   /// Note: Use this rather than calling Impl.importFullName directly!
   ImportedName importFullName(const clang::NamedDecl *D,
                               Optional<ImportedName> &correctPolarphpName) {
      ImportNameVersion canonicalVersion = getActivePolarphpVersion();
      if (isa<clang::TypeDecl>(D) || isa<clang::ObjCContainerDecl>(D)) {
         canonicalVersion = ImportNameVersion::forTypes();
      }
      correctPolarphpName = None;

      // First, import based on the Swift name of the canonical declaration:
      // the latest version for types and the current version for non-type
      // values. If that fails, we won't do anything.
      auto canonicalName = Impl.importFullName(D, canonicalVersion);
      if (!canonicalName)
         return ImportedName();

      if (getVersion() == canonicalVersion) {
         // Make sure we don't try to import the same type twice as canonical.
         if (canonicalVersion != getActivePolarphpVersion()) {
            auto activeName = Impl.importFullName(D, getActivePolarphpVersion());
            if (activeName &&
                activeName.getDeclName() == canonicalName.getDeclName() &&
                activeName.getEffectiveContext().equalsWithoutResolving(
                   canonicalName.getEffectiveContext())) {
               return ImportedName();
            }
         }

         return canonicalName;
      }

      // Special handling when we import using the older Swift name.
      //
      // Import using the alternate Swift name. If that fails, or if it's
      // identical to the active Swift name, we won't introduce an alternate
      // Swift name stub declaration.
      auto alternateName = Impl.importFullName(D, getVersion());
      if (!alternateName)
         return ImportedName();

      if (alternateName.getDeclName() == canonicalName.getDeclName() &&
          alternateName.getEffectiveContext().equalsWithoutResolving(
             canonicalName.getEffectiveContext())) {
         if (getVersion() == getActivePolarphpVersion()) {
            assert(canonicalVersion != getActivePolarphpVersion());
            return alternateName;
         }
         return ImportedName();
      }

      // Always use the active version as the preferred name, even if the
      // canonical name is a different version.
      correctPolarphpName = Impl.importFullName(D, getActivePolarphpVersion());
      assert(correctPolarphpName);

      return alternateName;
   }

   /// Create a declaration name for anonymous enums, unions and
   /// structs.
   ///
   /// Since polarphp does not natively support these features, we fake them by
   /// importing them as declarations with generated names. The generated name
   /// is derived from the name of the field in the outer type. Since the
   /// anonymous type is imported as a nested type of the outer type, this
   /// generated name will most likely be unique.
   ImportedName getClangDeclName(const clang::TagDecl *decl,
                                 Optional<ImportedName> &correctPolarphpName) {
      // If we have a name for this declaration, use it.
      if (auto name = importFullName(decl, correctPolarphpName))
         return name;

      // If that didn't succeed, check whether this is an anonymous tag declaration
      // with a corresponding typedef-name declaration.
      if (decl->getDeclName().isEmpty()) {
         if (auto *typedefForAnon = decl->getTypedefNameForAnonDecl())
            return importFullName(typedefForAnon, correctPolarphpName);
      }

      if (!decl->isRecord())
         return ImportedName();

      // If the type has no name and no structure name, but is not anonymous,
      // generate a name for it. Specifically this is for cases like:
      //   struct a {
      //     struct {} z;
      //   }
      // Where the member z is an unnamed struct, but does have a member-name
      // and is accessible as a member of struct a.
      correctPolarphpName = None;
      if (auto recordDecl = dyn_cast<clang::RecordDecl>(
         decl->getLexicalDeclContext())) {
         for (auto field : recordDecl->fields()) {
            if (field->getType()->getAsTagDecl() == decl) {
               // Create a name for the declaration from the field name.
               std::string Id;
               llvm::raw_string_ostream IdStream(Id);

               const char *kind;
               if (decl->isStruct())
                  kind = "struct";
               else if (decl->isUnion())
                  kind = "union";
               else
                  llvm_unreachable("unknown decl kind");

               IdStream << "__Unnamed_" << kind << "_";
               if (field->isAnonymousStructOrUnion()) {
                  IdStream << "__Anonymous_field" << field->getFieldIndex();
               } else {
                  IdStream << field->getName();
               }
               ImportedName Result;
               Result.setDeclName(Impl.TypePHPContext.getIdentifier(IdStream.str()));
               Result.setEffectiveContext(decl->getDeclContext());
               return Result;
            }
         }
      }

      return ImportedName();
   }

   bool isFactoryInit(ImportedName &name) {
      return name &&
             name.getDeclName().getBaseName() == DeclBaseName::createConstructor() &&
             (name.getInitKind() == CtorInitializerKind::Factory ||
              name.getInitKind() == CtorInitializerKind::ConvenienceFactory);
   }

public:
   explicit TypePHPDeclConverter(ClangImporter::Implementation &impl,
                                 ImportNameVersion vers)
      : Impl(impl), version(vers) { }

   bool hadForwardDeclaration() const {
      return forwardDeclaration;
   }

   Decl *VisitDecl(const clang::Decl *decl) {
      return nullptr;
   }

   Decl *VisitTranslationUnitDecl(const clang::TranslationUnitDecl *decl) {
      // Note: translation units are handled specially by importDeclContext.
      return nullptr;
   }

   Decl *VisitNamespaceDecl(const clang::NamespaceDecl *decl) {
      // If we have a name for this declaration, use it.
      Optional<ImportedName> correctPolarphpName;
      auto importedName = importFullName(decl, correctPolarphpName);
      if (!importedName) return nullptr;

      auto dc =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!dc)
         return nullptr;

      SourceLoc loc = Impl.importSourceLoc(decl->getBeginLoc());

      // FIXME: If Swift gets namespaces, import as a namespace.
      auto enumDecl = Impl.createDeclWithClangNode<EnumDecl>(
         decl, AccessLevel::Public, loc,
         importedName.getDeclName().getBaseIdentifier(),
         Impl.importSourceLoc(decl->getLocation()), None, nullptr, dc);
      enumDecl->setMemberLoader(&Impl, 0);
      return enumDecl;
   }

   Decl *VisitUsingDirectiveDecl(const clang::UsingDirectiveDecl *decl) {
      // Never imported.
      return nullptr;
   }

   Decl *VisitNamespaceAliasDecl(const clang::NamespaceAliasDecl *decl) {
      // FIXME: Implement once Swift has namespaces.
      return nullptr;
   }

   Decl *VisitLabelDecl(const clang::LabelDecl *decl) {
      // Labels are function-local, and therefore never imported.
      return nullptr;
   }

   ClassDecl *importCFClassType(const clang::TypedefNameDecl *decl,
                                Identifier className, CFPointeeInfo info,
                                EffectiveClangContext effectiveContext);

   /// Mark the given declaration as an older Swift version variant of the
   /// current name.
   void markAsVariant(Decl *decl, ImportedName correctPolarphpName) {
      // Types always import using the latest version. Make sure all names up
      // to that version are considered available.
      if (isa<TypeDecl>(decl)) {
         cast<TypeAliasDecl>(decl)->markAsCompatibilityAlias();

         if (getVersion() >= getActivePolarphpVersion())
            return;
      }

      // TODO: some versions should be deprecated instead of unavailable

      AstContext &ctx = decl->getAstContext();
      llvm::SmallString<64> renamed;
      {
         // Render a swift_name string.
         llvm::raw_svector_ostream os(renamed);

         // If we're importing a global as a member, we need to provide the
         // effective context.
         //
//         Impl.printTypePHPName(
//            correctPolarphpName, getActivePolarphpVersion(),
//            /*fullyQualified=*/correctPolarphpName.importAsMember(), os);
      }

      DeclAttribute *attr;
      if (isActivePolarphpVersion() || getVersion() == ImportNameVersion::raw()) {
         // "Raw" is the Objective-C name, which was never available in Swift.
         // Variants within the active version are usually declarations that
         // have been superseded, like the accessors of a property.
         attr = AvailableAttr::createPlatformAgnostic(
            ctx, /*Message*/StringRef(), ctx.AllocateCopy(renamed.str()),
            PlatformAgnosticAvailabilityKind::UnavailableInSwift);
      } else {
         unsigned majorVersion = getVersion().majorVersionNumber();
         unsigned minorVersion = getVersion().minorVersionNumber();
         if (getVersion() < getActivePolarphpVersion()) {
            // A Swift 2 name, for example, was obsoleted in Swift 3.
            // However, a Swift 4 name is obsoleted in Swift 4.2.
            // FIXME: it would be better to have a unified place
            // to represent Swift versions for API versioning.
            llvm::VersionTuple obsoletedVersion =
               (majorVersion == 4 && minorVersion < 2)
               ? llvm::VersionTuple(4, 2)
               : llvm::VersionTuple(majorVersion + 1);
            attr = AvailableAttr::createPlatformAgnostic(
               ctx, /*Message*/StringRef(), ctx.AllocateCopy(renamed.str()),
               PlatformAgnosticAvailabilityKind::SwiftVersionSpecific,
               obsoletedVersion);
         } else {
            // Future names are introduced in their future version.
            assert(getVersion() > getActivePolarphpVersion());
            llvm::VersionTuple introducedVersion =
               (majorVersion == 4 && minorVersion == 2)
               ? llvm::VersionTuple(4, 2)
               : llvm::VersionTuple(majorVersion);
            attr = new (ctx) AvailableAttr(
               SourceLoc(), SourceRange(), PlatformKind::none,
               /*Message*/StringRef(), ctx.AllocateCopy(renamed.str()),
               /*Introduced*/introducedVersion, SourceRange(),
               /*Deprecated*/llvm::VersionTuple(), SourceRange(),
               /*Obsoleted*/llvm::VersionTuple(), SourceRange(),
               PlatformAgnosticAvailabilityKind::SwiftVersionSpecific,
               /*Implicit*/false);
         }
      }

      decl->getAttrs().add(attr);
      decl->setImplicit();
   }

   /// Create a typealias for the name of a Clang type declaration in an
   /// alternate version of polarphp.
   Decl *importCompatibilityTypeAlias(const clang::NamedDecl *decl,
                                      ImportedName compatibilityName,
                                      ImportedName correctPolarphpName);

   /// Create a typephp_newtype struct corresponding to a typedef. Returns
   /// nullptr if unable.
   Decl *importTypePHPNewtype(const clang::TypedefNameDecl *decl,
                              clang::SwiftNewtypeAttr *newtypeAttr,
                              DeclContext *dc, Identifier name);

   Decl *VisitTypedefNameDecl(const clang::TypedefNameDecl *Decl) {
      Optional<ImportedName> correctPolarphpName;
      auto importedName = importFullName(Decl, correctPolarphpName);
      auto Name = importedName.getDeclName().getBaseIdentifier();
      if (Name.empty())
         return nullptr;

      // If we've been asked to produce a compatibility stub, handle it via a
      // typealias.
      if (correctPolarphpName)
         return importCompatibilityTypeAlias(Decl, importedName,
                                             *correctPolarphpName);
      // @todo

      Type TypePHPType;
//      if (Decl->getDeclContext()->getRedeclContext()->isTranslationUnit()) {
//         bool IsError;
//         StringRef StdlibTypeName;
//         MappedTypeNameKind NameMapping;
//         std::tie(TypePHPType, StdlibTypeName) =
//            getSwiftStdlibType(Decl, Name, Impl, &IsError, NameMapping);
//
//         if (IsError)
//            return nullptr;
//
//         // Import 'typedef struct __Blah *BlahRef;' and
//         // 'typedef const void *FooRef;' as CF types if they have the
//         // right attributes or match our list of known types.
//         if (!TypePHPType) {
//            auto DC = Impl.importDeclContextOf(
//               Decl, importedName.getEffectiveContext());
//            if (!DC)
//               return nullptr;
//
//            if (auto pointee = CFPointeeInfo::classifyTypedef(Decl)) {
//               // If the pointee is a record, consider creating a class type.
//               if (pointee.isRecord()) {
//                  auto swiftClass = importCFClassType(
//                     Decl, Name, pointee, importedName.getEffectiveContext());
//                  if (!swiftClass) return nullptr;
//
//                  Impl.SpecialTypedefNames[Decl->getCanonicalDecl()] =
//                     MappedTypeNameKind::DefineAndUse;
//                  return swiftClass;
//               }
//
//               // If the pointee is another CF typedef, create an extra typealias
//               // for the name without "Ref", but not a separate type.
//               if (pointee.isTypedef()) {
//                  auto underlying = cast_or_null<TypeDecl>(Impl.importDecl(
//                     pointee.getTypedef(), getActivePolarphpVersion()));
//                  if (!underlying)
//                     return nullptr;
//
//                  // Check for a newtype
//                  if (auto newtypeAttr =
//                     getSwiftNewtypeAttr(Decl, getVersion()))
//                     if (auto newtype =
//                        importTypePHPNewtype(Decl, newtypeAttr, DC, Name))
//                        return newtype;
//
//                  // Create a typealias for this CF typedef.
//                  TypeAliasDecl *typealias = nullptr;
//                  typealias = Impl.createDeclWithClangNode<TypeAliasDecl>(
//                     Decl, AccessLevel::Public,
//                     Impl.importSourceLoc(Decl->getBeginLoc()),
//                     SourceLoc(), Name,
//                     Impl.importSourceLoc(Decl->getLocation()),
//                     /*genericparams*/nullptr, DC);
//                  typealias->setUnderlyingType(
//                     underlying->getDeclaredInterfaceType());
//
//                  Impl.SpecialTypedefNames[Decl->getCanonicalDecl()] =
//                     MappedTypeNameKind::DefineAndUse;
//                  return typealias;
//               }
//
//               // If the pointee is 'void', 'CFTypeRef', bring it
//               // in specifically as AnyObject.
//               if (pointee.isVoid()) {
//                  // Create a typealias for this CF typedef.
//                  TypeAliasDecl *typealias = nullptr;
//                  typealias = Impl.createDeclWithClangNode<TypeAliasDecl>(
//                     Decl, AccessLevel::Public,
//                     Impl.importSourceLoc(Decl->getBeginLoc()),
//                     SourceLoc(), Name,
//                     Impl.importSourceLoc(Decl->getLocation()),
//                     /*genericparams*/nullptr, DC);
//                  typealias->setUnderlyingType(
//                     Impl.TypePHPContext.getAnyObjectType());
//
//                  Impl.SpecialTypedefNames[Decl->getCanonicalDecl()] =
//                     MappedTypeNameKind::DefineAndUse;
//                  return typealias;
//               }
//            }
//         }
//
//         if (TypePHPType) {
//            // Note that this typedef-name is special.
//            Impl.SpecialTypedefNames[Decl->getCanonicalDecl()] = NameMapping;
//
//            if (NameMapping == MappedTypeNameKind::DoNothing) {
//               // Record the remapping using the name of the Clang declaration.
//               // This will be useful for type checker diagnostics when
//               // a user tries to use the Objective-C/C type instead of the
//               // Swift type.
//               Impl.TypePHPContext.RemappedTypes[Decl->getNameAsString()]
//                  = TypePHPType;
//
//               // Don't create an extra typealias in the imported module because
//               // doing so will cause confusion (or even lookup ambiguity) between
//               // the name in the imported module and the same name in the
//               // standard library.
//               if (auto *NAT =
//                  dyn_cast<TypeAliasType>(TypePHPType.getPointer()))
//                  return NAT->getDecl();
//
//               auto *NTD = TypePHPType->getAnyNominal();
//               assert(NTD);
//               return NTD;
//            }
//         }
//      }

      auto DC =
         Impl.importDeclContextOf(Decl, importedName.getEffectiveContext());
      if (!DC)
         return nullptr;

//      // Check for swift_newtype
//      if (!TypePHPType)
//         if (auto newtypeAttr = getSwiftNewtypeAttr(Decl, getVersion()))
//            if (auto newtype = importTypePHPNewtype(Decl, newtypeAttr, DC, Name))
//               return newtype;
//
//      if (!TypePHPType) {
//         // Note that the code below checks to see if the typedef allows
//         // bridging, i.e. if the imported typealias should name a bridged type
//         // or the original C type.
//         clang::QualType ClangType = Decl->getUnderlyingType();
//         TypePHPType = Impl.importTypeIgnoreIUO(
//            ClangType, ImportTypeKind::Typedef, isInSystemModule(DC),
//            getTypedefBridgeability(Decl), OTK_Optional);
//      }
//
//      if (!TypePHPType)
//         return nullptr;

      auto Loc = Impl.importSourceLoc(Decl->getLocation());
      auto Result = Impl.createDeclWithClangNode<TypeAliasDecl>(Decl,
                                                                AccessLevel::Public,
                                                                Impl.importSourceLoc(Decl->getBeginLoc()),
                                                                SourceLoc(), Name,
                                                                Loc,
         /*genericparams*/nullptr, DC);
      Result->setUnderlyingType(TypePHPType);

      // Make Objective-C's 'id' unavailable.
      if (Impl.TypePHPContext.LangOpts.EnableObjCInterop && isObjCId(Decl)) {
         auto attr = AvailableAttr::createPlatformAgnostic(
            Impl.TypePHPContext,
            "'id' is not available in Swift; use 'Any'", "",
            PlatformAgnosticAvailabilityKind::UnavailableInSwift);
         Result->getAttrs().add(attr);
      }

      return Result;
   }

   Decl *
   VisitUnresolvedUsingTypenameDecl(const
                                    clang::UnresolvedUsingTypenameDecl *decl) {
      // Note: only occurs in templates.
      return nullptr;
   }

   /// Import an NS_ENUM constant as a case of a Swift enum.
   Decl *importEnumCase(const clang::EnumConstantDecl *decl,
                        const clang::EnumDecl *clangEnum,
                        EnumDecl *theEnum,
                        Decl *swift3Decl = nullptr);

   /// Import an NS_OPTIONS constant as a static property of a Swift struct.
   ///
   /// This is also used to import enum case aliases.
   Decl *importOptionConstant(const clang::EnumConstantDecl *decl,
                              const clang::EnumDecl *clangEnum,
                              NominalTypeDecl *theStruct);

   /// Import \p alias as an alias for the imported constant \p original.
   ///
   /// This builds the getter in a way that's compatible with switch
   /// statements. Changing the body here may require changing
   /// TypeCheckPattern.cpp as well.
   Decl *importEnumCaseAlias(Identifier name,
                             const clang::EnumConstantDecl *alias,
                             ValueDecl *original,
                             const clang::EnumDecl *clangEnum,
                             NominalTypeDecl *importedEnum,
                             DeclContext *importIntoDC = nullptr);

   NominalTypeDecl *importAsOptionSetType(DeclContext *dc,
                                          Identifier name,
                                          const clang::EnumDecl *decl);

   Decl *VisitEnumDecl(const clang::EnumDecl *decl) {
      decl = decl->getDefinition();
      if (!decl) {
         forwardDeclaration = true;
         return nullptr;
      }

      // Don't import nominal types that are over-aligned.
      if (Impl.isOverAligned(decl))
         return nullptr;

      Optional<ImportedName> correctPolarphpName;
      auto importedName = getClangDeclName(decl, correctPolarphpName);
      if (!importedName)
         return nullptr;

      // If we've been asked to produce a compatibility stub, handle it via a
      // typealias.
      if (correctPolarphpName)
         return importCompatibilityTypeAlias(decl, importedName,
                                             *correctPolarphpName);

      auto dc =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!dc)
         return nullptr;

      auto name = importedName.getDeclName().getBaseIdentifier();

      // Create the enum declaration and record it.
      StructDecl *errorWrapper = nullptr;
      NominalTypeDecl *result;
      auto enumInfo = Impl.getEnumInfo(decl);
      auto enumKind = enumInfo.getKind();
      switch (enumKind) {
         case EnumKind::Constants: {
            // There is no declaration. Rather, the type is mapped to the
            // underlying type.
            return nullptr;
         }

         case EnumKind::Unknown: {
            // Compute the underlying type of the enumeration.
            auto underlyingType = Impl.importTypeIgnoreIUO(
               decl->getIntegerType(), ImportTypeKind::Enum, isInSystemModule(dc),
               Bridgeability::None);
            if (!underlyingType)
               return nullptr;

            auto Loc = Impl.importSourceLoc(decl->getLocation());
            auto structDecl = Impl.createDeclWithClangNode<StructDecl>(decl,
                                                                       AccessLevel::Public, Loc, name, Loc, None, nullptr, dc);

            auto options = getDefaultMakeStructRawValuedOptions();
            options |= MakeStructRawValuedFlags::MakeUnlabeledValueInit;
            options -= MakeStructRawValuedFlags::IsLet;
            options -= MakeStructRawValuedFlags::IsImplicit;

            makeStructRawValued(Impl, structDecl, underlyingType,
                                {KnownInterfaceKind::RawRepresentable,
                                 KnownInterfaceKind::Equatable},
                                options, /*setterAccess=*/AccessLevel::Public);

            result = structDecl;
            break;
         }

         case EnumKind::NonFrozenEnum:
         case EnumKind::FrozenEnum: {
            auto &C = Impl.TypePHPContext;
            EnumDecl *nativeDecl;
            // @todo
//            bool declaredNative = hasNativeSwiftDecl(decl, name, dc, nativeDecl);
//            if (declaredNative && nativeDecl)
//               return nativeDecl;

            // Compute the underlying type.
            auto underlyingType = Impl.importTypeIgnoreIUO(
               decl->getIntegerType(), ImportTypeKind::Enum, isInSystemModule(dc),
               Bridgeability::None);
            if (!underlyingType)
               return nullptr;

            /// Basic information about the enum type we're building.
            Identifier enumName = name;
            DeclContext *enumDC = dc;
            SourceLoc loc = Impl.importSourceLoc(decl->getBeginLoc());

            // If this is an error enum, form the error wrapper type,
            // which is a struct containing an NSError instance.
            InterfaceDecl *bridgedNSError = nullptr;
            ClassDecl *nsErrorDecl = nullptr;
            InterfaceDecl *errorCodeProto = nullptr;
            // @todo
//            if (enumInfo.isErrorEnum() &&
//                (bridgedNSError =
//                    C.getInterface(KnownInterfaceKind::BridgedStoredNSError)) &&
//                (nsErrorDecl = C.getNSErrorDecl()) &&
//                (errorCodeProto =
//                    C.getInterface(KnownInterfaceKind::ErrorCodeInterface))) {
//               // Create the wrapper struct.
//               errorWrapper = new (C) StructDecl(loc, name, loc, None, nullptr, dc);
//               errorWrapper->setAccess(AccessLevel::Public);
//               errorWrapper->getAttrs().add(
//                  new (Impl.TypePHPContext) FrozenAttr(/*IsImplicit*/true));
//
//               StringRef nameForMangling;
//               ClangImporterSynthesizedTypeAttr::Kind relatedEntityKind;
//               if (decl->getDeclName().isEmpty()) {
//                  nameForMangling = decl->getTypedefNameForAnonDecl()->getName();
//                  relatedEntityKind =
//                     ClangImporterSynthesizedTypeAttr::Kind::NSErrorWrapperAnon;
//               } else {
//                  nameForMangling = decl->getName();
//                  relatedEntityKind =
//                     ClangImporterSynthesizedTypeAttr::Kind::NSErrorWrapper;
//               }
//               errorWrapper->getAttrs().add(new (C) ClangImporterSynthesizedTypeAttr(
//                  nameForMangling, relatedEntityKind));
//
//               // Add inheritance clause.
//               addSynthesizedInterfaceAttrs(Impl, errorWrapper,
//                                            {KnownInterfaceKind::BridgedStoredNSError});
//
//               // Create the _nsError member.
//               //   public let _nsError: NSError
//               auto nsErrorType = nsErrorDecl->getDeclaredInterfaceType();
//               auto nsErrorProp = new (C) VarDecl(/*IsStatic*/false,
//                                                              VarDecl::Introducer::Let,
//                  /*IsCaptureList*/false,
//                                                              loc, C.Id_nsError,
//                                                              errorWrapper);
//               nsErrorProp->setImplicit();
//               nsErrorProp->setAccess(AccessLevel::Public);
//               nsErrorProp->setInterfaceType(nsErrorType);
//
//               // Create a pattern binding to describe the variable.
//               Pattern *nsErrorPattern = createTypedNamedPattern(nsErrorProp);
//
//               auto *nsErrorBinding = PatternBindingDecl::createImplicit(
//                  C, StaticSpellingKind::None, nsErrorPattern, /*InitExpr*/ nullptr,
//                  /*ParentDC*/ errorWrapper, /*VarLoc*/ loc);
//               errorWrapper->addMember(nsErrorProp);
//               errorWrapper->addMember(nsErrorBinding);
//
//               // Create the _nsError initializer.
//               //   public init(_nsError error: NSError)
//               VarDecl *members[1] = { nsErrorProp };
//               auto nsErrorInit = createValueConstructor(Impl, errorWrapper, members,
//                  /*wantCtorParamNames=*/true,
//                  /*wantBody=*/true);
//               errorWrapper->addMember(nsErrorInit);
//
//               // Add the domain error member.
//               //   public static var errorDomain: String { return error-domain }
//               addErrorDomain(errorWrapper, enumInfo.getErrorDomain(), Impl);
//
//               // Note: the Code will be added after it's created.
//
//               // The enum itself will be nested within the error wrapper,
//               // and be named Code.
//               enumDC = errorWrapper;
//               enumName = C.Id_Code;
//            }

            // Create the enumeration.
            auto enumDecl = Impl.createDeclWithClangNode<EnumDecl>(
               decl, AccessLevel::Public, loc, enumName,
               Impl.importSourceLoc(decl->getLocation()), None, nullptr, enumDC);
            enumDecl->setHasFixedRawValues();

            // Annotate as 'frozen' if appropriate.
            if (enumKind == EnumKind::FrozenEnum)
               enumDecl->getAttrs().add(new (C) FrozenAttr(/*implicit*/false));

            // Set up the C underlying type as its Swift raw type.
            enumDecl->setRawType(underlyingType);

            // Add the C name.
//            addObjCAttribute(enumDecl,
//                             Impl.importIdentifier(decl->getIdentifier()));

            // Add protocol declarations to the enum declaration.
            SmallVector<TypeLoc, 2> inheritedTypes;
            inheritedTypes.push_back(TypeLoc::withoutLoc(underlyingType));
            enumDecl->setInherited(C.AllocateCopy(inheritedTypes));

            if (errorWrapper) {
               addSynthesizedInterfaceAttrs(Impl, enumDecl,
                                            {KnownInterfaceKind::ErrorCodeInterface,
                                             KnownInterfaceKind::RawRepresentable});
            } else {
               addSynthesizedInterfaceAttrs(Impl, enumDecl,
                                            {KnownInterfaceKind::RawRepresentable});
            }

            // Provide custom implementations of the init(rawValue:) and rawValue
            // conversions that just do a bitcast. We can't reliably filter a
            // C enum without additional knowledge that the type has no
            // undeclared values, and won't ever add cases.
            auto rawValueConstructor = makeEnumRawValueConstructor(Impl, enumDecl);

            auto varName = C.Id_rawValue;
            auto rawValue = new (C) VarDecl(/*IsStatic*/false,
                                                        VarDecl::Introducer::Var,
               /*IsCaptureList*/false,
                                                        SourceLoc(), varName,
                                                        enumDecl);
            rawValue->setImplicit();
            rawValue->setAccess(AccessLevel::Public);
            rawValue->setSetterAccess(AccessLevel::Private);
            rawValue->setInterfaceType(underlyingType);

            // Create a pattern binding to describe the variable.
            Pattern *varPattern = createTypedNamedPattern(rawValue);

            auto *rawValueBinding = PatternBindingDecl::createImplicit(
               C, StaticSpellingKind::None, varPattern, /*InitExpr*/ nullptr,
               enumDecl);

            makeEnumRawValueGetter(Impl, enumDecl, rawValue);

            enumDecl->addMember(rawValueConstructor);
            enumDecl->addMember(rawValue);
            enumDecl->addMember(rawValueBinding);

            addSynthesizedTypealias(enumDecl, C.Id_RawValue, underlyingType);
            Impl.RawTypes[enumDecl] = underlyingType;

            // If we have an error wrapper, finish it up now that its
            // nested enum has been constructed.
            if (errorWrapper) {
               // Add the ErrorType alias:
               //   public typealias ErrorType
               auto alias = Impl.createDeclWithClangNode<TypeAliasDecl>(
                  decl,
                  AccessLevel::Public, loc, SourceLoc(),
                  C.Id_ErrorType, loc,
                  /*genericparams=*/nullptr, enumDecl);
               alias->setUnderlyingType(errorWrapper->getDeclaredInterfaceType());
               enumDecl->addMember(alias);

               // Add the 'Code' enum to the error wrapper.
               errorWrapper->addMember(enumDecl);
               Impl.addAlternateDecl(enumDecl, errorWrapper);

               // Stash the 'Code' enum so we can find it later.
               Impl.ErrorCodeEnums[errorWrapper] = enumDecl;
            }

            // The enumerators go into this enumeration.
            result = enumDecl;
            break;
         }

         case EnumKind::Options: {
            result = importAsOptionSetType(dc, name, decl);
            if (!result)
               return nullptr;

            // HACK: Make sure PrintAsObjC always omits the 'enum' tag for
            // option set enums.
            Impl.DeclsWithSuperfluousTypedefs.insert(decl);
            break;
         }
      }

      const clang::EnumDecl *canonicalClangDecl = decl->getCanonicalDecl();
      Impl.ImportedDecls[{canonicalClangDecl, getVersion()}] = result;

      // Import each of the enumerators.

      bool addEnumeratorsAsMembers;
      switch (enumKind) {
         case EnumKind::Constants:
         case EnumKind::Unknown:
            addEnumeratorsAsMembers = false;
            break;
         case EnumKind::Options:
         case EnumKind::NonFrozenEnum:
         case EnumKind::FrozenEnum:
            addEnumeratorsAsMembers = true;
            break;
      }

      llvm::SmallDenseMap<const llvm::APSInt *,
         PointerUnion<const clang::EnumConstantDecl *,
            EnumElementDecl *>, 8,
         APSIntRefDenseMapInfo> canonicalEnumConstants;

      if (enumKind == EnumKind::NonFrozenEnum ||
          enumKind == EnumKind::FrozenEnum) {
         for (auto constant : decl->enumerators()) {
            if (Impl.isUnavailableInSwift(constant))
               continue;
            canonicalEnumConstants.insert({&constant->getInitVal(), constant});
         }
      }

      auto contextIsEnum = [&](const ImportedName &name) -> bool {
         EffectiveClangContext importContext = name.getEffectiveContext();
         switch (importContext.getKind()) {
            case EffectiveClangContext::DeclContext:
               return importContext.getAsDeclContext() == canonicalClangDecl;
            case EffectiveClangContext::TypedefContext: {
               auto *typedefName = importContext.getTypedefName();
               clang::QualType underlyingTy = typedefName->getUnderlyingType();
               return underlyingTy->getAsTagDecl() == canonicalClangDecl;
            }
            case EffectiveClangContext::UnresolvedContext:
               // Assume this is a context other than the enum.
               return false;
         }
         llvm_unreachable("unhandled kind");
      };

      for (auto constant : decl->enumerators()) {
         Decl *enumeratorDecl = nullptr;
         TinyPtrVector<Decl *> variantDecls;
         switch (enumKind) {
            case EnumKind::Constants:
            case EnumKind::Unknown:
               Impl.forEachDistinctName(constant,
                                        [&](ImportedName newName,
                                            ImportNameVersion nameVersion) -> bool {
                                           Decl *imported = Impl.importDecl(constant, nameVersion);
                                           if (!imported)
                                              return false;
                                           if (nameVersion == getActivePolarphpVersion())
                                              enumeratorDecl = imported;
                                           else
                                              variantDecls.push_back(imported);
                                           return true;
                                        });
               break;
            case EnumKind::Options:
               Impl.forEachDistinctName(constant,
                                        [&](ImportedName newName,
                                            ImportNameVersion nameVersion) -> bool {
                                           if (!contextIsEnum(newName))
                                              return true;
                                           TypePHPDeclConverter converter(Impl, nameVersion);
                                           Decl *imported =
                                              converter.importOptionConstant(constant, decl, result);
                                           if (!imported)
                                              return false;
                                           if (nameVersion == getActivePolarphpVersion())
                                              enumeratorDecl = imported;
                                           else
                                              variantDecls.push_back(imported);
                                           return true;
                                        });
               break;
            case EnumKind::NonFrozenEnum:
            case EnumKind::FrozenEnum: {
               auto canonicalCaseIter =
                  canonicalEnumConstants.find(&constant->getInitVal());

               if (canonicalCaseIter == canonicalEnumConstants.end()) {
                  // Unavailable declarations get no special treatment.
                  enumeratorDecl =
                     TypePHPDeclConverter(Impl, getActivePolarphpVersion())
                        .importEnumCase(constant, decl, cast<EnumDecl>(result));
               } else {
                  const clang::EnumConstantDecl *unimported =
                     canonicalCaseIter->
                        second.dyn_cast<const clang::EnumConstantDecl *>();

                  // Import the canonical enumerator for this case first.
                  if (unimported) {
                     enumeratorDecl = TypePHPDeclConverter(Impl, getActivePolarphpVersion())
                        .importEnumCase(unimported, decl, cast<EnumDecl>(result));
                     if (enumeratorDecl) {
                        canonicalCaseIter->getSecond() =
                           cast<EnumElementDecl>(enumeratorDecl);
                     }
                  } else {
                     enumeratorDecl =
                        canonicalCaseIter->second.get<EnumElementDecl *>();
                  }

                  if (unimported != constant && enumeratorDecl) {
                     ImportedName importedName =
                        Impl.importFullName(constant, getActivePolarphpVersion());
                     Identifier name = importedName.getDeclName().getBaseIdentifier();
                     if (name.empty()) {
                        // Clear the existing declaration so we don't try to process it
                        // twice later.
                        enumeratorDecl = nullptr;
                     } else {
                        auto original = cast<ValueDecl>(enumeratorDecl);
                        enumeratorDecl = importEnumCaseAlias(name, constant, original,
                                                             decl, result);
                     }
                  }
               }

               Impl.forEachDistinctName(constant,
                                        [&](ImportedName newName,
                                            ImportNameVersion nameVersion) -> bool {
                                           if (nameVersion == getActivePolarphpVersion())
                                              return true;
                                           if (!contextIsEnum(newName))
                                              return true;
                                           TypePHPDeclConverter converter(Impl, nameVersion);
                                           Decl *imported =
                                              converter.importEnumCase(constant, decl, cast<EnumDecl>(result),
                                                                       enumeratorDecl);
                                           if (!imported)
                                              return false;
                                           variantDecls.push_back(imported);
                                           return true;
                                        });
               break;
            }
         }
         if (!enumeratorDecl)
            continue;

         if (addEnumeratorsAsMembers) {
            // Add a member enumerator to the given nominal type.
            auto addDecl = [&](NominalTypeDecl *nominal, Decl *decl) {
               if (!decl) return;
               nominal->addMember(decl);
            };

            addDecl(result, enumeratorDecl);
            for (auto *variant : variantDecls)
               addDecl(result, variant);

            // If there is an error wrapper, add an alias within the
            // wrapper to the corresponding value within the enumerator
            // context.
            if (errorWrapper) {
               auto enumeratorValue = cast<ValueDecl>(enumeratorDecl);
               auto name = enumeratorValue->getBaseName().getIdentifier();
               auto alias = importEnumCaseAlias(name,
                                                constant,
                                                enumeratorValue,
                                                decl,
                                                result,
                                                errorWrapper);
               addDecl(errorWrapper, alias);
            }
         }
      }

      return result;
   }

   Decl *VisitRecordDecl(const clang::RecordDecl *decl) {
      // Track whether this record contains fields we can't reference in Swift
      // as stored properties.
      bool hasUnreferenceableStorage = false;

      // Track whether this record contains fields that can't be zero-
      // initialized.
      bool hasZeroInitializableStorage = true;

      // Track whether all fields in this record can be referenced in Swift,
      // either as stored or computed properties, in which case the record type
      // gets a memberwise initializer.
      bool hasMemberwiseInitializer = true;

      if (decl->isUnion()) {
         hasUnreferenceableStorage = true;

         // We generate initializers specially for unions below.
         hasMemberwiseInitializer = false;
      }

      // FIXME: Skip Microsoft __interfaces.
      if (decl->isInterface())
         return nullptr;

      // FIXME: Figure out how to deal with incomplete types, since that
      // notion doesn't exist in Swift.
      decl = decl->getDefinition();
      if (!decl) {
         forwardDeclaration = true;
         return nullptr;
      }

      // Don't import nominal types that are over-aligned.
      if (Impl.isOverAligned(decl))
         return nullptr;

      // FIXME: We should actually support strong ARC references and similar in
      // C structs. That'll require some SIL and IRGen work, though.
      if (decl->isNonTrivialToPrimitiveCopy() ||
          decl->isNonTrivialToPrimitiveDestroy()) {
         // Note that there is a third predicate related to these,
         // isNonTrivialToPrimitiveDefaultInitialize. That one's not important
         // for us because Swift never "trivially default-initializes" a struct
         // (i.e. uses whatever bits were lying around as an initial value).

         // FIXME: It would be nice to instead import the declaration but mark
         // it as unavailable, but then it might get used as a type for an
         // imported function and the developer would be able to use it without
         // referencing the name, which would sidestep our availability
         // diagnostics.
         return nullptr;
      }

      // Import the name.
      Optional<ImportedName> correctPolarphpName;
      auto importedName = getClangDeclName(decl, correctPolarphpName);
      if (!importedName)
         return nullptr;

      // If we've been asked to produce a compatibility stub, handle it via a
      // typealias.
      if (correctPolarphpName)
         return importCompatibilityTypeAlias(decl, importedName,
                                             *correctPolarphpName);

      auto dc =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!dc)
         return nullptr;

      // Create the struct declaration and record it.
      auto name = importedName.getDeclName().getBaseIdentifier();
      auto result = Impl.createDeclWithClangNode<StructDecl>(decl,
                                                             AccessLevel::Public,
                                                             Impl.importSourceLoc(decl->getBeginLoc()),
                                                             name,
                                                             Impl.importSourceLoc(decl->getLocation()),
                                                             None, nullptr, dc);
      Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = result;

      // FIXME: Figure out what to do with superclasses in C++. One possible
      // solution would be to turn them into members and add conversion
      // functions.

      // Import each of the members.
      SmallVector<VarDecl *, 4> members;
      SmallVector<FuncDecl *, 4> methods;
      SmallVector<ConstructorDecl *, 4> ctors;

      // FIXME: Import anonymous union fields and support field access when
      // it is nested in a struct.

      for (auto m : decl->decls()) {
         auto nd = dyn_cast<clang::NamedDecl>(m);
         if (!nd) {
            // We couldn't import the member, so we can't reference it in Swift.
            hasUnreferenceableStorage = true;
            hasMemberwiseInitializer = false;
            continue;
         }

         if (auto field = dyn_cast<clang::FieldDecl>(nd)) {
            // Non-nullable pointers can't be zero-initialized.
            if (auto nullability = field->getType()
               ->getNullability(Impl.getClangAstContext())) {
               if (*nullability == clang::NullabilityKind::NonNull)
                  hasZeroInitializableStorage = false;
            }
            // TODO: If we had the notion of a closed enum with no private
            // cases or resilience concerns, then complete NS_ENUMs with
            // no case corresponding to zero would also not be zero-
            // initializable.

            // Unnamed bitfields are just for padding and should not
            // inhibit creation of a memberwise initializer.
            if (field->isUnnamedBitfield()) {
               hasUnreferenceableStorage = true;
               continue;
            }
         }

         auto member = Impl.importDecl(nd, getActivePolarphpVersion());
         if (!member) {
            if (!isa<clang::TypeDecl>(nd)) {
               // We don't know what this field is.
               // Assume it may be important in C.
               hasUnreferenceableStorage = true;
               hasMemberwiseInitializer = false;
            }
            continue;
         }

         if (isa<TypeDecl>(member)) {
            // A struct nested inside another struct will either be logically
            // a sibling of the outer struct, or contained inside of it, depending
            // on if it has a declaration name or not.
            //
            // struct foo { struct bar { ... } baz; } // sibling
            // struct foo { struct { ... } baz; } // child
            //
            // In the latter case, we add the imported type as a nested type
            // of the parent.
            //
            // TODO: C++ types have different rules.
            if (auto nominalDecl = dyn_cast<NominalTypeDecl>(member->getDeclContext())) {
               assert(nominalDecl == result && "interesting nesting of C types?");
               nominalDecl->addMember(member);
            }
            continue;
         }

         if (auto MD = dyn_cast<FuncDecl>(member)) {
            methods.push_back(MD);
            continue;
         }
         auto VD = cast<VarDecl>(member);

         if (isa<clang::IndirectFieldDecl>(nd) || decl->isUnion()) {
            // Don't import unavailable fields that have no associated storage.
            if (VD->getAttrs().isUnavailable(Impl.TypePHPContext)) {
               continue;
            }
         }

         members.push_back(VD);

         // Bitfields are imported as computed properties with Clang-generated
         // accessors.
         bool isBitField = false;
         if (auto field = dyn_cast<clang::FieldDecl>(nd)) {
            if (field->isBitField()) {
               // We can't represent this struct completely in SIL anymore,
               // but we're still able to define a memberwise initializer.
               hasUnreferenceableStorage = true;
               isBitField = true;

               makeBitFieldAccessors(Impl,
                                     const_cast<clang::RecordDecl *>(decl),
                                     result,
                                     const_cast<clang::FieldDecl *>(field),
                                     VD);
            }
         }

         if (auto ind = dyn_cast<clang::IndirectFieldDecl>(nd)) {
            // Indirect fields are created as computed property accessible the
            // fields on the anonymous field from which they are injected.
            makeIndirectFieldAccessors(Impl, ind, members, result, VD);
         } else if (decl->isUnion() && !isBitField) {
            // Union fields should only be available indirectly via a computed
            // property. Since the union is made of all of the fields at once,
            // this is a trivial accessor that casts self to the correct
            // field type.
            makeUnionFieldAccessors(Impl, result, VD);

            // Create labeled initializers for unions that take one of the
            // fields, which only initializes the data for that field.
            auto valueCtor =
               createValueConstructor(Impl, result, VD,
                  /*want param names*/true,
                  /*wantBody=*/true);
            ctors.push_back(valueCtor);
         }
      }

      bool hasReferenceableFields = !members.empty();

      if (hasZeroInitializableStorage) {
         // Add constructors for the struct.
         ctors.push_back(createDefaultConstructor(Impl, result));
      }

      if (hasReferenceableFields && hasMemberwiseInitializer) {
         // The default zero initializer suppresses the implicit value
         // constructor that would normally be formed, so we have to add that
         // explicitly as well.
         //
         // If we can completely represent the struct in SIL, leave the body
         // implicit, otherwise synthesize one to call property setters.
         auto valueCtor = createValueConstructor(
            Impl, result, members,
            /*want param names*/true,
            /*want body*/hasUnreferenceableStorage);
         if (!hasUnreferenceableStorage)
            valueCtor->setIsMemberwiseInitializer();

         ctors.push_back(valueCtor);
      }

      for (auto member : members) {
         result->addMember(member);
      }

      for (auto ctor : ctors) {
         result->addMember(ctor);
      }

      for (auto method : methods) {
         result->addMember(method);
      }

      result->setHasUnreferenceableStorage(hasUnreferenceableStorage);

      return result;
   }

   Decl *VisitClassTemplateSpecializationDecl(
      const clang::ClassTemplateSpecializationDecl *decl) {
      // FIXME: We could import specializations, but perhaps only as unnamed
      // structural types.
      return nullptr;
   }

   Decl *VisitClassTemplatePartialSpecializationDecl(
      const clang::ClassTemplatePartialSpecializationDecl *decl) {
      // Note: templates are not imported.
      return nullptr;
   }

   Decl *VisitTemplateTypeParmDecl(const clang::TemplateTypeParmDecl *decl) {
      // Note: templates are not imported.
      return nullptr;
   }

   Decl *VisitEnumConstantDecl(const clang::EnumConstantDecl *decl) {
      auto clangEnum = cast<clang::EnumDecl>(decl->getDeclContext());

      Optional<ImportedName> correctPolarphpName;
      auto importedName = importFullName(decl, correctPolarphpName);
      if (!importedName) return nullptr;

      auto name = importedName.getDeclName().getBaseIdentifier();
      if (name.empty())
         return nullptr;

      switch (Impl.getEnumKind(clangEnum)) {
         case EnumKind::Constants: {
            // The enumeration was simply mapped to an integral type. Create a
            // constant with that integral type.

            // The context where the constant will be introduced.
            auto dc =
               Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
            if (!dc)
               return nullptr;

            // Enumeration type.
            auto &clangContext = Impl.getClangAstContext();
            auto type = Impl.importTypeIgnoreIUO(
               clangContext.getTagDeclType(clangEnum), ImportTypeKind::Value,
               isInSystemModule(dc), Bridgeability::None);
            if (!type)
               return nullptr;
            // FIXME: Importing the type will recursively revisit this same
            // EnumConstantDecl. Short-circuit out if we already emitted the import
            // for this decl.
            if (auto Known = Impl.importDeclCached(decl, getVersion()))
               return Known;

            // Create the global constant.
            auto result = Impl.createConstant(name, dc, type,
                                              clang::APValue(decl->getInitVal()),
                                              ConstantConvertKind::None,
               /*static*/dc->isTypeContext(), decl);
            Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = result;

            // If this is a compatibility stub, mark it as such.
            if (correctPolarphpName)
               markAsVariant(result, *correctPolarphpName);

            return result;
         }

         case EnumKind::Unknown: {
            // The enumeration was mapped to a struct containing the integral
            // type. Create a constant with that struct type.

            // The context where the constant will be introduced.
            auto dc =
               Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
            if (!dc)
               return nullptr;

            // Import the enumeration type.
            auto enumType = Impl.importTypeIgnoreIUO(
               Impl.getClangAstContext().getTagDeclType(clangEnum),
               ImportTypeKind::Value, isInSystemModule(dc), Bridgeability::None);
            if (!enumType)
               return nullptr;

            // FIXME: Importing the type will can recursively revisit this same
            // EnumConstantDecl. Short-circuit out if we already emitted the import
            // for this decl.
            if (auto Known = Impl.importDeclCached(decl, getVersion()))
               return Known;

            // Create the global constant.
            auto result = Impl.createConstant(name, dc, enumType,
                                              clang::APValue(decl->getInitVal()),
                                              ConstantConvertKind::Construction,
               /*static*/ false, decl);
            Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = result;

            // If this is a compatibility stub, mark it as such.
            if (correctPolarphpName)
               markAsVariant(result, *correctPolarphpName);

            return result;
         }

         case EnumKind::NonFrozenEnum:
         case EnumKind::FrozenEnum:
         case EnumKind::Options: {
            // The enumeration was mapped to a high-level Swift type, and its
            // elements were created as children of that enum. They aren't available
            // independently.

            // FIXME: This is gross. We shouldn't have to import
            // everything to get at the individual constants.
            return nullptr;
         }
      }

      llvm_unreachable("Invalid EnumKind.");
   }


   Decl *
   VisitUnresolvedUsingValueDecl(const clang::UnresolvedUsingValueDecl *decl) {
      // Note: templates are not imported.
      return nullptr;
   }

   Decl *VisitIndirectFieldDecl(const clang::IndirectFieldDecl *decl) {
      Optional<ImportedName> correctPolarphpName;
      auto importedName = importFullName(decl, correctPolarphpName);
      if (!importedName) return nullptr;

      auto name = importedName.getDeclName().getBaseIdentifier();

      auto dc =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!dc)
         return nullptr;

      auto importedType =
         Impl.importType(decl->getType(), ImportTypeKind::Variable,
                         isInSystemModule(dc), Bridgeability::None);
      if (!importedType)
         return nullptr;

      auto type = importedType.getType();

      // Map this indirect field to a Swift variable.
      auto result = Impl.createDeclWithClangNode<VarDecl>(decl,
                                                          AccessLevel::Public,
         /*IsStatic*/false,
                                                          VarDecl::Introducer::Var,
         /*IsCaptureList*/false,
                                                          Impl.importSourceLoc(decl->getBeginLoc()),
                                                          name, dc);
      result->setInterfaceType(type);
      // @todo
//      result->setIsObjC(false);
      result->setIsDynamic(false);
      Impl.recordImplicitUnwrapForDecl(result,
                                       importedType.isImplicitlyUnwrapped());

      // If this is a compatibility stub, mark is as such.
      if (correctPolarphpName)
         markAsVariant(result, *correctPolarphpName);

      return result;
   }

   ParameterList *getNonSelfParamList(
      DeclContext *dc, const clang::FunctionDecl *decl,
      Optional<unsigned> selfIdx, ArrayRef<Identifier> argNames,
      bool allowNSUIntegerAsInt, bool isAccessor) {
      if (bool(selfIdx)) {
         assert(((decl->getNumParams() == argNames.size() + 1) || isAccessor) &&
                (*selfIdx < decl->getNumParams()) && "where's self?");
      } else {
         assert(decl->getNumParams() == argNames.size() || isAccessor);
      }

      SmallVector<const clang::ParmVarDecl *, 4> nonSelfParams;
      for (unsigned i = 0; i < decl->getNumParams(); ++i) {
         if (selfIdx && i == *selfIdx)
            continue;
         nonSelfParams.push_back(decl->getParamDecl(i));
      }
      return Impl.importFunctionParameterList(dc, decl, nonSelfParams,
                                              decl->isVariadic(),
                                              allowNSUIntegerAsInt, argNames);
   }

   Decl *importGlobalAsInitializer(const clang::FunctionDecl *decl,
                                   DeclName name, DeclContext *dc,
                                   CtorInitializerKind initKind,
                                   Optional<ImportedName> correctPolarphpName);

   /// Create an implicit property given the imported name of one of
   /// the accessors.
   VarDecl *getImplicitProperty(ImportedName importedName,
                                const clang::FunctionDecl *accessor);

   Decl *VisitFunctionDecl(const clang::FunctionDecl *decl) {
      // Import the name of the function.
      Optional<ImportedName> correctPolarphpName;
      auto importedName = importFullName(decl, correctPolarphpName);
      if (!importedName)
         return nullptr;

      AbstractStorageDecl *owningStorage;
      switch (importedName.getAccessorKind()) {
         case ImportedAccessorKind::None:
            owningStorage = nullptr;
            break;

         case ImportedAccessorKind::SubscriptGetter:
         case ImportedAccessorKind::SubscriptSetter:
            llvm_unreachable("Not possible for a function");

         case ImportedAccessorKind::PropertyGetter: {
            auto property = getImplicitProperty(importedName, decl);
            if (!property) return nullptr;
            return property->getParsedAccessor(AccessorKind::Get);
         }

         case ImportedAccessorKind::PropertySetter:
            auto property = getImplicitProperty(importedName, decl);
            if (!property) return nullptr;
            return property->getParsedAccessor(AccessorKind::Set);
      }

      return importFunctionDecl(decl, importedName, correctPolarphpName, None);
   }

   Decl *importFunctionDecl(const clang::FunctionDecl *decl,
                            ImportedName importedName,
                            Optional<ImportedName> correctPolarphpName,
                            Optional<AccessorInfo> accessorInfo) {
      auto dc =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!dc)
         return nullptr;

      DeclName name = accessorInfo ? DeclName() : importedName.getDeclName();
      auto selfIdx = importedName.getSelfIndex();

      FuncDecl *result = nullptr;
      ImportedType importedType;
      bool selfIsInOut = false;
      ParameterList *bodyParams = nullptr;
      if (!dc->isModuleScopeContext() && !isa<clang::CXXMethodDecl>(decl)) {
         // Handle initializers.
         if (name.getBaseName() == DeclBaseName::createConstructor()) {
            assert(!accessorInfo);
            return importGlobalAsInitializer(decl, name, dc,
                                             importedName.getInitKind(),
                                             correctPolarphpName);
         }

         if (dc->getSelfInterfaceDecl() && !selfIdx) {
            // FIXME: source location...
            Impl.TypePHPContext.Diags.diagnose({}, diag::swift_name_protocol_static,
               /*isInit=*/false);
            Impl.TypePHPContext.Diags.diagnose({}, diag::note_while_importing,
                                               decl->getName());
            return nullptr;
         }

         if (!decl->hasPrototype()) {
            // FIXME: source location...
            Impl.TypePHPContext.Diags.diagnose({}, diag::swift_name_no_prototype);
            Impl.TypePHPContext.Diags.diagnose({}, diag::note_while_importing,
                                               decl->getName());
            return nullptr;
         }

         // There is an inout 'self' when the parameter is a pointer to a
         // non-const instance of the type we're importing onto. Importing this
         // as a method means that the method should be treated as mutating in
         // this situation.
         if (selfIdx &&
             !dc->getDeclaredInterfaceType()->hasReferenceSemantics()) {
            auto selfParam = decl->getParamDecl(*selfIdx);
            auto selfParamTy = selfParam->getType();
            if ((selfParamTy->isPointerType() ||
                 selfParamTy->isReferenceType()) &&
                !selfParamTy->getPointeeType().isConstQualified()) {
               selfIsInOut = true;

               // If there's a swift_newtype, check the levels of indirection: self
               // is only inout if this is a pointer to the typedef type (which
               // itself is a pointer).
               if (auto nominalTypeDecl = dc->getSelfNominalTypeDecl()) {
                  if (auto clangDCTy = dyn_cast_or_null<clang::TypedefNameDecl>(
                     nominalTypeDecl->getClangDecl()))
                     if (getSwiftNewtypeAttr(clangDCTy, getVersion()))
                        if (clangDCTy->getUnderlyingType().getCanonicalType() !=
                            selfParamTy->getPointeeType().getCanonicalType())
                           selfIsInOut = false;
               }
            }
         }

         bool allowNSUIntegerAsInt =
            Impl.shouldAllowNSUIntegerAsInt(isInSystemModule(dc), decl);

         bodyParams =
            getNonSelfParamList(dc, decl, selfIdx, name.getArgumentNames(),
                                allowNSUIntegerAsInt, !name);

         importedType =
            Impl.importFunctionReturnType(dc, decl, allowNSUIntegerAsInt);
      } else {
         // Import the function type. If we have parameters, make sure their
         // names get into the resulting function type.
         importedType = Impl.importFunctionParamsAndReturnType(
            dc, decl, {decl->param_begin(), decl->param_size()},
            decl->isVariadic(), isInSystemModule(dc), name, bodyParams);

         if (auto *mdecl = dyn_cast<clang::CXXMethodDecl>(decl)) {
            if (!mdecl->isStatic()) {
               selfIdx = 0;
               // Workaround until proper const support is handled: Force
               // everything to be mutating. This implicitly makes the parameter
               // indirect.
               selfIsInOut = true;
            } else {
               selfIdx = None;
            }
         }
      }

      if (name && name.isSimpleName()) {
         assert(importedName.hasCustomName() &&
                "imported function with simple name?");
         // Just fill in empty argument labels.
         name = DeclName(Impl.TypePHPContext, name.getBaseName(), bodyParams);
      }

      if (!importedType)
         return nullptr;

      auto resultTy = importedType.getType();
      auto loc = Impl.importSourceLoc(decl->getLocation());

      // FIXME: Poor location info.
      auto nameLoc = Impl.importSourceLoc(decl->getLocation());
      result = createFuncOrAccessor(Impl.TypePHPContext, loc, accessorInfo, name,
                                    nameLoc, bodyParams, resultTy,
         /*throws*/ false, dc, decl);

      if (!dc->isModuleScopeContext()) {
         if (selfIsInOut)
            result->setSelfAccessKind(SelfAccessKind::Mutating);
         else
            result->setSelfAccessKind(SelfAccessKind::NonMutating);
         if (selfIdx) {
            result->setSelfIndex(selfIdx.getValue());
         } else {
            result->setStatic();
            result->setImportAsStaticMember();
         }
      }

      // @todo

//      result->setIsObjC(false);
      result->setIsDynamic(false);

      Impl.recordImplicitUnwrapForDecl(result,
                                       importedType.isImplicitlyUnwrapped());

      if (dc->getSelfClassDecl())
         // FIXME: only if the class itself is not marked final
         result->getAttrs().add(new (Impl.TypePHPContext)
                                   FinalAttr(/*IsImplicit=*/true));

      // Someday, maybe this will need to be 'open' for C++ virtual methods.
      result->setAccess(AccessLevel::Public);
      finishFuncDecl(decl, result);

      // If this is a compatibility stub, mark it as such.
      if (correctPolarphpName)
         markAsVariant(result, *correctPolarphpName);

      return result;
   }

   void finishFuncDecl(const clang::FunctionDecl *decl,
                       AbstractFunctionDecl *result) {
      // Set availability.
      if (decl->isVariadic()) {
         Impl.markUnavailable(result, "Variadic function is unavailable");
      }

      if (decl->hasAttr<clang::ReturnsTwiceAttr>()) {
         // The Clang 'returns_twice' attribute is used for functions like
         // 'vfork' or 'setjmp'. Because these functions may return control flow
         // of a Swift program to an arbitrary point, Swift's guarantees of
         // definitive initialization of variables cannot be upheld. As a result,
         // functions like these cannot be used in Swift.
         Impl.markUnavailable(
            result,
            "Functions that may return more than one time (annotated with the "
            "'returns_twice' attribute) are unavailable in Swift");
      }
      /// TODO
//      recordObjCOverride(result);
   }

   Decl *VisitCXXMethodDecl(const clang::CXXMethodDecl *decl) {
      return VisitFunctionDecl(decl);
   }

   Decl *VisitFieldDecl(const clang::FieldDecl *decl) {
      // Fields are imported as variables.
      Optional<ImportedName> correctPolarphpName;
      ImportedName importedName;

      if (!decl->isAnonymousStructOrUnion()) {
         importedName = importFullName(decl, correctPolarphpName);
         if (!importedName) {
            return nullptr;
         }
      } else {
         // Generate a field name for anonymous fields, this will be used in
         // order to be able to expose the indirect fields injected from there
         // as computed properties forwarding the access to the subfield.
         std::string Id;
         llvm::raw_string_ostream IdStream(Id);

         IdStream << "__Anonymous_field" << decl->getFieldIndex();
         importedName.setDeclName(Impl.TypePHPContext.getIdentifier(IdStream.str()));
         importedName.setEffectiveContext(decl->getDeclContext());
      }

      auto name = importedName.getDeclName().getBaseIdentifier();

      auto dc =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!dc)
         return nullptr;

      auto importedType =
         Impl.importType(decl->getType(), ImportTypeKind::RecordField,
                         isInSystemModule(dc), Bridgeability::None);
      if (!importedType)
         return nullptr;

      auto type = importedType.getType();

      auto result =
         Impl.createDeclWithClangNode<VarDecl>(decl, AccessLevel::Public,
            /*IsStatic*/ false,
                                               VarDecl::Introducer::Var,
            /*IsCaptureList*/false,
                                               Impl.importSourceLoc(decl->getLocation()),
                                               name, dc);
      // @todo
//      result->setIsObjC(false);
      result->setIsDynamic(false);
      result->setInterfaceType(type);
      Impl.recordImplicitUnwrapForDecl(result,
                                       importedType.isImplicitlyUnwrapped());

      // Handle attributes.
      if (decl->hasAttr<clang::IBOutletAttr>())
         result->getAttrs().add(
            new (Impl.TypePHPContext) IBOutletAttr(/*IsImplicit=*/false));
      // FIXME: Handle IBOutletCollection.

      // If this is a compatibility stub, handle it as such.
      if (correctPolarphpName)
         markAsVariant(result, *correctPolarphpName);

      return result;
   }

   Decl *VisitObjCIvarDecl(const clang::ObjCIvarDecl *decl) {
      // Disallow direct ivar access (and avoid conflicts with property names).
      return nullptr;
   }

   Decl *VisitObjCAtDefsFieldDecl(const clang::ObjCAtDefsFieldDecl *decl) {
      // @defs is an anachronism; ignore it.
      return nullptr;
   }

   Decl *VisitVarDecl(const clang::VarDecl *decl) {
      // FIXME: Swift does not have static variables in structs/classes yet.
      if (decl->getDeclContext()->isRecord())
         return nullptr;

      // Variables are imported as... variables.
      Optional<ImportedName> correctPolarphpName;
      auto importedName = importFullName(decl, correctPolarphpName);
      if (!importedName) return nullptr;

      auto name = importedName.getDeclName().getBaseIdentifier();
      auto dc =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!dc)
         return nullptr;

      // If the declaration is const, consider it audited.
      // We can assume that loading a const global variable doesn't
      // involve an ownership transfer.
      bool isAudited = decl->getType().isConstQualified();

      auto declType = decl->getType();

      // Special case: NS Notifications
      if (isNSNotificationGlobal(decl))
         if (auto newtypeDecl = findSwiftNewtype(decl, Impl.getClangSema(),
                                                 Impl.CurrentVersion))
            declType = Impl.getClangAstContext().getTypedefType(newtypeDecl);

      // Note that we deliberately don't bridge most globals because we want to
      // preserve pointer identity.
      auto importedType =
         Impl.importType(declType,
                         (isAudited ? ImportTypeKind::AuditedVariable
                                    : ImportTypeKind::Variable),
                         isInSystemModule(dc), Bridgeability::None);

      if (!importedType)
         return nullptr;

      auto type = importedType.getType();

      // If we've imported this variable as a member, it's a static
      // member.
      bool isStatic = false;
      if (dc->isTypeContext())
         isStatic = true;

      auto introducer = Impl.shouldImportGlobalAsLet(decl->getType())
                        ? VarDecl::Introducer::Let
                        : VarDecl::Introducer::Var;
      auto result = Impl.createDeclWithClangNode<VarDecl>(decl,
                                                          AccessLevel::Public,
         /*IsStatic*/isStatic, introducer,
         /*IsCaptureList*/false,
                                                          Impl.importSourceLoc(decl->getLocation()),
                                                          name, dc);
      // @todo
//      result->setIsObjC(false);
      result->setIsDynamic(false);
      result->setInterfaceType(type);
      Impl.recordImplicitUnwrapForDecl(result,
                                       importedType.isImplicitlyUnwrapped());

      // If imported as member, the member should be final.
      if (dc->getSelfClassDecl())
         result->getAttrs().add(new (Impl.TypePHPContext)
                                   FinalAttr(/*IsImplicit=*/true));

      // If this is a compatibility stub, mark it as such.
      if (correctPolarphpName)
         markAsVariant(result, *correctPolarphpName);

      return result;
   }

   Decl *VisitImplicitParamDecl(const clang::ImplicitParamDecl *decl) {
      // Parameters are never directly imported.
      return nullptr;
   }

   Decl *VisitParmVarDecl(const clang::ParmVarDecl *decl) {
      // Parameters are never directly imported.
      return nullptr;
   }

   Decl *
   VisitNonTypeTemplateParmDecl(const clang::NonTypeTemplateParmDecl *decl) {
      // Note: templates are not imported.
      return nullptr;
   }

   Decl *VisitTemplateDecl(const clang::TemplateDecl *decl) {
      // Note: templates are not imported.
      return nullptr;
   }

   Decl *VisitUsingDecl(const clang::UsingDecl *decl) {
      // Using declarations are not imported.
      return nullptr;
   }

   Decl *VisitUsingShadowDecl(const clang::UsingShadowDecl *decl) {
      // Only import types for now.
      if (!isa<clang::TypeDecl>(decl->getUnderlyingDecl()))
         return nullptr;

      Optional<ImportedName> correctPolarphpName;
      auto importedName = importFullName(decl, correctPolarphpName);
      auto Name = importedName.getDeclName().getBaseIdentifier();
      if (Name.empty())
         return nullptr;

      // If we've been asked to produce a compatibility stub, handle it via a
      // typealias.
      if (correctPolarphpName)
         return importCompatibilityTypeAlias(decl, importedName,
                                             *correctPolarphpName);

      auto DC =
         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
      if (!DC)
         return nullptr;

      Decl *SwiftDecl = Impl.importDecl(decl->getUnderlyingDecl(), getActivePolarphpVersion());
      const TypeDecl *TypePHPTypeDecl = dyn_cast<TypeDecl>(SwiftDecl);

      if (!TypePHPTypeDecl)
         return nullptr;

      auto Loc = Impl.importSourceLoc(decl->getLocation());
      auto Result = Impl.createDeclWithClangNode<TypeAliasDecl>(
         decl,
         AccessLevel::Public,
         Impl.importSourceLoc(decl->getBeginLoc()),
         SourceLoc(), Name,
         Loc,
         /*genericparams*/nullptr, DC);
      Result->setUnderlyingType(TypePHPTypeDecl->getDeclaredInterfaceType());

      return Result;
   }

   // @todo
   /// Add an @objc(name) attribute with the given, optional name expressed as
   /// selector.
   ///
   /// The importer should use this rather than adding the attribute directly.
//   void addObjCAttribute(ValueDecl *decl, Optional<ObjCSelector> name) {
//      auto &ctx = Impl.TypePHPContext;
//      if (name) {
//         decl->getAttrs().add(ObjCAttr::create(ctx, name,
//            /*implicitName=*/true));
//      }
//      decl->setIsObjC(true);
//      decl->setIsDynamic(true);
//
//      // If the declaration we attached the 'objc' attribute to is within a
//      // class, record it in the class.
//      if (auto contextTy = decl->getDeclContext()->getDeclaredInterfaceType()) {
//         if (auto classDecl = contextTy->getClassOrBoundGenericClass()) {
//            if (auto method = dyn_cast<AbstractFunctionDecl>(decl)) {
//               if (name)
//                  classDecl->recordObjCMethod(method, *name);
//            }
//         }
//      }
//   }

   /// Add an @objc(name) attribute with the given, optional name expressed as
   /// selector.
   ///
   /// The importer should use this rather than adding the attribute directly.
//   void addObjCAttribute(ValueDecl *decl, Identifier name) {
//      addObjCAttribute(decl, ObjCSelector(Impl.TypePHPContext, 0, name));
//   }

//   Decl *VisitObjCMethodDecl(const clang::ObjCMethodDecl *decl) {
//      auto dc = Impl.importDeclContextOf(decl, decl->getDeclContext());
//      if (!dc)
//         return nullptr;
//
//      // While importing the DeclContext, we might have imported the decl
//      // itself.
//      if (auto Known = Impl.importDeclCached(decl, getVersion()))
//         return Known;
//
//      return importObjCMethodDecl(decl, dc, None);
//   }

   /// Check whether we have already imported a method with the given
   /// selector in the given context.
//   bool isMethodAlreadyImported(ObjCSelector selector, bool isInstance,
//                                DeclContext *dc,
//                                llvm::function_ref<bool(AbstractFunctionDecl *fn)> filter) {
//      // We only need to perform this check for classes.
//      auto classDecl
//         = dc->getDeclaredInterfaceType()->getClassOrBoundGenericClass();
//      if (!classDecl)
//         return false;
//
//      // Make sure we don't search in Clang modules for this method.
//      ++Impl.ActiveSelectors[{selector, isInstance}];
//
//      // Look for a matching imported or deserialized member.
//      bool result = false;
//      for (auto decl : classDecl->lookupDirect(selector, isInstance)) {
//         if ((decl->getClangDecl()
//              || !decl->getDeclContext()->getParentSourceFile())
//             && filter(decl)) {
//            result = true;
//            break;
//         }
//      }
//
//      // Restore the previous active count in the active-selector mapping.
//      auto activeCount = Impl.ActiveSelectors.find({selector, isInstance});
//      --activeCount->second;
//      if (activeCount->second == 0)
//         Impl.ActiveSelectors.erase(activeCount);
//
//      return result;
//   }

//   Decl *importObjCMethodDecl(const clang::ObjCMethodDecl *decl,
//                              DeclContext *dc,
//                              Optional<AccessorInfo> accessorInfo) {
//      return importObjCMethodDecl(decl, dc, false, accessorInfo);
//   }

private:
   static bool isAcceptableResult(Decl *fn,
                                  Optional<AccessorInfo> accessorInfo) {
      // We can't safely re-use the same declaration if it disagrees
      // in accessor-ness.
      auto accessor = dyn_cast<AccessorDecl>(fn);
      if (!accessorInfo)
         return accessor == nullptr;

      // For consistency with previous behavior, allow it even if it's been
      // imported for some other property.
      return (accessor && accessor->getAccessorKind() == accessorInfo->Kind);
   }

   // @todo
//   Decl *importObjCMethodDecl(const clang::ObjCMethodDecl *decl,
//                              DeclContext *dc,
//                              bool forceClassMethod,
//                              Optional<AccessorInfo> accessorInfo) {
//      // If we have an init method, import it as an initializer.
//      if (isInitMethod(decl)) {
//         // Cannot import initializers as accessors.
//         if (accessorInfo)
//            return nullptr;
//
//         // Cannot force initializers into class methods.
//         if (forceClassMethod)
//            return nullptr;
//
//         return importConstructor(decl, dc, /*implicit=*/false, None,
//            /*required=*/false);
//      }
//
//      // Check whether we already imported this method.
//      if (!forceClassMethod &&
//          dc == Impl.importDeclContextOf(decl, decl->getDeclContext())) {
//         // FIXME: Should also be able to do this for forced class
//         // methods.
//         auto known = Impl.ImportedDecls.find({decl->getCanonicalDecl(),
//                                               getVersion()});
//         if (known != Impl.ImportedDecls.end()) {
//            auto decl = known->second;
//            if (isAcceptableResult(decl, accessorInfo))
//               return decl;
//         }
//      }
//
//      // Check whether another method with the same selector has already been
//      // imported into this context.
//      ObjCSelector selector = Impl.importSelector(decl->getSelector());
//      bool isInstance = decl->isInstanceMethod() && !forceClassMethod;
//      if (isActivePolarphpVersion()) {
//         if (isMethodAlreadyImported(selector, isInstance, dc,
//                                     [&](AbstractFunctionDecl *fn) {
//                                        return isAcceptableResult(fn, accessorInfo);
//                                     })) {
//            return nullptr;
//         }
//      }
//
//      ImportedName importedName;
//      Optional<ImportedName> correctPolarphpName;
//      importedName = importFullName(decl, correctPolarphpName);
//      if (!importedName)
//         return nullptr;
//
//      // Normal case applies when we're importing an older name, or when we're
//      // not an init
//      if (!isFactoryInit(importedName)) {
//         auto result = importNonInitObjCMethodDecl(decl, dc, importedName,
//                                                   selector, forceClassMethod,
//                                                   accessorInfo);
//
//         if (!isActivePolarphpVersion() && result)
//            markAsVariant(result, *correctPolarphpName);
//
//         return result;
//      }
//
//      // We can't import a factory-initializer as an accessor.
//      if (accessorInfo)
//         return nullptr;
//
//      // We don't want to suppress init formation in Swift 3 names. Instead, we
//      // want the normal Swift 3 name, and a "raw" name for diagnostics. The
//      // "raw" name will be imported as unavailable with a more helpful and
//      // specific message.
//      ++NumFactoryMethodsAsInitializers;
//      bool redundant = false;
//      auto result =
//         importConstructor(decl, dc, false, importedName.getInitKind(),
//            /*required=*/false, selector, importedName,
//                           {decl->param_begin(), decl->param_size()},
//                           decl->isVariadic(), redundant);
//
//      if (!isActivePolarphpVersion() && result)
//         markAsVariant(result, *correctPolarphpName);
//
//      return result;
//   }

//   Decl *importNonInitObjCMethodDecl(const clang::ObjCMethodDecl *decl,
//                                     DeclContext *dc,
//                                     ImportedName importedName,
//                                     ObjCSelector selector,
//                                     bool forceClassMethod,
//                                     Optional<AccessorInfo> accessorInfo) {
//      assert(dc->isTypeContext() && "Method in non-type context?");
//      assert(isa<ClangModuleUnit>(dc->getModuleScopeContext()) &&
//             "Clang method in Swift context?");
//
//      // FIXME: We should support returning "Self.Type" for a root class
//      // instance method mirrored as a class method, but it currently causes
//      // problems for the type checker.
//      if (forceClassMethod && decl->hasRelatedResultType())
//         return nullptr;
//
//      // Hack: avoid importing methods named "print" that aren't available in
//      // the current version of Swift. We'd rather just let the user use
//      // Swift.print in that case.
//      if (!isActivePolarphpVersion() &&
//          isPrintLikeMethod(importedName.getDeclName(), dc)) {
//         return nullptr;
//      }
//
//      SpecialMethodKind kind = SpecialMethodKind::Regular;
//      if (isNSDictionaryMethod(decl, Impl.objectForKeyedSubscript))
//         kind = SpecialMethodKind::NSDictionarySubscriptGetter;
//
//      // Import the type that this method will have.
//      Optional<ForeignErrorConvention> errorConvention;
//
//      // If we have a property accessor, find the corresponding property
//      // declaration.
//      const clang::ObjCPropertyDecl *prop = nullptr;
//      if (decl->isPropertyAccessor()) {
//         prop = decl->findPropertyDecl();
//         if (!prop) return nullptr;
//
//         // If we're importing just the accessors (not the property), ignore
//         // the property.
//         if (shouldImportPropertyAsAccessors(prop))
//            prop = nullptr;
//      }
//
//      // If we have an accessor-import request but didn't find a property,
//      // reject the import request.
//      if (accessorInfo && !prop) {
//         return nullptr;
//      }
//
//      // Import the parameter list and result type.
//      ParameterList *bodyParams = nullptr;
//      ImportedType importedType;
//      if (prop) {
//         // If the matching property is in a superclass, or if the getter and
//         // setter are redeclared in a potentially incompatible way, bail out.
//         if (prop->getGetterMethodDecl() != decl &&
//             prop->getSetterMethodDecl() != decl)
//            return nullptr;
//         importedType =
//            Impl.importAccessorParamsAndReturnType(dc, prop, decl,
//                                                   isInSystemModule(dc),
//                                                   importedName, &bodyParams);
//      } else {
//         importedType = Impl.importMethodParamsAndReturnType(
//            dc, decl, decl->parameters(), decl->isVariadic(),
//            isInSystemModule(dc), &bodyParams, importedName,
//            errorConvention, kind);
//      }
//      if (!importedType)
//         return nullptr;
//
//      // Check whether we recursively imported this method
//      if (!forceClassMethod &&
//          dc == Impl.importDeclContextOf(decl, decl->getDeclContext())) {
//         // FIXME: Should also be able to do this for forced class
//         // methods.
//         auto known = Impl.ImportedDecls.find({decl->getCanonicalDecl(),
//                                               getVersion()});
//         if (known != Impl.ImportedDecls.end()) {
//            auto decl = known->second;
//            if (isAcceptableResult(decl, accessorInfo))
//               return decl;
//         }
//      }
//
//      auto result = createFuncOrAccessor(Impl.TypePHPContext,
//         /*funcLoc*/SourceLoc(),
//                                         accessorInfo,
//                                         importedName.getDeclName(),
//         /*nameLoc*/SourceLoc(),
//                                         bodyParams, Type(),
//                                         importedName.getErrorInfo().hasValue(),
//                                         dc, decl);
//
//      result->setAccess(getOverridableAccessLevel(dc));
//
//      auto resultTy = importedType.getType();
//      auto isIUO = importedType.isImplicitlyUnwrapped();
//
//      // If the method has a related result type that is representable
//      // in Swift as DynamicSelf, do so.
//      if (!prop && decl->hasRelatedResultType()) {
//         resultTy = dc->getSelfInterfaceType();
//         if (dc->getSelfClassDecl())
//            resultTy = DynamicSelfType::get(resultTy, Impl.TypePHPContext);
//         isIUO = false;
//
//         OptionalTypeKind nullability = OTK_ImplicitlyUnwrappedOptional;
//         if (auto typeNullability = decl->getReturnType()->getNullability(
//            Impl.getClangAstContext())) {
//            // If the return type has nullability, use it.
//            nullability = translateNullability(*typeNullability);
//         }
//         if (nullability != OTK_None && !errorConvention.hasValue()) {
//            resultTy = OptionalType::get(resultTy);
//            isIUO = nullability == OTK_ImplicitlyUnwrappedOptional;
//         }
//      }
//
//      // Record the return type.
//      result->getBodyResultTypeLoc().setType(resultTy);
//
//      // Optional methods in protocols.
//      if (decl->getImplementationControl() == clang::ObjCMethodDecl::Optional &&
//          isa<InterfaceDecl>(dc))
//         result->getAttrs().add(new (Impl.TypePHPContext)
//                                   OptionalAttr(/*implicit*/false));
//
//      // Mark class methods as static.
//      if (decl->isClassMethod() || forceClassMethod)
//         result->setStatic();
//      if (forceClassMethod)
//         result->setImplicit();
//
//      Impl.recordImplicitUnwrapForDecl(result, isIUO);
//
//      // Mark this method @objc.
//      addObjCAttribute(result, selector);
//
//      // If this method overrides another method, mark it as such.
//      recordObjCOverride(result);
//
//      // Record the error convention.
//      if (errorConvention) {
//         result->setForeignErrorConvention(*errorConvention);
//      }
//
//      // Handle attributes.
//      if (decl->hasAttr<clang::IBActionAttr>() &&
//          isa<FuncDecl>(result) &&
//          cast<FuncDecl>(result)->isPotentialIBActionTarget()) {
//         result->getAttrs().add(
//            new (Impl.TypePHPContext) IBActionAttr(/*IsImplicit=*/false));
//      }
//
//      // FIXME: Is there an IBSegueAction equivalent?
//
//      // Check whether there's some special method to import.
//      if (!forceClassMethod) {
//         if (dc == Impl.importDeclContextOf(decl, decl->getDeclContext()) &&
//             !Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}])
//            Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}]
//               = result;
//
//         if (importedName.isSubscriptAccessor()) {
//            // If this was a subscript accessor, try to create a
//            // corresponding subscript declaration.
//            (void)importSubscript(result, decl);
//         } else if (shouldAlsoImportAsClassMethod(result)) {
//            // If we should import this instance method also as a class
//            // method, do so and mark the result as an alternate
//            // declaration.
//            if (auto imported = importObjCMethodDecl(decl, dc,
//               /*forceClassMethod=*/true,
//               /*accessor*/None))
//               Impl.addAlternateDecl(result, cast<ValueDecl>(imported));
//         }
//      }
//
//      return result;
//   }

public:
//   /// Record the function or initializer overridden by the given Swift method.
//   void recordObjCOverride(AbstractFunctionDecl *decl);

   /// Given an imported method, try to import it as a constructor.
   ///
   /// Objective-C methods in the 'init' family are imported as
   /// constructors in Swift, enabling object construction syntax, e.g.,
   ///
   /// \code
   /// // in objc: [[NSArray alloc] initWithCapacity:1024]
   /// NSArray(capacity: 1024)
   /// \endcode
   ConstructorDecl *importConstructor(const clang::ObjCMethodDecl *objcMethod,
                                      DeclContext *dc,
                                      bool implicit,
                                      Optional<CtorInitializerKind> kind,
                                      bool required);

   /// Returns the latest "introduced" version on the current platform for
   /// \p D.
   llvm::VersionTuple findLatestIntroduction(const clang::Decl *D);

   /// Returns true if importing \p objcMethod will produce a "better"
   /// initializer than \p existingCtor.
   bool
   existingConstructorIsWorse(const ConstructorDecl *existingCtor,
                              const clang::ObjCMethodDecl *objcMethod,
                              CtorInitializerKind kind);

   /// Given an imported method, try to import it as a constructor.
   ///
   /// Objective-C methods in the 'init' family are imported as
   /// constructors in Swift, enabling object construction syntax, e.g.,
   ///
   /// \code
   /// // in objc: [[NSArray alloc] initWithCapacity:1024]
   /// NSArray(capacity: 1024)
   /// \endcode
   ///
   /// This variant of the function is responsible for actually binding the
   /// constructor declaration appropriately.
//   ConstructorDecl *importConstructor(const clang::ObjCMethodDecl *objcMethod,
//                                      DeclContext *dc,
//                                      bool implicit,
//                                      CtorInitializerKind kind,
//                                      bool required,
//                                      ObjCSelector selector,
//                                      ImportedName importedName,
//                                      ArrayRef<const clang::ParmVarDecl*> args,
//                                      bool variadic,
//                                      bool &redundant);

//   void recordObjCOverride(SubscriptDecl *subscript);

   /// Given either the getter or setter for a subscript operation,
   /// create the Swift subscript declaration.
//   SubscriptDecl *importSubscript(Decl *decl,
//                                  const clang::ObjCMethodDecl *objcMethod);

   /// Import the accessor and its attributes.
//   AccessorDecl *importAccessor(clang::ObjCMethodDecl *clangAccessor,
//                                AbstractStorageDecl *storage,
//                                AccessorKind accessorKind,
//                                DeclContext *dc);

public:

   /// Recursively add the given protocol and its inherited protocols to the
   /// given vector, guarded by the known set of protocols.
   void addInterfaces(InterfaceDecl *protocol,
                      SmallVectorImpl<InterfaceDecl *> &protocols,
                      llvm::SmallPtrSetImpl<InterfaceDecl *> &known);

   // Import the given Objective-C protocol list, along with any
   // implicitly-provided protocols, and attach them to the given
   // declaration.
//   void importObjCInterfaces(Decl *decl,
//                             const clang::ObjCInterfaceList &clangInterfaces,
//                             SmallVectorImpl<TypeLoc> &inheritedTypes);

   /// Add conformances to the given Objective-C protocols to the
   /// given declaration.
//   void addObjCInterfaceConformances(Decl *decl,
//                                     ArrayRef<InterfaceDecl*> protocols);

   // Returns None on error. Returns nullptr if there is no type param list to
   // import or we suppress its import, as in the case of NSArray, NSSet, and
   // NSDictionary.
//   Optional<GenericParamList *>
//   importObjCGenericParams(const clang::ObjCInterfaceDecl *decl,
//                           DeclContext *dc);

   /// Import the members of all of the protocols to which the given
   /// Objective-C class, category, or extension explicitly conforms into
   /// the given list of members, so long as the method was not already
   /// declared in the class.
   ///
   /// FIXME: This whole thing is a hack, because name lookup should really
   /// just find these members when it looks in the protocol. Unfortunately,
   /// that's not something the name lookup code can handle right now, and
   /// it may still be necessary when the protocol's instance methods become
   /// class methods on a root class (e.g. NSObject-the-protocol's instance
   /// methods become class methods on NSObject).
//   void importMirroredInterfaceMembers(const clang::ObjCContainerDecl *decl,
//                                       DeclContext *dc,
//                                       ArrayRef<InterfaceDecl *> protocols,
//                                       SmallVectorImpl<Decl *> &members,
//                                       AstContext &Ctx);

   void importNonOverriddenMirroredMethods(DeclContext *dc,
                                           MutableArrayRef<MirroredMethodEntry> entries,
                                           SmallVectorImpl<Decl *> &newMembers);

   /// TODO
   /// Import constructors from our superclasses (and their
   /// categories/extensions), effectively "inheriting" constructors.
//   void importInheritedConstructors(ClassDecl *classDecl,
//                                    SmallVectorImpl<Decl *> &newMembers);

//   Decl *VisitObjCCategoryDecl(const clang::ObjCCategoryDecl *decl) {
//      // If the declaration is invalid, fail.
//      if (decl->isInvalidDecl()) return nullptr;
//
//      // Objective-C categories and extensions map to Swift extensions.
//      if (importer::hasNativeSwiftDecl(decl))
//         return nullptr;
//
//      // Find the Swift class being extended.
//      auto objcClass = castIgnoringCompatibilityAlias<ClassDecl>(
//         Impl.importDecl(decl->getClassInterface(), getActivePolarphpVersion()));
//      if (!objcClass)
//         return nullptr;
//
//      auto dc = Impl.importDeclContextOf(decl, decl->getDeclContext());
//      if (!dc)
//         return nullptr;
//
//      auto loc = Impl.importSourceLoc(decl->getBeginLoc());
//      auto result = ExtensionDecl::create(
//         Impl.TypePHPContext, loc,
//         nullptr,
//         { }, dc, nullptr, decl);
//      Impl.TypePHPContext.evaluator.cacheOutput(ExtendedTypeRequest{result},
//                                                 objcClass->getDeclaredType());
//      Impl.TypePHPContext.evaluator.cacheOutput(ExtendedNominalRequest{result},
//                                                 std::move(objcClass));
//
//      // Determine the type and generic args of the extension.
//      if (objcClass->getGenericParams()) {
//         result->setGenericSignature(objcClass->getGenericSignature());
//      }
//
//      // Create the extension declaration and record it.
//      objcClass->addExtension(result);
//      Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = result;
//      SmallVector<TypeLoc, 4> inheritedTypes;
//      importObjCInterfaces(result, decl->getReferencedInterfaces(),
//                           inheritedTypes);
//      result->setInherited(Impl.TypePHPContext.AllocateCopy(inheritedTypes));
//      result->setMemberLoader(&Impl, 0);
//
//      return result;
//   }

//   template <typename T, typename U>
//   T *resolveSwiftDeclImpl(const U *decl, Identifier name,
//                           bool hasKnownSwiftName, ModuleDecl *overlay) {
//      const auto &languageVersion =
//         Impl.TypePHPContext.LangOpts.EffectiveLanguageVersion;
//
//      auto isMatch = [&](const T *singleResult, bool baseNameMatches,
//                         bool allowObjCMismatch) -> bool {
//         const DeclAttributes &attrs = singleResult->getAttrs();
//
//         // Skip versioned variants.
//         if (attrs.isUnavailableInSwiftVersion(languageVersion))
//            return false;
//
//         // If Clang decl has a custom Swift name, then we know that the name we
//         // did direct lookup for is correct.
//         // 'allowObjCMismatch' shouldn't exist, but we need it for source
//         // compatibility where a previous version of the compiler didn't check
//         // @objc-ness at all.
//         if (hasKnownSwiftName || allowObjCMismatch) {
//            assert(baseNameMatches);
//            return allowObjCMismatch || singleResult->isObjC();
//         }
//
//         // Skip if a different name is used for Objective-C.
//         if (auto objcAttr = attrs.getAttribute<ObjCAttr>())
//            if (auto objcName = objcAttr->getName())
//               return objcName->getSimpleName() == name;
//
//         return baseNameMatches && singleResult->isObjC();
//      };
//
//      // First look at Swift types with the same name.
//      SmallVector<ValueDecl *, 4> polarphpDeclsByName;
//      overlay->lookupValue(name, NLKind::QualifiedLookup, polarphpDeclsByName);
//      T *found = nullptr;
//      for (auto result : polarphpDeclsByName) {
//         if (auto singleResult = dyn_cast<T>(result)) {
//            if (isMatch(singleResult, /*baseNameMatches=*/true,
//               /*allowObjCMismatch=*/false)) {
//               if (found)
//                  return nullptr;
//               found = singleResult;
//            }
//         }
//      }
//
//      if (!found && hasKnownSwiftName)
//         return nullptr;
//
//      if (!found) {
//         // Try harder to find a match looking at just custom Objective-C names.
//         // Limit what we deserialize to decls with an @objc attribute.
//         SmallVector<Decl *, 4> matchingTopLevelDecls;
//
//         // Get decls with a matching @objc attribute
//         overlay->getTopLevelDeclsWhereAttributesMatch(
//            matchingTopLevelDecls,
//            [&name](const DeclAttributes attrs) -> bool {
//               if (auto objcAttr = attrs.getAttribute<ObjCAttr>())
//                  if (auto objcName = objcAttr->getName())
//                     return objcName->getSimpleName() == name;
//               return false;
//            });
//
//         // Filter by decl kind
//         for (auto result : matchingTopLevelDecls) {
//            if (auto singleResult = dyn_cast<T>(result)) {
//               if (found)
//                  return nullptr;
//               found = singleResult;
//            }
//         }
//      }
//
//      if (!found) {
//         // Go back to the first list and find classes with matching Swift names
//         // *even if the ObjC name doesn't match.*
//         // This shouldn't be allowed but we need it for source compatibility;
//         // people used `@class SwiftNameOfClass` as a workaround for not
//         // having the previous loop, and it "worked".
//         for (auto result : polarphpDeclsByName) {
//            if (auto singleResult = dyn_cast<T>(result)) {
//               if (isMatch(singleResult, /*baseNameMatches=*/true,
//                  /*allowObjCMismatch=*/true)) {
//                  if (found)
//                     return nullptr;
//                  found = singleResult;
//               }
//            }
//         }
//      }
//
//      if (found)
//         Impl.ImportedDecls[{decl->getCanonicalDecl(),
//                             getActivePolarphpVersion()}] = found;
//
//      return found;
//   }
//
//   template <typename T, typename U>
//   T *resolveSwiftDecl(const U *decl, Identifier name,
//                       bool hasKnownSwiftName, ClangModuleUnit *clangModule) {
//      if (auto overlay = clangModule->getOverlayModule())
//         return resolveSwiftDeclImpl<T>(decl, name, hasKnownSwiftName, overlay);
//      if (clangModule == Impl.ImportedHeaderUnit) {
//         // Use an index-based loop because new owners can come in as we're
//         // iterating.
//         for (size_t i = 0; i < Impl.ImportedHeaderOwners.size(); ++i) {
//            ModuleDecl *owner = Impl.ImportedHeaderOwners[i];
//            if (T *result = resolveSwiftDeclImpl<T>(decl, name,
//                                                    hasKnownSwiftName, owner))
//               return result;
//         }
//      }
//      return nullptr;
//   }
//
//   template <typename T, typename U>
//   bool hasNativeSwiftDecl(const U *decl, Identifier name,
//                           const DeclContext *dc, T *&polarphpDecl) {
//      if (!importer::hasNativeSwiftDecl(decl))
//         return false;
//      auto wrapperUnit = cast<ClangModuleUnit>(dc->getModuleScopeContext());
//      polarphpDecl = resolveSwiftDecl<T>(decl, name, /*hasCustomSwiftName=*/true,
//                                      wrapperUnit);
//      return true;
//   }
//
//   void markMissingSwiftDecl(ValueDecl *VD) {
//      const char *message;
//      if (isa<ClassDecl>(VD))
//         message = "cannot find Swift declaration for this class";
//      else if (isa<InterfaceDecl>(VD))
//         message = "cannot find Swift declaration for this protocol";
//      else
//         llvm_unreachable("unknown bridged decl kind");
//      auto attr = AvailableAttr::createPlatformAgnostic(Impl.TypePHPContext,
//                                                        message);
//      VD->getAttrs().add(attr);
//   }
//
//   Decl *VisitObjCInterfaceDecl(const clang::ObjCInterfaceDecl *decl) {
//      Optional<ImportedName> correctPolarphpName;
//      auto importedName = importFullName(decl, correctPolarphpName);
//      if (!importedName) return nullptr;
//
//      // If we've been asked to produce a compatibility stub, handle it via a
//      // typealias.
//      if (correctPolarphpName)
//         return importCompatibilityTypeAlias(decl, importedName,
//                                             *correctPolarphpName);
//
//      Identifier name = importedName.getDeclName().getBaseIdentifier();
//      bool hasKnownSwiftName = importedName.hasCustomName();
//
//      // FIXME: Figure out how to deal with incomplete protocols, since that
//      // notion doesn't exist in Swift.
//      if (!decl->hasDefinition()) {
//         // Check if this protocol is implemented in its overlay.
//         if (auto clangModule = Impl.getClangModuleForDecl(decl, true))
//            if (auto native = resolveSwiftDecl<InterfaceDecl>(decl, name,
//                                                              hasKnownSwiftName,
//                                                              clangModule))
//               return native;
//
//         forwardDeclaration = true;
//         return nullptr;
//      }
//
//      decl = decl->getDefinition();
//
//      auto dc =
//         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
//      if (!dc)
//         return nullptr;
//
//      InterfaceDecl *nativeDecl;
//      bool declaredNative = hasNativeSwiftDecl(decl, name, dc, nativeDecl);
//      if (declaredNative && nativeDecl)
//         return nativeDecl;
//
//      // Create the protocol declaration and record it.
//      auto result = Impl.createDeclWithClangNode<InterfaceDecl>(
//         decl, AccessLevel::Public, dc,
//         Impl.importSourceLoc(decl->getBeginLoc()),
//         Impl.importSourceLoc(decl->getLocation()), name, None,
//         /*TrailingWhere=*/nullptr);
//
//      addObjCAttribute(result, Impl.importIdentifier(decl->getIdentifier()));
//
//      if (declaredNative)
//         markMissingSwiftDecl(result);
//
//      Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = result;
//
//      // Import protocols this protocol conforms to.
//      SmallVector<TypeLoc, 4> inheritedTypes;
//      importObjCInterfaces(result, decl->getReferencedInterfaces(),
//                           inheritedTypes);
//      result->setInherited(Impl.TypePHPContext.AllocateCopy(inheritedTypes));
//
//      result->setMemberLoader(&Impl, 0);
//
//      return result;
//   }

   // Add inferred attributes.
   void addInferredAttributes(Decl *decl, unsigned attributes) {
      using namespace inferredattributes;
      if (attributes & requires_stored_property_inits) {
         auto a = new (Impl.TypePHPContext)
            RequiresStoredPropertyInitsAttr(/*IsImplicit=*/true);
         decl->getAttrs().add(a);
      }
   }

//   Decl *VisitObjCInterfaceDecl(const clang::ObjCInterfaceDecl *decl) {
//      auto createFakeRootClass = [=](Identifier name,
//                                     DeclContext *dc = nullptr) -> ClassDecl * {
//         if (!dc) {
//            dc = Impl.getClangModuleForDecl(decl->getCanonicalDecl(),
//               /*allowForwardDeclaration=*/true);
//         }
//
//         auto result = Impl.createDeclWithClangNode<ClassDecl>(decl,
//                                                               AccessLevel::Public,
//                                                               SourceLoc(), name,
//                                                               SourceLoc(), None,
//                                                               nullptr, dc);
//         Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = result;
//         result->setSuperclass(Type());
//         result->setAddedImplicitInitializers(); // suppress all initializers
//         result->setHasMissingVTableEntries(false);
//         addObjCAttribute(result, Impl.importIdentifier(decl->getIdentifier()));
//         return result;
//      };
//
//      // Special case for Interface, which gets forward-declared as an ObjC
//      // class which is hidden in modern Objective-C runtimes.
//      // We treat it as a foreign class (like a CF type) because it doesn't
//      // have a real public class object.
//      clang::ASTContext &clangCtx = Impl.getClangAstContext();
//      if (decl->getCanonicalDecl() ==
//          clangCtx.getObjCInterfaceDecl()->getCanonicalDecl()) {
//         Type nsObjectTy = Impl.getNSObjectType();
//         if (!nsObjectTy)
//            return nullptr;
//         const ClassDecl *nsObjectDecl =
//            nsObjectTy->getClassOrBoundGenericClass();
//
//         auto result = createFakeRootClass(Impl.TypePHPContext.Id_Interface,
//                                           nsObjectDecl->getDeclContext());
//         result->setForeignClassKind(ClassDecl::ForeignKind::RuntimeOnly);
//         return result;
//      }
//
//      if (auto *definition = decl->getDefinition())
//         decl = definition;
//
//      Optional<ImportedName> correctPolarphpName;
//      auto importedName = importFullName(decl, correctPolarphpName);
//      if (!importedName) return nullptr;
//
//      // If we've been asked to produce a compatibility stub, handle it via a
//      // typealias.
//      if (correctPolarphpName)
//         return importCompatibilityTypeAlias(decl, importedName,
//                                             *correctPolarphpName);
//
//      auto name = importedName.getDeclName().getBaseIdentifier();
//      bool hasKnownSwiftName = importedName.hasCustomName();
//
//      if (!decl->hasDefinition()) {
//         // Check if this class is implemented in its overlay.
//         if (auto clangModule = Impl.getClangModuleForDecl(decl, true)) {
//            if (auto native = resolveSwiftDecl<ClassDecl>(decl, name,
//                                                          hasKnownSwiftName,
//                                                          clangModule)) {
//               return native;
//            }
//         }
//
//         if (Impl.ImportForwardDeclarations) {
//            // Fake it by making an unavailable opaque @objc root class.
//            auto result = createFakeRootClass(name);
//            result->setImplicit();
//            auto attr = AvailableAttr::createPlatformAgnostic(Impl.TypePHPContext,
//                                                              "This Objective-C class has only been forward-declared; "
//                                                              "import its owning module to use it");
//            result->getAttrs().add(attr);
//            result->getAttrs().add(
//               new (Impl.TypePHPContext) ForbidSerializingReferenceAttr(true));
//            return result;
//         }
//
//         forwardDeclaration = true;
//         return nullptr;
//      }
//
//      auto dc =
//         Impl.importDeclContextOf(decl, importedName.getEffectiveContext());
//      if (!dc)
//         return nullptr;
//
//      ClassDecl *nativeDecl;
//      bool declaredNative = hasNativeSwiftDecl(decl, name, dc, nativeDecl);
//      if (declaredNative && nativeDecl)
//         return nativeDecl;
//
//      auto access = AccessLevel::Open;
//      if (decl->hasAttr<clang::ObjCSubclassingRestrictedAttr>() &&
//          Impl.TypePHPContext.isSwiftVersionAtLeast(5)) {
//         access = AccessLevel::Public;
//      }
//
//      // Create the class declaration and record it.
//      auto result = Impl.createDeclWithClangNode<ClassDecl>(
//         decl, access, Impl.importSourceLoc(decl->getBeginLoc()), name,
//         Impl.importSourceLoc(decl->getLocation()), None, nullptr, dc);
//
//      // Import generic arguments, if any.
//      if (auto gpImportResult = importObjCGenericParams(decl, dc)) {
//         auto genericParams = *gpImportResult;
//         if (genericParams) {
//            result->getAstContext().evaluator.cacheOutput(
//               GenericParamListRequest{result}, std::move(genericParams));
//
//            auto sig = Impl.buildGenericSignature(genericParams, dc);
//            result->setGenericSignature(sig);
//         }
//      } else {
//         return nullptr;
//      }
//
//      Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = result;
//      addObjCAttribute(result, Impl.importIdentifier(decl->getIdentifier()));
//
//      if (declaredNative)
//         markMissingSwiftDecl(result);
//      if (decl->getAttr<clang::ObjCRuntimeVisibleAttr>()) {
//         result->setForeignClassKind(ClassDecl::ForeignKind::RuntimeOnly);
//      }
//
//      // If this Objective-C class has a supertype, import it.
//      SmallVector<TypeLoc, 4> inheritedTypes;
//      Type superclassType;
//      if (decl->getSuperClass()) {
//         clang::QualType clangSuperclassType =
//            decl->getSuperClassType()->stripObjCKindOfTypeAndQuals(clangCtx);
//         clangSuperclassType =
//            clangCtx.getObjCObjectPointerType(clangSuperclassType);
//         superclassType = Impl.importTypeIgnoreIUO(
//            clangSuperclassType, ImportTypeKind::Abstract, isInSystemModule(dc),
//            Bridgeability::None);
//         if (superclassType) {
//            assert(superclassType->is<ClassType>() ||
//                   superclassType->is<BoundGenericClassType>());
//            inheritedTypes.push_back(TypeLoc::withoutLoc(superclassType));
//         }
//      }
//      result->setSuperclass(superclassType);
//
//      // Mark the class as runtime-only if it is named 'OS_object', even
//      // if it doesn't have the runtime-only Clang attribute. This is a
//      // targeted fix allowing IRGen to emit convenience initializers
//      // correctly.
//      //
//      // FIXME: Remove this once SILGen gets proper support for factory
//      // initializers.
//      if (decl->getName() == "OS_object" ||
//          decl->getName() == "OS_os_log") {
//         result->setForeignClassKind(ClassDecl::ForeignKind::RuntimeOnly);
//      }
//
//      // If the superclass is runtime-only, our class is also. This only
//      // matters in the case above.
//      if (superclassType) {
//         auto superclassDecl = cast<ClassDecl>(superclassType->getAnyNominal());
//         auto kind = superclassDecl->getForeignClassKind();
//         if (kind != ClassDecl::ForeignKind::Normal)
//            result->setForeignClassKind(kind);
//      }
//
//      // Import protocols this class conforms to.
//      importObjCInterfaces(result, decl->getReferencedInterfaces(),
//                           inheritedTypes);
//      result->setInherited(Impl.TypePHPContext.AllocateCopy(inheritedTypes));
//
//      // Add inferred attributes.
//#define INFERRED_ATTRIBUTES(ModuleName, ClassName, AttributeSet)               \
//  if (name.str().equals(#ClassName) &&                                         \
//      result->getParentModule()->getName().str().equals(#ModuleName)) {        \
//    using namespace inferredattributes;                                       \
//    addInferredAttributes(result, AttributeSet);                               \
//  }
//#include "polarphp/clangimporter/internal/InferredAttributesDef.h"
//
//      if (decl->isArcWeakrefUnavailable())
//         result->setIsIncompatibleWithWeakReferences();
//
//      result->setHasMissingVTableEntries(false);
//      result->setMemberLoader(&Impl, 0);
//
//      return result;
//   }
//
//   Decl *VisitObjCImplDecl(const clang::ObjCImplDecl *decl) {
//      // Implementations of Objective-C classes and categories are not
//      // reflected into Swift.
//      return nullptr;
//   }
//
//   Decl *VisitObjCPropertyDecl(const clang::ObjCPropertyDecl *decl) {
//      auto dc = Impl.importDeclContextOf(decl, decl->getDeclContext());
//      if (!dc)
//         return nullptr;
//
//      // While importing the DeclContext, we might have imported the decl
//      // itself.
//      if (auto Known = Impl.importDeclCached(decl, getVersion()))
//         return Known;
//
//      return importObjCPropertyDecl(decl, dc);
//   }
//
//   /// Hack: Handle the case where a property is declared \c readonly in the
//   /// main class interface (either explicitly or because of an adopted
//   /// protocol) and then \c readwrite in a category/extension.
//   ///
//   /// \see VisitObjCPropertyDecl
//   void handlePropertyRedeclaration(VarDecl *original,
//                                    const clang::ObjCPropertyDecl *redecl) {
//      // If the property isn't from Clang, we can't safely update it.
//      if (!original->hasClangNode())
//         return;
//
//      // If the original declaration was implicit, we may want to change that.
//      if (original->isImplicit() && !redecl->isImplicit() &&
//          !isa<clang::ObjCInterfaceDecl>(redecl->getDeclContext()))
//         original->setImplicit(false);
//
//      if (!original->getAttrs().hasAttribute<ReferenceOwnershipAttr>() &&
//          !original->getAttrs().hasAttribute<NSCopyingAttr>()) {
//         applyPropertyOwnership(original,
//                                redecl->getPropertyAttributesAsWritten());
//      }
//
//      auto clangSetter = redecl->getSetterMethodDecl();
//      if (!clangSetter)
//         return;
//
//      // The only other transformation we know how to do safely is add a
//      // setter. If the property is already settable, we're done.
//      if (original->isSettable(nullptr))
//         return;
//
//      AccessorDecl *setter = importAccessor(clangSetter,
//                                            original, AccessorKind::Set,
//                                            original->getDeclContext());
//      if (!setter)
//         return;
//
//      // Check that the redeclared property's setter uses the same type as the
//      // original property. Objective-C can get away with the types being
//      // different (usually in something like nullability), but for Swift it's
//      // an AST invariant that's assumed and asserted elsewhere. If the type is
//      // different, just drop the setter, and leave the property as get-only.
//      assert(setter->getParameters()->size() == 1);
//      const ParamDecl *param = setter->getParameters()->get(0);
//      if (!param->getInterfaceType()->isEqual(original->getInterfaceType()))
//         return;
//
//      original->setComputedSetter(setter);
//   }
//
//   Decl *importObjCPropertyDecl(const clang::ObjCPropertyDecl *decl,
//                                DeclContext *dc) {
//      assert(dc);
//
//      Optional<ImportedName> correctPolarphpName;
//      auto name = importFullName(decl, correctPolarphpName)
//         .getDeclName()
//         .getBaseIdentifier();
//      if (name.empty())
//         return nullptr;
//
//      if (shouldImportPropertyAsAccessors(decl))
//         return nullptr;
//
//      VarDecl *overridden = nullptr;
//      if (dc->getSelfClassDecl()) {
//         // Check whether there is a function with the same name as this
//         // property. If so, suppress the property; the user will have to use
//         // the methods directly, to avoid ambiguities.
//         NominalTypeDecl *lookupContext = dc->getSelfNominalTypeDecl();
//
//         if (auto *classDecl = dyn_cast<ClassDecl>(dc)) {
//            // If we're importing into the primary @interface for something, as
//            // opposed to an extension, make sure we don't try to load any
//            // categories...by just looking into the super type.
//            lookupContext = classDecl->getSuperclassDecl();
//         }
//
//         if (lookupContext) {
//            SmallVector<ValueDecl *, 2> lookup;
//            dc->lookupQualified(lookupContext, name,
//                                NL_QualifiedDefault | NL_KnownNoDependency,
//                                lookup);
//            bool foundMethod = false;
//            for (auto result : lookup) {
//               if (isa<FuncDecl>(result) &&
//                   result->isInstanceMember() == decl->isInstanceProperty() &&
//                   result->getFullName().getArgumentNames().empty())
//                  foundMethod = true;
//               // @todo
////               if (auto var = dyn_cast<VarDecl>(result)) {
////                  // If the selectors of the getter match in Objective-C, we have an
////                  // override.
////                  if (var->isInstanceMember() == decl->isInstanceProperty() &&
////                      var->getObjCGetterSelector() ==
////                      Impl.importSelector(decl->getGetterName()))
////                     overridden = var;
////               }
//            }
//            if (foundMethod && !overridden)
//               return nullptr;
//         }
//
//         if (overridden) {
//            const DeclContext *overrideContext = overridden->getDeclContext();
//            // It's okay to compare interface types directly because Objective-C
//            // does not have constrained extensions.
//            if (overrideContext != dc && overridden->hasClangNode() &&
//                overrideContext->getSelfNominalTypeDecl()
//                == dc->getSelfNominalTypeDecl()) {
//               // We've encountered a redeclaration of the property.
//               // HACK: Just update the original declaration instead of importing a
//               // second property.
//               handlePropertyRedeclaration(overridden, decl);
//               return nullptr;
//            }
//         }
//      }
//
//      auto importedType = Impl.importPropertyType(decl, isInSystemModule(dc));
//      if (!importedType)
//         return nullptr;
//
//      // Check whether the property already got imported.
//      if (dc == Impl.importDeclContextOf(decl, decl->getDeclContext())) {
//         auto known = Impl.ImportedDecls.find({decl->getCanonicalDecl(),
//                                               getVersion()});
//         if (known != Impl.ImportedDecls.end())
//            return known->second;
//      }
//
//      auto type = importedType.getType();
//      auto result = Impl.createDeclWithClangNode<VarDecl>(decl,
//                                                          getOverridableAccessLevel(dc),
//         /*IsStatic*/decl->isClassProperty(), VarDecl::Introducer::Var,
//         /*IsCaptureList*/false, Impl.importSourceLoc(decl->getLocation()),
//                                                          name, dc);
//      result->setInterfaceType(type);
//      Impl.recordImplicitUnwrapForDecl(result,
//                                       importedType.isImplicitlyUnwrapped());
//
//      // Recover from a missing getter in no-asserts builds. We're still not
//      // sure under what circumstances this occurs, but we shouldn't crash.
//      auto clangGetter = decl->getGetterMethodDecl();
//      assert(clangGetter && "ObjC property without getter");
//      if (!clangGetter)
//         return nullptr;
//
//      // Import the getter.
//      AccessorDecl *getter = importAccessor(clangGetter, result,
//                                            AccessorKind::Get, dc);
//      if (!getter)
//         return nullptr;
//
//      // Import the setter, if there is one.
//      AccessorDecl *setter = nullptr;
//      if (auto clangSetter = decl->getSetterMethodDecl()) {
//         setter = importAccessor(clangSetter, result, AccessorKind::Set, dc);
//         if (!setter)
//            return nullptr;
//      }
//
//      // Turn this into a computed property.
//      // FIXME: Fake locations for '{' and '}'?
//      result->setIsSetterMutating(false);
//      makeComputed(result, getter, setter);
//      addObjCAttribute(result, Impl.importIdentifier(decl->getIdentifier()));
//      applyPropertyOwnership(result, decl->getPropertyAttributesAsWritten());
//
//      // Handle attributes.
//      if (decl->hasAttr<clang::IBOutletAttr>())
//         result->getAttrs().add(
//            new (Impl.TypePHPContext) IBOutletAttr(/*IsImplicit=*/false));
//      if (decl->getPropertyImplementation() == clang::ObjCPropertyDecl::Optional
//          && isa<InterfaceDecl>(dc) &&
//          !result->getAttrs().hasAttribute<OptionalAttr>())
//         result->getAttrs().add(new (Impl.TypePHPContext)
//                                   OptionalAttr(/*implicit*/false));
//      // FIXME: Handle IBOutletCollection.
//
//      // Only record overrides of class members.
//      if (overridden) {
//         result->setOverriddenDecl(overridden);
//         getter->setOverriddenDecl(overridden->getParsedAccessor(AccessorKind::Get));
//         if (auto parentSetter = overridden->getParsedAccessor(AccessorKind::Set))
//            if (setter)
//               setter->setOverriddenDecl(parentSetter);
//      }
//
//      // If this is a compatibility stub, mark it as such.
//      if (correctPolarphpName)
//         markAsVariant(result, *correctPolarphpName);
//
//      return result;
//   }
//
//   Decl *
//   VisitObjCCompatibleAliasDecl(const clang::ObjCCompatibleAliasDecl *decl) {
//      // Import Objective-C's @compatibility_alias as typealias.
//      EffectiveClangContext effectiveContext(decl->getDeclContext()->getRedeclContext());
//      auto dc = Impl.importDeclContextOf(decl, effectiveContext);
//      if (!dc) return nullptr;
//
//      Optional<ImportedName> correctPolarphpName;
//      auto importedName = importFullName(decl, correctPolarphpName);
//      auto name = importedName.getDeclName().getBaseIdentifier();
//
//      if (name.empty()) return nullptr;
//
//      auto importedDecl =
//         Impl.importDecl(decl->getClassInterface(), getActivePolarphpVersion());
//      auto typeDecl = dyn_cast_or_null<TypeDecl>(importedDecl);
//      if (!typeDecl) return nullptr;
//
//      // Create typealias.
//      TypeAliasDecl *typealias = nullptr;
//      typealias = Impl.createDeclWithClangNode<TypeAliasDecl>(
//         decl, AccessLevel::Public,
//         Impl.importSourceLoc(decl->getBeginLoc()),
//         SourceLoc(), name,
//         Impl.importSourceLoc(decl->getLocation()),
//         /*genericparams=*/nullptr, dc);
//
//      if (auto *GTD = dyn_cast<GenericTypeDecl>(typeDecl)) {
//         typealias->setGenericSignature(GTD->getGenericSignature());
//         if (GTD->isGeneric()) {
//            typealias->getAstContext().evaluator.cacheOutput(
//               GenericParamListRequest{typealias},
//               std::move(GTD->getGenericParams()->clone(typealias)));
//         }
//      }
//
//      typealias->setUnderlyingType(typeDecl->getDeclaredInterfaceType());
//      return typealias;
//   }

   Decl *VisitLinkageSpecDecl(const clang::LinkageSpecDecl *decl) {
      // Linkage specifications are not imported.
      return nullptr;
   }

//   Decl *VisitObjCPropertyImplDecl(const clang::ObjCPropertyImplDecl *decl) {
//      // @synthesize and @dynamic are not imported, since they are not part
//      // of the interface to a class.
//      return nullptr;
//   }

   Decl *VisitFileScopeAsmDecl(const clang::FileScopeAsmDecl *decl) {
      return nullptr;
   }

   Decl *VisitAccessSpecDecl(const clang::AccessSpecDecl *decl) {
      return nullptr;
   }

   Decl *VisitFriendDecl(const clang::FriendDecl *decl) {
      // Friends are not imported; Swift has a different access control
      // mechanism.
      return nullptr;
   }

   Decl *VisitFriendTemplateDecl(const clang::FriendTemplateDecl *decl) {
      // Friends are not imported; Swift has a different access control
      // mechanism.
      return nullptr;
   }

   Decl *VisitStaticAssertDecl(const clang::StaticAssertDecl *decl) {
      // Static assertions are an implementation detail.
      return nullptr;
   }

   Decl *VisitBlockDecl(const clang::BlockDecl *decl) {
      // Blocks are not imported (although block types can be imported).
      return nullptr;
   }

   Decl *VisitClassScopeFunctionSpecializationDecl(
      const clang::ClassScopeFunctionSpecializationDecl *decl) {
      // Note: templates are not imported.
      return nullptr;
   }

   Decl *VisitImportDecl(const clang::ImportDecl *decl) {
      // Transitive module imports are not handled at the declaration level.
      // Rather, they are understood from the module itself.
      return nullptr;
   }
};
} // end anonymous namespace

/// Try to strip "Mutable" out of a type name.
static clang::IdentifierInfo *
getImmutableCFSuperclassName(const clang::TypedefNameDecl *decl, clang::ASTContext &ctx) {
   StringRef name = decl->getName();

   // Split at the first occurrence of "Mutable".
   StringRef _mutable = "Mutable";
   auto mutableIndex = camel_case::findWord(name, _mutable);
   if (mutableIndex == StringRef::npos)
      return nullptr;

   StringRef namePrefix = name.substr(0, mutableIndex);
   StringRef nameSuffix = name.substr(mutableIndex + _mutable.size());

   // Abort if "Mutable" appears twice.
   if (camel_case::findWord(nameSuffix, _mutable) != StringRef::npos)
      return nullptr;

   llvm::SmallString<128> buffer;
   buffer += namePrefix;
   buffer += nameSuffix;
   return &ctx.Idents.get(buffer.str());
}

/// Check whether this CF typedef is a Mutable type, and if so,
/// look for a non-Mutable typedef.
///
/// If the "subclass" is:
///   typedef struct __foo *XXXMutableYYY;
/// then we look for a "superclass" that matches:
///   typedef const struct __foo *XXXYYY;
static Type findImmutableCFSuperclass(ClangImporter::Implementation &impl,
                                      const clang::TypedefNameDecl *decl,
                                      CFPointeeInfo subclassInfo) {
   // If this type is already immutable, it has no immutable
   // superclass.
   if (subclassInfo.isConst())
      return Type();

   // If this typedef name does not contain "Mutable", it has no
   // immutable superclass.
   auto superclassName =
      getImmutableCFSuperclassName(decl, impl.getClangAstContext());
   if (!superclassName)
      return Type();

   // Look for a typedef that successfully classifies as a CF
   // typedef with the same underlying record.
   auto superclassTypedef = impl.lookupTypedef(superclassName);
   if (!superclassTypedef)
      return Type();
   auto superclassInfo = CFPointeeInfo::classifyTypedef(superclassTypedef);
   if (!superclassInfo || !superclassInfo.isRecord() ||
       !declaresSameEntity(superclassInfo.getRecord(), subclassInfo.getRecord()))
      return Type();

   // Try to import the superclass.
   Decl *importedSuperclassDecl =
      impl.importDeclReal(superclassTypedef, impl.CurrentVersion);
   if (!importedSuperclassDecl)
      return Type();

   auto importedSuperclass =
      cast<TypeDecl>(importedSuperclassDecl)->getDeclaredInterfaceType();
   assert(importedSuperclass->is<ClassType>() && "must have class type");
   return importedSuperclass;
}

/// Attempt to find a superclass for the given CF typedef.
static Type findCFSuperclass(ClangImporter::Implementation &impl,
                             const clang::TypedefNameDecl *decl,
                             CFPointeeInfo info) {
   if (Type immutable = findImmutableCFSuperclass(impl, decl, info))
      return immutable;

   // TODO: use NSObject if it exists?
   return Type();
}

//ClassDecl *
//TypePHPDeclConverter::importCFClassType(const clang::TypedefNameDecl *decl,
//                                      Identifier className, CFPointeeInfo info,
//                                      EffectiveClangContext effectiveContext) {
//   auto dc = Impl.importDeclContextOf(decl, effectiveContext);
//   if (!dc)
//      return nullptr;
//
//   Type superclass = findCFSuperclass(Impl, decl, info);
//
//   // TODO: maybe use NSObject as the superclass if we can find it?
//   // TODO: try to find a non-mutable type to use as the superclass.
//
//   auto theClass = Impl.createDeclWithClangNode<ClassDecl>(
//      decl, AccessLevel::Public, SourceLoc(), className, SourceLoc(), None,
//      nullptr, dc);
//   theClass->setSuperclass(superclass);
//   theClass->setAddedImplicitInitializers(); // suppress all initializers
//   theClass->setHasMissingVTableEntries(false);
//   theClass->setForeignClassKind(ClassDecl::ForeignKind::CFType);
//   // @todo
////   addObjCAttribute(theClass, None);
//
//   if (superclass) {
//      SmallVector<TypeLoc, 4> inheritedTypes;
//      inheritedTypes.push_back(TypeLoc::withoutLoc(superclass));
//      theClass->setInherited(Impl.TypePHPContext.AllocateCopy(inheritedTypes));
//   }
//
//   addSynthesizedInterfaceAttrs(Impl, theClass, {KnownInterfaceKind::CFObject});
//
//   // Look for bridging attributes on the clang record.  We can
//   // just check the most recent redeclaration, which will inherit
//   // any attributes from earlier declarations.
//   auto record = info.getRecord()->getMostRecentDecl();
//   if (info.isConst()) {
//      if (auto attr = record->getAttr<clang::ObjCBridgeAttr>()) {
//         // Record the Objective-C class to which this CF type is toll-free
//         // bridged.
//         if (ClassDecl *objcClass = dynCastIgnoringCompatibilityAlias<ClassDecl>(
//            Impl.importDeclByName(attr->getBridgedType()->getName()))) {
//            theClass->getAttrs().add(new (Impl.TypePHPContext)
//                                        ObjCBridgedAttr(objcClass));
//         }
//      }
//   } else {
//   // @todo
////      if (auto attr = record->getAttr<clang::ObjCBridgeMutableAttr>()) {
////         // Record the Objective-C class to which this CF type is toll-free
////         // bridged.
////         if (ClassDecl *objcClass = dynCastIgnoringCompatibilityAlias<ClassDecl>(
////            Impl.importDeclByName(attr->getBridgedType()->getName()))) {
////            theClass->getAttrs().add(new (Impl.TypePHPContext)
////                                        ObjCBridgedAttr(objcClass));
////         }
////      }
//   }
//
//   return theClass;
//}

Decl *TypePHPDeclConverter::importCompatibilityTypeAlias(
   const clang::NamedDecl *decl,
   ImportedName compatibilityName,
   ImportedName correctPolarphpName) {
   // Import the referenced declaration. If it doesn't come in as a type,
   // we don't care.
   Decl *importedDecl = nullptr;
   if (getVersion() >= getActivePolarphpVersion())
      importedDecl = Impl.importDecl(decl, ImportNameVersion::forTypes());
   if (!importedDecl && getVersion() != getActivePolarphpVersion())
      importedDecl = Impl.importDecl(decl, getActivePolarphpVersion());
   auto typeDecl = dyn_cast_or_null<TypeDecl>(importedDecl);
   if (!typeDecl)
      return nullptr;

   auto dc = Impl.importDeclContextOf(decl,
                                      compatibilityName.getEffectiveContext());
   if (!dc)
      return nullptr;

   // Create the type alias.
   auto alias = Impl.createDeclWithClangNode<TypeAliasDecl>(
      decl, AccessLevel::Public, Impl.importSourceLoc(decl->getBeginLoc()),
      SourceLoc(), compatibilityName.getDeclName().getBaseIdentifier(),
      Impl.importSourceLoc(decl->getLocation()), /*generic params*/nullptr, dc);

   auto *GTD = dyn_cast<GenericTypeDecl>(typeDecl);
   if (GTD && !isa<InterfaceDecl>(GTD)) {
      alias->setGenericSignature(GTD->getGenericSignature());
      if (GTD->isGeneric()) {
         alias->getAstContext().evaluator.cacheOutput(
            GenericParamListRequest{alias},
            std::move(GTD->getGenericParams()->clone(alias)));
      }
   }

   alias->setUnderlyingType(typeDecl->getDeclaredInterfaceType());

   // Record that this is the official version of this declaration.
   Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = alias;
   markAsVariant(alias, correctPolarphpName);
   return alias;
}

namespace {
template<typename D>
bool inheritanceListContainsInterface(D decl, const InterfaceDecl *proto) {
   bool anyObject = false;
   for (const auto &found :
      getDirectlyInheritedNominalTypeDecls(decl, anyObject)) {
      if (auto protoDecl = dyn_cast<InterfaceDecl>(found.second))
         if (protoDecl == proto || protoDecl->inheritsFrom(proto))
            return true;
   }

   return false;
}
}

static bool conformsToInterfaceInOriginalModule(NominalTypeDecl *nominal,
                                                const InterfaceDecl *proto,
                                                ModuleDecl *foundationModule) {
   auto &ctx = nominal->getAstContext();

   if (inheritanceListContainsInterface(nominal, proto))
      return true;

   for (auto attr : nominal->getAttrs().getAttributes<SynthesizedInterfaceAttr>())
      if (auto *otherProto = ctx.getInterface(attr->getInterfaceKind()))
         if (otherProto == proto || otherProto->inheritsFrom(proto))
            return true;

   // Only consider extensions from the original module...or from an overlay
   // or the Swift half of a mixed-source framework.
   const DeclContext *containingFile = nominal->getModuleScopeContext();
   ModuleDecl *originalModule = containingFile->getParentModule();

   ModuleDecl *overlayModule = nullptr;
   if (auto *clangUnit = dyn_cast<ClangModuleUnit>(containingFile))
      overlayModule = clangUnit->getOverlayModule();

   for (ExtensionDecl *extension : nominal->getExtensions()) {
      ModuleDecl *extensionModule = extension->getParentModule();
      if (extensionModule != originalModule && extensionModule != overlayModule &&
          extensionModule != foundationModule) {
         continue;
      }
      if (inheritanceListContainsInterface(extension, proto))
         return true;
   }

   return false;
}

Decl *
TypePHPDeclConverter::importTypePHPNewtype(const clang::TypedefNameDecl *decl,
                                           clang::SwiftNewtypeAttr *newtypeAttr,
                                           DeclContext *dc, Identifier name) {
   // The only (current) difference between swift_newtype(struct) and
   // swift_newtype(enum), until we can get real enum support, is that enums
   // have no un-labeled inits(). This is because enums are to be considered
   // closed, and if constructed from a rawValue, should be very explicit.
   bool unlabeledCtor = false;
   // @todo
//   switch (newtypeAttr->getNewtypeKind()) {
//      case clang::SwiftNewtypeAttr::NK_Enum:
//         unlabeledCtor = false;
//         // TODO: import as enum instead
//         break;
//
//      case clang::SwiftNewtypeAttr::NK_Struct:
//         unlabeledCtor = true;
//         break;
//         // No other cases yet
//   }

   auto &ctx = Impl.TypePHPContext;
   auto Loc = Impl.importSourceLoc(decl->getLocation());

   auto structDecl = Impl.createDeclWithClangNode<StructDecl>(
      decl, AccessLevel::Public, Loc, name, Loc, None, nullptr, dc);

   // Import the type of the underlying storage
   auto storedUnderlyingType = Impl.importTypeIgnoreIUO(
      decl->getUnderlyingType(), ImportTypeKind::Value, isInSystemModule(dc),
      Bridgeability::None, OTK_None);

   if (!storedUnderlyingType)
      return nullptr;

   if (auto objTy = storedUnderlyingType->getOptionalObjectType())
      storedUnderlyingType = objTy;

   // If the type is Unmanaged, that is it is not CF ARC audited,
   // we will store the underlying type and leave it up to the use site
   // to determine whether to use this new_type, or an Unmanaged<CF...> type.
   if (auto genericType = storedUnderlyingType->getAs<BoundGenericType>()) {
      if (genericType->getDecl() == Impl.TypePHPContext.getUnmanagedDecl()) {
         assert(genericType->getGenericArgs().size() == 1 && "other args?");
         storedUnderlyingType = genericType->getGenericArgs()[0];
      }
   }

   // Find a bridged type, which may be different
   auto computedPropertyUnderlyingType = Impl.importTypeIgnoreIUO(
      decl->getUnderlyingType(), ImportTypeKind::Property, isInSystemModule(dc),
      Bridgeability::Full, OTK_None);
   if (auto objTy = computedPropertyUnderlyingType->getOptionalObjectType())
      computedPropertyUnderlyingType = objTy;

   bool isBridged =
      !storedUnderlyingType->isEqual(computedPropertyUnderlyingType);

   // Determine the set of protocols to which the synthesized
   // type will conform.
   SmallVector<KnownInterfaceKind, 4> synthesizedInterfaces;

   // Local function to add a known protocol.
   auto addKnown = [&](KnownInterfaceKind kind) {
      synthesizedInterfaces.push_back(kind);
   };

   // Add conformances that are always available.
   addKnown(KnownInterfaceKind::RawRepresentable);
   addKnown(KnownInterfaceKind::SwiftNewtypeWrapper);

   // Local function to add a known protocol only when the
   // underlying type conforms to it.
   auto computedNominal = computedPropertyUnderlyingType->getAnyNominal();
   auto transferKnown = [&](KnownInterfaceKind kind) {
      if (!computedNominal)
         return false;

      auto proto = ctx.getInterface(kind);
      if (!proto)
         return false;

      // Break circularity by only looking for declared conformances in the
      // original module, or possibly its overlay.
      if (conformsToInterfaceInOriginalModule(computedNominal, proto,
                                              Impl.tryLoadFoundationModule())) {
         synthesizedInterfaces.push_back(kind);
         return true;
      }

      return false;
   };

   // Transfer conformances. Each of these needs a forwarding
   // implementation in the standard library.
   transferKnown(KnownInterfaceKind::Equatable);
   transferKnown(KnownInterfaceKind::Hashable);
   bool hasObjCBridgeable =
      transferKnown(KnownInterfaceKind::ObjectiveCBridgeable);
   bool wantsObjCBridgeableTypealias = hasObjCBridgeable && isBridged;

   // Wrappers around ObjC classes and protocols are also bridgeable.
   if (!hasObjCBridgeable) {
      if (isBridged) {
         if (auto *proto = dyn_cast_or_null<InterfaceDecl>(computedNominal))
            if (proto->getKnownInterfaceKind() == KnownInterfaceKind::Error)
               hasObjCBridgeable = true;
      } else {
         if (auto *objcClass = dyn_cast_or_null<ClassDecl>(computedNominal)) {
            switch (objcClass->getForeignClassKind()) {
               case ClassDecl::ForeignKind::Normal:
               case ClassDecl::ForeignKind::RuntimeOnly:
                  if (objcClass->hasClangNode())
                     hasObjCBridgeable = true;
                  break;
               case ClassDecl::ForeignKind::CFType:
                  break;
            }
         } else if (storedUnderlyingType->isObjCExistentialType()) {
            hasObjCBridgeable = true;
         }
      }

      if (hasObjCBridgeable) {
         addKnown(KnownInterfaceKind::ObjectiveCBridgeable);
         wantsObjCBridgeableTypealias = true;
      }
   }

   if (!isBridged) {
      // Simple, our stored type is equivalent to our computed
      // type.
      auto options = getDefaultMakeStructRawValuedOptions();
      if (unlabeledCtor)
         options |= MakeStructRawValuedFlags::MakeUnlabeledValueInit;

      makeStructRawValued(Impl, structDecl, storedUnderlyingType,
                          synthesizedInterfaces, options);
   } else {
      // We need to make a stored rawValue or storage type, and a
      // computed one of bridged type.
      makeStructRawValuedWithBridge(Impl, structDecl, storedUnderlyingType,
                                    computedPropertyUnderlyingType,
                                    synthesizedInterfaces,
         /*makeUnlabeledValueInit=*/unlabeledCtor);
   }

   if (wantsObjCBridgeableTypealias) {
      addSynthesizedTypealias(structDecl, ctx.Id_ObjectiveCType,
                              storedUnderlyingType);
   }

   Impl.ImportedDecls[{decl->getCanonicalDecl(), getVersion()}] = structDecl;
   return structDecl;
}

Decl *TypePHPDeclConverter::importEnumCase(const clang::EnumConstantDecl *decl,
                                           const clang::EnumDecl *clangEnum,
                                           EnumDecl *theEnum,
                                           Decl *correctDecl) {
   auto &context = Impl.TypePHPContext;
   Optional<ImportedName> correctPolarphpName;
   auto name =
      importFullName(decl, correctPolarphpName).getDeclName().getBaseIdentifier();
   if (name.empty())
      return nullptr;

   if (correctPolarphpName) {
      // We're creating a compatibility stub. Treat it as an enum case alias.
      auto correctCase = dyn_cast_or_null<EnumElementDecl>(correctDecl);
      if (!correctCase)
         return nullptr;

      // If the correct declaration was unavailable, don't map to it.
      // FIXME: This eliminates spurious errors, but affects QoI.
      if (correctCase->getAttrs().isUnavailable(Impl.TypePHPContext))
         return nullptr;

      auto compatibilityCase =
         importEnumCaseAlias(name, decl, correctCase, clangEnum, theEnum);
      if (compatibilityCase)
         markAsVariant(compatibilityCase, *correctPolarphpName);

      return compatibilityCase;
   }

   // Use the constant's underlying value as its raw value in Swift.
   bool negative = false;
   llvm::APSInt rawValue = decl->getInitVal();

   if (clangEnum->getIntegerType()->isSignedIntegerOrEnumerationType() &&
       rawValue.slt(0)) {
      rawValue = -rawValue;
      negative = true;
   }
   llvm::SmallString<12> rawValueText;
   rawValue.toString(rawValueText, 10, /*signed*/ false);
   StringRef rawValueTextC = context.AllocateCopy(StringRef(rawValueText));
   auto rawValueExpr =
      new (context) IntegerLiteralExpr(rawValueTextC, SourceLoc(),
         /*implicit*/ false);
   if (negative)
      rawValueExpr->setNegative(SourceLoc());

   auto element = Impl.createDeclWithClangNode<EnumElementDecl>(
      decl, AccessLevel::Public, SourceLoc(), name, nullptr,
      SourceLoc(), rawValueExpr, theEnum);

   Impl.importAttributes(decl, element);

   return element;
}

Decl *
TypePHPDeclConverter::importOptionConstant(const clang::EnumConstantDecl *decl,
                                           const clang::EnumDecl *clangEnum,
                                           NominalTypeDecl *theStruct) {
   Optional<ImportedName> correctPolarphpName;
   ImportedName nameInfo = importFullName(decl, correctPolarphpName);
   Identifier name = nameInfo.getDeclName().getBaseIdentifier();
   if (name.empty())
      return nullptr;

   // Create the constant.
   auto convertKind = ConstantConvertKind::Construction;
   if (isa<EnumDecl>(theStruct))
      convertKind = ConstantConvertKind::ConstructionWithUnwrap;
   Decl *CD = Impl.createConstant(
      name, theStruct, theStruct->getDeclaredInterfaceType(),
      clang::APValue(decl->getInitVal()), convertKind, /*isStatic*/ true, decl);
   Impl.importAttributes(decl, CD);

   // NS_OPTIONS members that have a value of 0 (typically named "None") do
   // not operate as a set-like member.  Mark them unavailable with a message
   // that says that they should be used as [].
   if (decl->getInitVal() == 0 && !nameInfo.hasCustomName() &&
       !CD->getAttrs().isUnavailable(Impl.TypePHPContext)) {
      /// Create an AvailableAttr that indicates specific availability
      /// for all platforms.
      auto attr = AvailableAttr::createPlatformAgnostic(
         Impl.TypePHPContext, "use [] to construct an empty option set");
      CD->getAttrs().add(attr);
   }

   // If this is a compatibility stub, mark it as such.
   if (correctPolarphpName)
      markAsVariant(CD, *correctPolarphpName);

   return CD;
}

Decl *TypePHPDeclConverter::importEnumCaseAlias(
   Identifier name, const clang::EnumConstantDecl *alias, ValueDecl *original,
   const clang::EnumDecl *clangEnum, NominalTypeDecl *importedEnum,
   DeclContext *importIntoDC) {
   if (name.empty())
      return nullptr;

   // Default the DeclContext to the enum type.
   if (!importIntoDC)
      importIntoDC = importedEnum;

   // Construct the original constant. Enum constants without payloads look
   // like simple values, but actually have type 'MyEnum.Type -> MyEnum'.
   auto constantRef =
      new (Impl.TypePHPContext) DeclRefExpr(original, DeclNameLoc(),
         /*implicit*/ true);
   constantRef->setType(original->getInterfaceType());

   Type importedEnumTy = importedEnum->getDeclaredInterfaceType();

   auto typeRef = TypeExpr::createImplicit(importedEnumTy, Impl.TypePHPContext);
   auto instantiate = new (Impl.TypePHPContext)
      DotSyntaxCallExpr(constantRef, SourceLoc(), typeRef);
   instantiate->setType(importedEnumTy);
   instantiate->setThrows(false);

   Decl *CD = Impl.createConstant(name, importIntoDC, importedEnumTy,
                                  instantiate, ConstantConvertKind::None,
      /*isStatic*/ true, alias);
   Impl.importAttributes(alias, CD);
   return CD;
}

NominalTypeDecl *
TypePHPDeclConverter::importAsOptionSetType(DeclContext *dc, Identifier name,
                                            const clang::EnumDecl *decl) {
   AstContext &ctx = Impl.TypePHPContext;

   // Compute the underlying type.
   auto underlyingType = Impl.importTypeIgnoreIUO(
      decl->getIntegerType(), ImportTypeKind::Enum, isInSystemModule(dc),
      Bridgeability::None);
   if (!underlyingType)
      return nullptr;

   auto Loc = Impl.importSourceLoc(decl->getLocation());

   // Create a struct with the underlying type as a field.
   auto structDecl = Impl.createDeclWithClangNode<StructDecl>(
      decl, AccessLevel::Public, Loc, name, Loc, None, nullptr, dc);

   makeStructRawValued(Impl, structDecl, underlyingType,
                       {KnownInterfaceKind::OptionSet});
   auto selfType = structDecl->getDeclaredInterfaceType();
   addSynthesizedTypealias(structDecl, ctx.Id_Element, selfType);
   addSynthesizedTypealias(structDecl, ctx.Id_ArrayLiteralElement, selfType);
   return structDecl;
}

Decl *TypePHPDeclConverter::importGlobalAsInitializer(
   const clang::FunctionDecl *decl,
   DeclName name,
   DeclContext *dc,
   CtorInitializerKind initKind,
   Optional<ImportedName> correctPolarphpName) {
   // TODO: Should this be an error? How can this come up?
   assert(dc->isTypeContext() && "cannot import as member onto non-type");

   // Check for some invalid imports
   if (dc->getSelfInterfaceDecl()) {
      // FIXME: clang source location
      Impl.TypePHPContext.Diags.diagnose({}, diag::swift_name_protocol_static,
         /*isInit=*/true);
      Impl.TypePHPContext.Diags.diagnose({}, diag::note_while_importing,
                                         decl->getName());
      return nullptr;
   }

   bool allowNSUIntegerAsInt =
      Impl.shouldAllowNSUIntegerAsInt(isInSystemModule(dc), decl);

   ArrayRef<Identifier> argNames = name.getArgumentNames();

   ParameterList *parameterList = nullptr;
   if (argNames.size() == 1 && decl->getNumParams() == 0) {
      // Special case: We need to create an empty first parameter for our
      // argument label
      auto *paramDecl =
         new (Impl.TypePHPContext) ParamDecl(
            SourceLoc(), SourceLoc(), argNames.front(),
            SourceLoc(), argNames.front(), dc);
      paramDecl->setSpecifier(ParamSpecifier::Default);
      paramDecl->setInterfaceType(Impl.TypePHPContext.TheEmptyTupleType);

      parameterList = ParameterList::createWithoutLoc(paramDecl);
   } else {
      parameterList = Impl.importFunctionParameterList(
         dc, decl, {decl->param_begin(), decl->param_end()}, decl->isVariadic(),
         allowNSUIntegerAsInt, argNames);
   }
   if (!parameterList)
      return nullptr;

   auto importedType =
      Impl.importFunctionReturnType(dc, decl, allowNSUIntegerAsInt);

   // Update the failability appropriately based on the imported method type.
   bool failable = false, isIUO = false;
   if (importedType.isImplicitlyUnwrapped()) {
      assert(importedType.getType()->getOptionalObjectType());
      failable = true;
      isIUO = true;
   } else if (importedType.getType()->getOptionalObjectType()) {
      failable = true;
   }

   auto result = Impl.createDeclWithClangNode<ConstructorDecl>(
      decl, AccessLevel::Public, name, /*NameLoc=*/SourceLoc(),
      failable, /*FailabilityLoc=*/SourceLoc(),
      /*Throws=*/false, /*ThrowsLoc=*/SourceLoc(), parameterList,
      /*GenericParams=*/nullptr, dc);
   result->setImplicitlyUnwrappedOptional(isIUO);
   result->getAstContext().evaluator.cacheOutput(InitKindRequest{result},
                                                 std::move(initKind));
   result->setImportAsStaticMember();

   Impl.recordImplicitUnwrapForDecl(result,
                                    importedType.isImplicitlyUnwrapped());
   result->setOverriddenDecls({ });
   // @todo
//   result->setIsObjC(false);
   result->setIsDynamic(false);

   finishFuncDecl(decl, result);
   if (correctPolarphpName)
      markAsVariant(result, *correctPolarphpName);
   return result;
}


/// Create an implicit property given the imported name of one of
/// the accessors.
VarDecl *
TypePHPDeclConverter::getImplicitProperty(ImportedName importedName,
                                          const clang::FunctionDecl *accessor) {
   // Check whether we already know about the property.
   auto knownProperty = Impl.FunctionsAsProperties.find(accessor);
   if (knownProperty != Impl.FunctionsAsProperties.end())
      return knownProperty->second;

   // Determine whether we have the getter or setter.
   const clang::FunctionDecl *getter = nullptr;
   ImportedName getterName;
   Optional<ImportedName> swift3GetterName;
   const clang::FunctionDecl *setter = nullptr;
   ImportedName setterName;
   Optional<ImportedName> swift3SetterName;
   switch (importedName.getAccessorKind()) {
      case ImportedAccessorKind::None:
      case ImportedAccessorKind::SubscriptGetter:
      case ImportedAccessorKind::SubscriptSetter:
         llvm_unreachable("Not a property accessor");

      case ImportedAccessorKind::PropertyGetter:
         getter = accessor;
         getterName = importedName;
         break;

      case ImportedAccessorKind::PropertySetter:
         setter = accessor;
         setterName = importedName;
         break;
   }

   // Find the other accessor, if it exists.
   auto propertyName = importedName.getDeclName().getBaseIdentifier();
   auto lookupTable =
      Impl.findLookupTable(*getClangSubmoduleForDecl(accessor));
   assert(lookupTable && "No lookup table?");
   bool foundAccessor = false;
   for (auto entry : lookupTable->lookup(SerializedTypePHPName(propertyName),
                                         importedName.getEffectiveContext())) {
      auto decl = entry.dyn_cast<clang::NamedDecl *>();
      if (!decl)
         continue;

      auto function = dyn_cast<clang::FunctionDecl>(decl);
      if (!function)
         continue;

      if (function->getCanonicalDecl() == accessor->getCanonicalDecl()) {
         foundAccessor = true;
         continue;
      }

      if (!getter) {
         // Find the self index for the getter.
         getterName = importFullName(function, swift3GetterName);
         if (!getterName)
            continue;

         getter = function;
         continue;
      }

      if (!setter) {
         // Find the self index for the setter.
         setterName = importFullName(function, swift3SetterName);
         if (!setterName)
            continue;

         setter = function;
         continue;
      }

      // We already have both a getter and a setter; something is
      // amiss, so bail out.
      return nullptr;
   }

   assert(foundAccessor && "Didn't find the original accessor? "
                           "Try clearing your module cache");

   // If there is no getter, there's nothing we can do.
   if (!getter)
      return nullptr;

   // Retrieve the type of the property that is implied by the getter.
   auto propertyType =
      getAccessorPropertyType(getter, false, getterName.getSelfIndex());
   if (propertyType.isNull())
      return nullptr;

   // If there is a setter, check that the property it implies
   // matches that of the getter.
   if (setter) {
      auto setterPropertyType =
         getAccessorPropertyType(setter, true, setterName.getSelfIndex());
      if (setterPropertyType.isNull())
         return nullptr;

      // If the inferred property types don't match up, we can't
      // form a property.
      if (!getter->getASTContext().hasSameType(propertyType, setterPropertyType))
         return nullptr;
   }

   // Import the property's context.
   auto dc = Impl.importDeclContextOf(getter, getterName.getEffectiveContext());
   if (!dc)
      return nullptr;

   // Is this a static property?
   bool isStatic = false;
   if (dc->isTypeContext() && !getterName.getSelfIndex())
      isStatic = true;

   // Compute the property type.
   bool isFromSystemModule = isInSystemModule(dc);
   auto importedType = Impl.importType(
      propertyType, ImportTypeKind::Property,
      Impl.shouldAllowNSUIntegerAsInt(isFromSystemModule, getter),
      Bridgeability::Full, OTK_ImplicitlyUnwrappedOptional);
   if (!importedType)
      return nullptr;

   Type swiftPropertyType = importedType.getType();

   auto property = Impl.createDeclWithClangNode<VarDecl>(
      getter, AccessLevel::Public, /*IsStatic*/isStatic,
      VarDecl::Introducer::Var, /*IsCaptureList*/false, SourceLoc(),
      propertyName, dc);
   property->setInterfaceType(swiftPropertyType);
   // @todo
//   property->setIsObjC(false);
   property->setIsDynamic(false);

   Impl.recordImplicitUnwrapForDecl(property,
                                    importedType.isImplicitlyUnwrapped());

   // Note that we've formed this property.
   Impl.FunctionsAsProperties[getter] = property;
   if (setter)
      Impl.FunctionsAsProperties[setter] = property;

   // If this property is in a class or class extension context,
   // add "final".
   if (dc->getSelfClassDecl())
      property->getAttrs().add(new (Impl.TypePHPContext)
                                  FinalAttr(/*IsImplicit=*/true));

   // Import the getter.
   auto *typePHPGetter = dyn_cast_or_null<AccessorDecl>(
      importFunctionDecl(getter, getterName, None,
                         AccessorInfo{property, AccessorKind::Get}));
   if (!typePHPGetter)
      return nullptr;

   Impl.importAttributes(getter, typePHPGetter);
   Impl.ImportedDecls[{getter, getVersion()}] = typePHPGetter;
   if (swift3GetterName)
      markAsVariant(typePHPGetter, *swift3GetterName);

   // Import the setter.
   AccessorDecl *typePHPSetter = nullptr;
   if (setter) {
      typePHPSetter = dyn_cast_or_null<AccessorDecl>(
         importFunctionDecl(setter, setterName, None,
                            AccessorInfo{property, AccessorKind::Set}));
      if (!typePHPSetter)
         return nullptr;

      Impl.importAttributes(setter, typePHPSetter);
      Impl.ImportedDecls[{setter, getVersion()}] = typePHPSetter;
      if (swift3SetterName)
         markAsVariant(typePHPSetter, *swift3SetterName);
   }

   if (typePHPGetter) property->setIsGetterMutating(typePHPGetter->isMutating());
   if (typePHPSetter) property->setIsSetterMutating(typePHPSetter->isMutating());

   // Make this a computed property.
   makeComputed(property, typePHPGetter, typePHPSetter);

   // Make the property the alternate declaration for the getter.
   Impl.addAlternateDecl(typePHPGetter, property);

   return property;
}
//
//ConstructorDecl *TypePHPDeclConverter::importConstructor(
//   const clang::ObjCMethodDecl *objcMethod, DeclContext *dc, bool implicit,
//   Optional<CtorInitializerKind> kind, bool required) {
//   // Only methods in the 'init' family can become constructors.
//   assert(isInitMethod(objcMethod) && "Not a real init method");
//
//   // Check whether we've already created the constructor.
//   auto known =
//      Impl.Constructors.find(std::make_tuple(objcMethod, dc, getVersion()));
//   if (known != Impl.Constructors.end())
//      return known->second;
//
//   // Check whether there is already a method with this selector.
////   auto selector = Impl.importSelector(objcMethod->getSelector());
//   if (isActivePolarphpVersion() &&
//       isMethodAlreadyImported(selector, /*isInstance=*/true, dc,
//                               [](AbstractFunctionDecl *fn) {
//                                  return true;
//                               }))
//      return nullptr;
//
//   // Map the name and complete the import.
//   ArrayRef<const clang::ParmVarDecl *> params{objcMethod->param_begin(),
//                                               objcMethod->param_end()};
//
//   bool variadic = objcMethod->isVariadic();
//   Optional<ImportedName> correctPolarphpName;
//   auto importedName = importFullName(objcMethod, correctPolarphpName);
//   if (!importedName)
//      return nullptr;
//
//   // If we dropped the variadic, handle it now.
//   if (importedName.droppedVariadic()) {
//      selector = ObjCSelector(Impl.TypePHPContext, selector.getNumArgs() - 1,
//                              selector.getSelectorPieces().drop_back());
//      params = params.drop_back(1);
//      variadic = false;
//   }
//
//   bool redundant;
//   auto result = importConstructor(objcMethod, dc, implicit,
//                                   kind.getValueOr(importedName.getInitKind()),
//                                   required, selector, importedName, params,
//                                   variadic, redundant);
//
//   // If this is a compatibility stub, mark it as such.
//   if (result && correctPolarphpName)
//      markAsVariant(result, *correctPolarphpName);
//
//   return result;
//}

/// Returns the latest "introduced" version on the current platform for
/// \p D.
llvm::VersionTuple
TypePHPDeclConverter::findLatestIntroduction(const clang::Decl *D) {
   llvm::VersionTuple result;

   for (auto *attr : D->specific_attrs<clang::AvailabilityAttr>()) {
      if (attr->getPlatform()->getName() == "swift") {
         llvm::VersionTuple maxVersion{~0U, ~0U, ~0U};
         return maxVersion;
      }

      // Does this availability attribute map to the platform we are
      // currently targeting?
      if (!Impl.platformAvailability.isPlatformRelevant(
         attr->getPlatform()->getName())) {
         continue;
      }
      // Take advantage of the empty version being 0.0.0.0.
      result = std::max(result, attr->getIntroduced());
   }

   return result;
}

/// Returns true if importing \p objcMethod will produce a "better"
/// initializer than \p existingCtor.
bool TypePHPDeclConverter::existingConstructorIsWorse(
   const ConstructorDecl *existingCtor,
   const clang::ObjCMethodDecl *objcMethod, CtorInitializerKind kind) {
   CtorInitializerKind existingKind = existingCtor->getInitKind();

   // If one constructor is unavailable in Swift and the other is
   // not, keep the available one.
   bool existingIsUnavailable =
      existingCtor->getAttrs().isUnavailable(Impl.TypePHPContext);
   bool newIsUnavailable = Impl.isUnavailableInSwift(objcMethod);
   if (existingIsUnavailable != newIsUnavailable)
      return existingIsUnavailable;

   // If the new kind is the same as the existing kind, stick with
   // the existing constructor.
   if (existingKind == kind)
      return false;

   // Check for cases that are obviously better or obviously worse.
   if (kind == CtorInitializerKind::Designated ||
       existingKind == CtorInitializerKind::Factory)
      return true;

   if (kind == CtorInitializerKind::Factory ||
       existingKind == CtorInitializerKind::Designated)
      return false;

   assert(kind == CtorInitializerKind::Convenience ||
          kind == CtorInitializerKind::ConvenienceFactory);
   assert(existingKind == CtorInitializerKind::Convenience ||
          existingKind == CtorInitializerKind::ConvenienceFactory);

   // Between different kinds of convenience initializers, keep the one that
   // was introduced first.
   // FIXME: But if one of them is now deprecated, should we prefer the
   // other?
   llvm::VersionTuple introduced = findLatestIntroduction(objcMethod);
   AvailabilityContext existingAvailability =
      AvailabilityInference::availableRange(existingCtor, Impl.TypePHPContext);
   assert(!existingAvailability.isKnownUnreachable());

   if (existingAvailability.isAlwaysAvailable()) {
      if (!introduced.empty())
         return false;
   } else {
      VersionRange existingIntroduced = existingAvailability.getOSVersion();
      if (introduced != existingIntroduced.getLowerEndpoint()) {
         return introduced < existingIntroduced.getLowerEndpoint();
      }
   }

   // The "introduced" versions are the same. Prefer Convenience over
   // ConvenienceFactory, but otherwise prefer leaving things as they are.
   if (kind == CtorInitializerKind::Convenience &&
       existingKind == CtorInitializerKind::ConvenienceFactory)
      return true;

   return false;
}

/// Given an imported method, try to import it as a constructor.
///
/// Objective-C methods in the 'init' family are imported as
/// constructors in Swift, enabling object construction syntax, e.g.,
///
/// \code
/// // in objc: [[NSArray alloc] initWithCapacity:1024]
/// NSArray(capacity: 1024)
/// \endcode
///
/// This variant of the function is responsible for actually binding the
/// constructor declaration appropriately.
//ConstructorDecl *TypePHPDeclConverter::importConstructor(
//   const clang::ObjCMethodDecl *objcMethod, DeclContext *dc, bool implicit,
//   CtorInitializerKind kind, bool required, ObjCSelector selector,
//   ImportedName importedName, ArrayRef<const clang::ParmVarDecl *> args,
//   bool variadic, bool &redundant) {
//   redundant = false;
//
//   // Figure out the type of the container.
//   auto ownerNominal = dc->getSelfNominalTypeDecl();
//   assert(ownerNominal && "Method in non-type context?");
//
//   // Import the type that this method will have.
//   Optional<ForeignErrorConvention> errorConvention;
//   ParameterList *bodyParams;
//   auto importedType = Impl.importMethodParamsAndReturnType(
//      dc, objcMethod, args, variadic, isInSystemModule(dc), &bodyParams,
//      importedName, errorConvention, SpecialMethodKind::Constructor);
//   if (!importedType)
//      return nullptr;
//
//   // Determine the failability of this initializer.
//   bool resultIsOptional = (bool) importedType.getType()->getOptionalObjectType();
//
//   // Update the failability appropriately based on the imported method type.
//   assert(resultIsOptional || !importedType.isImplicitlyUnwrapped());
//   OptionalTypeKind failability = OTK_None;
//   if (resultIsOptional) {
//      failability = OTK_Optional;
//      if (importedType.isImplicitlyUnwrapped())
//         failability = OTK_ImplicitlyUnwrappedOptional;
//   }
//
//   // Rebuild the function type with the appropriate result type;
//   Type resultTy = dc->getSelfInterfaceType();
//   if (resultIsOptional)
//      resultTy = OptionalType::get(resultTy);
//
//   // Look for other imported constructors that occur in this context with
//   // the same name.
//   SmallVector<AnyFunctionType::Param, 4> allocParams;
//   bodyParams->getParams(allocParams);
//
//   TinyPtrVector<ConstructorDecl *> ctors;
//   auto found = Impl.ConstructorsForNominal.find(ownerNominal);
//   if (found != Impl.ConstructorsForNominal.end())
//      ctors = found->second;
//
//   for (auto ctor : ctors) {
//      if (ctor->isInvalid() ||
//          ctor->getAttrs().isUnavailable(Impl.TypePHPContext) ||
//          !ctor->getClangDecl())
//         continue;
//
//      // If the types don't match, this is a different constructor with
//      // the same selector. This can happen when an overlay overloads an
//      // existing selector with a Swift-only signature.
//      auto ctorParams = ctor->getInterfaceType()
//         ->castTo<AnyFunctionType>()
//         ->getResult()
//         ->castTo<AnyFunctionType>()
//         ->getParams();
//      if (!AnyFunctionType::equalParams(ctorParams, allocParams)) {
//         continue;
//      }
//
//      // If the existing constructor has a less-desirable kind, mark
//      // the existing constructor unavailable.
//      if (existingConstructorIsWorse(ctor, objcMethod, kind)) {
//         // Show exactly where this constructor came from.
//         llvm::SmallString<32> errorStr;
//         errorStr += "superseded by import of ";
//         if (objcMethod->isClassMethod())
//            errorStr += "+[";
//         else
//            errorStr += "-[";
//
//         auto objcDC = objcMethod->getDeclContext();
//         if (auto objcClass = dyn_cast<clang::ObjCInterfaceDecl>(objcDC)) {
//            errorStr += objcClass->getName();
//            errorStr += ' ';
//         } else if (auto objcCat = dyn_cast<clang::ObjCCategoryDecl>(objcDC)) {
//            errorStr += objcCat->getClassInterface()->getName();
//            auto catName = objcCat->getName();
//            if (!catName.empty()) {
//               errorStr += '(';
//               errorStr += catName;
//               errorStr += ')';
//            }
//            errorStr += ' ';
//         } else if (auto objcProto = dyn_cast<clang::ObjCInterfaceDecl>(objcDC)) {
//            errorStr += objcProto->getName();
//            errorStr += ' ';
//         }
//
//         errorStr += objcMethod->getSelector().getAsString();
//         errorStr += ']';
//
//         auto attr = AvailableAttr::createPlatformAgnostic(
//            Impl.TypePHPContext, Impl.TypePHPContext.AllocateCopy(errorStr.str()));
//         ctor->getAttrs().add(attr);
//         continue;
//      }
//
//      // Otherwise, we shouldn't create a new constructor, because
//      // it will be no better than the existing one.
//      redundant = true;
//      return nullptr;
//   }
//
//   // Check whether we've already created the constructor.
//   auto known =
//      Impl.Constructors.find(std::make_tuple(objcMethod, dc, getVersion()));
//   if (known != Impl.Constructors.end())
//      return known->second;
//
//   // Create the actual constructor.
//   auto result = Impl.createDeclWithClangNode<ConstructorDecl>(
//      objcMethod, AccessLevel::Public, importedName.getDeclName(),
//      /*NameLoc=*/SourceLoc(), failability, /*FailabilityLoc=*/SourceLoc(),
//      /*Throws=*/importedName.getErrorInfo().hasValue(),
//      /*ThrowsLoc=*/SourceLoc(), bodyParams,
//      /*GenericParams=*/nullptr, dc);
//
//   addObjCAttribute(result, selector);
//
//   Impl.recordImplicitUnwrapForDecl(result,
//                                    importedType.isImplicitlyUnwrapped());
//
//   if (implicit)
//      result->setImplicit();
//
//   // Set the kind of initializer.
//   result->getAstContext().evaluator.cacheOutput(InitKindRequest{result},
//                                                 std::move(kind));
//
//   // Consult API notes to determine whether this initializer is required.
//   if (!required && isRequiredInitializer(objcMethod))
//      required = true;
//
//   // Check whether this initializer satisfies a requirement in a protocol.
//   if (!required && !isa<InterfaceDecl>(dc) && objcMethod->isInstanceMethod()) {
//      auto objcParent =
//         cast<clang::ObjCContainerDecl>(objcMethod->getDeclContext());
//
//      if (isa<clang::ObjCInterfaceDecl>(objcParent)) {
//         // An initializer declared in a protocol is required.
//         required = true;
//      } else {
//         // If the class in which this initializer was declared conforms to a
//         // protocol that requires this initializer, then this initializer is
//         // required.
//         SmallPtrSet<clang::ObjCInterfaceDecl *, 8> objcInterfaces;
//         objcParent->getAstContext().CollectInheritedInterfaces(objcParent,
//                                                                objcInterfaces);
//         for (auto objcProto : objcInterfaces) {
//            for (auto decl : objcProto->lookup(objcMethod->getSelector())) {
//               if (cast<clang::ObjCMethodDecl>(decl)->isInstanceMethod()) {
//                  required = true;
//                  break;
//               }
//            }
//
//            if (required)
//               break;
//         }
//      }
//   }
//
//   // If this initializer is required, add the appropriate attribute.
//   if (required) {
//      result->getAttrs().add(new (Impl.TypePHPContext)
//                                RequiredAttr(/*IsImplicit=*/true));
//   }
//
//   // Record the error convention.
//   if (errorConvention) {
//      result->setForeignErrorConvention(*errorConvention);
//   }
//
//   // Record the constructor for future re-use.
//   Impl.Constructors[std::make_tuple(objcMethod, dc, getVersion())] = result;
//   Impl.ConstructorsForNominal[ownerNominal].push_back(result);
//
//   // If this constructor overrides another constructor, mark it as such.
//   recordObjCOverride(result);
//
//   return result;
//}
//
//void TypePHPDeclConverter::recordObjCOverride(AbstractFunctionDecl *decl) {
//   // Make sure that we always set the overriden declarations.
//   SWIFT_DEFER {
//      if (!decl->overriddenDeclsComputed())
//      decl->setOverriddenDecls({ });
//   };
//
//   // Figure out the class in which this method occurs.
//   auto classDecl = decl->getDeclContext()->getSelfClassDecl();
//   if (!classDecl)
//      return;
//   auto superDecl = classDecl->getSuperclassDecl();
//   if (!superDecl)
//      return;
//   // Dig out the Objective-C superclass.
//   SmallVector<ValueDecl *, 4> results;
//   superDecl->lookupQualified(superDecl, decl->getFullName(),
//                              NL_QualifiedDefault | NL_KnownNoDependency,
//                              results);
//   for (auto member : results) {
//      if (member->getKind() != decl->getKind() ||
//          member->isInstanceMember() != decl->isInstanceMember() ||
//          member->isObjC() != decl->isObjC())
//         continue;
//      // Set function override.
//      if (auto func = dyn_cast<FuncDecl>(decl)) {
//         auto foundFunc = cast<FuncDecl>(member);
//         // Require a selector match.
//         if (foundFunc->isObjC() &&
//             func->getObjCSelector() != foundFunc->getObjCSelector())
//            continue;
//         func->setOverriddenDecl(foundFunc);
//         func->getAttrs().add(new (func->getAstContext()) OverrideAttr(true));
//         return;
//      }
//      // Set constructor override.
//      auto ctor = cast<ConstructorDecl>(decl);
//      auto memberCtor = cast<ConstructorDecl>(member);
//      // Require a selector match.
//      if (ctor->isObjC() &&
//          ctor->getObjCSelector() != memberCtor->getObjCSelector())
//         continue;
//      ctor->setOverriddenDecl(memberCtor);
//      ctor->getAttrs().add(new (ctor->getAstContext()) OverrideAttr(true));
//
//      // Propagate 'required' to subclass initializers.
//      if (memberCtor->isRequired() &&
//          !ctor->getAttrs().hasAttribute<RequiredAttr>()) {
//         ctor->getAttrs().add(new (Impl.TypePHPContext)
//                                 RequiredAttr(/*IsImplicit=*/true));
//      }
//   }
//}

// Note: This function ignores labels.
static bool areParameterTypesEqual(const ParameterList &params1,
                                   const ParameterList &params2) {
   if (params1.size() != params2.size())
      return false;

   for (unsigned i : indices(params1)) {
      if (!params1[i]->getInterfaceType()->isEqual(
         params2[i]->getInterfaceType())) {
         return false;
      }

      if (params1[i]->getValueOwnership() !=
          params2[i]->getValueOwnership()) {
         return false;
      }
   }

   return true;
}
//
//void TypePHPDeclConverter::recordObjCOverride(SubscriptDecl *subscript) {
//   // Figure out the class in which this subscript occurs.
//   auto classTy = subscript->getDeclContext()->getSelfClassDecl();
//   if (!classTy)
//      return;
//
//   auto superDecl = classTy->getSuperclassDecl();
//   if (!superDecl)
//      return;
//
//   // Determine whether this subscript operation overrides another subscript
//   // operation.
//   SmallVector<ValueDecl *, 2> lookup;
//   subscript->getModuleContext()->lookupQualified(
//      superDecl, subscript->getFullName(),
//      NL_QualifiedDefault | NL_KnownNoDependency, lookup);
//
//   for (auto result : lookup) {
//      auto parentSub = dyn_cast<SubscriptDecl>(result);
//      if (!parentSub)
//         continue;
//
//      if (!areParameterTypesEqual(*subscript->getIndices(),
//                                  *parentSub->getIndices()))
//         continue;
//
//      // The index types match. This is an override, so mark it as such.
//      subscript->setOverriddenDecl(parentSub);
//      auto getterThunk = subscript->getParsedAccessor(AccessorKind::Get);
//      getterThunk->setOverriddenDecl(parentSub->getParsedAccessor(AccessorKind::Get));
//      if (auto parentSetter = parentSub->getParsedAccessor(AccessorKind::Set)) {
//         if (auto setterThunk = subscript->getParsedAccessor(AccessorKind::Set))
//            setterThunk->setOverriddenDecl(parentSetter);
//      }
//
//      // FIXME: Eventually, deal with multiple overrides.
//      break;
//   }
//}
//
///// Given either the getter or setter for a subscript operation,
///// create the Swift subscript declaration.
//SubscriptDecl *
//TypePHPDeclConverter::importSubscript(Decl *decl,
//                                    const clang::ObjCMethodDecl *objcMethod) {
//   assert(objcMethod->isInstanceMethod() && "Caller must filter");
//
//   // If the method we're attempting to import has the
//   // swift_private attribute, don't import as a subscript.
//   if (objcMethod->hasAttr<clang::SwiftPrivateAttr>())
//      return nullptr;
//
//   // Figure out where to look for the counterpart.
//   const clang::ObjCInterfaceDecl *interface = nullptr;
//   const clang::ObjCInterfaceDecl *protocol =
//      dyn_cast<clang::ObjCInterfaceDecl>(objcMethod->getDeclContext());
//   if (!protocol)
//      interface = objcMethod->getClassInterface();
//   auto lookupInstanceMethod = [&](
//      clang::Selector Sel) -> const clang::ObjCMethodDecl * {
//      if (interface)
//         return interface->lookupInstanceMethod(Sel);
//
//      return protocol->lookupInstanceMethod(Sel);
//   };
//
//   auto findCounterpart = [&](clang::Selector sel) -> FuncDecl * {
//      // If the declaration we're starting from is in a class, first
//      // look for a class member with the appropriate selector.
//      if (auto classDecl = decl->getDeclContext()->getSelfClassDecl()) {
//         auto swiftSel = Impl.importSelector(sel);
//         for (auto found : classDecl->lookupDirect(swiftSel, true)) {
//            if (auto foundFunc = dyn_cast<FuncDecl>(found))
//               if (foundFunc->hasClangNode())
//                  return foundFunc;
//         }
//      }
//
//      // Find based on selector within the current type.
//      auto counterpart = lookupInstanceMethod(sel);
//      if (!counterpart)
//         return nullptr;
//
//      return cast_or_null<FuncDecl>(
//         Impl.importDecl(counterpart, getActivePolarphpVersion()));
//   };
//
//   // Determine the selector of the counterpart.
//   FuncDecl *getter = nullptr, *setter = nullptr;
//   const clang::ObjCMethodDecl *getterObjCMethod = nullptr,
//      *setterObjCMethod = nullptr;
//   clang::Selector counterpartSelector;
//   if (objcMethod->getSelector() == Impl.objectAtIndexedSubscript) {
//      getter = cast<FuncDecl>(decl);
//      getterObjCMethod = objcMethod;
//      counterpartSelector = Impl.setObjectAtIndexedSubscript;
//   } else if (objcMethod->getSelector() == Impl.setObjectAtIndexedSubscript) {
//      setter = cast<FuncDecl>(decl);
//      setterObjCMethod = objcMethod;
//      counterpartSelector = Impl.objectAtIndexedSubscript;
//   } else if (objcMethod->getSelector() == Impl.objectForKeyedSubscript) {
//      getter = cast<FuncDecl>(decl);
//      getterObjCMethod = objcMethod;
//      counterpartSelector = Impl.setObjectForKeyedSubscript;
//   } else if (objcMethod->getSelector() == Impl.setObjectForKeyedSubscript) {
//      setter = cast<FuncDecl>(decl);
//      setterObjCMethod = objcMethod;
//      counterpartSelector = Impl.objectForKeyedSubscript;
//   } else {
//      llvm_unreachable("Unknown getter/setter selector");
//   }
//
//   // Find the counterpart.
//   bool optionalMethods = (objcMethod->getImplementationControl() ==
//                           clang::ObjCMethodDecl::Optional);
//
//   if (auto *counterpart = findCounterpart(counterpartSelector)) {
//      const clang::ObjCMethodDecl *counterpartMethod = nullptr;
//
//      // If the counterpart to the method we're attempting to import has the
//      // swift_private attribute, don't import as a subscript.
//      if (auto importedFrom = counterpart->getClangDecl()) {
//         if (importedFrom->hasAttr<clang::SwiftPrivateAttr>())
//            return nullptr;
//
//         counterpartMethod = cast<clang::ObjCMethodDecl>(importedFrom);
//         if (optionalMethods)
//            optionalMethods = (counterpartMethod->getImplementationControl() ==
//                               clang::ObjCMethodDecl::Optional);
//      }
//
//      assert(!counterpart || !counterpart->isStatic());
//
//      if (getter) {
//         setter = counterpart;
//         setterObjCMethod = counterpartMethod;
//      } else {
//         getter = counterpart;
//         getterObjCMethod = counterpartMethod;
//      }
//   }
//
//   // Swift doesn't have write-only subscripting.
//   if (!getter)
//      return nullptr;
//
//   // Check whether we've already created a subscript operation for
//   // this getter/setter pair.
//   if (auto subscript = Impl.Subscripts[{getter, setter}]) {
//      return subscript->getDeclContext() == decl->getDeclContext() ? subscript
//                                                                   : nullptr;
//   }
//
//   // Find the getter indices and make sure they match.
//   ParamDecl *getterIndex;
//   {
//      auto params = getter->getParameters();
//      if (params->size() != 1)
//         return nullptr;
//      getterIndex = params->get(0);
//   }
//
//   // Compute the element type based on the getter, looking through
//   // the implicit 'self' parameter and the normal function
//   // parameters.
//   auto elementTy = getter->getResultInterfaceType();
//
//   // Local function to mark the setter unavailable.
//   auto makeSetterUnavailable = [&] {
//      if (setter && !setter->getAttrs().isUnavailable(Impl.TypePHPContext))
//         Impl.markUnavailable(setter, "use subscripting");
//   };
//
//   // If we have a setter, rectify it with the getter.
//   ParamDecl *setterIndex;
//   bool getterAndSetterInSameType = false;
//   bool isIUO = getter->isImplicitlyUnwrappedOptional();
//   if (setter) {
//      // Whether there is an existing read-only subscript for which
//      // we have now found a setter.
//      SubscriptDecl *existingSubscript = Impl.Subscripts[{getter, nullptr}];
//
//      // Are the getter and the setter in the same type.
//      getterAndSetterInSameType =
//         (getter->getDeclContext()->getSelfNominalTypeDecl() ==
//          setter->getDeclContext()->getSelfNominalTypeDecl());
//
//      // Whether we can update the types involved in the subscript
//      // operation.
//      bool canUpdateSubscriptType =
//         !existingSubscript && getterAndSetterInSameType;
//
//      // Determine the setter's element type and indices.
//      Type setterElementTy;
//      std::tie(setterElementTy, setterIndex) = decomposeSubscriptSetter(setter);
//
//      // Rectify the setter element type with the getter's element type,
//      // and determine if the result is an implicitly unwrapped optional
//      // type.
//      auto importedType = rectifySubscriptTypes(elementTy, isIUO, setterElementTy,
//                                                canUpdateSubscriptType);
//      if (!importedType)
//         return decl == getter ? existingSubscript : nullptr;
//
//      isIUO = importedType.isImplicitlyUnwrapped();
//
//      // Update the element type.
//      elementTy = importedType.getType();
//
//      // Make sure that the index types are equivalent.
//      // FIXME: Rectify these the same way we do for element types.
//      if (!setterIndex->getType()->isEqual(getterIndex->getType())) {
//         // If there is an existing subscript operation, we're done.
//         if (existingSubscript)
//            return decl == getter ? existingSubscript : nullptr;
//
//         // Otherwise, just forget we had a setter.
//         // FIXME: This feels very, very wrong.
//         setter = nullptr;
//         setterObjCMethod = nullptr;
//         setterIndex = nullptr;
//      }
//
//      // If there is an existing subscript within this context, we
//      // cannot create a new subscript. Update it if possible.
//      if (setter && existingSubscript && getterAndSetterInSameType) {
//         // Can we update the subscript by adding the setter?
//         if (existingSubscript->hasClangNode() &&
//             !existingSubscript->supportsMutation()) {
//            // Create the setter thunk.
//            auto setterThunk = buildSubscriptSetterDecl(
//               Impl, existingSubscript, setter, elementTy,
//               setter->getDeclContext(), setterIndex);
//
//            // Set the computed setter.
//            existingSubscript->setComputedSetter(setterThunk);
//
//            // Mark the setter as unavailable; one should use
//            // subscripting when it is present.
//            makeSetterUnavailable();
//         }
//
//         return decl == getter ? existingSubscript : nullptr;
//      }
//   }
//
//   // The context into which the subscript should go. We prefer wherever the
//   // getter is declared unless the two accessors are in different types and the
//   // one we started with is the setter. This happens when:
//   // - A read-only subscript is made read/write is a subclass.
//   // - A setter is redeclared in a subclass, but not the getter.
//   // And not when:
//   // - A getter is redeclared in a subclass, but not the setter.
//   // - The getter and setter are part of the same type.
//   // - There is no setter.
//   bool associateWithSetter = !getterAndSetterInSameType && setter == decl;
//   DeclContext *dc =
//      associateWithSetter ? setter->getDeclContext() : getter->getDeclContext();
//
//   // Build the subscript declaration.
//   auto &C = Impl.TypePHPContext;
//   auto bodyParams = ParameterList::create(C, getterIndex);
//   DeclName name(C, DeclBaseName::createSubscript(), {Identifier()});
//   auto subscript = Impl.createDeclWithClangNode<SubscriptDecl>(
//      getter->getClangNode(), getOverridableAccessLevel(dc), name,
//      /*StaticLoc=*/SourceLoc(), StaticSpellingKind::None,
//      decl->getLoc(), bodyParams, decl->getLoc(),
//      TypeLoc::withoutLoc(elementTy), dc,
//      /*GenericParams=*/nullptr);
//
//   // Build the thunks.
//   AccessorDecl *getterThunk =
//      buildSubscriptGetterDecl(Impl, subscript, getter, elementTy,
//                               dc, getterIndex);
//
//   AccessorDecl *setterThunk = nullptr;
//   if (setter)
//      setterThunk =
//         buildSubscriptSetterDecl(Impl, subscript, setter, elementTy,
//                                  dc, setterIndex);
//
//   // Record the subscript as an alternative declaration.
//   Impl.addAlternateDecl(associateWithSetter ? setter : getter, subscript);
//
//   // Import attributes for the accessors if there is a pair.
//   Impl.importAttributes(getterObjCMethod, getterThunk);
//   if (setterObjCMethod)
//      Impl.importAttributes(setterObjCMethod, setterThunk);
//
//   subscript->setIsSetterMutating(false);
//   makeComputed(subscript, getterThunk, setterThunk);
//
//   Impl.recordImplicitUnwrapForDecl(subscript, isIUO);
//
//   addObjCAttribute(subscript, None);
//
//   // Optional subscripts in protocols.
//   if (optionalMethods && isa<InterfaceDecl>(dc))
//      subscript->getAttrs().add(new (Impl.TypePHPContext) OptionalAttr(true));
//
//   // Note that we've created this subscript.
//   Impl.Subscripts[{getter, setter}] = subscript;
//   if (setter && !Impl.Subscripts[{getter, nullptr}])
//      Impl.Subscripts[{getter, nullptr}] = subscript;
//
//   // Make the getter/setter methods unavailable.
//   if (!getter->getAttrs().isUnavailable(Impl.TypePHPContext))
//      Impl.markUnavailable(getter, "use subscripting");
//   makeSetterUnavailable();
//
//   // Wire up overrides.
//   recordObjCOverride(subscript);
//
//   return subscript;
//}
//
//AccessorDecl *
//TypePHPDeclConverter::importAccessor(clang::ObjCMethodDecl *clangAccessor,
//                                   AbstractStorageDecl *storage,
//                                   AccessorKind accessorKind,
//                                   DeclContext *dc) {
//   TypePHPDeclConverter converter(Impl, getActivePolarphpVersion());
//   auto *accessor = cast_or_null<AccessorDecl>(
//      converter.importObjCMethodDecl(clangAccessor, dc,
//                                     AccessorInfo{storage, accessorKind}));
//   if (!accessor) {
//      return nullptr;
//   }
//
//   Impl.importAttributes(clangAccessor, accessor);
//
//   return accessor;
//}

//void TypePHPDeclConverter::addInterfaces(
//   InterfaceDecl *protocol, SmallVectorImpl<InterfaceDecl *> &protocols,
//   llvm::SmallPtrSetImpl<InterfaceDecl *> &known) {
//   if (!known.insert(protocol).second)
//      return;
//
//   protocols.push_back(protocol);
//   for (auto inherited : protocol->getInheritedInterfaces())
//      addInterfaces(inherited, protocols, known);
//}
//
//void TypePHPDeclConverter::importObjCInterfaces(
//   Decl *decl, const clang::ObjCInterfaceList &clangInterfaces,
//   SmallVectorImpl<TypeLoc> &inheritedTypes) {
//   SmallVector<InterfaceDecl *, 4> protocols;
//   llvm::SmallPtrSet<InterfaceDecl *, 4> knownInterfaces;
//   if (auto nominal = dyn_cast<NominalTypeDecl>(decl)) {
//      nominal->getImplicitInterfaces(protocols);
//      knownInterfaces.insert(protocols.begin(), protocols.end());
//   }
//
//   for (auto cp = clangInterfaces.begin(), cpEnd = clangInterfaces.end();
//        cp != cpEnd; ++cp) {
//      if (auto proto = castIgnoringCompatibilityAlias<InterfaceDecl>(
//         Impl.importDecl(*cp, getActivePolarphpVersion()))) {
//         addInterfaces(proto, protocols, knownInterfaces);
//         inheritedTypes.push_back(TypeLoc::withoutLoc(proto->getDeclaredType()));
//      }
//   }
//
//   addObjCInterfaceConformances(decl, protocols);
//}
//
//void TypePHPDeclConverter::addObjCInterfaceConformances(
//   Decl *decl, ArrayRef<InterfaceDecl *> protocols) {
//   // Nothing to do for protocols.
//   if (isa<InterfaceDecl>(decl)) return;
//
//   Impl.recordImportedInterfaces(decl, protocols);
//
//   if (auto nominal = dyn_cast<NominalTypeDecl>(decl)) {
//      nominal->setConformanceLoader(&Impl, 0);
//   } else {
//      auto ext = cast<ExtensionDecl>(decl);
//      ext->setConformanceLoader(&Impl, 0);
//   }
//}
//
//Optional<GenericParamList *> TypePHPDeclConverter::importObjCGenericParams(
//   const clang::ObjCInterfaceDecl *decl, DeclContext *dc) {
//   auto typeParamList = decl->getTypeParamList();
//   if (!typeParamList) {
//      return nullptr;
//   }
//   if (shouldSuppressGenericParamsImport(Impl.TypePHPContext.LangOpts, decl)) {
//      return nullptr;
//   }
//   assert(typeParamList->size() > 0);
//   SmallVector<GenericTypeParamDecl *, 4> genericParams;
//   for (auto *objcGenericParam : *typeParamList) {
//      auto genericParamDecl = Impl.createDeclWithClangNode<GenericTypeParamDecl>(
//         objcGenericParam, AccessLevel::Public, dc,
//         Impl.TypePHPContext.getIdentifier(objcGenericParam->getName()),
//         Impl.importSourceLoc(objcGenericParam->getLocation()),
//         /*depth*/ 0, /*index*/ genericParams.size());
//      // NOTE: depth is always 0 for ObjC generic type arguments, since only
//      // classes may have generic types in ObjC, and ObjC classes cannot be
//      // nested.
//
//      // Import parameter constraints.
//      SmallVector<TypeLoc, 1> inherited;
//      if (objcGenericParam->hasExplicitBound()) {
//         assert(!objcGenericParam->getUnderlyingType().isNull());
//         auto clangBound = objcGenericParam->getUnderlyingType()
//            ->castAs<clang::ObjCObjectPointerType>();
//         if (clangBound->getInterfaceDecl()) {
//            auto unqualifiedClangBound =
//               clangBound->stripObjCKindOfTypeAndQuals(Impl.getClangAstContext());
//            Type superclassType = Impl.importTypeIgnoreIUO(
//               clang::QualType(unqualifiedClangBound, 0), ImportTypeKind::Abstract,
//               false, Bridgeability::None);
//            if (!superclassType) {
//               return None;
//            }
//            inherited.push_back(TypeLoc::withoutLoc(superclassType));
//         }
//         for (clang::ObjCInterfaceDecl *clangProto : clangBound->quals()) {
//            InterfaceDecl *proto = castIgnoringCompatibilityAlias<InterfaceDecl>(
//               Impl.importDecl(clangProto, getActivePolarphpVersion()));
//            if (!proto) {
//               return None;
//            }
//            inherited.push_back(TypeLoc::withoutLoc(proto->getDeclaredType()));
//         }
//      }
//      if (inherited.empty()) {
//         inherited.push_back(
//            TypeLoc::withoutLoc(Impl.TypePHPContext.getAnyObjectType()));
//      }
//      genericParamDecl->setInherited(Impl.TypePHPContext.AllocateCopy(inherited));
//
//      genericParams.push_back(genericParamDecl);
//   }
//   return GenericParamList::create(
//      Impl.TypePHPContext, Impl.importSourceLoc(typeParamList->getLAngleLoc()),
//      genericParams, Impl.importSourceLoc(typeParamList->getRAngleLoc()));
//}

// @todo
//
//void TypePHPDeclConverter::importMirroredInterfaceMembers(
//   const clang::ObjCContainerDecl *decl, DeclContext *dc,
//   ArrayRef<InterfaceDecl *> protocols, SmallVectorImpl<Decl *> &members,
//   AstContext &Ctx) {
//   assert(dc);
//   const clang::ObjCInterfaceDecl *interfaceDecl = nullptr;
//   const ClangModuleUnit *declModule;
//   const ClangModuleUnit *interfaceModule;
//
//   // 'protocols' is, for some reason, the full recursive expansion of
//   // the protocol hierarchy, so there's no need to recursively descend
//   // into inherited protocols.
//
//   // Try to import only the most specific methods with a particular name.
//   // We use a MapVector to get deterministic iteration order later.
//   llvm::MapVector<clang::Selector, std::vector<MirroredMethodEntry>>
//      methodsByName;
//
//   for (auto proto : protocols) {
//      auto clangProto =
//         cast_or_null<clang::ObjCInterfaceDecl>(proto->getClangDecl());
//      if (!clangProto)
//         continue;
//
//      if (!interfaceDecl) {
//         declModule = Impl.getClangModuleForDecl(decl);
//         if ((interfaceDecl = dyn_cast<clang::ObjCInterfaceDecl>(decl))) {
//            interfaceModule = declModule;
//         } else {
//            auto category = cast<clang::ObjCCategoryDecl>(decl);
//            interfaceDecl = category->getClassInterface();
//            interfaceModule = Impl.getClangModuleForDecl(interfaceDecl);
//         }
//      }
//
//      // Don't import a protocol's members if the superclass already adopts
//      // the protocol, or (for categories) if the class itself adopts it
//      // in its main @interface.
//      if (decl != interfaceDecl)
//         if (classImplementsInterface(interfaceDecl, clangProto, false))
//            continue;
//      if (auto superInterface = interfaceDecl->getSuperClass())
//         if (classImplementsInterface(superInterface, clangProto, true))
//            continue;
//
//      const auto &languageVersion =
//         Impl.TypePHPContext.LangOpts.EffectiveLanguageVersion;
//      for (auto member : proto->getMembers()) {
//         // Skip compatibility stubs; there's no reason to mirror them.
//         if (member->getAttrs().isUnavailableInSwiftVersion(languageVersion))
//            continue;
//
//         if (auto prop = dyn_cast<VarDecl>(member)) {
//            auto objcProp =
//               dyn_cast_or_null<clang::ObjCPropertyDecl>(prop->getClangDecl());
//            if (!objcProp)
//               continue;
//
//            // We can't import a property if there's already a method with this
//            // name. (This also covers other properties with that same name.)
//            // FIXME: We should still mirror the setter as a method if it's
//            // not already there.
//            clang::Selector sel = objcProp->getGetterName();
//            if (interfaceDecl->getInstanceMethod(sel))
//               continue;
//
//            bool inNearbyCategory =
//               std::any_of(interfaceDecl->visible_categories_begin(),
//                           interfaceDecl->visible_categories_end(),
//                           [=](const clang::ObjCCategoryDecl *category) -> bool {
//                              if (category != decl) {
//                                 auto *categoryModule =
//                                    Impl.getClangModuleForDecl(category);
//                                 if (categoryModule != declModule &&
//                                     categoryModule != interfaceModule) {
//                                    return false;
//                                 }
//                              }
//                              return category->getInstanceMethod(sel);
//                           });
//            if (inNearbyCategory)
//               continue;
//
//            if (auto imported =
//               Impl.importMirroredDecl(objcProp, dc, getVersion(), proto)) {
//               members.push_back(imported);
//               // FIXME: We should mirror properties of the root class onto the
//               // metatype.
//            }
//
//            continue;
//         }
//
//         auto afd = dyn_cast<AbstractFunctionDecl>(member);
//         if (!afd)
//            continue;
//
//         if (isa<AccessorDecl>(afd))
//            continue;
//
//         auto objcMethod =
//            dyn_cast_or_null<clang::ObjCMethodDecl>(member->getClangDecl());
//         if (!objcMethod)
//            continue;
//
//         // For now, just remember that we saw this method.
//         methodsByName[objcMethod->getSelector()]
//            .push_back(MirroredMethodEntry{objcMethod, proto});
//      }
//   }
//
//   // Process all the methods, now that we've arranged them by selector.
//   for (auto &mapEntry : methodsByName) {
//      importNonOverriddenMirroredMethods(dc, mapEntry.second, members);
//   }
//}

enum MirrorImportComparison {
   // There's no suppression relationship between the methods.
      NoSuppression,

   // The first method suppresses the second.
      Suppresses,

   // The second method suppresses the first.
      IsSuppressed,
};

// @todo
/// Should the mirror import of the first method be suppressed in favor
/// of the second method?  The methods are known to have the same selector
/// and (because this is mirror-import) to be declared on protocols.
///
/// The algorithm that uses this assumes that it is transitive.
//static bool isMirrorImportSuppressedBy(ClangImporter::Implementation &importer,
//                                       const clang::ObjCMethodDecl *first,
//                                       const clang::ObjCMethodDecl *second) {
//   if (first->isInstanceMethod() != second->isInstanceMethod())
//      return false;
//
//   auto firstProto = cast<clang::ObjCInterfaceDecl>(first->getDeclContext());
//   auto secondProto = cast<clang::ObjCInterfaceDecl>(second->getDeclContext());
//
//   // If the first method's protocol is a super-protocol of the second's,
//   // then the second method overrides the first and we should suppress.
//   // Clang provides a function to check that, phrased in terms of whether
//   // a value of one protocol (the RHS) can be assigned to an l-value of
//   // the other (the LHS).
//   auto &ctx = importer.getClangAstContext();
//   return ctx.InterfaceCompatibleWithInterface(
//      const_cast<clang::ObjCInterfaceDecl*>(firstProto),
//      const_cast<clang::ObjCInterfaceDecl*>(secondProto));
//}
//
///// Compare two methods for mirror-import purposes.
//static MirrorImportComparison
//compareMethodsForMirrorImport(ClangImporter::Implementation &importer,
//                              const clang::ObjCMethodDecl *first,
//                              const clang::ObjCMethodDecl *second) {
//   if (isMirrorImportSuppressedBy(importer, first, second))
//      return IsSuppressed;
//   if (isMirrorImportSuppressedBy(importer, second, first))
//      return Suppresses;
//   return NoSuppression;
//}

/// Mark any methods in the given array that are overridden by this method
/// as suppressed by nulling their entries out.
/// Return true if this method is overridden by any methods in the array.
//static bool suppressOverriddenMethods(ClangImporter::Implementation &importer,
//                                      const clang::ObjCMethodDecl *method,
//                                      MutableArrayRef<MirroredMethodEntry> entries) {
//   assert(method && "method was already suppressed");
//
//   for (auto &entry: entries) {
//      auto otherMethod = entry.first;
//      if (!otherMethod) continue;
//
//      assert(method != otherMethod && "found same method twice?");
//      switch (compareMethodsForMirrorImport(importer, method, otherMethod)) {
//         // If the second method is suppressed, null it out.
//         case Suppresses:
//            entry.first = nullptr;
//            continue;
//
//            // If the first method is suppressed, return immediately.  We should
//            // be able to suppress any following methods.
//         case IsSuppressed:
//            return true;
//
//         case NoSuppression:
//            continue;
//      }
//      llvm_unreachable("bad comparison result");
//   }
//
//   return false;
//}

/// Given a set of methods with the same selector, each taken from a
/// different protocol in the protocol hierarchy of a class into which
/// we want to introduce mirror imports, import only the methods which
/// are not overridden by another method in the set.
///
/// It's possible that we'll end up selecting multiple methods to import
/// here, in the cases where there's no hierarchical relationship between
/// two methods.  The importer already has code to handle this case.
//void TypePHPDeclConverter::importNonOverriddenMirroredMethods(DeclContext *dc,
//                                                            MutableArrayRef<MirroredMethodEntry> entries,
//                                                            SmallVectorImpl<Decl *> &members) {
//   for (size_t i = 0, e = entries.size(); i != e; ++i) {
//      auto objcMethod = entries[i].first;
//
//      // If the method was suppressed by a previous method, ignore it.
//      if (!objcMethod)
//         continue;
//
//      // Compare this method to all the following methods, suppressing any
//      // that it overrides.  If it is overridden by any of them, suppress it
//      // instead; but there's no need to mark that in the array, just continue
//      // on to the next method.
//      if (suppressOverriddenMethods(Impl, objcMethod, entries.slice(i + 1)))
//         continue;
//
//      // Okay, the method wasn't suppressed, import it.
//
//      // When mirroring an initializer, make it designated and required.
//      if (isInitMethod(objcMethod)) {
//         // Import the constructor.
//         if (auto imported = importConstructor(objcMethod, dc, /*implicit=*/true,
//                                               CtorInitializerKind::Designated,
//            /*required=*/true)) {
//            members.push_back(imported);
//         }
//         continue;
//      }
//
//      // Import the method.
//      auto proto = entries[i].second;
//      if (auto imported =
//         Impl.importMirroredDecl(objcMethod, dc, getVersion(), proto)) {
//         members.push_back(imported);
//
//         for (auto alternate : Impl.getAlternateDecls(imported))
//            if (imported->getDeclContext() == alternate->getDeclContext())
//               members.push_back(alternate);
//      }
//   }
//}

//void TypePHPDeclConverter::importInheritedConstructors(
//   ClassDecl *classDecl, SmallVectorImpl<Decl *> &newMembers) {
//   if (!classDecl->hasSuperclass())
//      return;
//
//   auto curObjCClass = cast<clang::ObjCInterfaceDecl>(classDecl->getClangDecl());
//
//   auto inheritConstructors = [&](TinyPtrVector<ValueDecl *> members,
//                                  Optional<CtorInitializerKind> kind) {
//      const auto &languageVersion =
//         Impl.TypePHPContext.LangOpts.EffectiveLanguageVersion;
//
//      for (auto member : members) {
//         auto ctor = dyn_cast<ConstructorDecl>(member);
//         if (!ctor)
//            continue;
//
//         // Don't inherit compatibility stubs.
//         if (ctor->getAttrs().isUnavailableInSwiftVersion(languageVersion))
//            continue;
//
//         // Don't inherit (non-convenience) factory initializers.
//         // Note that convenience factories return instancetype and can be
//         // inherited.
//         switch (ctor->getInitKind()) {
//            case CtorInitializerKind::Factory:
//               continue;
//            case CtorInitializerKind::ConvenienceFactory:
//            case CtorInitializerKind::Convenience:
//            case CtorInitializerKind::Designated:
//               break;
//         }
//
//         auto objcMethod =
//            dyn_cast_or_null<clang::ObjCMethodDecl>(ctor->getClangDecl());
//         if (!objcMethod)
//            continue;
//
//         auto &clangSourceMgr = Impl.getClangAstContext().getSourceManager();
//         clang::PrettyStackTraceDecl trace(objcMethod, clang::SourceLocation(),
//                                           clangSourceMgr,
//                                           "importing (inherited)");
//
//         // If this initializer came from a factory method, inherit
//         // it as an initializer.
//         if (objcMethod->isClassMethod()) {
//            assert(ctor->getInitKind() == CtorInitializerKind::ConvenienceFactory);
//
//            Optional<ImportedName> correctPolarphpName;
//            ImportedName importedName =
//               importFullName(objcMethod, correctPolarphpName);
//            assert(
//               !correctPolarphpName &&
//               "Import inherited initializers never references correctPolarphpName");
//            importedName.setHasCustomName();
//            bool redundant;
//            if (auto newCtor =
//               importConstructor(objcMethod, classDecl,
//                  /*implicit=*/true, ctor->getInitKind(),
//                  /*required=*/false, ctor->getObjCSelector(),
//                                 importedName, objcMethod->parameters(),
//                                 objcMethod->isVariadic(), redundant)) {
//               // If this is a compatibility stub, mark it as such.
//               if (correctPolarphpName)
//                  markAsVariant(newCtor, *correctPolarphpName);
//
//               Impl.importAttributes(objcMethod, newCtor, curObjCClass);
//               newMembers.push_back(newCtor);
//            }
//            continue;
//         }
//
//         // Figure out what kind of constructor this will be.
//         CtorInitializerKind myKind;
//         bool isRequired = false;
//         if (ctor->isRequired()) {
//            // Required initializers are always considered designated.
//            isRequired = true;
//            myKind = CtorInitializerKind::Designated;
//         } else if (kind) {
//            myKind = *kind;
//         } else {
//            myKind = ctor->getInitKind();
//         }
//
//         // Import the constructor into this context.
//         if (auto newCtor =
//            importConstructor(objcMethod, classDecl,
//               /*implicit=*/true, myKind, isRequired)) {
//            Impl.importAttributes(objcMethod, newCtor, curObjCClass);
//            newMembers.push_back(newCtor);
//         }
//      }
//   };
//
//   // The kind of initializer to import. If this class has designated
//   // initializers, everything it inherits is a convenience initializer.
//   Optional<CtorInitializerKind> kind;
//   if (curObjCClass->hasDesignatedInitializers())
//      kind = CtorInitializerKind::Convenience;
//
//
//   // If we have a superclass, import from it.
//   auto superclass = classDecl->getSuperclassDecl();
//   if (auto superclassClangDecl = superclass->getClangDecl()) {
//      if (isa<clang::ObjCInterfaceDecl>(superclassClangDecl)) {
//         inheritConstructors(superclass->lookupDirect(DeclBaseName::createConstructor()),
//                             kind);
//      }
//   }
//}

Decl *ClangImporter::Implementation::importDeclCached(
   const clang::NamedDecl *ClangDecl,
   ImportNameVersion version) {
   auto Known = ImportedDecls.find({ClangDecl->getCanonicalDecl(), version});
   if (Known != ImportedDecls.end())
      return Known->second;

   return nullptr;
}

/// Checks if we don't need to import the typedef itself.  If the typedef
/// should be skipped, returns the underlying declaration that the typedef
/// refers to -- this declaration should be imported instead.
static const clang::TagDecl *
canSkipOverTypedef(ClangImporter::Implementation &Impl,
                   const clang::NamedDecl *D,
                   bool &TypedefIsSuperfluous) {
   // If we have a typedef that refers to a tag type of the same name,
   // skip the typedef and import the tag type directly.

   TypedefIsSuperfluous = false;

   auto *ClangTypedef = dyn_cast<clang::TypedefNameDecl>(D);
   if (!ClangTypedef)
      return nullptr;

   const clang::DeclContext *RedeclContext =
      ClangTypedef->getDeclContext()->getRedeclContext();
   if (!RedeclContext->isTranslationUnit())
      return nullptr;

   clang::QualType UnderlyingType = ClangTypedef->getUnderlyingType();

   // A typedef to a typedef should get imported as a typealias.
   auto *TypedefT = UnderlyingType->getAs<clang::TypedefType>();
   if (TypedefT)
      return nullptr;

   auto *TT = UnderlyingType->getAs<clang::TagType>();
   if (!TT)
      return nullptr;

   clang::TagDecl *UnderlyingDecl = TT->getDecl();
   if (UnderlyingDecl->getDeclContext()->getRedeclContext() != RedeclContext)
      return nullptr;

   if (UnderlyingDecl->getDeclName().isEmpty())
      return UnderlyingDecl;

   auto TypedefName = ClangTypedef->getDeclName();
   auto TagDeclName = UnderlyingDecl->getDeclName();
   if (TypedefName != TagDeclName)
      return nullptr;

   TypedefIsSuperfluous = true;
   return UnderlyingDecl;
}

StringRef ClangImporter::Implementation::
getSwiftNameFromClangName(StringRef replacement) {
   auto &clangSema = getClangSema();

   clang::IdentifierInfo *identifier =
      &clangSema.getASTContext().Idents.get(replacement);
   clang::LookupResult lookupResult(clangSema, identifier,
                                    clang::SourceLocation(),
                                    clang::Sema::LookupOrdinaryName);
   if (!clangSema.LookupName(lookupResult, nullptr))
      return "";

   auto clangDecl = lookupResult.getAsSingle<clang::NamedDecl>();
   if (!clangDecl)
      return "";

   auto importedName = importFullName(clangDecl, CurrentVersion);
   if (!importedName)
      return "";

   llvm::SmallString<64> renamed;
   /// TODO
//   {
//      // Render a swift_name string.
//      llvm::raw_svector_ostream os(renamed);
//      printTypePHPName(importedName, CurrentVersion, /*fullyQualified=*/true, os);
//   }

   return TypePHPContext.AllocateCopy(StringRef(renamed));
}

bool importer::isSpecialUIKitStructZeroProperty(const clang::NamedDecl *decl) {
   // FIXME: Once UIKit removes the "nonswift" availability in their versioned
   // API notes, this workaround can go away.
   auto *constant = dyn_cast<clang::VarDecl>(decl);
   if (!constant)
      return false;

   clang::DeclarationName name = constant->getDeclName();
   const clang::IdentifierInfo *ident = name.getAsIdentifierInfo();
   if (!ident)
      return false;

   return ident->isStr("UIEdgeInsetsZero") || ident->isStr("UIOffsetZero");
}

/// Import Clang attributes as Polarphp attributes.
void ClangImporter::Implementation::importAttributes(
   const clang::NamedDecl *ClangDecl,
   Decl *MappedDecl,
   const clang::ObjCContainerDecl *NewContext)
{
   // Subscripts are special-cased since there isn't a 1:1 mapping
   // from its accessor(s) to the subscript declaration.
   if (isa<SubscriptDecl>(MappedDecl))
      return;

   AstContext &C = TypePHPContext;

   if (auto maybeDefinition = getDefinitionForClangTypeDecl(ClangDecl))
      if (maybeDefinition.getValue())
         ClangDecl = cast<clang::NamedDecl>(maybeDefinition.getValue());

   // Scan through Clang attributes and map them onto Swift
   // equivalents.
   bool AnyUnavailable = MappedDecl->getAttrs().isUnavailable(C);
   for (clang::NamedDecl::attr_iterator AI = ClangDecl->attr_begin(),
           AE = ClangDecl->attr_end(); AI != AE; ++AI) {
      //
      // __attribute__((unavailable))
      //
      // Mapping: @available(*,unavailable)
      //
      if (auto unavailable = dyn_cast<clang::UnavailableAttr>(*AI)) {
         auto Message = unavailable->getMessage();
         auto attr = AvailableAttr::createPlatformAgnostic(C, Message);
         MappedDecl->getAttrs().add(attr);
         AnyUnavailable = true;
         continue;
      }

      //
      // __attribute__((annotate(swift1_unavailable)))
      //
      // Mapping: @available(*, unavailable)
      //
      if (auto unavailable_annot = dyn_cast<clang::AnnotateAttr>(*AI))
         if (unavailable_annot->getAnnotation() == "swift1_unavailable") {
            auto attr = AvailableAttr::createPlatformAgnostic(
               C, "", "", PlatformAgnosticAvailabilityKind::UnavailableInSwift);
            MappedDecl->getAttrs().add(attr);
            AnyUnavailable = true;
            continue;
         }

      //
      // __attribute__((deprecated))
      //
      // Mapping: @available(*,deprecated)
      //
      if (auto deprecated = dyn_cast<clang::DeprecatedAttr>(*AI)) {
         auto Message = deprecated->getMessage();
         auto attr = AvailableAttr::createPlatformAgnostic(C, Message, "",
                                                           PlatformAgnosticAvailabilityKind::Deprecated);
         MappedDecl->getAttrs().add(attr);
         continue;
      }

      // __attribute__((availability))
      //
      if (auto avail = dyn_cast<clang::AvailabilityAttr>(*AI)) {
         StringRef Platform = avail->getPlatform()->getName();

         // Is this our special "availability(swift, unavailable)" attribute?
         if (Platform == "swift") {
            // FIXME: Until Apple gets a chance to update UIKit's API notes, ignore
            // the Swift-unavailability for certain properties.
            if (isSpecialUIKitStructZeroProperty(ClangDecl))
               continue;

            auto replacement = avail->getReplacement();
            StringRef swiftReplacement = "";
            if (!replacement.empty())
               swiftReplacement = getSwiftNameFromClangName(replacement);

            auto attr = AvailableAttr::createPlatformAgnostic(
               C, avail->getMessage(), swiftReplacement,
               PlatformAgnosticAvailabilityKind::UnavailableInSwift);
            MappedDecl->getAttrs().add(attr);
            AnyUnavailable = true;
            continue;
         }

         // Does this availability attribute map to the platform we are
         // currently targeting?
         if (!platformAvailability.isPlatformRelevant(Platform))
            continue;

         auto platformK =
            llvm::StringSwitch<Optional<PlatformKind>>(Platform)
               .Case("ios", PlatformKind::iOS)
               .Case("macos", PlatformKind::OSX)
               .Case("tvos", PlatformKind::tvOS)
               .Case("watchos", PlatformKind::watchOS)
               .Case("ios_app_extension", PlatformKind::iOSApplicationExtension)
               .Case("macos_app_extension",
                     PlatformKind::OSXApplicationExtension)
               .Case("tvos_app_extension",
                     PlatformKind::tvOSApplicationExtension)
               .Case("watchos_app_extension",
                     PlatformKind::watchOSApplicationExtension)
               .Default(None);
         if (!platformK)
            continue;

         // Is this declaration marked platform-agnostically unavailable?
         auto PlatformAgnostic = PlatformAgnosticAvailabilityKind::None;
         if (avail->getUnavailable()) {
            PlatformAgnostic = PlatformAgnosticAvailabilityKind::Unavailable;
            AnyUnavailable = true;
         }

         StringRef message = avail->getMessage();

         llvm::VersionTuple deprecated = avail->getDeprecated();

         if (!deprecated.empty()) {
            if (platformAvailability.treatDeprecatedAsUnavailable(ClangDecl,
                                                                  deprecated)) {
               AnyUnavailable = true;
               PlatformAgnostic = PlatformAgnosticAvailabilityKind::Unavailable;
               if (message.empty())
                  message = platformAvailability.deprecatedAsUnavailableMessage;
            }
         }

         llvm::VersionTuple obsoleted = avail->getObsoleted();
         llvm::VersionTuple introduced = avail->getIntroduced();

         const auto &replacement = avail->getReplacement();

         StringRef swiftReplacement = "";
         if (!replacement.empty())
            swiftReplacement = getSwiftNameFromClangName(replacement);

         auto AvAttr = new (C) AvailableAttr(SourceLoc(), SourceRange(),
                                             platformK.getValue(),
                                             message, swiftReplacement,
                                             introduced,
            /*IntroducedRange=*/SourceRange(),
                                             deprecated,
            /*DeprecatedRange=*/SourceRange(),
                                             obsoleted,
            /*ObsoletedRange=*/SourceRange(),
                                             PlatformAgnostic, /*Implicit=*/false);

         MappedDecl->getAttrs().add(AvAttr);
      }
   }

   // If the declaration is unavailable, we're done.
   if (AnyUnavailable)
      return;

   if (auto ID = dyn_cast<clang::ObjCInterfaceDecl>(ClangDecl)) {
      // Ban NSInvocation.
      if (ID->getName() == "NSInvocation") {
         auto attr = AvailableAttr::createPlatformAgnostic(C, "");
         MappedDecl->getAttrs().add(attr);
         return;
      }

      // @todo
      // Map Clang's swift_objc_members attribute to @objcMembers.
//      if (ID->hasAttr<clang::SwiftObjCMembersAttr>() &&
//          isa<ClassDecl>(MappedDecl)) {
//         if (!MappedDecl->getAttrs().hasAttribute<ObjCMembersAttr>()) {
//            auto attr = new (C) ObjCMembersAttr(/*IsImplicit=*/true);
//            MappedDecl->getAttrs().add(attr);
//         }
//      }

      // Infer @objcMembers on XCTestCase.
//      if (ID->getName() == "XCTestCase") {
//         if (!MappedDecl->getAttrs().hasAttribute<ObjCMembersAttr>()) {
//            auto attr = new (C) ObjCMembersAttr(/*IsImplicit=*/true);
//            MappedDecl->getAttrs().add(attr);
//         }
//      }
   }

   // Ban CFRelease|CFRetain|CFAutorelease(CFTypeRef) as well as custom ones
   // such as CGColorRelease(CGColorRef).
   if (auto FD = dyn_cast<clang::FunctionDecl>(ClangDecl)) {
      if (FD->getNumParams() == 1 &&
          (FD->getName().endswith("Release") ||
           FD->getName().endswith("Retain") ||
           FD->getName().endswith("Autorelease")) /*&&
          !FD->getAttr<clang::SwiftNameAttr>()*/) {
         if (auto t = FD->getParamDecl(0)->getType()->getAs<clang::TypedefType>()){
            if (isCFTypeDecl(t->getDecl())) {
               auto attr = AvailableAttr::createPlatformAgnostic(C,
                                                                 "Core Foundation objects are automatically memory managed");
               MappedDecl->getAttrs().add(attr);
               return;
            }
         }
      }
   }

   // Hack: mark any method named "print" with less than two parameters as
   // warn_unqualified_access.
   if (auto MD = dyn_cast<FuncDecl>(MappedDecl)) {
      if (isPrintLikeMethod(MD->getFullName(), MD->getDeclContext())) {
         // Use a non-implicit attribute so it shows up in the generated
         // interface.
         MD->getAttrs().add(new (C) WarnUnqualifiedAccessAttr(/*implicit*/false));
      }
   }

   // Map __attribute__((warn_unused_result)).
   if (!ClangDecl->hasAttr<clang::WarnUnusedResultAttr>()) {
      if (auto MD = dyn_cast<FuncDecl>(MappedDecl)) {
         if (!MD->getResultInterfaceType()->isVoid()) {
            MD->getAttrs().add(new (C) DiscardableResultAttr(/*implicit*/true));
         }
      }
   }
   // Map __attribute__((const)).
   if (ClangDecl->hasAttr<clang::ConstAttr>()) {
      MappedDecl->getAttrs().add(new (C) EffectsAttr(EffectsKind::ReadNone));
   }
   // Map __attribute__((pure)).
   if (ClangDecl->hasAttr<clang::PureAttr>()) {
      MappedDecl->getAttrs().add(new (C) EffectsAttr(EffectsKind::ReadOnly));
   }
}

Decl *
ClangImporter::Implementation::importDeclImpl(const clang::NamedDecl *ClangDecl,
                                              ImportNameVersion version,
                                              bool &TypedefIsSuperfluous,
                                              bool &HadForwardDeclaration) {
   assert(ClangDecl);

   bool SkippedOverTypedef = false;
   Decl *Result = nullptr;
   if (auto *UnderlyingDecl = canSkipOverTypedef(*this, ClangDecl,
                                                 TypedefIsSuperfluous)) {
      Result = importDecl(UnderlyingDecl, version);
      SkippedOverTypedef = true;
   }

   if (!Result) {
      TypePHPDeclConverter converter(*this, version);
      Result = converter.Visit(ClangDecl);
      HadForwardDeclaration = converter.hadForwardDeclaration();
   }
   if (!Result && version == CurrentVersion) {
      // If we couldn't import this Objective-C entity, determine
      // whether it was a required member of a protocol, or a designated
      // initializer of a class.
      bool hasMissingRequiredMember = false;
      if (auto clangProto
         = dyn_cast<clang::ObjCInterfaceDecl>(ClangDecl->getDeclContext())) {
         if (auto method = dyn_cast<clang::ObjCMethodDecl>(ClangDecl)) {
            if (method->getImplementationControl()
                == clang::ObjCMethodDecl::Required)
               hasMissingRequiredMember = true;
         } else if (auto prop = dyn_cast<clang::ObjCPropertyDecl>(ClangDecl)) {
            if (prop->getPropertyImplementation()
                == clang::ObjCPropertyDecl::Required)
               hasMissingRequiredMember = true;
         }

         if (hasMissingRequiredMember) {
            // Mark the protocol as having missing requirements.
            if (auto proto = castIgnoringCompatibilityAlias<InterfaceDecl>(
               importDecl(clangProto, CurrentVersion))) {
               proto->setHasMissingRequirements(true);
            }
         }
      }
      if (auto method = dyn_cast<clang::ObjCMethodDecl>(ClangDecl)) {
         if (method->isDesignatedInitializerForTheInterface()) {
            const clang::ObjCInterfaceDecl *theClass = method->getClassInterface();
            assert(theClass && "cannot be a protocol method here");
            // Only allow this to affect declarations in the same top-level module
            // as the original class.
            if (getClangModuleForDecl(theClass) == getClangModuleForDecl(method)) {
               if (auto swiftClass = castIgnoringCompatibilityAlias<ClassDecl>(
                  importDecl(theClass, CurrentVersion))) {
                  swiftClass->setHasMissingDesignatedInitializers();
               }
            }
         }
      }

      return nullptr;
   }

   // Finalize the imported declaration.
   auto finalizeDecl = [&](Decl *result) {
      importAttributes(ClangDecl, result);

      // Hack to deal with Objective-C protocols without availability annotation.
      // If the protocol comes from clang and is not annotated and the protocol
      // requirement itself is not annotated, then infer availability of the
      // requirement based on its types. This makes it possible for a type to
      // conform to an Objective-C protocol that is missing annotations but whose
      // requirements use types that are less available than the conforming type.
      auto dc = result->getDeclContext();
      auto *proto = dyn_cast<InterfaceDecl>(dc);
      if (!proto || proto->getAttrs().hasAttribute<AvailableAttr>())
         return;

      inferInterfaceMemberAvailability(*this, dc, result);
   };

   if (Result) {
      finalizeDecl(Result);

      for (auto alternate : getAlternateDecls(Result))
         finalizeDecl(alternate);
   }

#ifndef NDEBUG
   auto Canon = cast<clang::NamedDecl>(ClangDecl->getCanonicalDecl());

   // Note that the decl was imported from Clang.  Don't mark Swift decls as
   // imported.
   if (Result &&
       (!Result->getDeclContext()->isModuleScopeContext() ||
        isa<ClangModuleUnit>(Result->getDeclContext()))) {
      // Either the Swift declaration was from stdlib,
      // or we imported the underlying decl of the typedef,
      // or we imported the decl itself.
      bool ImportedCorrectly =
         !Result->getClangDecl() || SkippedOverTypedef ||
         Result->getClangDecl()->getCanonicalDecl() == Canon;

      // Or the other type is a typedef,
      if (!ImportedCorrectly &&
          isa<clang::TypedefNameDecl>(Result->getClangDecl())) {
         // both types are ValueDecls:
         if (isa<clang::ValueDecl>(Result->getClangDecl())) {
            ImportedCorrectly =
               getClangAstContext().hasSameType(
                  cast<clang::ValueDecl>(Result->getClangDecl())->getType(),
                  cast<clang::ValueDecl>(Canon)->getType());
         } else if (isa<clang::TypeDecl>(Result->getClangDecl())) {
            // both types are TypeDecls:
            ImportedCorrectly =
               getClangAstContext().hasSameUnqualifiedType(
                  getClangAstContext().getTypeDeclType(
                     cast<clang::TypeDecl>(Result->getClangDecl())),
                  getClangAstContext().getTypeDeclType(
                     cast<clang::TypeDecl>(Canon)));
         }
         assert(ImportedCorrectly);
      }
      assert(Result->hasClangNode());
   }
#else
   (void)SkippedOverTypedef;
#endif

   return Result;
}

void ClangImporter::Implementation::startedImportingEntity() {
   ++NumTotalImportedEntities;
   // FIXME: (transitional) increment the redundant "always-on" counter.
   if (TypePHPContext.Stats)
      TypePHPContext.Stats->getFrontendCounters().NumTotalClangImportedEntities++;
}

/// Look up associated type requirements in the conforming type.
static void finishTypeWitnesses(
   NormalInterfaceConformance *conformance) {
   auto *dc = conformance->getDeclContext();
   auto nominal = dc->getSelfNominalTypeDecl();
   auto *module = dc->getParentModule();

   auto *proto = conformance->getInterface();
   auto selfType = conformance->getType();

   for (auto *assocType : proto->getAssociatedTypeMembers()) {
      // FIXME: This should not happen?
      if (conformance->hasTypeWitness(assocType)) continue;

      bool satisfied = false;

      SmallVector<ValueDecl *, 4> lookupResults;
      NLOptions options = (NL_QualifiedDefault |
                           NL_OnlyTypes |
                           NL_InterfaceMembers);

      dc->lookupQualified(nominal, assocType->getFullName(), options,
                          lookupResults);
      for (auto member : lookupResults) {
         auto typeDecl = cast<TypeDecl>(member);
         if (isa<AssociatedTypeDecl>(typeDecl)) continue;

         auto memberType = typeDecl->getDeclaredInterfaceType();
         auto subMap = selfType->getContextSubstitutionMap(
            module, typeDecl->getDeclContext());
         memberType = memberType.subst(subMap);
         conformance->setTypeWitness(assocType, memberType, typeDecl);
         satisfied = true;
         break;
      }

      if (!satisfied) {
         llvm::errs() << ("Cannot look up associated type for "
                          "imported conformance:\n");
         conformance->getType().dump(llvm::errs());
         assocType->dump(llvm::errs());
         abort();
      }
   }
}

/// Create witnesses for requirements not already met.
static void finishMissingOptionalWitnesses(
   NormalInterfaceConformance *conformance) {
   auto *proto = conformance->getInterface();

   for (auto req : proto->getMembers()) {
      auto valueReq = dyn_cast<ValueDecl>(req);
      if (!valueReq)
         continue;

      if (!conformance->hasWitness(valueReq)) {
         if (auto func = dyn_cast<AbstractFunctionDecl>(valueReq)){
            // For an optional requirement, record an empty witness:
            // we'll end up querying this at runtime.
            auto Attrs = func->getAttrs();
            if (Attrs.hasAttribute<OptionalAttr>()) {
               conformance->setWitness(valueReq, Witness());
               continue;
            }
         }

         conformance->setWitness(valueReq, valueReq);
      } else {
         // An initializer that conforms to a requirement is required.
         auto witness = conformance->getWitness(valueReq).getDecl();
         if (auto ctor = dyn_cast_or_null<ConstructorDecl>(witness)) {
            if (!ctor->getAttrs().hasAttribute<RequiredAttr>()) {
               auto &ctx = proto->getAstContext();
               ctor->getAttrs().add(new (ctx) RequiredAttr(/*IsImplicit=*/true));
            }
         }
      }
   }
}

void ClangImporter::Implementation::finishNormalConformance(
   NormalInterfaceConformance *conformance,
   uint64_t unused) {
   (void)unused;

   auto *proto = conformance->getInterface();
   PrettyStackTraceConformance trace(TypePHPContext, "completing import of",
                                     conformance);

   finishTypeWitnesses(conformance);
   conformance->finishSignatureConformances();

   // Imported conformances to @objc protocols also require additional
   // initialization to complete the requirement to witness mapping.
   // @todo
//   if (!proto->isObjC())
//      return;

   assert(conformance->isComplete());
   conformance->setState(InterfaceConformanceState::Incomplete);

   finishMissingOptionalWitnesses(conformance);

   conformance->setState(InterfaceConformanceState::Complete);
}

Decl *ClangImporter::Implementation::importDeclAndCacheImpl(
   const clang::NamedDecl *ClangDecl,
   ImportNameVersion version,
   bool SuperfluousTypedefsAreTransparent) {
   if (!ClangDecl)
      return nullptr;

   FrontendStatsTracer StatsTracer(TypePHPContext.Stats,
                                   "import-clang-decl", ClangDecl);
   clang::PrettyStackTraceDecl trace(ClangDecl, clang::SourceLocation(),
                                     Instance->getSourceManager(), "importing");

   auto Canon = cast<clang::NamedDecl>(ClangDecl->getCanonicalDecl());

   if (auto Known = importDeclCached(Canon, version)) {
      if (!SuperfluousTypedefsAreTransparent &&
          SuperfluousTypedefs.count(Canon))
         return nullptr;
      return Known;
   }

   bool TypedefIsSuperfluous = false;
   bool HadForwardDeclaration = false;

   startedImportingEntity();
   Decl *Result = importDeclImpl(ClangDecl, version, TypedefIsSuperfluous,
                                 HadForwardDeclaration);
   if (!Result)
      return nullptr;

   if (TypedefIsSuperfluous) {
      SuperfluousTypedefs.insert(Canon);
      if (auto tagDecl = dyn_cast_or_null<clang::TagDecl>(Result->getClangDecl()))
         DeclsWithSuperfluousTypedefs.insert(tagDecl);
   }

   if (!HadForwardDeclaration)
      ImportedDecls[{Canon, version}] = Result;

   if (!SuperfluousTypedefsAreTransparent && TypedefIsSuperfluous)
      return nullptr;

   return Result;
}

// @todo
//Decl *
//ClangImporter::Implementation::importMirroredDecl(const clang::NamedDecl *decl,
//                                                  DeclContext *dc,
//                                                  ImportNameVersion version,
//                                                  InterfaceDecl *proto) {
//   assert(dc);
//   if (!decl)
//      return nullptr;
//
//   clang::PrettyStackTraceDecl trace(decl, clang::SourceLocation(),
//                                     Instance->getSourceManager(),
//                                     "importing (mirrored)");
//
//   auto canon = decl->getCanonicalDecl();
//   auto known = ImportedInterfaceDecls.find(std::make_tuple(canon, dc, version));
//   if (known != ImportedInterfaceDecls.end())
//      return known->second;
//
//   TypePHPDeclConverter converter(*this, version);
//   Decl *result;
//   // @todo
////   if (auto method = dyn_cast<clang::ObjCMethodDecl>(decl)) {
////      result = converter.importObjCMethodDecl(method, dc, /*accessor*/None);
////   } else if (auto prop = dyn_cast<clang::ObjCPropertyDecl>(decl)) {
////      result = converter.importObjCPropertyDecl(prop, dc);
////   } else {
////      llvm_unreachable("unexpected mirrored decl");
////   }
//
//   if (result) {
//      assert(result->getClangDecl() && result->getClangDecl() == canon);
//
//      auto updateMirroredDecl = [&](Decl *result) {
//         result->setImplicit();
//
//         // Map the Clang attributes onto Swift attributes.
//         importAttributes(decl, result);
//
//         if (proto->getAttrs().hasAttribute<AvailableAttr>()) {
//            if (!result->getAttrs().hasAttribute<AvailableAttr>()) {
//               AvailabilityContext protoRange =
//                  AvailabilityInference::availableRange(proto, TypePHPContext);
//               applyAvailableAttribute(result, protoRange, TypePHPContext);
//            }
//         } else {
//            // Infer the same availability for the mirrored declaration as
//            // we would for the protocol member it is mirroring.
//            inferInterfaceMemberAvailability(*this, dc, result);
//         }
//      };
//
//      updateMirroredDecl(result);
//
//      // Update the alternate declaration as well.
//      for (auto alternate : getAlternateDecls(result))
//         updateMirroredDecl(alternate);
//   }
//   if (result || !converter.hadForwardDeclaration())
//      ImportedInterfaceDecls[std::make_tuple(canon, dc, version)] = result;
//   return result;
//}

DeclContext *ClangImporter::Implementation::importDeclContextImpl(
   const clang::DeclContext *dc) {
   // Every declaration should come from a module, so we should not see the
   // TranslationUnit DeclContext here.
   assert(!dc->isTranslationUnit());

   auto decl = dyn_cast<clang::NamedDecl>(dc);
   if (!decl)
      return nullptr;

   auto polarphpDecl = importDecl(decl, CurrentVersion);
   if (!polarphpDecl)
      return nullptr;

   if (auto nominal = dynCastIgnoringCompatibilityAlias<NominalTypeDecl>(polarphpDecl))
      return nominal;
   if (auto extension = dyn_cast<ExtensionDecl>(polarphpDecl))
      return extension;
   if (auto constructor = dyn_cast<ConstructorDecl>(polarphpDecl))
      return constructor;
   if (auto destructor = dyn_cast<DestructorDecl>(polarphpDecl))
      return destructor;
   return nullptr;
}

GenericSignature ClangImporter::Implementation::buildGenericSignature(
   GenericParamList *genericParams, DeclContext *dc) {
   SmallVector<GenericTypeParamType *, 2> genericParamTypes;
   for (auto param : *genericParams) {
      genericParamTypes.push_back(
         param->getDeclaredInterfaceType()->castTo<GenericTypeParamType>());
   }

   SmallVector<Requirement, 2> requirements;
   for (auto param : *genericParams) {
      Type paramType = param->getDeclaredInterfaceType();
      for (const auto &inherited : param->getInherited()) {
         Type inheritedType = inherited.getType();
         if (inheritedType->isAnyObject()) {
            requirements.push_back(
               Requirement(
                  RequirementKind::Layout, paramType,
                  LayoutConstraint::getLayoutConstraint(LayoutConstraintKind::Class)));
            continue;
         }
         if (inheritedType->getClassOrBoundGenericClass()) {
            requirements.push_back(
               Requirement(RequirementKind::Superclass, paramType, inheritedType));
            continue;
         }
         assert(inheritedType->isExistentialType());
         requirements.push_back(
            Requirement(RequirementKind::Conformance, paramType, inheritedType));
      }
   }

   return evaluateOrDefault(
      TypePHPContext.evaluator,
      AbstractGenericSignatureRequest{
         nullptr, std::move(genericParamTypes), std::move(requirements)},
      GenericSignature());
}

DeclContext *
ClangImporter::Implementation::importDeclContextOf(
   const clang::Decl *decl,
   EffectiveClangContext context)
{
   DeclContext *importedDC = nullptr;
   switch (context.getKind()) {
      case EffectiveClangContext::DeclContext: {
         auto dc = context.getAsDeclContext();
         if (dc->isTranslationUnit()) {
            if (auto *module = getClangModuleForDecl(decl))
               return module;
            else
               return nullptr;
         }

         // Import the DeclContext.
         importedDC = importDeclContextImpl(dc);
         break;
      }

      case EffectiveClangContext::TypedefContext: {
         // Import the typedef-name as a declaration.
         auto importedDecl = importDecl(context.getTypedefName(), CurrentVersion);
         if (!importedDecl) return nullptr;

         // Dig out the imported DeclContext.
         importedDC = dynCastIgnoringCompatibilityAlias<NominalTypeDecl>(importedDecl);
         break;
      }

      case EffectiveClangContext::UnresolvedContext: {
         // FIXME: Resolve through name lookup. This is brittle.
         auto submodule =
            getClangSubmoduleForDecl(decl, /*allowForwardDeclaration=*/false);
         if (!submodule) return nullptr;

         if (auto lookupTable = findLookupTable(*submodule)) {
            if (auto clangDecl
               = lookupTable->resolveContext(context.getUnresolvedName())) {
               // Import the Clang declaration.
               auto decl = importDecl(clangDecl, CurrentVersion);
               if (!decl) return nullptr;

               // Look through typealiases.
               if (auto typealias = dyn_cast<TypeAliasDecl>(decl))
                  importedDC = typealias->getDeclaredInterfaceType()->getAnyNominal();
               else // Map to a nominal type declaration.
                  importedDC = dyn_cast<NominalTypeDecl>(decl);
               break;
            }
         }
      }
   }

   // If we didn't manage to import the declaration context, we're done.
   if (!importedDC) return nullptr;

   // If the declaration was not global to start with, we're done.
   bool isGlobal =
      decl->getDeclContext()->getRedeclContext()->isTranslationUnit();
   if (!isGlobal) return importedDC;

   // If the resulting declaration context is not a nominal type,
   // we're done.
   auto nominal = dyn_cast<NominalTypeDecl>(importedDC);
   if (!nominal) return importedDC;

   // Look for the extension for the given nominal type within the
   // Clang submodule of the declaration.
   const clang::Module *declSubmodule = *getClangSubmoduleForDecl(decl);
   auto extensionKey = std::make_pair(nominal, declSubmodule);
   auto knownExtension = extensionPoints.find(extensionKey);
   if (knownExtension != extensionPoints.end())
      return knownExtension->second;

   // Create a new extension for this nominal type/Clang submodule pair.
   auto ext = ExtensionDecl::create(TypePHPContext, SourceLoc(), nullptr, {},
                                    getClangModuleForDecl(decl), nullptr);
   TypePHPContext.evaluator.cacheOutput(ExtendedTypeRequest{ext},
                                        nominal->getDeclaredType());
   TypePHPContext.evaluator.cacheOutput(ExtendedNominalRequest{ext},
                                        std::move(nominal));
   ext->setMemberLoader(this, reinterpret_cast<uintptr_t>(declSubmodule));

   if (auto protoDecl = ext->getExtendedInterfaceDecl()) {
      ext->setGenericSignature(protoDecl->getGenericSignature());
   }

   // Add the extension to the nominal type.
   nominal->addExtension(ext);

   // Record this extension so we can find it later.
   extensionPoints[extensionKey] = ext;
   return ext;
}

static Type getConstantLiteralType(ClangImporter::Implementation &Impl,
                                   Type type, ConstantConvertKind convertKind) {
   switch (convertKind) {
      case ConstantConvertKind::Construction:
      case ConstantConvertKind::ConstructionWithUnwrap: {
         auto found = Impl.RawTypes.find(type->getAnyNominal());
         assert(found != Impl.RawTypes.end());
         return found->second;
      }

      default:
         return type;
   }
}

ValueDecl *
ClangImporter::Implementation::createConstant(Identifier name, DeclContext *dc,
                                              Type type,
                                              const clang::APValue &value,
                                              ConstantConvertKind convertKind,
                                              bool isStatic,
                                              ClangNode ClangN) {
   auto &context = TypePHPContext;

   // Create the integer literal value.
   Expr *expr = nullptr;
   switch (value.getKind()) {
      case clang::APValue::AddrLabelDiff:
      case clang::APValue::Array:
      case clang::APValue::ComplexFloat:
      case clang::APValue::ComplexInt:
      case clang::APValue::FixedPoint:
      case clang::APValue::Indeterminate:
      case clang::APValue::LValue:
      case clang::APValue::MemberPointer:
      case clang::APValue::None:
      case clang::APValue::Struct:
      case clang::APValue::Union:
      case clang::APValue::Vector:
         llvm_unreachable("Unhandled APValue kind");

      case clang::APValue::Float:
      case clang::APValue::Int: {
         // Print the value.
         llvm::SmallString<16> printedValueBuf;
         if (value.getKind() == clang::APValue::Int) {
            value.getInt().toString(printedValueBuf);
         } else {
            assert(value.getFloat().isFinite() && "can't handle infinities or NaNs");
            value.getFloat().toString(printedValueBuf);
         }
         StringRef printedValue = printedValueBuf.str();

         // If this was a negative number, record that and strip off the '-'.
         bool isNegative = printedValue.front() == '-';
         if (isNegative)
            printedValue = printedValue.drop_front();

         auto literalType = getConstantLiteralType(*this, type, convertKind);

         // Create the expression node.
         StringRef printedValueCopy(context.AllocateCopy(printedValue));
         if (value.getKind() == clang::APValue::Int) {
            if (type->getCanonicalType()->isBool()) {
               auto *boolExpr =
                  new (context) BooleanLiteralExpr(value.getInt().getBoolValue(),
                                                   SourceLoc(),
                     /**Implicit=*/true);

               boolExpr->setBuiltinInitializer(
                  context.getBoolBuiltinInitDecl());
               boolExpr->setType(literalType);

               expr = boolExpr;
            } else {
               auto *intExpr =
                  new (context) IntegerLiteralExpr(printedValueCopy, SourceLoc(),
                     /*Implicit=*/true);

               auto *intDecl = literalType->getAnyNominal();
               intExpr->setBuiltinInitializer(
                  context.getIntBuiltinInitDecl(intDecl));
               intExpr->setType(literalType);

               expr = intExpr;
            }
         } else {
            auto *floatExpr =
               new (context) FloatLiteralExpr(printedValueCopy, SourceLoc(),
                  /*Implicit=*/true);

            auto maxFloatTypeDecl = context.get_MaxBuiltinFloatTypeDecl();
            floatExpr->setBuiltinType(
               maxFloatTypeDecl->getUnderlyingType());

            auto *floatDecl = literalType->getAnyNominal();
            floatExpr->setBuiltinInitializer(
               context.getFloatBuiltinInitDecl(floatDecl));
            floatExpr->setType(literalType);

            expr = floatExpr;
         }

         if (isNegative)
            cast<NumberLiteralExpr>(expr)->setNegative(SourceLoc());

         break;
      }
   }

   assert(expr);
   return createConstant(name, dc, type, expr, convertKind, isStatic, ClangN);
}


ValueDecl *
ClangImporter::Implementation::createConstant(Identifier name, DeclContext *dc,
                                              Type type, StringRef value,
                                              ConstantConvertKind convertKind,
                                              bool isStatic,
                                              ClangNode ClangN) {
   auto expr = new (TypePHPContext) StringLiteralExpr(value, SourceRange());

   auto literalType = getConstantLiteralType(*this, type, convertKind);
   auto *stringDecl = literalType->getAnyNominal();
   expr->setBuiltinInitializer(
      TypePHPContext.getStringBuiltinInitDecl(stringDecl));
   expr->setType(literalType);

   return createConstant(name, dc, type, expr, convertKind, isStatic, ClangN);
}

namespace {
using ConstantGetterBodyContextData =
llvm::PointerIntPair<Expr *, 2, ConstantConvertKind>;
}

/// Synthesizer callback to synthesize the getter for a constant value.
static std::pair<BraceStmt *, bool>
synthesizeConstantGetterBody(AbstractFunctionDecl *afd, void *voidContext) {
   AstContext &ctx = afd->getAstContext();
   auto func = cast<AccessorDecl>(afd);
   VarDecl *constantVar = cast<VarDecl>(func->getStorage());
   Type type = func->mapTypeIntoContext(constantVar->getValueInterfaceType());

   auto contextData = ConstantGetterBodyContextData::getFromOpaqueValue(
      voidContext);
   Expr *expr = contextData.getPointer();
   ConstantConvertKind convertKind = contextData.getInt();

   // If we need a conversion, add one now.
   switch (convertKind) {
      case ConstantConvertKind::None:
         break;

      case ConstantConvertKind::Construction:
      case ConstantConvertKind::ConstructionWithUnwrap: {
         auto typeRef = TypeExpr::createImplicit(type, ctx);

         // Reference init(rawValue: T)
         ConstructorDecl *init = nullptr;
         DeclName initName = DeclName(ctx, DeclBaseName::createConstructor(),
                                      { ctx.Id_rawValue });
         auto nominal = type->getAnyNominal();
         for (auto found : nominal->lookupDirect(initName)) {
            init = dyn_cast<ConstructorDecl>(found);
            if (init && init->getDeclContext() == nominal)
               break;
         }
         assert(init && "did not find init(rawValue:)");

         auto initTy = init->getInterfaceType()->removeArgumentLabels(1);
         auto declRef =
            new (ctx) DeclRefExpr(init, DeclNameLoc(), /*Implicit=*/true,
                                  AccessSemantics::Ordinary, initTy);

         // (Self) -> ...
         initTy = initTy->castTo<FunctionType>()->getResult();
         auto initRef = new (ctx) DotSyntaxCallExpr(declRef, SourceLoc(),
                                                    typeRef, initTy);
         initRef->setThrows(false);

         // (rawValue: T) -> ...
         initTy = initTy->castTo<FunctionType>()->getResult();

         auto initCall = CallExpr::createImplicit(ctx, initRef, { expr },
                                                  { ctx.Id_rawValue });
         initCall->setType(initTy);
         initCall->setThrows(false);

         expr = initCall;

         // Force unwrap if our init(rawValue:) is failable, which is currently
         // the case with enums.
         if (convertKind == ConstantConvertKind::ConstructionWithUnwrap) {
            initTy = initTy->getOptionalObjectType();
            expr = new (ctx) ForceValueExpr(expr, SourceLoc());
            expr->setType(initTy);
         }

         assert(initTy->isEqual(type));
         break;
      }
   }

   // Create the return statement.
   auto ret = new (ctx) ReturnStmt(SourceLoc(), expr);

   return { BraceStmt::create(ctx, SourceLoc(), AstNode(ret), SourceLoc()),
      /*isTypeChecked=*/true };
}

ValueDecl *
ClangImporter::Implementation::createConstant(Identifier name, DeclContext *dc,
                                              Type type, Expr *valueExpr,
                                              ConstantConvertKind convertKind,
                                              bool isStatic,
                                              ClangNode ClangN) {
   auto &C = TypePHPContext;

   VarDecl *var = nullptr;
   if (ClangN) {
      var = createDeclWithClangNode<VarDecl>(ClangN, AccessLevel::Public,
         /*IsStatic*/isStatic,
                                             VarDecl::Introducer::Var,
         /*IsCaptureList*/false, SourceLoc(),
                                             name, dc);
   } else {
      var = new (TypePHPContext)
         VarDecl(/*IsStatic*/isStatic, VarDecl::Introducer::Var,
         /*IsCaptureList*/false, SourceLoc(), name, dc);
   }

   var->setInterfaceType(type);
   // @todo
//   var->setIsObjC(false);
   var->setIsDynamic(false);

   auto *params = ParameterList::createEmpty(C);

   // Create the getter function declaration.
   auto func = AccessorDecl::create(C,
      /*FuncLoc=*/SourceLoc(),
      /*AccessorKeywordLoc=*/SourceLoc(),
                                    AccessorKind::Get,
                                    var,
      /*StaticLoc=*/SourceLoc(),
                                    StaticSpellingKind::None,
      /*Throws=*/false,
      /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr,
                                    params,
                                    TypeLoc::withoutLoc(type), dc);
   func->setStatic(isStatic);
   func->setAccess(getOverridableAccessLevel(dc));
   // @todo
//   func->setIsObjC(false);
   func->setIsDynamic(false);

   func->setBodySynthesizer(synthesizeConstantGetterBody,
                            ConstantGetterBodyContextData(valueExpr, convertKind)
                               .getOpaqueValue());

   // Mark the function transparent so that we inline it away completely.
   func->getAttrs().add(new (C) TransparentAttr(/*implicit*/ true));

   // Set the function up as the getter.
   makeComputed(var, func, nullptr);

   return var;
}

/// Create a decl with error type and an "unavailable" attribute on it
/// with the specified message.
void ClangImporter::Implementation::
markUnavailable(ValueDecl *decl, StringRef unavailabilityMsgRef) {

   unavailabilityMsgRef = TypePHPContext.AllocateCopy(unavailabilityMsgRef);
   auto ua = AvailableAttr::createPlatformAgnostic(TypePHPContext,
                                                   unavailabilityMsgRef);
   decl->getAttrs().add(ua);
}

/// Create a decl with error type and an "unavailable" attribute on it
/// with the specified message.
ValueDecl *ClangImporter::Implementation::
createUnavailableDecl(Identifier name, DeclContext *dc, Type type,
                      StringRef UnavailableMessage, bool isStatic,
                      ClangNode ClangN) {

   // Create a new VarDecl with dummy type.
   auto var = createDeclWithClangNode<VarDecl>(ClangN, AccessLevel::Public,
      /*IsStatic*/isStatic,
                                               VarDecl::Introducer::Var,
      /*IsCaptureList*/false,
                                               SourceLoc(), name, dc);
   // @todo
//   var->setIsObjC(false);
   var->setIsDynamic(false);
   var->setInterfaceType(type);
   markUnavailable(var, UnavailableMessage);

   return var;
}


void
ClangImporter::Implementation::loadAllMembers(Decl *D, uint64_t extra) {

   FrontendStatsTracer tracer(D->getAstContext().Stats, "load-all-members", D);
   assert(D);

   // Check whether we're importing an Objective-C container of some sort.
   auto objcContainer =
      dyn_cast_or_null<clang::ObjCContainerDecl>(D->getClangDecl());

   // If not, we're importing globals-as-members into an extension.
   if (objcContainer) {
      loadAllMembersOfObjcContainer(D, objcContainer);
      return;
   }

   auto namespaceDecl =
      dyn_cast_or_null<clang::NamespaceDecl>(D->getClangDecl());
   if (namespaceDecl) {
      auto *enumDecl = cast<EnumDecl>(D);
      // TODO: This redecls should only match redecls that are in the same
      // module as namespaceDecl after we import one namespace per clang module.
      for (auto ns : namespaceDecl->redecls()) {
         for (auto m : ns->decls()) {
            auto nd = dyn_cast<clang::NamedDecl>(m);
            if (!nd)
               continue;
            auto member = importDecl(nd, CurrentVersion);
            if (!member)
               continue;

            enumDecl->addMember(member);
         }
      }
      return;
   }

   loadAllMembersIntoExtension(D, extra);
}

void ClangImporter::Implementation::loadAllMembersIntoExtension(
   Decl *D, uint64_t extra) {
   // We have extension.
   auto ext = cast<ExtensionDecl>(D);
   auto nominal = ext->getExtendedNominal();

   // The submodule of the extension is encoded in the extra data.
   clang::Module *submodule =
      reinterpret_cast<clang::Module *>(static_cast<uintptr_t>(extra));

   // Find the lookup table.
   auto topLevelModule = submodule;
   if (topLevelModule)
      topLevelModule = topLevelModule->getTopLevelModule();
   auto table = findLookupTable(topLevelModule);
   if (!table)
      return;

   PrettyStackTraceStringAction trace(
      "loading import-as-members from",
      topLevelModule ? topLevelModule->getTopLevelModuleName()
                     : "(bridging header)");
   PrettyStackTraceDecl trace2("...for", nominal);

   // Dig out the effective Clang context for this nominal type.
   auto effectiveClangContext = getEffectiveClangContext(nominal);
   if (!effectiveClangContext)
      return;

   // Get ready to actually load the members.
   startedImportingEntity();

   // Load the members.
   for (auto entry : table->lookupGlobalsAsMembers(effectiveClangContext)) {
      auto decl = entry.get<clang::NamedDecl *>();

      // Only include members in the same submodule as this extension.
      if (getClangSubmoduleForDecl(decl) != submodule)
         continue;

      forEachDistinctName(
         decl, [&](ImportedName newName, ImportNameVersion nameVersion) -> bool {
            return addMemberAndAlternatesToExtension(decl, newName, nameVersion, ext);
         });
   }
}

static Decl *findMemberThatWillLandInAnExtensionContext(Decl *member) {
   Decl *result = member;
   while (!isa<ExtensionDecl>(result->getDeclContext())) {
      auto nominal = dyn_cast<NominalTypeDecl>(result->getDeclContext());
      if (!nominal)
         return nullptr;

      result = nominal;
      if (result->hasClangNode())
         return nullptr;
   }
   return result;
}

bool ClangImporter::Implementation::addMemberAndAlternatesToExtension(
   clang::NamedDecl *decl, ImportedName newName, ImportNameVersion nameVersion,
   ExtensionDecl *ext) {
   // Quickly check the context and bail out if it obviously doesn't
   // belong here.
   if (auto *importDC = newName.getEffectiveContext().getAsDeclContext())
      if (importDC->isFileContext())
         return true;

   // Then try to import the decl under the specified name.
   auto *member = importDecl(decl, nameVersion);
   if (!member)
      return false;

   member = findMemberThatWillLandInAnExtensionContext(member);
   if (!member || member->getDeclContext() != ext)
      return true;
   if (!isa<AccessorDecl>(member))
      ext->addMember(member);

   for (auto alternate : getAlternateDecls(member)) {
      if (alternate->getDeclContext() == ext)
         if (!isa<AccessorDecl>(alternate))
            ext->addMember(alternate);
   }
   return true;
}

static ExtensionDecl *
figureOutTheDeclarationContextToImportInto(Decl *D, DeclContext *&DC,
                                           IterableDeclContext *&IDC) {
   if (auto *nominal = dyn_cast<NominalTypeDecl>(D)) {
      DC = nominal;
      IDC = nominal;
      return nullptr;
   }
   ExtensionDecl *ext = cast<ExtensionDecl>(D);
   DC = ext;
   IDC = ext;
   return ext;
}

static void loadMembersOfBaseImportedFromClang(ExtensionDecl *ext) {
   const NominalTypeDecl *base = ext->getExtendedNominal();
   auto *clangBase = base->getClangDecl();
   if (!clangBase)
      return;
   base->loadAllMembers();

   // Sanity check: make sure we don't jump over to a category /while/
   // loading the original class's members. Right now we only check if this
   // happens on the first member.
   if (auto *clangContainer = dyn_cast<clang::ObjCContainerDecl>(clangBase))
      assert((clangContainer->decls_empty() || !base->getMembers().empty()) &&
             "can't load extension members before base has finished");
}

void ClangImporter::Implementation::loadAllMembersOfObjcContainer(
   Decl *D, const clang::ObjCContainerDecl *objcContainer) {
   clang::PrettyStackTraceDecl trace(objcContainer, clang::SourceLocation(),
                                     Instance->getSourceManager(),
                                     "loading members for");

   DeclContext *DC;
   IterableDeclContext *IDC;
   if (ExtensionDecl *ext =
      figureOutTheDeclarationContextToImportInto(D, DC, IDC)) {
      // If the base is also imported from Clang, load its members first.
      loadMembersOfBaseImportedFromClang(ext);
   }

   startedImportingEntity();

   SmallVector<Decl *, 16> members;
   collectMembersToAdd(objcContainer, D, DC, members);

   for (auto member : members) {
      if (!isa<AccessorDecl>(member))
         IDC->addMember(member);
   }
}

void ClangImporter::Implementation::insertMembersAndAlternates(
   const clang::NamedDecl *nd, SmallVectorImpl<Decl *> &members) {
   llvm::SmallPtrSet<Decl *, 4> knownAlternateMembers;
   forEachDistinctName(
      nd, [&](ImportedName name, ImportNameVersion nameVersion) -> bool {
         auto member = importDecl(nd, nameVersion);
         if (!member)
            return false;

         // If there are alternate declarations for this member, add them.
         for (auto alternate : getAlternateDecls(member)) {
            if (alternate->getDeclContext() == member->getDeclContext() &&
                knownAlternateMembers.insert(alternate).second) {
               members.push_back(alternate);
            }
         }

         // If this declaration shouldn't be visible, don't add it to
         // the list.
         if (shouldSuppressDeclImport(nd))
            return true;

         members.push_back(member);
         return true;
      });
}

void ClangImporter::Implementation::collectMembersToAdd(
   const clang::ObjCContainerDecl *objcContainer, Decl *D, DeclContext *DC,
   SmallVectorImpl<Decl *> &members) {
   for (const clang::Decl *m : objcContainer->decls()) {
      auto nd = dyn_cast<clang::NamedDecl>(m);
      if (nd && nd == nd->getCanonicalDecl() &&
          nd->getDeclContext() == objcContainer)
         insertMembersAndAlternates(nd, members);
   }

   TypePHPDeclConverter converter(*this, CurrentVersion);

   auto protos = getImportedInterfaces(D);
   if (auto clangClass = dyn_cast<clang::ObjCInterfaceDecl>(objcContainer)) {
      auto swiftClass = cast<ClassDecl>(D);
      objcContainer = clangClass = clangClass->getDefinition();

      // Imported inherited initializers.
//      if (clangClass->getName() != "Interface") {
//         converter.importInheritedConstructors(const_cast<ClassDecl *>(swiftClass),
//                                               members);
//      }

   } else if (auto clangProto
      = dyn_cast<clang::ObjCInterfaceDecl>(objcContainer)) {
      objcContainer = clangProto->getDefinition();
   }
   // Import mirrored declarations for protocols to which this category
   // or extension conforms.
   // FIXME: This is supposed to be a short-term hack.
   // @todo
//   converter.importMirroredInterfaceMembers(objcContainer, DC,
//                                            protos, members, TypePHPContext);
}

void ClangImporter::Implementation::loadAllConformances(
   const Decl *decl, uint64_t contextData,
   SmallVectorImpl<InterfaceConformance *> &Conformances) {
   auto dc = decl->getInnermostDeclContext();

   // Synthesize trivial conformances for each of the protocols.
   for (auto *protocol : getImportedInterfaces(decl)) {
      // FIXME: Build a superclass conformance if the superclass
      // conforms.
      auto conformance = TypePHPContext.getConformance(
         dc->getDeclaredInterfaceType(),
         protocol, SourceLoc(), dc,
         InterfaceConformanceState::Incomplete);
      conformance->setLazyLoader(this, /*context*/0);
      conformance->setState(InterfaceConformanceState::Complete);
      Conformances.push_back(conformance);
   }
}

Optional<MappedTypeNameKind>
ClangImporter::Implementation::getSpecialTypedefKind(clang::TypedefNameDecl *decl) {
   auto iter = SpecialTypedefNames.find(decl->getCanonicalDecl());
   if (iter == SpecialTypedefNames.end())
      return None;
   return iter->second;
}

Identifier
ClangImporter::getEnumConstantName(const clang::EnumConstantDecl *enumConstant){
   return Impl.importFullName(enumConstant, Impl.CurrentVersion)
      .getDeclName()
      .getBaseIdentifier();
}

// See swift/Basic/Statistic.h for declaration: this enables tracing
// clang::Decls, is defined here to avoid too much layering violation / circular
// linkage dependency.

struct ClangDeclTraceFormatter : public UnifiedStatsReporter::TraceFormatter {
   void traceName(const void *Entity, raw_ostream &OS) const {
      if (!Entity)
         return;
      const clang::Decl *CD = static_cast<const clang::Decl *>(Entity);
      if (auto const *ND = dyn_cast<const clang::NamedDecl>(CD)) {
         ND->printName(OS);
      } else {
         OS << "<unnamed-clang-decl>";
      }
   }

   static inline bool printClangShortLoc(raw_ostream &OS,
                                         clang::SourceManager *CSM,
                                         clang::SourceLocation L) {
      if (!L.isValid() || !L.isFileID())
         return false;
      auto PLoc = CSM->getPresumedLoc(L);
      OS << llvm::sys::path::filename(PLoc.getFilename()) << ':' << PLoc.getLine()
         << ':' << PLoc.getColumn();
      return true;
   }

   void traceLoc(const void *Entity, SourceManager *SM,
                 clang::SourceManager *CSM, raw_ostream &OS) const {
      if (!Entity)
         return;
      if (CSM) {
         const clang::Decl *CD = static_cast<const clang::Decl *>(Entity);
         auto Range = CD->getSourceRange();
         if (printClangShortLoc(OS, CSM, Range.getBegin()))
            OS << '-';
         printClangShortLoc(OS, CSM, Range.getEnd());
      }
   }
};

static ClangDeclTraceFormatter TF;

template<>
const UnifiedStatsReporter::TraceFormatter*
FrontendStatsTracer::getTraceFormatter<const clang::Decl *>() {
   return &TF;
}
