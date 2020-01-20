//===------- ModuleInterfaceSupport.cpp - swiftinterface files ------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/AstPrinter.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/ast/ExistentialLayout.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/frontend/Frontend.h"
#include "polarphp/frontend/ModuleInterfaceSupport.h"
#include "polarphp/frontend/PrintingDiagnosticConsumer.h"
//#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/serialization/SerializationOptions.h"
#include "polarphp/serialization/Validation.h"

#include "clang/Basic/Module.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Regex.h"

using namespace polar;

version::Version polar::InterfaceFormatVersion({1, 0});

/// Diagnose any scoped imports in \p imports, i.e. those with a non-empty
/// access path. These are not yet supported by module interfaces, since the
/// information about the declaration kind is not preserved through the binary
/// serialization that happens as an intermediate step in non-whole-module
/// builds.
///
/// These come from declarations like `import class FooKit.MainFooController`.
static void diagnoseScopedImports(DiagnosticEngine &diags,
                                  ArrayRef<ModuleDecl::ImportedModule> imports){
   for (const ModuleDecl::ImportedModule &importPair : imports) {
      if (importPair.first.empty())
         continue;
      diags.diagnose(importPair.first.front().second,
                     diag::module_interface_scoped_import_unsupported);
   }
}

/// Prints to \p out a comment containing a format version number, tool version
/// string as well as any relevant command-line flags in \p Opts used to
/// construct \p M.
static void printToolVersionAndFlagsComment(raw_ostream &out,
                                            ModuleInterfaceOptions const &Opts,
                                            ModuleDecl *M) {
   auto &Ctx = M->getAstContext();
   auto ToolsVersion = polar::version::retrieve_polarphp_full_version(
      Ctx.LangOpts.EffectiveLanguageVersion);
   out << "// " POLAR_INTERFACE_FORMAT_VERSION_KEY ": "
       << InterfaceFormatVersion << "\n";
   out << "// " POLAR_COMPILER_VERSION_KEY ": "
       << ToolsVersion << "\n";
   out << "// " POLAR_MODULE_FLAGS_KEY ": "
       << Opts.Flags << "\n";
}

llvm::Regex polar::getPHPInterfaceFormatVersionRegex() {
   return llvm::Regex("^// " POLAR_INTERFACE_FORMAT_VERSION_KEY
                      ": ([0-9\\.]+)$", llvm::Regex::Newline);
}

llvm::Regex polar::getPHPInterfaceModuleFlagsRegex() {
   return llvm::Regex("^// " POLAR_MODULE_FLAGS_KEY ":(.*)$",
                      llvm::Regex::Newline);
}

/// Prints the imported modules in \p M to \p out in the form of \c import
/// source declarations.
static void printImports(raw_ostream &out, ModuleDecl *M) {
   // FIXME: This is very similar to what's in Serializer::writeInputBlock, but
   // it's not obvious what higher-level optimization would be factored out here.
   ModuleDecl::ImportFilter allImportFilter;
   allImportFilter |= ModuleDecl::ImportFilterKind::Public;
   allImportFilter |= ModuleDecl::ImportFilterKind::Private;

   SmallVector<ModuleDecl::ImportedModule, 8> allImports;
   M->getImportedModules(allImports, allImportFilter);
   ModuleDecl::removeDuplicateImports(allImports);
   diagnoseScopedImports(M->getAstContext().Diags, allImports);

   // Collect the public imports as a subset so that we can mark them with
   // '@_exported'.
   SmallVector<ModuleDecl::ImportedModule, 8> publicImports;
   M->getImportedModules(publicImports, ModuleDecl::ImportFilterKind::Public);
   llvm::SmallSet<ModuleDecl::ImportedModule, 8,
      ModuleDecl::OrderImportedModules> publicImportSet;
   publicImportSet.insert(publicImports.begin(), publicImports.end());

   for (auto import : allImports) {
      if (import.second->isOnoneSupportModule() ||
          import.second->isBuiltinModule()) {
         continue;
      }

      if (publicImportSet.count(import))
         out << "@_exported ";
      out << "import ";
      import.second->getReverseFullModuleName().printForward(out);

      // Write the access path we should be honoring but aren't.
      // (See diagnoseScopedImports above.)
      if (!import.first.empty()) {
         out << "/*";
         for (const auto &accessPathElem : import.first)
            out << "." << accessPathElem.first;
         out << "*/";
      }

      out << "\n";
   }
}

