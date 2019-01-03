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

#ifndef POLARPHP_VMAPI_DS_ARRAY_ITEM_PROXY_H
#define POLARPHP_VMAPI_DS_ARRAY_ITEM_PROXY_H

#include "polarphp/vm/ZendApi.h"

namespace polar {
namespace vmapi {

// forward declare class with namespace
namespace internal
{
class ArrayItemProxyPrivate;
} // internal

class ArrayItemProxy;
class Variant;
class NumericVariant;
class DoubleVariant;
class StringVariant;
class BooleanVariant;
class ArrayVariant;
using internal::ArrayItemProxyPrivate;

extern bool array_unset(ArrayItemProxy &&arrayItem);
extern bool array_isset(ArrayItemProxy &&arrayItem);

class VMAPI_DECL_EXPORT ArrayItemProxy final
{
public:
   using KeyType = std::pair<vmapi_ulong, std::shared_ptr<std::string>>;
public:
   ArrayItemProxy(zval *array, const KeyType &requestKey, ArrayItemProxy *parent = nullptr);
   ArrayItemProxy(zval *array, const std::string &key, ArrayItemProxy *parent = nullptr);
   ArrayItemProxy(zval *array, vmapi_ulong index, ArrayItemProxy *parent = nullptr);
   ArrayItemProxy(const ArrayItemProxy &other); // shadow copy
   ArrayItemProxy(ArrayItemProxy &&other) noexcept;
   ~ArrayItemProxy();
   // operators
   ArrayItemProxy &operator =(const ArrayItemProxy &other);
   ArrayItemProxy &operator =(ArrayItemProxy &&other) noexcept;
   ArrayItemProxy &operator =(const Variant &value);
   ArrayItemProxy &operator =(const NumericVariant &value);
   ArrayItemProxy &operator =(const DoubleVariant &value);
   ArrayItemProxy &operator =(const StringVariant &value);
   ArrayItemProxy &operator =(const BooleanVariant &value);
   ArrayItemProxy &operator =(const ArrayVariant &value);

   ArrayItemProxy &operator =(NumericVariant &&value);
   ArrayItemProxy &operator =(DoubleVariant &&value);
   ArrayItemProxy &operator =(StringVariant &&value);
   ArrayItemProxy &operator =(BooleanVariant &&value);
   ArrayItemProxy &operator =(ArrayVariant &&value);

   ArrayItemProxy &operator =(const char *value);
   ArrayItemProxy &operator =(const char value);
   ArrayItemProxy &operator =(const std::string &value);
   template <typename T,
             typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   ArrayItemProxy &operator =(T value);
   template <size_t arrayLength>
   ArrayItemProxy &operator =(char (&value)[arrayLength]);
   // cast operators
   operator Variant();
   operator NumericVariant();
   operator DoubleVariant();
   operator StringVariant();
   operator BooleanVariant();
   operator ArrayVariant();
   Variant toVariant();
   NumericVariant toNumericVariant();
   DoubleVariant toDoubleVariant();
   StringVariant toStringVariant();
   BooleanVariant toBooleanVariant();
   ArrayVariant toArrayVariant();
   // nest assign
   ArrayItemProxy operator [](vmapi_long index);
   ArrayItemProxy operator [](const std::string &key);
protected:
   bool ensureArrayExistRecusive(zval *&childArrayPtr, const KeyType &childRequestKey,
                                 ArrayItemProxy *mostDerivedProxy);
   void checkExistRecursive(bool &stop, zval *&checkExistRecursive,
                            ArrayItemProxy *mostDerivedProxy, bool quiet = false);
   bool isKeychianOk(bool quiet = false);
   zval *retrieveZvalPtr(bool quiet = false) const;
protected:
   friend bool array_unset(ArrayItemProxy &&arrayItem);
   friend bool array_isset(ArrayItemProxy &&arrayItem);
   VMAPI_DECLARE_PRIVATE(ArrayItemProxy);
   std::shared_ptr<ArrayItemProxyPrivate> m_implPtr;
};

template <typename T, typename Selector>
ArrayItemProxy &ArrayItemProxy::operator =(T value)
{
   return operator =(Variant(value));
}

template <size_t arrayLength>
ArrayItemProxy &ArrayItemProxy::operator =(char (&value)[arrayLength])
{
   return operator =(Variant(value));
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_ARRAY_ITEM_PROXY_H
