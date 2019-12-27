//===--- Cache.h - Caching mechanism interface ------------------*- C++ -*-===//
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

#ifndef POLARPHP_BASIC_CACHE_H
#define POLARPHP_BASIC_CACHE_H

#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/ADT/IntrusiveRefCntPtr.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/Optional.h"

namespace polar::sys {

template <typename T>
struct CacheKeyHashInfo
{
   static uintptr_t getHashValue(const T &value)
   {
      return llvm::DenseMapInfo<T>::getHashValue(value);
   }

   static bool isEqual(void *lhs, void *rhs)
   {
      return llvm::DenseMapInfo<T>::isEqual(*static_cast<T*>(lhs),
                                            *static_cast<T*>(rhs));
   }
};

template <typename T>
struct CacheKeyInfo : public CacheKeyHashInfo<T>
{
   static void *enterCache(const T &value)
   {
      return new T(value);
   }

   static void exitCache(void *ptr)
   {
      delete static_cast<T*>(ptr);
   }

   static const void *getLookupKey(const T *value)
   {
      return value;
   }

   static const T &getFromCache(void *ptr)
   {
      return *static_cast<T*>(ptr);
   }
};

template <typename T>
struct CacheValueCostInfo
{
   static size_t getCost(const T &value)
   {
      return sizeof(value);
   }
};

template <typename T>
struct CacheValueInfo : public CacheValueCostInfo<T>
{
   static void *enterCache(const T &value)
   {
      return new T(value);
   }

   static void retain(void *ptr) {}
   static void release(void *ptr)
   {
      delete static_cast<T *>(ptr);
   }

   static const T &getFromCache(void *ptr)
   {
      return *static_cast<T *>(ptr);
   }
};

/// The underlying implementation of the caching mechanism.
/// It should be inherently thread-safe.
class CacheImpl
{
public:
   using ImplTy = void *;

   struct CallBacks
   {
      void *userData;
      uintptr_t (*keyHashCB)(void *key, void *userData);
      bool (*keyIsEqualCB)(void *key1, void *key2, void *userData);

      void (*keyDestroyCB)(void *key, void *userData);
      void (*valueRetainCB)(void *value, void *userData);
      void (*valueReleaseCB)(void *value, void *userData);
   };

protected:
   CacheImpl() = default;

   ImplTy impl = nullptr;

   static ImplTy create(llvm::StringRef name, const CallBacks &callbacks);

   /// Sets value for key.
   ///
   /// \param key key to add.  Must not be nullptr.
   /// \param value value to add. If value is nullptr, key is associated with the
   /// value nullptr.
   /// \param cost cost of maintaining value in cache.
   ///
   /// Sets value for key.  value is retained until released using
   /// \c releaseValue().
   ///
   /// Replaces previous key and value if present.  Invokes the key destroy
   /// callback immediately for the previous key.  Invokes the value destroy
   /// callback once the previous value's retain count is zero.
   ///
   /// cost indicates the relative cost of maintaining value in the cache
   /// (e.g., size of value in bytes) and may be used by the cache under
   /// memory pressure to select which cache values to evict.  Zero is a
   /// valid cost.
   void setAndRetain(void *key, void *value, size_t cost);

   /// Fetches value for key.
   ///
   /// \param key key used to lookup value.  Must not be nullptr.
   /// \param valueOut value is stored here if found.  Must not be nullptr.
   /// \returns True if the key was found, false otherwise.
   ///
   /// Fetches value for key, retains value, and stores value in valueOut.
   /// Caller should release value using \c releaseValue().
   bool getAndRetain(const void *key, void **valueOut);

   /// Releases a previously retained cache value.
   ///
   /// \param value value to release.  Must not be nullptr.
   ///
   /// Releases a previously retained cache value. When the reference count
   /// reaches zero the cache may destroy the value.
   void releaseValue(void *value);

