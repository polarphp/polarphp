// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/25.

//===----------------------------------------------------------------------===//
//
// This file defines the RefCountedBase, ThreadSafeRefCountedBase, and
// IntrusiveRefCountPtr classes.
//
// IntrusiveRefCountPtr is a smart pointer to an object which maintains a
// reference count.  (ThreadSafe)RefCountedBase is a mixin class that adds a
// refcount member variable and methods for updating the refcount.  An object
// that inherits from (ThreadSafe)RefCountedBase deletes itself when its
// refcount hits zero.
//
// For example:
//
//   class MyClass : public RefCountedBase<MyClass> {};
//
//   void foo() {
//     // Constructing an IntrusiveRefCountPtr increases the pointee's refcount by
//     // 1 (from 0 in this case).
//     IntrusiveRefCountPtr<MyClass> Ptr1(new MyClass());
//
//     // Copying an IntrusiveRefCountPtr increases the pointee's refcount by 1.
//     IntrusiveRefCountPtr<MyClass> Ptr2(Ptr1);
//
//     // Constructing an IntrusiveRefCountPtr has no effect on the object's
//     // refcount.  After a move, the moved-from pointer is null.
//     IntrusiveRefCountPtr<MyClass> Ptr3(std::move(Ptr1));
//     assert(Ptr1 == nullptr);
//
//     // Clearing an IntrusiveRefCountPtr decreases the pointee's refcount by 1.
//     Ptr2.reset();
//
//     // The object deletes itself when we return from the function, because
//     // Ptr3's destructor decrements its refcount to 0.
//   }
//
// You can use IntrusiveRefCountPtr with isa<T>(), dyn_cast<T>(), etc.:
//
//   IntrusiveRefCountPtr<MyClass> Ptr(new MyClass());
//   OtherClass *Other = dyn_cast<OtherClass>(Ptr);  // Ptr.get() not required
//
// IntrusiveRefCountPtr works with any class that
//
//  - inherits from (ThreadSafe)RefCountedBase,
//  - has retain() and release() methods, or
//  - specializes IntrusiveRefCountPtrInfo.
//
//===----------------------------------------------------------------------===//


#ifndef POLARPHP_BASIC_ADT_INTRUSIVE_REF_COUNT_PTR_H
#define POLARPHP_BASIC_ADT_INTRUSIVE_REF_COUNT_PTR_H

#include <atomic>
#include <cassert>
#include <cstddef>

namespace polar {
namespace basic {

/// A CRTP mixin class that adds reference counting to a type.
///
/// The lifetime of an object which inherits from RefCountedBase is managed by
/// calls to release() and retain(), which increment and decrement the object's
/// refcount, respectively.  When a release() call decrements the refcount to 0,
/// the object deletes itself.
template <typename Derived>
class RefCountedBase
{
   mutable unsigned m_refCount = 0;

public:
   RefCountedBase() = default;
   RefCountedBase(const RefCountedBase &)
   {}

   void retain() const
   {
      ++m_refCount;
   }

   void release() const
   {
      assert(m_refCount > 0 && "Reference count is already zero.");
      if (--m_refCount == 0) {
         delete static_cast<const Derived *>(this);
      }
   }
};

/// A thread-safe version of \c RefCountedBase.
template <typename Derived>
class ThreadSafeRefCountedBase
{
   mutable std::atomic<int> m_refCount;

protected:
   ThreadSafeRefCountedBase() : m_refCount(0)
   {}

public:
   void retain() const
   {
      m_refCount.fetch_add(1, std::memory_order_relaxed);
   }

   void release() const
   {
      int newRefCount = m_refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
      assert(newRefCount >= 0 && "Reference count was already zero.");
      if (newRefCount == 0) {
         delete static_cast<const Derived *>(this);
      }
   }
};

/// Class you can specialize to provide custom retain/release functionality for
/// a type.
///
/// Usually specializing this class is not necessary, as IntrusiveRefCountPtr
/// works with any type which defines retain() and release() functions -- you
/// can define those functions yourself if RefCountedBase doesn't work for you.
///
/// One case when you might want to specialize this type is if you have
///  - Foo.h defines type Foo and includes Bar.h, and
///  - Bar.h uses IntrusiveRefCountPtr<Foo> in inline functions.
///
/// Because Foo.h includes Bar.h, Bar.h can't include Foo.h in order to pull in
/// the declaration of Foo.  Without the declaration of Foo, normally Bar.h
/// wouldn't be able to use IntrusiveRefCountPtr<Foo>, which wants to call
/// T::retain and T::release.
///
/// To resolve this, Bar.h could include a third header, FooFwd.h, which
/// forward-declares Foo and specializes IntrusiveRefCountPtrInfo<Foo>.  Then
/// Bar.h could use IntrusiveRefCountPtr<Foo>, although it still couldn't call any
/// functions on Foo itself, because Foo would be an incomplete type.
template <typename T>
struct IntrusiveRefCountPtrInfo
{
   static void retain(T *obj)
   {
      obj->retain();
   }
   static void release(T *obj)
   {
      obj->release();
   }
};

/// A smart pointer to a reference-counted object that inherits from
/// RefCountedBase or ThreadSafeRefCountedBase.
///
/// This class increments its pointee's reference count when it is created, and
/// decrements its refcount when it's destroyed (or is changed to point to a
/// different object).
template <typename T>
class IntrusiveRefCountPtr
{
   T *m_obj = nullptr;

public:
   using element_type = T;

