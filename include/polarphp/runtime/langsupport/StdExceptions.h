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

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

PHP_MINIT_FUNCTION(stdexceptions);

extern POLAR_DECL_EXPORT zend_class_entry *g_LogicException;
extern POLAR_DECL_EXPORT zend_class_entry *g_BadFunctionCallException;
extern POLAR_DECL_EXPORT zend_class_entry *g_BadMethodCallException;
extern POLAR_DECL_EXPORT zend_class_entry *g_DomainException;
extern POLAR_DECL_EXPORT zend_class_entry *g_InvalidArgumentException;
extern POLAR_DECL_EXPORT zend_class_entry *g_LengthException;
extern POLAR_DECL_EXPORT zend_class_entry *g_OutOfRangeException;

extern POLAR_DECL_EXPORT zend_class_entry *g_RuntimeException;
extern POLAR_DECL_EXPORT zend_class_entry *g_OutOfBoundsException;
extern POLAR_DECL_EXPORT zend_class_entry *g_OverflowException;
extern POLAR_DECL_EXPORT zend_class_entry *g_RangeException;
extern POLAR_DECL_EXPORT zend_class_entry *g_UnderflowException;
extern POLAR_DECL_EXPORT zend_class_entry *g_UnexpectedValueException;

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_STDEXCEPTIONS_H
