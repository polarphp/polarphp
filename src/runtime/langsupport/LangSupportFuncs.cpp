// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/09.

#include "polarphp/runtime/langsupport/LangSupportFuncs.h"
#include "polarphp/runtime/langsupport/TypeFuncs.h"
#include "polarphp/runtime/langsupport/VariableFuncs.h"
#include "polarphp/runtime/langsupport/ArrayFuncs.h"
#include "polarphp/runtime/langsupport/AssertFuncs.h"
#include "polarphp/runtime/langsupport/Reflection.h"
#include "polarphp/runtime/langsupport/StdExceptions.h"
#include "polarphp/runtime/langsupport/ClassLoader.h"
#include "polarphp/runtime/langsupport/SerializeFuncs.h"

namespace polar {
namespace runtime {

RuntimeModuleData &retrieve_runtime_module_data()
{
   thread_local RuntimeModuleData rdata;
   return rdata;
}

static HashTable sg_RuntimeSubmodules;

#include "ArgInfo.defs"

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
   /// functions for serialize and unserialize
   ///
   PHP_FE(serialize,       arginfo_serialize)
   PHP_FE(unserialize,		arginfo_unserialize)
   ///
   /// functions for variables
   ///
   PHP_FE(var_dump,        arginfo_var_dump)
   PHP_FE(var_export,      arginfo_var_export)
   PHP_FE(debug_zval_dump, arginfo_debug_zval_dump)
   ///
   /// Functions for array
   ///
   PHP_FE(ksort,															arginfo_ksort)
   PHP_FE(krsort,															arginfo_krsort)
   PHP_FE(natsort,                                          arginfo_natsort)
   PHP_FE(natcasesort,												   arginfo_natcasesort)
   PHP_FE(asort,															arginfo_asort)
   PHP_FE(arsort,															arginfo_arsort)
   PHP_FE(sort,															arginfo_sort)
   PHP_FE(rsort,															arginfo_rsort)
   PHP_FE(usort,															arginfo_usort)
   PHP_FE(uasort,															arginfo_uasort)
   PHP_FE(uksort,															arginfo_uksort)
   PHP_FE(shuffle,														arginfo_shuffle)
   PHP_FE(array_walk,													arginfo_array_walk)
   PHP_FE(array_walk_recursive,										arginfo_array_walk_recursive)
   PHP_FE(count,												         arginfo_count)
   PHP_FE(array_count,												   arginfo_array_count)
   PHP_FE(end,																arginfo_end)
   PHP_FE(prev,															arginfo_prev)
   PHP_FE(next,															arginfo_next)
   PHP_FE(reset,															arginfo_reset)
   PHP_FE(current,														arginfo_current)
   PHP_FE(key,																arginfo_key)
   PHP_FE(min,																arginfo_min)
   PHP_FE(max,																arginfo_max)
   PHP_FE(in_array,														arginfo_in_array)
   PHP_FE(array_search,													arginfo_array_search)
   PHP_FE(extract,														arginfo_extract)
   PHP_FE(compact,														arginfo_compact)
   PHP_FE(array_fill,													arginfo_array_fill)
   PHP_FE(array_fill_keys,												arginfo_array_fill_keys)
   PHP_FE(range,															arginfo_range)
   PHP_FE(array_multisort,												arginfo_array_multisort)
   PHP_FE(array_push,													arginfo_array_push)
   PHP_FE(array_pop,														arginfo_array_pop)
   PHP_FE(array_shift,													arginfo_array_shift)
   PHP_FE(array_unshift,												arginfo_array_unshift)
   PHP_FE(array_splice,													arginfo_array_splice)
   PHP_FE(array_slice,													arginfo_array_slice)
   PHP_FE(array_merge,													arginfo_array_merge)
   PHP_FE(array_merge_recursive,										arginfo_array_merge_recursive)
   PHP_FE(array_replace,												arginfo_array_replace)
   PHP_FE(array_replace_recursive,									arginfo_array_replace_recursive)
   PHP_FE(array_keys,													arginfo_array_keys)
   PHP_FE(array_key_first,												arginfo_array_key_first)
   PHP_FE(array_key_last,												arginfo_array_key_last)
   PHP_FE(array_values,													arginfo_array_values)
   PHP_FE(array_count_values,											arginfo_array_count_values)
   PHP_FE(array_column,													arginfo_array_column)
   PHP_FE(array_reverse,												arginfo_array_reverse)
   PHP_FE(array_reduce,													arginfo_array_reduce)
   PHP_FE(array_pad,														arginfo_array_pad)
   PHP_FE(array_flip,													arginfo_array_flip)
   PHP_FE(array_change_key_case,										arginfo_array_change_key_case)
   PHP_FE(array_rand,													arginfo_array_rand)
   PHP_FE(array_unique,													arginfo_array_unique)
   PHP_FE(array_intersect,												arginfo_array_intersect)
   PHP_FE(array_intersect_key,										arginfo_array_intersect_key)
   PHP_FE(array_intersect_ukey,										arginfo_array_intersect_ukey)
   PHP_FE(array_uintersect,											arginfo_array_uintersect)
   PHP_FE(array_intersect_assoc,										arginfo_array_intersect_assoc)
   PHP_FE(array_uintersect_assoc,									arginfo_array_uintersect_assoc)
   PHP_FE(array_intersect_uassoc,									arginfo_array_intersect_uassoc)
   PHP_FE(array_uintersect_uassoc,									arginfo_array_uintersect_uassoc)
   PHP_FE(array_diff,													arginfo_array_diff)
   PHP_FE(array_diff_key,												arginfo_array_diff_key)
   PHP_FE(array_diff_ukey,												arginfo_array_diff_ukey)
   PHP_FE(array_udiff,													arginfo_array_udiff)
   PHP_FE(array_diff_assoc,											arginfo_array_diff_assoc)
   PHP_FE(array_udiff_assoc,											arginfo_array_udiff_assoc)
   PHP_FE(array_diff_uassoc,											arginfo_array_diff_uassoc)
   PHP_FE(array_udiff_uassoc,											arginfo_array_udiff_uassoc)
   PHP_FE(array_sum,														arginfo_array_sum)
   PHP_FE(array_product,												arginfo_array_product)
   PHP_FE(array_filter,													arginfo_array_filter)
   PHP_FE(array_map,													   arginfo_array_map)
   PHP_FE(array_chunk,													arginfo_array_chunk)
   PHP_FE(array_combine,												arginfo_array_combine)
   PHP_FE(array_key_exists,											arginfo_array_key_exists)

