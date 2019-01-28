// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#include "polarphp/vm/ds/ArrayVariant.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"
#include "polarphp/vm/ds/internal/VariantPrivate.h"

#include <iostream>
#include <string>

namespace polar {
namespace vmapi {

#if ZEND_DEBUG

#define HASH_MASK_CONSISTENCY	0xc0
#define HT_OK					0x00
#define HT_IS_DESTROYING		0x40
#define HT_DESTROYED			0x80
#define HT_CLEANING				0xc0

static void _zend_is_inconsistent(const HashTable *ht, const char *file, int line)
{
   if ((ht->u.flags & HASH_MASK_CONSISTENCY) == HT_OK) {
      return;
   }
   switch ((ht->u.flags & HASH_MASK_CONSISTENCY)) {
   case HT_IS_DESTROYING:
      zend_output_debug_string(1, "%s(%d) : ht=%p is being destroyed", file, line,
                               reinterpret_cast<void *>(const_cast<HashTable *>(ht)));
      break;
   case HT_DESTROYED:
      zend_output_debug_string(1, "%s(%d) : ht=%p is already destroyed", file, line,
                               reinterpret_cast<void *>(const_cast<HashTable *>(ht)));
      break;
   case HT_CLEANING:
      zend_output_debug_string(1, "%s(%d) : ht=%p is being cleaned", file, line,
                               reinterpret_cast<void *>(const_cast<HashTable *>(ht)));
      break;
   default:
      zend_output_debug_string(1, "%s(%d) : ht=%p is inconsistent", file, line,
                               reinterpret_cast<void *>(const_cast<HashTable *>(ht)));
      break;
   }
   vmapi_bailout();
}
#define IS_CONSISTENT(a) _zend_is_inconsistent(a, __FILE__, __LINE__);
#define SET_INCONSISTENT(n) do { \
   (ht)->u.flags |= n; \
} while (0)
#else
#define IS_CONSISTENT(a)
#define SET_INCONSISTENT(n)
#endif

namespace
{

// copy from zend_operators.c
int hash_zval_identical_function(const void *z1, const void *z2)
{
   zval result;
   zval *zval1 = const_cast<zval *>(reinterpret_cast<const zval *>(z1));
   zval *zval2 = const_cast<zval *>(reinterpret_cast<const zval *>(z2));
   /* is_identical_function() returns 1 in case of identity and 0 in case
    * of a difference;
    * whereas this comparison function is expected to return 0 on identity,
    * and non zero otherwise.
    */
   ZVAL_DEREF(zval1);
   ZVAL_DEREF(zval2);
   if (is_identical_function(&result, zval1, zval2) == FAILURE) {
      return 1;
   }
   return Z_TYPE(result) != IS_TRUE;
}
} // anonymous namespace

using internal::VariantPrivate;

// iterator class alias
using ArrayIterator = ArrayVariant::Iterator;
using ConstArrayIterator = ArrayVariant::ConstIterator;

ArrayVariant::ArrayVariant()
{
   // constructor empty array
   array_init(getUnDerefZvalPtr());
}

ArrayVariant::ArrayVariant(const ArrayVariant &other)
   : Variant(other)
{
   if (other.getUnDerefType() == Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
}

ArrayVariant::ArrayVariant(ArrayVariant &other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (!isRef) {
      zval *otherPtr = other.getUnDerefZvalPtr();
      stdCopyZval(self, const_cast<zval *>(otherPtr));
      if (Z_TYPE_P(otherPtr) == IS_REFERENCE) {
         SEPARATE_ARRAY(self);
      }
   } else {
      zval *source = other.getUnDerefZvalPtr();
      ZVAL_MAKE_REF(source);
      ZVAL_COPY(self, source);
      // if the reference array reference count > 1
      // separate here
      SEPARATE_ARRAY(getZvalPtr());
   }
}

ArrayVariant::ArrayVariant(zval &other, bool isRef)
   : ArrayVariant(&other, isRef)
{}

ArrayVariant::ArrayVariant(zval &&other, bool isRef)
   : ArrayVariant(&other, isRef)
{}

ArrayVariant::ArrayVariant(zval *other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (nullptr != other) {
      if (isRef && (Z_TYPE_P(other) == IS_ARRAY ||
                    (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_ARRAY))) {
         SEPARATE_STRING(other);
         ZVAL_MAKE_REF(other);
         zend_reference *ref = Z_REF_P(other);
         GC_ADDREF(ref);
         ZVAL_REF(self, ref);
      } else if ((Z_TYPE_P(other) == IS_ARRAY ||
                  (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_ARRAY))) {
         ZVAL_DEREF(other);
         ZVAL_COPY(self, other);
      }else {
         ZVAL_DUP(self, other);
         convert_to_array(self);
      }
   } else {
      array_init(self);
   }
}

ArrayVariant::ArrayVariant(ArrayVariant &&other) noexcept
   : Variant(std::move(other))
{}

ArrayVariant::ArrayVariant(const Variant &other)
{
   zval *from = const_cast<zval *>(other.getZvalPtr());
   zval *self = getUnDerefZvalPtr();
   if (other.getType() == Type::Array) {
      ZVAL_COPY(self, from);
   } else {
      zval temp;
      ZVAL_DUP(&temp, from);
      convert_to_array(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
}

ArrayVariant::ArrayVariant(const std::initializer_list<Variant> &list)
   : ArrayVariant()
{
   for (const Variant &item : list) {
      append(item);
   }
}

ArrayVariant::ArrayVariant(const std::map<Variant, Variant, VariantKeyLess> &map)
   : ArrayVariant()
{
   for(auto &item : map) {
      Type type = item.first.getType();
      if (type != Type::Long && type != Type::String) {
         continue;
      }
      if (type == Type::Long) {
         insert(Z_LVAL_P(item.first.getZvalPtr()), item.second);
      } else {
         insert(Z_STRVAL_P(item.first.getZvalPtr()), item.second);
      }
   }
}

ArrayVariant::ArrayVariant(Variant &&other)
   : Variant(std::move(other))
{
   if (getType() != Type::Array) {
      convert_to_array(getUnDerefZvalPtr());
   }
}

ArrayItemProxy ArrayVariant::operator [](vmapi_ulong index)
{
   return ArrayItemProxy(getZvalPtr(), index);
}

ArrayItemProxy ArrayVariant::operator [](const std::string &key)
{
   return ArrayItemProxy(getZvalPtr(), key);
}

ArrayItemProxy ArrayVariant::operator [](const char *key)
{
   return ArrayItemProxy(getZvalPtr(), key);
}

bool ArrayVariant::operator ==(const ArrayVariant &other) const
{
   return this == &other ? true
                         : 0 == zend_compare_arrays(const_cast<zval *>(getZvalPtr()), const_cast<zval *>(other.getZvalPtr()));
}

bool ArrayVariant::operator !=(const ArrayVariant &other) const
{
   return !operator ==(other);
}

ArrayVariant &ArrayVariant::operator =(const ArrayVariant &other)
{
   if (this != &other) {
      if (getUnDerefType() != Type::Reference) {
         SEPARATE_ARRAY(getUnDerefZvalPtr());
      }
      Variant::operator =(other);
   }
   return *this;
}

ArrayVariant &ArrayVariant::operator =(const Variant &other)
{
   zval *self = getZvalPtr();
   zval *from = const_cast<zval *>(other.getZvalPtr());
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(self);
   }
   // need set gc info
   if (other.getType() == Type::Array) {
      // standard copy
      Variant::operator =(from);
   } else {
      zval temp;
      // will increase 1 to gc refcount
      ZVAL_DUP(&temp, from);
      // will decrease 1 to gc refcount
      convert_to_array(&temp);
      // we need free original zend_array memory
      zend_array_destroy(Z_ARR_P(self));
      ZVAL_COPY_VALUE(self, &temp);
   }
   return *this;
}

bool ArrayVariant::strictEqual(const ArrayVariant &other) const
{
   if (this == &other) {
      return true;
   }
   return zend_hash_compare(getZendArrayPtr(), other.getZendArrayPtr(),
                            static_cast<compare_func_t>(hash_zval_identical_function), 1) == 0;
}

bool ArrayVariant::strictNotEqual(const ArrayVariant &other) const
{
   return !strictEqual(other);
}

ArrayIterator ArrayVariant::insert(vmapi_ulong index, const Variant &value)
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zval *zvalPtr = const_cast<zval *>(value.getZvalPtr());
   zval temp;
   ZVAL_COPY(&temp, zvalPtr);
   zend_array *selfArrPtr = getZendArrayPtr();
   zval *valPtr = zend_hash_index_update(selfArrPtr, index, &temp);
   if (valPtr) {
      HashPosition pos = calculateIdxFromZval(valPtr);
      return ArrayIterator(selfArrPtr, &pos);
   } else {
      return ArrayIterator(selfArrPtr, nullptr);
   }
}

ArrayIterator ArrayVariant::insert(vmapi_ulong index, Variant &&value)
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zval *zvalPtr = value.getZvalPtr();
   zval temp;
   ZVAL_COPY_VALUE(&temp, zvalPtr);
   std::memset(&value.m_implPtr->m_buffer, 0, sizeof(value.m_implPtr->m_buffer));
   zend_array *selfArrPtr = getZendArrayPtr();
   zval *valPtr = zend_hash_index_update(selfArrPtr, index, &temp);
   if (valPtr) {
      HashPosition pos = calculateIdxFromZval(valPtr);
      return ArrayIterator(selfArrPtr, &pos);
   } else {
      return ArrayIterator(selfArrPtr, nullptr);
   }
}

ArrayIterator ArrayVariant::insert(const std::string &key, const Variant &value)
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zval *zvalPtr = const_cast<zval *>(value.getZvalPtr());
   zval temp;
   ZVAL_COPY(&temp, zvalPtr);
   zend_array *selfArrPtr = getZendArrayPtr();
   zval *valPtr = zend_hash_str_update(selfArrPtr, key.c_str(), key.length(), &temp);
   if (valPtr) {
      HashPosition pos = calculateIdxFromZval(valPtr);
      return ArrayIterator(selfArrPtr, &pos);
   } else {
      return ArrayIterator(selfArrPtr, nullptr);
   }
}

