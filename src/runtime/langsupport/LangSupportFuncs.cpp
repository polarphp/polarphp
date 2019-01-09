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

#include "polarphp/runtime/langsupport/LangSupportFuncs.h"
#include "polarphp/runtime/langsupport/TypeFuncs.h"
#include "polarphp/runtime/PhpDefs.h"

namespace polar {
namespace runtime {

ZEND_BEGIN_ARG_INFO(arginfo_is_null, 0)
   ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_langSupportFunctions[] = {
   PHP_FE(is_null,  arginfo_is_null)
   ZEND_FE_END
};

zend_module_entry g_langSupportModule = {
   STANDARD_MODULE_HEADER,
   "Runtime",
   sg_langSupportFunctions,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   nullptr,
   ZEND_VERSION,
   STANDARD_MODULE_PROPERTIES
};

bool register_lang_support_funcs()
{
   g_langSupportModule.type = MODULE_PERSISTENT;
   g_langSupportModule.module_number = zend_next_free_module();
   g_langSupportModule.handle = nullptr;
   if ((EG(current_module) = zend_register_module_ex(&g_langSupportModule)) == NULL) {
      return false;
   }
   return true;
}

} // runtime
} // polar
