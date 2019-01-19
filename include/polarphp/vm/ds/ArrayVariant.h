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

#ifndef POLARPHP_VMAPI_DS_ARRAY_VARIANT_H
#define POLARPHP_VMAPI_DS_ARRAY_VARIANT_H

#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"
#include "polarphp/vm/utils/Funcs.h"

#include <cstddef>
#include <utility>
#include <string>
#include <list>
#include <map>
#include <type_traits>
#include <initializer_list>

namespace polar {
namespace vmapi {

class NumericVariant;
class BoolVarint;
class DoubleVariant;
class StringVariant;

class VMAPI_DECL_EXPORT ArrayVariant final : public Variant
{
public:
   using IndexType = uint32_t;
   using SizeType = uint32_t;
   using KeyType = std::pair<vmapi_ulong, std::shared_ptr<std::string>>;
   using DifferenceType = std::ptrdiff_t;
   using ValueType = Variant;
   using InitMapType = std::map<Variant, Variant, VariantKeyLess>;
   using Visitor = std::function<bool(const KeyType &key, const Variant &value)>;
   // forward declare
   class Iterator;
   class ConstIterator;
public:
   ArrayVariant();
   ArrayVariant(const ArrayVariant &other);
   ArrayVariant(ArrayVariant &other, bool isRef);
   ArrayVariant(zval &other, bool isRef = false);
   ArrayVariant(zval &&other, bool isRef = false);
   ArrayVariant(zval *other, bool isRef = false);
   ArrayVariant(ArrayVariant &&other) noexcept;
   ArrayVariant(const Variant &other);
   ArrayVariant(Variant &&other);
   ArrayVariant(const std::initializer_list<Variant> &list);
   explicit ArrayVariant(const std::map<Variant, Variant, VariantKeyLess> &map);
   // operators
   ArrayItemProxy operator [](vmapi_ulong index);
   template <typename T,
             typename Selector = typename std::enable_if<std::is_integral<T>::value>::type>
   ArrayItemProxy operator [](T index);
   ArrayItemProxy operator [](const std::string &key);
   ArrayItemProxy operator [](const char *key);
   bool operator ==(const ArrayVariant &other) const;
   bool operator !=(const ArrayVariant &other) const;
   ArrayVariant &operator =(const ArrayVariant &other);
   ArrayVariant &operator =(const Variant &other);

   bool strictEqual(const ArrayVariant &other) const;
   bool strictNotEqual(const ArrayVariant &other) const;
   // modify methods
   Iterator insert(vmapi_ulong index, const Variant &value);
   Iterator insert(vmapi_ulong index, Variant &&value);
   Iterator insert(const std::string &key, const Variant &value);
   Iterator insert(const std::string &key, Variant &&value);
   Iterator append(const Variant &value);
   Iterator append(Variant &&value);
   void clear() noexcept;
   bool remove(vmapi_ulong index) noexcept;
   bool remove(const std::string &key) noexcept;
   Iterator erase(ConstIterator &iter);
   Iterator erase(Iterator &iter);
   Variant take(const std::string &key);
   Variant take(vmapi_ulong index);
   // info access
   bool isEmpty() const noexcept;
   bool isNull() const noexcept;
   SizeType getSize() const noexcept;
   SizeType getCapacity() const noexcept;
   SizeType count() const noexcept;
   Variant getValue(vmapi_ulong index) const;
   Variant getValue(const std::string &key) const;
   bool contains(vmapi_ulong index) const;
   bool contains(const std::string &key) const;
   vmapi_long getNextInsertIndex() const;
   std::list<KeyType> getKeys() const;
   std::list<KeyType> getKeys(const Variant &value, bool strict = false) const;
   std::list<Variant> getValues() const;
   Iterator find(vmapi_ulong index);
   Iterator find(const std::string &key);
   ConstIterator find(vmapi_ulong index) const;
   ConstIterator find(const std::string &key) const;
   void map(Visitor visitor) const noexcept;
   // iterators
   Iterator begin() noexcept;
   ConstIterator begin() const noexcept;
   ConstIterator cbegin() const noexcept;
   Iterator end() noexcept;
   ConstIterator end() const noexcept;
   ConstIterator cend() const noexcept;
   ~ArrayVariant();

   // iterator class
   class Iterator
   {
   public:
      using IteratorCategory = std::bidirectional_iterator_tag;
      using DifferenceType = std::ptrdiff_t;
      using ValueType = Variant;
      using ZvalPointer = zval *;
      using ZvalReference = zval &;
   public:
      Iterator(const Iterator &other);
      Iterator(Iterator &&other) noexcept;
      ~Iterator();
      Variant getValue() const;
      ZvalReference getZval() const;
      ZvalPointer getZvalPtr() const;
      KeyType getKey() const;
      HashPosition getCurrentPos() const;
      // operators
      Iterator &operator =(const Iterator &other);
      Iterator &operator =(Iterator &&other) noexcept;
      ZvalReference operator *();
      ZvalPointer operator->();
      bool operator ==(const Iterator &other);
      bool operator !=(const Iterator &other);
      Iterator &operator ++();
      Iterator operator ++(int);
      Iterator &operator --();
      Iterator operator --(int);
      Iterator operator +(int32_t step) const;
      Iterator operator -(int32_t step) const;
      Iterator &operator +=(int32_t step);
      Iterator &operator -=(int32_t step);
   protected:
      Iterator(_zend_array *array, HashPosition *pos = nullptr);
   protected:
      friend class ArrayVariant;
      _zend_array *m_array;
      uint32_t m_idx = -1; // invalid idx
      bool m_isEnd;
   };

   class ConstIterator : public Iterator
   {
   public:
      using IteratorCategory = std::bidirectional_iterator_tag;
      using DifferenceType = std::ptrdiff_t;
      using ValueType = Variant;
      using ZvalPointer = const zval *;
      using ZvalReference = const zval &;
   public:
      ConstIterator(const ConstIterator &other);
      ConstIterator(ConstIterator &&other) noexcept;
      ~ConstIterator();
      const Variant getValue() const;
      ZvalReference getZval() const;
      ZvalPointer getZvalPtr() const;
      const KeyType getKey() const;
      // operators
      ConstIterator &operator =(const ConstIterator &other);
      ConstIterator &operator =(ConstIterator &&other) noexcept;
      ZvalReference operator *();
      ZvalPointer operator->();
      bool operator ==(const ConstIterator &other) const;
      bool operator !=(const ConstIterator &other) const;
      ConstIterator &operator ++();
      ConstIterator operator ++(int);
      ConstIterator &operator --();
      ConstIterator operator --(int);
      ConstIterator operator +(int32_t step) const;
      ConstIterator operator -(int32_t step) const;
      ConstIterator &operator +=(int32_t step);
      ConstIterator &operator -=(int32_t step);
   protected:
      ConstIterator(_zend_array *array, HashPosition *pos = nullptr);
   protected:
      friend class ArrayVariant;
   };

protected:
   _zend_array *getZendArrayPtr() const noexcept;
   _zend_array &getZendArray() const noexcept;
   uint32_t calculateIdxFromZval(zval *val) const noexcept;
   uint32_t findArrayIdx(const std::string &key) const noexcept;
   uint32_t findArrayIdx(vmapi_ulong index) const noexcept;
protected:
   friend class ArrayItemProxy;
   friend class Iterator;
   friend class ConstIterator;
};

template <typename T, typename Selector>
ArrayItemProxy ArrayVariant::operator [](T index)
{
   if (index < 0) {
      index = 0;
   }
   return operator [](static_cast<vmapi_ulong>(index));
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_ARRAY_VARIANT_H