ArrayIterator ArrayVariant::insert(const std::string &key, Variant &&value)
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zval *zvalPtr = value.getZvalPtr();
   zval temp;
   ZVAL_COPY_VALUE(&temp, zvalPtr);
   std::memset(&value.m_implPtr->m_buffer, 0, sizeof(value.m_implPtr->m_buffer));
   zend_array *selfArrPtr = getZendArrayPtr();
   zval *valPtr = zend_hash_str_update(selfArrPtr, key.c_str(), key.length(), &temp);
   if (valPtr) {
      HashPosition pos = calculateIdxFromZval(valPtr);
      return ArrayIterator(selfArrPtr, &pos);
   } else {
      return ArrayIterator(selfArrPtr, nullptr);
   }
}

ArrayIterator ArrayVariant::append(const Variant &value)
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zval *zvalPtr = const_cast<zval *>(value.getZvalPtr());
   zval temp;
   ZVAL_COPY(&temp, zvalPtr);
   zend_array *selfArrPtr = getZendArrayPtr();
   zval *valPtr = zend_hash_next_index_insert(selfArrPtr, &temp);
   if (valPtr) {
      HashPosition pos = calculateIdxFromZval(valPtr);
      return ArrayIterator(selfArrPtr, &pos);
   } else {
      return ArrayIterator(selfArrPtr, nullptr);
   }
}

