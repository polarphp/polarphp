//===--- USRGeneration.cpp - Routines for USR generation ------------------===//
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

#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/ClangModuleLoader.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/UsrGeneration.h"
#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/TypeCheckRequests.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/Index/USRGeneration.h"
#include "clang/Lex/PreprocessingRecord.h"
#include "clang/Lex/Preprocessor.h"

namespace polar {

namespace ide {
static inline StringRef getUSRSpacePrefix() {
   return "s:";
}

bool printTypeUSR(Type Ty, raw_ostream &OS) {
   assert(!Ty->hasArchetype() && "cannot have contextless archetypes mangled.");
   mangle::AstMangler Mangler;
   OS << Mangler.mangleTypeAsUSR(Ty->getRValueType());
   return false;
}

bool printDeclTypeUSR(const ValueDecl *D, raw_ostream &OS) {
   mangle::AstMangler Mangler;
   std::string MangledName = Mangler.mangleDeclType(D);
   OS << MangledName;
   return false;
}

} // ide

llvm::Expected<std::string> USRGenerationRequest::evaluate(Evaluator &evaluator,
                                      const ValueDecl *D) const {
   if (auto *VD = dyn_cast<VarDecl>(D))
      D = VD->getCanonicalVarDecl();

   if (!D->hasName() && !isa<ParamDecl>(D) && !isa<AccessorDecl>(D))
      return std::string(); // Ignore.
   if (D->getModuleContext()->isBuiltinModule())
      return std::string(); // Ignore.
   if (isa<ModuleDecl>(D))
      return std::string(); // Ignore.

   auto interpretAsClangNode = [](const ValueDecl *D) -> ClangNode {
      ClangNode ClangN = D->getClangNode();
      if (auto ClangD = ClangN.getAsDecl()) {
         // NSErrorDomain causes the clang enum to be imported like this:
         //
         // struct MyError {
         //     enum Code : Int32 {
         //         case errFirst
         //         case errSecond
         //     }
         //     static var errFirst: MyError.Code { get }
         //     static var errSecond: MyError.Code { get }
         // }
         //
         // The clang enum constants are associated with both the static vars and
         // the enum cases.
         // But we want unique USRs for the above symbols, so use the clang USR
         // for the enum cases, and the Swift USR for the vars.
         //
         if (auto *ClangEnumConst = dyn_cast<clang::EnumConstantDecl>(ClangD)) {
            if (auto *ClangEnum = dyn_cast<clang::EnumDecl>(ClangEnumConst->getDeclContext())) {
            /// @todo
//               if (ClangEnum->hasAttr<clang::NSErrorDomainAttr>() && isa<VarDecl>(D))
//                  return ClangNode();
            }
         }
      }
      return ClangN;
   };

   llvm::SmallString<128> Buffer;
   llvm::raw_svector_ostream OS(Buffer);

   if (ClangNode ClangN = interpretAsClangNode(D)) {
      if (auto ClangD = ClangN.getAsDecl()) {
         bool Ignore = clang::index::generateUSRForDecl(ClangD, Buffer);
         if (!Ignore) {
            return Buffer.str();
         } else {
            return std::string();
         }
      }

      auto &Importer = *D->getAstContext().getClangModuleLoader();

      auto ClangMacroInfo = ClangN.getAsMacro();
      bool Ignore = clang::index::generateUSRForMacro(
         D->getBaseName().getIdentifier().str(),
         ClangMacroInfo->getDefinitionLoc(),
         Importer.getClangAstContext().getSourceManager(), Buffer);
      if (!Ignore)
         return Buffer.str();
      else
         return std::string();
   }

   auto declIFaceTy = D->getInterfaceType();

   // Invalid code.
   if (declIFaceTy.findIf([](Type t) -> bool {
      return t->is<ModuleType>();
   }))
      return std::string();

   mangle::AstMangler NewMangler;
   return NewMangler.mangleDeclAsUSR(D, ide::getUSRSpacePrefix());
}

llvm::Expected<std::string>
 MangleLocalTypeDeclRequest::evaluate(Evaluator &evaluator,
                                            const TypeDecl *D) const {
   if (isa<ModuleDecl>(D))
      return std::string(); // Ignore.

   mangle::AstMangler NewMangler;
   return NewMangler.mangleLocalTypeDecl(D);
}

namespace ide {

bool printModuleUSR(ModuleEntity Mod, raw_ostream &OS) {
   if (auto *D = Mod.getAsSwiftModule()) {
      StringRef moduleName = D->getName().str();
      return clang::index::generateFullUSRForTopLevelModuleName(moduleName, OS);
   } else if (auto ClangM = Mod.getAsClangModule()) {
      return clang::index::generateFullUSRForModule(ClangM, OS);
   } else {
      return true;
   }
}

bool printValueDeclUSR(const ValueDecl *D, raw_ostream &OS) {
   auto result = evaluateOrDefault(D->getAstContext().evaluator,
                                   USRGenerationRequest{D},
                                   std::string());
   if (result.empty())
      return true;
   OS << result;
   return false;
}

bool printAccessorUSR(const AbstractStorageDecl *D, AccessorKind AccKind,
                      llvm::raw_ostream &OS) {
   // AccKind should always be either IsGetter or IsSetter here, based
   // on whether a reference is a mutating or non-mutating use.  USRs
   // aren't supposed to reflect implementation differences like stored
   // vs. addressed vs. observing.
   //
   // On the other side, the implementation indexer should be
   // registering the getter/setter USRs independently of how they're
   // actually implemented.  So a stored variable should still have
   // getter/setter USRs (pointing to the variable declaration), and an
   // addressed variable should have its "getter" point at the
   // addressor.

   AbstractStorageDecl *SD = const_cast<AbstractStorageDecl *>(D);

   mangle::AstMangler NewMangler;
   std::string Mangled = NewMangler.mangleAccessorEntityAsUSR(AccKind,
                                                              SD, getUSRSpacePrefix(), SD->isStatic());

   OS << Mangled;

   return false;
}

bool printExtensionUSR(const ExtensionDecl *ED, raw_ostream &OS) {
   auto nominal = ED->getExtendedNominal();
   if (!nominal)
      return true;

   // We make up a unique usr for each extension by combining a prefix
   // and the USR of the first value member of the extension.
   for (auto D : ED->getMembers()) {
      if (auto VD = dyn_cast<ValueDecl>(D)) {
         OS << getUSRSpacePrefix() << "e:";
         return printValueDeclUSR(VD, OS);
      }
   }
   OS << getUSRSpacePrefix() << "e:";
   printValueDeclUSR(nominal, OS);
   for (auto Inherit : ED->getInherited()) {
      if (auto T = Inherit.getType()) {
         if (T->getAnyNominal())
            return printValueDeclUSR(T->getAnyNominal(), OS);
      }
   }
   return true;
}

bool printDeclUSR(const Decl *D, raw_ostream &OS) {
   if (auto *VD = dyn_cast<ValueDecl>(D)) {
      if (printValueDeclUSR(VD, OS)) {
         return true;
      }
   } else if (auto *ED = dyn_cast<ExtensionDecl>(D)) {
      if (printExtensionUSR(ED, OS)) {
         return true;
      }
   } else {
      return true;
   }
   return false;
}
} // ide
} // polar