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

#ifndef POLARPHP_VMAPI_LANG_INTERFACE_H
#define POLARPHP_VMAPI_LANG_INTERFACE_H

#include "polarphp/vm/AbstractClass.h"

namespace polar {
namespace vmapi {

class VMAPI_DECL_EXPORT Interface final : public AbstractClass
{
public:
   Interface(const char *name);
   virtual ~Interface();

public:
   Interface &registerMethod(const char *name, const Arguments args = {});
   Interface &registerMethod(const char *name, Modifier flags, const Arguments args = {});
   Interface &registerBaseInterface(const Interface &interface);

protected:
   template<typename ANYTHING> friend class Class;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_LANG_INTERFACE_H
