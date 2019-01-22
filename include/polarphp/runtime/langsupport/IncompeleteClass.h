// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/20.

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_INCOMPLETE_CLASS_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_INCOMPLETE_CLASS_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

#define PHP_IC_ENTRY \
   retrieve_runtime_module_data().incompleteClass

#define PHP_SET_CLASS_ATTRIBUTES(struc) \
   /* OBJECTS_FIXME: Fix for new object model */	\
   if (Z_OBJCE_P(struc) == PHP_IC_ENTRY) {	\
      class_name = lookup_class_name(struc); \
      if (!class_name) { \
         class_name = zend_string_init(INCOMPLETE_CLASS, sizeof(INCOMPLETE_CLASS) - 1, 0); \
      } \
      incomplete_class = 1; \
   } else { \
      class_name = zend_string_copy(Z_OBJCE_P(struc)->name); \
   }

#define PHP_CLEANUP_CLASS_ATTRIBUTES()	\
   zend_string_release_ex(class_name, 0)

#define PHP_CLASS_ATTRIBUTES											\
   zend_string *class_name;											\
   zend_bool incomplete_class ZEND_ATTRIBUTE_UNUSED = 0

#define INCOMPLETE_CLASS "__PHP_Incomplete_Class"
#define MAGIC_MEMBER "__PHP_Incomplete_Class_Name"


POLAR_DECL_EXPORT zend_class_entry *create_incomplete_class(void);
POLAR_DECL_EXPORT zend_string *lookup_class_name(zval *object);
POLAR_DECL_EXPORT void  store_class_name(zval *object, const char *name, size_t len);


} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_INCOMPLETE_CLASS_H
