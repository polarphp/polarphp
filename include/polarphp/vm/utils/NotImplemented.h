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

#ifndef POLARPHP_VMAPI_UTILS_NOT_IMPLEMENTED_H
#define POLARPHP_VMAPI_UTILS_NOT_IMPLEMENTED_H

#include "polarphp/vm/utils/Exception.h"

#include <exception>

namespace polar {
namespace vmapi {

class VMAPI_DECL_EXPORT NotImplemented : public std::exception
{
public:
   NotImplemented();
   virtual ~NotImplemented() noexcept;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_NOT_IMPLEMENTED_H
