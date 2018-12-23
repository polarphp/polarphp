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

class VMAPI_DECL_EXPORT BoolVariant final : public Variant
{
public:
   BoolVariant();
   BoolVariant(const BoolVariant &other);
   BoolVariant(BoolVariant &other, bool isRef);
   BoolVariant(BoolVariant &&other) noexcept;
   BoolVariant(const Variant &other);
   BoolVariant(Variant &&other);
   BoolVariant(bool value);
   BoolVariant(zval &other, bool isRef = false);
   BoolVariant(zval &&other, bool isRef = false);
   BoolVariant(zval *other, bool isRef = false);
   virtual bool toBool() const noexcept override;
   operator bool () const override;
   BoolVariant &operator=(const BoolVariant &other);
   BoolVariant &operator=(const Variant &other);
   BoolVariant &operator=(ArrayItemProxy &&other);
   BoolVariant &operator=(bool value);
   virtual ~BoolVariant();
};

VMAPI_DECL_EXPORT bool operator ==(const BoolVariant &lhs, const BoolVariant &rhs);
VMAPI_DECL_EXPORT bool operator !=(const BoolVariant &lhs, const BoolVariant &rhs);

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_DS_BOOLEAN_VARIANT_H
