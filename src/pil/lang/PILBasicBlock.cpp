//===--- PILBasicBlock.cpp - Basic blocks for high-level PIL code ---------===//
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
// This file defines the high-level BasicBlocks used for Swift PIL code.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/STLExtras.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/DebugUtils.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILBuilder.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILDebugScope.h"
#include "polarphp/pil/lang/PILFunction.h"
//#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/global/NameStrings.h"

using namespace polar;
using namespace llvm;

//===----------------------------------------------------------------------===//
// PILBasicBlock Implementation
//===----------------------------------------------------------------------===//

PILBasicBlock::PILBasicBlock(PILFunction *parent, PILBasicBlock *relativeToBB,
                             bool after)
   : Parent(parent), PredList(nullptr) {
   if (!relativeToBB) {
      parent->getBlocks().push_back(this);
   } else if (after) {
      parent->getBlocks().insertAfter(relativeToBB->getIterator(), this);
   } else {
      parent->getBlocks().insert(relativeToBB->getIterator(), this);
   }
}
PILBasicBlock::~PILBasicBlock() {
   // Invalidate all of the basic block arguments.
   for (auto *Arg : ArgumentList) {
      getModule().notifyDeleteHandlers(Arg);
   }

   dropAllReferences();

   PILModule *M = nullptr;
   if (getParent())
      M = &getParent()->getModule();

   for (auto I = begin(), E = end(); I != E;) {
      auto Inst = &*I;
      ++I;
      if (M) {
         // Notify the delete handlers that the instructions in this block are
         // being deleted.
         M->notifyDeleteHandlers(Inst);
      }
      erase(Inst);
   }

   // iplist's destructor is going to destroy the InstList.
   InstList.clearAndLeakNodesUnsafely();
}

int PILBasicBlock::getDebugID() const {
   if (!getParent())
      return -1;
   int idx = 0;
   for (const PILBasicBlock &B : *getParent()) {
      if (&B == this)
         return idx;
      idx++;
   }
   llvm_unreachable("block not in function's block list");
}

PILModule &PILBasicBlock::getModule() const {
   return getParent()->getModule();
}

void PILBasicBlock::insert(iterator InsertPt, PILInstruction *I) {
   InstList.insert(InsertPt, I);
}

void PILBasicBlock::push_back(PILInstruction *I) {
   InstList.push_back(I);
}

void PILBasicBlock::push_front(PILInstruction *I) {
   InstList.push_front(I);
}

void PILBasicBlock::remove(PILInstruction *I) {
   InstList.remove(I);
}

void PILBasicBlock::eraseInstructions() {
   for (auto It = begin(); It != end();) {
      auto *Inst = &*It++;
      Inst->replaceAllUsesOfAllResultsWithUndef();
      Inst->eraseFromParent();
   }
}

/// Returns the iterator following the erased instruction.
PILBasicBlock::iterator PILBasicBlock::erase(PILInstruction *I) {
   // Notify the delete handlers that this instruction is going away.
   getModule().notifyDeleteHandlers(&*I);
   auto *F = getParent();
   auto nextIter = InstList.erase(I);
   F->getModule().deallocateInst(I);
   return nextIter;
}

/// This method unlinks 'self' from the containing PILFunction and deletes it.
void PILBasicBlock::eraseFromParent() {
   getParent()->getBlocks().erase(this);
}

void PILBasicBlock::cloneArgumentList(PILBasicBlock *Other) {
   assert(Other->isEntry() == isEntry() &&
          "Expected to both blocks to be entries or not");
   if (isEntry()) {
      assert(args_empty() && "Expected to have no arguments");
      for (auto *FuncArg : Other->getPILFunctionArguments()) {
         createFunctionArgument(FuncArg->getType(),
                                FuncArg->getDecl());
      }
      return;
   }

   for (auto *PHIArg : Other->getPILPhiArguments()) {
      createPhiArgument(PHIArg->getType(), PHIArg->getOwnershipKind(),
                        PHIArg->getDecl());
   }
}

PILFunctionArgument *
PILBasicBlock::createFunctionArgument(PILType Ty, const ValueDecl *D,
                                      bool disableEntryBlockVerification) {
   assert((disableEntryBlockVerification || isEntry()) &&
          "Function Arguments can only be in the entry block");
   const PILFunction *Parent = getParent();
   auto OwnershipKind = ValueOwnershipKind(
      *Parent, Ty,
      Parent->getConventions().getPILArgumentConvention(getNumArguments()));
   return new (getModule()) PILFunctionArgument(this, Ty, OwnershipKind, D);
}

PILFunctionArgument *PILBasicBlock::insertFunctionArgument(arg_iterator Iter,
                                                           PILType Ty,
                                                           ValueOwnershipKind OwnershipKind,
                                                           const ValueDecl *D) {
   assert(isEntry() && "Function Arguments can only be in the entry block");
   return new (getModule()) PILFunctionArgument(this, Iter, Ty, OwnershipKind, D);
}