   /// aseert
   PHP_FE(assert,                                           arginfo_assert)
   PHP_FE(assert_options,                                   arginfo_assert_options)

   /// class loader
   PHP_FE(default_class_loader,                             arginfo_default_class_loader)
   PHP_FE(set_autoload_file_extensions,                     arginfo_set_autoload_file_extensions)
   PHP_FE(register_class_loader,                            arginfo_register_class_loader)
   PHP_FE(unregister_class_loader,                          arginfo_unregister_class_loader)
   PHP_FE(retrieve_registered_class_loaders,                arginfo_retrieve_registered_class_loaders)
   PHP_FE(load_class,                                       arginfo_load_class)
   PHP_FE(class_parents,                                    arginfo_class_parents)
   PHP_FE(class_implements,                                 arginfo_class_implements)
   PHP_FE(class_uses,                                       arginfo_class_uses)
   PHP_FE(object_hash,                                      arginfo_object_hash)
   PHP_FE(object_id,                                        arginfo_object_id)
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
   RUNTIME_MINIT_SUBMODULE(assert);
   RUNTIME_MINIT_SUBMODULE(stdexceptions);
   RUNTIME_MINIT_SUBMODULE(classloader);
   return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(Runtime)
{
   RUNTIME_MSHUTDOWN_SUBMODULE(array);
   RUNTIME_MSHUTDOWN_SUBMODULE(assert);
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
   RUNTIME_RINIT_SUBMODULE(classloader);
   return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(Runtime)
{
   RuntimeModuleData &rdata = retrieve_runtime_module_data();
   if (rdata.userTickFunctions) {
      zend_llist_destroy(rdata.userTickFunctions);
      efree(rdata.userTickFunctions);
      rdata.userTickFunctions = nullptr;
   }
   RUNTIME_RSHUTDOWN_SUBMODULE(classloader);
   RUNTIME_RSHUTDOWN_SUBMODULE(assert);
   return SUCCESS;
}

namespace {
inline bool register_basic_lang_support_module()
{
   g_langSupportModule.type = MODULE_PERSISTENT;
   g_langSupportModule.module_number = zend_next_free_module();
   g_langSupportModule.handle = nullptr;
   if ((EG(current_module) = zend_register_module_ex(&g_langSupportModule)) == nullptr) {
      return false;
   }
   return true;
}
} // anonymous namespace

bool register_lang_support_funcs()
{
   if (!register_basic_lang_support_module()){
      return false;
   }
   /// register reflection module
   if (!register_reflection_module()) {
      return false;
   }
   return true;
}

} // runtime
} // polar
