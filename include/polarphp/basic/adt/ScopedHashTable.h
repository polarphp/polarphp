// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/26.

//===----------------------------------------------------------------------===//
//
// This file implements an efficient scoped hash table, which is useful for
// things like dominator-based optimizations.  This allows clients to do things
// like this:
//
//  ScopedHashTable<int, int> HT;
//  {
//    ScopedHashTableScope<int, int> Scope1(HT);
//    HT.insert(0, 0);
//    HT.insert(1, 1);
//    {
//      ScopedHashTableScope<int, int> Scope2(HT);
//      HT.insert(0, 42);
//    }
//  }
//
// Looking up the value for "0" in the Scope2 block will return 42.  Looking
// up the value for 0 before 42 is inserted or after Scope2 is popped will
// return 0.
//
//===----------------------------------------------------------------------===//

#ifndef POLAR_BASIC_ADT_SCOPED_HASH_TABLE_H
#define POLAR_BASIC_ADT_SCOPED_HASH_TABLE_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/DenseMapInfo.h"
#include "polarphp/utils/Allocator.h"
#include <cassert>
#include <new>

namespace polar {
namespace basic {

template <typename K, typename V, typename KInfo = DenseMapInfo<K>,
          typename AllocatorTy = MallocAllocator>
class ScopedHashTable;

template <typename K, typename V>
class ScopedHashTableValue
{
   ScopedHashTableValue *m_nextInScope;
   ScopedHashTableValue *m_nextForKey;
   K m_key;
   V m_value;

   ScopedHashTableValue(const K &key, const V &value) : m_key(key), m_value(value)
   {}

public:
   const K &getKey() const
   {
      return m_key;
   }

   const V &getValue() const
   {
      return m_value;
   }

   V &getValue()
   {
      return m_value;
   }

   ScopedHashTableValue *getNextForKey()
   {
      return m_nextForKey;
   }

   const ScopedHashTableValue *getNextForKey() const
   {
      return m_nextForKey;
   }

   ScopedHashTableValue *getNextInScope()
   {
      return m_nextInScope;
   }

   template <typename AllocatorTy>
   static ScopedHashTableValue *create(ScopedHashTableValue *nextInScope,
                                       ScopedHashTableValue *nextForKey,
                                       const K &key, const V &value,
                                       AllocatorTy &allocator)
   {
      ScopedHashTableValue *newTable = allocator.template allocator<ScopedHashTableValue>();
      // Set up the value.
      new (newTable) ScopedHashTableValue(key, value);
      newTable->m_nextInScope = nextInScope;
      newTable->m_nextForKey = nextForKey;
      return newTable;
   }

   template <typename AllocatorTy>
   void destroy(AllocatorTy &allocator)
   {
      // Free memory referenced by the item.
      this->~ScopedHashTableValue();
      allocator.deallocate(this);
   }
};

template <typename K, typename V, typename KInfo = DenseMapInfo<K>,
          typename AllocatorTy = MallocAllocator>
class ScopedHashTableScope
{
   /// HT - The hashtable that we are active for.
   ScopedHashTable<K, V, KInfo, AllocatorTy> &m_hashTable;

   /// PrevScope - This is the scope that we are shadowing in m_hashTable
   ScopedHashTableScope *m_prevScope;

   /// LastValInScope - This is the last value that was inserted for this scope
   /// or null if none have been inserted yet.
   ScopedHashTableValue<K, V> *m_lastValInScope;

public:
   ScopedHashTableScope(ScopedHashTable<K, V, KInfo, AllocatorTy> &hashTable);
   ScopedHashTableScope(ScopedHashTableScope &) = delete;
   ScopedHashTableScope &operator=(ScopedHashTableScope &) = delete;
   ~ScopedHashTableScope();

   ScopedHashTableScope *getParentScope()
   {
      return m_prevScope;
   }

   const ScopedHashTableScope *getParentScope() const
   {
      return m_prevScope;
   }

private:
   friend class ScopedHashTable<K, V, KInfo, AllocatorTy>;

   ScopedHashTableValue<K, V> *getLastValInScope()
   {
      return m_lastValInScope;
   }

   void setLastValInScope(ScopedHashTableValue<K, V> *value)
   {
      m_lastValInScope = value;
   }
};

template <typename K, typename V, typename KInfo = DenseMapInfo<K>>
class ScopedHashTableIterator
{
   ScopedHashTableValue<K, V> *m_node;

public:
   ScopedHashTableIterator(ScopedHashTableValue<K, V> *node) : m_node(node)
   {}

   V &operator*() const
   {
      assert(m_node && "Dereference end()");
      return m_node->getValue();
   }

   V *operator->() const
   {
      return &m_node->getValue();
   }

   bool operator==(const ScopedHashTableIterator &other) const
   {
      return m_node == other.m_node;
   }

   bool operator!=(const ScopedHashTableIterator &other) const
   {
      return m_node != other.m_node;
   }