PILFunctionArgument *PILBasicBlock::replaceFunctionArgument(
   unsigned i, PILType Ty, ValueOwnershipKind Kind, const ValueDecl *D) {
   assert(isEntry() && "Function Arguments can only be in the entry block");

   PILFunction *F = getParent();
   PILModule &M = F->getModule();
   if (Ty.isTrivial(*F))
      Kind = ValueOwnershipKind::None;

   assert(ArgumentList[i]->use_empty() && "Expected no uses of the old arg!");

   // Notify the delete handlers that this argument is being deleted.
   M.notifyDeleteHandlers(ArgumentList[i]);

   PILFunctionArgument *NewArg = new (M) PILFunctionArgument(Ty, Kind, D);
   NewArg->setParent(this);

   // TODO: When we switch to malloc/free allocation we'll be leaking memory
   // here.
   ArgumentList[i] = NewArg;

   return NewArg;
}

/// Replace the ith BB argument with a new one with type Ty (and optional
/// ValueDecl D).
PILPhiArgument *PILBasicBlock::replacePhiArgument(unsigned i, PILType Ty,
                                                  ValueOwnershipKind Kind,
                                                  const ValueDecl *D) {
   assert(!isEntry() && "PHI Arguments can not be in the entry block");
   PILFunction *F = getParent();
   PILModule &M = F->getModule();
   if (Ty.isTrivial(*F))
      Kind = ValueOwnershipKind::None;

   assert(ArgumentList[i]->use_empty() && "Expected no uses of the old BB arg!");

   // Notify the delete handlers that this argument is being deleted.
   M.notifyDeleteHandlers(ArgumentList[i]);

   PILPhiArgument *NewArg = new (M) PILPhiArgument(Ty, Kind, D);
   NewArg->setParent(this);

   // TODO: When we switch to malloc/free allocation we'll be leaking memory
   // here.
   ArgumentList[i] = NewArg;

   return NewArg;
}

PILPhiArgument *PILBasicBlock::replacePhiArgumentAndReplaceAllUses(
   unsigned i, PILType ty, ValueOwnershipKind kind, const ValueDecl *d) {
   // Put in an undef placeholder before we do the replacement since
   // replacePhiArgument() expects the replaced argument to not have
   // any uses.
   SmallVector<Operand *, 16> operands;
   PILValue undef = PILUndef::get(ty, *getParent());
   for (auto *use : getArgument(i)->getUses()) {
      use->set(undef);
      operands.push_back(use);
   }

   // Perform the replacement.
   auto *newArg = replacePhiArgument(i, ty, kind, d);

   // Wire back up the uses.
   while (!operands.empty()) {
      operands.pop_back_val()->set(newArg);
   }

   return newArg;
}

PILPhiArgument *PILBasicBlock::createPhiArgument(PILType Ty,
                                                 ValueOwnershipKind Kind,
                                                 const ValueDecl *D) {
   assert(!isEntry() && "PHI Arguments can not be in the entry block");
   if (Ty.isTrivial(*getParent()))
      Kind = ValueOwnershipKind::None;
   return new (getModule()) PILPhiArgument(this, Ty, Kind, D);
}

PILPhiArgument *PILBasicBlock::insertPhiArgument(arg_iterator Iter, PILType Ty,
                                                 ValueOwnershipKind Kind,
                                                 const ValueDecl *D) {
   assert(!isEntry() && "PHI Arguments can not be in the entry block");
   if (Ty.isTrivial(*getParent()))
      Kind = ValueOwnershipKind::None;
   return new (getModule()) PILPhiArgument(this, Iter, Ty, Kind, D);
}

void PILBasicBlock::eraseArgument(int Index) {
   assert(getArgument(Index)->use_empty() &&
          "Erasing block argument that has uses!");
   // Notify the delete handlers that this BB argument is going away.
   getModule().notifyDeleteHandlers(getArgument(Index));
   ArgumentList.erase(ArgumentList.begin() + Index);
}

/// Splits a basic block into two at the specified instruction.
///
/// Note that all the instructions BEFORE the specified iterator
/// stay as part of the original basic block. The old basic block is left
/// without a terminator.
PILBasicBlock *PILBasicBlock::split(iterator I) {
   PILBasicBlock *New =
      new (Parent->getModule()) PILBasicBlock(Parent, this, /*after*/true);
   // Move all of the specified instructions from the original basic block into
   // the new basic block.
   New->InstList.splice(New->end(), InstList, I, end());
   return New;
}

/// Move the basic block to after the specified basic block in the IR.
void PILBasicBlock::moveAfter(PILBasicBlock *After) {
   assert(getParent() && getParent() == After->getParent() &&
          "Blocks must be in the same function");
   auto InsertPt = std::next(PILFunction::iterator(After));
   auto &BlkList = getParent()->getBlocks();
   BlkList.splice(InsertPt, BlkList, this);
}

