//===--- DebugUtils.cpp ---------------------------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/pil/lang/DebugUtils.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "llvm/ADT/PointerIntPair.h"
#include "llvm/ADT/SmallPtrSet.h"

namespace polar {

bool hasNonTrivialNonDebugTransitiveUsers(
   PointerUnion<PILInstruction *, PILArgument *> V) {
   llvm::SmallVector<PILInstruction *, 8> Worklist;
   llvm::SmallPtrSet<PILInstruction *, 8> SeenInsts;

   // Initialize our worklist.
   if (auto *A = V.dyn_cast<PILArgument *>()) {
      for (Operand *Op : getNonDebugUses(PILValue(A))) {
         auto *User = Op->getUser();
         if (!SeenInsts.insert(User).second)
            continue;
         Worklist.push_back(User);
      }
   } else {
      auto *I = V.get<PILInstruction *>();
      SeenInsts.insert(I);
      Worklist.push_back(I);
   }

   while (!Worklist.empty()) {
      PILInstruction *U = Worklist.pop_back_val();
      assert(SeenInsts.count(U) &&
             "Worklist should only contain seen instructions?!");

      // If U is a terminator inst, return false.
      if (isa<TermInst>(U))
         return true;

      // If U has side effects...
      if (U->mayHaveSideEffects())
         return true;

      // Otherwise add all non-debug uses of I that we have not seen yet to the
      // worklist.
      for (PILValue Result : U->getResults()) {
         for (Operand *I : getNonDebugUses(Result)) {
            auto *User = I->getUser();
            if (!SeenInsts.insert(User).second)
               continue;
            Worklist.push_back(User);
         }
      }
   }
   return false;
}

} // polar