//===--- PILAllocated.h - Defines the PILAllocated class --------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_PILALLOCATED_H
#define POLARPHP_PIL_PILALLOCATED_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/global/CompilerFeature.h"
#include "llvm/Support/ErrorHandling.h"
#include <stddef.h>

namespace polar {

class PILModule;

/// PILAllocated - This class enforces that derived classes are allocated out of
/// the PILModule bump pointer allocator.
template <typename DERIVED>
class PILAllocated {
public:
   /// Disable non-placement new.
   void *operator new(size_t) = delete;
   void *operator new[](size_t) = delete;

   /// Disable non-placement delete.
   void operator delete(void *) POLAR_DELETE_OPERATOR_DELETED;
   void operator delete[](void *) = delete;

   /// Custom version of 'new' that uses the PILModule's BumpPtrAllocator with
   /// precise alignment knowledge.  This is templated on the allocator type so
   /// that this doesn't require including PILModule.h.
   template <typename ContextTy>
   void *operator new(size_t Bytes, const ContextTy &C,
                      size_t Alignment = alignof(DERIVED)) {
      return C.allocate(Bytes, Alignment);
   }
};

} // end polar namespace

#endif