// FIXME: Copied from AstPrinter.cpp...
static bool isPublicOrUsableFromInline(const ValueDecl *VD) {
   AccessScope scope =
      VD->getFormalAccessScope(/*useDC*/nullptr,
         /*treatUsableFromInlineAsPublic*/true);
   return scope.isPublic();
}

static bool isPublicOrUsableFromInline(Type ty) {
   // Note the double negative here: we're looking for any referenced decls that
   // are *not* public-or-usableFromInline.
   return !ty.findIf([](Type typePart) -> bool {
      // FIXME: If we have an internal typealias for a non-internal type, we ought
      // to be able to print it by desugaring.
      if (auto *aliasTy = dyn_cast<TypeAliasType>(typePart.getPointer()))
         return !isPublicOrUsableFromInline(aliasTy->getDecl());
      if (auto *nominal = typePart->getAnyNominal())
         return !isPublicOrUsableFromInline(nominal);
      return false;
   });
}

namespace {
/// Collects protocols that are conformed to by a particular nominal. Since
/// AstPrinter will only print the public ones, the non-public ones get left by
/// the wayside. This is a problem when a non-public protocol inherits from a
/// public protocol; the generated module interface still needs to make that
/// dependency public.
///
/// The solution implemented here is to generate synthetic extensions that
/// declare the extra conformances. This isn't perfect (it loses the sugared
/// spelling of the protocol type, as well as the locality in the file), but it
/// does work.
class InheritedInterfaceCollector {
   static const StringLiteral DummyInterfaceName;

   using AvailableAttrList = TinyPtrVector<const AvailableAttr *>;
   using InterfaceAndAvailability =
   std::pair<InterfaceDecl *, AvailableAttrList>;

   /// Interfaces that will be included by the AstPrinter without any extra work.
   SmallVector<InterfaceDecl *, 8> IncludedInterfaces;
   /// Interfaces that will not be printed by the AstPrinter, along with the
   /// availability they were declared with.
   SmallVector<InterfaceAndAvailability, 8> ExtraInterfaces;
   /// Interfaces that can be printed, but whose conformances are constrained with
   /// something that \e can't be printed.
   SmallVector<const InterfaceType *, 8> ConditionalConformanceInterfaces;

   /// Helper to extract the `@available` attributes on a decl.
   static AvailableAttrList
   getAvailabilityAttrs(const Decl *D, Optional<AvailableAttrList> &cache) {
      if (cache.hasValue())
         return cache.getValue();

      cache.emplace();
      while (D) {
         for (auto *nextAttr : D->getAttrs().getAttributes<AvailableAttr>()) {
            // FIXME: This is just approximating the effects of nested availability
            // attributes for the same platform; formally they'd need to be merged.
            bool alreadyHasMoreSpecificAttrForThisPlatform =
               llvm::any_of(*cache, [nextAttr](const AvailableAttr *existingAttr) {
                  return existingAttr->Platform == nextAttr->Platform;
               });
            if (alreadyHasMoreSpecificAttrForThisPlatform)
               continue;
            cache->push_back(nextAttr);
         }
         D = D->getDeclContext()->getAsDecl();
      }

      return cache.getValue();
   }

   /// For each type in \p directlyInherited, classify the protocols it refers to
   /// as included for printing or not, and record them in the appropriate
   /// vectors.
   void recordInterfaces(ArrayRef<TypeLoc> directlyInherited, const Decl *D) {
      Optional<AvailableAttrList> availableAttrs;

      for (TypeLoc inherited : directlyInherited) {
         Type inheritedTy = inherited.getType();
         if (!inheritedTy || !inheritedTy->isExistentialType())
            continue;

         bool canPrintNormally = isPublicOrUsableFromInline(inheritedTy);
         ExistentialLayout layout = inheritedTy->getExistentialLayout();
         for (InterfaceType *protoTy : layout.getInterfaces()) {
            if (canPrintNormally)
               IncludedInterfaces.push_back(protoTy->getDecl());
            else
               ExtraInterfaces.push_back({protoTy->getDecl(),
                                          getAvailabilityAttrs(D, availableAttrs)});
         }
         // FIXME: This ignores layout constraints, but currently we don't support
         // any of those besides 'AnyObject'.
      }

      // Check for synthesized protocols, like Hashable on enums.
      if (auto *nominal = dyn_cast<NominalTypeDecl>(D)) {
         SmallVector<InterfaceConformance *, 4> localConformances =
            nominal->getLocalConformances(ConformanceLookupKind::NonInherited);

         for (auto *conf : localConformances) {
            if (conf->getSourceKind() != ConformanceEntryKind::Synthesized)
               continue;
            ExtraInterfaces.push_back({conf->getInterface(),
                                       getAvailabilityAttrs(D, availableAttrs)});
         }
      }
   }

