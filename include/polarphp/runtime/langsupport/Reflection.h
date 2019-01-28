// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/11.

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_REFLECTION_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_REFLECTION_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

#define PHP_REFLECTION_VERSION POLARPHP_VERSION

extern zend_module_entry g_reflectionModuleEntry;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectorPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionExceptionPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionFunctionAbstractPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionFunctionPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionGeneratorPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionParameterPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionTypePtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionNamedTypePtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionClassPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionClassConstantPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionObjectPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionMethodPtr;
extern POLAR_DECL_EXPORT zend_class_entry *g_reflectionPropertyPtr;
POLAR_DECL_EXPORT void zend_reflection_class_factory(zend_class_entry *ce, zval *object);
POLAR_DECL_EXPORT bool register_reflection_module();

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_REFLECTION_H
