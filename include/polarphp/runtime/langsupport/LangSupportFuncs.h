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

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_LANG_SUPPORT_FUNCS_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_LANG_SUPPORT_FUNCS_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

PHP_MINIT_FUNCTION(Runtime);
PHP_MSHUTDOWN_FUNCTION(Runtime);
PHP_RINIT_FUNCTION(Runtime);
PHP_RSHUTDOWN_FUNCTION(Runtime);
PHP_MINFO_FUNCTION(Runtime);

struct RuntimeModuleData
{
   HashTable *userShutdownFunctionNames = nullptr;
   zend_fcall_info arrayWalkFci;
   zend_fcall_info_cache arrayWalkFciCache;
   zend_fcall_info userCompareFci;
   zend_fcall_info_cache userCompareFciCache;
   zend_llist *userTickFunctions = nullptr;
   zend_class_entry *incompleteClass;

   unsigned serializeLock; /* whether to use the locally supplied var_hash instead (__sleep/__wakeup) */
   struct {
      struct SerializeData *data;
      unsigned level;
   } serialize;
   struct {
      struct UnserializeData *data;
      unsigned level;
   } unserialize;
};

RuntimeModuleData &retrieve_runtime_module_data();
bool register_lang_support_funcs();
POLAR_DECL_EXPORT int prefix_varname(zval *result, const zval *prefix,
                                     const char *var_name, size_t var_name_len,
                                     zend_bool add_underscore);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_LANG_SUPPORT_FUNCS_H
