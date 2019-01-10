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
#include "polarphp/runtime/langsupport/VariableFuncs.h"
#include "polarphp/runtime/langsupport/ArrayFuncs.h"

namespace polar {
namespace runtime {

RuntimeModuleData &retrieve_runtime_module_data()
{
   thread_local RuntimeModuleData rdata;
   return rdata;
}

static HashTable sg_RuntimeSubmodules;

///
/// type args
///
ZEND_BEGIN_ARG_INFO(arginfo_gettype, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_settype, 0)
ZEND_ARG_INFO(1, var)
ZEND_ARG_INFO(0, type)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_intval, 0, 0, 1)
ZEND_ARG_INFO(0, var)
ZEND_ARG_INFO(0, base)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_floatval, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_strval, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_boolval, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_null, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_resource, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_bool, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_int, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_float, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_string, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_array, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_object, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_numeric, 0)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_scalar, 0)
ZEND_ARG_INFO(0, value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_is_callable, 0, 0, 1)
ZEND_ARG_INFO(0, var)
ZEND_ARG_INFO(0, syntax_only)
ZEND_ARG_INFO(1, callable_name)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_is_iterable, 0, 0, 1)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_is_countable, 0)
ZEND_ARG_INFO(0, var)
ZEND_END_ARG_INFO()

///
/// variable args
///
ZEND_BEGIN_ARG_INFO_EX(arginfo_var_dump, 0, 0, 1)
ZEND_ARG_VARIADIC_INFO(0, vars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_debug_zval_dump, 0, 0, 1)
ZEND_ARG_VARIADIC_INFO(0, vars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_var_export, 0, 0, 1)
ZEND_ARG_INFO(0, var)
ZEND_ARG_INFO(0, return)
ZEND_END_ARG_INFO()

static const zend_function_entry sg_langSupportFunctions[] = {
   ///
   /// functions for types
   ///
   PHP_FE(intval,          arginfo_intval)
   PHP_FE(floatval,        arginfo_floatval)
   PHP_FALIAS(doubleval,   floatval,            arginfo_floatval)
   PHP_FE(strval,          arginfo_strval)
   PHP_FE(boolval,         arginfo_boolval)
   PHP_FE(gettype,         arginfo_gettype)
   PHP_FE(settype,         arginfo_settype)
   PHP_FE(is_null,         arginfo_is_null)
   PHP_FE(is_resource,     arginfo_is_resource)
   PHP_FE(is_bool,         arginfo_is_bool)
   PHP_FE(is_int,          arginfo_is_int)
   PHP_FE(is_float,        arginfo_is_float)
   PHP_FALIAS(is_integer,  is_int,	             arginfo_is_int)
   PHP_FALIAS(is_long,     is_int,               arginfo_is_int)
   PHP_FALIAS(is_double,   is_float,             arginfo_is_float)
   PHP_FALIAS(is_real,     is_float,             arginfo_is_float)
   PHP_FE(is_numeric,      arginfo_is_numeric)
   PHP_FE(is_string,       arginfo_is_string)
   PHP_FE(is_array,        arginfo_is_array)
   PHP_FE(is_object,       arginfo_is_object)
   PHP_FE(is_scalar,       arginfo_is_scalar)
   PHP_FE(is_callable,     arginfo_is_callable)
   PHP_FE(is_iterable,     arginfo_is_iterable)
   PHP_FE(is_countable,    arginfo_is_countable)
   ///
   /// functions for variables
   ///
   PHP_FE(var_dump,        arginfo_var_dump)
   PHP_FE(var_export,      arginfo_var_export)
   PHP_FE(debug_zval_dump, arginfo_debug_zval_dump)
   ZEND_FE_END
};

zend_module_entry g_langSupportModule = {
   STANDARD_MODULE_HEADER,
   "Runtime",
   sg_langSupportFunctions,
   PHP_MINIT(Runtime),			/* process startup */
   PHP_MSHUTDOWN(Runtime),		/* process shutdown */
   PHP_RINIT(Runtime),			/* request startup */
   PHP_RSHUTDOWN(Runtime),		/* request shutdown */
   nullptr,			/* extension info */
   ZEND_VERSION,
   STANDARD_MODULE_PROPERTIES
};

#define RUNTIME_MINIT_SUBMODULE(module) \
   if (PHP_MINIT(module)(INIT_FUNC_ARGS_PASSTHRU) == SUCCESS) {\
   RUNTIME_ADD_SUBMODULE(module); \
}

