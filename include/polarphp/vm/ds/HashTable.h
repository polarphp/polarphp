// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/29.

#ifndef POLARPHP_VMAPI_DS_HASHTABLE_H
#define POLARPHP_VMAPI_DS_HASHTABLE_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/basic/adt/StringRef.h"

#include <string>
#include <functional>

namespace polar {
namespace vmapi {

using polar::basic::StringRef;

extern HashTableDataDeleter sg_zValDataDeleter;

class VMAPI_DECL_EXPORT HashTable
{
public:
   constexpr static uint32_t DEFAULT_HASH_SIZE = 8;
   using IndexType = zend_ulong;
   using HashPosition = ::HashPosition;
   using DefaultForeachVisitor = std::function<void(const Variant&, const Variant&)>;
   enum class HashKeyType : unsigned char
   {
      String      = HASH_KEY_IS_STRING,
      Long        = HASH_KEY_IS_LONG,
      NotExistent = HASH_KEY_NON_EXISTENT
   };

   enum class HashActionType : unsigned char
   {
      Update         = HASH_UPDATE,
      Add            = HASH_ADD,
      UpdateIndirect = HASH_UPDATE_INDIRECT,
      AddNew         = HASH_ADD_NEW,
      AddNext        = HASH_ADD_NEXT
   };

   enum class HashFlagType : unsigned char
   {
      consistency       = HASH_FLAG_CONSISTENCY,
      Packed            = HASH_FLAG_PACKED,
      Initialized       = HASH_FLAG_INITIALIZED,
      StaticKeys        = HASH_FLAG_STATIC_KEYS,
      HasEmptyIndirect  = HASH_FLAG_HAS_EMPTY_IND,
      AllowCowViolation = HASH_FLAG_ALLOW_COW_VIOLATION
   };

public:
   HashTable(uint32_t tableSize = DEFAULT_HASH_SIZE,
             HashTableDataDeleter defaultDeleter = sg_zValDataDeleter,
             bool persistent = false)
   {
      zend_hash_init(&m_hashTable, tableSize, nullptr, defaultDeleter, persistent ? 1 : 0);
   }

   HashTable(uint32_t tableSize, bool persistent = false)
      : HashTable(tableSize, nullptr, persistent)
   {}

   HashTable(const ::HashTable &hashTable)
   {
      m_hashTable = hashTable;
      GC_ADDREF(&m_hashTable);
   }

   virtual ~HashTable();

   uint32_t getSize() const
   {
      return zend_hash_num_elements(&m_hashTable);
   }

   uint32_t count() const
   {
      return zend_hash_num_elements(&m_hashTable);
   }

   bool isEmpty() const
   {
      return 0 != getSize();
   }

   HashTable &insert(StringRef key, const Variant &value, bool forceNew = false);
   HashTable &insert(vmapi_ulong index, const Variant &value, bool forceNew = false);
   HashTable &insert(StringRef key, Variant &&value, bool forceNew = false);
   HashTable &insert(vmapi_ulong index, Variant &&value, bool forceNew = false);

   HashTable &append(const Variant &value, bool forceNew = true);
   HashTable &append(Variant &&value, bool forceNew = true);

   Variant update(StringRef key, const Variant &value)
   {
      std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
      return zend_hash_update(&m_hashTable, zkey.get(), value);
   }

   Variant update(vmapi_ulong index, const Variant &value)
   {
      return zend_hash_index_update(&m_hashTable, index, value);
   }

   bool remove(StringRef key)
   {
      std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
      return zend_hash_del(&m_hashTable, zkey.get()) == VMAPI_SUCCESS ? true : false;
   }

   bool remove(int16_t index)
   {
      return remove(static_cast<vmapi_ulong>(index < 0 ? 0 : index));
   }

   bool remove(int32_t index)
   {
      return remove(static_cast<vmapi_ulong>(index < 0 ? 0 : index));
   }

   bool remove(uint16_t index)
   {
      return remove(static_cast<vmapi_ulong>(index));
   }

   bool remove(uint32_t index)
   {
      return remove(static_cast<vmapi_ulong>(index));
   }

   bool remove(vmapi_ulong index)
   {
      return zend_hash_index_del(&m_hashTable, index) == VMAPI_SUCCESS ? true : false;
   }

   Variant getValue(StringRef key) const
   {
      std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
      return zend_hash_find(&m_hashTable, zkey.get());
   }

