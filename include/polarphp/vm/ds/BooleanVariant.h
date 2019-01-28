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

#ifndef POLARPHP_VMAPI_DS_BOOLEAN_VARIANT_H
#define POLARPHP_VMAPI_DS_BOOLEAN_VARIANT_H

#include "polarphp/vm/ds/Variant.h"

namespace polar {
namespace vmapi {

class ArrayItemProxy;

class VMAPI_DECL_EXPORT BooleanVariant final : public Variant
{
public:
   BooleanVariant();
   BooleanVariant(const BooleanVariant &other);
   BooleanVariant(BooleanVariant &other, bool isRef);
   BooleanVariant(BooleanVariant &&other) noexcept;
   BooleanVariant(const Variant &other);
   BooleanVariant(Variant &&other);
   BooleanVariant(bool value);
   BooleanVariant(zval &other, bool isRef = false);
   BooleanVariant(zval &&other, bool isRef = false);
   BooleanVariant(zval *other, bool isRef = false);
   virtual bool toBoolean() const noexcept override;
   operator bool () const override;
   BooleanVariant &operator=(const BooleanVariant &other);
   BooleanVariant &operator=(const Variant &other);
   BooleanVariant &operator=(ArrayItemProxy &&other);
   BooleanVariant &operator=(bool value);
   virtual ~BooleanVariant();
};

VMAPI_DECL_EXPORT bool operator ==(const BooleanVariant &lhs, const BooleanVariant &rhs);
VMAPI_DECL_EXPORT bool operator !=(const BooleanVariant &lhs, const BooleanVariant &rhs);

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_BOOLEAN_VARIANT_H
