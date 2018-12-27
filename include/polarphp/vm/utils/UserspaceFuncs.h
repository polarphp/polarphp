// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/27.

#ifndef POLARPHP_VMAPI_UTILS_USERSPACE_FUNCS_H
#define POLARPHP_VMAPI_UTILS_USERSPACE_FUNCS_H

#include "polarphp/vm/ZendApi.h"
#include "polarphp/vm/lang/Ini.h"

namespace polar {
namespace vmapi {

class ArrayItemProxy;

// here we define some php function that can been used in c++ space

VMAPI_DECL_EXPORT inline IniValue ini_get(const char *name)
{
   return IniValue(name, false);
}

VMAPI_DECL_EXPORT inline IniValue ini_get_orig(const char *name)
{
   return IniValue(name, true);
}

VMAPI_DECL_EXPORT bool array_unset(ArrayItemProxy &&arrayItem);
VMAPI_DECL_EXPORT bool array_isset(ArrayItemProxy &&arrayItem);
VMAPI_DECL_EXPORT bool empty(const Variant &value);

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_USERSPACE_FUNCS_H