   /// For each type directly inherited by \p extension, record any protocols
   /// that we would have printed in ConditionalConformanceInterfaces.
   void recordConditionalConformances(const ExtensionDecl *extension) {
      for (TypeLoc inherited : extension->getInherited()) {
         Type inheritedTy = inherited.getType();
         if (!inheritedTy || !inheritedTy->isExistentialType())
            continue;

         ExistentialLayout layout = inheritedTy->getExistentialLayout();
         for (InterfaceType *protoTy : layout.getInterfaces()) {
            if (!isPublicOrUsableFromInline(protoTy))
               continue;
            ConditionalConformanceInterfaces.push_back(protoTy);
         }
         // FIXME: This ignores layout constraints, but currently we don't support
         // any of those besides 'AnyObject'.
      }
   }

public:
   using PerTypeMap = llvm::MapVector<const NominalTypeDecl *,
      InheritedInterfaceCollector>;

   /// Given that we're about to print \p D, record its protocols in \p map.
   ///
   /// \sa recordInterfaces
   static void collectInterfaces(PerTypeMap &map, const Decl *D) {
      ArrayRef<TypeLoc> directlyInherited;
      const NominalTypeDecl *nominal;
      const IterableDeclContext *memberContext;

      if ((nominal = dyn_cast<NominalTypeDecl>(D))) {
         directlyInherited = nominal->getInherited();
         memberContext = nominal;

      } else if (auto *extension = dyn_cast<ExtensionDecl>(D)) {
         if (extension->isConstrainedExtension()) {
            // Conditional conformances never apply to inherited protocols, nor
            // can they provide unconditional conformances that might be used in
            // other extensions.
            return;
         }
         nominal = extension->getExtendedNominal();
         directlyInherited = extension->getInherited();
         memberContext = extension;

      } else {
         return;
      }

      if (!isPublicOrUsableFromInline(nominal))
         return;

      map[nominal].recordInterfaces(directlyInherited, D);

      // Recurse to find any nested types.
      for (const Decl *member : memberContext->getMembers())
         collectInterfaces(map, member);
   }

   /// If \p D is an extension providing conditional conformances, record those
   /// in \p map.
   ///
   /// \sa recordConditionalConformances
   static void collectSkippedConditionalConformances(PerTypeMap &map,
                                                     const Decl *D) {
      auto *extension = dyn_cast<ExtensionDecl>(D);
      if (!extension || !extension->isConstrainedExtension())
         return;

      const NominalTypeDecl *nominal = extension->getExtendedNominal();
      if (!isPublicOrUsableFromInline(nominal))
         return;

      map[nominal].recordConditionalConformances(extension);
      // No recursion here because extensions are never nested.
   }

   /// Returns true if the conformance of \p nominal to \p proto is declared in
   /// module \p M.
   static bool conformanceDeclaredInModule(ModuleDecl *M,
                                           const NominalTypeDecl *nominal,
                                           InterfaceDecl *proto) {
      SmallVector<InterfaceConformance *, 4> conformances;
      nominal->lookupConformance(M, proto, conformances);
      return llvm::all_of(conformances,
                          [M](const InterfaceConformance *conformance) -> bool {
                             return M == conformance->getDeclContext()->getParentModule();
                          });
   }