ArrayIterator ArrayVariant::append(Variant &&value)
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zval *zvalPtr = value.getZvalPtr();
   zval temp;
   ZVAL_COPY_VALUE(&temp, zvalPtr);
   std::memset(&value.m_implPtr->m_buffer, 0, sizeof(value.m_implPtr->m_buffer));
   zend_array *selfArrPtr = getZendArrayPtr();
   zval *valPtr = zend_hash_next_index_insert(selfArrPtr, &temp);
   if (valPtr) {
      HashPosition pos = calculateIdxFromZval(valPtr);
      return ArrayIterator(selfArrPtr, &pos);
   } else {
      return ArrayIterator(selfArrPtr, nullptr);
   }
}

void ArrayVariant::clear() noexcept
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zend_hash_clean(getZendArrayPtr());
}

bool ArrayVariant::remove(vmapi_ulong index) noexcept
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   return zend_hash_index_del(getZendArrayPtr(), index) == VMAPI_SUCCESS;
}

bool ArrayVariant::remove(const std::string &key) noexcept
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   return zend_hash_str_del(getZendArrayPtr(), key.c_str(), key.length()) == VMAPI_SUCCESS;
}

ArrayVariant::Iterator ArrayVariant::erase(ConstIterator &iter)
{
   if (getUnDerefType() != Type::Reference) {
      SEPARATE_ARRAY(getUnDerefZvalPtr());
   }
   zend_array *array = getZendArrayPtr();
   // here we need check the iter check whether iter pointer this array
   if (iter == cend() || array != iter.m_array) {
      return iter;
   }
   Iterator nextIter = iter;
   ++nextIter;
   KeyType key = iter.getKey();
   int deleteStatus;
   if (key.second) {
      std::string *keyStr = key.second.get();
      deleteStatus = zend_hash_str_del(array, keyStr->c_str(), keyStr->length());
   } else {
      deleteStatus = zend_hash_index_del(array, key.first);
   }
   if (VMAPI_FAILURE == deleteStatus) {
      return iter;
   }
   return nextIter;
}

