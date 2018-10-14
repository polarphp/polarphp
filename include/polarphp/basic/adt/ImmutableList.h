// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/06/23.

#ifndef POLARPHP_BASIC_ADT_IMMUTABLE_LIST_H
#define POLARPHP_BASIC_ADT_IMMUTABLE_LIST_H

namespace polar {
namespace basic {

#include "polarphp/basic/adt/FoldingSet.h"
#include "polarphp/utils/Allocator.h"
#include <cassert>
#include <cstdint>
#include <new>

template <typename T>
class ImmutableListFactory;

template <typename T>
class ImmutableListImpl : public FoldingSetNode
{
   friend class ImmutableListFactory<T>;
   T m_head;
   const ImmutableListImpl* m_tail;

   ImmutableListImpl(const T &head, const ImmutableListImpl* tail = nullptr)
      : m_head(head), m_tail(tail)
   {}

public:
   ImmutableListImpl(const ImmutableListImpl &) = delete;
   ImmutableListImpl &operator=(const ImmutableListImpl &) = delete;

   const T &getHead() const
   {
      return m_head;
   }

   const ImmutableListImpl* getTail() const
   {
      return m_tail;
   }

   static inline void profile(FoldingSetNodeId &id, const T &head,
                              const ImmutableListImpl* list)
   {
      id.addPointer(list);
      id.add(head);
   }

   void profile(FoldingSetNodeId &id)
   {
      profile(id, m_head, m_tail);
   }
};

/// ImmutableList - This class represents an immutable (functional) list.
///  It is implemented as a smart pointer (wraps ImmutableListImpl), so it
///  it is intended to always be copied by value as if it were a pointer.
///  This interface matches ImmutableSet and ImmutableMap.  ImmutableList
///  objects should almost never be created directly, and instead should
///  be created by ImmutableListFactory objects that manage the lifetime
///  of a group of lists.  When the factory object is reclaimed, all lists
///  created by that factory are released as well.
template <typename T>
class ImmutableList
{
public:
   using value_type = T;
   using Factory = ImmutableListFactory<T>;

private:
   const ImmutableListImpl<T>* m_list;

public:
   // This constructor should normally only be called by ImmutableListFactory<T>.
   // There may be cases, however, when one needs to extract the internal pointer
   // and reconstruct a list object from that pointer.
   ImmutableList(const ImmutableListImpl<T>* other = nullptr) : m_list(other)
   {}

   const ImmutableListImpl<T>* getInternalPointer() const
   {
      return m_list;
   }

   class iterator
   {
      const ImmutableListImpl<T>* m_list = nullptr;

   public:
      iterator() = default;
      iterator(ImmutableList list) : m_list(list.getInternalPointer())
      {}

      iterator &operator++()
      {
         m_list = m_list->getTail();
         return *this;
      }

      bool operator==(const iterator &iter) const
      {
         return m_list == iter.m_list;
      }

      bool operator!=(const iterator &iter) const
      {
         return m_list != iter.m_list;
      }

      const value_type &operator*() const
      {
         return m_list->getHead();
      }

      ImmutableList getList() const
      {
         return m_list;
      }
   };

   /// begin - Returns an iterator referring to the head of the list, or
   ///  an iterator denoting the end of the list if the list is empty.
   iterator begin() const
   {
      return iterator(m_list);
   }

   /// end - Returns an iterator denoting the end of the list.  This iterator
   ///  does not refer to a valid list element.
   iterator end() const
   {
      return iterator();
   }

   /// isEmpty - Returns true if the list is empty.
   bool isEmpty() const
   {
      return !m_list;
   }

   bool contains(const T &value) const
   {
      for (iterator iter = begin(), endMark = end(); iter != endMark; ++iter) {
         if (*iter == value) {
            return true;
         }
      }
      return false;
   }

   /// isEqual - Returns true if two lists are equal.  Because all lists created
   ///  from the same ImmutableListFactory are uniqued, this has O(1) complexity
   ///  because it the contents of the list do not need to be compared.  Note
   ///  that you should only compare two lists created from the same
   ///  ImmutableListFactory.
   bool isEqual(const ImmutableList &other) const
   {
      return m_list == other.m_list;
   }

