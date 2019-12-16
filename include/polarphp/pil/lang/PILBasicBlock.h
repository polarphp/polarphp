//===--- PILBasicBlock.h - Basic blocks for PIL -----------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_BASICBLOCK_H
#define POLARPHP_PIL_BASICBLOCK_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/basic/Range.h"
#include "polarphp/pil/lang/PILArgumentArrayRef.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "llvm/ADT/ilist_node.h"
#include "llvm/ADT/ilist.h"

namespace polar {

class PILFunction;
class PILArgument;
class PILPrintContext;

class PILBasicBlock :
   public llvm::ilist_node<PILBasicBlock>, public PILAllocated<PILBasicBlock> {
   friend class PILSuccessor;
   friend class PILFunction;
   friend class PILGlobalVariable;
public:
   using InstListType = llvm::iplist<PILInstruction>;
private:
   /// A backreference to the containing PILFunction.
   PILFunction *Parent;

   /// PrevList - This is a list of all of the terminator operands that are
   /// branching to this block, forming the predecessor list.  This is
   /// automatically managed by the PILSuccessor class.
   PILSuccessor *PredList;

   /// This is the list of basic block arguments for this block.
   std::vector<PILArgument *> ArgumentList;

   /// The ordered set of instructions in the PILBasicBlock.
   InstListType InstList;

   friend struct llvm::ilist_traits<PILBasicBlock>;
   PILBasicBlock() : Parent(nullptr) {}
   void operator=(const PILBasicBlock &) = delete;

   void operator delete(void *Ptr, size_t) POLAR_DELETE_OPERATOR_DELETED;

   PILBasicBlock(PILFunction *F, PILBasicBlock *relativeToBB, bool after);

public:
   ~PILBasicBlock();

   /// Gets the ID (= index in the function's block list) of the block.
   ///
   /// Returns -1 if the block is not contained in a function.
   /// Warning: This function is slow. Therefore it should only be used for
   ///          debug output.
   int getDebugID() const;

   PILFunction *getParent() { return Parent; }
   const PILFunction *getParent() const { return Parent; }

   PILModule &getModule() const;

   /// This method unlinks 'self' from the containing PILFunction and deletes it.
   void eraseFromParent();

   //===--------------------------------------------------------------------===//
   // PILInstruction List Inspection and Manipulation
   //===--------------------------------------------------------------------===//

   using iterator = InstListType::iterator;
   using const_iterator = InstListType::const_iterator;
   using reverse_iterator = InstListType::reverse_iterator;
   using const_reverse_iterator = InstListType::const_reverse_iterator;

   void insert(iterator InsertPt, PILInstruction *I);
   void insert(PILInstruction *InsertPt, PILInstruction *I) {
      insert(InsertPt->getIterator(), I);
   }

   void push_back(PILInstruction *I);
   void push_front(PILInstruction *I);
   void remove(PILInstruction *I);
   iterator erase(PILInstruction *I);

   PILInstruction &back() { return InstList.back(); }
   const PILInstruction &back() const {
      return const_cast<PILBasicBlock *>(this)->back();
   }

   PILInstruction &front() { return InstList.front(); }
   const PILInstruction &front() const {
      return const_cast<PILBasicBlock *>(this)->front();
   }

   /// Transfer the instructions from Other to the end of this block.
   void spliceAtEnd(PILBasicBlock *Other) {
      InstList.splice(end(), Other->InstList);
   }

   bool empty() const { return InstList.empty(); }
   iterator begin() { return InstList.begin(); }
   iterator end() { return InstList.end(); }
   const_iterator begin() const { return InstList.begin(); }
   const_iterator end() const { return InstList.end(); }
   reverse_iterator rbegin() { return InstList.rbegin(); }
   reverse_iterator rend() { return InstList.rend(); }
   const_reverse_iterator rbegin() const { return InstList.rbegin(); }
   const_reverse_iterator rend() const { return InstList.rend(); }

   TermInst *getTerminator() {
      assert(!InstList.empty() && "Can't get successors for malformed block");
      return cast<TermInst>(&InstList.back());
   }

   const TermInst *getTerminator() const {
      return const_cast<PILBasicBlock *>(this)->getTerminator();
   }

   /// Splits a basic block into two at the specified instruction.
   ///
   /// Note that all the instructions BEFORE the specified iterator
   /// stay as part of the original basic block. The old basic block is left
   /// without a terminator.
   PILBasicBlock *split(iterator I);

   /// Move the basic block to after the specified basic block in the IR.
   ///
   /// Assumes that the basic blocks must reside in the same function. In asserts
   /// builds, an assert verifies that this is true.
   void moveAfter(PILBasicBlock *After);

   /// Moves the instruction to the iterator in this basic block.
   void moveTo(PILBasicBlock::iterator To, PILInstruction *I);

   //===--------------------------------------------------------------------===//
   // PILBasicBlock Argument List Inspection and Manipulation
   //===--------------------------------------------------------------------===//

   using arg_iterator = std::vector<PILArgument *>::iterator;
   using const_arg_iterator = std::vector<PILArgument *>::const_iterator;

   bool args_empty() const { return ArgumentList.empty(); }
   size_t args_size() const { return ArgumentList.size(); }
   arg_iterator args_begin() { return ArgumentList.begin(); }
   arg_iterator args_end() { return ArgumentList.end(); }
   const_arg_iterator args_begin() const { return ArgumentList.begin(); }
   const_arg_iterator args_end() const { return ArgumentList.end(); }

   /// Iterator over the PHI arguments of a basic block.
   /// Defines an implicit cast operator on the iterator, so that this iterator
   /// can be used in the SSAUpdaterImpl.
   template <typename PHIArgT = PILPhiArgument,
      typename IteratorT = arg_iterator>
   class phi_iterator_impl {
   private:
      IteratorT It;

   public:
      explicit phi_iterator_impl(IteratorT A) : It(A) {}
      phi_iterator_impl &operator++() { ++It; return *this; }

      operator PHIArgT *() { return cast<PHIArgT>(*It); }
      bool operator==(const phi_iterator_impl& x) const { return It == x.It; }
      bool operator!=(const phi_iterator_impl& x) const { return !operator==(x); }
   };
   typedef phi_iterator_impl<> phi_iterator;
   typedef phi_iterator_impl<const PILPhiArgument,
      PILBasicBlock::const_arg_iterator>
      const_phi_iterator;

   inline iterator_range<phi_iterator> phis() {
      return llvm::make_range(phi_iterator(args_begin()), phi_iterator(args_end()));
   }
   inline iterator_range<const_phi_iterator> phis() const {
      return llvm::make_range(const_phi_iterator(args_begin()),
                        const_phi_iterator(args_end()));
   }

   ArrayRef<PILArgument *> getArguments() const { return ArgumentList; }

   /// Returns a transform array ref that performs llvm::cast<NAME>
   /// each argument and then returns the downcasted value.
#define ARGUMENT(NAME, PARENT) NAME##ArrayRef get##NAME##s() const;
#include "polarphp/pil/lang/PILNodesDef.h"

   unsigned getNumArguments() const { return ArgumentList.size(); }
   const PILArgument *getArgument(unsigned i) const { return ArgumentList[i]; }
   PILArgument *getArgument(unsigned i) { return ArgumentList[i]; }

   void cloneArgumentList(PILBasicBlock *Other);

   /// Erase a specific argument from the arg list.
   void eraseArgument(int Index);

   /// Allocate a new argument of type \p Ty and append it to the argument
   /// list. Optionally you can pass in a value decl parameter.
   PILFunctionArgument *createFunctionArgument(PILType Ty,
                                               const ValueDecl *D = nullptr,
                                               bool disableEntryBlockVerification = false);

   PILFunctionArgument *insertFunctionArgument(unsigned Index, PILType Ty,
                                               ValueOwnershipKind OwnershipKind,
                                               const ValueDecl *D = nullptr) {
      arg_iterator Pos = ArgumentList.begin();
      std::advance(Pos, Index);
      return insertFunctionArgument(Pos, Ty, OwnershipKind, D);
   }

   /// Replace the \p{i}th Function arg with a new Function arg with PILType \p
   /// Ty and ValueDecl \p D.
   PILFunctionArgument *replaceFunctionArgument(unsigned i, PILType Ty,
                                                ValueOwnershipKind Kind,
                                                const ValueDecl *D = nullptr);

   /// Replace the \p{i}th BB arg with a new BBArg with PILType \p Ty and
   /// ValueDecl \p D.
   ///
   /// NOTE: This assumes that the current argument in position \p i has had its
   /// uses eliminated. To replace/replace all uses with, use
   /// replacePhiArgumentAndRAUW.
   PILPhiArgument *replacePhiArgument(unsigned i, PILType type,
                                      ValueOwnershipKind kind,
                                      const ValueDecl *decl = nullptr);

   /// Replace phi argument \p i and RAUW all uses.
   PILPhiArgument *
   replacePhiArgumentAndReplaceAllUses(unsigned i, PILType type,
                                       ValueOwnershipKind kind,
                                       const ValueDecl *decl = nullptr);

   /// Allocate a new argument of type \p Ty and append it to the argument
   /// list. Optionally you can pass in a value decl parameter.
   PILPhiArgument *createPhiArgument(PILType Ty, ValueOwnershipKind Kind,
                                     const ValueDecl *D = nullptr);

   /// Insert a new PILPhiArgument with type \p Ty and \p Decl at position \p
   /// Pos.
   PILPhiArgument *insertPhiArgument(arg_iterator Pos, PILType Ty,
                                     ValueOwnershipKind Kind,
                                     const ValueDecl *D = nullptr);

   PILPhiArgument *insertPhiArgument(unsigned Index, PILType Ty,
                                     ValueOwnershipKind Kind,
                                     const ValueDecl *D = nullptr) {
      arg_iterator Pos = ArgumentList.begin();
      std::advance(Pos, Index);
      return insertPhiArgument(Pos, Ty, Kind, D);
   }

   /// Remove all block arguments.
   void dropAllArguments() { ArgumentList.clear(); }

   //===--------------------------------------------------------------------===//
   // Successors
   //===--------------------------------------------------------------------===//

   using SuccessorListTy = TermInst::SuccessorListTy;
   using ConstSuccessorListTy = TermInst::ConstSuccessorListTy;

   /// The successors of a PILBasicBlock are defined either explicitly as
   /// a single successor as the branch targets of the terminator instruction.
   ConstSuccessorListTy getSuccessors() const {
      return getTerminator()->getSuccessors();
   }
   SuccessorListTy getSuccessors() {
      return getTerminator()->getSuccessors();
   }

   using const_succ_iterator = TermInst::const_succ_iterator;
   using succ_iterator = TermInst::succ_iterator;

   bool succ_empty() const { return getTerminator()->succ_empty(); }
   succ_iterator succ_begin() { return getTerminator()->succ_begin(); }
   succ_iterator succ_end() { return getTerminator()->succ_end(); }
   const_succ_iterator succ_begin() const {
      return getTerminator()->succ_begin();
   }
   const_succ_iterator succ_end() const { return getTerminator()->succ_end(); }

   using succblock_iterator = TermInst::succblock_iterator;
   using const_succblock_iterator = TermInst::const_succblock_iterator;

   succblock_iterator succblock_begin() {
      return getTerminator()->succblock_begin();
   }
   succblock_iterator succblock_end() {
      return getTerminator()->succblock_end();
   }
   const_succblock_iterator succblock_begin() const {
      return getTerminator()->succblock_begin();
   }
   const_succblock_iterator succblock_end() const {
      return getTerminator()->succblock_end();
   }

   PILBasicBlock *getSingleSuccessorBlock() {
      return getTerminator()->getSingleSuccessorBlock();
   }

   const PILBasicBlock *getSingleSuccessorBlock() const {
      return getTerminator()->getSingleSuccessorBlock();
   }

   /// Returns true if \p BB is a successor of this block.
   bool isSuccessorBlock(PILBasicBlock *Block) const {
      return getTerminator()->isSuccessorBlock(Block);
   }

   using SuccessorBlockListTy = TermInst::SuccessorBlockListTy;
   using ConstSuccessorBlockListTy = TermInst::ConstSuccessorBlockListTy;

   /// Return the range of PILBasicBlocks that are successors of this block.
   SuccessorBlockListTy getSuccessorBlocks() {
      return getTerminator()->getSuccessorBlocks();
   }

   /// Return the range of PILBasicBlocks that are successors of this block.
   ConstSuccessorBlockListTy getSuccessorBlocks() const {
      return getTerminator()->getSuccessorBlocks();
   }

   //===--------------------------------------------------------------------===//
   // Predecessors
   //===--------------------------------------------------------------------===//

   using pred_iterator = PILSuccessor::pred_iterator;

   bool pred_empty() const { return PredList == nullptr; }
   pred_iterator pred_begin() const { return pred_iterator(PredList); }
   pred_iterator pred_end() const { return pred_iterator(); }

   iterator_range<pred_iterator> getPredecessorBlocks() const {
      return {pred_begin(), pred_end()};
   }

   bool isPredecessorBlock(PILBasicBlock *BB) const {
      return any_of(
         getPredecessorBlocks(),
         [&BB](const PILBasicBlock *PredBB) -> bool { return BB == PredBB; });
   }

   PILBasicBlock *getSinglePredecessorBlock() {
      if (pred_empty() || std::next(pred_begin()) != pred_end())
         return nullptr;
      return *pred_begin();
   }

   const PILBasicBlock *getSinglePredecessorBlock() const {
      return const_cast<PILBasicBlock *>(this)->getSinglePredecessorBlock();
   }

   //===--------------------------------------------------------------------===//
   // Utility
   //===--------------------------------------------------------------------===//

   /// Returns true if this BB is the entry BB of its parent.
   bool isEntry() const;

   /// Returns true if this block ends in an unreachable or an apply of a
   /// no-return apply or builtin.
   bool isNoReturn() const;

   /// Returns true if this block only contains a branch instruction.
   bool isTrampoline() const;

   /// Returns true if it is legal to hoist instructions into this block.
   ///
   /// Used by llvm::LoopInfo.
   bool isLegalToHoistInto() const;

   /// Returns the debug scope of the first non-meta instructions in the
   /// basic block. PILBuilderWithScope uses this to correctly set up
   /// the debug scope for newly created instructions.
   const PILDebugScope *getScopeOfFirstNonMetaInstruction();

   //===--------------------------------------------------------------------===//
   // Debugging
   //===--------------------------------------------------------------------===//

   /// Pretty-print the PILBasicBlock.
   void dump() const;

   /// Pretty-print the PILBasicBlock with the designated stream.
   void print(llvm::raw_ostream &OS) const;

   /// Pretty-print the PILBasicBlock with the designated stream and context.
   void print(llvm::raw_ostream &OS, PILPrintContext &Ctx) const;

   void printAsOperand(raw_ostream &OS, bool PrintType = true);

   /// getSublistAccess() - returns pointer to member of instruction list
   static InstListType PILBasicBlock::*getSublistAccess() {
      return &PILBasicBlock::InstList;
   }

   /// Drops all uses that belong to this basic block.
   void dropAllReferences() {
      dropAllArguments();
      for (PILInstruction &I : *this)
         I.dropAllReferences();
   }

   void eraseInstructions();

private:
   friend class PILArgument;

   /// BBArgument's ctor adds it to the argument list of this block.
   void insertArgument(arg_iterator Iter, PILArgument *Arg) {
      ArgumentList.insert(Iter, Arg);
   }

   /// Insert a new PILFunctionArgument with type \p Ty and \p Decl at position
   /// \p Pos.
   PILFunctionArgument *insertFunctionArgument(arg_iterator Pos, PILType Ty,
                                               ValueOwnershipKind OwnershipKind,
                                               const ValueDecl *D = nullptr);
};

inline llvm::raw_ostream &operator<<(llvm::raw_ostream &OS,
                                     const PILBasicBlock &BB) {
   BB.print(OS);
   return OS;
}
} // end polar namespace

namespace llvm {

//===----------------------------------------------------------------------===//
// ilist_traits for PILBasicBlock
//===----------------------------------------------------------------------===//
template <>
struct ilist_traits<polar::PILBasicBlock>
   : ilist_node_traits<polar::PILBasicBlock> {
   using SelfTy = ilist_traits<polar::PILBasicBlock>;
   using PILBasicBlock = polar::PILBasicBlock;
   using PILFunction = polar::PILFunction;
   using FunctionPtrTy = polar::NullablePtr<PILFunction>;

private:
   friend class polar::PILFunction;

   PILFunction *Parent;
   using block_iterator = simple_ilist<PILBasicBlock>::iterator;

public:
   static void deleteNode(PILBasicBlock *BB) { BB->~PILBasicBlock(); }

   void transferNodesFromList(ilist_traits<PILBasicBlock> &SrcTraits,
                              block_iterator First, block_iterator Last);
private:
   static void createNode(const PILBasicBlock &);
};

} // end llvm namespace

#endif
