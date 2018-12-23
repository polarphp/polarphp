// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/12/23.

#ifndef POLAR_VMAPI_INTERNAL_DEPS_ZENDVM_HEADERS_H
#define POLAR_VMAPI_INTERNAL_DEPS_ZENDVM_HEADERS_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/global/SystemDetection.h"

POLAR_BEGIN_DISABLE_ZENDVM_WARNING()

extern "C" {
#include "polarphp/vm/zend/zend_types.h"
#include "polarphp/vm/zend/zend_exceptions.h"
#include "polarphp/global/php_stdint.h"

#ifdef POLAR_OS_WIN32
#	include "polarphp/vm/tsrm/tsrm_win32.h"
#endif
}

POLAR_END_DISABLE_ZENDVM_WARNING()

#endif // POLAR_VMAPI_INTERNAL_DEPS_ZENDVM_HEADERS_H
