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

#ifndef POLARPHP_VMAPI_PROTOCOL_ARRAY_ACCESS_H
#define POLARPHP_VMAPI_PROTOCOL_ARRAY_ACCESS_H

#include "polarphp/vm/ZendApi.h"

namespace polar {
namespace vmapi {

/// forward declare class
class Variant;

class VMAPI_DECL_EXPORT ArrayAccess
{
public:
   virtual bool offsetExists(Variant offset) = 0;
   virtual void offsetSet(Variant offset, Variant value) = 0;
   virtual Variant offsetGet(Variant offset) = 0;
   virtual void offsetUnset(Variant offset) = 0;
   virtual ~ArrayAccess() = default;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_PROTOCOL_ARRAY_ACCESS_H
