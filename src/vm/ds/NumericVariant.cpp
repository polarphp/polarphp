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

#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/DoubleVariant.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"
#include <cmath>

namespace polar {
namespace vmapi {

NumericVariant::NumericVariant()
   : Variant(0)
{}

NumericVariant::NumericVariant(std::int8_t value)
   : Variant(value)
{}

NumericVariant::NumericVariant(std::int16_t value)
   : Variant(value)
{}

NumericVariant::NumericVariant(std::int32_t value)
   : Variant(value)
{}

#if SIZEOF_ZEND_LONG == 8
NumericVariant::NumericVariant(std::int64_t value)
   : Variant(value)
{}
#endif

NumericVariant::NumericVariant(zval &other, bool isRef)
   : NumericVariant(&other, isRef)
{}

NumericVariant::NumericVariant(zval &&other, bool isRef)
   : NumericVariant(&other, isRef)
{}

NumericVariant::NumericVariant(zval *other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (nullptr != other) {
      if ((isRef && (Z_TYPE_P(other) == IS_LONG ||
                    (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_LONG))) ||
          (!isRef && (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_LONG))) {
         // for support pass ref arg when pass variant args
         ZVAL_MAKE_REF(other);
         zend_reference *ref = Z_REF_P(other);
         GC_ADDREF(ref);
         ZVAL_REF(self, ref);
      } else {
         // here the zval of other pointer maybe not initialize
         ZVAL_DUP(self, other);
         convert_to_long(self);
      }
   } else {
      ZVAL_LONG(self, 0);
   }
}

NumericVariant::NumericVariant(const NumericVariant &other)
   : Variant(other)
{}

NumericVariant::NumericVariant(NumericVariant &other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (!isRef) {
      ZVAL_LONG(self, other.toLong());
   } else {
      zval *source = other.getUnDerefZvalPtr();
      ZVAL_MAKE_REF(source);
      ZVAL_COPY(self, source);
   }
}

NumericVariant::NumericVariant(NumericVariant &&other) noexcept
   : Variant(std::move(other))
{}

NumericVariant::NumericVariant(const Variant &other)
{
   zval *from = const_cast<zval *>(other.getZvalPtr());
   zval *self = getZvalPtr();
   if (other.getType() == Type::Numeric) {
      ZVAL_LONG(self, zval_get_long(from));
   } else {
      zval temp;
      ZVAL_DUP(&temp, from);
      convert_to_long(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
}

NumericVariant::NumericVariant(Variant &&other)
   : Variant(std::move(other))
{
   if (getType() != Type::Long) {
      convert_to_long(getUnDerefZvalPtr());
   }
}

//NumericVariant::operator vmapi_long () const
//{
//   return zval_get_long(const_cast<zval *>(getZvalPtr()));
//}

//NumericVariant::operator int () const
//{
//   return zval_get_long(const_cast<zval *>(getZvalPtr()));
//}

vmapi_long NumericVariant::toLong() const noexcept
{
   return zval_get_long(const_cast<zval *>(getZvalPtr()));
}

bool NumericVariant::toBoolean() const noexcept
{
   return zval_get_long(const_cast<zval *>(getZvalPtr()));
}

NumericVariant &NumericVariant::operator =(std::int8_t other)
{
   ZVAL_LONG(getZvalPtr(), static_cast<vmapi_long>(other));
   return *this;
}

NumericVariant &NumericVariant::operator =(std::int16_t other)
{
   ZVAL_LONG(getZvalPtr(), static_cast<vmapi_long>(other));
   return *this;
}

NumericVariant &NumericVariant::operator =(std::int32_t other)
{
   ZVAL_LONG(getZvalPtr(), static_cast<vmapi_long>(other));
   return *this;
}

NumericVariant &NumericVariant::operator =(std::int64_t other)
{
   ZVAL_LONG(getZvalPtr(), static_cast<vmapi_long>(other));
   return *this;
}

NumericVariant &NumericVariant::operator =(double other)
{
   ZVAL_LONG(getZvalPtr(), static_cast<vmapi_long>(other));
   return *this;
}

NumericVariant &NumericVariant::operator =(const NumericVariant &other)
{
   if (this != &other) {
      ZVAL_LONG(getZvalPtr(), other.toLong());
   }
   return *this;
}

NumericVariant &NumericVariant::operator =(const DoubleVariant &other)
{
   ZVAL_LONG(getZvalPtr(), static_cast<vmapi_long>(other.toDouble()));
   return *this;
}

NumericVariant &NumericVariant::operator =(ArrayItemProxy &&other)
{
   return operator =(other.toNumericVariant());
}

NumericVariant &NumericVariant::operator =(const Variant &other)
{
   zval *self = getZvalPtr();
   zval *from = const_cast<zval *>(other.getZvalPtr());
   if (other.getType() == Type::Long) {
      ZVAL_LONG(self, Z_LVAL_P(from));
   } else {
      zval temp;
      ZVAL_DUP(&temp, from);
      convert_to_long(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
   return *this;
}

NumericVariant &NumericVariant::operator ++()
{
   ZVAL_LONG(getZvalPtr(), toLong() + 1);
   return *this;
}

NumericVariant NumericVariant::operator ++(int)
{
   NumericVariant ret(toLong());
   ZVAL_LONG(getZvalPtr(), toLong() + 1);
   return ret;
}

NumericVariant &NumericVariant::operator --()
{
   ZVAL_LONG(getZvalPtr(), toLong() - 1);
   return *this;
}

NumericVariant NumericVariant::operator --(int)
{
   NumericVariant ret(toLong());
   ZVAL_LONG(getZvalPtr(), toLong() - 1);
   return ret;
}

NumericVariant &NumericVariant::operator +=(double value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() + std::lround(value));
   return *this;
}

NumericVariant &NumericVariant::operator +=(const NumericVariant &value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() + value.toLong());
   return *this;
}

NumericVariant &NumericVariant::operator -=(double value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() - std::lround(value));
   return *this;
}

NumericVariant &NumericVariant::operator -=(const NumericVariant &value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() - value.toLong());
   return *this;
}

NumericVariant &NumericVariant::operator *=(double value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() * std::lround(value));
   return *this;
}

NumericVariant &NumericVariant::operator *=(const NumericVariant &value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() * value.toLong());
   return *this;
}

