//===--- DerivedConformances.cpp - Derived conformance utilities ----------===//
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

#include "polarphp/sema/internal/TypeChecker.h"
#include "polarphp/sema/internal/DerivedConformances.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/Stmt.h"
#include "polarphp/ast/Expr.h"
#include "polarphp/ast/Pattern.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/ast/SourceFile.h"
#include "polarphp/ast/Types.h"
#include "polarphp/clangimporter/ClangModule.h"

using namespace polar;

DerivedConformance::DerivedConformance(AstContext &ctx, Decl *conformanceDecl,
                                       NominalTypeDecl *nominal,
                                       InterfaceDecl *protocol)
   : Context(ctx), ConformanceDecl(conformanceDecl), Nominal(nominal),
     Interface(protocol) {
   assert(getConformanceContext()->getSelfNominalTypeDecl() == nominal);
}

DeclContext *DerivedConformance::getConformanceContext() const {
   return cast<DeclContext>(ConformanceDecl);
}

void DerivedConformance::addMembersToConformanceContext(
   ArrayRef<Decl *> children) {
   auto IDC = cast<IterableDeclContext>(ConformanceDecl);
   auto *SF = ConformanceDecl->getDeclContext()->getParentSourceFile();
   for (auto child : children) {
      IDC->addMember(child);
      if (SF)
         SF->SynthesizedDecls.push_back(child);
   }
}

Type DerivedConformance::getInterfaceType() const {
   return Interface->getDeclaredType();
}

bool DerivedConformance::derivesInterfaceConformance(DeclContext *DC,
                                                     NominalTypeDecl *Nominal,
                                                     InterfaceDecl *Interface) {
   // Only known protocols can be derived.
   auto knownInterface = Interface->getKnownInterfaceKind();
   if (!knownInterface)
      return false;

   if (*knownInterface == KnownInterfaceKind::Hashable) {
      // We can always complete a partial Hashable implementation, and we can
      // synthesize a full Hashable implementation for structs and enums with
      // Hashable components.
      return canDeriveHashable(Nominal);
   }

   if (auto *enumDecl = dyn_cast<EnumDecl>(Nominal)) {
      switch (*knownInterface) {
         // The presence of a raw type is an explicit declaration that
         // the compiler should derive a RawRepresentable conformance.
         case KnownInterfaceKind::RawRepresentable:
            return enumDecl->hasRawType();

            // Enums without associated values can implicitly derive Equatable
            // conformance.
         case KnownInterfaceKind::Equatable:
            return canDeriveEquatable(DC, Nominal);

            // "Simple" enums without availability attributes can explicitly derive
            // a CaseIterable conformance.
            //
            // FIXME: Lift the availability restriction.
         case KnownInterfaceKind::CaseIterable:
            return !enumDecl->hasPotentiallyUnavailableCaseValue()
                   && enumDecl->hasOnlyCasesWithoutAssociatedValues();

            // @objc enums can explicitly derive their _BridgedNSError conformance.
            // @todo
//         case KnownInterfaceKind::BridgedNSError:
//            return enumDecl->isObjC() && enumDecl->hasCases()
//                   && enumDecl->hasOnlyCasesWithoutAssociatedValues();
//
//            // Enums without associated values and enums with a raw type of String
//            // or Int can explicitly derive CodingKey conformance.
         case KnownInterfaceKind::CodingKey: {
            Type rawType = enumDecl->getRawType();
            if (rawType) {
               auto parentDC = enumDecl->getDeclContext();
               AstContext &C = parentDC->getAstContext();

               auto nominal = rawType->getAnyNominal();
               return nominal == C.getStringDecl() || nominal == C.getIntDecl();
            }

            // hasOnlyCasesWithoutAssociatedValues will return true for empty enums;
            // empty enums are allowed to conform as well.
            return enumDecl->hasOnlyCasesWithoutAssociatedValues();
         }

         default:
            return false;
      }
   } else if (isa<StructDecl>(Nominal) || isa<ClassDecl>(Nominal)) {
      // Structs and classes can explicitly derive Encodable and Decodable
      // conformance (explicitly meaning we can synthesize an implementation if
      // a type conforms manually).
      if (*knownInterface == KnownInterfaceKind::Encodable ||
          *knownInterface == KnownInterfaceKind::Decodable) {
         // FIXME: This is not actually correct. We cannot promise to always
         // provide a witness here for all structs and classes. Unfortunately,
         // figuring out whether this is actually possible requires much more
         // context -- a TypeChecker and the parent decl context at least -- and is
         // tightly coupled to the logic within DerivedConformance.
         // This unfortunately means that we expect a witness even if one will not
         // be produced, which requires DerivedConformance::deriveCodable to output
         // its own diagnostics.
         return true;
      }

      // Structs can explicitly derive Equatable conformance.
      if (isa<StructDecl>(Nominal)) {
         switch (*knownInterface) {
            case KnownInterfaceKind::Equatable:
               return canDeriveEquatable(DC, Nominal);
            default:
               return false;
         }
      }
   }
   return false;
}

