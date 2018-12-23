// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/29.

#ifndef POLARPHP_BASIC_ADT_ARRAY_REF_H
#define POLARPHP_BASIC_ADT_ARRAY_REF_H

#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/StlExtras.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

namespace polar {
namespace basic {

/// ArrayRef - Represent a constant reference to an array (0 or more elements
/// consecutively in memory), i.e. a start pointer and a length.  It allows
/// various APIs to take consecutive elements easily and conveniently.
///
/// This class does not own the underlying data, it is expected to be used in
/// situations where the data resides in some other buffer, whose lifetime
/// extends past that of the ArrayRef. For this reason, it is not in general
/// safe to store an ArrayRef.
///
/// This is intended to be trivially copyable, so it should be passed by
/// value.
template<typename T>
class POLAR_NODISCARD ArrayRef
{
public:
   using iterator = const T *;
   using const_iterator = const T *;
   using size_type = size_t;
   using reverse_iterator = std::reverse_iterator<iterator>;

private:
   /// The start of the array, in an external buffer.
   const T *m_data = nullptr;

   /// The number of elements.
   size_type m_length = 0;

public:
   /// @name Constructors
   /// @{

   /// Construct an empty ArrayRef.
   /*implicit*/ ArrayRef() = default;

   /// Construct an empty ArrayRef from None.
   /*implicit*/ ArrayRef(std::nullopt_t)
   {}

   /// Construct an ArrayRef from a single element.
   /*implicit*/ ArrayRef(const T &oneElt)
      : m_data(&oneElt), m_length(1)
   {}

   /// Construct an ArrayRef from a pointer and length.
   /*implicit*/ ArrayRef(const T *data, size_t length)
      : m_data(data), m_length(length)
   {}

   /// Construct an ArrayRef from a range.
   ArrayRef(const T *begin, const T *end)
      : m_data(begin), m_length(end - begin)
   {}

   /// Construct an ArrayRef from a SmallVector. This is templated in order to
   /// avoid instantiating SmallVectorTemplateCommon<T> whenever we
   /// copy-construct an ArrayRef.
   template<typename U>
   /*implicit*/ ArrayRef(const SmallVectorTemplateCommon<T, U> &vector)
      : m_data(vector.getData()), m_length(vector.getSize())
   {}

   /// Construct an ArrayRef from a std::vector.
   template<typename A>
   /*implicit*/ ArrayRef(const std::vector<T, A> &vector)
      : m_data(vector.data()), m_length(vector.size())
   {}

   /// Construct an ArrayRef from a std::array
   template <size_t N>
   /*implicit*/ constexpr ArrayRef(const std::array<T, N> &array)
      : m_data(array.data()), m_length(N)
   {}

   /// Construct an ArrayRef from a C array.
   template <size_t N>
   /*implicit*/ constexpr ArrayRef(const T (&array)[N])
      : m_data(array), m_length(N)
   {}

   /// Construct an ArrayRef from a std::initializer_list.
   /*implicit*/ ArrayRef(const std::initializer_list<T> &vector)
      : m_data(vector.begin() == vector.end() ? (T*)nullptr : vector.begin()),
        m_length(vector.size())
   {}

   /// Construct an ArrayRef<const T*> from ArrayRef<T*>. This uses SFINAE to
   /// ensure that only ArrayRefs of pointers can be converted.
   template <typename U>
   ArrayRef(
         const ArrayRef<U *> &array,
         typename std::enable_if<
         std::is_convertible<U *const *, T const *>::value>::type * = nullptr)
      : m_data(array.getData()), m_length(array.getSize()) {}

   /// Construct an ArrayRef<const T*> from a SmallVector<T*>. This is
   /// templated in order to avoid instantiating SmallVectorTemplateCommon<T>
   /// whenever we copy-construct an ArrayRef.
   template<typename U, typename DummyT>
   /*implicit*/ ArrayRef(
         const SmallVectorTemplateCommon<U *, DummyT> &vector,
         typename std::enable_if<
         std::is_convertible<U *const *, T const *>::value>::type * = nullptr)
      : m_data(vector.getData()), m_length(vector.getSize())
   {}

   /// Construct an ArrayRef<const T*> from std::vector<T*>. This uses SFINAE
   /// to ensure that only vectors of pointers can be converted.
   template<typename U, typename A>
   ArrayRef(const std::vector<U *, A> &vector,
            typename std::enable_if<
            std::is_convertible<U *const *, T const *>::value>::type* = nullptr)
      : m_data(vector.getData()), m_length(vector.getSize()) {}

   /// @}
   /// @name Simple Operations
   /// @{

   iterator begin() const
   {
      return m_data;
   }

   iterator end() const
   {
      return m_data + m_length;
   }

   reverse_iterator rbegin() const
   {
      return reverse_iterator(end());
   }

