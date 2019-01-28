// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/25.

/*
   +----------------------------------------------------------------------+
   | PHP Version 7                                                        |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997-2018 The PHP Group                                |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Author: Marcus Boerger <helly@php.net>                               |
   +----------------------------------------------------------------------+
 */

#ifndef POLARPHP_RUNTIME_PHP_SPPRINTF_H
#define POLARPHP_RUNTIME_PHP_SPPRINTF_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/vm/zend/zend_smart_str_public.h"
#include "polarphp/vm/zend/zend_smart_string_public.h"
#include <cstdarg>

#define polar_spprintf zend_spprintf
#define polar_strpprintf zend_strpprintf
#define polar_vspprintf zend_vspprintf
#define polar_vstrpprintf zend_vstrpprintf

namespace polar {
namespace runtime {

POLAR_DECL_EXPORT void php_printf_to_smart_string(smart_string *buf, const char *format, va_list ap);
POLAR_DECL_EXPORT void php_printf_to_smart_str(smart_str *buf, const char *format, va_list ap);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_PHP_SPPRINTF_H
