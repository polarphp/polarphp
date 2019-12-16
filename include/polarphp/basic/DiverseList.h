//===--- DiverseList.h - List of variably-sized objects ---------*- C++ -*-===//
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
//
// This file defines a data structure for representing a list of
// variably-sized objects.  It is a requirement that the object type
// be trivially movable, meaning that it has a trivial move
// constructor and a trivial destructor.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_DIVERSE_LIST_H
#define POLARPHP_BASIC_DIVERSE_LIST_H

#include "polarphp/basic/Malloc.h"
#include <cassert>
#include <cstring>
#include <utility>

namespace polar {

template <typename T>
class DiverseListImpl;

/// DiverseList - A list of heterogeneously-typed objects.
///
/// \tparam T - A common base class of the objects in the list; must
///   provide an allocated_size() const method.
/// \tparam InlineCapacity - the amount of inline storage to provide, in bytes.
template <typename T, unsigned InlineCapacity>
class DiverseList : public DiverseListImpl<T>
{
   char m_inlineStorage[InlineCapacity];

public:
   DiverseList()
      : DiverseListImpl<T>(m_inlineStorage + InlineCapacity)
   {}

   DiverseList(const DiverseList &other)
      : DiverseListImpl<T>(other, m_inlineStorage + InlineCapacity)
   {}

   DiverseList(const DiverseListImpl<T> &other)
      : DiverseListImpl<T>(other, m_inlineStorage + InlineCapacity)
   {}

   DiverseList(DiverseList<T, InlineCapacity> &&other)
      : DiverseListImpl<T>(std::move(other), m_inlineStorage + InlineCapacity)
   {}

   DiverseList(DiverseListImpl<T> &&other)
      : DiverseListImpl<T>(std::move(other), m_inlineStorage + InlineCapacity)
   {}
};

/// A base class for DiverseListImpl.
class DiverseListBase
{
public:
   /// The first element in the list and the beginning of the allocation.
   char *begin;

   /// A pointer past the last element in the list.
   char *end;

   /// A pointer past the end of the allocation.
   char *endOfAllocation;

   bool isAllocatedInline() const
   {
      return (begin == reinterpret_cast<const char *>(this + 1));
   }

   void checkValid() const
   {
      assert(begin <= end);
      assert(end <= endOfAllocation);
   }

   void initialize(char *endOfAllocation)
   {
      this->endOfAllocation = endOfAllocation;
      begin = end = reinterpret_cast<char*>(this + 1);
   }

   void copyFrom(const DiverseListBase &other)
   {
      // Ensure that we're large enough to store all the data.
      std::size_t size = static_cast<std::size_t>(other.end - other.begin);
      char *newStorage = addNewStorage(size);
      std::memcpy(newStorage, other.begin, size);
   }

   char *addNewStorage(std::size_t needed)
   {
      checkValid();
      if (std::size_t(endOfAllocation - end) >= needed) {
         char *newStorage = end;
         end += needed;
         return newStorage;
      }
      return addNewStorageSlow(needed);
   }

   char *addNewStorageSlow(std::size_t needed);

   /// A stable iterator is the equivalent of an index into the list.
   /// It's an iterator that stays stable across modification of the
   /// list.
   class stable_iterator
   {
      std::size_t m_offset;
      friend class DiverseListBase;
      template <class T> friend class DiverseListImpl;
      stable_iterator(std::size_t offset)
         : m_offset(offset)
      {}
   public:
      stable_iterator() = default;
      friend bool operator==(stable_iterator lhs, stable_iterator rhs)
      {
         return lhs.m_offset == rhs.m_offset;
      }

      friend bool operator!=(stable_iterator lhs, stable_iterator rhs)
      {
         return !operator==(lhs, rhs);
      }
   };

   stable_iterator stable_begin() const
   {
      return stable_iterator(0);
   }

   stable_iterator stable_end()
   {
      return stable_iterator(std::size_t(end - begin));
   }

protected:
   ~DiverseListBase()
   {
      checkValid();
      if (!isAllocatedInline()) {
         delete[] begin;
      }
   }
};

/// An "abstract" base class for DiverseList<T> which does not
/// explicitly set the preferred inline capacity.  Most of the
/// implementation is in this class.
template <typename T>
class DiverseListImpl : private DiverseListBase
{
   DiverseListImpl(const DiverseListImpl<T> &other) = delete;
   DiverseListImpl(DiverseListImpl<T> &&other) = delete;

protected:
   DiverseListImpl(char *endOfAlloc)
   {
      initialize(endOfAlloc);
   }

