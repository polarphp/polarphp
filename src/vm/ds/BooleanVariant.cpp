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

#include "polarphp/vm/ds/BooleanVariant.h"
#include "polarphp/vm/ds/ArrayItemProxy.h"

namespace polar {
namespace vmapi {

using internal::VariantPrivate;

BoolVariant::BoolVariant()
   : Variant(false)
{}

BoolVariant::BoolVariant(bool value)
   : Variant(value)
{}

BoolVariant::BoolVariant(zval &other, bool isRef)
   : BoolVariant(&other, isRef)
{}

BoolVariant::BoolVariant(zval &&other, bool isRef)
   : BoolVariant(&other, isRef)
{}

BoolVariant::BoolVariant(zval *other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (nullptr != other) {
      if ((isRef && ((Z_TYPE_P(other) == IS_TRUE || Z_TYPE_P(other) == IS_FALSE) ||
                     (Z_TYPE_P(other) == IS_REFERENCE &&
                      (Z_TYPE_P(Z_REFVAL_P(other)) == IS_TRUE || Z_TYPE_P(Z_REFVAL_P(other)) == IS_FALSE)))) ||
          (!isRef && (Z_TYPE_P(other) == IS_REFERENCE &&
                      (Z_TYPE_P(Z_REFVAL_P(other)) == IS_TRUE || Z_TYPE_P(Z_REFVAL_P(other)) == IS_FALSE)))) {
         ZVAL_MAKE_REF(other);
         zend_reference *ref = Z_REF_P(other);
         ++GC_REFCOUNT(ref);
         ZVAL_REF(self, ref);
      } else {
         ZVAL_DUP(self, other);
         convert_to_boolean(self);
      }
   } else {
      ZVAL_BOOL(self, false);
   }
}

BoolVariant::BoolVariant(const BoolVariant &other)
   : Variant(other)
{}

BoolVariant::BoolVariant(BoolVariant &other, bool isRef)
{
   zval *self = getUnDerefZvalPtr();
   if (!isRef) {
      ZVAL_BOOL(self, Z_TYPE_INFO_P(other.getZvalPtr()) == IS_TRUE);
   } else {
      zval *source = other.getUnDerefZvalPtr();
      ZVAL_MAKE_REF(source);
      ZVAL_COPY(self, source);
   }
}

BoolVariant::BoolVariant(BoolVariant &&other) noexcept
   : Variant(std::move(other))
{}

BoolVariant::BoolVariant(const Variant &other)
{
   zval *self = getZvalPtr();
   zval *from = const_cast<zval *>(other.getZvalPtr());
   if (other.getType() == Type::True || other.getType() == Type::False) {
      ZVAL_BOOL(self, Z_TYPE_INFO_P(from) == IS_TRUE);
   } else {
      zval temp;
      ZVAL_DUP(&temp, from);
      convert_to_boolean(&temp);
      ZVAL_COPY_VALUE(self, &temp);
   }
}

BoolVariant::BoolVariant(Variant &&other)
   : Variant(std::move(other))
{
   if (getType() != Type::Boolean) {
      convert_to_boolean(getUnDerefZvalPtr());
   }
}

BoolVariant &BoolVariant::operator=(const BoolVariant &other)
{
   if (this != &other) {
      ZVAL_BOOL(getZvalPtr(), Z_TYPE_INFO_P(other.getZvalPtr()) == IS_TRUE);
   }
   return *this;
}

BoolVariant &BoolVariant::operator=(const Variant &other)
{
   ZVAL_BOOL(getZvalPtr(), other.toBool());
   return *this;
}

BoolVariant &BoolVariant::operator=(ArrayItemProxy &&other)
{
   return operator =(other.toBoolVariant());
}

BoolVariant &BoolVariant::operator=(bool value)
{
   ZVAL_BOOL(getZvalPtr(), value);
   return *this;
}

BoolVariant::~BoolVariant()
{}

BoolVariant::operator bool() const
{
   return toBool();
}

bool BoolVariant::toBool() const noexcept
{
   return Z_TYPE_INFO_P(getZvalPtr()) == IS_TRUE ? true : false;
}

bool operator ==(const BoolVariant &lhs, const BoolVariant &rhs)
{
   return lhs.toBool() == rhs.toBool();
}

bool operator !=(const BoolVariant &lhs, const BoolVariant &rhs)
{
   return lhs.toBool() != rhs.toBool();
}

} // vmapi
} // polar