   reverse_iterator rend() const
   {
      return reverse_iterator(begin());
   }

   /// empty - Check if the array is empty.
   bool empty() const
   {
      return m_length == 0;
   }

   const T *getData() const
   {
      return m_data;
   }

   /// size - Get the array size.
   size_t getSize() const
   {
      return m_length;
   }

   /// front - Get the first element.
   const T &getFront() const
   {
      assert(!empty());
      return m_data[0];
   }

   /// back - Get the last element.
   const T &getBack() const
   {
      assert(!empty());
      return m_data[m_length - 1];
   }

   const T &front() const
   {
      return getFront();
   }

   /// back - Get the last element.
   const T &back() const
   {
      return getBack();
   }

   // copy - Allocate copy in Allocator and return ArrayRef<T> to it.
   template <typename Allocator>
   ArrayRef<T> copy(Allocator &allocator)
   {
      T *buffer = allocator.template allocate<T>(m_length);
      std::uninitialized_copy(begin(), end(), buffer);
      return ArrayRef<T>(buffer, m_length);
   }

   /// equals - Check for element-wise equality.
   bool equals(ArrayRef rhs) const
   {
      if (m_length != rhs.m_length) {
         return false;
      }
      return std::equal(begin(), end(), rhs.begin());
   }

   /// slice(n, m) - Chop off the first N elements of the array, and keep M
   /// elements in the array.
   ArrayRef<T> slice(size_t start, size_t size) const
   {
      assert(start + size <= getSize() && "Invalid specifier");
      return ArrayRef<T>(getData() + start, size);
   }

   /// slice(n) - Chop off the first N elements of the array.
   ArrayRef<T> slice(size_t size) const
   {
      return slice(size, getSize() - size);
   }

   /// \brief Drop the first \p N elements of the array.
   ArrayRef<T> dropFront(size_t size = 1) const
   {
      assert(getSize() >= size && "Dropping more elements than exist");
      return slice(size, getSize() - size);
   }

   /// \brief Drop the last \p N elements of the array.
   ArrayRef<T> dropBack(size_t size = 1) const
   {
      assert(getSize() >= size && "Dropping more elements than exist");
      return slice(0, getSize() - size);
   }

   /// \brief Return a copy of *this with the first N elements satisfying the
   /// given predicate removed.
   template <class PredicateType>
   ArrayRef<T> dropWhile(PredicateType pred) const
   {
      return ArrayRef<T>(find_if_not(*this, pred), end());
   }

   /// \brief Return a copy of *this with the first N elements not satisfying
   /// the given predicate removed.
   template <class PredicateType>
   ArrayRef<T> dropUntil(PredicateType pred) const
   {
      return ArrayRef<T>(find_if(*this, pred), end());
   }

   /// \brief Return a copy of *this with only the first \p N elements.
   ArrayRef<T> takeFront(size_t size = 1) const
   {
      if (size >= getSize()) {
         return *this;
      }
      return dropBack(getSize() - size);
   }

   /// \brief Return a copy of *this with only the last \p N elements.
   ArrayRef<T> takeBack(size_t size = 1) const
   {
      if (size >= getSize()) {
         return *this;
      }
      return dropFront(getSize() - size);
   }

   /// \brief Return the first N elements of this Array that satisfy the given
   /// predicate.
   template <class PredicateType>
   ArrayRef<T> takeWhile(PredicateType pred) const
   {
      return ArrayRef<T>(begin(), find_if_not(*this, pred));
   }

   /// \brief Return the first N elements of this Array that don't satisfy the
   /// given predicate.
   template <class PredicateType>
   ArrayRef<T> takeUntil(PredicateType pred) const
   {
      return ArrayRef<T>(begin(), find_if(*this, pred));
   }

   /// @}
   /// @name Operator Overloads
   /// @{
   const T &operator[](size_t index) const
   {
      assert(index < m_length && "Invalid index!");
      return m_data[index];
   }

   /// Disallow accidental assignment from a temporary.
   ///
   /// The declaration here is extra complicated so that "arrayRef = {}"
   /// continues to select the move assignment operator.
   template <typename U>
   typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type &
   operator=(U &&temporary) = delete;

   /// Disallow accidental assignment from a temporary.
   ///
   /// The declaration here is extra complicated so that "arrayRef = {}"
   /// continues to select the move assignment operator.
   template <typename U>
   typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type &
   operator=(std::initializer_list<U>) = delete;

   /// @}
   /// @name Expensive Operations
   /// @{
   std::vector<T> getVector() const
   {
      return std::vector<T>(m_data, m_data + m_length);
   }

   /// @}
   /// @name Conversion operators
   /// @{
   operator std::vector<T>() const
   {
      return std::vector<T>(m_data, m_data + m_length);
   }

