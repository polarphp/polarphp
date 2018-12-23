// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/05/28.

#ifndef POLARPHP_BASIC_ADT_SMALL_VECTOR_H
#define POLARPHP_BASIC_ADT_SMALL_VECTOR_H

#include "polarphp/basic/adt/IteratorRange.h"
#include "polarphp/utils/AlignOf.h"
#include "polarphp/utils/MathExtras.h"
#include "polarphp/utils/TypeTraits.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/MemoryAlloc.h"
#include "polarphp/global/CompilerDetection.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace polar {
namespace basic {

namespace utils = polar::utils;

/// This is all the non-templated stuff common to all SmallVectors.
class SmallVectorBase
{
protected:
   void *m_beginX;
   unsigned m_size = 0;
   unsigned m_capacity;

protected:
   SmallVectorBase(void *firstEl, size_t capacity)
      : m_beginX(firstEl),
        m_capacity(capacity)
   {}

   /// This is an implementation of the grow() method which only works
   /// on POD-like data types and is out of line to reduce code duplication.
   void growPod(void *firstEl, size_t minCapacity, size_t tsize);

public:
   size_t getSize() const
   {
      return m_size;
   }

   size_t size() const
   {
      return m_size;
   }

   size_t getCapacity() const
   {
      return m_capacity;
   }

   POLAR_NODISCARD bool empty() const
   {
      return !m_size;
   }

   /// Set the array size to \p N, which the current array must have enough
   /// capacity for.
   ///
   /// This does not construct or destroy any elements in the vector.
   ///
   /// Clients can use this in conjunction with getCapacity() to write past the end
   /// of the buffer when they know that more elements are available, and only
   /// update the size later. This avoids the cost of value initializing elements
   /// which will only be overwritten.
   void setSize(size_t size)
   {
      assert(size <= getCapacity());
      this->m_size = size;
   }
};

/// Figure out the offset of the first element.
template <typename T, typename = void>
struct SmallVectorAlignmentAndSize
{
   polar::utils::AlignedCharArrayUnion<SmallVectorBase> m_base;
   polar::utils::AlignedCharArrayUnion<T> m_firstEl;
};

/// This is the part of SmallVectorTemplateBase which does not depend on whether
/// the type T is a POD. The extra dummy template argument is used by ArrayRef
/// to avoid unnecessarily requiring T to be complete.
template <typename T, typename = void>
class SmallVectorTemplateCommon : public SmallVectorBase
{
private:
   /// Find the address of the first element.  For this pointer math to be valid
   /// with small-size of 0 for T with lots of alignment, it's important that
   /// SmallVectorStorage is properly-aligned even for small-size of 0.
   void *getFirstEl() const
   {
      return const_cast<void *>(reinterpret_cast<const void *>(
                                   reinterpret_cast<const char *>(this) +
                                   offsetof(SmallVectorAlignmentAndSize<T>, m_firstEl)));
   }
   // Space after 'FirstEl' is clobbered, do not add any instance vars after it.

protected:
   SmallVectorTemplateCommon(size_t size)
      : SmallVectorBase(getFirstEl(), size)
   {}

   void growPod(size_t minCapacity, size_t tsize)
   {
      SmallVectorBase::growPod(getFirstEl(), minCapacity, tsize);
   }

   /// Return true if this is a smallvector which has not had dynamic
   /// memory allocated for it.
   bool isSmall() const
   {
      return m_beginX == getFirstEl();
   }

   /// Put this vector in a state of being small.
   void resetToSmall()
   {
      m_beginX = getFirstEl();
      m_size = m_capacity = 0; // FIXME: Setting Capacity to 0 is suspect.
   }

public:
   using size_type = size_t;
   using difference_type = ptrdiff_t;
   using value_type = T;
   using iterator = T *;
   using const_iterator = const T *;

   using const_reverse_iterator = std::reverse_iterator<const_iterator>;
   using reverse_iterator = std::reverse_iterator<iterator>;

   using reference = T &;
   using const_reference = const T &;
   using pointer = T *;
   using const_pointer = const T *;

