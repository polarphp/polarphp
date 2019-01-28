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

#include "polarphp/vm/ds/HashTable.h"

namespace polar {
namespace vmapi {

HashTableDataDeleter sg_zValDataDeleter = ZVAL_PTR_DTOR;

HashTable &HashTable::insert(StringRef key, const Variant &value, bool forceNew)
{
   zval val;
   ZVAL_DUP(&val, value.getZvalPtr());
   std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
   if (forceNew) {
      zend_hash_add_new(&m_hashTable, zkey.get(), &val);
   } else {
      zend_hash_add(&m_hashTable, zkey.get(), &val);
   }
   return *this;
}

HashTable &HashTable::insert(vmapi_ulong index, const Variant &value, bool forceNew)
{
   zval val;
   ZVAL_DUP(&val, value.getZvalPtr());
   if (forceNew) {
      zend_hash_index_add_new(&m_hashTable, index, &val);
   } else {
      zend_hash_index_add(&m_hashTable, index, &val);
   }
   return *this;
}

HashTable &HashTable::insert(StringRef key, Variant &&value, bool forceNew)
{
   zval val = value.detach(true);
   std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
   if (forceNew) {
      zend_hash_add_new(&m_hashTable, zkey.get(), &val);
   } else {
      zend_hash_add(&m_hashTable, zkey.get(), &val);
   }
   return *this;
}

HashTable &HashTable::insert(vmapi_ulong index, Variant &&value, bool forceNew)
{
   zval val = value.detach(true);
   if (forceNew) {
      zend_hash_index_add_new(&m_hashTable, index, &val);
   } else {
      zend_hash_index_add(&m_hashTable, index, &val);
   }
   return *this;
}

HashTable &HashTable::append(const Variant &value, bool forceNew)
{
   zval val;
   ZVAL_DUP(&val, value.getZvalPtr());
   if (forceNew) {
      zend_hash_next_index_insert_new(&m_hashTable, &val);
   } else {
      zend_hash_next_index_insert(&m_hashTable, &val);
   }
   return *this;
}

HashTable &HashTable::append(Variant &&value, bool forceNew)
{
   zval val = value.detach(true);
   if (forceNew) {
      zend_hash_next_index_insert_new(&m_hashTable, &val);
   } else {
      zend_hash_next_index_insert(&m_hashTable, &val);
   }
   return *this;
}

HashTable::~HashTable()
{
   zend_hash_destroy(&m_hashTable);
}

Variant HashTable::operator [](StringRef key)
{
   std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
   zval *value = zend_hash_find(&m_hashTable, zkey.get());
   if (nullptr == value) {
      value = zend_hash_add_empty_element(&m_hashTable, zkey.get());
   }
   return Variant(value, true);
}

Variant HashTable::operator [](vmapi_ulong index)
{
   zval *value = zend_hash_index_find(&m_hashTable, index);
   if (nullptr == value) {
      value = zend_hash_index_add_empty_element(&m_hashTable, index);
   }
   return Variant(value, true);
}

Variant HashTable::iterator::getKey() const
{
   zend_string *key;
   IndexType index;
   int keyType = zend_hash_get_current_key_ex(const_cast<::HashTable *>(m_hashTable),
                                              &key,
                                              &index,
                                              const_cast<::HashPosition *>(&m_index));
   VMAPI_ASSERT_X(keyType != HASH_KEY_NON_EXISTENT, "vmapi::HashTable::iterator", "Current key is not exist");
   // we just copy here
   if (keyType == HASH_KEY_IS_STRING) {
      return Variant(ZSTR_VAL(key), ZSTR_LEN(key));
   } else {
      return Variant(static_cast<vmapi_long>(index));
   }
}

std::string HashTable::iterator::getStrKey() const
{
   zend_string *key;
   int keyType = zend_hash_get_current_key_ex(const_cast<::HashTable *>(m_hashTable),
                                              &key,
                                              nullptr,
                                              const_cast<::HashPosition *>(&m_index));
   VMAPI_ASSERT_X(keyType != HASH_KEY_NON_EXISTENT, "vmapi::HashTable::iterator", "Current key is not exist");
   // we just copy here
   return std::string(ZSTR_VAL(key), ZSTR_LEN(key));
}

HashTable::IndexType HashTable::iterator::getNumericKey() const
{
   IndexType index;
   int keyType = zend_hash_get_current_key_ex(const_cast<::HashTable *>(m_hashTable),
                                              nullptr,
                                              &index,
                                              const_cast<::HashPosition *>(&m_index));
   VMAPI_ASSERT_X(keyType != HASH_KEY_NON_EXISTENT, "vmapi::HashTable::iterator", "Current key is not exist");
   return index;
}

Variant HashTable::getValue(vmapi_ulong index, const Variant &defaultValue) const
{
   zval *result = zend_hash_index_find(&m_hashTable, index);
   if (nullptr == defaultValue) {
      return defaultValue;
   }
   return result;
}

Variant HashTable::getValue(StringRef key, const Variant &defaultValue) const
{
   std::shared_ptr<zend_string> zkey = initZStrFromStringRef(key);
   zval *result = zend_hash_find(&m_hashTable, zkey.get());
   if (nullptr == result) {
      return defaultValue;
   }
   return result;
}

Variant HashTable::getKey() const
{
   zend_string *key;
   zend_ulong index;
   int keyType = zend_hash_get_current_key(const_cast<::HashTable *>(&m_hashTable), &key, &index);
   VMAPI_ASSERT_X(keyType != HASH_KEY_NON_EXISTENT, "vmapi::HashTable::iterator", "Current key is not exist");
   return keyType == HASH_KEY_IS_STRING ? Variant(ZSTR_VAL(key), ZSTR_LEN(key)) : Variant(static_cast<vmapi_long>(index));
}

Variant HashTable::getKey(const Variant &value, const Variant &defaultKey) const
{
   zend_string *key = nullptr;
   vmapi_ulong index = 0;
   zval *targetValue;
   POLAR_HASH_FOREACH_KEY_VAL(const_cast<::HashTable *>(&m_hashTable), index, key, targetValue)
         if (value == *targetValue) {
      if (key != nullptr) {
         return Variant(ZSTR_VAL(key), ZSTR_LEN(key));
      } else {
         // maybe overflow
         return Variant(static_cast<vmapi_long>(index));
      }
   }
   POLAR_HASH_FOREACH_END();
   return defaultKey;
}

Variant HashTable::getKey(const Variant &value) const
{
   zend_string *key = nullptr;
   vmapi_ulong index = 0;
   zval *targetValue;
   POLAR_HASH_FOREACH_KEY_VAL(const_cast<::HashTable *>(&m_hashTable), index, key, targetValue)
         if (value == *targetValue) {
      if (key != nullptr) {
         return Variant(ZSTR_VAL(key), ZSTR_LEN(key));
      } else {
         // maybe overflow
         return Variant(static_cast<vmapi_long>(index));
      }
   }
   POLAR_HASH_FOREACH_END();
   return Variant(nullptr);
}

HashTable::iterator &HashTable::iterator::operator ++()
{
   VMAPI_ASSERT_X(m_hashTable != nullptr, "vmapi::HashTable::iterator", "m_hashTable can't be nullptr");
   int result = zend_hash_move_forward_ex(m_hashTable, &m_index);
   if (result == VMAPI_FAILURE) {
      m_index = HT_INVALID_IDX;
   }
   return *this;
}

HashTable::iterator HashTable::iterator::operator ++(int)
{
   iterator iter = *this;
   VMAPI_ASSERT_X(m_hashTable != nullptr, "vmapi::HashTable::iterator", "m_hashTable can't be nullptr");
   int result = zend_hash_move_forward_ex(m_hashTable, &m_index);
   if (result == VMAPI_FAILURE) {
      m_index = HT_INVALID_IDX;
   }
   return iter;
}

HashTable::iterator &HashTable::iterator::operator --()
{
   VMAPI_ASSERT_X(m_hashTable != nullptr, "vmapi::HashTable::iterator", "m_hashTable can't be nullptr");
   int result = zend_hash_move_backwards_ex(m_hashTable, &m_index);
   VMAPI_ASSERT_X(result == VMAPI_SUCCESS, "vmapi::HashTable::iterator", "Iterating backward beyond begin()");
   return *this;
}

HashTable::iterator HashTable::iterator::operator --(int)
{
   iterator iter = *this;
   VMAPI_ASSERT_X(m_hashTable != nullptr, "vmapi::HashTable::iterator", "m_hashTable can't be nullptr");
   int result = zend_hash_move_backwards_ex(m_hashTable, &m_index);
   VMAPI_ASSERT_X(result == VMAPI_SUCCESS, "vmapi::HashTable::iterator", "Iterating backward beyond begin()");
   return iter;
}

HashTable::iterator HashTable::iterator::operator +(int32_t step)
{
   iterator iter = *this;
   if (step > 0) {
      while (step--) {
         ++iter;
      }
   } else {
      while (step++) {
         --iter;
      }
   }
   return iter;
}

void HashTable::each(DefaultForeachVisitor visitor) const
{
   zend_string *key = nullptr;
   vmapi_ulong index = 0;
   zval *value;
   POLAR_HASH_FOREACH_KEY_VAL(const_cast<::HashTable *>(&m_hashTable), index, key, value)
         if (key != nullptr) {
      visitor(Variant(ZSTR_VAL(key), ZSTR_LEN(key)), Variant(value));
   } else {
      // maybe overflow
      visitor(Variant(static_cast<vmapi_long>(index)), Variant(value));
   }
   POLAR_HASH_FOREACH_END();
}

void HashTable::reverseEach(DefaultForeachVisitor visitor) const
{
   zend_string *key = nullptr;
   vmapi_ulong index = 0;
   zval *value;
   POLAR_HASH_REVERSE_FOREACH_KEY_VAL(const_cast<::HashTable *>(&m_hashTable), index, key, value)
         if (key != nullptr) {
      visitor(Variant(ZSTR_VAL(key), ZSTR_LEN(key)), Variant(value));
   } else {
      // maybe overflow
      visitor(Variant(static_cast<vmapi_long>(index)), Variant(value));
   }
   POLAR_HASH_FOREACH_END();
}

std::vector<Variant> HashTable::getKeys() const
{
   zend_string *key = nullptr;
   vmapi_ulong index = 0;
   std::vector<Variant> keys;
   POLAR_HASH_FOREACH_KEY(const_cast<::HashTable *>(&m_hashTable), index, key)
         if (key != nullptr) {
      keys.push_back(Variant(ZSTR_VAL(key), ZSTR_LEN(key)));
   } else {
      // maybe overflow
      keys.push_back(Variant(static_cast<vmapi_long>(index)));
   }
   POLAR_HASH_FOREACH_END();
   return keys;
}

std::vector<Variant> HashTable::getKeys(const Variant &value) const
{
   zend_string *key = nullptr;
   vmapi_ulong index = 0;
   zval *curValue;
   std::vector<Variant> keys;
   POLAR_HASH_REVERSE_FOREACH_KEY_VAL(const_cast<::HashTable *>(&m_hashTable), index, key, curValue)
         if (value == *curValue) {
      if (key != nullptr) {
         keys.push_back(Variant(ZSTR_VAL(key), ZSTR_LEN(key)));
      } else {
         // maybe overflow
         keys.push_back(Variant(static_cast<vmapi_long>(index)));
      }
   }
   POLAR_HASH_FOREACH_END();
   return keys;
}

std::vector<Variant> HashTable::getValues() const
{
   zval *value;
   std::vector<Variant> values;
   POLAR_HASH_FOREACH_VAL(const_cast<::HashTable *>(&m_hashTable), value)
         values.push_back(value);
   POLAR_HASH_FOREACH_END();
   return values;
}

HashTable::iterator::~iterator()
{}

std::shared_ptr<zend_string> HashTable::initZStrFromStringRef(StringRef str) const
{
   return std::shared_ptr<zend_string>(zend_string_init(str.getData(), str.size(), 0), [](zend_string *ptr){
      zend_string_release(ptr);
   });
}

} // vmapi
} // polar
