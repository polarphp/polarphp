// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/14.

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_STDEXCEPTIONS_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_STDEXCEPTIONS_H

#include "polarphp/runtime/PhpDefs.h"

namespace polar {
namespace runtime {

extern POLAR_DECL_EXPORT zend_class_entry *g_logicExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_badFunctionCallExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_badMethodCallExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_domainExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_invalidArgumentExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_lengthExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_outOfRangeExceptionClass;

extern POLAR_DECL_EXPORT zend_class_entry *g_runtimeExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_outOfBoundsExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_overflowExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_rangeExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_underflowExceptionClass;
extern POLAR_DECL_EXPORT zend_class_entry *g_unexpectedValueExceptionClass;

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_STDEXCEPTIONS_H