   // forward iterator creation methods.
   POLAR_ATTRIBUTE_ALWAYS_INLINE
   iterator begin()
   {
      return (iterator)m_beginX;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   const_iterator begin() const
   {
      return (const_iterator)m_beginX;
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   iterator end()
   {
      return begin() + getSize();
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   const_iterator end() const
   {
      return begin() + getSize();
   }

public:
   // reverse iterator creation methods.
   reverse_iterator rbegin()
   {
      return reverse_iterator(end());
   }

   const_reverse_iterator rbegin() const
   {
      return const_reverse_iterator(end());
   }

   reverse_iterator rend()
   {
      return reverse_iterator(begin());
   }

   const_reverse_iterator rend() const
   {
      return const_reverse_iterator(begin());
   }

   size_type getSizeInBytes() const
   {
      return getSize() * sizeof(T);
   }

   size_type getMaxSize() const
   {
      return size_type(-1) / sizeof(T);
   }

   size_t getCapacityInBytes() const
   {
      return getCapacity() * sizeof(T);
   }

   /// Return a pointer to the vector's buffer, even if empty().
   pointer getData()
   {
      return pointer(begin());
   }

   /// Return a pointer to the vector's buffer, even if empty().
   const_pointer getData() const
   {
      return const_pointer(begin());
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   reference operator[](size_type idx)
   {
      assert(idx < getSize());
      return begin()[idx];
   }

   POLAR_ATTRIBUTE_ALWAYS_INLINE
   const_reference operator[](size_type idx) const
   {
      assert(idx < getSize());
      return begin()[idx];
   }

   reference getFront() {
      assert(!empty());
      return begin()[0];
   }

   const_reference getFront() const
   {
      assert(!empty());
      return begin()[0];
   }

   reference front()
   {
      return getFront();
   }

   const_reference front() const
   {
      return getFront();
   }

   reference getBack()
   {
      assert(!empty());
      return end()[-1];
   }
   const_reference getBack() const
   {
      assert(!empty());
      return end()[-1];
   }

   reference back()
   {
      return getBack();
   }

   const_reference back() const
   {
      return getBack();
   }
};

/// SmallVectorTemplateBase<IsPodLike = false> - This is where we put method
/// implementations that are designed to work with non-POD-like T's.
template <typename T, bool = polar::utils::IsPodLike<T>::value>
class SmallVectorTemplateBase : public SmallVectorTemplateCommon<T>
{
protected:
   SmallVectorTemplateBase(size_t size) : SmallVectorTemplateCommon<T>(size)
   {}

   static void destroyRange(T *start, T *end)
   {
      while (start != end) {
         --end;
         end->~T();
      }
   }

   /// Move the range [I, E) into the uninitialized memory starting with "Dest",
   /// constructing elements as needed.
   template<typename It1, typename It2>
   static void uninitializedMove(It1 Iter, It1 end, It2 dest)
   {
      std::uninitialized_copy(std::make_move_iterator(Iter),
                              std::make_move_iterator(end), dest);
   }

   /// Copy the range [I, E) onto the uninitialized memory starting with "Dest",
   /// constructing elements as needed.
   template<typename It1, typename It2>
   static void uninitializedCopy(It1 Iter, It1 end, It2 dest)
   {
      std::uninitialized_copy(Iter, end, dest);
   }

   /// Grow the allocated memory (without initializing new elements), doubling
   /// the size of the allocated memory. Guarantees space for at least one more
   /// element, or minSize more elements if specified.
   void grow(size_t minSize = 0);

public:

   void pushBack(const T &element)
   {
      if (POLAR_UNLIKELY(this->getSize() >= this->getCapacity())) {
         this->grow();
      }
      ::new ((void*) this->end()) T(element);
      this->setSize(this->getSize() + 1);
   }

   inline void push_back(const T &element)
   {
      pushBack(element);
   }

   void pushBack(T &&element)
   {
      if (POLAR_UNLIKELY(this->getSize() >= this->getCapacity())) {
         this->grow();
      }
      ::new ((void*) this->end()) T(::std::move(element));
      this->setSize(this->getSize() + 1);
   }

   inline void push_back(T &&element)
   {
      pushBack(std::move(element));
   }

   void popBack()
   {
      this->setSize(this->getSize() - 1);
      this->end()->~T();
   }

   inline void pop_back()
   {
      popBack();
   }
};

// Define this out-of-line to dissuade the C++ compiler from inlining it.
template <typename T, bool IsPodLike>
void SmallVectorTemplateBase<T, IsPodLike>::grow(size_t minSize)
{
   if (minSize > UINT32_MAX) {
      utils::report_bad_alloc_error("SmallVector capacity overflow during allocation");
   }
   // Always grow, even from zero.
   size_t newCapacity = size_t(polar::utils::next_power_of_two(this->getCapacity() + 2));
   newCapacity = std::min(std::max(newCapacity, minSize), size_t(UINT32_MAX));
   T *newElts = static_cast<T*>(polar::utils::safe_malloc(newCapacity*sizeof(T)));

   // Move the elements over.
   this->uninitializedMove(this->begin(), this->end(), newElts);

   // Destroy the original elements.
   destroyRange(this->begin(), this->end());

   // If this wasn't grown from the inline copy, deallocate the old space.
   if (!this->isSmall()) {
      free(this->begin());
   }
   this->m_beginX = newElts;
   this->m_capacity = newCapacity;
}


/// SmallVectorTemplateBase<IsPodLike = true> - This is where we put method
/// implementations that are designed to work with POD-like T's.
template <typename T>
class SmallVectorTemplateBase<T, true> : public SmallVectorTemplateCommon<T>
{
protected:
   SmallVectorTemplateBase(size_t size) : SmallVectorTemplateCommon<T>(size)
   {}

   // No need to do a destroy loop for POD's.
   static void destroyRange(T *, T *)
   {}

   /// Move the range [I, E) onto the uninitialized memory
   /// starting with "Dest", constructing elements into it as needed.
   template<typename It1, typename It2>
   static void uninitializedMove(It1 iter, It1 end, It2 dest)
   {
      // Just do a copy.
      uninitializedCopy(iter, end, dest);
   }

   /// Copy the range [I, E) onto the uninitialized memory
   /// starting with "Dest", constructing elements into it as needed.
   template<typename It1, typename It2>
   static void uninitializedCopy(It1 iter, It1 end, It2 dest)
   {
      // Arbitrary iterator types; just use the basic implementation.
      std::uninitialized_copy(iter, end, dest);
   }

   /// Copy the range [I, E) onto the uninitialized memory
   /// starting with "Dest", constructing elements into it as needed.
   template <typename T1, typename T2>
   static void uninitializedCopy(
         T1 *iter, T1 *end, T2 *dest,
         typename std::enable_if<std::is_same<typename std::remove_const<T1>::type,
         T2>::value>::type * = nullptr)
   {
      // Use memcpy for PODs iterated by pointers (which includes SmallVector
      // iterators): std::uninitialized_copy optimizes to memmove, but we can
      // use memcpy here. Note that iter and end are iterators and thus might be
      // invalid for memcpy if they are equal.
      if (iter != end) {
         std::memcpy(reinterpret_cast<void *>(dest), iter, (end - iter) * sizeof(T));
      }
   }

   /// Double the size of the allocated memory, guaranteeing space for at
   /// least one more element or minSize if specified.
   void grow(size_t minSize = 0)
   {
      this->growPod(minSize, sizeof(T));
   }

public:
   void pushBack(const T &element)
   {
      if (POLAR_UNLIKELY(this->getSize() >= this->getCapacity())) {
         this->grow();
      }
      memcpy(reinterpret_cast<void *>(this->end()), &element, sizeof(T));
      this->setSize(this->getSize() + 1);
   }

   inline void push_back(const T &element)
   {
      pushBack(element);
   }

   void popBack()
   {
      this->setSize(this->getSize() - 1);
   }

   inline void pop_back()
   {
      popBack();
   }
};

/// This class consists of common code factored out of the SmallVector class to
/// reduce code duplication based on the SmallVector 'N' template parameter.
template <typename T>
class SmallVectorImpl : public SmallVectorTemplateBase<T>
{
   using SuperClass = SmallVectorTemplateBase<T>;

public:
   using iterator = typename SuperClass::iterator;
   using const_iterator = typename SuperClass::const_iterator;
   using ConstIterator = const_iterator;
   using Iterator = iterator;
   using size_type = typename SuperClass::size_type;

protected:
   // Default ctor - Initialize to empty.
   explicit SmallVectorImpl(unsigned N)
      : SmallVectorTemplateBase<T, utils::IsPodLike<T>::value>(N)
   {
   }

public:
   SmallVectorImpl(const SmallVectorImpl &) = delete;

   ~SmallVectorImpl()
   {
      // Subclass has already destructed this vector's elements.
      // If this wasn't grown from the inline copy, deallocate the old space.
      if (!this->isSmall()) {
         free(this->begin());
      }
   }

   void clear()
   {
      this->destroyRange(this->begin(), this->end());
      this->m_size = 0;
   }

   void resize(size_type size)
   {
      if (size < this->getSize()) {
         this->destroyRange(this->begin() + size, this->end());
         this->setSize(size);
      } else if (size > this->getSize()) {
         if (this->getCapacity() < size) {
            this->grow(size);
         }
         for (auto iter = this->end(), end = this->begin() + size; iter != end; ++iter) {
            new (&*iter) T();
         }
         this->setSize(size);
      }
   }

   void resize(size_type size, const T &newValue)
   {
      if (size < this->getSize()) {
         this->destroyRange(this->begin() + size, this->end());
         this->setSize(size);
      } else if (size > this->getSize()) {
         if (this->getCapacity() < size) {
            this->grow(size);
         }
         std::uninitialized_fill(this->end(), this->begin() + size, newValue);
         this->setSize(size);
      }
   }

   void reserve(size_type size)
   {
      if (this->getCapacity() < size) {
         this->grow(size);
      }
   }

   POLAR_NODISCARD T popBackValue()
   {
      T result = ::std::move(this->getBack());
      this->popBack();
      return result;
   }

   void swap(SmallVectorImpl &rhs);

   /// Add the specified range to the end of the SmallVector.
   template <typename InputIter,
             typename = typename std::enable_if<std::is_convertible<
                                                   typename std::iterator_traits<InputIter>::iterator_category,
                                                   std::input_iterator_tag>::value>::type>
   void append(InputIter start, InputIter end)
   {
      size_type numInputs = std::distance(start, end);
      // Grow allocated space if needed.
      if (numInputs > this->getCapacity() - this->getSize()) {
         this->grow(this->getSize() + numInputs);
      }
      // Copy the new elements over.
      this->uninitializedCopy(start, end, this->end());
      this->setSize(this->getSize() + numInputs);
   }

   /// Add the specified range to the end of the SmallVector.
   void append(size_type numInputs, const T &element)
   {
      // Grow allocated space if needed.
      if (numInputs > this->getCapacity() - this->getSize()) {
         this->grow(this->getSize() + numInputs);
      }
      // Copy the new elements over.
      std::uninitialized_fill_n(this->end(), numInputs, element);
      this->setSize(this->getSize() + numInputs);
   }

   void append(std::initializer_list<T> elements)
   {
      append(elements.begin(), elements.end());
   }

   // FIXME: Consider assigning over existing elements, rather than clearing &
   // re-initializing them - for all assign(...) variants.

   void assign(size_type numElts, const T &element)
   {
      clear();
      if (this->getCapacity() < numElts) {
         this->grow(numElts);
      }
      this->setSize(numElts);
      std::uninitialized_fill(this->begin(), this->end(), element);
   }

   template <typename InIter,
             typename = typename std::enable_if<std::is_convertible<
                                                   typename std::iterator_traits<InIter>::iterator_category,
                                                   std::input_iterator_tag>::value>::type>
   void assign(InIter start, InIter end)
   {
      clear();
      append(start, end);
   }

   void assign(std::initializer_list<T> elements)
   {
      clear();
      append(elements);
   }

   iterator erase(const_iterator citer)
   {
      // Just cast away constness because this is a non-const member function.
      iterator iter = const_cast<iterator>(citer);

      assert(iter >= this->begin() && "Iterator to erase is out of bounds.");
      assert(iter < this->end() && "Erasing at past-the-end iterator.");

      iterator removed = iter;
      // Shift all elts down one.
      std::move(iter + 1, this->end(), iter);
      // Drop the last elt.
      this->pop_back();
      return removed;
   }

   iterator erase(const_iterator cstart, const_iterator cend)
   {
      // Just cast away constness because this is a non-const member function.
      iterator startIter = const_cast<iterator>(cstart);
      iterator endIter = const_cast<iterator>(cend);

      assert(startIter >= this->begin() && "Range to erase is out of bounds.");
      assert(startIter <= endIter && "Trying to erase invalid range.");
      assert(endIter <= this->end() && "Trying to erase past the end.");

      iterator removed = startIter;
      // Shift all elts down.
      iterator iter = std::move(endIter, this->end(), startIter);
      // Drop the last elts.
      this->destroyRange(iter, this->end());
      this->setSize(iter - this->begin());
      return removed;
   }

   iterator insert(iterator iter, T &&element)
   {
      if (iter == this->end()) {  // Important special case for empty vector.
         this->pushBack(::std::move(element));
         return this->end() - 1;
      }

      assert(iter >= this->begin() && "Insertion iterator is out of bounds.");
      assert(iter <= this->end() && "Inserting past the end of the vector.");

      if (this->getSize() >= this->getCapacity()) {
         size_t eltNo = iter - this->begin();
         this->grow();
         iter = this->begin() + eltNo;
      }

      ::new ((void*) this->end()) T(::std::move(this->getBack()));
      // Push everything else over.
      std::move_backward(iter, this->end() - 1, this->end());
      this->setSize(this->getSize() + 1);

      // If we just moved the element we're inserting, be sure to update
      // the reference.
      T *eltPtr = &element;
      if (iter <= eltPtr && eltPtr < this->end()) {
         ++eltPtr;
      }
      *iter = ::std::move(*eltPtr);
      return iter;
   }

   iterator insert(iterator iter, const T &element)
   {
      if (iter == this->end()) {  // Important special case for empty vector.
         this->push_back(element);
         return this->end() - 1;
      }

      assert(iter >= this->begin() && "Insertion iterator is out of bounds.");
      assert(iter <= this->end() && "Inserting past the end of the vector.");

      if (this->getSize() >= this->getCapacity()) {
         size_t eltNo = iter - this->begin();
         this->grow();
         iter = this->begin() + eltNo;
      }
      ::new ((void*) this->end()) T(std::move(this->getBack()));
      // Push everything else over.
      std::move_backward(iter, this->end() - 1, this->end());
      this->setSize(this->getSize() + 1);

      // If we just moved the element we're inserting, be sure to update
      // the reference.
      const T *eltPtr = &element;
      if (iter <= eltPtr && eltPtr < this->end()) {
         ++eltPtr;
      }
      *iter = *eltPtr;
      return iter;
   }

   iterator insert(iterator iter, size_type numToInsert, const T &element)
   {
      // Convert iterator to elt# to avoid invalidating iterator when we reserve()
      size_t insertElt = iter - this->begin();

      if (iter == this->end()) {  // Important special case for empty vector.
         append(numToInsert, element);
         return this->begin() + insertElt;
      }

      assert(iter >= this->begin() && "Insertion iterator is out of bounds.");
      assert(iter <= this->end() && "Inserting past the end of the vector.");

      // Ensure there is enough space.
      reserve(this->getSize() + numToInsert);

      // Uninvalidate the iterator.
      iter = this->begin() + insertElt;

      // If there are more elements between the insertion point and the end of the
      // range than there are being inserted, we can use a simple approach to
      // insertion.  Since we already reserved space, we know that this won't
      // reallocate the vector.
      if (size_t(this->end() - iter) >= numToInsert) {
         T *oldEnd = this->end();
         append(std::move_iterator<iterator>(this->end() - numToInsert),
                std::move_iterator<iterator>(this->end()));

         // Copy the existing elements that get replaced.
         std::move_backward(iter, oldEnd - numToInsert, oldEnd);

         std::fill_n(iter, numToInsert, element);
         return iter;
      }

      // Otherwise, we're inserting more elements than exist already, and we're
      // not inserting at the end.

      // Move over the elements that we're about to overwrite.
      T *oldEnd = this->end();
      this->setSize(this->getSize() + numToInsert);
      size_t numOverwritten = oldEnd - iter;
      this->uninitializedMove(iter, oldEnd, this->end() - numOverwritten);

      // Replace the overwritten part.
      std::fill_n(iter, numOverwritten, element);

      // Insert the non-overwritten middle part.
      std::uninitialized_fill_n(oldEnd, numToInsert - numOverwritten, element);
      return iter;
   }

   template <typename ItTy,
             typename = typename std::enable_if<std::is_convertible<
                                                   typename std::iterator_traits<ItTy>::iterator_category,
                                                   std::input_iterator_tag>::value>::type>
   iterator insert(iterator iter, ItTy from, ItTy to)
   {
      // Convert iterator to elt# to avoid invalidating iterator when we reserve()
      size_t insertElt = iter - this->begin();

      if (iter == this->end()) {  // Important special case for empty vector.
         append(from, to);
         return this->begin() + insertElt;
      }

      assert(iter >= this->begin() && "Insertion iterator is out of bounds.");
      assert(iter <= this->end() && "Inserting past the end of the vector.");

      size_t numToInsert = std::distance(from, to);

      // Ensure there is enough space.
      reserve(this->getSize() + numToInsert);

      // Uninvalidate the iterator.
      iter = this->begin() + insertElt;

      // If there are more elements between the insertion point and the end of the
      // range than there are being inserted, we can use a simple approach to
      // insertion.  Since we already reserved space, we know that this won't
      // reallocate the vector.
      if (size_t(this->end() - iter) >= numToInsert) {
         T *oldEnd = this->end();
         append(std::move_iterator<iterator>(this->end() - numToInsert),
                std::move_iterator<iterator>(this->end()));

         // Copy the existing elements that get replaced.
         std::move_backward(iter, oldEnd - numToInsert, oldEnd);

         std::copy(from, to, iter);
         return iter;
      }

      // Otherwise, we're inserting more elements than exist already, and we're
      // not inserting at the end.

      // Move over the elements that we're about to overwrite.
      T *oldEnd = this->end();
      this->setSize(this->getSize() + numToInsert);
      size_t numOverwritten = oldEnd - iter;
      this->uninitializedMove(iter, oldEnd, this->end() - numOverwritten);

      // Replace the overwritten part.
      for (T *targetIter = iter; numOverwritten > 0; --numOverwritten) {
         *targetIter = *from;
         ++targetIter;
         ++from;
      }

      // Insert the non-overwritten middle part.
      this->uninitializedCopy(from, to, oldEnd);
      return iter;
   }

   void insert(iterator iter, std::initializer_list<T> elements)
   {
      insert(iter, elements.begin(), elements.end());
   }

   template <typename... ArgTypes>
   void emplaceBack(ArgTypes &&... args)
   {
      if (POLAR_UNLIKELY(this->getSize() >= this->getCapacity())) {
         this->grow();
      }
      ::new ((void *)this->end()) T(std::forward<ArgTypes>(args)...);
      this->setSize(this->getSize() + 1);
   }

   template <typename... ArgTypes>
   inline void emplace_back(ArgTypes &&... args)
   {
      emplaceBack(std::forward<ArgTypes>(args)...);
   }

   SmallVectorImpl &operator=(const SmallVectorImpl &rhs);

   SmallVectorImpl &operator=(SmallVectorImpl &&rhs);

   bool operator==(const SmallVectorImpl &rhs) const
   {
      if (this->getSize() != rhs.getSize()) {
         return false;
      }
      return std::equal(this->begin(), this->end(), rhs.begin());
   }

   bool operator!=(const SmallVectorImpl &rhs) const
   {
      return !(*this == rhs);
   }

   bool operator<(const SmallVectorImpl &rhs) const
   {
      return std::lexicographical_compare(this->begin(), this->end(),
                                          rhs.begin(), rhs.end());
   }
};

template <typename T>
void SmallVectorImpl<T>::swap(SmallVectorImpl<T> &rhs)
{
   if (this == &rhs) {
      return;
   }
   // We can only avoid copying elements if neither vector is small.
   if (!this->isSmall() && !rhs.isSmall()) {
      std::swap(this->m_beginX, rhs.m_beginX);
      std::swap(this->m_size, rhs.m_size);
      std::swap(this->m_capacity, rhs.m_capacity);
      return;
   }
   if (rhs.getSize() > this->getCapacity()) {
      this->grow(rhs.getSize());
   }
   if (this->getSize() > rhs.getCapacity()) {
      rhs.grow(this->getSize());
   }
   // Swap the shared elements.
   size_t numShared = this->getSize();
   if (numShared > rhs.getSize()) {
      numShared = rhs.getSize();
   }
   for (size_type i = 0; i != numShared; ++i) {
      std::swap((*this)[i], rhs[i]);
   }
   // Copy over the extra elts.
   if (this->getSize() > rhs.getSize()) {
      size_t eltDiff = this->getSize() - rhs.getSize();
      this->uninitializedCopy(this->begin() + numShared, this->end(), rhs.end());
      rhs.setSize(rhs.getSize() + eltDiff);
      this->destroyRange(this->begin() + numShared, this->end());
      this->setSize(numShared);
   } else if (rhs.getSize() > this->getSize()) {
      size_t eltDiff = rhs.getSize() - this->getSize();
      this->uninitializedCopy(rhs.begin() + numShared, rhs.end(), this->end());
      this->setSize(this->getSize() + eltDiff);
      this->destroyRange(rhs.begin() + numShared, rhs.end());
      rhs.setSize(numShared);
   }
}

template <typename T>
SmallVectorImpl<T> &SmallVectorImpl<T>::operator= (const SmallVectorImpl<T> &rhs)
{
   // Avoid self-assignment.
   if (this == &rhs) {
      return *this;
   }
   // If we already have sufficient space, assign the common elements, then
   // destroy any excess.
   size_t rhsSize = rhs.getSize();
   size_t curSize = this->getSize();
   if (curSize >= rhsSize) {
      // Assign common elements.
      iterator newEnd;
      if (rhsSize) {
         newEnd = std::copy(rhs.begin(), rhs.begin() + rhsSize, this->begin());
      } else {
         newEnd = this->begin();
      }
      // Destroy excess elements.
      this->destroyRange(newEnd, this->end());
      // Trim.
      this->setSize(rhsSize);
      return *this;
   }

   // If we have to grow to have enough elements, destroy the current elements.
   // This allows us to avoid copying them during the grow.
   // FIXME: don't do this if they're efficiently moveable.
   if (this->getCapacity() < rhsSize) {
      // Destroy current elements.
      this->destroyRange(this->begin(), this->end());
      this->setSize(0);
      curSize = 0;
      this->grow(rhsSize);
   } else if (curSize) {
      // Otherwise, use assignment for the already-constructed elements.
      std::copy(rhs.begin(), rhs.begin() + curSize, this->begin());
   }

   // Copy construct the new elements in place.
   this->uninitializedCopy(rhs.begin() + curSize, rhs.end(),
                           this->begin() + curSize);

   // Set end.
   this->setSize(rhsSize);
   return *this;
}

template <typename T>
SmallVectorImpl<T> &SmallVectorImpl<T>::operator=(SmallVectorImpl<T> &&rhs)
{
   // Avoid self-assignment.
   if (this == &rhs) {
      return *this;
   }
   // If the RHS isn't small, clear this vector and then steal its buffer.
   if (!rhs.isSmall()) {
      this->destroyRange(this->begin(), this->end());
      if (!this->isSmall()) {
         free(this->begin());
      }
      this->m_beginX = rhs.m_beginX;
      this->m_size = rhs.m_size;
      this->m_capacity = rhs.m_capacity;
      rhs.resetToSmall();
      return *this;
   }

   // If we already have sufficient space, assign the common elements, then
   // destroy any excess.
   size_t rhsSize = rhs.getSize();
   size_t curSize = this->getSize();
   if (curSize >= rhsSize) {
      // Assign common elements.
      iterator newEnd = this->begin();
      if (rhsSize) {
         newEnd = std::move(rhs.begin(), rhs.end(), newEnd);
      }
      // Destroy excess elements and trim the bounds.
      this->destroyRange(newEnd, this->end());
      this->setSize(rhsSize);
      // Clear the RHS.
      rhs.clear();
      return *this;
   }

   // If we have to grow to have enough elements, destroy the current elements.
   // This allows us to avoid copying them during the grow.
   // FIXME: this may not actually make any sense if we can efficiently move
   // elements.
   if (this->getCapacity() < rhsSize) {
      // Destroy current elements.
      this->destroyRange(this->begin(), this->end());
      this->setSize(0);
      curSize = 0;
      this->grow(rhsSize);
   } else if (curSize) {
      // Otherwise, use assignment for the already-constructed elements.
      std::move(rhs.begin(), rhs.begin() + curSize, this->begin());
   }

   // Move-construct the new elements in place.
   this->uninitializedMove(rhs.begin() + curSize, rhs.end(),
                           this->begin() + curSize);
   // Set end.
   this->setSize(rhsSize);
   rhs.clear();
   return *this;
}

/// Storage for the SmallVector elements which aren't contained in
/// SmallVectorTemplateCommon. There are 'N-1' elements here. The remaining '1'
/// element is in the base class. This is specialized for the N=1 and N=0 cases
/// to avoid allocating unnecessary storage.
template <typename T, unsigned N>
struct SmallVectorStorage
{
   polar::utils::AlignedCharArrayUnion<T> m_inlineElts[N];
};

/// We need the storage to be properly aligned even for small-size of 0 so that
/// the pointer math in \a SmallVectorTemplateCommon::getFirstEl() is
/// well-defined.
template <typename T>
struct alignas(alignof(T)) SmallVectorStorage<T, 0>
{};

/// This is a 'vector' (really, a variable-sized array), optimized
/// for the case when the array is small.  It contains some number of elements
/// in-place, which allows it to avoid heap allocation when the actual number of
/// elements is below that threshold.  This allows normal "small" cases to be
/// fast without losing generality for large inputs.
///
/// Note that this does not attempt to be exception safe.
///
template <typename T, unsigned N>
class SmallVector : public SmallVectorImpl<T>, SmallVectorStorage<T, N>
{

public:
   SmallVector() : SmallVectorImpl<T>(N)
   {}

   ~SmallVector()
   {
      // Destroy the constructed elements in the vector.
      this->destroyRange(this->begin(), this->end());
   }

   explicit SmallVector(size_t size, const T &value = T())
      : SmallVectorImpl<T>(N)
   {
      this->assign(size, value);
   }

   template <typename ItTy,
             typename = typename std::enable_if<std::is_convertible<
                                                   typename std::iterator_traits<ItTy>::iterator_category,
                                                   std::input_iterator_tag>::value>::type>
   SmallVector(ItTy start, ItTy end) : SmallVectorImpl<T>(N)
   {
      this->append(start, end);
   }

   template <typename RangeTy>
   explicit SmallVector(const IteratorRange<RangeTy> &range)
      : SmallVectorImpl<T>(N)
   {
      this->append(range.begin(), range.end());
   }

   SmallVector(std::initializer_list<T> elements) : SmallVectorImpl<T>(N)
   {
      this->assign(elements);
   }

   SmallVector(const SmallVector &rhs) : SmallVectorImpl<T>(N)
   {
      if (!rhs.empty()) {
         SmallVectorImpl<T>::operator=(rhs);
      }
   }

   const SmallVector &operator=(const SmallVector &rhs)
   {
      SmallVectorImpl<T>::operator=(rhs);
      return *this;
   }

   SmallVector(SmallVector &&rhs) : SmallVectorImpl<T>(N) {
      if (!rhs.empty()) {
         SmallVectorImpl<T>::operator=(::std::move(rhs));
      }
   }

   SmallVector(SmallVectorImpl<T> &&rhs) : SmallVectorImpl<T>(N)
   {
      if (!rhs.empty())
         SmallVectorImpl<T>::operator=(::std::move(rhs));
   }

   const SmallVector &operator=(SmallVector &&rhs)
   {
      SmallVectorImpl<T>::operator=(::std::move(rhs));
      return *this;
   }

   const SmallVector &operator=(SmallVectorImpl<T> &&rhs)
   {
      SmallVectorImpl<T>::operator=(::std::move(rhs));
      return *this;
   }

   const SmallVector &operator=(std::initializer_list<T> elements)
   {
      this->assign(elements);
      return *this;
   }
};

template <typename T, unsigned N>
inline size_t capacity_in_bytes(const SmallVector<T, N> &vector)
{
   return vector.getCapacityInBytes();
}

} // basic
} // polar

namespace std {

/// Implement std::swap in terms of SmallVector swap.
template<typename T>
inline void
swap(::polar::basic::SmallVectorImpl<T> &lhs, ::polar::basic::SmallVectorImpl<T> &rhs)
{
   lhs.swap(rhs);
}

/// Implement std::swap in terms of SmallVector swap.
template<typename T, unsigned N>
inline void
swap(::polar::basic::SmallVector<T, N> &lhs, ::polar::basic::SmallVector<T, N> &rhs)
{
   lhs.swap(rhs);
}

} // end namespace std

#endif // POLARPHP_BASIC_ADT_SMALL_VECTOR_H
