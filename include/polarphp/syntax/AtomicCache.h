//===------------- AtomicCache.h - Lazy Atomic Cache ------------*- C++ -*-===//
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
// Created by polarboy on 2019/05/07.

#ifndef POLARPHP_SYNTAX_ATOMICCACHE_H
#define POLARPHP_SYNTAX_ATOMICCACHE_H

#include <functional>
#include "polarphp/syntax/References.h"
#include "llvm/ADT/STLExtras.h"

namespace polar::syntax {

/// AtomicCache is an atomic cache for a reference-counted value. It maintains
/// a reference-counted pointer with a facility for atomically getting or
/// creating it with a lambda.
template <typename T>
class AtomicCache
{
   // Whatever type is created through this cache must be pointer-sized,
   // othwerise, we can't pretend it's a uintptr_t and use its
   // compare_exchange_strong.
   static_assert(sizeof(uintptr_t) == sizeof(RefCountPtr<T>),
                 "RefCountPtr<T> must be pointer sized!");

public:
   /// The empty constructor initializes the storage to nullptr.
   AtomicCache()
   {}

   /// Gets the value inside the cache, or creates it atomically using the
   /// provided lambda if it doesn't already exist.
   RefCountPtr<T> getOrCreate(llvm::function_ref<RefCountPtr<T>()> create) const
   {
      auto &ptr = *reinterpret_cast<std::atomic<uintptr_t> *>(&m_storage);
      // If an atomic load gets an initialized value, then return m_storage.
      if (ptr) {
         return m_storage;
      }
      // We expect the uncached value to wrap a nullptr. If another thread
      // beats us to caching the child, it'll be non-null, so we would
      // leave it alone.
      uintptr_t expected = 0;
      // Make a RefCountPtr<T> at RefCount == 1, which we'll try to
      // atomically swap in.
      auto data = create();
      // Try to swap in raw pointer value.
      // If we won, then leave the RefCount == 1.
      if (ptr.compare_exchange_strong(expected,
                                      reinterpret_cast<uintptr_t>(data.get()))) {
         data.resetWithoutRelease();
      }
      // Otherwise, the data we just made is unfortunately useless.
      // Let it die on this scope exit after its terminal release.
      return m_storage;
   }

private:
   /// This must only be mutated in one place: AtomicCache::getOrCreate.
   mutable RefCountPtr<T> m_storage = nullptr;
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_ATOMICCACHE_H
