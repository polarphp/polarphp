//===--- GuaranteedARCOpts.cpp --------------------------------------------===//
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

#define DEBUG_TYPE "pil-guaranteed-arc-opts"

#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "llvm/ADT/Statistic.h"

using namespace polar;

STATISTIC(NumInstsEliminated, "Number of instructions eliminated");

namespace {

struct GuaranteedARCOptsVisitor
   : PILInstructionVisitor<GuaranteedARCOptsVisitor, bool> {
   bool visitPILInstruction(PILInstruction *I) { return false; }
   bool visitDestroyAddrInst(DestroyAddrInst *DAI);
   bool visitStrongReleaseInst(StrongReleaseInst *SRI);
   bool visitDestroyValueInst(DestroyValueInst *DVI);
   bool visitReleaseValueInst(ReleaseValueInst *RVI);
};

} // end anonymous namespace

static PILBasicBlock::reverse_iterator
getPrevReverseIterator(PILInstruction *I) {
   return std::next(I->getIterator().getReverse());
}

bool GuaranteedARCOptsVisitor::visitDestroyAddrInst(DestroyAddrInst *DAI) {
   PILValue Operand = DAI->getOperand();

   for (auto II = getPrevReverseIterator(DAI), IE = DAI->getParent()->rend();
        II != IE;) {
      auto *Inst = &*II;
      ++II;

      if (auto *CA = dyn_cast<CopyAddrInst>(Inst)) {
         if (CA->getSrc() == Operand && !CA->isTakeOfSrc()) {
            CA->setIsTakeOfSrc(IsTake);
            DAI->eraseFromParent();
            NumInstsEliminated += 2;
            return true;
         }
      }

      // destroy_addrs commonly exist in a block of dealloc_stack's, which don't
      // affect take-ability.
      if (isa<DeallocStackInst>(Inst))
         continue;

      // This code doesn't try to prove tricky validity constraints about whether
      // it is safe to push the destroy_addr past interesting instructions.
      if (Inst->mayHaveSideEffects())
         break;
   }

   // If we didn't find a copy_addr to fold this into, emit the destroy_addr.
   return false;
}

static bool couldReduceStrongRefcount(PILInstruction *Inst) {
   // Simple memory accesses cannot reduce refcounts.
   switch (Inst->getKind()) {
#define UNCHECKED_REF_STORAGE(Name, ...)                                       \
  case PILInstructionKind::Name##RetainValueInst:                              \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#define NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  case PILInstructionKind::Store##Name##Inst:
#define ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, ...)                         \
  case PILInstructionKind::Name##RetainInst:                                   \
  case PILInstructionKind::StrongRetain##Name##Inst:                           \
  case PILInstructionKind::StrongCopy##Name##ValueInst:
#define SOMETIMES_LOADABLE_CHECKED_REF_STORAGE(Name, ...) \
  NEVER_LOADABLE_CHECKED_REF_STORAGE(Name, "...") \
  ALWAYS_LOADABLE_CHECKED_REF_STORAGE(Name, "...")
#include "polarphp/ast/ReferenceStorageDef.h"
      case PILInstructionKind::LoadInst:
      case PILInstructionKind::StoreInst:
      case PILInstructionKind::RetainValueInst:
      case PILInstructionKind::StrongRetainInst:
      case PILInstructionKind::AllocStackInst:
      case PILInstructionKind::DeallocStackInst:
      case PILInstructionKind::BeginAccessInst:
      case PILInstructionKind::EndAccessInst:
      case PILInstructionKind::BeginUnpairedAccessInst:
      case PILInstructionKind::EndUnpairedAccessInst:
         return false;
      default:
         break;
   }

   // Assign and copyaddr of trivial types cannot drop refcounts, and 'inits'
   // never can either.  Nontrivial ones can though, because the overwritten
   // value drops a retain.  We would have to do more alias analysis to be able
   // to safely ignore one of those.
   if (auto *AI = dyn_cast<AssignInst>(Inst)) {
      PILType StoredType = AI->getOperand(0)->getType();
      if (StoredType.isTrivial(*Inst->getFunction()) ||
          StoredType.is<ReferenceStorageType>())
         return false;
   }

   if (auto *CAI = dyn_cast<CopyAddrInst>(Inst)) {
      // Initializations can only increase refcounts.
      if (CAI->isInitializationOfDest())
         return false;

      PILType StoredType = CAI->getOperand(0)->getType().getObjectType();
      if (StoredType.isTrivial(*Inst->getFunction()) ||
          StoredType.is<ReferenceStorageType>())
         return false;
   }

   // This code doesn't try to prove tricky validity constraints about whether
   // it is safe to push the release past interesting instructions.
   return Inst->mayHaveSideEffects();
}

