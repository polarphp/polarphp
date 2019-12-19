//===--- CSE.cpp - Simple and fast CSE pass -------------------------------===//
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
// This pass performs a simple dominator tree walk that eliminates trivially
// redundant instructions.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-cse"

#include "polarphp/pil/lang/DebugUtils.h"
#include "polarphp/pil/lang/Dominance.h"
#include "polarphp/pil/lang/PILCloner.h"
#include "polarphp/pil/lang/PILOpenedArchetypesTracker.h"
#include "polarphp/pil/lang/PILType.h"
#include "polarphp/pil/lang/PILValue.h"
#include "polarphp/pil/lang/PILVisitor.h"
#include "polarphp/pil/optimizer/analysis/ArraySemantic.h"
#include "polarphp/pil/optimizer/analysis/DominanceAnalysis.h"
#include "polarphp/pil/optimizer/analysis/SideEffectAnalysis.h"
#include "polarphp/pil/optimizer/analysis/SimplifyInstruction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/ScopedHashTable.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/RecyclingAllocator.h"

STATISTIC(NumOpenExtRemoved,
          "Number of open_existential_addr instructions removed");

STATISTIC(NumSimplify, "Number of instructions simplified or DCE'd");
STATISTIC(NumCSE,      "Number of instructions CSE'd");

using namespace polar;

//===----------------------------------------------------------------------===//
//                                Simple Value
//===----------------------------------------------------------------------===//

namespace {
/// SimpleValue - Instances of this struct represent available values in the
/// scoped hash table.
struct SimpleValue {
   PILInstruction *Inst;

   SimpleValue(PILInstruction *I) : Inst(I) { }

   bool isSentinel() const {
      return Inst == llvm::DenseMapInfo<PILInstruction *>::getEmptyKey() ||
             Inst == llvm::DenseMapInfo<PILInstruction *>::getTombstoneKey();
   }
};
} // end anonymous namespace

namespace llvm {
template <> struct DenseMapInfo<SimpleValue> {
   static inline SimpleValue getEmptyKey() {
      return DenseMapInfo<PILInstruction *>::getEmptyKey();
   }
   static inline SimpleValue getTombstoneKey() {
      return DenseMapInfo<PILInstruction *>::getTombstoneKey();
   }
   static unsigned getHashValue(SimpleValue Val);
   static bool isEqual(SimpleValue LHS, SimpleValue RHS);
};
} // end namespace llvm

namespace {
class HashVisitor : public PILInstructionVisitor<HashVisitor, llvm::hash_code> {
   using hash_code = llvm::hash_code;

public:
   hash_code visitPILInstruction(PILInstruction *) {
      llvm_unreachable("No hash implemented for the given type");
   }

   hash_code visitBridgeObjectToRefInst(BridgeObjectToRefInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitBridgeObjectToWordInst(BridgeObjectToWordInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitClassifyBridgeObjectInst(ClassifyBridgeObjectInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand());
   }

   hash_code visitValueToBridgeObjectInst(ValueToBridgeObjectInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand());
   }

   hash_code visitRefToBridgeObjectInst(RefToBridgeObjectInst *X) {
      OperandValueArrayRef Operands(X->getAllOperands());
      return llvm::hash_combine(
         X->getKind(), X->getType(),
         llvm::hash_combine_range(Operands.begin(), Operands.end()));
   }

   hash_code visitUncheckedTrivialBitCastInst(UncheckedTrivialBitCastInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitUncheckedBitwiseCastInst(UncheckedBitwiseCastInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitUncheckedAddrCastInst(UncheckedAddrCastInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitFunctionRefInst(FunctionRefInst *X) {
      return llvm::hash_combine(X->getKind(),
                                X->getInitiallyReferencedFunction());
   }

   hash_code visitGlobalAddrInst(GlobalAddrInst *X) {
      return llvm::hash_combine(X->getKind(), X->getReferencedGlobal());
   }

   hash_code visitIntegerLiteralInst(IntegerLiteralInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getValue());
   }

   hash_code visitFloatLiteralInst(FloatLiteralInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getBits());
   }

   hash_code visitRefElementAddrInst(RefElementAddrInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getField());
   }

   hash_code visitRefTailAddrInst(RefTailAddrInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand());
   }

   hash_code visitProjectBoxInst(ProjectBoxInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand());
   }

   hash_code visitRefToRawPointerInst(RefToRawPointerInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand());
   }

   hash_code visitRawPointerToRefInst(RawPointerToRefInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand());
   }

#define LOADABLE_REF_STORAGE(Name, ...) \
  hash_code visit##Name##ToRefInst(Name##ToRefInst *X) { \
    return llvm::hash_combine(X->getKind(), X->getOperand()); \
  } \
  hash_code visitRefTo##Name##Inst(RefTo##Name##Inst *X) { \
    return llvm::hash_combine(X->getKind(), X->getOperand()); \
  }
