//===--- TBDGenVisitor.h - AST Visitor for TBD generation -----------------===//
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
//  This file defines the visitor that finds all symbols in a swift AST.
//
//===----------------------------------------------------------------------===//
#ifndef POLARPHP_TBDGEN_TBDGENVISITOR_H
#define POLARPHP_TBDGEN_TBDGENVISITOR_H

#include "polarphp/ast/AstMangler.h"
#include "polarphp/ast/AstVisitor.h"
#include "polarphp/ast/FileUnit.h"
#include "polarphp/ast/Module.h"
#include "polarphp/ast/ParameterList.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/irgen/Linking.h"
#include "polarphp/pil/lang/FormalLinkage.h"
#include "polarphp/pil/lang/PILDeclRef.h"
#include "polarphp/pil/lang/PILWitnessTable.h"
#include "polarphp/pil/lang/TypeLowering.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/ADT/Triple.h"
#include "llvm/TextAPI/MachO/InterfaceFile.h"

using namespace polar::irgen;
using StringSet = llvm::StringSet<>;

namespace llvm {
class DataLayout;
}

namespace polar {

struct TBDGenOptions;

namespace tbdgen {

class TBDGenVisitor : public AstVisitor<TBDGenVisitor> {
public:
   llvm::MachO::InterfaceFile &Symbols;
   llvm::MachO::TargetList Targets;
   StringSet *StringSymbols;
   const llvm::DataLayout &DataLayout;

   const UniversalLinkageInfo &UniversalLinkInfo;
   ModuleDecl *SwiftModule;
   const TBDGenOptions &Opts;

private:
   void addSymbol(StringRef name, llvm::MachO::SymbolKind kind =
   llvm::MachO::SymbolKind::GlobalSymbol);

   void addSymbol(PILDeclRef declRef);

   void addSymbol(LinkEntity entity);

   void addConformances(DeclContext *DC);

   void addDispatchThunk(PILDeclRef declRef);

   void addMethodDescriptor(PILDeclRef declRef);

   void addInterfaceRequirementsBaseDescriptor(InterfaceDecl *proto);
   void addAssociatedTypeDescriptor(AssociatedTypeDecl *assocType);
   void addAssociatedConformanceDescriptor(AssociatedConformance conformance);
   void addBaseConformanceDescriptor(BaseConformance conformance);

public:
   TBDGenVisitor(llvm::MachO::InterfaceFile &symbols,
                 llvm::MachO::TargetList targets, StringSet *stringSymbols,
                 const llvm::DataLayout &dataLayout,
                 const UniversalLinkageInfo &universalLinkInfo,
                 ModuleDecl *swiftModule, const TBDGenOptions &opts)
      : Symbols(symbols), Targets(targets), StringSymbols(stringSymbols),
        DataLayout(dataLayout), UniversalLinkInfo(universalLinkInfo),
        SwiftModule(swiftModule), Opts(opts) {}

   void addMainIfNecessary(FileUnit *file) {
      // HACK: 'main' is a special symbol that's always emitted in PILGen if
      //       the file has an entry point. Since it doesn't show up in the
      //       module until PILGen, we need to explicitly add it here.
      if (file->hasEntryPoint())
         addSymbol("main");
   }

   /// Adds the global symbols associated with the first file.
   void addFirstFileSymbols();

   void visitDefaultArguments(ValueDecl *VD, ParameterList *PL);

   void visitAbstractFunctionDecl(AbstractFunctionDecl *AFD);

   void visitAccessorDecl(AccessorDecl *AD);

   void visitNominalTypeDecl(NominalTypeDecl *NTD);

   void visitClassDecl(ClassDecl *CD);

   void visitConstructorDecl(ConstructorDecl *CD);

   void visitDestructorDecl(DestructorDecl *DD);

   void visitExtensionDecl(ExtensionDecl *ED);

   void visitFuncDecl(FuncDecl *FD);

   void visitInterfaceDecl(InterfaceDecl *PD);

   void visitAbstractStorageDecl(AbstractStorageDecl *ASD);

   void visitVarDecl(VarDecl *VD);

   void visitEnumDecl(EnumDecl *ED);

   void visitEnumElementDecl(EnumElementDecl *EED);

   void visitDecl(Decl *D) {}
};
} // end namespace tbdgen
} // end namespace polar

#endif // POLARPHP_TBDGEN_TBDGENVISITOR_H
