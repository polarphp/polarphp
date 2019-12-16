//===--- JumpDest.h - Jump Destination Representation -----------*- C++ -*-===//
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
// Types relating to branch destinations.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_GEN_JUMPDEST_H
#define POLARPHP_PIL_GEN_JUMPDEST_H

#include "polarphp/pil/lang/PILLocation.h"
#include "llvm/Support/Compiler.h"
#include "polarphp/pil/gen/Cleanup.h"

namespace polar {
class PILBasicBlock;
class CaseStmt;

namespace lowering {

/// The destination of a direct jump.  Swift currently does not
/// support indirect branches or goto, so the jump mechanism only
/// needs to worry about branches out of scopes, not into them.
class LLVM_LIBRARY_VISIBILITY JumpDest {
   PILBasicBlock *Block = nullptr;
   CleanupsDepth Depth = CleanupsDepth::invalid();
   CleanupLocation CleanupLoc;
public:
   JumpDest(CleanupLocation L) : CleanupLoc(L) {}

   JumpDest(PILBasicBlock *block, CleanupsDepth depth, CleanupLocation l)
      : Block(block), Depth(depth), CleanupLoc(l) {}

   PILBasicBlock *getBlock() const { return Block; }

   PILBasicBlock *takeBlock() {
      auto *BB = Block;
      Block = nullptr;
      return BB;
   }

   CleanupsDepth getDepth() const { return Depth; }

   CleanupLocation getCleanupLocation() const { return CleanupLoc; }

   JumpDest translate(CleanupsDepth NewDepth) &&{
      JumpDest NewValue(Block, NewDepth, CleanupLoc);
      Block = nullptr;
      Depth = CleanupsDepth::invalid();
      // Null location.
      CleanupLoc = CleanupLocation::get(ArtificialUnreachableLocation());
      return NewValue;
   }

   bool isValid() const { return Block != nullptr; }

   static JumpDest invalid() {
      return JumpDest(CleanupLocation((Expr * )
      nullptr));
   }
};

} // lowering
} // end namespace polar

#endif // POLARPHP_PIL_GEN_JUMPDEST_H
