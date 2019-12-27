//===--- Cache-Mac.cpp - Caching mechanism implementation -----------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
//===----------------------------------------------------------------------===//
//
//  This file implements the caching mechanism using darwin's libcache.
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/Cache.h"
#include "llvm/ADT/SmallString.h"
#include <cache.h>

namespace polar::sys {

using llvm::StringRef;

CacheImpl::ImplTy CacheImpl::create(StringRef Name, const CallBacks &callbacks)
{
   llvm::SmallString<32> NameBuf(Name);
   cache_attributes_t attrs = {
      CACHE_ATTRIBUTES_VERSION_2,
      callbacks.keyHashCB,
      callbacks.keyIsEqualCB,
      nullptr,
      callbacks.keyDestroyCB,
      callbacks.valueReleaseCB,
      nullptr,
      nullptr,
      callbacks.userData,
      callbacks.valueRetainCB,
   };

   cache_t *cacheOut = nullptr;
   cache_create(NameBuf.c_str(), &attrs, &cacheOut);
   assert(cacheOut);
   return cacheOut;
}

void CacheImpl::setAndRetain(void *key, void *value, size_t cost)
{
   cache_set_and_retain(static_cast<cache_t*>(impl), key, value, cost);
}

bool CacheImpl::getAndRetain(const void *key, void **valueOut)
{
   int ret = cache_get_and_retain(static_cast<cache_t*>(impl),
                                  const_cast<void*>(key), valueOut);
   return ret == 0;
}

void CacheImpl::releaseValue(void *value)
{
   cache_release_value(static_cast<cache_t*>(impl), value);
}

bool CacheImpl::remove(const void *key)
{
   int ret = cache_remove(static_cast<cache_t*>(impl), const_cast<void*>(key));
   return ret == 0;
}

void CacheImpl::removeAll()
{
   cache_remove_all(static_cast<cache_t*>(impl));
}

void CacheImpl::destroy()
{
   cache_destroy(static_cast<cache_t*>(impl));
}

} // polar::sys