#include "polarphp/ast/ReferenceStorageDef.h"

   hash_code visitUpcastInst(UpcastInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitStringLiteralInst(StringLiteralInst *X) {
      return llvm::hash_combine(X->getKind(), X->getEncoding(), X->getValue());
   }

   hash_code visitStructInst(StructInst *X) {
      // This is safe since we are hashing the operands using the actual pointer
      // values of the values being used by the operand.
      OperandValueArrayRef Operands(X->getAllOperands());
      return llvm::hash_combine(X->getKind(), X->getStructDecl(),
                                llvm::hash_combine_range(Operands.begin(), Operands.end()));
   }

   hash_code visitStructExtractInst(StructExtractInst *X) {
      return llvm::hash_combine(X->getKind(), X->getStructDecl(), X->getField(),
                                X->getOperand());
   }

   hash_code visitStructElementAddrInst(StructElementAddrInst *X) {
      return llvm::hash_combine(X->getKind(), X->getStructDecl(), X->getField(),
                                X->getOperand());
   }

   hash_code visitCondFailInst(CondFailInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand());
   }

   hash_code visitClassMethodInst(ClassMethodInst *X) {
      return llvm::hash_combine(X->getKind(),
                                X->getType(),
                                X->getOperand());
   }

   hash_code visitSuperMethodInst(SuperMethodInst *X) {
      return llvm::hash_combine(X->getKind(),
                                X->getType(),
                                X->getOperand());
   }

   hash_code visitTupleInst(TupleInst *X) {
      OperandValueArrayRef Operands(X->getAllOperands());
      return llvm::hash_combine(X->getKind(), X->getTupleType(),
                                llvm::hash_combine_range(Operands.begin(), Operands.end()));
   }

   hash_code visitTupleExtractInst(TupleExtractInst *X) {
      return llvm::hash_combine(X->getKind(), X->getTupleType(), X->getFieldNo(),
                                X->getOperand());
   }

   hash_code visitTupleElementAddrInst(TupleElementAddrInst *X) {
      return llvm::hash_combine(X->getKind(), X->getTupleType(), X->getFieldNo(),
                                X->getOperand());
   }

   hash_code visitMetatypeInst(MetatypeInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType());
   }

   hash_code visitValueMetatypeInst(ValueMetatypeInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitExistentialMetatypeInst(ExistentialMetatypeInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType());
   }

// @todo
//   hash_code visitObjCProtocolInst(ObjCProtocolInst *X) {
//      return llvm::hash_combine(X->getKind(), X->getType(), X->getProtocol());
//   }

   hash_code visitIndexRawPointerInst(IndexRawPointerInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getBase(),
                                X->getIndex());
   }

   hash_code visitPointerToAddressInst(PointerToAddressInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand(),
                                X->isStrict());
   }

   hash_code visitAddressToPointerInst(AddressToPointerInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getOperand());
   }

   hash_code visitApplyInst(ApplyInst *X) {
      OperandValueArrayRef Operands(X->getAllOperands());
      return llvm::hash_combine(X->getKind(), X->getCallee(),
                                llvm::hash_combine_range(Operands.begin(),
                                                         Operands.end()),
                                X->hasSubstitutions());
   }

   hash_code visitBuiltinInst(BuiltinInst *X) {
      OperandValueArrayRef Operands(X->getAllOperands());
      return llvm::hash_combine(X->getKind(), X->getName().get(),
                                llvm::hash_combine_range(Operands.begin(),
                                                         Operands.end()),
                                X->hasSubstitutions());
   }

   hash_code visitEnumInst(EnumInst *X) {
      // We hash the enum by hashing its kind, element, and operand if it has one.
      if (!X->hasOperand())
         return llvm::hash_combine(X->getKind(), X->getElement());
      return llvm::hash_combine(X->getKind(), X->getElement(), X->getOperand());
   }

   hash_code visitUncheckedEnumDataInst(UncheckedEnumDataInst *X) {
      // We hash the enum by hashing its kind, element, and operand.
      return llvm::hash_combine(X->getKind(), X->getElement(), X->getOperand());
   }

   hash_code visitIndexAddrInst(IndexAddrInst *X) {
      return llvm::hash_combine(X->getKind(), X->getType(), X->getBase(),
                                X->getIndex());
   }

// @todo
//   hash_code visitThickToObjCMetatypeInst(ThickToObjCMetatypeInst *X) {
//      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getType());
//   }
//
//   hash_code visitObjCToThickMetatypeInst(ObjCToThickMetatypeInst *X) {
//      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getType());
//   }
//
//   hash_code visitObjCMetatypeToObjectInst(ObjCMetatypeToObjectInst *X) {
//      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getType());
//   }
//
//   hash_code visitObjCExistentialMetatypeToObjectInst(
//      ObjCExistentialMetatypeToObjectInst *X) {
//      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getType());
//   }

   hash_code visitUncheckedRefCastInst(UncheckedRefCastInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getType());
   }

   hash_code visitSelectEnumInstBase(SelectEnumInstBase *X) {
      auto hash = llvm::hash_combine(X->getKind(),
                                     X->getEnumOperand(),
                                     X->getType(),
                                     X->hasDefault());

      for (unsigned i = 0, e = X->getNumCases(); i < e; ++i) {
         hash = llvm::hash_combine(hash, X->getCase(i).first,
                                   X->getCase(i).second);
      }

      if (X->hasDefault())
         hash = llvm::hash_combine(hash, X->getDefaultResult());

      return hash;
   }

   hash_code visitSelectEnumInst(SelectEnumInst *X) {
      return visitSelectEnumInstBase(X);
   }

   hash_code visitSelectEnumAddrInst(SelectEnumAddrInst *X) {
      return visitSelectEnumInstBase(X);
   }

   hash_code visitSelectValueInst(SelectValueInst *X) {
      auto hash = llvm::hash_combine(X->getKind(),
                                     X->getOperand(),
                                     X->getType(),
                                     X->hasDefault());

      for (unsigned i = 0, e = X->getNumCases(); i < e; ++i) {
         hash = llvm::hash_combine(hash, X->getCase(i).first,
                                   X->getCase(i).second);
      }

      if (X->hasDefault())
         hash = llvm::hash_combine(hash, X->getDefaultResult());

      return hash;
   }

   hash_code visitThinFunctionToPointerInst(ThinFunctionToPointerInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getType());
   }

   hash_code visitPointerToThinFunctionInst(PointerToThinFunctionInst *X) {
      return llvm::hash_combine(X->getKind(), X->getOperand(), X->getType());
   }

   hash_code visitWitnessMethodInst(WitnessMethodInst *X) {
      OperandValueArrayRef Operands(X->getAllOperands());
      return llvm::hash_combine(X->getKind(),
                                X->getLookupType().getPointer(),
                                X->getMember().getHashCode(),
                                X->getConformance(),
                                X->getType(),
                                !X->getTypeDependentOperands().empty(),
                                llvm::hash_combine_range(
                                   Operands.begin(),
                                   Operands.end()));
   }

   hash_code visitMarkDependenceInst(MarkDependenceInst *X) {
      OperandValueArrayRef Operands(X->getAllOperands());
      return llvm::hash_combine(
         X->getKind(), X->getType(),
         llvm::hash_combine_range(Operands.begin(), Operands.end()));
   }

   hash_code visitOpenExistentialRefInst(OpenExistentialRefInst *X) {
      auto ArchetypeTy = X->getType().castTo<ArchetypeType>();
      auto ConformsTo = ArchetypeTy->getConformsTo();
      return llvm::hash_combine(
         X->getKind(), X->getOperand(),
         llvm::hash_combine_range(ConformsTo.begin(), ConformsTo.end()));
   }
};
} // end anonymous namespace

