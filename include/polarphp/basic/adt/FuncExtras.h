// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/19.

//===- FunctionExtras.h - Function type erasure utilities -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file provides a collection of function (or more generally, callable)
/// type erasure utilities supplementing those provided by the standard library
/// in `<function>`.
///
/// It provides `unique_function`, which works like `std::function` but supports
/// move-only callable objects.
///
/// Future plans:
/// - Add a `function` that provides const, volatile, and ref-qualified support,
///   which doesn't work with `std::function`.
/// - Provide support for specifying multiple signatures to type erase callable
///   objects with an overload set, such as those produced by generic lambdas.
/// - Expand to include a copyable utility that directly replaces std::function
///   but brings the above improvements.
///
/// Note that LLVM's utilities are greatly simplified by not supporting
/// allocators.
///
/// If the standard library ever begins to provide comparable facilities we can
/// consider switching to those.
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_ADT_FUNCTION_EXTRAS_H
#define LLVM_ADT_FUNCTION_EXTRAS_H

#include "polarphp/basic/adt/PointerIntPair.h"
#include "polarphp/basic/adt/PointerUnion.h"
#include "polarphp/utils/TypeTraits.h"
#include <memory>

namespace polar {

template <typename FunctionT> class unique_function;

template <typename ReturnT, typename... ParamTs>
class unique_function<ReturnT(ParamTs...)>
{
   static constexpr size_t InlineStorageSize = sizeof(void *) * 3;

   // MSVC has a bug and ICEs if we give it a particular dependent value
   // expression as part of the `std::conditional` below. To work around this,
   // we build that into a template struct's constexpr bool.
   template <typename T>
   struct IsSizeLessThanThresholdT
   {
      static constexpr bool value = sizeof(T) <= (2 * sizeof(void *));
   };

   // Provide a type function to map parameters that won't observe extra copies
   // or moves and which are small enough to likely pass in register to values
   // and all other types to l-value reference types. We use this to compute the
   // types used in our erased call utility to minimize copies and moves unless
   // doing so would force things unnecessarily into memory.
   //
   // The heuristic used is related to common ABI register passing conventions.
   // It doesn't have to be exact though, and in one way it is more strict
   // because we want to still be able to observe either moves *or* copies.
   template <typename T>
   using AdjustedParamT = typename std::conditional<
   !std::is_reference<T>::value &&
   polar::utils::is_trivially_copy_constructible<T>::value &&
   polar::utils::is_trivially_move_constructible<T>::value &&
   IsSizeLessThanThresholdT<T>::value,
   T, T &>::type;

   // The type of the erased function pointer we use as a callback to dispatch to
   // the stored callable when it is trivial to move and destroy.
   using CallPtrT = ReturnT (*)(void *callableAddr,
   AdjustedParamT<ParamTs>... params);
   using MovePtrT = void (*)(void *lhsCallableAddr, void *rhsCallableAddr);
   using DestroyPtrT = void (*)(void *callableAddr);

   /// A struct to hold a single trivial callback with sufficient alignment for
   /// our bitpacking.
   struct alignas(8) TrivialCallback
   {
      CallPtrT m_callPtr;
   };

   /// A struct we use to aggregate three callbacks when we need full set of
   /// operations.
   struct alignas(8) NonTrivialCallbacks
   {
      CallPtrT m_callPtr;
      MovePtrT m_movePtr;
      DestroyPtrT m_destroyPtr;
   };

   // Create a pointer union between either a pointer to a static trivial call
   // pointer in a struct or a pointer to a static struct of the call, move, and
   // destroy pointers.
   using CallbackPointerUnionT =
   polar::basic::PointerUnion<TrivialCallback *, NonTrivialCallbacks *>;

   // The main storage buffer. This will either have a pointer to out-of-line
   // storage or an inline buffer storing the callable.
   union StorageUnionT
   {
      // For out-of-line storage we keep a pointer to the underlying storage and
      // the size. This is enough to deallocate the memory.
      struct OutOfLineStorageT {
         void *m_storagePtr;
         size_t m_size;
         size_t Alignment;
      } m_outOfLineStorage;
      static_assert(
               sizeof(OutOfLineStorageT) <= InlineStorageSize,
               "Should always use all of the out-of-line storage for inline storage!");

      // For in-line storage, we just provide an aligned character buffer. We
      // provide three pointers worth of storage here.
      typename std::aligned_storage<InlineStorageSize, alignof(void *)>::type
            inlineStorage;
   } m_storageUnion;

   // A compressed pointer to either our dispatching callback or our table of
   // dispatching callbacks and the flag for whether the callable itself is
   // stored inline or not.
   polar::utils::PointerIntPair<CallbackPointerUnionT, 1, bool> m_callbackAndInlineFlag;

   bool isInlineStorage() const
   {
      return m_callbackAndInlineFlag.getInt();
   }

   bool isTrivialCallback() const
   {
      return m_callbackAndInlineFlag.getPointer().template is<TrivialCallback *>();
   }

   CallPtrT getTrivialCallback() const
   {
      return m_callbackAndInlineFlag.getPointer().template get<TrivialCallback *>()->m_callPtr;
   }

   NonTrivialCallbacks *getNonTrivialCallbacks() const
   {
      return m_callbackAndInlineFlag.getPointer()
            .template get<NonTrivialCallbacks *>();
   }

   void *getInlineStorage()
   {
      return &m_storageUnion.inlineStorage;
   }

   void *getOutOfLineStorage()
   {
      return m_storageUnion.m_outOfLineStorage.m_storagePtr;
   }

   size_t getOutOfLineStorageSize() const
   {
      return m_storageUnion.m_outOfLineStorage.m_size;
   }