   explicit IntrusiveRefCountPtr() = default;
   IntrusiveRefCountPtr(T *obj)
      : m_obj(obj)
   {
      retain();
   }

   IntrusiveRefCountPtr(const IntrusiveRefCountPtr &other) : m_obj(other.m_obj)
   {
      retain();
   }

   IntrusiveRefCountPtr(IntrusiveRefCountPtr &&other) : m_obj(other.m_obj)
   {
      other.m_obj = nullptr;
   }

   template <typename X>
   IntrusiveRefCountPtr(IntrusiveRefCountPtr<X> &&other) : m_obj(other.get())
   {
      other.m_obj = nullptr;
   }

   template <typename X>
   IntrusiveRefCountPtr(const IntrusiveRefCountPtr<X> &other) : m_obj(other.get())
   {
      retain();
   }

   ~IntrusiveRefCountPtr()
   {
      release();
   }

   IntrusiveRefCountPtr &operator=(IntrusiveRefCountPtr other)
   {
      swap(other);
      return *this;
   }

   T &operator*() const
   {
      return *m_obj;
   }

   T *operator->() const
   {
      return m_obj;
   }

   T *get() const
   {
      return m_obj;
   }

   explicit operator bool() const
   {
      return m_obj;
   }

   void swap(IntrusiveRefCountPtr &other)
   {
      T *tmp = other.m_obj;
      other.m_obj = m_obj;
      m_obj = tmp;
   }

   void reset()
   {
      release();
      m_obj = nullptr;
   }

   void resetWithoutrelease()
   {
      m_obj = nullptr;
   }

private:
   void retain()
   {
      if (m_obj) {
         IntrusiveRefCountPtrInfo<T>::retain(m_obj);
      }
   }

   void release()
   {
      if (m_obj) {
         IntrusiveRefCountPtrInfo<T>::release(m_obj);
      }
   }

   template <typename X> friend class IntrusiveRefCountPtr;
};

template <typename T, typename U>
inline bool operator==(const IntrusiveRefCountPtr<T> &lhs,
                       const IntrusiveRefCountPtr<U> &rhs)
{
   return lhs.get() == rhs.get();
}

template <typename T, typename U>
inline bool operator!=(const IntrusiveRefCountPtr<T> &lhs,
                       const IntrusiveRefCountPtr<U> &rhs)
{
   return lhs.get() != rhs.get();
}

template <typename T, typename U>
inline bool operator==(const IntrusiveRefCountPtr<T> &lhs, U *rhs)
{
   return lhs.get() == rhs;
}

template <typename T, typename U>
inline bool operator!=(const IntrusiveRefCountPtr<T> &lhs, U *rhs)
{
   return lhs.get() != rhs;
}

template <typename T, typename U>
inline bool operator==(T *lhs, const IntrusiveRefCountPtr<U> &rhs)
{
   return lhs == rhs.get();
}

template <typename T, typename U>
inline bool operator!=(T *lhs, const IntrusiveRefCountPtr<U> &rhs)
{
   return lhs != rhs.get();
}

template <typename T>
bool operator==(std::nullptr_t lhs, const IntrusiveRefCountPtr<T> &rhs)
{
   return !rhs;
}

template <typename T>
bool operator==(const IntrusiveRefCountPtr<T> &lhs, std::nullptr_t rhs)
{
   return rhs == lhs;
}

template <typename T>
bool operator!=(std::nullptr_t lhs, const IntrusiveRefCountPtr<T> &rhs)
{
   return !(lhs == rhs);
}

template <typename T>
bool operator!=(const IntrusiveRefCountPtr<T> &lhs, std::nullptr_t rhs)
{
   return !(lhs == rhs);
}

// Make IntrusiveRefCountPtr work with dyn_cast, isa, and the other idioms from
// Casting.h.
template <typename From>
struct SimplifyType;

template <typename T>
struct SimplifyType<IntrusiveRefCountPtr<T>>
{
   using SimpleType = T *;

   static SimpleType getSimplifiedValue(IntrusiveRefCountPtr<T> &value)
   {
      return value.get();
   }
};

template <typename T>
struct SimplifyType<const IntrusiveRefCountPtr<T>>
{
   using SimpleType = /*const*/ T *;

   static SimpleType getSimplifiedValue(const IntrusiveRefCountPtr<T> &value)
   {
      return value.get();
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_INTRUSIVE_REF_COUNT_PTR_H
