//===--- PILGenLazyConformance.cpp ----------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/gen/PILGen.h"
#include "polarphp/ast/Decl.h"
#include "polarphp/ast/InterfaceConformance.h"
#include "polarphp/clangimporter/ClangModule.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILVisitor.h"

using namespace polar;
using namespace polar::lowering;

void PILGenModule::useConformance(InterfaceConformanceRef conformanceRef) {
   // We don't need to emit dependent conformances.
   if (conformanceRef.isAbstract())
      return;

   auto conformance = conformanceRef.getConcrete();

   // Always look through inherited conformances.
   if (auto *inherited = dyn_cast<InheritedInterfaceConformance>(conformance))
      conformance = inherited->getInheritedConformance();

   // Get the normal conformance. If we don't have one, this is a self
   // conformance, which we can ignore.
   auto normal = dyn_cast<NormalInterfaceConformance>(
      conformance->getRootConformance());
   if (normal == nullptr)
      return;

   // Emit any conformances implied by conditional requirements.
   if (auto *specialized = dyn_cast<SpecializedInterfaceConformance>(conformance))
      useConformancesFromSubstitutions(specialized->getSubstitutionMap());

   // If this conformance was not synthesized by the ClangImporter, we're not
   // going to be emitting it lazily either, so we can avoid doing anything
   // below.
   if (!isa<ClangModuleUnit>(normal->getDeclContext()->getModuleScopeContext()))
      return;

   // If we already emitted this witness table, we don't need to track the fact
   // we need it.
   if (emittedWitnessTables.count(normal))
      return;

   // Check if we already forced this witness table but haven't emitted it yet.
   if (!forcedConformances.insert(normal).second)
      return;

   pendingConformances.push_back(normal);
}

void PILGenModule::useConformancesFromSubstitutions(
   const SubstitutionMap subs) {
   for (auto conf : subs.getConformances())
      useConformance(conf);
}

void PILGenModule::useConformancesFromType(CanType type) {
   if (!usedConformancesFromTypes.insert(type.getPointer()).second)
      return;

   type.visit([&](Type t) {
      auto *decl = t->getAnyNominal();
      if (!decl)
         return;

      if (isa<InterfaceDecl>(decl))
         return;

      auto genericSig = decl->getGenericSignature();
      if (!genericSig)
         return;

      auto subMap = t->getContextSubstitutionMap(PolarphpModule, decl);
      useConformancesFromSubstitutions(subMap);
      return;
   });
}

void PILGenModule::useConformancesFromObjectiveCType(CanType type) {
   if (!usedConformancesFromObjectiveCTypes.insert(type.getPointer()).second)
      return;

   auto &ctx = getAstContext();
   auto objectiveCBridgeable = ctx.getInterface(
      KnownInterfaceKind::ObjectiveCBridgeable);
   auto bridgedStoredNSError = ctx.getInterface(
      KnownInterfaceKind::BridgedStoredNSError);
   if (!objectiveCBridgeable && !bridgedStoredNSError)
      return;

   type.visit([&](Type t) {
      auto *decl = t->getAnyNominal();
      if (!decl)
         return;

      if (!isa<ClangModuleUnit>(decl->getModuleScopeContext()))
         return;

      if (objectiveCBridgeable) {
         if (auto subConformance =
            PolarphpModule->lookupConformance(t, objectiveCBridgeable))
            useConformance(subConformance);
      }

      if (bridgedStoredNSError) {
         if (auto subConformance =
            PolarphpModule->lookupConformance(t, bridgedStoredNSError))
            useConformance(subConformance);
      }
   });
}

/// A visitor class that tries to guess which PIL instructions can cause
/// IRGen to emit references to witness tables. This is used to emit
/// ClangImporter-synthesized conformances lazily.
///
/// In the long run, we'll instead have IRGen directly ask PILGen to
/// generate a witness table when needed, so that we don't have to do
/// any "guessing" here.
class LazyConformanceEmitter : public PILInstructionVisitor<LazyConformanceEmitter> {
   PILGenModule &SGM;

public:
   LazyConformanceEmitter(PILGenModule &SGM) : SGM(SGM) {}

   void visitAllocExistentialBoxInst(AllocExistentialBoxInst *AEBI) {
      SGM.useConformancesFromType(AEBI->getFormalConcreteType());
      SGM.useConformancesFromObjectiveCType(AEBI->getFormalConcreteType());
      for (auto conformance : AEBI->getConformances())
         SGM.useConformance(conformance);
   }

   void visitAllocGlobalInst(AllocGlobalInst *AGI) {
      SGM.useConformancesFromType(
         AGI->getReferencedGlobal()->getLoweredType().getAstType());
   }