   Variant getValue(vmapi_ulong index) const
   {
      return zend_hash_index_find(&m_hashTable, index);
   }

   Variant getValue(int16_t index, const Variant &defaultValue) const
   {
      return getValue(static_cast<vmapi_ulong>(index < 0 ? 0 : index), defaultValue);
   }

   Variant getValue(int32_t index, const Variant &defaultValue) const
   {
      return getValue(static_cast<vmapi_ulong>(index < 0 ? 0 : index), defaultValue);
   }

   Variant getValue(uint16_t index, const Variant &defaultValue) const
   {
      return getValue(static_cast<vmapi_ulong>(index), defaultValue);
   }

   Variant getValue(uint32_t index, const Variant &defaultValue) const
   {
      return getValue(static_cast<vmapi_ulong>(index), defaultValue);
   }

   Variant getValue(vmapi_ulong index, const Variant &defaultValue) const;
   Variant getValue(StringRef key, const Variant &defaultValue) const;

   Variant getKey() const;
   Variant getKey(const Variant &value) const;
   Variant getKey(const Variant &value, int16_t defaultKey) const
   {
      return getKey(value, Variant(defaultKey));
   }

   Variant getKey(const Variant &value, int32_t defaultKey) const
   {
      return getKey(value, Variant(defaultKey));
   }

   Variant getKey(const Variant &value, uint16_t defaultKey) const
   {
      return getKey(value, Variant(static_cast<vmapi_long>(defaultKey)));
   }

   Variant getKey(const Variant &value, uint32_t defaultKey) const
   {
      return getKey(value, Variant(static_cast<vmapi_long>(defaultKey)));
   }

   Variant getKey(const Variant &value, vmapi_ulong defaultKey) const
   {
      return getKey(value, Variant(static_cast<vmapi_long>(defaultKey)));
   }

   Variant getKey(const Variant &value, const std::string &defaultKey) const
   {
      return getKey(value, Variant(defaultKey));
   }

   Variant getKey(const Variant &value, const char *defaultKey) const
   {
      return getKey(value, Variant(defaultKey));
   }

   Variant getKey(const Variant &value, const Variant &key) const;

   HashTable &clear()
   {
      zend_hash_clean(&m_hashTable);
      return *this;
   }

   bool contains(StringRef key)
   {
      std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
      return zend_hash_exists(&m_hashTable, zkey.get()) == 1 ? true : false;
   }

   bool contains(int16_t index)
   {
      return contains(static_cast<vmapi_ulong>(index < 0 ? 0 : index));
   }

   bool contains(int32_t index)
   {
      return contains(static_cast<vmapi_ulong>(index < 0 ? 0 : index));
   }

   bool contains(uint16_t index)
   {
      return contains(static_cast<vmapi_ulong>(index));
   }

   bool contains(uint32_t index)
   {
      return contains(static_cast<vmapi_ulong>(index));
   }

   bool contains(const vmapi_ulong index)
   {
      return zend_hash_index_exists(&m_hashTable, index) == 1 ? true : false;
   }

   std::vector<Variant> getKeys() const;
   std::vector<Variant> getKeys(const Variant &value) const;
   std::vector<Variant> getValues() const;

public:
   Variant operator [](int16_t index)
   {
      return operator [](static_cast<vmapi_ulong>(index < 0 ? 0 : index));
   }

   Variant operator [](int32_t index)
   {
      return operator [](static_cast<vmapi_ulong>(index < 0 ? 0 : index));
   }

   Variant operator [](uint16_t index)
   {
      return operator [](static_cast<vmapi_ulong>(index));
   }

   Variant operator [](uint32_t index)
   {
      return operator [](static_cast<vmapi_ulong>(index));
   }

   Variant operator [](vmapi_ulong index);
   Variant operator [](StringRef key);
public:
   class iterator
   {
   public:
      typedef std::bidirectional_iterator_tag iterator_category;
      typedef std::ptrdiff_t difference_type;
      typedef Variant value_type;
      typedef Variant *pointer;
      typedef Variant &reference;
   public:
      iterator()
         : m_index(HT_INVALID_IDX),
           m_hashTable(nullptr)
      {}

      explicit iterator(::HashTable *hashTable, HashPosition index)
         : m_index(index),
           m_hashTable(hashTable)
      {}

      std::string getStrKey() const;
      IndexType getNumericKey() const;

      iterator &reset()
      {
         zend_hash_internal_pointer_reset_ex(m_hashTable, &m_index);
         return *this;
      }

