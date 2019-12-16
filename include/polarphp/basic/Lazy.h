//===--- Lazy.h - A lazily-initialized object -------------------*- C++ -*-===//
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
// Created by polarboy on 2019/12/02.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_LAZY_H
#define POLARPHP_BASIC_LAZY_H

#include <memory>
#ifdef __APPLE__
#include <dispatch/dispatch.h>
#else
#include <mutex>
#endif
#include "polarphp/basic/Malloc.h"

namespace polar {

#ifdef __APPLE__
using OnceToken_t = dispatch_once_t;
# define POLARPHP_ONCE_F(TOKEN, FUNC, CONTEXT) \
   ::dispatch_once_f(&TOKEN, CONTEXT, FUNC)
#elif defined(__CYGWIN__)
// _polarphp_once_f() is declared in Private.h.
// This prototype is copied instead including the header file.
void _polarphp_once_f(uintptr_t *predicate, void *context,
                      void (*function)(void *));
using OnceToken_t = unsigned long;
# define POLARPHP_ONCE_F(TOKEN, FUNC, CONTEXT) \
   _polarphp_once_f(&TOKEN, CONTEXT, FUNC)
#else
using OnceToken_t = std::once_flag;
# define POLARPHP_ONCE_F(TOKEN, FUNC, CONTEXT) \
   ::std::call_once(TOKEN, FUNC, CONTEXT)
#endif

/// A template for lazily-constructed, zero-initialized, leaked-on-exit
/// global objects.
template <typename T>
class Lazy
{
   alignas(T) char m_value[sizeof(T)] = { 0 };

   OnceToken_t m_onceToken = {};

   static void defaultInitCallback(void *valueAddr)
   {
      ::new (valueAddr) T();
   }

public:
   using Type = T;

   T &get(void (*initCallback)(void *) = defaultInitCallback);

   template<typename Arg1>
   T &getWithInit(Arg1 &&arg1);

   /// Get the value, assuming it must have already been initialized by this
   /// point.
   T &unsafeGetAlreadyInitialized()
   {
      return *reinterpret_cast<T *>(&m_value);
   }

   constexpr Lazy() = default;

   T *operator->() { return &get(); }
   T &operator*() { return get(); }

private:
   Lazy(const Lazy &) = delete;
   Lazy &operator=(const Lazy &) = delete;
};

template <typename T>
inline T &Lazy<T>::get(void (*initCallback)(void*))
{
   static_assert(std::is_literal_type<Lazy<T>>::value,
                 "Lazy<T> must be a literal type");

   POLARPHP_ONCE_F(m_onceToken, initCallback, &m_value);
   return unsafeGetAlreadyInitialized();
}

template <typename T>
template <typename Arg1>
inline T &Lazy<T>::getWithInit(Arg1 &&arg1)
{
   struct Data
   {
      void *address;
      Arg1 &&arg1;
      static void init(void *context)
      {
         Data *data = static_cast<Data *>(context);
         ::new (data->address) T(static_cast<Arg1&&>(data->arg1));
      }
   } data{&m_value, static_cast<Arg1&&>(arg1)};

   POLARPHP_ONCE_F(m_onceToken, &Data::init, &data);
   return unsafeGetAlreadyInitialized();
}

} // polar

#define POLAR_LAZY_CONSTANT(INITIAL_VALUE) \
   ([]{ \
   using T = ::std::remove_reference<decltype(INITIAL_VALUE)>::type; \
   static ::polar::Lazy<T> TheLazy; \
   return TheLazy.get([](void *valueAddr){ ::new(valueAddr) T{INITIAL_VALUE}; });\
   }())

#endif // POLARPHP_BASIC_LAZY_H