ArrayVariant::Iterator ArrayVariant::erase(Iterator &iter)
{
   return erase(static_cast<ConstIterator &>(iter));
}

Variant ArrayVariant::take(const std::string &key)
{
   Iterator iter = find(key);
   Variant ret(iter.getValue());
   if (iter != end()) {
      remove(key);
   }
   return ret;
}

Variant ArrayVariant::take(vmapi_ulong index)
{
   Iterator iter = find(index);
   Variant ret(iter.getValue());
   if (iter != end()) {
      remove(index);
   }
   return ret;
}

bool ArrayVariant::isEmpty() const noexcept
{
   return 0 == getSize();
}

bool ArrayVariant::isNull() const noexcept
{
   return false;
}

ArrayVariant::SizeType ArrayVariant::getSize() const noexcept
{
   // @TODO here we just use zend_hash_num_elements or zend_array_count
   return zend_hash_num_elements(getZendArrayPtr());
}

ArrayVariant::SizeType ArrayVariant::count() const noexcept
{
   return getSize();
}

ArrayVariant::SizeType ArrayVariant::getCapacity() const noexcept
{
   return getZendArrayPtr()->nTableSize;
}

Variant ArrayVariant::getValue(vmapi_ulong index) const
{
   zval *val = zend_hash_index_find(getZendArrayPtr(), index);
   if (nullptr == val) {
      vmapi::notice() << "Undefined offset: " << index << std::endl;
   }
   return val;
}

Variant ArrayVariant::getValue(const std::string &key) const
{
   zval *val = zend_hash_str_find(getZendArrayPtr(), key.c_str(), key.length());
   if (nullptr == val) {
      vmapi::notice() << "Undefined index: " << key << std::endl;
   }
   return val;
}

bool ArrayVariant::contains(vmapi_ulong index) const
{
   return zend_hash_index_exists(getZendArrayPtr(), index) == 1;
}

bool ArrayVariant::contains(const std::string &key) const
{
   return zend_hash_str_exists(getZendArrayPtr(), key.c_str(), key.length()) == 1;
}

vmapi_long ArrayVariant::getNextInsertIndex() const
{
   return getZendArrayPtr()->nNextFreeElement;
}

std::list<ArrayVariant::KeyType> ArrayVariant::getKeys() const
{
   std::list<KeyType> keys;
   vmapi_ulong index;
   zend_string *key;
   if (0 == getSize()) {
      return keys;
   }
   POLAR_HASH_FOREACH_KEY(getZendArrayPtr(), index, key) {
      if (key) {
         keys.push_back(KeyType(-1, std::shared_ptr<std::string>(new std::string(ZSTR_VAL(key), ZSTR_LEN(key)))));
      } else {
         keys.push_back(KeyType(index, nullptr));
      }
   } POLAR_HASH_FOREACH_END();
   return keys;
}

std::list<ArrayVariant::KeyType> ArrayVariant::getKeys(const Variant &value, bool strict) const
{
   std::list<KeyType> keys;
   vmapi_ulong index;
   zend_string *key;
   zval *entry;
   zval *searchValue = const_cast<zval *>(value.getZvalPtr());
   if (0 == getSize()) {
      return keys;
   }
   if (strict) {
      POLAR_HASH_FOREACH_KEY_VAL_IND(getZendArrayPtr(), index, key, entry) {
         ZVAL_DEREF(entry);
         if (fast_is_identical_function(searchValue, entry)) {
            if (key) {
               keys.push_back(KeyType(-1, std::shared_ptr<std::string>(new std::string(ZSTR_VAL(key), ZSTR_LEN(key)))));
            } else {
               keys.push_back(KeyType(index, nullptr));
            }
         }
      } POLAR_HASH_FOREACH_END();
   } else {
      POLAR_HASH_FOREACH_KEY_VAL_IND(getZendArrayPtr(), index, key, entry) {
         if (fast_equal_check_function(searchValue, entry)) {
            if (key) {
               keys.push_back(KeyType(-1, std::shared_ptr<std::string>(new std::string(ZSTR_VAL(key), ZSTR_LEN(key)))));
            } else {
               keys.push_back(KeyType(index, nullptr));
            }
         }
      } POLAR_HASH_FOREACH_END();
   }
   return keys;
}

