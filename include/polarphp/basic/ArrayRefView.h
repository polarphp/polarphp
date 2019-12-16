//===- ArrayRefView.h - Adapter for iterating over an ArrayRef --*- C++ -*-===//
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
// Created by polarboy on 2019/11/27.
//===----------------------------------------------------------------------===//
//
//  This file defines ArrayRefView, a template class which provides a
//  proxied view of the elements of an array.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_ARRAYREF_VIEW_H
#define POLARPHP_BASIC_ARRAYREF_VIEW_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Casting.h"

namespace polar {

/// An adapter for iterating over a range of values as a range of
/// values of a different type.
template <class Orig, class Projected, Projected (&Project)(const Orig &),
          bool AllowOrigAccess = false>
class ArrayRefView
{
public:
   ArrayRefView() {}
   ArrayRefView(llvm::ArrayRef<Orig> array) : m_array(array) {}

   class iterator
   {
   public:
      using value_type = Projected;
      using reference = Projected;
      using pointer = void;
      using difference_type = ptrdiff_t;
      using iterator_category = std::random_access_iterator_tag;

      Projected operator*() const
      {
         return Project(*m_ptr);
      }

      Projected operator->() const
      {
         return operator*();
      }

      iterator &operator++()
      {
         m_ptr++;
         return *this;
      }

      iterator operator++(int)
      {
         return iterator(m_ptr++);
      }

      iterator &operator--()
      {
         m_ptr--;
         return *this;
      }

      iterator operator--(int)
      {
         return iterator(m_ptr--);
      }

      bool operator==(iterator rhs) const
      {
         return m_ptr == rhs.m_ptr;
      }

      bool operator!=(iterator rhs) const
      {
         return m_ptr != rhs.m_ptr;
      }

      iterator &operator+=(difference_type i)
      {
         m_ptr += i;
         return *this;
      }

      iterator operator+(difference_type i) const
      {
         return iterator(m_ptr + i);
      }

      friend iterator operator+(difference_type i, iterator base)
      {
         return iterator(base.m_ptr + i);
      }

      iterator &operator-=(difference_type i)
      {
         m_ptr -= i;
         return *this;
      }

      iterator operator-(difference_type i) const
      {
         return iterator(m_ptr - i);
      }

      difference_type operator-(iterator rhs) const
      {
         return m_ptr - rhs.m_ptr;
      }

      Projected operator[](difference_type i) const
      {
         return Project(m_ptr[i]);
      }

      bool operator<(iterator rhs) const
      {
         return m_ptr < rhs.m_ptr;
      }

      bool operator<=(iterator rhs) const
      {
         return m_ptr <= rhs.m_ptr;
      }

      bool operator>(iterator rhs) const
      {
         return m_ptr > rhs.m_ptr;
      }

      bool operator>=(iterator rhs) const
      {
         return m_ptr >= rhs.m_ptr;
      }
   private:
      friend class ArrayRefView<Orig,Projected,Project,AllowOrigAccess>;
      const Orig *m_ptr;
      iterator(const Orig *ptr) : m_ptr(ptr) {}
   };

   iterator begin() const
   {
      return iterator(m_array.begin());
   }

   iterator end() const
   {
      return iterator(m_array.end());
   }

   bool empty() const
   {
      return m_array.empty();
   }

   size_t size() const
   {
      return m_array.size();
   }

   Projected operator[](unsigned i) const
   {
      return Project(m_array[i]);
   }

   Projected front() const
   {
      return Project(m_array.front());
   }

   Projected back() const
   {
      return Project(m_array.back());
   }

   ArrayRefView drop_back(unsigned count = 1) const
   {
      return ArrayRefView(m_array.drop_back(count));
   }

   ArrayRefView slice(unsigned start) const
   {
      return ArrayRefView(m_array.slice(start));
   }

   ArrayRefView slice(unsigned start, unsigned length) const
   {
      return ArrayRefView(m_array.slice(start, length));
   }

   /// Peek through to the underlying array.  This operation is not
   /// supported by default; it must be enabled at specialization time.
   llvm::ArrayRef<Orig> getOriginalArray() const
   {
      static_assert(AllowOrigAccess,
                    "original array access not enabled for this view");
      return m_array;
   }

   friend bool operator==(ArrayRefView lhs, ArrayRefView rhs)
   {
      if (lhs.size() != rhs.size()) {
         return false;
      }
      for (auto i : indices(lhs)) {
         if (lhs[i] != rhs[i]) {
            return false;
         }
      }
      return true;
   }

   friend bool operator==(llvm::ArrayRef<Projected> lhs, ArrayRefView rhs)
   {
      if (lhs.size() != rhs.size())
         return false;
      for (auto i : indices(lhs))
         if (lhs[i] != rhs[i])
            return false;
      return true;
   }
   friend bool operator==(ArrayRefView lhs, llvm::ArrayRef<Projected> rhs)
   {
      if (lhs.size() != rhs.size())
         return false;
      for (auto i : indices(lhs))
         if (lhs[i] != rhs[i])
            return false;
      return true;
   }

   friend bool operator!=(ArrayRefView lhs, ArrayRefView rhs)
   {
      return !(lhs == rhs);
   }

   friend bool operator!=(llvm::ArrayRef<Projected> lhs, ArrayRefView rhs)
   {
      return !(lhs == rhs);
   }

   friend bool operator!=(ArrayRefView lhs, llvm::ArrayRef<Projected> rhs)
   {
      return !(lhs == rhs);
   }

private:
   llvm::ArrayRef<Orig> m_array;
};

/// Helper for \c CastArrayRefView that casts the original type to the
/// projected type.
template<typename Projected, typename Orig>
inline Projected *array_ref_view_cast_helper(const Orig &value)
{
   using llvm::cast_or_null;
   return cast_or_null<Projected>(value);
}

/// An ArrayRefView that performs a cast_or_null on each element in the
/// underlying ArrayRef.
template<typename Orig, typename Projected>
using CastArrayRefView =
ArrayRefView<Orig, Projected *, array_ref_view_cast_helper<Projected, Orig>>;

} // polar

#endif // POLARPHP_BASIC_ARRAYREF_VIEW_H
