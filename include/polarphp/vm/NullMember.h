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

#ifndef POLARPHP_VMAPI_NULL_MEMBER_H
#define POLARPHP_VMAPI_NULL_MEMBER_H

#include "polarphp/vm/AbstractMember.h"

namespace polar {
namespace vmapi {

class NullMember : public AbstractMember
{
public:
   NullMember(StringRef name, Modifier flags);
   virtual ~NullMember();
protected:
   virtual void setupConstant(zend_class_entry *entry);
   virtual void setupProperty(zend_class_entry *entry);
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_NULL_MEMBER_H