   void visitAllocRefInst(AllocRefInst *ARI) {
      SGM.useConformancesFromType(ARI->getType().getAstType());
   }

   void visitAllocStackInst(AllocStackInst *ASI) {
      SGM.useConformancesFromType(ASI->getType().getAstType());
   }

   void visitAllocValueBufferInst(AllocValueBufferInst *AVBI) {
      SGM.useConformancesFromType(AVBI->getType().getAstType());
   }

   void visitApplyInst(ApplyInst *AI) {
      SGM.useConformancesFromObjectiveCType(AI->getSubstCalleeType());
      SGM.useConformancesFromSubstitutions(AI->getSubstitutionMap());
   }

   void visitBeginApplyInst(BeginApplyInst *BAI) {
      SGM.useConformancesFromObjectiveCType(BAI->getSubstCalleeType());
      SGM.useConformancesFromSubstitutions(BAI->getSubstitutionMap());
   }

   void visitBuiltinInst(BuiltinInst *BI) {
      SGM.useConformancesFromSubstitutions(BI->getSubstitutions());
   }

   void visitCheckedCastBranchInst(CheckedCastBranchInst *CCBI) {
      SGM.useConformancesFromType(CCBI->getSourceFormalType());
      SGM.useConformancesFromType(CCBI->getTargetFormalType());
      SGM.useConformancesFromObjectiveCType(CCBI->getSourceFormalType());
      SGM.useConformancesFromObjectiveCType(CCBI->getTargetFormalType());
   }

   void visitCheckedCastAddrBranchInst(CheckedCastAddrBranchInst *CCABI) {
      SGM.useConformancesFromType(CCABI->getSourceFormalType());
      SGM.useConformancesFromType(CCABI->getTargetFormalType());
      SGM.useConformancesFromObjectiveCType(CCABI->getSourceFormalType());
      SGM.useConformancesFromObjectiveCType(CCABI->getTargetFormalType());
   }

   void visitCheckedCastValueBranchInst(CheckedCastValueBranchInst *CCVBI) {
      SGM.useConformancesFromType(CCVBI->getSourceFormalType());
      SGM.useConformancesFromType(CCVBI->getTargetFormalType());
      SGM.useConformancesFromObjectiveCType(CCVBI->getSourceFormalType());
      SGM.useConformancesFromObjectiveCType(CCVBI->getTargetFormalType());
   }

   void visitCopyAddrInst(CopyAddrInst *CAI) {
      SGM.useConformancesFromType(CAI->getSrc()->getType().getAstType());
      SGM.useConformancesFromType(CAI->getDest()->getType().getAstType());
   }

   void visitCopyValueInst(CopyValueInst *CVI) {
      SGM.useConformancesFromType(CVI->getOperand()->getType().getAstType());
   }

   void visitDestroyAddrInst(DestroyAddrInst *DAI) {
      SGM.useConformancesFromType(DAI->getOperand()->getType().getAstType());
   }

   void visitDestroyValueInst(DestroyValueInst *DVI) {
      SGM.useConformancesFromType(DVI->getOperand()->getType().getAstType());
   }

   void visitGlobalAddrInst(GlobalAddrInst *GAI) {
      SGM.useConformancesFromType(
         GAI->getReferencedGlobal()->getLoweredType().getAstType());
   }

   void visitGlobalValueInst(GlobalValueInst *GVI) {
      SGM.useConformancesFromType(
         GVI->getReferencedGlobal()->getLoweredType().getAstType());
   }

   void visitKeyPathInst(KeyPathInst *KPI) {
      SGM.useConformancesFromSubstitutions(KPI->getSubstitutions());
   }

   void visitInitEnumDataAddrInst(InitEnumDataAddrInst *IEDAI) {
      SGM.useConformancesFromType(
         IEDAI->getOperand()->getType().getAstType());
   }

   void visitInjectEnumAddrInst(InjectEnumAddrInst *IEAI) {
      SGM.useConformancesFromType(IEAI->getOperand()->getType().getAstType());
   }

   void visitInitExistentialAddrInst(InitExistentialAddrInst *IEAI) {
      SGM.useConformancesFromType(IEAI->getFormalConcreteType());
      SGM.useConformancesFromObjectiveCType(IEAI->getFormalConcreteType());
      for (auto conformance : IEAI->getConformances())
         SGM.useConformance(conformance);
   }

   void visitInitExistentialMetatypeInst(InitExistentialMetatypeInst *IEMI) {
      SGM.useConformancesFromType(IEMI->getOperand()->getType().getAstType());
      for (auto conformance : IEMI->getConformances())
         SGM.useConformance(conformance);
   }