   /// @}
};

/// MutableArrayRef - Represent a mutable reference to an array (0 or more
/// elements consecutively in memory), i.e. a start pointer and a length.  It
/// allows various APIs to take and modify consecutive elements easily and
/// conveniently.
///
/// This class does not own the underlying data, it is expected to be used in
/// situations where the data resides in some other buffer, whose lifetime
/// extends past that of the MutableArrayRef. For this reason, it is not in
/// general safe to store a MutableArrayRef.
///
/// This is intended to be trivially copyable, so it should be passed by
/// value.
template<typename T>
class POLAR_NODISCARD MutableArrayRef : public ArrayRef<T>
{
public:
   using iterator = T *;
   using reverse_iterator = std::reverse_iterator<iterator>;

   /// Construct an empty MutableArrayRef.
   /*implicit*/ MutableArrayRef() = default;

   /// Construct an empty MutableArrayRef from None.
   /*implicit*/ MutableArrayRef(std::nullopt_t) : ArrayRef<T>()
   {}

   /// Construct an MutableArrayRef from a single element.
   /*implicit*/ MutableArrayRef(T &oneElt) : ArrayRef<T>(oneElt)
   {}

   /// Construct an MutableArrayRef from a pointer and length.
   /*implicit*/ MutableArrayRef(T *data, size_t length)
      : ArrayRef<T>(data, length)
   {}

   /// Construct an MutableArrayRef from a range.
   MutableArrayRef(T *begin, T *end) : ArrayRef<T>(begin, end)
   {}

   /// Construct an MutableArrayRef from a SmallVector.
   /*implicit*/ MutableArrayRef(SmallVectorImpl<T> &vector)
      : ArrayRef<T>(vector)
   {}

   /// Construct a MutableArrayRef from a std::vector.
   /*implicit*/ MutableArrayRef(std::vector<T> &vector)
      : ArrayRef<T>(vector)
   {}

   /// Construct an ArrayRef from a std::array
   template <size_t N>
   /*implicit*/ constexpr MutableArrayRef(std::array<T, N> &array)
      : ArrayRef<T>(array)
   {}

   /// Construct an MutableArrayRef from a C array.
   template <size_t N>
   /*implicit*/ constexpr MutableArrayRef(T (&array)[N])
      : ArrayRef<T>(array)
   {}

   T *getData() const
   {
      return const_cast<T*>(ArrayRef<T>::getData());
   }

   iterator begin() const
   {
      return getData();
   }

   iterator end() const
   {
      return getData() + this->getSize();
   }

   reverse_iterator rbegin() const
   {
      return reverse_iterator(end());
   }

   reverse_iterator rend() const
   {
      return reverse_iterator(begin());
   }

   /// front - Get the first element.
   T &front() const
   {
      assert(!this->empty());
      return getData()[0];
   }

   /// back - Get the last element.
   T &back() const
   {
      assert(!this->empty());
      return getData()[this->getSize() - 1];
   }

   /// slice(n, m) - Chop off the first N elements of the array, and keep M
   /// elements in the array.
   MutableArrayRef<T> slice(size_t start, size_t size) const
   {
      assert(start + size <= this->getSize() && "Invalid specifier");
      return MutableArrayRef<T>(getData() + start, size);
   }

   /// slice(n) - Chop off the first N elements of the array.
   MutableArrayRef<T> slice(size_t size) const
   {
      return slice(size, this->getSize() - size);
   }

   /// \brief Drop the first \p N elements of the array.
   MutableArrayRef<T> dropFront(size_t size = 1) const
   {
      assert(this->getSize() >= size && "Dropping more elements than exist");
      return slice(size, this->getSize() - size);
   }

   MutableArrayRef<T> dropBack(size_t size = 1) const
   {
      assert(this->getSize() >= size && "Dropping more elements than exist");
      return slice(0, this->getSize() - size);
   }

   /// \brief Return a copy of *this with the first N elements satisfying the
   /// given predicate removed.
   template <class PredicateType>
   MutableArrayRef<T> dropWhile(PredicateType pred) const
   {
      return MutableArrayRef<T>(find_if_not(*this, pred), end());
   }

   /// \brief Return a copy of *this with the first N elements not satisfying
   /// the given predicate removed.
   template <class PredicateType>
   MutableArrayRef<T> dropUntil(PredicateType pred) const
   {
      return MutableArrayRef<T>(find_if(*this, pred), end());
   }

   /// \brief Return a copy of *this with only the first \p N elements.
   MutableArrayRef<T> takeFront(size_t size = 1) const
   {
      if (size >= this->getSize()) {
         return *this;
      }
      return dropBack(this->getSize() - size);
   }