   inline ScopedHashTableIterator& operator++()
   {          // Preincrement
      assert(m_node && "incrementing past end()");
      m_node = m_node->getNextForKey();
      return *this;
   }

   ScopedHashTableIterator operator++(int)
   {        // Postincrement
      ScopedHashTableIterator temp = *this;
      ++*this;
      return temp;
   }
};

template <typename K, typename V, typename KInfo, typename AllocatorTy>
class ScopedHashTable
{
public:
   /// ScopeType - This is a helpful typedef that allows clients to get easy access
   /// to the name of the scope for this hash table.
   using ScopeType = ScopedHashTableScope<K, V, KInfo, AllocatorTy>;
   using size_type = unsigned;

private:
   friend class ScopedHashTableScope<K, V, KInfo, AllocatorTy>;

   using ValTy = ScopedHashTableValue<K, V>;

   DenseMap<K, ValTy*, KInfo> m_topLevelMap;
   ScopeType *m_curScope = nullptr;

   AllocatorTy m_allocator;

public:
   ScopedHashTable() = default;
   ScopedHashTable(AllocatorTy allocator) : m_allocator(allocator)
   {}
   ScopedHashTable(const ScopedHashTable &) = delete;
   ScopedHashTable &operator=(const ScopedHashTable &) = delete;

   ~ScopedHashTable()
   {
      assert(!m_curScope && m_topLevelMap.empty() && "Scope imbalance!");
   }

   /// Access to the allocator.
   AllocatorTy &getAllocator()
   {
      return m_allocator;
   }

   const AllocatorTy &getAllocator() const
   {
      return m_allocator;
   }

   /// Return 1 if the specified key is in the table, 0 otherwise.
   size_type count(const K &key) const
   {
      return m_topLevelMap.count(key);
   }

   V lookup(const K &key) const
   {
      auto iter = m_topLevelMap.find(key);
      if (iter != m_topLevelMap.end()) {
         return iter->second->getValue();
      }
      return V();
   }

   void insert(const K &key, const V &value)
   {
      insertIntoScope(m_curScope, key, value);
   }

   using iterator = ScopedHashTableIterator<K, V, KInfo>;

   iterator end()
   {
      return iterator(0);
   }

   iterator begin(const K &key)
   {
      typename DenseMap<K, ValTy*, KInfo>::iterator iter =
            m_topLevelMap.find(key);
      if (iter == m_topLevelMap.end()) {
         return end();
      }
      return iterator(iter->second);
   }

   ScopeType *getCurScope()
   {
      return m_curScope;
   }

   const ScopeType *getCurScope() const
   {
      return m_curScope;
   }

   /// insertIntoScope - This inserts the specified key/value at the specified
   /// (possibly not the current) scope.  While it is ok to insert into a scope
   /// that isn't the current one, it isn't ok to insert *underneath* an existing
   /// value of the specified key.
   void insertIntoScope(ScopeType *table, const K &key, const V &value)
   {
      assert(table && "No scope active!");
      ScopedHashTableValue<K, V> *&keyEntry = m_topLevelMap[key];
      keyEntry = ValTy::create(table->getLastValInScope(), keyEntry, key, value,
                               m_allocator);
      table->setLastValInScope(keyEntry);
   }
};

/// ScopedHashTableScope ctor - Install this as the current scope for the hash
/// table.
template <typename K, typename V, typename KInfo, typename Allocator>
ScopedHashTableScope<K, V, KInfo, Allocator>::
ScopedHashTableScope(ScopedHashTable<K, V, KInfo, Allocator> &hashTable) : m_hashTable(hashTable)
{
   m_prevScope = m_hashTable.m_curScope;
   m_hashTable.m_curScope = this;
   m_lastValInScope = nullptr;
}

template <typename K, typename V, typename KInfo, typename Allocator>
ScopedHashTableScope<K, V, KInfo, Allocator>::~ScopedHashTableScope()
{
   assert(m_hashTable.m_curScope == this && "Scope imbalance!");
   m_hashTable.m_curScope = m_prevScope;
   // Pop and delete all values corresponding to this scope.
   while (ScopedHashTableValue<K, V> *thisEntry = m_lastValInScope) {
      // Pop this value out of the m_topLevelMap.
      if (!thisEntry->getNextForKey()) {
         assert(m_hashTable.m_topLevelMap[thisEntry->getKey()] == thisEntry &&
               "Scope imbalance!");
         m_hashTable.m_topLevelMap.erase(thisEntry->getKey());
      } else {
         ScopedHashTableValue<K, V> *&keyEntry = m_hashTable.m_topLevelMap[thisEntry->getKey()];
         assert(keyEntry == thisEntry && "Scope imbalance!");
         keyEntry = thisEntry->getNextForKey();
      }

      // Pop this value out of the scope.
      m_lastValInScope = thisEntry->getNextInScope();

      // Delete this entry.
      thisEntry->destroy(m_hashTable.getAllocator());
   }
}

} // basic
} // polar

#endif // POLAR_BASIC_ADT_SCOPED_HASH_TABLE_H