      Variant getKey() const;

      HashKeyType getKeyType() const
      {
         return static_cast<HashKeyType>(zend_hash_get_current_key_type_ex(const_cast<::HashTable *>(m_hashTable),
                                                                           const_cast<HashPosition *>(&m_index)));
      }

      virtual Variant getValue() const
      {
         return Variant(zend_hash_get_current_data_ex(const_cast<::HashTable *>(m_hashTable),
                                                      const_cast<HashPosition *>(&m_index)), true);
      }

   public:
      bool operator==(const iterator &other) const
      {
         return m_index == other.m_index;
      }

      bool operator!=(const iterator &other) const
      {
         return m_index != other.m_index;
      }

      iterator &operator ++();
      iterator operator ++(int);
      iterator &operator --();
      iterator operator --(int);
      iterator operator+(int32_t step);

      iterator operator-(int32_t step)
      {
         return operator+(-step);
      }

      iterator &operator+=(int step)
      {
         return *this = *this + step;
      }

      iterator &operator-=(int step) {
         return *this = *this - step;
      }

      Variant operator*()
      {
         return getValue();
      }

      virtual ~iterator();

   protected:
      /**
       * @brief current hash table index
       */
      HashPosition m_index;
      ::HashTable *m_hashTable;
   };

   class const_iterator : public iterator
   {
   public:
      using iterator::iterator;
      virtual Variant getValue() const override
      {
         return Variant(zend_hash_get_current_data_ex(const_cast<::HashTable *>(m_hashTable),
                                                      const_cast<HashPosition *>(&m_index)));
      }
      const_iterator &operator ++()
      {
         iterator::operator ++();
         return *this;
      }

      const_iterator operator ++(int)
      {
         const_iterator iter = *this;
         iterator::operator++();
         return iter;
      }

      const_iterator &operator --()
      {
         iterator::operator--();
         return *this;
      }

      const_iterator operator --(int)
      {
         const_iterator iter = *this;
         iterator::operator--();
         return iter;
      }

      const_iterator operator +(int32_t step)
      {
         iterator::operator+(step);
         return *this;
      }

      const_iterator operator -(int32_t step)
      {
         return operator+(-step);
      }

      const_iterator &operator +=(int step)
      {
         return *this = *this + step;
      }

      const_iterator &operator -=(int step) {
         return *this = *this - step;
      }

   };

   class key_iterator
   {
   public:
      typedef typename const_iterator::iterator_category iterator_category;
      typedef typename const_iterator::difference_type difference_type;
      typedef Variant value_type;
      typedef Variant *pointer;
      typedef Variant &reference;
      explicit key_iterator(const_iterator iter) : iter(iter)
      {}

      Variant operator*() const
      {
         return iter.getKey();
      }

      bool operator ==(key_iterator other) const
      {
         return iter == other.iter;
      }

      bool operator !=(key_iterator other) const
      {
         return iter != other.iter;
      }

      key_iterator &operator++()
      {
         ++iter;
         return *this;
      }

      key_iterator operator++(int)
      {
         return key_iterator(iter++);
      }

      key_iterator &operator--()
      {
         --iter;
         return *this;
      }

      key_iterator operator--(int)
      {
         return key_iterator(iter--);
      }

      const_iterator base() const
      {
         return iter;
      }
   private:
      const_iterator iter;
   };

   // STL style
   iterator begin() const
   {
      return iterator(const_cast<::HashTable *>(&m_hashTable), m_hashTable.nInternalPointer);
   }

   const_iterator cbegin() const
   {
      return const_iterator(const_cast<::HashTable *>(&m_hashTable), m_hashTable.nInternalPointer);
   }

   iterator end() const
   {
      return iterator(const_cast<::HashTable *>(&m_hashTable), HT_INVALID_IDX);
   }

   const_iterator cend() const
   {
      return const_iterator(const_cast<::HashTable *>(&m_hashTable), HT_INVALID_IDX);
   }

   key_iterator keyBegin() const
   {
      return key_iterator(cbegin());
   }

   key_iterator keyEnd() const
   {
      return key_iterator(cend());
   }

   void each(DefaultForeachVisitor visitor) const;
   void reverseEach(DefaultForeachVisitor visitor) const;

protected:
   std::shared_ptr<zend_string> initZStrFromStringRef(StringRef str) const;

protected:
   ::HashTable m_hashTable;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_HASHTABLE_H
