//===--- TransformArrayRef.h ------------------------------------*- C++ -*-===//
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
///
/// \file
///
/// This file defines TransformArrayRef, a template class that provides a
/// transformed view of an ArrayRef. The difference from ArrayRefView is that
/// ArrayRefView takes its transform as a template argument, while
/// TransformArrayRef only takes a type as its template argument. This means it
/// can be used to define types used with forward declaration pointers without
/// needing to define the relevant function in headers.
///
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_TRANSFORM_ARRAYREF_H
#define POLARPHP_BASIC_TRANSFORM_ARRAYREF_H

#include "polarphp/basic/StlExtras.h"
#include "llvm/ADT/ArrayRef.h"

namespace polar {

/// A transformation of an ArrayRef using a function of type FuncTy. This is
/// different than ArrayRefView since the underlying function is stored instead
/// of used as a template parameter. This allows it to be used in declarations
/// where the underlying function is not known. This is useful when defining the
/// underlying function would require forward declarations to need to be
/// defined.
template <class FuncTy>
class TransformArrayRef
{
public:
   using FunctionTraits = function_traits<FuncTy>;
   using Orig =
   typename std::tuple_element<0, typename FunctionTraits::argument_types>::type;
   using Projected = typename FunctionTraits::result_type;

   TransformArrayRef(llvm::ArrayRef<Orig> array, FuncTy func) : m_array(array), m_func(func) {}

   class iterator
   {
      friend class TransformArrayRef<FuncTy>;
      const Orig *m_ptr;
      FuncTy m_func;
      iterator(const Orig *ptr, FuncTy func)
         : m_ptr(ptr),
           m_func(func) {}
   public:
      using value_type = Projected;
      using reference = Projected;
      using pointer = void;
      using difference_type = ptrdiff_t;
      using iterator_category = std::random_access_iterator_tag;

      Projected operator*() const
      {
         return m_func(*m_ptr);
      }

      iterator &operator++()
      {
         m_ptr++;
         return *this;
      }

      iterator operator++(int)
      {
         return iterator(m_ptr++, m_func);
      }

      iterator &operator--()
      {
         --m_ptr;
         return *this;
      }

      iterator operator--(int)
      {
         return iterator(m_ptr--, m_func);
      }

      bool operator==(iterator other) const
      {
         return m_ptr == other.m_ptr;
      }

      bool operator!=(iterator other) const
      {
         return m_ptr != other.m_ptr;
      }

      iterator &operator+=(difference_type i)
      {
         m_ptr += i;
         return *this;
      }

      iterator operator+(difference_type i) const
      {
         return iterator(m_ptr + i, m_func);
      }

      friend iterator operator+(difference_type i, iterator base)
      {
         return iterator(base.m_ptr + i, base.m_func);
      }

      iterator &operator-=(difference_type i)
      {
         m_ptr -= i;
         return *this;
      }

      iterator operator-(difference_type i) const
      {
         return iterator(m_ptr - i, m_func);
      }

      difference_type operator-(iterator other) const
      {
         return m_ptr - other.m_ptr;
      }

      Projected operator[](difference_type i) const
      {
         return m_func(m_ptr[i]);
      }

      bool operator<(iterator other) const
      {
         return m_ptr < other.m_ptr;
      }

      bool operator<=(iterator other) const
      {
         return m_ptr <= other.m_ptr;
      }

      bool operator>(iterator other) const
      {
         return m_ptr > other.m_ptr;
      }

      bool operator>=(iterator other) const
      {
         return m_ptr >= other.m_ptr;
      }
   };

   iterator begin() const
   {
      return iterator(m_array.begin(), m_func);
   }

   iterator end() const
   {
      return iterator(m_array.end(), m_func);
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
      return m_func(m_array[i]);
   }

   Projected front() const
   {
      return m_func(m_array.front());
   }

   Projected back() const
   {
      return m_func(m_array.back());
   }

   TransformArrayRef slice(unsigned start) const
   {
      return TransformArrayRef(m_array.slice(start), m_func);
   }

   TransformArrayRef slice(unsigned start, unsigned length) const
   {
      return TransformArrayRef(m_array.slice(start, length), m_func);
   }

private:
   llvm::ArrayRef<Orig> m_array;
   FuncTy m_func;
};

template <typename Orig, typename Proj>
TransformArrayRef<std::function<Proj(Orig)>>
makeTransformArrayRef(llvm::ArrayRef<Orig> array, std::function<Proj (Orig)> func)
{
   return TransformArrayRef<decltype(func)>(array, func);
}

} // polar

#endif
