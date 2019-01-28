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

#ifndef POLARPHP_VMAPI_INTERNAL_DEPS_ZENDVM_HEADERS_H
#define POLARPHP_VMAPI_INTERNAL_DEPS_ZENDVM_HEADERS_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/SystemDetection.h"

POLAR_BEGIN_DISABLE_ZENDVM_WARNING()

extern "C" {
#include "polarphp/vm/zend/zend.h"
#include "polarphp/vm/zend/zend_alloc.h"
#include "polarphp/vm/zend/zend_API.h"
#include "polarphp/vm/zend/zend_types.h"
#include "polarphp/vm/zend/zend_errors.h"
#include "polarphp/vm/zend/zend_closures.h"
#include "polarphp/vm/zend/zend_compile.h"
#include "polarphp/vm/zend/zend_exceptions.h"
#include "polarphp/vm/zend/zend_constants.h"
#include "polarphp/vm/zend/zend_smart_str.h"
#include "polarphp/vm/zend/zend_modules.h"
#include "polarphp/vm/zend/zend_ini.h"
#include "polarphp/vm/zend/zend_inheritance.h"
#include "polarphp/vm/tsrm/tsrm.h"
#include "polarphp/global/php_stdint.h"

#ifdef POLAR_OS_WIN32
#	include "polarphp/vm/tsrm/tsrm_win32.h"
#endif
}

POLAR_END_DISABLE_ZENDVM_WARNING()

#endif // POLARPHP_VMAPI_INTERNAL_DEPS_ZENDVM_HEADERS_H