unsigned llvm::DenseMapInfo<SimpleValue>::getHashValue(SimpleValue Val) {
   return HashVisitor().visit(Val.Inst);
}

bool llvm::DenseMapInfo<SimpleValue>::isEqual(SimpleValue LHS,
                                              SimpleValue RHS) {
   PILInstruction *LHSI = LHS.Inst, *RHSI = RHS.Inst;
   if (LHS.isSentinel() || RHS.isSentinel())
      return LHSI == RHSI;

   auto LOpen = dyn_cast<OpenExistentialRefInst>(LHSI);
   auto ROpen = dyn_cast<OpenExistentialRefInst>(RHSI);
   if (LOpen && ROpen) {
      // Check operands.
      if (LOpen->getOperand() != ROpen->getOperand())
         return false;

      // Consider the types of two open_existential_ref instructions to be equal,
      // if the sets of protocols they conform to are equal ...
      auto LHSArchetypeTy = LOpen->getType().castTo<ArchetypeType>();
      auto RHSArchetypeTy = ROpen->getType().castTo<ArchetypeType>();

      auto LHSConformsTo = LHSArchetypeTy->getConformsTo();
      auto RHSConformsTo = RHSArchetypeTy->getConformsTo();
      if (LHSConformsTo != RHSConformsTo)
         return false;

      // ... and other constraints are equal.
      if (LHSArchetypeTy->requiresClass() != RHSArchetypeTy->requiresClass())
         return false;

      if (LHSArchetypeTy->getSuperclass().getPointer() !=
          RHSArchetypeTy->getSuperclass().getPointer())
         return false;

      if (LHSArchetypeTy->getLayoutConstraint() !=
          RHSArchetypeTy->getLayoutConstraint())
         return false;

      return true;
   }
   return LHSI->getKind() == RHSI->getKind() && LHSI->isIdenticalTo(RHSI);
}

//===----------------------------------------------------------------------===//
//                               CSE Interface
//===----------------------------------------------------------------------===//

namespace polar {

/// CSE - This pass does a simple depth-first walk over the dominator tree,
/// eliminating trivially redundant instructions and using simplifyInstruction
/// to canonicalize things as it goes. It is intended to be fast and catch
/// obvious cases so that PILCombine and other passes are more effective.
class CSE {
public:
   typedef llvm::ScopedHashTableVal<SimpleValue, ValueBase *> SimpleValueHTType;
   typedef llvm::RecyclingAllocator<llvm::BumpPtrAllocator, SimpleValueHTType>
      AllocatorTy;
   typedef llvm::ScopedHashTable<SimpleValue, PILInstruction *,
      llvm::DenseMapInfo<SimpleValue>,
      AllocatorTy> ScopedHTType;

   /// AvailableValues - This scoped hash table contains the current values of
   /// all of our simple scalar expressions.  As we walk down the domtree, we
   /// look to see if instructions are in this: if so, we replace them with what
   /// we find, otherwise we insert them so that dominated values can succeed in
   /// their lookup.
   ScopedHTType *AvailableValues;

   SideEffectAnalysis *SEA;

   CSE(bool RunsOnHighLevelSil, SideEffectAnalysis *SEA)
      : SEA(SEA), RunsOnHighLevelSil(RunsOnHighLevelSil) {}

   bool processFunction(PILFunction &F, DominanceInfo *DT);

   bool canHandle(PILInstruction *Inst);

private:

   /// True if CSE is done on high-level PIL, i.e. semantic calls are not inlined
   /// yet. In this case some semantic calls can be CSEd.
   bool RunsOnHighLevelSil;

   // NodeScope - almost a POD, but needs to call the constructors for the
   // scoped hash tables so that a new scope gets pushed on. These are RAII so
   // that the scope gets popped when the NodeScope is destroyed.
   class NodeScope {
   public:
      NodeScope(ScopedHTType *availableValues) : Scope(*availableValues) {}

   private:
      NodeScope(const NodeScope &) = delete;
      void operator=(const NodeScope &) = delete;

      ScopedHTType::ScopeTy Scope;
   };

