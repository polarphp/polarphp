//===--- PILSSAUpdater.h - Unstructured SSA Update Tool ---------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_UTILS_PILSSAUPDATER_H
#define POLARPHP_PIL_OPTIMIZER_UTILS_PILSSAUPDATER_H

#include "llvm/Support/Allocator.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILValue.h"

namespace llvm {
template<typename T> class SSAUpdaterTraits;
template<typename T> class SmallVectorImpl;
}

namespace polar {

class PILPhiArgument;
class PILBasicBlock;
class PILType;
class PILUndef;

/// Independent utility that canonicalizes BB arguments by reusing structurally
/// equivalent arguments and replacing the original arguments with casts.
PILValue replaceBBArgWithCast(PILPhiArgument *Arg);

/// This class updates SSA for a set of PIL instructions defined in multiple
/// blocks.
class PILSSAUpdater {
   friend class llvm::SSAUpdaterTraits<PILSSAUpdater>;

   // A map of basic block to available phi value.
   using AvailableValsTy = llvm::DenseMap<PILBasicBlock *, PILValue>;
   std::unique_ptr<AvailableValsTy> AV;

   PILType ValType;

   // The SSAUpdaterTraits specialization uses this sentinel to mark 'new' phi
   // nodes (all the incoming edge arguments have this sentinel set).
   std::unique_ptr<PILUndef, void(*)(PILUndef *)> PHISentinel;

   // If not null updated with inserted 'phi' nodes (PILArgument).
   SmallVectorImpl<PILPhiArgument *> *InsertedPHIs;

   // Not copyable.
   void operator=(const PILSSAUpdater &) = delete;
   PILSSAUpdater(const PILSSAUpdater &) = delete;

public:
   explicit PILSSAUpdater(
      SmallVectorImpl<PILPhiArgument *> *InsertedPHIs = nullptr);
   ~PILSSAUpdater();

   void setInsertedPhis(SmallVectorImpl<PILPhiArgument *> *insertedPhis) {
      InsertedPHIs = insertedPhis;
   }

   /// Initialize for a use of a value of type.
   void Initialize(PILType T);

   bool HasValueForBlock(PILBasicBlock *BB) const;
   void AddAvailableValue(PILBasicBlock *BB, PILValue V);

   /// Construct SSA for a value that is live at the *end* of a basic block.
   PILValue GetValueAtEndOfBlock(PILBasicBlock *BB);

   /// Construct SSA for a value that is live in the middle of a block.
   /// This handles the case where the use is before a definition of the value.
   ///  BB1:
   ///    val_1 = def
   ///    br BB2:
   ///  BB2:
   ///         = use(val_?)
   ///    val_2 = def
   ///    cond_br ..., BB2, BB3
   ///
   /// In this case we need to insert a 'PHI' node at the beginning of BB2
   /// merging val_1 and val_2.
   PILValue GetValueInMiddleOfBlock(PILBasicBlock *BB);

   void RewriteUse(Operand &Op);

   void *allocate(unsigned Size, unsigned Align) const;
   static void deallocateSentinel(PILUndef *U);
private:

   PILValue GetValueAtEndOfBlockInternal(PILBasicBlock *BB);
};

/// Utility to wrap 'Operand's to deal with invalidation of
/// ValueUseIterators during SSA construction.
///
/// Uses in branches change under us - we need to identify them by an
/// indirection. A ValueUseIterator is just an Operand pointer. As we update SSA
/// form we change branches and invalidate (by deleting the old branch and
/// creating a new one) the Operand pointed to by the ValueUseIterator.
///
/// This class wraps such uses (uses in branches) to provide a level of
/// indirection. We can restore the information - the use - by looking at the
/// new branch and the operand index.
///
/// Uses in branches are stored as an index and the parent block to
/// identify the use allowing us to reconstruct the use after the branch has
/// been changed.
class UseWrapper {
   Operand *U;
   PILBasicBlock *Parent;
   enum {
      kRegularUse,
      kBranchUse,
      kCondBranchUseTrue,
      kCondBranchUseFalse
   } Type;
   unsigned Idx;

public:

   /// Construct a use wrapper. For branches we store information so that
   /// we can reconstruct the use after the branch has been modified.
   ///
   /// When a branch is modified existing pointers to the operand
   /// (ValueUseIterator) become invalid as they point to freed operands.
   /// Instead we store the branch's parent and the idx so that we can
   /// reconstruct the use.
   UseWrapper(Operand *Use);

   Operand *getOperand();

   /// Return the operand we wrap. Reconstructing branch operands.
   operator Operand*() { return getOperand(); }
};

} // end namespace polar
#endif
