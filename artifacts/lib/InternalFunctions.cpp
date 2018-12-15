// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/15.

#include "ZendHeaders.h"

#include <cstdarg>
#include <cstdlib>
#include <cstdio>

namespace polar {

//static zend_module_entry * const sg_phpBuiltinExtensions[] = {

//};

POLAR_DECL_EXPORT int php_register_internal_extensions()
{
   //return php_register_extensions(php_builtin_extensions, EXTCOUNT);
}

} // polar