   // StackNode - contains all the needed information to create a stack for doing
   // a depth first traversal of the tree. This includes scopes for values and
   // loads as well as the generation. There is a child iterator so that the
   // children do not need to be store separately.
   class StackNode {
   public:
      StackNode(ScopedHTType *availableValues, DominanceInfoNode *n,
                DominanceInfoNode::iterator child,
                DominanceInfoNode::iterator end)
         : Node(n), ChildIter(child), EndIter(end), Scopes(availableValues),
           Processed(false) {}

      // Accessors.
      DominanceInfoNode *node() { return Node; }
      DominanceInfoNode::iterator childIter() { return ChildIter; }
      DominanceInfoNode *nextChild() {
         DominanceInfoNode *child = *ChildIter;
         ++ChildIter;
         return child;
      }
      DominanceInfoNode::iterator end() { return EndIter; }
      bool isProcessed() { return Processed; }
      void process() { Processed = true; }

   private:
      StackNode(const StackNode &) = delete;
      void operator=(const StackNode &) = delete;

      // Members.
      DominanceInfoNode *Node;
      DominanceInfoNode::iterator ChildIter;
      DominanceInfoNode::iterator EndIter;
      NodeScope Scopes;
      bool Processed;
   };

   bool processNode(DominanceInfoNode *Node);
   bool processOpenExistentialRef(OpenExistentialRefInst *Inst, ValueBase *V);
};
} // namespace polar

//===----------------------------------------------------------------------===//
//                             CSE Implementation
//===----------------------------------------------------------------------===//

bool CSE::processFunction(PILFunction &Fm, DominanceInfo *DT) {
   std::vector<StackNode *> nodesToProcess;

   // Tables that the pass uses when walking the domtree.
   ScopedHTType AVTable;
   AvailableValues = &AVTable;

   bool Changed = false;

   // Process the root node.
   nodesToProcess.push_back(new StackNode(AvailableValues, DT->getRootNode(),
                                          DT->getRootNode()->begin(),
                                          DT->getRootNode()->end()));

   // Process the stack.
   while (!nodesToProcess.empty()) {
      // Grab the first item off the stack. Set the current generation, remove
      // the node from the stack, and process it.
      StackNode *NodeToProcess = nodesToProcess.back();

      // Check if the node needs to be processed.
      if (!NodeToProcess->isProcessed()) {
         // Process the node.
         Changed |= processNode(NodeToProcess->node());
         NodeToProcess->process();

      } else if (NodeToProcess->childIter() != NodeToProcess->end()) {
         // Push the next child onto the stack.
         DominanceInfoNode *child = NodeToProcess->nextChild();
         nodesToProcess.push_back(
            new StackNode(AvailableValues, child, child->begin(), child->end()));
      } else {
         // It has been processed, and there are no more children to process,
         // so delete it and pop it off the stack.
         delete NodeToProcess;
         nodesToProcess.pop_back();
      }
   } // while (!nodes...)

   return Changed;
}

namespace {
// A very simple cloner for cloning instructions inside
// the same function. The only interesting thing it does
// is remapping the archetypes when it is required.
class InstructionCloner : public PILCloner<InstructionCloner> {
   friend class PILCloner<InstructionCloner>;
   friend class PILInstructionVisitor<InstructionCloner>;
   PILInstruction *Result = nullptr;
public:
   InstructionCloner(PILFunction *F) : PILCloner(*F) {}

   static PILInstruction *doIt(PILInstruction *I) {
      InstructionCloner TC(I->getFunction());
      return TC.clone(I);
   }

   PILInstruction *clone(PILInstruction *I) {
      visit(I);
      return Result;
   }

   void postProcess(PILInstruction *Orig, PILInstruction *Cloned) {
      assert(Orig->getFunction() == &getBuilder().getFunction() &&
             "cloning between functions is not supported");

      Result = Cloned;
      PILCloner<InstructionCloner>::postProcess(Orig, Cloned);
   }
   PILValue getMappedValue(PILValue Value) {
      return Value;
   }
   PILBasicBlock *remapBasicBlock(PILBasicBlock *BB) { return BB; }
};
} // end anonymous namespace

/// Update PIL basic block's arguments types which refer to opened
/// archetypes. Replace such types by performing type substitutions
/// according to the provided type substitution map.
static void updateBasicBlockArgTypes(PILBasicBlock *BB,
                                     ArchetypeType *OldOpenedArchetype,
                                     ArchetypeType *NewOpenedArchetype) {
   // Check types of all BB arguments.
   for (auto *Arg : BB->getPILPhiArguments()) {
      if (!Arg->getType().hasOpenedExistential())
         continue;
      // Type of this BB argument uses an opened existential.
      // Try to apply substitutions to it and if it produces a different type,
      // use this type as new type of the BB argument.
      auto OldArgType = Arg->getType();
      auto NewArgType = OldArgType.subst(BB->getModule(),
                                         [&](SubstitutableType *type) -> Type {
                                            if (type == OldOpenedArchetype)
                                               return NewOpenedArchetype;
                                            return type;
                                         },
                                         MakeAbstractConformanceForGenericType());
      if (NewArgType == Arg->getType())
         continue;
      // Replace the type of this BB argument. The type of a BBArg
      // can only be changed using replaceBBArg, if the BBArg has no uses.
      // So, make it look as if it has no uses.

      // First collect all uses, before changing the type.
      SmallVector<Operand *, 4> OriginalArgUses;
      for (auto *ArgUse : Arg->getUses()) {
         OriginalArgUses.push_back(ArgUse);
      }
      // Then replace all uses by an undef.
      Arg->replaceAllUsesWith(PILUndef::get(Arg->getType(), *BB->getParent()));
      // Replace the type of the BB argument.
      auto *NewArg = BB->replacePhiArgument(Arg->getIndex(), NewArgType,
                                            Arg->getOwnershipKind(),
                                            Arg->getDecl());
      // Restore all uses to refer to the BB argument with updated type.
      for (auto ArgUse : OriginalArgUses) {
         ArgUse->set(NewArg);
      }
   }
}