void DerivedConformance::tryDiagnoseFailedDerivation(DeclContext *DC,
                                                     NominalTypeDecl *nominal,
                                                     InterfaceDecl *protocol) {
   auto knownInterface = protocol->getKnownInterfaceKind();
   if (!knownInterface)
      return;

   if (*knownInterface == KnownInterfaceKind::Equatable) {
      tryDiagnoseFailedEquatableDerivation(DC, nominal);
   }

   if (*knownInterface == KnownInterfaceKind::Hashable) {
      tryDiagnoseFailedHashableDerivation(DC, nominal);
   }
}

ValueDecl *DerivedConformance::getDerivableRequirement(NominalTypeDecl *nominal,
                                                       ValueDecl *requirement) {
   // Note: whenever you update this function, also update
   // TypeChecker::deriveInterfaceRequirement.
   AstContext &ctx = nominal->getAstContext();
   auto name = requirement->getFullName();

   // Local function that retrieves the requirement with the same name as
   // the provided requirement, but within the given known protocol.
   auto getRequirement = [&](KnownInterfaceKind kind) -> ValueDecl * {
      // Dig out the protocol.
      auto proto = ctx.getInterface(kind);
      if (!proto) return nullptr;

      auto conformance = TypeChecker::conformsToInterface(
         nominal->getDeclaredInterfaceType(), proto, nominal,
         ConformanceCheckFlags::SkipConditionalRequirements);
      if (conformance) {
         auto DC = conformance.getConcrete()->getDeclContext();
         // Check whether this nominal type derives conformances to the protocol.
         if (!DerivedConformance::derivesInterfaceConformance(DC, nominal, proto))
            return nullptr;
      }

      // Retrieve the requirement.
      return proto->getSingleRequirement(name);
   };

   // Properties.
   if (isa<VarDecl>(requirement)) {
      // RawRepresentable.rawValue
      if (name.isSimpleName(ctx.Id_rawValue))
         return getRequirement(KnownInterfaceKind::RawRepresentable);

      // Hashable.hashValue
      if (name.isSimpleName(ctx.Id_hashValue))
         return getRequirement(KnownInterfaceKind::Hashable);

      // CaseIterable.allValues
      if (name.isSimpleName(ctx.Id_allCases))
         return getRequirement(KnownInterfaceKind::CaseIterable);

      // _BridgedNSError._nsErrorDomain
      if (name.isSimpleName(ctx.Id_nsErrorDomain))
         return getRequirement(KnownInterfaceKind::BridgedNSError);

      // CodingKey.stringValue
      if (name.isSimpleName(ctx.Id_stringValue))
         return getRequirement(KnownInterfaceKind::CodingKey);

      // CodingKey.intValue
      if (name.isSimpleName(ctx.Id_intValue))
         return getRequirement(KnownInterfaceKind::CodingKey);

      return nullptr;
   }

   // Functions.
   if (auto func = dyn_cast<FuncDecl>(requirement)) {
      if (func->isOperator() && name.getBaseName() == "==")
         return getRequirement(KnownInterfaceKind::Equatable);

      // Encodable.encode(to: Encoder)
      if (name.isCompoundName() && name.getBaseName() == ctx.Id_encode) {
         auto argumentNames = name.getArgumentNames();
         if (argumentNames.size() == 1 && argumentNames[0] == ctx.Id_to)
            return getRequirement(KnownInterfaceKind::Encodable);
      }

      // Hashable.hash(into: inout Hasher)
      if (name.isCompoundName() && name.getBaseName() == ctx.Id_hash) {
         auto argumentNames = name.getArgumentNames();
         if (argumentNames.size() == 1 && argumentNames[0] == ctx.Id_into)
            return getRequirement(KnownInterfaceKind::Hashable);
      }

      return nullptr;
   }

   // Initializers.
   if (auto ctor = dyn_cast<ConstructorDecl>(requirement)) {
      auto argumentNames = name.getArgumentNames();
      if (argumentNames.size() == 1) {
         if (argumentNames[0] == ctx.Id_rawValue)
            return getRequirement(KnownInterfaceKind::RawRepresentable);

         // CodingKey.init?(stringValue:), CodingKey.init?(intValue:)
         if (ctor->isFailable() &&
             !ctor->isImplicitlyUnwrappedOptional() &&
             (argumentNames[0] == ctx.Id_stringValue ||
              argumentNames[0] == ctx.Id_intValue))
            return getRequirement(KnownInterfaceKind::CodingKey);

         // Decodable.init(from: Decoder)
         if (argumentNames[0] == ctx.Id_from)
            return getRequirement(KnownInterfaceKind::Decodable);
      }

      return nullptr;
   }

   // Associated types.
   if (isa<AssociatedTypeDecl>(requirement)) {
      // RawRepresentable.RawValue
      if (name.isSimpleName(ctx.Id_RawValue))
         return getRequirement(KnownInterfaceKind::RawRepresentable);

      // CaseIterable.AllCases
      if (name.isSimpleName(ctx.Id_AllCases))
         return getRequirement(KnownInterfaceKind::CaseIterable);

      return nullptr;
   }

   return nullptr;
}