   /// Removes a key and its value.
   ///
   /// \param key key to remove.  Must not be nullptr.
   /// \returns True if the key was found, false otherwise.
   ///
   /// Removes a key and its value from the cache such that \c getAndRetain()
   /// will return false.  Invokes the key destroy callback immediately.
   /// Invokes the value destroy callback once value's retain count is zero.
   bool remove(const void *key);

   /// Invokes \c remove on all keys.
   void removeAll();

   /// Destroys cache.
   void destroy();
};

/// Caching mechanism, that is thread-safe and can evict its entries when there
/// is memory pressure.
///
/// This works like a dictionary, you use a key to store and retrieve a value.
/// The value is copied (during storing or retrieval), but an IntrusiveRefCntPtr
/// can be used directly as a value.
///
/// It is important to provide a proper 'cost' function for the value (via
/// \c CacheValueCostInfo trait); e.g. the cost for an AstContext would be the
/// memory usage of the data structures it owns.
template <typename KeyT, typename ValueT,
          typename KeyInfoT = CacheKeyInfo<KeyT>,
          typename ValueInfoT = CacheValueInfo<ValueT> >
class Cache : CacheImpl
{
public:
   explicit Cache(llvm::StringRef name)
   {
      CallBacks callbacks = {
         /*userData=*/nullptr,
         keyHash,
         keyIsEqual,
         keyDestroy,
         valueRetain,
         valueRelease,
      };
      impl = create(name, callbacks);
   }

   ~Cache()
   {
      destroy();
   }

   void set(const KeyT &key, const ValueT &value)
   {
      void *cacheKeyPtr = KeyInfoT::enterCache(key);
      void *cacheValuePtr = ValueInfoT::enterCache(value);
      setAndRetain(cacheKeyPtr, cacheValuePtr, ValueInfoT::getCost(value));
      releaseValue(cacheValuePtr);
   }

   llvm::Optional<ValueT> get(const KeyT &key)
   {
      const void *cacheKeyPtr = KeyInfoT::getLookupKey(&key);
      void *cacheValuePtr;
      bool found = getAndRetain(cacheKeyPtr, &cacheValuePtr);
      if (!found) {
         return llvm::None;
      }
      ValueT value(ValueInfoT::getFromCache(cacheValuePtr));
      releaseValue(cacheValuePtr);
      return std::move(value);
   }

   /// \returns True if the key was found, false otherwise.
   bool remove(const KeyT &key)
   {
      const void *cacheKeyPtr = KeyInfoT::getLookupKey(&key);
      return CacheImpl::remove(cacheKeyPtr);
   }

   void clear()
   {
      removeAll();
   }

private:
   static uintptr_t keyHash(void *key, void *userData)
   {
      return KeyInfoT::getHashValue(*static_cast<KeyT*>(key));
   }

   static bool keyIsEqual(void *key1, void *key2, void *userData)
   {
      return KeyInfoT::isEqual(key1, key2);
   }

   static void keyDestroy(void *key, void *userData)
   {
      KeyInfoT::exitCache(key);
   }

   static void valueRetain(void *value, void *userData)
   {
      ValueInfoT::retain(value);
   }

   static void valueRelease(void *value, void *userData)
   {
      ValueInfoT::release(value);
   }
};

template <typename T>
struct CacheValueInfo<llvm::IntrusiveRefCntPtr<T>>
{
   static void *enterCache(const llvm::IntrusiveRefCntPtr<T> &value)
   {
      return const_cast<T *>(value.get());
   }

   static void retain(void *ptr)
   {
      static_cast<T*>(ptr)->Retain();
   }

   static void release(void *ptr)
   {
      static_cast<T*>(ptr)->Release();
   }

   static llvm::IntrusiveRefCntPtr<T> getFromCache(void *ptr)
   {
      return static_cast<T*>(ptr);
   }

   static size_t getCost(const llvm::IntrusiveRefCntPtr<T> &value)
   {
      return CacheValueCostInfo<T>::getCost(*value);
   }
};

} // polar::sys

#endif // POLARPHP_BASIC_CACHE_H
