//===--- Cache.cpp - Caching mechanism implementation ---------------------===//
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
// Created by polarboy on 2019/11/30.

//  This file implements a default caching implementation that never evicts
//  its entries.

#include "polarphp/basic/Cache.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/Mutex.h"

namespace polar::sys {

using llvm::StringRef;

namespace {
struct DefaultCacheKey
{
   void *key = nullptr;
   CacheImpl::CallBacks *callbacks = nullptr;

   //DefaultCacheKey() = default;
   DefaultCacheKey(void *key, CacheImpl::CallBacks *callbacks) : key(key), callbacks(callbacks) {}
};

struct DefaultCache
{
   llvm::sys::Mutex mutex;
   CacheImpl::CallBacks callbacks;
   llvm::DenseMap<DefaultCacheKey, void *> entries;

   explicit DefaultCache(CacheImpl::CallBacks callbacks)
      : callbacks(std::move(callbacks)) {}
};
} // end anonymous namespace

} // polar

namespace llvm {
using polar::DefaultCacheKey;

template<>
struct DenseMapInfo<DefaultCacheKey>
{
   static inline DefaultCacheKey getEmptyKey()
   {
      return { DenseMapInfo<void*>::getEmptyKey(), nullptr };
   }

   static inline DefaultCacheKey getTombstoneKey()
   {
      return { DenseMapInfo<void*>::getTombstoneKey(), nullptr };
   }

   static unsigned getHashValue(const DefaultCacheKey &value)
   {
      uintptr_t Hash = value.callbacks->keyHashCB(value.key, nullptr);
      return DenseMapInfo<uintptr_t>::getHashValue(Hash);
   }

   static bool isEqual(const DefaultCacheKey &lhs, const DefaultCacheKey &rhs)
   {
      if (lhs.key == rhs.key) {
         return true;
      }
      if (lhs.key == DenseMapInfo<void*>::getEmptyKey() ||
          lhs.key == DenseMapInfo<void*>::getTombstoneKey() ||
          rhs.key == DenseMapInfo<void*>::getEmptyKey() ||
          rhs.key == DenseMapInfo<void*>::getTombstoneKey()) {
         return false;
      }
      return lhs.callbacks->keyIsEqualCB(lhs.key, rhs.key, nullptr);
   }
};
} // namespace llvm

namespace polar {

CacheImpl::ImplTy CacheImpl::create(StringRef name, const CallBacks &callbacks)
{
   return new DefaultCache(callbacks);
}

void CacheImpl::setAndRetain(void *key, void *value, size_t cost)
{
   DefaultCache &defaultCache = *static_cast<DefaultCache*>(impl);
   llvm::sys::ScopedLock lock(defaultCache.mutex);

   DefaultCacheKey ckey(key, &defaultCache.callbacks);
   auto entry = defaultCache.entries.find(ckey);
   if (entry != defaultCache.entries.end()) {
      if (entry->second == value) {
         return;
      }
      defaultCache.callbacks.keyDestroyCB(entry->first.key, nullptr);
      defaultCache.callbacks.valueReleaseCB(entry->second, nullptr);
      defaultCache.entries.erase(entry);
   }

   defaultCache.callbacks.valueRetainCB(value, nullptr);
   defaultCache.entries[ckey] = value;

   // FIXME: Not thread-safe! It should avoid deleting the value until
   // 'releaseValue is called on it.
}

bool CacheImpl::getAndRetain(const void *key, void **valueOut)
{
   DefaultCache &defaultCache = *static_cast<DefaultCache*>(impl);
   llvm::sys::ScopedLock lock(defaultCache.mutex);

   DefaultCacheKey ckey(const_cast<void*>(key), &defaultCache.callbacks);
   auto entry = defaultCache.entries.find(ckey);
   if (entry != defaultCache.entries.end()) {
      // FIXME: Not thread-safe! It should avoid deleting the value until
      // 'releaseValue is called on it.
      *valueOut = entry->second;
      return true;
   }
   return false;
}

void CacheImpl::releaseValue(void *value)
{
   // FIXME: Implementation.
}

bool CacheImpl::remove(const void *key)
{
   DefaultCache &defaultCache = *static_cast<DefaultCache*>(impl);
   llvm::sys::ScopedLock lock(defaultCache.mutex);

   DefaultCacheKey ckey(const_cast<void*>(key), &defaultCache.callbacks);
   auto entry = defaultCache.entries.find(ckey);
   if (entry != defaultCache.entries.end()) {
      defaultCache.callbacks.keyDestroyCB(entry->first.key, nullptr);
      defaultCache.callbacks.valueReleaseCB(entry->second, nullptr);
      defaultCache.entries.erase(entry);
      return true;
   }
   return false;
}

void CacheImpl::removeAll()
{
   DefaultCache &defaultCache = *static_cast<DefaultCache*>(impl);
   llvm::sys::ScopedLock lock(defaultCache.mutex);

   for (auto entry : defaultCache.entries) {
      defaultCache.callbacks.keyDestroyCB(entry.first.key, nullptr);
      defaultCache.callbacks.valueReleaseCB(entry.second, nullptr);
   }
   defaultCache.entries.clear();
}

void CacheImpl::destroy()
{
   removeAll();
   delete static_cast<DefaultCache*>(impl);
}

} // polar::sys