/// Handle CSE of open_existential_ref instructions.
/// Returns true if uses of open_existential_ref can
/// be replaced by a dominating instruction.
/// \Inst is the open_existential_ref instruction
/// \V is the dominating open_existential_ref instruction
bool CSE::processOpenExistentialRef(OpenExistentialRefInst *Inst,
                                    ValueBase *V) {
   // All the open instructions are single-value instructions.
   auto VI = dyn_cast<SingleValueInstruction>(V);
   if (!VI) return false;

   llvm::SmallSetVector<PILInstruction *, 16> Candidates;
   auto OldOpenedArchetype = getOpenedArchetypeOf(Inst);
   auto NewOpenedArchetype = getOpenedArchetypeOf(VI);

   // Collect all candidates that may contain opened archetypes
   // that need to be replaced.
   for (auto Use : Inst->getUses()) {
      auto User = Use->getUser();
      if (!User->getTypeDependentOperands().empty()) {
         if (canHandle(User)) {
            auto It = AvailableValues->begin(User);
            if (It != AvailableValues->end()) {
               return false;
            }
         }
         Candidates.insert(User);
      }
      if (!isa<TermInst>(User))
         continue;
      // The current use of the opened archetype is a terminator instruction.
      // Check if any of the successor BBs uses this opened archetype in the
      // types of its basic block arguments. If this is the case, replace
      // those uses by the new opened archetype.
      auto Successors = User->getParent()->getSuccessorBlocks();
      for (auto Successor : Successors) {
         if (Successor->args_empty())
            continue;
         // If a BB has any arguments, update their types if necessary.
         updateBasicBlockArgTypes(Successor,
                                  OldOpenedArchetype,
                                  NewOpenedArchetype);
      }
   }
   // Now process candidates.
   // TODO: Move it to CSE instance to avoid recreating it every time?
   PILOpenedArchetypesTracker OpenedArchetypesTracker(Inst->getFunction());
   // Register the new archetype to be used.
   OpenedArchetypesTracker.registerOpenedArchetypes(VI);
   // Use a cloner. It makes copying the instruction and remapping of
   // opened archetypes trivial.
   InstructionCloner Cloner(Inst->getFunction());
   Cloner.registerOpenedExistentialRemapping(
      OldOpenedArchetype->castTo<ArchetypeType>(), NewOpenedArchetype);
   auto &Builder = Cloner.getBuilder();
   Builder.setOpenedArchetypesTracker(&OpenedArchetypesTracker);

   llvm::SmallPtrSet<PILInstruction *, 16> Processed;
   // Now clone each candidate and replace the opened archetype
   // by a dominating one.
   while (!Candidates.empty()) {
      auto Candidate = Candidates.pop_back_val();
      if (Processed.count(Candidate))
         continue;

      // Compute if a candidate depends on the old opened archetype.
      // It always does if it has any type-dependent operands.
      bool DependsOnOldOpenedArchetype =
         !Candidate->getTypeDependentOperands().empty();

      // Look for dependencies propagated via the candidate's results.
      for (auto CandidateResult : Candidate->getResults()) {
         if (CandidateResult->use_empty() ||
             !CandidateResult->getType().hasOpenedExistential())
            continue;

         // Check if the result type depends on this specific opened existential.
         auto ResultDependsOnOldOpenedArchetype =
            CandidateResult->getType().getAstType().findIf(
               [&OldOpenedArchetype](Type t) -> bool {
                  return (CanType(t) == OldOpenedArchetype);
               });

         // If it does, the candidate depends on the opened existential.
         if (ResultDependsOnOldOpenedArchetype) {
            DependsOnOldOpenedArchetype |= ResultDependsOnOldOpenedArchetype;

            // The users of this candidate are new candidates.
            for (auto Use : CandidateResult->getUses()) {
               Candidates.insert(Use->getUser());
            }
         }
      }
      // Remember that this candidate was processed already.
      Processed.insert(Candidate);

      // No need to clone if there is no dependency on the old opened archetype.
      if (!DependsOnOldOpenedArchetype)
         continue;

      Builder.getOpenedArchetypes().addOpenedArchetypeOperands(
         Candidate->getTypeDependentOperands());
      Builder.setInsertionPoint(Candidate);
      auto NewI = Cloner.clone(Candidate);
      // Result types of candidate's uses instructions may be using this archetype.
      // Thus, we need to try to replace it there.
      Candidate->replaceAllUsesPairwiseWith(NewI);
      eraseFromParentWithDebugInsts(Candidate);
   }
   return true;
}

