// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

#ifndef POLARPHP_UTILS_MANAGED_STATICS_H
#define POLARPHP_UTILS_MANAGED_STATICS_H

#include <atomic>
#include <cstddef>

namespace polar {
namespace utils {

/// object_creator - Helper method for ManagedStatic.
template <typename Class>
struct ObjectCreator
{
   static void *call()
   {
      return new Class();
   }
};

/// object_deleter - Helper method for ManagedStatic.
///
template <typename T>
struct ObjectDeleter
{
   static void call(void *ptr)
   {
      delete (T *)ptr;
   }
};

template <typename T, size_t N>
struct ObjectDeleter<T[N]>
{
   static void call(void *ptr)
   {
      delete[](T *)ptr;
   }
};

/// ManagedStaticBase - Common base class for ManagedStatic instances.
class ManagedStaticBase
{
protected:
   // This should only be used as a static variable, which guarantees that this
   // will be zero initialized.
   mutable std::atomic<void *> m_ptr;
   mutable void (*m_deleterFunc)(void*);
   mutable const ManagedStaticBase *m_next;
   void registerManagedStatic(void *(*creator)(), void (*deleter)(void*)) const;
public:
   /// isConstructed - Return true if this object has not been created yet.
   bool isConstructed() const
   {
      return m_ptr != nullptr;
   }
   void destroy() const;
};

/// ManagedStatic - This transparently changes the behavior of global statics to
/// be lazily constructed on demand (good for reducing startup times of dynamic
/// libraries that link in LLVM components) and for making destruction be
/// explicit through the llvm_shutdown() function call.
///
template <typename Class, typename Creator = ObjectCreator<Class>,
          typename Deleter = ObjectDeleter<Class>>
class ManagedStatic : public ManagedStaticBase
{
public:
   // Accessors.
   Class &operator*()
   {
      void *tmp = m_ptr.load(std::memory_order_acquire);
      if (!tmp) {
         registerManagedStatic(Creator::call, Deleter::call);
      }
      return *static_cast<Class *>(m_ptr.load(std::memory_order_relaxed));
   }

   Class *operator->()
   {
      return &**this;
   }

   const Class &operator*() const
   {
      void *tmp = m_ptr.load(std::memory_order_acquire);
      if (!tmp) {
         registerManagedStatic(Creator::call, Deleter::call);
      }
      return *static_cast<Class *>(m_ptr.load(std::memory_order_relaxed));
   }

   const Class *operator->() const
   {
      return &**this;
   }
};

/// managed_statics_shutdown - Deallocate and destroy all ManagedStatic variables.
void managed_statics_shutdown();

/// ManagedStaticsReleaser - This is a simple helper class that calls
/// managed_statics_shutdown() when it is destroyed.
struct ManagedStaticsReleaser
{
   ManagedStaticsReleaser() = default;
   ~ManagedStaticsReleaser()
   {
      managed_statics_shutdown();
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_MANAGED_STATICS_H
