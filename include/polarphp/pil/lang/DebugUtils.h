//===--- DebugUtils.h - Utilities for debug-info instructions ---*- C++ -*-===//
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
// This file contains utilities to work with debug-info related instructions:
// debug_value and debug_value_addr.
//
// PIL optimizations should deal with debug-info related instructions when
// looking at the uses of a value.
// When performing an analysis, the usual thing is to just ignore all debug-info
// instructions.
// When transforming the PIL, a pass must decide what to do with debug-info
// instructions. Either delete them (if their value is no longer available),
// keep them (if the transformation has no effect on debug-info values) or
// update them.
//
// To ignore debug-info instructions during an analysis, this file provides
// some utility functions, which can be used instead of the relevant member
// functions in ValueBase and PILValue:
//
// V->use_empty()        ->  onlyHaveDebugUses(V)
// V.hasOneUse()         ->  hasOneNonDebugUse(V)
// V.getUses()           ->  getNonDebugUses(V)
// I->eraseFromParent()  ->  eraseFromParentWithDebugInsts(I)
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_DEBUGUTILS_H
#define POLARPHP_PIL_DEBUGUTILS_H

#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILInstruction.h"

namespace polar {

class PILInstruction;

/// Deletes all of the debug instructions that use \p value.
inline void deleteAllDebugUses(PILValue value) {
   for (auto ui = value->use_begin(), ue = value->use_end(); ui != ue;) {
      auto *inst = ui->getUser();
      ++ui;
      if (inst->isDebugInstruction()) {
         inst->eraseFromParent();
      }
   }
}

/// Deletes all of the debug uses of any result of \p inst.
inline void deleteAllDebugUses(PILInstruction *inst) {
   for (PILValue v : inst->getResults()) {
      deleteAllDebugUses(v);
   }
}

/// This iterator filters out any debug (or non-debug) instructions from a range
/// of uses, provided by the underlying ValueBaseUseIterator.
/// If \p nonDebugInsts is true, then the iterator provides a view to all non-
/// debug instructions. Otherwise it provides a view ot all debug-instructions.
template <bool nonDebugInsts> class DebugUseIterator
   : public std::iterator<std::forward_iterator_tag, Operand *, ptrdiff_t> {

   ValueBaseUseIterator BaseIterator;

   // Skip any debug or non-debug instructions (depending on the nonDebugInsts
   // template argument).
   void skipInsts() {
      while (true) {
         if (*BaseIterator == nullptr)
            return;

         PILInstruction *User = BaseIterator->getUser();
         if (User->isDebugInstruction() != nonDebugInsts)
            return;

         BaseIterator++;
      }
   }

public:

   DebugUseIterator(ValueBaseUseIterator BaseIterator) :
      BaseIterator(BaseIterator) {
      skipInsts();
   }

   DebugUseIterator() = default;

   Operand *operator*() const { return *BaseIterator; }
   Operand *operator->() const { return *BaseIterator; }
   PILInstruction *getUser() const { return BaseIterator.getUser(); }

   DebugUseIterator &operator++() {
      BaseIterator++;
      skipInsts();
      return *this;
   }

   DebugUseIterator operator++(int unused) {
      DebugUseIterator Copy = *this;
      ++*this;
      return Copy;
   }
   friend bool operator==(DebugUseIterator lhs,
                          DebugUseIterator rhs) {
      return lhs.BaseIterator == rhs.BaseIterator;
   }
   friend bool operator!=(DebugUseIterator lhs,
                          DebugUseIterator rhs) {
      return !(lhs == rhs);
   }
};

/// Iterator for iteration over debug instructions.
using DUIterator = DebugUseIterator<false>;

/// Iterator for iteration over non-debug instructions.
using NonDUIterator = DebugUseIterator<true>;


/// Returns a range of all debug instructions in the uses of a value (e.g.
/// PILValue or PILInstruction).
inline iterator_range<DUIterator> getDebugUses(PILValue V) {
   return make_range(DUIterator(V->use_begin()), DUIterator(V->use_end()));
}

/// Returns a range of all non-debug instructions in the uses of a value (e.g.
/// PILValue or PILInstruction).
inline iterator_range<NonDUIterator> getNonDebugUses(PILValue V) {
   return make_range(NonDUIterator(V->use_begin()), NonDUIterator(V->use_end()));
}

/// Returns true if a value (e.g. PILInstruction) has no uses except debug
/// instructions.
inline bool onlyHaveDebugUses(PILValue V) {
   auto NonDebugUses = getNonDebugUses(V);
   return NonDebugUses.begin() == NonDebugUses.end();
}

/// Return true if all of the results of the given instruction have no uses
/// except debug instructions.
inline bool onlyHaveDebugUsesOfAllResults(PILInstruction *I) {
   for (auto result : I->getResults()) {
      if (!onlyHaveDebugUses(result))
         return false;
   }
   return true;
}

/// Returns true if a value (e.g. PILInstruction) has exactly one use which is
/// not a debug instruction.
inline bool hasOneNonDebugUse(PILValue V) {
   auto Range = getNonDebugUses(V);
   auto I = Range.begin(), E = Range.end();
   if (I == E) return false;
   return ++I == E;
}

// Returns the use if the value has only one non debug user.
inline PILInstruction *getSingleNonDebugUser(PILValue V) {
   auto Range = getNonDebugUses(V);
   auto I = Range.begin(), E = Range.end();
   if (I == E) return nullptr;
   if (std::next(I) != E)
      return nullptr;
   return I->getUser();
}

/// Erases the instruction \p I from it's parent block and deletes it, including
/// all debug instructions which use \p I.
/// Precondition: The instruction may only have debug instructions as uses.
/// If the iterator \p InstIter references any deleted instruction, it is
/// incremented.
///
/// \p callBack will be invoked before each instruction is deleted. \p callBack
/// is not responsible for deleting the instruction because this utility
/// unconditionally deletes the \p I and its debug users.
///
/// Returns an iterator to the next non-deleted instruction after \p I.
inline PILBasicBlock::iterator eraseFromParentWithDebugInsts(
   PILInstruction *I, llvm::function_ref<void(PILInstruction *)> callBack =
[](PILInstruction *) {}) {

   auto nextII = std::next(I->getIterator());

   auto results = I->getResults();

   bool foundAny;
   do {
      foundAny = false;
      for (auto result : results) {
         while (!result->use_empty()) {
            foundAny = true;
            auto *User = result->use_begin()->getUser();
            assert(User->isDebugInstruction());
            if (nextII == User->getIterator())
               nextII++;
            callBack(User);
            User->eraseFromParent();
         }
      }
   } while (foundAny);

   I->eraseFromParent();
   return nextII;
}

/// Return true if the def-use graph rooted at \p V contains any non-debug,
/// non-trivial users.
bool hasNonTrivialNonDebugTransitiveUsers(
   PointerUnion<PILInstruction *, PILArgument *> V);

} // end namespace polar

#endif // POLARPHP_PIL_DEBUGUTILS_H
