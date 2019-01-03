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

#ifndef POLARPHP_VMAPI_DS_DOUBLE_VARIANT_H
#define POLARPHP_VMAPI_DS_DOUBLE_VARIANT_H

#include "polarphp/vm/ds/Variant.h"

#include <cmath>

namespace polar {
namespace vmapi {

// forward declare
class NumericVariant;
class ArrayItemProxy;

class VMAPI_DECL_EXPORT DoubleVariant final : public Variant
{
public:
   DoubleVariant();
   DoubleVariant(std::int8_t value);
   DoubleVariant(std::int16_t value);
   DoubleVariant(std::int32_t value);
   DoubleVariant(std::int64_t value);
   DoubleVariant(double value);
   DoubleVariant(zval &other, bool isRef = false);
   DoubleVariant(zval &&other, bool isRef = false);
   DoubleVariant(zval *other, bool isRef = false);
   DoubleVariant(const DoubleVariant &other);
   DoubleVariant(DoubleVariant &other, bool isRef);
   DoubleVariant(DoubleVariant &&other) noexcept;
   DoubleVariant(const Variant &other);
   DoubleVariant(Variant &&other);
   virtual bool toBoolean() const noexcept override;
   double toDouble() const noexcept;
   operator double() const;
   DoubleVariant &operator =(std::int8_t other);
   DoubleVariant &operator =(std::int16_t other);
   DoubleVariant &operator =(std::int32_t other);
   DoubleVariant &operator =(std::int64_t other);
   DoubleVariant &operator =(double other);
   DoubleVariant &operator =(const DoubleVariant &other);
   DoubleVariant &operator =(const Variant &other);
   DoubleVariant &operator =(const NumericVariant &other);
   DoubleVariant &operator =(ArrayItemProxy &&other);
   template <typename T, typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   DoubleVariant &operator +=(T value) noexcept;
   DoubleVariant &operator +=(const DoubleVariant &value) noexcept;
   template <typename T, typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   DoubleVariant &operator -=(T value) noexcept;
   DoubleVariant &operator -=(const DoubleVariant &value) noexcept;
   template <typename T, typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   DoubleVariant &operator *=(T value) noexcept;
   DoubleVariant &operator *=(const DoubleVariant &value) noexcept;
   template <typename T, typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   DoubleVariant &operator /=(T value) noexcept;
   DoubleVariant &operator /=(const DoubleVariant &value) noexcept;
   template <typename T, typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
   DoubleVariant &operator %=(T value) noexcept;
   DoubleVariant &operator %=(const DoubleVariant &value) noexcept;

   virtual ~DoubleVariant() noexcept;
};

// member template operators

template <typename T, typename Selector>
DoubleVariant &DoubleVariant::operator +=(T value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() + value);
   return *this;
}

template <typename T, typename Selector>
DoubleVariant &DoubleVariant::operator -=(T value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() - value);
   return *this;
}

template <typename T, typename Selector>
DoubleVariant &DoubleVariant::operator *=(T value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() * value);
   return *this;
}

template <typename T, typename Selector>
DoubleVariant &DoubleVariant::operator /=(T value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() / value);
   return *this;
}

template <typename T, typename Selector>
DoubleVariant &DoubleVariant::operator %=(T value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), std::fmod(toDouble(), static_cast<double>(value)));
   return *this;
}

// template compare operators
template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator ==(T lhs, const DoubleVariant &rhs) noexcept
{
   return (static_cast<T>(lhs) - rhs.toDouble()) == 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator !=(T lhs, const DoubleVariant &rhs) noexcept
{
   return (static_cast<T>(lhs) - rhs.toDouble()) != 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <(T lhs, const DoubleVariant &rhs) noexcept
{
   return (static_cast<T>(lhs) - rhs.toDouble()) < 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <=(T lhs, const DoubleVariant &rhs) noexcept
{
   return (static_cast<T>(lhs) - rhs.toDouble()) <= 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >(T lhs, const DoubleVariant &rhs) noexcept
{
   return (static_cast<T>(lhs) - rhs.toDouble()) > 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >=(T lhs, const DoubleVariant &rhs) noexcept
{
   return (static_cast<T>(lhs) - rhs.toDouble()) >= 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator ==(const DoubleVariant &lhs, T rhs) noexcept
{
   return (lhs.toDouble() - static_cast<T>(rhs)) == 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator !=(const DoubleVariant &lhs, T rhs) noexcept
{
   return (lhs.toDouble() - static_cast<T>(rhs)) != 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <(const DoubleVariant &lhs, T rhs) noexcept
{
   return (lhs.toDouble() - static_cast<T>(rhs)) < 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator <=(const DoubleVariant &lhs, T rhs) noexcept
{
   return (lhs.toDouble() - static_cast<T>(rhs)) <= 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >(const DoubleVariant &lhs, T rhs) noexcept
{
   return (lhs.toDouble() - static_cast<T>(rhs)) > 0;
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT bool operator >=(const DoubleVariant &lhs, T rhs) noexcept
{
   return (lhs.toDouble() - static_cast<T>(rhs)) >= 0;
}

VMAPI_DECL_EXPORT bool operator ==(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator !=(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator <=(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT bool operator >=(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;

VMAPI_DECL_EXPORT double operator +(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator -(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator *(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator /(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator %(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept;

VMAPI_DECL_EXPORT double operator +(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator -(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator *(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator /(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator %(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept;

VMAPI_DECL_EXPORT double operator +(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator -(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator *(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator /(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept;
VMAPI_DECL_EXPORT double operator %(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept;

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator +(T lhs, const DoubleVariant &rhs) noexcept
{
   return static_cast<double>(lhs) + rhs.toDouble();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator -(T lhs, const DoubleVariant &rhs) noexcept
{
   return static_cast<double>(lhs) - rhs.toDouble();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator *(T lhs, const DoubleVariant &rhs) noexcept
{
   return static_cast<double>(lhs) * rhs.toDouble();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator /(T lhs, const DoubleVariant &rhs) noexcept
{
   return static_cast<double>(lhs) / rhs.toDouble();
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator %(T lhs, const DoubleVariant &rhs) noexcept
{
   return std::fmod(static_cast<double>(lhs), rhs.toDouble());
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator +(const DoubleVariant &lhs, T rhs) noexcept
{
   return lhs.toDouble() + static_cast<double>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator -(const DoubleVariant &lhs, T rhs) noexcept
{
   return lhs.toDouble() - static_cast<double>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator *(const DoubleVariant &lhs, T rhs) noexcept
{
   return lhs.toDouble() * static_cast<double>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator /(const DoubleVariant &lhs, T rhs) noexcept
{
   return lhs.toDouble() / static_cast<double>(rhs);
}

template <typename T,
          typename Selector = typename std::enable_if<std::is_arithmetic<T>::value>::type>
VMAPI_DECL_EXPORT double operator %(const DoubleVariant &lhs, T rhs) noexcept
{
   return std::fmod(lhs.toDouble(), static_cast<double>(rhs));
}

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_DOUBLE_VARIANT_H
