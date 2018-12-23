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

#ifndef POLARPHP_BASIC_ADT_PACKED_VECTOR_H
#define POLARPHP_BASIC_ADT_PACKED_VECTOR_H

#include "polarphp/basic/adt/BitVector.h"
#include <cassert>
#include <limits>

namespace polar {
namespace basic {

template <typename T, unsigned BitNum, typename BitVectorTy, bool isSigned>
class PackedVectorBase;

// This won't be necessary if we can specialize members without specializing
// the parent template.
template <typename T, unsigned BitNum, typename BitVectorTy>
class PackedVectorBase<T, BitNum, BitVectorTy, false>
{
protected:
   static T getValue(const BitVectorTy &bits, unsigned idx)
   {
      T value = T();
      for (unsigned i = 0; i != BitNum; ++i) {
         value = T(value | ((bits[(idx << (BitNum-1)) + i] ? 1UL : 0UL) << i));
      }
      return value;
   }

   static void setValue(BitVectorTy &bits, unsigned idx, T value)
   {
      assert((value >> BitNum) == 0 && "value is too big");
      for (unsigned i = 0; i != BitNum; ++i) {
         bits[(idx << (BitNum-1)) + i] = value & (T(1) << i);
      }
   }
};

template <typename T, unsigned BitNum, typename BitVectorTy>
class PackedVectorBase<T, BitNum, BitVectorTy, true>
{
protected:
   static T getValue(const BitVectorTy &bits, unsigned idx)
   {
      T value = T();
      for (unsigned i = 0; i != BitNum-1; ++i) {
         value = T(value | ((bits[(idx << (BitNum-1)) + i] ? 1UL : 0UL) << i));
      }
      if (bits[(idx << (BitNum-1)) + BitNum-1]) {
         value = ~value;
      }
      return value;
   }

   static void setValue(BitVectorTy &bits, unsigned idx, T value)
   {
      if (value < 0) {
         value = ~value;
         bits.set((idx << (BitNum-1)) + BitNum-1);
      }
      assert((value >> (BitNum-1)) == 0 && "value is too big");
      for (unsigned i = 0; i != BitNum-1; ++i) {
         bits[(idx << (BitNum-1)) + i] = value & (T(1) << i);
      }
   }
};

/// \brief Store a vector of values using a specific number of m_bits for each
/// value. Both signed and unsigned types can be used, e.g
/// @code
///   PackedVector<signed, 2> vec;
/// @endcode
/// will create a vector accepting values -2, -1, 0, 1. Any other value will hit
/// an assertion.
template <typename T, unsigned BitNum, typename BitVectorTy = BitVector>
class PackedVector : public PackedVectorBase<T, BitNum, BitVectorTy,
      std::numeric_limits<T>::is_signed>
{
   BitVectorTy m_bits;
   using base = PackedVectorBase<T, BitNum, BitVectorTy,
   std::numeric_limits<T>::is_signed>;

public:
   class Reference
   {
      PackedVector &m_vector;
      const unsigned m_idx;

   public:
      Reference() = delete;
      Reference(PackedVector &vec, unsigned idx) : m_vector(vec), m_idx(idx) {}

      Reference &operator=(T value)
      {
         m_vector.setValue(m_vector.m_bits, m_idx, value);
         return *this;
      }

      operator T() const
      {
         return m_vector.getValue(m_vector.m_bits, m_idx);
      }
   };

   PackedVector() = default;
   explicit PackedVector(unsigned size) : m_bits(size << (BitNum-1))
   {}

   bool empty() const
   {
      return m_bits.empty();
   }

   unsigned size() const
   {
      return m_bits.size() >> (BitNum - 1);
   }

   unsigned getSize() const
   {
      return size();
   }

   void clear()
   {
      m_bits.clear();
   }

   void resize(unsigned N)
   {
      m_bits.resize(N << (BitNum - 1));
   }

   void reserve(unsigned N)
   {
      m_bits.reserve(N << (BitNum-1));
   }

   PackedVector &reset()
   {
      m_bits.reset();
      return *this;
   }

   void push_back(T value)
   {
      resize(size()+1);
      (*this)[size()-1] = value;
   }

   Reference operator[](unsigned idx)
   {
      return Reference(*this, idx);
   }

   T operator[](unsigned idx) const
   {
      return base::getValue(m_bits, idx);
   }

   bool operator==(const PackedVector &other) const
   {
      return m_bits == other.m_bits;
   }

   bool operator!=(const PackedVector &other) const
   {
      return m_bits != other.m_bits;
   }

   PackedVector &operator|=(const PackedVector &other)
   {
      m_bits |= other.m_bits;
      return *this;
   }
};

// Leave BitNum=0 undefined.
template <typename T>
class PackedVector<T, 0>;

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_PACKED_VECTOR_H
