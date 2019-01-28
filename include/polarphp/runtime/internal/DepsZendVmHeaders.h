// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/13.

#ifndef POLARPHP_RUNTIME_INTERNAL_DEPS_ZENDVM_HEADERS_H
#define POLARPHP_RUNTIME_INTERNAL_DEPS_ZENDVM_HEADERS_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/SystemDetection.h"

POLAR_BEGIN_DISABLE_ZENDVM_WARNING()

extern "C" {
#include "polarphp/vm/zend/zend.h"
#include "polarphp/vm/zend/zend_API.h"
#include "polarphp/vm/zend/zend_config.h"
#include "polarphp/vm/zend/zend_compile.h"
#include "polarphp/vm/zend/zend_execute.h"
#include "polarphp/vm/zend/zend_constants.h"
#include "polarphp/vm/zend/zend_modules.h"
#include "polarphp/vm/zend/zend_highlight.h"
#include "polarphp/vm/zend/zend_exceptions.h"
#include "polarphp/vm/zend/zend_extensions.h"
#include "polarphp/vm/zend/zend_ini_scanner.h"
#include "polarphp/vm/zend/zend_ini.h"
#include "polarphp/vm/zend/zend_bitset.h"
#include "polarphp/vm/zend/zend_interfaces.h"
#include "polarphp/vm/zend/zend_dtrace.h"
#include "polarphp/vm/zend/zend_long.h"
#include "polarphp/vm/zend/zend_portability.h"
#include "polarphp/vm/zend/zend_hash.h"
#include "polarphp/vm/zend/zend_sort.h"
#include "polarphp/vm/zend/zend_alloc.h"
#include "polarphp/vm/zend/zend_stack.h"
#include "polarphp/vm/zend/zend_virtual_cwd.h"
#include "polarphp/vm/zend/zend_smart_str_public.h"
#include "polarphp/vm/zend/zend_smart_string_public.h"
#include "polarphp/vm/zend/zend_smart_string.h"
#include "polarphp/vm/zend/zend_smart_str.h"
#include "polarphp/vm/zend/zend_operators.h"
#include "polarphp/vm/zend/zend_closures.h"
#include "polarphp/vm/zend/zend_generators.h"
#include "polarphp/vm/zend/zend_builtin_functions.h"
#include "polarphp/vm/tsrm/tsrm.h"
#include "polarphp/global/php_stdint.h"

#ifdef POLAR_OS_WIN32
#	include "polarphp/vm/tsrm/tsrm_win32.h"
#endif
}

POLAR_END_DISABLE_ZENDVM_WARNING()

#endif // POLARPHP_RUNTIME_INTERNAL_DEPS_ZENDVM_HEADERS_H