   /// If there were any public protocols that need to be printed (i.e. they
   /// weren't conformed to explicitly or inherited by another printed protocol),
   /// do so now by printing a dummy extension on \p nominal to \p out.
   void
   printSynthesizedExtensionIfNeeded(raw_ostream &out,
                                     const PrintOptions &printOptions,
                                     ModuleDecl *M,
                                     const NominalTypeDecl *nominal) const {
      if (ExtraInterfaces.empty())
         return;

      SmallPtrSet<InterfaceDecl *, 16> handledInterfaces;

      // First record all protocols that have already been handled.
      for (InterfaceDecl *proto : IncludedInterfaces) {
         proto->walkInheritedInterfaces(
            [&handledInterfaces](InterfaceDecl *inherited) -> TypeWalker::Action {
               handledInterfaces.insert(inherited);
               return TypeWalker::Action::Continue;
            });
      }

      // Then walk the remaining ones, and see what we need to print.
      // Note: We could do this in one pass, but the logic is easier to
      // understand if we build up the list and then print it, even if it takes
      // a bit more memory.
      // FIXME: This will pick the availability attributes from the first sight
      // of a protocol rather than the maximally available case.
      SmallVector<InterfaceAndAvailability, 16> protocolsToPrint;
      for (const auto &protoAndAvailability : ExtraInterfaces) {
         protoAndAvailability.first->walkInheritedInterfaces(
            [&](InterfaceDecl *inherited) -> TypeWalker::Action {
               if (!handledInterfaces.insert(inherited).second)
                  return TypeWalker::Action::SkipChildren;

               if (isPublicOrUsableFromInline(inherited) &&
                   conformanceDeclaredInModule(M, nominal, inherited)) {
                  protocolsToPrint.push_back({inherited, protoAndAvailability.second});
                  return TypeWalker::Action::SkipChildren;
               }

               return TypeWalker::Action::Continue;
            });
      }
      if (protocolsToPrint.empty())
         return;

      for (const auto &protoAndAvailability : protocolsToPrint) {
         StreamPrinter printer(out);
         // FIXME: Shouldn't this be an implicit conversion?
         TinyPtrVector<const DeclAttribute *> attrs;
         attrs.insert(attrs.end(), protoAndAvailability.second.begin(),
                      protoAndAvailability.second.end());
         DeclAttributes::print(printer, printOptions, attrs);

         printer << "extension ";
         nominal->getDeclaredType().print(printer, printOptions);
         printer << " : ";

         InterfaceDecl *proto = protoAndAvailability.first;
         proto->getDeclaredType()->print(printer, printOptions);

         printer << " {}\n";
      }
   }

   /// If there were any conditional conformances that couldn't be printed,
   /// make a dummy extension that conforms to all of them, constrained by a
   /// fake protocol.
   bool printInaccessibleConformanceExtensionIfNeeded(
      raw_ostream &out, const PrintOptions &printOptions,
      const NominalTypeDecl *nominal) const {
      if (ConditionalConformanceInterfaces.empty())
         return false;
      assert(nominal->isGenericContext());

      out << "@available(*, unavailable)\nextension ";
      nominal->getDeclaredType().print(out, printOptions);
      out << " : ";
      polar::interleave(ConditionalConformanceInterfaces,
                        [&out, &printOptions](const InterfaceType *protoTy) {
                           protoTy->print(out, printOptions);
                        }, [&out] { out << ", "; });
      out << " where "
          << nominal->getGenericSignature()->getGenericParams().front()->getName()
          << " : " << DummyInterfaceName << " {}\n";
      return true;
   }

   /// Print a fake protocol declaration for use by
   /// #printInaccessibleConformanceExtensionIfNeeded.
   static void printDummyInterfaceDeclaration(raw_ostream &out) {
      out << "\n@usableFromInline\ninternal protocol " << DummyInterfaceName
          << " {}\n";
   }
};

const StringLiteral InheritedInterfaceCollector::DummyInterfaceName =
   "_ConstraintThatIsNotPartOfTheAPIOfThisLibrary";
} // end anonymous namespace

bool polar::emitPHPInterface(raw_ostream &out,
                             ModuleInterfaceOptions const &Opts,
                             ModuleDecl *M) {
   assert(M);

   printToolVersionAndFlagsComment(out, Opts, M);
   printImports(out, M);

   const PrintOptions printOptions = PrintOptions::printSwiftInterfaceFile(
      Opts.PreserveTypesAsWritten);
   InheritedInterfaceCollector::PerTypeMap inheritedInterfaceMap;

   SmallVector<Decl *, 16> topLevelDecls;
   M->getTopLevelDecls(topLevelDecls);
   for (const Decl *D : topLevelDecls) {
      InheritedInterfaceCollector::collectInterfaces(inheritedInterfaceMap, D);

      if (!D->shouldPrintInContext(printOptions) ||
          !printOptions.shouldPrint(D)) {
         InheritedInterfaceCollector::collectSkippedConditionalConformances(
            inheritedInterfaceMap, D);
         continue;
      }

      D->print(out, printOptions);
      out << "\n";
   }

   // Print dummy extensions for any protocols that were indirectly conformed to.
   bool needDummyInterfaceDeclaration = false;
   for (const auto &nominalAndCollector : inheritedInterfaceMap) {
      const NominalTypeDecl *nominal = nominalAndCollector.first;
      const InheritedInterfaceCollector &collector = nominalAndCollector.second;
      collector.printSynthesizedExtensionIfNeeded(out, printOptions, M, nominal);
      needDummyInterfaceDeclaration |=
         collector.printInaccessibleConformanceExtensionIfNeeded(out,
                                                                 printOptions,
                                                                 nominal);
   }
   if (needDummyInterfaceDeclaration)
      InheritedInterfaceCollector::printDummyInterfaceDeclaration(out);

   return false;
}