   void visitInitExistentialRefInst(InitExistentialRefInst *IERI) {
      SGM.useConformancesFromType(IERI->getFormalConcreteType());
      SGM.useConformancesFromObjectiveCType(IERI->getFormalConcreteType());
      for (auto conformance : IERI->getConformances())
         SGM.useConformance(conformance);
   }

   void visitInitExistentialValueInst(InitExistentialValueInst *IEVI) {
      SGM.useConformancesFromType(IEVI->getFormalConcreteType());
      SGM.useConformancesFromObjectiveCType(IEVI->getFormalConcreteType());
      for (auto conformance : IEVI->getConformances())
         SGM.useConformance(conformance);
   }

   void visitMetatypeInst(MetatypeInst *MI) {
      SGM.useConformancesFromType(MI->getType().getAstType());
   }

   void visitPartialApplyInst(PartialApplyInst *PAI) {
      SGM.useConformancesFromObjectiveCType(PAI->getSubstCalleeType());
      SGM.useConformancesFromSubstitutions(PAI->getSubstitutionMap());
   }

   void visitSelectEnumAddrInst(SelectEnumAddrInst *SEAI) {
      SGM.useConformancesFromType(
         SEAI->getEnumOperand()->getType().getAstType());
   }

   void visitStructElementAddrInst(StructElementAddrInst *SEAI) {
      SGM.useConformancesFromType(SEAI->getOperand()->getType().getAstType());
   }

   void visitTryApplyInst(TryApplyInst *TAI) {
      SGM.useConformancesFromObjectiveCType(TAI->getSubstCalleeType());
      SGM.useConformancesFromSubstitutions(TAI->getSubstitutionMap());
   }

   void visitTupleElementAddrInst(TupleElementAddrInst *TEAI) {
      SGM.useConformancesFromType(TEAI->getOperand()->getType().getAstType());
   }

   void visitUnconditionalCheckedCastInst(UnconditionalCheckedCastInst *UCCI) {
      SGM.useConformancesFromType(UCCI->getSourceFormalType());
      SGM.useConformancesFromType(UCCI->getTargetFormalType());
      SGM.useConformancesFromObjectiveCType(UCCI->getSourceFormalType());
      SGM.useConformancesFromObjectiveCType(UCCI->getTargetFormalType());
   }

   void visitUnconditionalCheckedCastAddrInst(UnconditionalCheckedCastAddrInst *UCCAI) {
      SGM.useConformancesFromType(UCCAI->getSourceFormalType());
      SGM.useConformancesFromType(UCCAI->getTargetFormalType());
      SGM.useConformancesFromObjectiveCType(UCCAI->getSourceFormalType());
      SGM.useConformancesFromObjectiveCType(UCCAI->getTargetFormalType());
   }

   void visitUncheckedTakeEnumDataAddrInst(UncheckedTakeEnumDataAddrInst *UTEDAI) {
      SGM.useConformancesFromType(UTEDAI->getOperand()->getType().getAstType());
   }

   void visitWitnessMethodInst(WitnessMethodInst *WMI) {
      SGM.useConformance(WMI->getConformance());
   }

   void visitPILInstruction(PILInstruction *I) {}
};

void PILGenModule::emitLazyConformancesForFunction(PILFunction *F) {
   LazyConformanceEmitter emitter(*this);

   for (auto &BB : *F)
      for (auto &I : BB)
         emitter.visit(&I);
}

void PILGenModule::emitLazyConformancesForType(NominalTypeDecl *NTD) {
   auto genericSig = NTD->getGenericSignature();

   if (genericSig) {
      for (auto reqt : genericSig->getRequirements()) {
         if (reqt.getKind() != RequirementKind::Layout)
            useConformancesFromType(reqt.getSecondType()->getCanonicalType());
      }
   }

   if (auto *ED = dyn_cast<EnumDecl>(NTD)) {
      for (auto *EED : ED->getAllElements()) {
         if (EED->hasAssociatedValues()) {
            useConformancesFromType(EED->getArgumentInterfaceType()
                                       ->getCanonicalType(genericSig));
         }
      }
   }

   if (isa<StructDecl>(NTD) || isa<ClassDecl>(NTD)) {
      for (auto *VD : NTD->getStoredProperties()) {
         useConformancesFromType(VD->getValueInterfaceType()
                                    ->getCanonicalType(genericSig));
      }
   }

   if (auto *CD = dyn_cast<ClassDecl>(NTD))
      if (auto superclass = CD->getSuperclass())
         useConformancesFromType(superclass->getCanonicalType(genericSig));

   if (auto *PD = dyn_cast<InterfaceDecl>(NTD)) {
      for (auto reqt : PD->getRequirementSignature()) {
         if (reqt.getKind() != RequirementKind::Layout)
            useConformancesFromType(reqt.getSecondType()->getCanonicalType());
      }
   }
}
