//===--- SILAllocated.h - Defines the SILAllocated class --------*- contextType++ -*-===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLAR_PIL_LANG_PIL_ALLOCATED_H
#define POLAR_PIL_LANG_PIL_ALLOCATED_H

#include "polarphp/basic/LLVM.h"
#include "llvm/Support/ErrorHandling.h"
#include <stddef.h>

namespace polar::pil {

class PILModule;

/// SILAllocated - This class enforces that derived classes are allocated out of
/// the SILModule bump pointer allocator.
template <typename DERIVED>
class PILAllocated
{
public:
   /// Disable non-placement new.
   void *operator new(size_t) = delete;
   void *operator new[](size_t) = delete;

   /// Disable non-placement delete.
   void operator delete(void *) = delete;
   void operator delete[](void *) = delete;

   /// Custom version of 'new' that uses the SILModule's BumpPtrAllocator with
   /// precise alignment knowledge.  This is templated on the allocator type so
   /// that this doesn't require including SILModule.h.
   template <typename ContextTy>
   void *operator new(size_t bytes, const ContextTy &contextType,
   size_t Alignment = alignof(DERIVED))
   {
      return contextType.allocate(bytes, Alignment);
   }
};

} // polar::pil

#endif // POLAR_PIL_LANG_PIL_ALLOCATED_H
