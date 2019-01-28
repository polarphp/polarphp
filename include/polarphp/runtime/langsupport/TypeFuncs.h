// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/09.

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_TYPE_FUNCS_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_TYPE_FUNCS_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

PHP_FUNCTION(intval);
PHP_FUNCTION(floatval);
PHP_FUNCTION(strval);
PHP_FUNCTION(boolval);
PHP_FUNCTION(gettype);
PHP_FUNCTION(settype);
PHP_FUNCTION(is_null);
PHP_FUNCTION(is_resource);
PHP_FUNCTION(is_bool);
PHP_FUNCTION(is_int);
PHP_FUNCTION(is_float);
PHP_FUNCTION(is_numeric);
PHP_FUNCTION(is_string);
PHP_FUNCTION(is_array);
PHP_FUNCTION(is_object);
PHP_FUNCTION(is_scalar);
PHP_FUNCTION(is_callable);
PHP_FUNCTION(is_iterable);
PHP_FUNCTION(is_countable);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_TYPE_FUNCS_H