std::list<Variant> ArrayVariant::getValues() const
{
   std::list<Variant> values;
   zval *entry;
   if (0 == getSize()) {
      return values;
   }
   POLAR_HASH_FOREACH_VAL(getZendArrayPtr(), entry) {
      if (UNEXPECTED(Z_ISREF_P(entry) && Z_REFCOUNT_P(entry) == 1)) {
         entry = Z_REFVAL_P(entry);
      }
      values.emplace_back(entry);
   } POLAR_HASH_FOREACH_END();
   return values;
}

ArrayIterator ArrayVariant::find(vmapi_ulong index)
{
   return static_cast<ArrayIterator>(static_cast<const ArrayVariant>(*this).find(index));
}

ArrayIterator ArrayVariant::find(const std::string &key)
{
   return static_cast<ArrayIterator>(static_cast<const ArrayVariant>(*this).find(key));
}

ConstArrayIterator ArrayVariant::find(vmapi_ulong index) const
{
   zend_array *array = getZendArrayPtr();
   Bucket *p;
   IS_CONSISTENT(array);
   uint32_t idx = HT_INVALID_IDX;
   if (array->u.flags & HASH_FLAG_PACKED) {
      if (index < array->nNumUsed) {
         p = array->arData + index;
         if (Z_TYPE(p->val) != IS_UNDEF) {
            idx = index;
         }
      }
   }
   if (idx == HT_INVALID_IDX) {
      idx = findArrayIdx(index);
   }
   if (idx == HT_INVALID_IDX) {
      return end();
   }
   return ConstArrayIterator(array, &idx);
}

ConstArrayIterator ArrayVariant::find(const std::string &key) const
{
   zend_array *array = getZendArrayPtr();
   IS_CONSISTENT(array);
   uint32_t idx = findArrayIdx(key);
   if (idx == HT_INVALID_IDX) {
      return end();
   }
   return ConstArrayIterator(array, &idx);
}

void ArrayVariant::map(Visitor visitor) const noexcept
{
   vmapi_ulong index;
   zend_string *key;
   zval *entry;
   bool visitorRet = true;
   POLAR_HASH_FOREACH_KEY_VAL_IND(getZendArrayPtr(), index, key, entry) {
      if (key) {
         visitorRet = visitor(KeyType(-1, std::shared_ptr<std::string>(new std::string(ZSTR_VAL(key), ZSTR_LEN(key)))), Variant(entry));
      } else {
         visitorRet = visitor(KeyType(index, nullptr), Variant(entry));
      }
      if (!visitorRet) {
         return;
      }
   } POLAR_HASH_FOREACH_END();
}

ArrayIterator ArrayVariant::begin() noexcept
{
   HashPosition pos = 0;
   return ArrayIterator(getZendArrayPtr(), &pos);
}

ConstArrayIterator ArrayVariant::begin() const noexcept
{
   HashPosition pos = 0;
   return ConstArrayIterator(getZendArrayPtr(), &pos);
}

ConstArrayIterator ArrayVariant::cbegin() const noexcept
{
   HashPosition pos = 0;
   return ConstArrayIterator(getZendArrayPtr(), &pos);
}

ArrayIterator ArrayVariant::end() noexcept
{
   return ArrayIterator(getZendArrayPtr(), nullptr);
}

ConstArrayIterator ArrayVariant::end() const noexcept
{
   return ConstArrayIterator(getZendArrayPtr(), nullptr);
}

ConstArrayIterator ArrayVariant::cend() const noexcept
{
   return ConstArrayIterator(getZendArrayPtr(), nullptr);
}

ArrayVariant::~ArrayVariant()
{
}

