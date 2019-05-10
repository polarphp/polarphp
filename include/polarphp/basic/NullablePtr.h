//===--- NullablePtr.h - A pointer that allows null -------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/28.
//===----------------------------------------------------------------------===//
//
// This file defines and implements the NullablePtr class.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_NULLABLEPTR_H
#define POLARPHP_BASIC_NULLABLEPTR_H

#include <cassert>
#include <cstddef>
#include <type_traits>

namespace polar::basic {

/// NullablePtr pointer wrapper - NullablePtr is used for APIs where a
/// potentially-null pointer gets passed around that must be explicitly handled
/// in lots of places.  By putting a wrapper around the null pointer, it makes
/// it more likely that the null pointer case will be handled correctly.
template<class T>
class NullablePtr
{
   T *m_ptr;
   struct PlaceHolder
   {};

public:
   NullablePtr(T *ptr = 0)
      : m_ptr(ptr)
   {}

   template<typename OtherT>
   NullablePtr(NullablePtr<OtherT> other,
               typename std::enable_if<
               std::is_convertible<OtherT, T>::value,
               PlaceHolder
               >::type = PlaceHolder())
      : m_ptr(other.getPtrOrNull())
   {}

   bool isNull() const
   {
      return m_ptr == 0;
   }

   bool isNonNull() const
   {
      return m_ptr != 0;
   }

   /// get - Return the pointer if it is non-null.
   const T *get() const
   {
      assert(m_ptr && "Pointer wasn't checked for null!");
      return m_ptr;
   }

   /// get - Return the pointer if it is non-null.
   T *get()
   {
      assert(m_ptr && "Pointer wasn't checked for null!");
      return m_ptr;
   }

   T *getPtrOrNull()
   {
      return m_ptr;
   }

   const T *getPtrOrNull() const
   {
      return m_ptr;
   }

   explicit operator bool() const
   {
      return m_ptr;
   }
};

} // polar::basic

#endif // POLARPHP_BASIC_NULLABLEPTR_H