bool GuaranteedARCOptsVisitor::visitStrongReleaseInst(StrongReleaseInst *SRI) {
   PILValue Operand = SRI->getOperand();
   // Release on a functionref is a noop.
   if (isa<FunctionRefInst>(Operand) || isa<DynamicFunctionRefInst>(Operand) ||
       isa<PreviousDynamicFunctionRefInst>(Operand)) {
      SRI->eraseFromParent();
      ++NumInstsEliminated;
      return true;
   }

   // Check to see if the instruction immediately before the insertion point is a
   // strong_retain of the specified operand.  If so, we can zap the pair.
   for (auto II = getPrevReverseIterator(SRI), IE = SRI->getParent()->rend();
        II != IE;) {
      auto *Inst = &*II;
      ++II;

      if (isa<StrongRetainInst>(Inst) || isa<RetainValueInst>(Inst)) {
         if (Inst->getOperand(0) == Operand) {
            Inst->eraseFromParent();
            SRI->eraseFromParent();
            NumInstsEliminated += 2;
            return true;
         }
         // Skip past unrelated retains.
         continue;
      }

      // Scan past simple instructions that cannot reduce strong refcounts.
      if (couldReduceStrongRefcount(Inst))
         break;
   }

   // If we didn't find a retain to fold this into, return false.
   return false;
}

bool GuaranteedARCOptsVisitor::visitDestroyValueInst(DestroyValueInst *DVI) {
   PILValue Operand = DVI->getOperand();
   for (auto II = getPrevReverseIterator(DVI), IE = DVI->getParent()->rend();
        II != IE;) {
      auto *Inst = &*II;
      ++II;

      if (auto *CVI = dyn_cast<CopyValueInst>(Inst)) {
         if (PILValue(CVI) == Operand || CVI->getOperand() == Operand) {
            CVI->replaceAllUsesWith(CVI->getOperand());
            CVI->eraseFromParent();
            DVI->eraseFromParent();
            NumInstsEliminated += 2;
            return true;
         }
         // Skip past unrelated retains.
         continue;
      }

      // Scan past simple instructions that cannot reduce refcounts.
      if (couldReduceStrongRefcount(Inst))
         break;
   }

   return false;
}

bool GuaranteedARCOptsVisitor::visitReleaseValueInst(ReleaseValueInst *RVI) {
   PILValue Operand = RVI->getOperand();

   for (auto II = getPrevReverseIterator(RVI), IE = RVI->getParent()->rend();
        II != IE;) {
      auto *Inst = &*II;
      ++II;

      if (isa<RetainValueInst>(Inst) || isa<StrongRetainInst>(Inst)) {
         if (Inst->getOperand(0) == Operand) {
            Inst->eraseFromParent();
            RVI->eraseFromParent();
            NumInstsEliminated += 2;
            return true;
         }
         // Skip past unrelated retains.
         continue;
      }

      // Scan past simple instructions that cannot reduce refcounts.
      if (couldReduceStrongRefcount(Inst))
         break;
   }

   // If we didn't find a retain to fold this into, emit the release.
   return false;
}

//===----------------------------------------------------------------------===//
//                            Top Level Entrypoint
//===----------------------------------------------------------------------===//

namespace {

// Even though this is a mandatory pass, it is rerun after deserialization in
// case DiagnosticConstantPropagation exposed anything new in this assert
// configuration.
struct GuaranteedARCOpts : PILFunctionTransform {
   void run() override {
      // Skip ownership PIL. We are going to have a run of semantic arc opts here.
      if (getFunction()->hasOwnership())
         return;

      GuaranteedARCOptsVisitor Visitor;

      bool MadeChange = false;
      PILFunction *F = getFunction();
      for (auto &BB : *F) {
         for (auto II = BB.begin(), IE = BB.end(); II != IE;) {
            PILInstruction *I = &*II;
            ++II;
            MadeChange |= Visitor.visit(I);
         }
      }

      if (MadeChange) {
         invalidateAnalysis(PILAnalysis::InvalidationKind::Instructions);
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createGuaranteedARCOpts() {
   return new GuaranteedARCOpts();
}