_zend_array *ArrayVariant::getZendArrayPtr() const noexcept
{
   return Z_ARR(getZval());
}

_zend_array &ArrayVariant::getZendArray() const noexcept
{
   return *Z_ARR(getZval());
}

uint32_t ArrayVariant::calculateIdxFromZval(zval *val) const noexcept
{
   zend_array *arr = getZendArrayPtr();
   return (reinterpret_cast<char *>(val) - reinterpret_cast<char *>(arr->arData)) / sizeof(Bucket);
}

uint32_t ArrayVariant::findArrayIdx(const std::string &key) const noexcept
{
   const char *keyStr = key.c_str();
   size_t length = key.length();
   vmapi_ulong h = zend_inline_hash_func(key.c_str(), key.length());
   uint32_t nIndex;
   uint32_t idx;
   zend_array *ht = getZendArrayPtr();
   Bucket *p, *arData;
   arData = ht->arData;
   nIndex = h | ht->nTableMask;
   idx = HT_HASH_EX(arData, nIndex);
   while (idx != HT_INVALID_IDX) {
      ZEND_ASSERT(idx < HT_IDX_TO_HASH(ht->nTableSize));
      p = HT_HASH_TO_BUCKET_EX(arData, idx);
      if ((p->h == h)
          && p->key
          && (ZSTR_LEN(p->key) == length)
          && !memcmp(ZSTR_VAL(p->key), keyStr, length)) {
         return idx;
      }
      idx = Z_NEXT(p->val);
   }
   return HT_INVALID_IDX;
}

uint32_t ArrayVariant::findArrayIdx(vmapi_ulong index) const noexcept
{
   uint32_t nIndex;
   uint32_t idx;
   Bucket *p, *arData;
   zend_array *ht = getZendArrayPtr();
   arData = ht->arData;
   nIndex = index | ht->nTableMask;
   idx = HT_HASH_EX(arData, nIndex);
   while (idx != HT_INVALID_IDX) {
      ZEND_ASSERT(idx < HT_IDX_TO_HASH(ht->nTableSize));
      p = HT_HASH_TO_BUCKET_EX(arData, idx);
      if (p->h == index && !p->key) {
         return idx;
      }
      idx = Z_NEXT(p->val);
   }
   return HT_INVALID_IDX;
}

// iterator classes

ArrayIterator::Iterator(_zend_array *array, HashPosition *pos)
   : m_array(array),
     m_isEnd(nullptr != pos ? false : true)
{
   VMAPI_ASSERT_X(m_array != nullptr, "ArrayVariant::Iterator", "m_array can't be nullptr");
   if (!m_isEnd) {
      if (0 == *pos) {
         // auto detect
         zend_hash_internal_pointer_reset_ex(m_array, pos);
      }
      m_idx = zend_hash_iterator_add(m_array, *pos);
   }
}

ArrayIterator::Iterator(const Iterator &other)
{
   // manual copy if not end iterator
   // we create a new iterator here
   m_isEnd = other.m_isEnd;
   m_array = other.m_array;
   if (!m_isEnd) {
      HashPosition otherPos = zend_hash_iterator_pos(other.m_idx, other.m_array);
      m_idx = zend_hash_iterator_add(m_array, otherPos);
   }
}

ArrayIterator::Iterator(Iterator &&other) noexcept
{
   m_isEnd = other.m_isEnd;
   m_array = other.m_array;
   m_idx = other.m_idx;
   // prevent relase iterator
   other.m_isEnd = true;
}

ArrayIterator::~Iterator()
{
   if (!m_isEnd) {
      zend_hash_iterator_del(m_idx);
   }
}

Variant ArrayIterator::getValue() const
{
   return getZvalPtr();
}

ArrayIterator::ZvalReference ArrayIterator::getZval() const
{
   HashPosition pos = getCurrentPos();
   zval *valPtr = zend_hash_get_current_data_ex(m_array, &pos);
   VMAPI_ASSERT_X(valPtr != nullptr, "ArrayVariant::Iterator::getZval", "can't apply * operator on nullptr");
   return *valPtr;
}

ArrayIterator::ZvalPointer ArrayIterator::getZvalPtr() const
{
   HashPosition pos = getCurrentPos();
   return zend_hash_get_current_data_ex(m_array, &pos);
}