DeclRefExpr *
DerivedConformance::createSelfDeclRef(AbstractFunctionDecl *fn) {
   AstContext &C = fn->getAstContext();

   auto selfDecl = fn->getImplicitSelfDecl();
   return new (C) DeclRefExpr(selfDecl, DeclNameLoc(), /*implicit*/true);
}

AccessorDecl *DerivedConformance::
addGetterToReadOnlyDerivedProperty(VarDecl *property,
                                   Type propertyContextType) {
   auto getter =
      declareDerivedPropertyGetter(property, propertyContextType);

   property->setImplInfo(StorageImplInfo::getImmutableComputed());
   property->setAccessors(SourceLoc(), {getter}, SourceLoc());

   return getter;
}

AccessorDecl *
DerivedConformance::declareDerivedPropertyGetter(VarDecl *property,
                                                 Type propertyContextType) {
   auto &C = property->getAstContext();
   auto parentDC = property->getDeclContext();
   ParameterList *params = ParameterList::createEmpty(C);

   Type propertyInterfaceType = property->getInterfaceType();

   auto getterDecl = AccessorDecl::create(C,
      /*FuncLoc=*/SourceLoc(), /*AccessorKeywordLoc=*/SourceLoc(),
                                          AccessorKind::Get, property,
      /*StaticLoc=*/SourceLoc(), StaticSpellingKind::None,
      /*Throws=*/false, /*ThrowsLoc=*/SourceLoc(),
      /*GenericParams=*/nullptr, params,
                                          TypeLoc::withoutLoc(propertyInterfaceType), parentDC);
   getterDecl->setImplicit();
   getterDecl->setIsTransparent(false);

   getterDecl->copyFormalAccessFrom(property);


   return getterDecl;
}

std::pair<VarDecl *, PatternBindingDecl *>
DerivedConformance::declareDerivedProperty(Identifier name,
                                           Type propertyInterfaceType,
                                           Type propertyContextType,
                                           bool isStatic, bool isFinal) {
   auto parentDC = getConformanceContext();

   VarDecl *propDecl = new (Context)
      VarDecl(/*IsStatic*/ isStatic, VarDecl::Introducer::Var,
      /*IsCaptureList*/ false, SourceLoc(), name, parentDC);
   propDecl->setImplicit();
   propDecl->copyFormalAccessFrom(Nominal, /*sourceIsParentContext*/ true);
   propDecl->setInterfaceType(propertyInterfaceType);

   Pattern *propPat = new (Context) NamedPattern(propDecl, /*implicit*/ true);
   propPat->setType(propertyContextType);

   propPat = TypedPattern::createImplicit(Context, propPat, propertyContextType);
   propPat->setType(propertyContextType);

   auto *pbDecl = PatternBindingDecl::createImplicit(
      Context, StaticSpellingKind::None, propPat, /*InitExpr*/ nullptr,
      parentDC);
   return {propDecl, pbDecl};
}

bool DerivedConformance::checkAndDiagnoseDisallowedContext(
   ValueDecl *synthesizing) const {
   // In general, conformances can't be synthesized in extensions across files;
   // but we have to allow it as a special case for Equatable and Hashable on
   // enums with no associated values to preserve source compatibility.
   bool allowCrossfileExtensions = false;
   if (Interface->isSpecificInterface(KnownInterfaceKind::Equatable) ||
       Interface->isSpecificInterface(KnownInterfaceKind::Hashable)) {
      auto ED = dyn_cast<EnumDecl>(Nominal);
      allowCrossfileExtensions = ED && ED->hasOnlyCasesWithoutAssociatedValues();
   }

   if (!allowCrossfileExtensions &&
       Nominal->getModuleScopeContext() !=
       getConformanceContext()->getModuleScopeContext()) {
      ConformanceDecl->diagnose(diag::cannot_synthesize_in_crossfile_extension,
                                getInterfaceType());
      Nominal->diagnose(diag::kind_declared_here, DescriptiveDeclKind::Type);
      return true;
   }

   // A non-final class can't have an protocol-witnesss initializer in an
   // extension.
   if (auto CD = dyn_cast<ClassDecl>(Nominal)) {
      if (!CD->isFinal() && isa<ConstructorDecl>(synthesizing) &&
          isa<ExtensionDecl>(ConformanceDecl)) {
         ConformanceDecl->diagnose(
            diag::cannot_synthesize_init_in_extension_of_nonfinal,
            getInterfaceType(), synthesizing->getFullName());
         return true;
      }
   }

   return false;
}
