//===--- TypeLayoutDumper.cpp ---------------------------------------------===//
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
// This file defines a tool for dumping layouts of fixed-size types in a simple
// YAML format.
//
//===----------------------------------------------------------------------===//

#include "polarphp/irgen/internal/TypeLayoutDumper.h"
#include "polarphp/irgen/internal/FixedTypeInfo.h"
#include "polarphp/irgen/internal/GenType.h"
#include "polarphp/irgen/internal/IRGenModule.h"
#include "polarphp/irgen/internal/LegacyLayoutFormat.h"

#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/AstWalker.h"
#include "polarphp/ast/IRGenOptions.h"
#include "polarphp/ast/Types.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/global/Subsystems.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/YAMLTraits.h"
#include "llvm/Support/raw_ostream.h"

#include <string>
#include <vector>

using namespace polar;
using namespace irgen;

namespace {

class NominalTypeWalker : public AstWalker {
   std::vector<NominalTypeDecl *> &Results;

public:
   NominalTypeWalker(std::vector<NominalTypeDecl *> &Results)
      : Results(Results) {}

   bool walkToDeclPre(Decl *D) override {
      if (auto *NTD = dyn_cast<NominalTypeDecl>(D))
         Results.push_back(NTD);

      return true;
   }
};

} // end anonymous namespace

static std::string mangleTypeAsContext(const NominalTypeDecl *type) {
   mangle::AstMangler Mangler;
   return Mangler.mangleTypeAsContextUSR(type);
}

static YAMLTypeInfoNode createYAMLTypeInfoNode(NominalTypeDecl *NTD,
                                               IRGenModule &IGM,
                                               const FixedTypeInfo *fixedTI) {
   return {mangleTypeAsContext(NTD),
           fixedTI->getFixedSize().getValue(),
           fixedTI->getFixedAlignment().getValue(),
           fixedTI->getFixedExtraInhabitantCount(IGM)};
}

static void addYAMLTypeInfoNode(NominalTypeDecl *NTD,
                                IRGenModule &IGM,
                                std::vector<YAMLTypeInfoNode> &Result) {
   // We only care about public and @usableFromInline declarations.
   if (NTD->getEffectiveAccess() < AccessLevel::Public)
      return;

   // We don't care about protocols or classes.
   if (isa<InterfaceDecl>(NTD) || isa<ClassDecl>(NTD))
      return;

   assert(isa<StructDecl>(NTD) || isa<EnumDecl>(NTD));

   auto &Opts = IGM.getOptions();

   switch (Opts.TypeInfoFilter) {
      case IRGenOptions::TypeInfoDumpFilter::All:
         break;
      case IRGenOptions::TypeInfoDumpFilter::Resilient:
         if (!NTD->isFormallyResilient())
            return;
         break;
      case IRGenOptions::TypeInfoDumpFilter::Fragile:
         if (NTD->isFormallyResilient())
            return;
         break;
   }

   auto *TI = &IGM.getTypeInfoForUnlowered(NTD->getDeclaredTypeInContext());
   auto *fixedTI = dyn_cast<FixedTypeInfo>(TI);
   if (!fixedTI)
      return;

   Result.push_back(createYAMLTypeInfoNode(NTD, IGM, fixedTI));
}

static Optional<YAMLModuleNode>
createYAMLModuleNode(ModuleDecl *Mod, IRGenModule &IGM) {
   std::vector<NominalTypeDecl *> Decls;
   NominalTypeWalker Walker(Decls);

   // Collect all nominal types, including nested types.
   SmallVector<Decl *, 16> TopLevelDecls;
   Mod->getTopLevelDecls(TopLevelDecls);

   for (auto *D : TopLevelDecls)
      D->walk(Walker);

   std::vector<YAMLTypeInfoNode> Nodes;

   // Convert each nominal type.
   for (auto *D : Decls) {
      if (auto *NTD = dyn_cast<NominalTypeDecl>(D)) {
         addYAMLTypeInfoNode(NTD, IGM, Nodes);
      }
   }

   if (Nodes.empty())
      return None;

   std::sort(Nodes.begin(), Nodes.end());

   return YAMLModuleNode{Mod->getName().str(), Nodes};
}

void TypeLayoutDumper::write(ArrayRef<ModuleDecl *> AllModules,
                             llvm::raw_ostream &os) const {
   llvm::yaml::Output yout(os);

   // Collect all nominal types, including nested types.
   for (auto *Mod : AllModules) {
      auto Node = createYAMLModuleNode(Mod, IGM);
      if (Node)
         yout << *Node;
   }
}

bool polar::performDumpTypeInfo(IRGenOptions &Opts,
                                PILModule &PILMod,
                                llvm::LLVMContext &LLVMContext) {
   auto &Ctx = PILMod.getAstContext();
   assert(!Ctx.hadError());
   (void)Ctx;

   IRGenerator IRGen(Opts, PILMod);
   IRGenModule IGM(IRGen, IRGen.createTargetMachine(), LLVMContext);

   // We want to bypass resilience.
   LoweringModeScope scope(IGM, TypeConverter::Mode::CompletelyFragile);

   auto *Mod = PILMod.getTypePHPModule();
   SmallVector<Decl *, 16> AllDecls;
   Mod->getTopLevelDecls(AllDecls);

   SmallVector<ModuleDecl *, 4> AllModules;
   for (auto *D : AllDecls) {
      if (auto *ID = dyn_cast<ImportDecl>(D)) {
         if (auto *M = ID->getModule())
            AllModules.push_back(M);
      }
   }

   TypeLayoutDumper dumper(IGM);
   dumper.write(AllModules, llvm::outs());

   return false;
}