ArrayVariant::KeyType ArrayIterator::getKey() const
{
   VMAPI_ASSERT_X(m_array != nullptr, "ArrayVariant::Iterator::getKey", "m_array can't be nullptr");
   zend_string *keyStr;
   vmapi_ulong index;
   HashPosition pos = getCurrentPos();
   int keyType = zend_hash_get_current_key_ex(m_array, &keyStr, &index, &pos);
   VMAPI_ASSERT_X(keyType != HASH_KEY_NON_EXISTENT, "ArrayVariant::Iterator::getKey", "Key can't not exist");
   if (HASH_KEY_IS_STRING == keyType) {
      return KeyType(-1, std::shared_ptr<std::string>(new std::string(ZSTR_VAL(keyStr), ZSTR_LEN(keyStr))));
   } else {
      return KeyType(index, nullptr);
   }
}

HashPosition ArrayIterator::getCurrentPos() const
{
   if (m_isEnd) {
      return HT_INVALID_IDX;
   }
   return zend_hash_iterator_pos(m_idx, m_array);
}

// operators
ArrayIterator::ZvalReference ArrayIterator::operator *()
{
   return getZval();
}

ArrayIterator::ZvalPointer ArrayIterator::operator->()
{
   return getZvalPtr();
}

ArrayIterator &ArrayIterator::operator =(const ArrayIterator &other)
{
   if (this != &other) {
      m_array = other.m_array;
      if (!other.m_isEnd && m_isEnd) {
         m_isEnd = false;
         HashPosition otherPos = zend_hash_iterator_pos(other.m_idx, other.m_array);
         m_idx = zend_hash_iterator_add(m_array, otherPos);
      } else if (!other.m_isEnd && !m_isEnd) {
         EG(ht_iterators)[m_idx].pos = zend_hash_iterator_pos(other.m_idx, other.m_array);
         EG(ht_iterators)[m_idx].ht = other.m_array;
      } else if (other.m_isEnd && !m_isEnd) {
         m_isEnd = true;
         zend_hash_iterator_del(m_idx);
         m_idx = -1;
      }
   }
   return *this;
}

ArrayIterator &ArrayIterator::operator =(ArrayIterator &&other) noexcept
{
   assert(this != &other);
   m_array = other.m_array;
   if (!other.m_isEnd && m_isEnd) {
      std::swap(m_isEnd, other.m_isEnd);
      std::swap(m_idx, other.m_idx);
   } else if (!other.m_isEnd && !m_isEnd) {
      EG(ht_iterators)[m_idx].pos = zend_hash_iterator_pos(other.m_idx, other.m_array);
      EG(ht_iterators)[m_idx].ht = other.m_array;
   } else if (other.m_isEnd && !m_isEnd) {
      std::swap(m_isEnd, other.m_isEnd);
      std::swap(m_idx, other.m_idx);
   }
   return *this;
}

bool ArrayIterator::operator ==(const ArrayIterator &other)
{
   return m_array == other.m_array &&
         getCurrentPos() == other.getCurrentPos();
}

bool ArrayIterator::operator !=(const ArrayIterator &other)
{
   return m_array != other.m_array ||
         getCurrentPos() != other.getCurrentPos();
}

ArrayIterator &ArrayIterator::operator ++()
{
   VMAPI_ASSERT_X(m_array != nullptr, "ArrayVariant::Iterator", "m_array can't be nullptr");
   HashPosition pos = getCurrentPos();
   int result = zend_hash_move_forward_ex(m_array, &pos);
   VMAPI_ASSERT_X(result == VMAPI_SUCCESS, "ArrayVariant::Iterator", "Iterating beyond end()");
   EG(ht_iterators)[m_idx].pos = pos;
   return *this;
}

ArrayIterator ArrayIterator::operator ++(int)
{
   ArrayIterator iter = *this;
   VMAPI_ASSERT_X(m_array != nullptr, "ArrayVariant::Iterator", "m_array can't be nullptr");
   HashPosition pos = getCurrentPos();
   int result = zend_hash_move_forward_ex(m_array, &pos);
   VMAPI_ASSERT_X(result == VMAPI_SUCCESS, "ArrayVariant::Iterator", "Iterating beyond end()");
   EG(ht_iterators)[m_idx].pos = pos;
   return iter;
}

