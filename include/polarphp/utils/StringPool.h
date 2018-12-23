// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/21.

//===----------------------------------------------------------------------===//
//
// This file declares an interned string pool, which helps reduce the cost of
// strings by using the same storage for identical strings.
//
// To intern a string:
//
//   StringPool Pool;
//   PooledStringPtr Str = Pool.intern("wakka wakka");
//
// To use the value of an interned string, use operator bool and operator*:
//
//   if (Str)
//     cerr << "the string is" << *Str << "\n";
//
// Pooled strings are immutable, but you can change a PooledStringPtr to point
// to another instance. So other interned strings can eventually be freed,
// strings in the string pool are reference-counted (automatically).
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_STRING_POOL_H
#define POLARPHP_UTILS_STRING_POOL_H

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringRef.h"
#include <cassert>

namespace polar {
namespace utils {

using polar::basic::StringMap;
using polar::basic::StringMapEntry;
using polar::basic::StringRef;

class PooledStringPtr;

/// StringPool - An interned string pool. Use the intern method to add a
/// string. Strings are removed automatically as PooledStringPtrs are
/// destroyed.
class StringPool
{
   /// PooledString - This is the value of an entry in the pool's interning
   /// table.
   struct PooledString
   {
      StringPool *m_pool = nullptr;  ///< So the string can remove itself.
      unsigned m_refCount = 0;       ///< Number of referencing PooledStringPtrs.

   public:
      PooledString() = default;
   };

   friend class PooledStringPtr;

   using TableType = StringMap<PooledString>;
   using EntryType = StringMapEntry<PooledString>;
   TableType m_internTable;

public:
   StringPool();
   ~StringPool();

   /// intern - Adds a string to the pool and returns a reference-counted
   /// pointer to it. No additional memory is allocated if the string already
   /// exists in the pool.
   PooledStringPtr intern(StringRef str);

   /// empty - Checks whether the pool is empty. Returns true if so.
   ///
   inline bool empty() const
   {
      return m_internTable.empty();
   }
};

/// PooledStringPtr - A pointer to an interned string. Use operator bool to
/// test whether the pointer is valid, and operator * to get the string if so.
/// This is a lightweight value class with storage requirements equivalent to
/// a single pointer, but it does have reference-counting overhead when
/// copied.
class PooledStringPtr
{
   using EntryType = StringPool::EntryType;

   EntryType *m_entryStr = nullptr;

public:
   PooledStringPtr() = default;

   explicit PooledStringPtr(EntryType *entry)
      : m_entryStr(entry)
   {
      if (m_entryStr) {
         ++m_entryStr->getValue().m_refCount;
      }
   }

   PooledStringPtr(const PooledStringPtr &other) : m_entryStr(other.m_entryStr)
   {
      if (m_entryStr) {
         ++m_entryStr->getValue().m_refCount;
      }
   }

   PooledStringPtr &operator=(const PooledStringPtr &other)
   {
      if (m_entryStr != other.m_entryStr) {
         clear();
         m_entryStr = other.m_entryStr;
         if (m_entryStr) {
            ++m_entryStr->getValue().m_refCount;
         }
      }
      return *this;
   }

   void clear()
   {
      if (!m_entryStr) {
         return;
      }
      if (--m_entryStr->getValue().m_refCount == 0) {
         m_entryStr->getValue().m_pool->m_internTable.remove(m_entryStr);
         m_entryStr->destroy();
      }
      m_entryStr = nullptr;
   }

   ~PooledStringPtr()
   {
      clear();
   }

   inline const char *begin() const
   {
      assert(*this && "Attempt to dereference empty PooledStringPtr!");
      return m_entryStr->getKeyData();
   }

   inline const char *end() const
   {
      assert(*this && "Attempt to dereference empty PooledStringPtr!");
      return m_entryStr->getKeyData() + m_entryStr->getKeyLength();
   }

   inline unsigned size() const
   {
      assert(*this && "Attempt to dereference empty PooledStringPtr!");
      return m_entryStr->getKeyLength();
   }

   inline const char *operator*() const
   {
      return begin();
   }

   inline explicit operator bool() const
   {
      return m_entryStr != nullptr;
   }

   inline bool operator==(const PooledStringPtr &other) const
   {
      return m_entryStr == other.m_entryStr;
   }

   inline bool operator!=(const PooledStringPtr &other) const
   {
      return m_entryStr != other.m_entryStr;
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_STRING_POOL_H