bool CSE::processNode(DominanceInfoNode *Node) {
   PILBasicBlock *BB = Node->getBlock();
   bool Changed = false;

   // See if any instructions in the block can be eliminated.  If so, do it.  If
   // not, add them to AvailableValues. Assume the block terminator can't be
   // erased.
   for (PILBasicBlock::iterator nextI = BB->begin(), E = BB->end();
        nextI != E;) {
      PILInstruction *Inst = &*nextI;
      ++nextI;

      LLVM_DEBUG(llvm::dbgs() << "PILCSE VISITING: " << *Inst << "\n");

      // Dead instructions should just be removed.
      if (isInstructionTriviallyDead(Inst)) {
         LLVM_DEBUG(llvm::dbgs() << "PILCSE DCE: " << *Inst << '\n');
         nextI = eraseFromParentWithDebugInsts(Inst);
         Changed = true;
         ++NumSimplify;
         continue;
      }

      // If the instruction can be simplified (e.g. X+0 = X) then replace it with
      // its simpler value.
      if (PILValue V = simplifyInstruction(Inst)) {
         LLVM_DEBUG(llvm::dbgs() << "PILCSE SIMPLIFY: " << *Inst << "  to: " << *V
                                 << '\n');
         nextI = replaceAllSimplifiedUsesAndErase(Inst, V);
         Changed = true;
         ++NumSimplify;
         continue;
      }

      // If this is not a simple instruction that we can value number, skip it.
      if (!canHandle(Inst))
         continue;

      // If an instruction can be handled here, then it must also be handled
      // in isIdenticalTo, otherwise looking up a key in the map with fail to
      // match itself.
      assert(Inst->isIdenticalTo(Inst) &&
             "Inst must match itself for map to work");

      // Now that we know we have an instruction we understand see if the
      // instruction has an available value.  If so, use it.
      if (PILInstruction *AvailInst = AvailableValues->lookup(Inst)) {
         LLVM_DEBUG(llvm::dbgs() << "PILCSE CSE: " << *Inst << "  to: "
                                 << *AvailInst << '\n');
         // Instructions producing a new opened archetype need a special handling,
         // because replacing these instructions may require a replacement
         // of the opened archetype type operands in some of the uses.
         if (!isa<OpenExistentialRefInst>(Inst)
             || processOpenExistentialRef(
            cast<OpenExistentialRefInst>(Inst),
            cast<OpenExistentialRefInst>(AvailInst))) {
            // processOpenExistentialRef may delete instructions other than Inst, so
            // nextI must be reassigned.
            nextI = std::next(Inst->getIterator());
            Inst->replaceAllUsesPairwiseWith(AvailInst);
            Inst->eraseFromParent();
            Changed = true;
            ++NumCSE;
            continue;
         }
      }

      // Otherwise, just remember that this value is available.
      AvailableValues->insert(Inst, Inst);
      LLVM_DEBUG(llvm::dbgs() << "PILCSE Adding to value table: " << *Inst
                              << " -> " << *Inst << "\n");
   }

   return Changed;
}

bool CSE::canHandle(PILInstruction *Inst) {
   if (auto *AI = dyn_cast<ApplyInst>(Inst)) {
      if (!AI->mayReadOrWriteMemory())
         return true;

      if (RunsOnHighLevelSil) {
         ArraySemanticsCall SemCall(AI);
         switch (SemCall.getKind()) {
            case ArrayCallKind::kGetCount:
            case ArrayCallKind::kGetCapacity:
            case ArrayCallKind::kCheckIndex:
            case ArrayCallKind::kCheckSubscript:
               return SemCall.hasGuaranteedSelf();
            default:
               return false;
         }
      }

      // We can CSE function calls which do not read or write memory and don't
      // have any other side effects.
      FunctionSideEffects Effects;
      SEA->getCalleeEffects(Effects, AI);

      // Note that the function also may not contain any retains. And there are
      // functions which are read-none and have a retain, e.g. functions which
      // _convert_ a global_addr to a reference and retain it.
      auto MB = Effects.getMemBehavior(RetainObserveKind::ObserveRetains);
      if (MB == PILInstruction::MemoryBehavior::None)
         return true;

      return false;
   }
   if (auto *BI = dyn_cast<BuiltinInst>(Inst)) {
      // Although the onFastPath builtin has no side-effects we don't want to
      // (re-)move it.
      if (BI->getBuiltinInfo().ID == BuiltinValueKind::OnFastPath)
         return false;
      return !BI->mayReadOrWriteMemory();
   }
   if (auto *EMI = dyn_cast<ExistentialMetatypeInst>(Inst)) {
      return !EMI->getOperand()->getType().isAddress();
   }
   switch (Inst->getKind()) {
      case PILInstructionKind::ClassMethodInst:
      case PILInstructionKind::SuperMethodInst:
      case PILInstructionKind::FunctionRefInst:
      case PILInstructionKind::GlobalAddrInst:
      case PILInstructionKind::IntegerLiteralInst:
      case PILInstructionKind::FloatLiteralInst:
      case PILInstructionKind::StringLiteralInst:
      case PILInstructionKind::StructInst:
      case PILInstructionKind::StructExtractInst:
      case PILInstructionKind::StructElementAddrInst:
      case PILInstructionKind::TupleInst:
      case PILInstructionKind::TupleExtractInst:
      case PILInstructionKind::TupleElementAddrInst:
      case PILInstructionKind::MetatypeInst:
      case PILInstructionKind::ValueMetatypeInst:
//      case PILInstructionKind::ObjCProtocolInst:
      case PILInstructionKind::RefElementAddrInst:
      case PILInstructionKind::RefTailAddrInst:
      case PILInstructionKind::ProjectBoxInst:
      case PILInstructionKind::IndexRawPointerInst:
      case PILInstructionKind::IndexAddrInst:
      case PILInstructionKind::PointerToAddressInst:
      case PILInstructionKind::AddressToPointerInst:
      case PILInstructionKind::CondFailInst:
      case PILInstructionKind::EnumInst:
      case PILInstructionKind::UncheckedEnumDataInst:
      case PILInstructionKind::UncheckedTrivialBitCastInst:
      case PILInstructionKind::UncheckedBitwiseCastInst:
      case PILInstructionKind::RefToRawPointerInst:
      case PILInstructionKind::RawPointerToRefInst:
      case PILInstructionKind::UpcastInst:
//      case PILInstructionKind::ThickToObjCMetatypeInst:
//      case PILInstructionKind::ObjCToThickMetatypeInst:
      case PILInstructionKind::UncheckedRefCastInst:
      case PILInstructionKind::UncheckedAddrCastInst:
//      case PILInstructionKind::ObjCMetatypeToObjectInst:
//      case PILInstructionKind::ObjCExistentialMetatypeToObjectInst:
      case PILInstructionKind::SelectEnumInst:
      case PILInstructionKind::SelectValueInst:
      case PILInstructionKind::RefToBridgeObjectInst:
      case PILInstructionKind::BridgeObjectToRefInst:
      case PILInstructionKind::BridgeObjectToWordInst:
      case PILInstructionKind::ClassifyBridgeObjectInst:
      case PILInstructionKind::ValueToBridgeObjectInst:
      case PILInstructionKind::ThinFunctionToPointerInst:
      case PILInstructionKind::PointerToThinFunctionInst:
      case PILInstructionKind::MarkDependenceInst:
      case PILInstructionKind::OpenExistentialRefInst:
      case PILInstructionKind::WitnessMethodInst:
         // Intentionally we don't handle (prev_)dynamic_function_ref.
         // They change at runtime.
#define LOADABLE_REF_STORAGE(Name, ...) \
  case PILInstructionKind::RefTo##Name##Inst: \
  case PILInstructionKind::Name##ToRefInst:
#include "polarphp/ast/ReferenceStorageDef.h"
         return true;
      default:
         return false;
   }
}

