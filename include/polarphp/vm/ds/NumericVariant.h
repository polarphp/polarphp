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

#ifndef POLARPHP_VMAPI_DS_NUMERIC_VARIANT_H
#define POLARPHP_VMAPI_DS_NUMERIC_VARIANT_H

#include "polarphp/vm/ds/Variant.h"
#include <type_traits>

namespace polar {
namespace vmapi {

class DoubleVariant;
class ArrayItemProxy;

class VMAPI_DECL_EXPORT NumericVariant final : public Variant
{
public:
   NumericVariant();
   NumericVariant(std::int8_t value);
   NumericVariant(std::int16_t value);
   NumericVariant(std::int32_t value);
#if SIZEOF_ZEND_LONG == 8
   NumericVariant(std::int64_t value);
#endif
   NumericVariant(zval &other, bool isRef = false);
   NumericVariant(zval &&other, bool isRef = false);
   NumericVariant(zval *other, bool isRef = false);
   NumericVariant(const NumericVariant &other);
   NumericVariant(NumericVariant &other, bool isRef);
   NumericVariant(NumericVariant &&other) noexcept;
   NumericVariant(const Variant &other);
   NumericVariant(Variant &&other);
   NumericVariant &operator =(std::int8_t other);
   NumericVariant &operator =(std::int16_t other);
   NumericVariant &operator =(std::int32_t other);
   NumericVariant &operator =(std::int64_t other);
   NumericVariant &operator =(double other);
   NumericVariant &operator =(const NumericVariant &other);
   NumericVariant &operator =(const Variant &other);
   NumericVariant &operator =(const DoubleVariant &other);
   NumericVariant &operator =(ArrayItemProxy &&other);
   template <typename T,
             typename Selector = typename std::enable_if<std::is_integral<T>::value>::type>
   operator T () const;
   NumericVariant &operator++();
   NumericVariant operator++(int);
   NumericVariant &operator--();
   NumericVariant operator--(int);
   template <typename T,
             typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
   NumericVariant &operator +=(T value) noexcept;
   NumericVariant &operator +=(double value) noexcept;
   NumericVariant &operator +=(const NumericVariant &value) noexcept;

   template <typename T,
             typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
   NumericVariant &operator -=(T value) noexcept;
   NumericVariant &operator -=(double value) noexcept;
   NumericVariant &operator -=(const NumericVariant &value) noexcept;

   template <typename T,
             typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
   NumericVariant &operator *=(T value) noexcept;
   NumericVariant &operator *=(double value) noexcept;
   NumericVariant &operator *=(const NumericVariant &value) noexcept;

   template <typename T,
             typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
   NumericVariant &operator /=(T value) noexcept;
   NumericVariant &operator /=(double value) noexcept;
   NumericVariant &operator /=(const NumericVariant &value) noexcept;

   template <typename T,
             typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
   NumericVariant &operator %=(T value) noexcept;
   NumericVariant &operator %=(const NumericVariant &value) noexcept;

   virtual bool toBoolean() const noexcept override;
   vmapi_long toLong() const noexcept;
   virtual ~NumericVariant() noexcept;
};

template <typename T,
          typename Selector>
NumericVariant::operator T () const
{
   return zval_get_long(const_cast<zval *>(getZvalPtr()));
}

template <typename T, typename Selector>
NumericVariant &NumericVariant::operator +=(T value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() + static_cast<vmapi_long>(value));
   return *this;
}

template <typename T, typename Selector>
NumericVariant &NumericVariant::operator -=(T value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() - static_cast<vmapi_long>(value));
   return *this;
}

template <typename T, typename Selector>
NumericVariant &NumericVariant::operator *=(T value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() * static_cast<vmapi_long>(value));
   return *this;
}

template <typename T, typename Selector>
NumericVariant &NumericVariant::operator /=(T value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() / static_cast<vmapi_long>(value));
   return *this;
}

template <typename T, typename Selector>
NumericVariant &NumericVariant::operator %=(T value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() % static_cast<vmapi_long>(value));
   return *this;
}

VMAPI_DECL_EXPORT bool operator ==(const NumericVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator !=(const NumericVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <(const NumericVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <=(const NumericVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >(const NumericVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >=(const NumericVariant &lhs, const NumericVariant &rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator ==(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() == static_cast<vmapi_long>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator !=(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() != static_cast<vmapi_long>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() < static_cast<vmapi_long>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <=(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() <= static_cast<vmapi_long>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() > static_cast<vmapi_long>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >=(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() >= static_cast<vmapi_long>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator ==(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) == rhs.toLong();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator !=(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) != rhs.toLong();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) < rhs.toLong();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <=(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) <= rhs.toLong();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) > rhs.toLong();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >=(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) >= rhs.toLong();
}

VMAPI_DECL_EXPORT bool operator ==(double lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator !=(double lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <(double lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <=(double lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >(double lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >=(double lhs, const NumericVariant &rhs) noexcept;

VMAPI_DECL_EXPORT bool operator ==(const NumericVariant &lhs, double rhs) noexcept;
VMAPI_DECL_EXPORT bool operator !=(const NumericVariant &lhs, double rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <(const NumericVariant &lhs, double rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <=(const NumericVariant &lhs, double rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >(const NumericVariant &lhs, double rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >=(const NumericVariant &lhs, double rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator +(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) + rhs.toLong();
}

VMAPI_DECL_EXPORT double operator +(double lhs, const NumericVariant &rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator -(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) - rhs.toLong();
}

VMAPI_DECL_EXPORT double operator -(double lhs, const NumericVariant &rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator *(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) * rhs.toLong();
}

VMAPI_DECL_EXPORT double operator *(double lhs, const NumericVariant &rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator /(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) / rhs.toLong();
}

VMAPI_DECL_EXPORT double operator/(double lhs, const NumericVariant &rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator %(T lhs, const NumericVariant &rhs) noexcept
{
   return static_cast<vmapi_long>(lhs) % rhs.toLong();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator +(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() + static_cast<vmapi_long>(rhs);
}

VMAPI_DECL_EXPORT double operator +(const NumericVariant &lhs, double rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator -(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() - static_cast<vmapi_long>(rhs);
}

VMAPI_DECL_EXPORT double operator -(const NumericVariant &lhs, double rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator *(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() * static_cast<vmapi_long>(rhs);
}

VMAPI_DECL_EXPORT double operator *(const NumericVariant &lhs, double rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator /(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() / static_cast<vmapi_long>(rhs);
}

VMAPI_DECL_EXPORT double operator /(const NumericVariant &lhs, double rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_integral<T>::value && std::is_signed<T>::value>::type>
VMAPI_DECL_EXPORT vmapi_long operator %(const NumericVariant &lhs, T rhs) noexcept
{
   return lhs.toLong() % static_cast<vmapi_long>(rhs);
}

VMAPI_DECL_EXPORT vmapi_long operator +(const NumericVariant &lhs, NumericVariant rhs) noexcept;
VMAPI_DECL_EXPORT vmapi_long operator -(const NumericVariant &lhs, NumericVariant rhs) noexcept;
VMAPI_DECL_EXPORT vmapi_long operator *(const NumericVariant &lhs, NumericVariant rhs) noexcept;
VMAPI_DECL_EXPORT vmapi_long operator /(const NumericVariant &lhs, NumericVariant rhs) noexcept;
VMAPI_DECL_EXPORT vmapi_long operator %(const NumericVariant &lhs, NumericVariant rhs) noexcept;

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_NUMERIC_VARIANT_H
