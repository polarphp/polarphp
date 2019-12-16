//===--- Linker.h -----------------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_LANG_INTERNAL_LINKER_H
#define POLARPHP_PIL_LANG_INTERNAL_LINKER_H

#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "polarphp/pil/lang/PILModule.h"
#include <functional>

namespace polar {

/// Visitor that knows how to link in dependencies of PILInstructions.
class PILLinkerVisitor : public PILInstructionVisitor<PILLinkerVisitor, void> {
   using LinkingMode = PILModule::LinkingMode;

   /// The PILModule that we are loading from.
   PILModule &Mod;

   /// Break cycles visiting recursive protocol conformances.
   llvm::DenseSet<InterfaceConformance *> VisitedConformances;

   /// Worklist of PILFunctions we are processing.
   llvm::SmallVector<PILFunction *, 128> Worklist;

   /// A list of callees of the current instruction being visited. cleared after
   /// every instruction is visited.
   llvm::SmallVector<PILFunction *, 4> FunctionDeserializationWorklist;

   /// The current linking mode.
   LinkingMode Mode;

   /// Whether any functions were deserialized.
   bool Changed;

public:
   PILLinkerVisitor(PILModule &M, PILModule::LinkingMode LinkingMode)
      : Mod(M), Worklist(), FunctionDeserializationWorklist(),
        Mode(LinkingMode), Changed(false) {}

   /// Process F, recursively deserializing any thing F may reference.
   /// Returns true if any deserialization was performed.
   bool processFunction(PILFunction *F);

   /// Deserialize the VTable mapped to C if it exists and all PIL the VTable
   /// transitively references.
   ///
   /// This method assumes that the caller made sure that no vtable existed in
   /// Mod.
   PILVTable *processClassDecl(const ClassDecl *C);

   /// We do not want to visit callee functions if we just have a value base.
   void visitPILInstruction(PILInstruction *I) { }

   void visitApplyInst(ApplyInst *AI);
   void visitTryApplyInst(TryApplyInst *TAI);
   void visitPartialApplyInst(PartialApplyInst *PAI);
   void visitFunctionRefInst(FunctionRefInst *FRI);
   void visitDynamicFunctionRefInst(DynamicFunctionRefInst *FRI);
   void visitPreviousDynamicFunctionRefInst(PreviousDynamicFunctionRefInst *FRI);
   void visitInterfaceConformance(InterfaceConformanceRef C,
                                 const Optional<PILDeclRef> &Member);
   void visitApplySubstitutions(SubstitutionMap subs);
   void visitWitnessMethodInst(WitnessMethodInst *WMI) {
      visitInterfaceConformance(WMI->getConformance(), WMI->getMember());
   }
   void visitInitExistentialAddrInst(InitExistentialAddrInst *IEI);
   void visitInitExistentialRefInst(InitExistentialRefInst *IERI);
   void visitAllocRefInst(AllocRefInst *ARI);
   void visitMetatypeInst(MetatypeInst *MI);

private:
   /// Cause a function to be deserialized, and visit all other functions
   /// referenced from this function according to the linking mode.
   void addFunctionToWorklist(PILFunction *F);

   /// Consider a function for deserialization if the current linking mode
   /// requires it.
   void maybeAddFunctionToWorklist(PILFunction *F);

   /// Is the current mode link all? Link all implies we should try and link
   /// everything, not just transparent/shared functions.
   bool isLinkAll() const { return Mode == LinkingMode::LinkAll; }

   void linkInVTable(ClassDecl *D);

   // Main loop of the visitor. Called by one of the other *visit* methods.
   void process();
};

} // polar

#endif //POLARPHP_PIL_LANG_INTERNAL_LINKER_H
