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

#include "polarphp/vm/ds/DoubleVariant.h"
#include "polarphp/vm/ds/NumericVariant.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"

namespace polar {
namespace vmapi {

DoubleVariant::DoubleVariant()
   : Variant(0.0)
{}

DoubleVariant::DoubleVariant(std::int8_t value)
   : Variant(static_cast<double>(value))
{}

DoubleVariant::DoubleVariant(std::int16_t value)
   : Variant(static_cast<double>(value))
{}

DoubleVariant::DoubleVariant(std::int32_t value)
   : Variant(static_cast<double>(value))
{}

DoubleVariant::DoubleVariant(std::int64_t value)
   : Variant(static_cast<double>(value))
{}

DoubleVariant::DoubleVariant(double value)
   : Variant(value)
{}

DoubleVariant::DoubleVariant(zval &other, bool isRef)
   : DoubleVariant(&other, isRef)
{}

DoubleVariant::DoubleVariant(zval &&other, bool isRef)
   : DoubleVariant(&other, isRef)
{}

DoubleVariant::DoubleVariant(zval *other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (nullptr != other) {
      if ((isRef && (Z_TYPE_P(other) == IS_DOUBLE ||
                     (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_DOUBLE))) ||
          (!isRef && (Z_TYPE_P(other) == IS_REFERENCE && Z_TYPE_P(Z_REFVAL_P(other)) == IS_DOUBLE))) {
         ZVAL_MAKE_REF(other);
         zend_reference *ref = Z_REF_P(other);
         GC_ADDREF(ref);
         ZVAL_REF(self, ref);
      } else {
         ZVAL_DUP(self, other);
         convert_to_double(self);
      }
   } else {
      ZVAL_DOUBLE(self, 0);
   }
}

DoubleVariant::DoubleVariant(const DoubleVariant &other)
   : Variant(other)
{}

DoubleVariant::DoubleVariant(DoubleVariant &other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (!isRef) {
      ZVAL_DOUBLE(self, other.toDouble());
   } else {
      zval *source = other.getUnDerefZvalPtr();
      ZVAL_MAKE_REF(source);
      ZVAL_COPY(self, source);
   }
}

DoubleVariant::DoubleVariant(DoubleVariant &&other) noexcept
   : Variant(std::move(other))
{}

DoubleVariant::DoubleVariant(const Variant &other)
{
   zval *self = getZvalPtr();
   zval *from = const_cast<zval *>(other.getZvalPtr());
   if (other.getType() == Type::Double) {
      ZVAL_DOUBLE(self, zval_get_double(from));
   } else {
      zval temp;
      ZVAL_DUP(&temp, from);
      convert_to_double(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
}

DoubleVariant::DoubleVariant(Variant &&other)
   : Variant(std::move(other))
{
   if (getType() != Type::Double) {
      convert_to_double(getUnDerefZvalPtr());
   }
}

bool DoubleVariant::toBoolean() const noexcept
{
   return toDouble();
}

DoubleVariant::operator double() const
{
   return toDouble();
}

double DoubleVariant::toDouble() const noexcept
{
   return zval_get_double(const_cast<zval *>(getZvalPtr()));
}

DoubleVariant &DoubleVariant::operator =(std::int8_t other)
{
   ZVAL_DOUBLE(getZvalPtr(), static_cast<double>(other));
   return *this;
}

DoubleVariant &DoubleVariant::operator =(std::int16_t other)
{
   ZVAL_DOUBLE(getZvalPtr(), static_cast<double>(other));
   return *this;
}

DoubleVariant &DoubleVariant::operator =(std::int32_t other)
{
   ZVAL_DOUBLE(getZvalPtr(), static_cast<double>(other));
   return *this;
}

DoubleVariant &DoubleVariant::operator =(std::int64_t other)
{
   ZVAL_DOUBLE(getZvalPtr(), static_cast<double>(other));
   return *this;
}

DoubleVariant &DoubleVariant::operator =(double other)
{
   ZVAL_DOUBLE(getZvalPtr(), other);
   return *this;
}

DoubleVariant &DoubleVariant::operator =(const DoubleVariant &other)
{
   assert(this != &other);
   ZVAL_DOUBLE(getZvalPtr(), other.toDouble());
   return *this;
}

DoubleVariant &DoubleVariant::operator =(const Variant &other)
{
   zval *self = getZvalPtr();
   zval *from = const_cast<zval *>(other.getZvalPtr());
   if (other.getType() == Type::Double) {
      ZVAL_DOUBLE(self, zval_get_double(from));
   } else {
      zval temp;
      ZVAL_DUP(&temp, from);
      convert_to_double(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
   return *this;
}

DoubleVariant &DoubleVariant::operator =(const NumericVariant &other)
{
   ZVAL_DOUBLE(getZvalPtr(), static_cast<double>(other.toLong()));
   return *this;
}

DoubleVariant &DoubleVariant::operator =(ArrayItemProxy &&other)
{
   return operator =(other.toDoubleVariant());
}

DoubleVariant::~DoubleVariant() noexcept
{}

DoubleVariant &DoubleVariant::operator +=(const DoubleVariant &value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() + value.toDouble());
   return *this;
}

DoubleVariant &DoubleVariant::operator -=(const DoubleVariant &value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() + value.toDouble());
   return *this;
}

DoubleVariant &DoubleVariant::operator *=(const DoubleVariant &value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() + value.toDouble());
   return *this;
}

DoubleVariant &DoubleVariant::operator /=(const DoubleVariant &value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), toDouble() + value.toDouble());
   return *this;
}

DoubleVariant &DoubleVariant::operator %=(const DoubleVariant &value) noexcept
{
   ZVAL_DOUBLE(getZvalPtr(), std::fmod(toDouble(), value.toDouble()));
   return *this;
}

bool operator ==(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return (lhs.toDouble() - rhs.toDouble()) == 0;
}

bool operator !=(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return (lhs.toDouble() - rhs.toDouble()) != 0;
}

bool operator <(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return (lhs.toDouble() - rhs.toDouble()) < 0;
}

bool operator <=(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return (lhs.toDouble() - rhs.toDouble()) <= 0;
}

bool operator >(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return (lhs.toDouble() - rhs.toDouble()) > 0;
}

bool operator >=(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return (lhs.toDouble() - rhs.toDouble()) >= 0;
}

double operator +(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toDouble() + rhs.toDouble();
}

double operator -(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toDouble() - rhs.toDouble();
}

double operator *(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toDouble() * rhs.toDouble();
}

double operator /(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toDouble() / rhs.toDouble();
}

double operator %(const DoubleVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return std::fmod(lhs.toDouble(), rhs.toDouble());
}

double operator +(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toDouble() + rhs.toLong();
}

double operator -(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toDouble() - rhs.toLong();
}

double operator *(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toDouble() * rhs.toLong();
}

double operator /(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept
{
   return lhs.toDouble() / rhs.toLong();
}

double operator %(const DoubleVariant &lhs, const NumericVariant &rhs) noexcept
{
   return std::fmod(lhs.toDouble(), static_cast<double>(rhs.toLong()));
}

double operator +(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toLong() + rhs.toDouble();
}

double operator -(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toLong() - rhs.toDouble();
}

double operator *(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toLong() * rhs.toDouble();
}

double operator /(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return lhs.toLong() / rhs.toDouble();
}

double operator %(const NumericVariant &lhs, const DoubleVariant &rhs) noexcept
{
   return std::fmod(static_cast<double>(lhs.toLong()), rhs.toDouble());
}

} // vmapi
} // polar