NumericVariant &NumericVariant::operator /=(double value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() / std::lround(value));
   return *this;
}

NumericVariant &NumericVariant::operator /=(const NumericVariant &value) noexcept
{
   ZVAL_LONG(getZvalPtr(), toLong() / value.toLong());
   return *this;
}

NumericVariant::~NumericVariant() noexcept
{}

bool operator ==(const NumericVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toLong() == rhs.toLong();
}

bool operator !=(const NumericVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toLong() != rhs.toLong();
}

bool operator <(const NumericVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toLong() < rhs.toLong();
}

bool operator <=(const NumericVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toLong() <= rhs.toLong();
}

bool operator >(const NumericVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toLong() > rhs.toLong();
}

bool operator >=(const NumericVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toLong() >= rhs.toLong();
}

bool operator ==(double lhs, const NumericVariant &rhs) noexcept
{
   return (lhs - static_cast<double>(rhs.toLong())) == 0;
}

bool operator !=(double lhs, const NumericVariant &rhs) noexcept
{
   return (lhs - static_cast<double>(rhs.toLong())) != 0;
}

bool operator <(double lhs, const NumericVariant &rhs) noexcept
{
   return (lhs - static_cast<double>(rhs.toLong())) < 0;
}

bool operator <=(double lhs, const NumericVariant &rhs) noexcept
{
   return (lhs - static_cast<double>(rhs.toLong())) <= 0;
}

bool operator >(double lhs, const NumericVariant &rhs) noexcept
{
   return (lhs - static_cast<double>(rhs.toLong())) > 0;
}

bool operator >=(double lhs, const NumericVariant &rhs) noexcept
{
   return (lhs - static_cast<double>(rhs.toLong())) >= 0;
}

bool operator ==(const NumericVariant &lhs, double rhs) noexcept
{
   return (static_cast<double>(lhs.toLong()) - rhs) == 0;
}

bool operator !=(const NumericVariant &lhs, double rhs) noexcept
{
   return (static_cast<double>(lhs.toLong()) - rhs) != 0;
}

bool operator <(const NumericVariant &lhs, double rhs) noexcept
{
   return (static_cast<double>(lhs.toLong()) - rhs) < 0;
}

bool operator <=(const NumericVariant &lhs, double rhs) noexcept
{
   return (static_cast<double>(lhs.toLong()) - rhs) <= 0;
}

bool operator >(const NumericVariant &lhs, double rhs) noexcept
{
   return (static_cast<double>(lhs.toLong()) - rhs) > 0;
}

bool operator >=(const NumericVariant &lhs, double rhs) noexcept
{
   return (static_cast<double>(lhs.toLong()) - rhs) >= 0;
}

double operator +(double lhs, const NumericVariant &rhs) noexcept
{
   return lhs + rhs.toLong();
}

double operator -(double lhs, const NumericVariant &rhs) noexcept
{
   return lhs - rhs.toLong();
}

double operator *(double lhs, const NumericVariant &rhs) noexcept
{
   return lhs * rhs.toLong();
}

double operator /(double lhs, const NumericVariant &rhs) noexcept
{
   return lhs / rhs.toLong();
}

double operator +(const NumericVariant &lhs, double rhs) noexcept
{
   return lhs.toLong() + rhs;
}

double operator -(const NumericVariant &lhs, double rhs) noexcept
{
   return lhs.toLong() - rhs;
}

double operator *(const NumericVariant &lhs, double rhs) noexcept
{
   return lhs.toLong() * rhs;
}

double operator /(const NumericVariant &lhs, double rhs) noexcept
{
   return lhs.toLong() / rhs;
}

vmapi_long operator +(const NumericVariant &lhs, NumericVariant rhs) noexcept
{
   return lhs.toLong() + rhs.toLong();
}

vmapi_long operator -(const NumericVariant &lhs, NumericVariant rhs) noexcept
{
   return lhs.toLong() + rhs.toLong();
}

vmapi_long operator *(const NumericVariant &lhs, NumericVariant rhs) noexcept
{
   return lhs.toLong() + rhs.toLong();
}

vmapi_long operator /(const NumericVariant &lhs, NumericVariant rhs) noexcept
{
   return lhs.toLong() + rhs.toLong();
}

vmapi_long operator %(const NumericVariant &lhs, NumericVariant rhs) noexcept
{
   return lhs.toLong() + rhs.toLong();
}

} // vmapi
} // polar