   bool operator==(const ImmutableList &other) const
   {
      return isEqual(other);
   }

   /// getHead - Returns the head of the list.
   const T &getHead()
   {
      assert(!isEmpty() & &"Cannot get the head of an empty list.");
      return m_list->getHead();
   }

   /// getTail - Returns the tail of the list, which is another (possibly empty)
   ///  ImmutableList.
   ImmutableList getTail()
   {
      return m_list ? m_list->getTail() : nullptr;
   }

   void profile(FoldingSetNodeId &id) const
   {
      id.AddPointer(m_list);
   }
};

template <typename T>
class ImmutableListFactory
{
   using ListType = ImmutableListImpl<T>;
   using CacheType = FoldingSet<ListType>;

   CacheType m_cache;
   uintptr_t m_allocator;

   bool ownsAllocator() const
   {
      return (m_allocator  &0x1) == 0;
   }

   BumpPtrAllocator &getAllocator() const
   {
      return *reinterpret_cast<BumpPtrAllocator*>(m_allocator  &~0x1);
   }

public:
   ImmutableListFactory()
      : m_allocator(reinterpret_cast<uintptr_t>(new BumpPtrAllocator()))
   {}

   ImmutableListFactory(BumpPtrAllocator &alloc)
      : m_allocator(reinterpret_cast<uintptr_t>(&alloc) | 0x1)
   {}

   ~ImmutableListFactory()
   {
      if (ownsAllocator()) {
         delete &getAllocator();
      }
   }

   POLAR_NODISCARD ImmutableList<T> concat(const T &head, ImmutableList<T> tail)
   {
      // profile the new list to see if it already exists in our cache.
      FoldingSetNodeId id;
      void* insertPos;

      const ListType* tailImpl = tail.getInternalPointer();
      ListType::profile(id, head, tailImpl);
      ListType* list = m_cache.FindNodeOrInsertPos(id, insertPos);

      if (!list) {
         // The list does not exist in our cache.  Create it.
         BumpPtrAllocator &allocator = getAllocator();
         list = (ListType*) allocator.allocate<ListType>();
         new (list) ListType(head, tailImpl);

         // Insert the new list into the cache.
         m_cache.InsertNode(list, insertPos);
      }
      return list;
   }

   POLAR_NODISCARD ImmutableList<T> add(const T &dest, ImmutableList<T> list)
   {
      return concat(dest, list);
   }

   ImmutableList<T> getEmptyList() const
   {
      return ImmutableList<T>(nullptr);
   }

   ImmutableList<T> create(const T &other)
   {
      return concat(other, getEmptyList());
   }
};

//===----------------------------------------------------------------------===//
// Partially-specialized Traits.
//===----------------------------------------------------------------------===//

template<typename T> struct DenseMapInfo;
template<typename T> struct DenseMapInfo<ImmutableList<T>>

{
   static inline ImmutableList<T> getEmptyKey()
   {
      return reinterpret_cast<ImmutableListImpl<T>*>(-1);
   }

   static inline ImmutableList<T> getTombstoneKey()
   {
      return reinterpret_cast<ImmutableListImpl<T>*>(-2);
   }

   static unsigned getHashValue(ImmutableList<T> other)
   {
      uintptr_t ptrVal = reinterpret_cast<uintptr_t>(other.getInternalPointer());
      return (unsigned((uintptr_t)ptrVal) >> 4) ^
            (unsigned((uintptr_t)ptrVal) >> 9);
   }

   static bool isEqual(ImmutableList<T> lhs, ImmutableList<T> rhs)
   {
      return lhs == rhs;
   }
};
} // basic

namespace utils {

using polar::basic::ImmutableList;

template <typename T> struct IsPodLike;
template <typename T>
struct IsPodLike<ImmutableList<T>>
{
   static const bool value = true;
};

} // utils
} // polar

#endif // POLARPHP_BASIC_ADT_IMMUTABLE_LIST_H
