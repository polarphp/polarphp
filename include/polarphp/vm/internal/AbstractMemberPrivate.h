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

#ifndef POLARPHP_VMAPI_INTERNAL_ABSTRACT_MEMBER_PRIVATE_H
#define POLARPHP_VMAPI_INTERNAL_ABSTRACT_MEMBER_PRIVATE_H

#include "polarphp/vm/ZendApi.h"

#include <string>

namespace polar {
namespace vmapi {
namespace internal
{
class AbstractMemberPrivate
{
public:
   AbstractMemberPrivate(std::string name, Modifier flags)
      : m_flags(flags),
        m_name(name)
   {}

   Modifier m_flags;
   std::string m_name;
};

} // internal
} // vmapi
} // polar

#endif // POLARPHP_VMAPI_INTERNAL_ABSTRACT_MEMBER_PRIVATE_H