ArrayIterator &ArrayIterator::operator --()
{
   VMAPI_ASSERT_X(m_array != nullptr, "ArrayVariant::Iterator", "m_array can't be nullptr");
   HashPosition pos = getCurrentPos();
   int result = zend_hash_move_backwards_ex(m_array, &pos);
   VMAPI_ASSERT_X(result == VMAPI_SUCCESS, "ArrayVariant::Iterator", "Iterating beyond end()");
   EG(ht_iterators)[m_idx].pos = pos;
   return *this;
}

ArrayIterator ArrayIterator::operator --(int)
{
   ArrayIterator iter = *this;
   VMAPI_ASSERT_X(m_array != nullptr, "ArrayVariant::Iterator", "m_array can't be nullptr");
   HashPosition pos = getCurrentPos();
   int result = zend_hash_move_backwards_ex(m_array, &pos);
   VMAPI_ASSERT_X(result == VMAPI_SUCCESS, "ArrayVariant::Iterator", "Iterating beyond end()");
   EG(ht_iterators)[m_idx].pos = pos;
   return iter;
}

ArrayIterator ArrayIterator::operator +(int32_t step) const
{
   Iterator iter = *this;
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

ArrayIterator ArrayIterator::operator -(int32_t step) const
{
   return operator +(-step);
}

ArrayIterator &ArrayIterator::operator +=(int32_t step)
{
   *this = *this + step;
   return *this;
}

ArrayIterator &ArrayIterator::operator -=(int32_t step)
{
   *this = *this - step;
   return *this;
}

ConstArrayIterator::ConstIterator(_zend_array *array, HashPosition *pos)
   : Iterator(array, pos)
{}

ConstArrayIterator::ConstIterator(const ConstIterator &other)
   : Iterator(other)
{}

ConstArrayIterator::ConstIterator(ConstIterator &&other) noexcept
   : Iterator(other)
{}

ConstArrayIterator::~ConstIterator()
{}

const Variant ConstArrayIterator::getValue() const
{
   return Iterator::getZvalPtr();
}

ConstArrayIterator::ZvalReference ConstArrayIterator::getZval() const
{
   return Iterator::getZval();
}

ConstArrayIterator::ZvalPointer ConstArrayIterator::getZvalPtr() const
{
   return Iterator::getZvalPtr();
}

const ArrayVariant::KeyType ConstArrayIterator::getKey() const
{
   return Iterator::getKey();
}

// operators
ConstArrayIterator &ConstArrayIterator::operator =(const ConstArrayIterator &other)
{
   if (this != &other) {
      Iterator::operator =(other);
   }
   return *this;
}

ConstArrayIterator &ConstArrayIterator::operator =(ConstArrayIterator &&other) noexcept
{
   assert(this != &other);
   Iterator::operator =(std::move(other));
   return *this;
}

ConstArrayIterator::ZvalReference ConstArrayIterator::operator *()
{
   return getZval();
}

ConstArrayIterator::ZvalPointer ConstArrayIterator::operator->()
{
   return getZvalPtr();
}

bool ConstArrayIterator::operator ==(const ConstIterator &other) const
{
   return m_array == other.m_array &&
         getCurrentPos() == other.getCurrentPos();
}

bool ConstArrayIterator::operator !=(const ConstIterator &other) const
{
   return m_array != other.m_array ||
         getCurrentPos() != other.getCurrentPos();
}

ConstArrayIterator &ConstArrayIterator::operator ++()
{
   Iterator::operator ++();
   return *this;
}

ConstArrayIterator ConstArrayIterator::operator ++(int)
{
   ConstIterator iter = *this;
   Iterator::operator ++(1);
   return iter;
}

ConstArrayIterator &ConstArrayIterator::operator --()
{
   Iterator::operator --();
   return *this;
}

ConstArrayIterator ConstArrayIterator::operator --(int)
{
   ConstIterator iter = *this;
   Iterator::operator --(1);
   return iter;
}

ConstArrayIterator ConstArrayIterator::operator +(int32_t step) const
{
   Iterator::operator +(step);
   return *this;
}

ConstArrayIterator ConstArrayIterator::operator -(int32_t step) const
{
   Iterator::operator -(step);
   return *this;
}

ConstArrayIterator &ConstArrayIterator::operator +=(int32_t step)
{
   Iterator::operator +=(step);
   return *this;
}

ConstArrayIterator &ConstArrayIterator::operator -=(int32_t step)
{
   Iterator::operator -=(step);
   return *this;
}

} // vmapi
} // polar