#define RUNTIME_ADD_SUBMODULE(module) \
   zend_hash_str_add_empty_element(&sg_RuntimeSubmodules, #module, strlen(#module));

#define RUNTIME_RINIT_SUBMODULE(module) \
   if (zend_hash_str_exists(&sg_RuntimeSubmodules, #module, strlen(#module))) { \
   PHP_RINIT(module)(INIT_FUNC_ARGS_PASSTHRU); \
}

#define RUNTIME_MINFO_SUBMODULE(module) \
   if (zend_hash_str_exists(&sg_RuntimeSubmodules, #module, strlen(#module))) { \
   PHP_MINFO(module)(ZEND_MODULE_INFO_FUNC_ARGS_PASSTHRU); \
}

#define RUNTIME_RSHUTDOWN_SUBMODULE(module) \
   if (zend_hash_str_exists(&sg_RuntimeSubmodules, #module, strlen(#module))) { \
   PHP_RSHUTDOWN(module)(SHUTDOWN_FUNC_ARGS_PASSTHRU); \
}

#define RUNTIME_MSHUTDOWN_SUBMODULE(module) \
   if (zend_hash_str_exists(&sg_RuntimeSubmodules, #module, strlen(#module))) { \
   PHP_MSHUTDOWN(module)(SHUTDOWN_FUNC_ARGS_PASSTHRU); \
}

PHP_MINIT_FUNCTION(Runtime)
{
   zend_hash_init(&sg_RuntimeSubmodules, 0, NULL, NULL, 1);

   REGISTER_LONG_CONSTANT("INI_USER",   ZEND_INI_USER,   CONST_CS | CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("INI_PERDIR", ZEND_INI_PERDIR, CONST_CS | CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("INI_SYSTEM", ZEND_INI_SYSTEM, CONST_CS | CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("INI_ALL",    ZEND_INI_ALL,    CONST_CS | CONST_PERSISTENT);

#define REGISTER_MATH_CONSTANT(x)  REGISTER_DOUBLE_CONSTANT(#x, x, CONST_CS | CONST_PERSISTENT)
   REGISTER_MATH_CONSTANT(M_E);
   REGISTER_MATH_CONSTANT(M_LOG2E);
   REGISTER_MATH_CONSTANT(M_LOG10E);
   REGISTER_MATH_CONSTANT(M_LN2);
   REGISTER_MATH_CONSTANT(M_LN10);
   REGISTER_MATH_CONSTANT(M_PI);
   REGISTER_MATH_CONSTANT(M_PI_2);
   REGISTER_MATH_CONSTANT(M_PI_4);
   REGISTER_MATH_CONSTANT(M_1_PI);
   REGISTER_MATH_CONSTANT(M_2_PI);
   REGISTER_MATH_CONSTANT(M_2_SQRTPI);
   REGISTER_MATH_CONSTANT(M_SQRT2);
   REGISTER_MATH_CONSTANT(M_SQRT1_2);
   REGISTER_DOUBLE_CONSTANT("INF", ZEND_INFINITY, CONST_CS | CONST_PERSISTENT);
   REGISTER_DOUBLE_CONSTANT("NAN", ZEND_NAN, CONST_CS | CONST_PERSISTENT);
   RUNTIME_MINIT_SUBMODULE(array);
   return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(Runtime)
{
   RUNTIME_MSHUTDOWN_SUBMODULE(array);
   zend_hash_destroy(&sg_RuntimeSubmodules);
   return SUCCESS;
}

PHP_RINIT_FUNCTION(Runtime)
{
   RuntimeModuleData &rdata = retrieve_runtime_module_data();
   rdata.arrayWalkFci = empty_fcall_info;
   rdata.arrayWalkFciCache = empty_fcall_info_cache;
   rdata.userCompareFci = empty_fcall_info;
   rdata.userCompareFciCache = empty_fcall_info_cache;
   return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(Runtime) /* {{{ */
{
   RuntimeModuleData &rdata = retrieve_runtime_module_data();
   if (rdata.userTickFunctions) {
      zend_llist_destroy(rdata.userTickFunctions);
      efree(rdata.userTickFunctions);
      rdata.userTickFunctions = nullptr;
   }
   return SUCCESS;
}

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