   /// \brief Return a copy of *this with only the last \p N elements.
   MutableArrayRef<T> takeBack(size_t size = 1) const
   {
      if (size >= this->getSize()) {
         return *this;
      }
      return dropFront(this->getSize() - size);
   }

   /// \brief Return the first N elements of this Array that satisfy the given
   /// predicate.
   template <class PredicateType>
   MutableArrayRef<T> takeWhile(PredicateType pred) const
   {
      return MutableArrayRef<T>(begin(), find_if_not(*this, pred));
   }

   /// \brief Return the first N elements of this Array that don't satisfy the
   /// given predicate.
   template <class PredicateType>
   MutableArrayRef<T> takeUntil(PredicateType pred) const
   {
      return MutableArrayRef<T>(begin(), find_if(*this, pred));
   }

   /// @}
   /// @name Operator Overloads
   /// @{
   T &operator[](size_t index) const
   {
      assert(index < this->getSize() && "Invalid index!");
      return getData()[index];
   }
};

/// This is a MutableArrayRef that owns its array.
template <typename T>
class OwningArrayRef : public MutableArrayRef<T>
{
public:
   OwningArrayRef() = default;
   OwningArrayRef(size_t size) : MutableArrayRef<T>(new T[size], size)
   {}

   OwningArrayRef(ArrayRef<T> data)
      : MutableArrayRef<T>(new T[data.getSize()], data.getSize())
   {
      std::copy(data.begin(), data.end(), this->begin());
   }

   OwningArrayRef(OwningArrayRef &&other)
   {
      *this = other;
   }

   OwningArrayRef &operator=(OwningArrayRef &&other)
   {
      delete[] this->getData();
      MutableArrayRef<T>::operator=(other);
      other.MutableArrayRef<T>::operator=(MutableArrayRef<T>());
      return *this;
   }

   ~OwningArrayRef()
   {
      delete[] this->getData();
   }
};

/// @name ArrayRef Convenience constructors
/// @{

/// Construct an ArrayRef from a single element.
template<typename T>
ArrayRef<T> make_array_ref(const T &oneElt)
{
   return oneElt;
}

/// Construct an ArrayRef from a pointer and length.
template<typename T>
ArrayRef<T> make_array_ref(const T *data, size_t length)
{
   return ArrayRef<T>(data, length);
}

/// Construct an ArrayRef from a range.
template<typename T>
ArrayRef<T> make_array_ref(const T *begin, const T *end)
{
   return ArrayRef<T>(begin, end);
}

/// Construct an ArrayRef from a SmallVector.
template <typename T>
ArrayRef<T> make_array_ref(const SmallVectorImpl<T> &vector)
{
   return vector;
}

/// Construct an ArrayRef from a SmallVector.
template <typename T, unsigned N>
ArrayRef<T> make_array_ref(const SmallVector<T, N> &vector)
{
   return vector;
}

/// Construct an ArrayRef from a std::vector.
template<typename T>
ArrayRef<T> make_array_ref(const std::vector<T> &vector)
{
   return vector;
}

/// Construct an ArrayRef from an ArrayRef (no-op) (const)
template <typename T> ArrayRef<T> make_array_ref(const ArrayRef<T> &vector)
{
   return vector;
}

/// Construct an ArrayRef from an ArrayRef (no-op)
template <typename T> ArrayRef<T> &make_array_ref(ArrayRef<T> &vector)
{
   return vector;
}

/// Construct an ArrayRef from a C array.
template<typename T, size_t N>
ArrayRef<T> make_array_ref(const T (&array)[N])
{
   return ArrayRef<T>(array);
}

/// Construct a MutableArrayRef from a single element.
template<typename T>
MutableArrayRef<T> make_mutable_array_ref(T &oneElt)
{
   return oneElt;
}

/// Construct a MutableArrayRef from a pointer and length.
template<typename T>
MutableArrayRef<T> make_mutable_array_ref(T *data, size_t length)
{
   return MutableArrayRef<T>(data, length);
}

/// @}
/// @name ArrayRef Comparison Operators
/// @{

template<typename T>
inline bool operator==(ArrayRef<T> lhs, ArrayRef<T> rhs)
{
   return lhs.equals(rhs);
}

template<typename T>
inline bool operator!=(ArrayRef<T> lhs, ArrayRef<T> rhs)
{
   return !(lhs == rhs);
}

/// @}

} // basic

namespace utils {

// ArrayRefs can be treated like a POD type.
template <typename T> struct IsPodLike;
template <typename T> struct IsPodLike<polar::basic::ArrayRef<T>>
{
   static const bool value = true;
};

template <typename T>
polar::basic::HashCode hash_value(polar::basic::ArrayRef<T> array)
{
   return polar::basic::hash_combine_range(array.begin(), array.end());
}

} // utils
} // polar

#endif // POLARPHP_BASIC_ADT_ARRAY_REF_H