using ApplyWitnessPair = std::pair<ApplyInst *, WitnessMethodInst *>;

/// Returns the Apply and WitnessMethod instructions that use the
/// open_existential_addr instructions, or null if at least one of the
/// instructions is missing.
static ApplyWitnessPair getOpenExistentialUsers(OpenExistentialAddrInst *OE) {
   ApplyInst *AI = nullptr;
   WitnessMethodInst *WMI = nullptr;
   ApplyWitnessPair Empty = std::make_pair(nullptr, nullptr);

   for (auto *UI : getNonDebugUses(OE)) {
      auto *User = UI->getUser();
      if (!isa<WitnessMethodInst>(User) &&
          User->isTypeDependentOperand(UI->getOperandNumber()))
         continue;
      // Check that we have a single Apply user.
      if (auto *AA = dyn_cast<ApplyInst>(User)) {
         if (AI)
            return Empty;

         AI = AA;
         continue;
      }

      // Check that we have a single WMI user.
      if (auto *W = dyn_cast<WitnessMethodInst>(User)) {
         if (WMI)
            return Empty;

         WMI = W;
         continue;
      }

      // Unknown instruction.
      return Empty;
   }

   // Both instructions need to exist.
   if (!WMI || !AI)
      return Empty;

   // Make sure that the WMI and AI match.
   if (AI->getCallee() != WMI)
      return Empty;

   // We have exactly the pattern that we expected.
   return std::make_pair(AI, WMI);
}

/// Try to CSE the users of \p From to the users of \p To.
/// The original users of \p To are passed in ToApplyWitnessUsers.
/// Returns true on success.
static bool tryToCSEOpenExtCall(OpenExistentialAddrInst *From,
                                OpenExistentialAddrInst *To,
                                ApplyWitnessPair ToApplyWitnessUsers,
                                DominanceInfo *DA) {
   assert(From != To && "Can't replace instruction with itself");

   ApplyInst *FromAI = nullptr;
   ApplyInst *ToAI = nullptr;
   WitnessMethodInst *FromWMI = nullptr;
   WitnessMethodInst *ToWMI = nullptr;
   std::tie(FromAI, FromWMI) = getOpenExistentialUsers(From);
   std::tie(ToAI, ToWMI) = ToApplyWitnessUsers;

   // Make sure that the OEA instruction has exactly two expected users.
   if (!FromAI || !ToAI || !FromWMI || !ToWMI)
      return false;

   // Make sure we are calling the same method.
   if (FromWMI->getMember() != ToWMI->getMember())
      return false;

   // We are going to reuse the TO-WMI, so make sure it dominates the call site.
   if (!DA->properlyDominates(ToWMI, FromWMI))
      return false;

   PILBuilder Builder(FromAI);
   // Make archetypes used by the ToAI available to the builder.
   PILOpenedArchetypesTracker OpenedArchetypesTracker(FromAI->getFunction());
   OpenedArchetypesTracker.registerUsedOpenedArchetypes(ToAI);
   Builder.setOpenedArchetypesTracker(&OpenedArchetypesTracker);

   assert(FromAI->getArguments().size() == ToAI->getArguments().size() &&
          "Invalid number of arguments");

   // Don't handle any apply instructions that involve substitutions.
   if (ToAI->getSubstitutionMap().getReplacementTypes().size() != 1)
      return false;

   // Prepare the Apply args.
   SmallVector<PILValue, 8> Args;
   for (auto Op : FromAI->getArguments()) {
      Args.push_back(Op == From ? To : Op);
   }

   ApplyInst *NAI = Builder.createApply(ToAI->getLoc(), ToWMI,
                                        ToAI->getSubstitutionMap(), Args,
                                        ToAI->isNonThrowing());
   FromAI->replaceAllUsesWith(NAI);
   FromAI->eraseFromParent();
   NumOpenExtRemoved++;
   return true;
}