void PILBasicBlock::moveTo(PILBasicBlock::iterator To, PILInstruction *I) {
   assert(I->getParent() != this && "Must move from different basic block");
   InstList.splice(To, I->getParent()->InstList, I);
   ScopeCloner ScopeCloner(*Parent);
   I->setDebugScope(ScopeCloner.getOrCreateClonedScope(I->getDebugScope()));
}

void
llvm::ilist_traits<polar::PILBasicBlock>::
transferNodesFromList(llvm::ilist_traits<PILBasicBlock> &SrcTraits,
                      block_iterator First, block_iterator Last) {
   assert(&Parent->getModule() == &SrcTraits.Parent->getModule() &&
          "Module mismatch!");

   // If we are asked to splice into the same function, don't update parent
   // pointers.
   if (Parent == SrcTraits.Parent)
      return;

   ScopeCloner ScopeCloner(*Parent);

   // If splicing blocks not in the same function, update the parent pointers.
   for (; First != Last; ++First) {
      First->Parent = Parent;
      for (auto &II : *First)
         II.setDebugScope(ScopeCloner.getOrCreateClonedScope(II.getDebugScope()));
   }
}

/// ScopeCloner expects NewFn to be a clone of the original
/// function, with all debug scopes and locations still pointing to
/// the original function.
ScopeCloner::ScopeCloner(PILFunction &NewFn) : NewFn(NewFn) {
   // Some clients of PILCloner copy over the original function's
   // debug scope. Create a new one here.
   // FIXME: Audit all call sites and make them create the function
   // debug scope.
   auto *PILFn = NewFn.getDebugScope()->Parent.get<PILFunction *>();
   if (PILFn != &NewFn) {
      PILFn->setInlined();
      NewFn.setDebugScope(getOrCreateClonedScope(NewFn.getDebugScope()));
   }
}

const PILDebugScope *
ScopeCloner::getOrCreateClonedScope(const PILDebugScope *OrigScope) {
   if (!OrigScope)
      return nullptr;

   auto it = ClonedScopeCache.find(OrigScope);
   if (it != ClonedScopeCache.end())
      return it->second;

   auto ClonedScope = new (NewFn.getModule()) PILDebugScope(*OrigScope);
   if (OrigScope->InlinedCallSite) {
      // For inlined functions, we need to rewrite the inlined call site.
      ClonedScope->InlinedCallSite =
         getOrCreateClonedScope(OrigScope->InlinedCallSite);
   } else {
      if (auto *ParentScope = OrigScope->Parent.dyn_cast<const PILDebugScope *>())
         ClonedScope->Parent = getOrCreateClonedScope(ParentScope);
      else
         ClonedScope->Parent = &NewFn;
   }
   // Create an inline scope for the cloned instruction.
   assert(ClonedScopeCache.find(OrigScope) == ClonedScopeCache.end());
   ClonedScopeCache.insert({OrigScope, ClonedScope});
   return ClonedScope;
}

bool PILBasicBlock::isEntry() const {
   return this == &*getParent()->begin();
}

/// Declared out of line so we can have a declaration of PILArgument.
#define ARGUMENT(NAME, PARENT)                                                 \
  NAME##ArrayRef PILBasicBlock::get##NAME##s() const {                         \
    return NAME##ArrayRef(getArguments(),                                      \
                          [](PILArgument *arg) { return cast<NAME>(arg); });   \
  }
#include "polarphp/pil/lang/PILNodesDef.h"

/// Returns true if this block ends in an unreachable or an apply of a
/// no-return apply or builtin.
bool PILBasicBlock::isNoReturn() const {
   if (isa<UnreachableInst>(getTerminator()))
      return true;

   auto Iter = prev_or_begin(getTerminator()->getIterator(), begin());
   FullApplySite FAS = FullApplySite::isa(const_cast<PILInstruction *>(&*Iter));
   if (FAS && FAS.isCalleeNoReturn()) {
      return true;
   }

   if (auto *BI = dyn_cast<BuiltinInst>(&*Iter)) {
      return BI->getModule().isNoReturnBuiltinOrIntrinsic(BI->getName());
   }

   return false;
}

bool PILBasicBlock::isTrampoline() const {
   auto *Branch = dyn_cast<BranchInst>(getTerminator());
   if (!Branch)
      return false;
   return begin() == Branch->getIterator();
}

bool PILBasicBlock::isLegalToHoistInto() const {
   return true;
}

const PILDebugScope *PILBasicBlock::getScopeOfFirstNonMetaInstruction() {
   for (auto &Inst : *this)
      if (Inst.isMetaInstruction())
         return Inst.getDebugScope();
   return begin()->getDebugScope();
}