   DiverseListImpl(const DiverseListImpl<T> &other, char *endOfAlloc) {
      initialize(endOfAlloc);
      copyFrom(other);
   }
   DiverseListImpl(DiverseListImpl<T> &&other, char *endOfAlloc)
   {
      // If the other is allocated inline, just initialize and copy.
      if (other.isAllocatedInline()) {
         initialize(endOfAlloc);
         copyFrom(other);
         return;
      }

      // Otherwise, steal its allocations.
      this->begin = other.begin;
      this->end = other.end;
      endOfAllocation = other.endOfAllocation;
      other.begin = other.end = other.endOfAllocation = reinterpret_cast<char*>((&other + 1));
      assert(other.isAllocatedInline());
   }

public:
   /// Query whether the stack is empty.
   bool empty() const
   {
      checkValid();
      return this->begin == this->end;
   }

   /// Return a reference to the first element in the list.
   T &front()
   {
      assert(!empty());
      return *reinterpret_cast<T*>(begin);
   }

   /// Return a reference to the first element in the list.
   const T &front() const
   {
      assert(!empty());
      return *reinterpret_cast<const T*>(begin);
   }

   class const_iterator;
   class iterator
   {
      char *m_ptr;
      friend class DiverseListImpl;
      friend class const_iterator;
      iterator(char *ptr)
         : m_ptr(ptr)
      {}

   public:
      iterator() = default;

      T &operator*() const
      {
         return *reinterpret_cast<T*>(m_ptr);
      }

      T *operator->() const
      {
         return reinterpret_cast<T*>(m_ptr);
      }

      iterator &operator++()
      {
         m_ptr += (*this)->allocated_size();
         return *this;
      }

      iterator operator++(int)
      {
         auto copy = *this;
         operator++();
         return copy;
      }

      /// advancePast - Like operator++, but asserting that the current
      /// object has a known type.
      template <typename U>
      void advancePast()
      {
         assert((*this)->allocated_size() == sizeof(U));
         m_ptr += sizeof(U);
      }

      friend bool operator==(iterator lhs, iterator rhs)
      {
         return lhs.m_ptr == rhs.m_ptr;
      }

      friend bool operator!=(iterator lhs, iterator rhs)
      {
         return !operator==(lhs, rhs);
      }
   };
   iterator begin()
   {
      checkValid();
      return iterator(begin);
   }

   iterator end()
   {
      checkValid();
      return iterator(end);
   }

   iterator find(stable_iterator iter)
   {
      checkValid();
      assert(iter.m_offset <= this->end - this->begin);
      return iterator(this->begin + iter.m_offset);
   }

   stable_iterator stabilize(iterator iter) const
   {
      checkValid();
      assert(this->begin <= iter.m_ptr && iter.m_ptr <= this->end);
      return stable_iterator(iter.m_ptr - this->begin);
   }

   class const_iterator
   {
      const char *m_ptr;
      friend class DiverseListImpl;
      const_iterator(const char *ptr)
         : m_ptr(ptr)
      {}
   public:
      const_iterator() = default;
      const_iterator(iterator iter)
         : m_ptr(iter.m_ptr) {}

      const T &operator*() const
      {
         return *reinterpret_cast<const T*>(m_ptr);
      }

      const T *operator->() const
      {
         return reinterpret_cast<const T*>(m_ptr);
      }

      const_iterator &operator++()
      {
         m_ptr += (*this)->allocated_size();
         return *this;
      }

      const_iterator operator++(int)
      {
         auto copy = *this;
         operator++();
         return copy;
      }

      /// advancePast - Like operator++, but asserting that the current
      /// object has a known type.
      template <typename U>
      void advancePast()
      {
         assert((*this)->allocated_size() == sizeof(U));
         m_ptr += sizeof(U);
      }

      friend bool operator==(const_iterator lhs, const_iterator rhs)
      {
         return lhs.m_ptr == rhs.m_ptr;
      }

      friend bool operator!=(const_iterator lhs, const_iterator rhs)
      {
         return !operator==(lhs, rhs);
      }
   };

   const_iterator begin() const
   {
      checkValid();
      return const_iterator(begin);
   }

   const_iterator end() const
   {
      checkValid();
      return const_iterator(end);
   }

   const_iterator find(stable_iterator iter) const
   {
      checkValid();
      assert(iter.m_offset <= this->end - this->begin);
      return const_iterator(this->begin + iter.m_offset);
   }

   stable_iterator stabilize(const_iterator iter) const
   {
      checkValid();
      assert(this->begin <= iter.m_ptr && iter.m_ptr <= this->end);
      return stable_iterator(iter.m_ptr - this->begin);
   }

   /// Add a new object onto the end of the list.
   template <typename U, typename... A>
   U &add(A && ...args)
   {
      char *storage = addNewStorage(sizeof(U));
      U &newObject = *::new (storage) U(::std::forward<A>(args)...);
      return newObject;
   }

   /// Add a new object onto the end of the list with some extra storage.
   template <typename U, typename... A>
   U &addWithExtra(size_t extra, A && ...args)
   {
      char *storage = addNewStorage(sizeof(U) + extra);
      U &newObject = *::new (storage) U(::std::forward<A>(args)...);
      return newObject;
   }
};

} // polar

#endif // POLARPHP_BASIC_DIVERSE_LIST_H