/// Try to CSE the users of the protocol that's passed in argument \p Arg.
/// \returns True if some instructions were modified.
static bool CSExistentialInstructions(PILFunctionArgument *Arg,
                                      DominanceInfo *DA) {
   ParameterConvention Conv = Arg->getKnownParameterInfo().getConvention();
   // We can assume that the address of Proto does not alias because the
   // calling convention is In or In-guaranteed.
   bool MayAlias = Conv != ParameterConvention::Indirect_In_Guaranteed &&
                   Conv != ParameterConvention::Indirect_In;
   if (MayAlias)
      return false;

   // Now check that the only uses of the protocol are witness_method,
   // open_existential_addr and destroy_addr. Also, collect all of the 'opens'.
   llvm::SmallVector<OpenExistentialAddrInst*, 8> Opens;
   for (auto *UI : getNonDebugUses(Arg)) {
      auto *User = UI->getUser();
      if (auto *Open = dyn_cast<OpenExistentialAddrInst>(User)) {
         Opens.push_back(Open);
         continue;
      }

      if (isa<WitnessMethodInst>(User) || isa<DestroyAddrInst>(User))
         continue;

      // Bail out if we found an instruction that we can't handle.
      return false;
   }

   // Find the best dominating 'open' for each open existential.
   llvm::SmallVector<OpenExistentialAddrInst*, 8> TopDominator(Opens);

   bool Changed = false;

   // Try to CSE the users of the current open_existential_addr instruction with
   // one of the other open_existential_addr that dominate it.
   int NumOpenInstr = Opens.size();
   for (int i = 0; i < NumOpenInstr; i++) {
      // Try to find a better dominating 'open' for the i-th instruction.
      OpenExistentialAddrInst *SomeOpen = TopDominator[i];
      for (int j = 0; j < NumOpenInstr; j++) {

         if (i == j || TopDominator[i] == TopDominator[j])
            continue;

         OpenExistentialAddrInst *DominatingOpen = TopDominator[j];

         if (DominatingOpen->getOperand() != SomeOpen->getOperand())
            continue;

         if (DA->properlyDominates(DominatingOpen, SomeOpen)) {
            // We found an open instruction that DominatingOpen dominates:
            TopDominator[i] = TopDominator[j];
         }
      }
   }


   // Inspect all of the open_existential_addr instructions and record the
   // apply-witness users. We need to save the original Apply-Witness users
   // because we'll be adding new users and we need to make sure that we can
   // find the original users.
   llvm::SmallVector<ApplyWitnessPair, 8> OriginalAW;
   for (int i=0; i < NumOpenInstr; i++) {
      OriginalAW.push_back(getOpenExistentialUsers(TopDominator[i]));
   }

   // Perform the CSE for the open_existential_addr instruction and their
   // dominating instruction.
   for (int i=0; i < NumOpenInstr; i++) {
      if (Opens[i] != TopDominator[i])
         Changed |= tryToCSEOpenExtCall(Opens[i], TopDominator[i],
                                        OriginalAW[i], DA);
   }

   return Changed;
}

/// Detect multiple calls to existential members and try to CSE the instructions
/// that perform the method lookup (the open_existential_addr and
/// witness_method):
///
/// open_existential_addr %0 : $*Pingable to $*@opened("1E467EB8-...")
/// witness_method $@opened("1E467EB8-...") Pingable, #Pingable.ping!1, %2
/// apply %3<@opened("1E467EB8-...") Pingable>(%2)
///
/// \returns True if some instructions were modified.
static bool CSEExistentialCalls(PILFunction *Func, DominanceInfo *DA) {
   bool Changed = false;
   for (auto *Arg : Func->getArgumentsWithoutIndirectResults()) {
      if (Arg->getType().isExistentialType()) {
         auto *FArg = cast<PILFunctionArgument>(Arg);
         Changed |= CSExistentialInstructions(FArg, DA);
      }
   }

   return Changed;
}

namespace {
class PILCSE : public PILFunctionTransform {

   /// True if CSE is done on high-level PIL, i.e. semantic calls are not inlined
   /// yet. In this case some semantic calls can be CSEd.
   /// We only CSE semantic calls on high-level PIL because we can be sure that
   /// e.g. an Array as PILValue is really immutable (including its content).
   bool RunsOnHighLevelSil;

   void run() override {
      // FIXME: We should be able to support ownership.
      if (getFunction()->hasOwnership())
         return;

      LLVM_DEBUG(llvm::dbgs() << "***** CSE on function: "
                              << getFunction()->getName() << " *****\n");

      DominanceAnalysis* DA = getAnalysis<DominanceAnalysis>();

      auto *SEA = PM->getAnalysis<SideEffectAnalysis>();

      CSE C(RunsOnHighLevelSil, SEA);
      bool Changed = false;

      // Perform the traditional CSE.
      Changed |= C.processFunction(*getFunction(), DA->get(getFunction()));

      // Perform CSE of existential and witness_method instructions.
      Changed |= CSEExistentialCalls(getFunction(),
                                     DA->get(getFunction()));
      if (Changed) {
         invalidateAnalysis(PILAnalysis::InvalidationKind::CallsAndInstructions);
      }
   }

public:
   PILCSE(bool RunsOnHighLevelSil) : RunsOnHighLevelSil(RunsOnHighLevelSil) {}
};
} // end anonymous namespace

PILTransform *polar::createCSE() {
   return new PILCSE(false);
}

PILTransform *polar::createHighLevelCSE() {
   return new PILCSE(true);
}
