// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/24.

#ifndef POLARPHP_VMAPI_NUMERIC_MEMBER_H
#define POLARPHP_VMAPI_NUMERIC_MEMBER_H

#include "polarphp/vm/AbstractMember.h"

namespace polar {
namespace vmapi {

/// forward declare with namespace
namespace internal
{
class NumericMemberPrivate;
} // internal

using internal::NumericMemberPrivate;

class NumericMember : public AbstractMember
{
public:
   NumericMember(StringRef name, double value, Modifier flags);
   NumericMember(const NumericMember &other);
   NumericMember &operator=(const NumericMember &other);
   virtual ~NumericMember();
protected:
   virtual void setupConstant(zend_class_entry *entry) override;
   virtual void setupProperty(zend_class_entry *entry) override;
   VMAPI_DECLARE_PRIVATE(NumericMember);
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_NUMERIC_MEMBER_H