   size_t getOutOfLineStorageAlignment() const
   {
      return m_storageUnion.m_outOfLineStorage.Alignment;
   }

   void setOutOfLineStorage(void *ptr, size_t size, size_t Alignment)
   {
      m_storageUnion.m_outOfLineStorage = {ptr, size, Alignment};
   }

   template <typename CallableT>
   static ReturnT CallImpl(void *callableAddr, AdjustedParamT<ParamTs>... params)
   {
      return (*reinterpret_cast<CallableT *>(callableAddr))(
               std::forward<ParamTs>(params)...);
   }

   template <typename CallableT>
   static void MoveImpl(void *lhsCallableAddr, void *rhsCallableAddr) noexcept
   {
      new (lhsCallableAddr)
            CallableT(std::move(*reinterpret_cast<CallableT *>(rhsCallableAddr)));
   }

   template <typename CallableT>
   static void DestroyImpl(void *callableAddr) noexcept
   {
      reinterpret_cast<CallableT *>(callableAddr)->~CallableT();
   }

   public:
   unique_function() = default;
   unique_function(std::nullptr_t /*null_callable*/) {}

   ~unique_function() {
      if (!m_callbackAndInlineFlag.getPointer())
         return;

      // Cache this value so we don't re-check it after type-erased operations.
      bool isInlineStorageFlag = isInlineStorage();

      if (!isTrivialCallback())
         getNonTrivialCallbacks()->m_destroyPtr(
                  isInlineStorageFlag ? getInlineStorage() : getOutOfLineStorage());

      if (!isInlineStorageFlag)
         deallocate_buffer(getOutOfLineStorage(), getOutOfLineStorageSize(),
                           getOutOfLineStorageAlignment());
   }

   unique_function(unique_function &&rhs) noexcept
   {
      // Copy the callback and inline flag.
      m_callbackAndInlineFlag = rhs.m_callbackAndInlineFlag;

      // If the rhs is empty, just copying the above is sufficient.
      if (!rhs)
         return;

      if (!isInlineStorage()) {
         // The out-of-line case is easiest to move.
         m_storageUnion.m_outOfLineStorage = rhs.m_storageUnion.m_outOfLineStorage;
      } else if (isTrivialCallback()) {
         // Move is trivial, just memcpy the bytes across.
         memcpy(getInlineStorage(), rhs.getInlineStorage(), InlineStorageSize);
      } else {
         // Non-trivial move, so dispatch to a type-erased implementation.
         getNonTrivialCallbacks()->m_movePtr(getInlineStorage(),
                                           rhs.getInlineStorage());
      }

      // Clear the old callback and inline flag to get back to as-if-null.
      rhs.m_callbackAndInlineFlag = {};

#ifndef NDEBUG
      // In debug builds, we also scribble across the rest of the storage.
      memset(rhs.getInlineStorage(), 0xAD, InlineStorageSize);
#endif
   }

   unique_function &operator=(unique_function &&rhs) noexcept
   {
      if (this == &rhs)
         return *this;

      // Because we don't try to provide any exception safety guarantees we can
      // implement move assignment very simply by first destroying the current
      // object and then move-constructing over top of it.
      this->~unique_function();
      new (this) unique_function(std::move(rhs));
      return *this;
   }

   template <typename CallableT> unique_function(CallableT callable)
   {
      bool isInlineStorage = true;
      void *callableAddr = getInlineStorage();
      if (sizeof(CallableT) > InlineStorageSize ||
          alignof(CallableT) > alignof(decltype(m_storageUnion.inlineStorage))) {
         isInlineStorage = false;
         // Allocate out-of-line storage. FIXME: Use an explicit alignment
         // parameter in C++17 mode.
         auto size = sizeof(CallableT);
         auto Alignment = alignof(CallableT);
         callableAddr = allocate_buffer(size, Alignment);
         setOutOfLineStorage(callableAddr, size, Alignment);
      }

      // Now move into the storage.
      new (callableAddr) CallableT(std::move(callable));

      // See if we can create a trivial callback. We need the callable to be
      // trivially moved and trivially destroyed so that we don't have to store
      // type erased callbacks for those operations.
      //
      // FIXME: We should use constexpr if here and below to avoid instantiating
      // the non-trivial static objects when unnecessary. While the linker should
      // remove them, it is still wasteful.
      if (polar::utils::is_trivially_move_constructible<CallableT>::value &&
          std::is_trivially_destructible<CallableT>::value) {
         // We need to create a nicely aligned object. We use a static variable
         // for this because it is a trivial struct.
         static TrivialCallback Callback = { &CallImpl<CallableT> };

         m_callbackAndInlineFlag = {&Callback, isInlineStorage};
         return;
      }

      // Otherwise, we need to point at an object that contains all the different
      // type erased behaviors needed. Create a static instance of the struct type
      // here and then use a pointer to that.
      static NonTrivialCallbacks Callbacks = {
         &CallImpl<CallableT>, &MoveImpl<CallableT>, &DestroyImpl<CallableT>};

      m_callbackAndInlineFlag = {&Callbacks, isInlineStorage};
   }

   ReturnT operator()(ParamTs... params)
   {
      void *callableAddr =
            isInlineStorage() ? getInlineStorage() : getOutOfLineStorage();

      return (isTrivialCallback()
              ? getTrivialCallback()
              : getNonTrivialCallbacks()->m_callPtr)(callableAddr, params...);
   }

   explicit operator bool() const
   {
      return (bool)m_callbackAndInlineFlag.getPointer();
   }
};

} // end namespace polar

#endif // POLARPHP_ADT_FUNCTION_H
