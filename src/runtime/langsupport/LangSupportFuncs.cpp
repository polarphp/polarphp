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
#include "polarphp/runtime/langsupport/AssertFuncs.h"
#include "polarphp/runtime/langsupport/Reflection.h"

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

///
/// array functions args
///
ZEND_BEGIN_ARG_INFO_EX(arginfo_krsort, 0, 0, 1)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, sort_flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_ksort, 0, 0, 1)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, sort_flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_count, 0, 0, 1)
ZEND_ARG_ARRAY_INFO(0, var, 0)
ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_count, 0, 0, 1)
ZEND_ARG_INFO(0, var)
ZEND_ARG_INFO(0, mode)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_natsort, 0)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_natcasesort, 0)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_asort, 0, 0, 1)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, sort_flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_arsort, 0, 0, 1)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, sort_flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_sort, 0, 0, 1)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, sort_flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_rsort, 0, 0, 1)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, sort_flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_usort, 0)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, cmp_function)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_uasort, 0)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, cmp_function)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_uksort, 0)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, cmp_function)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_end, 0)
ZEND_ARG_INFO(1, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_prev, 0)
ZEND_ARG_INFO(1, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_next, 0)
ZEND_ARG_INFO(1, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_reset, 0)
ZEND_ARG_INFO(1, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_current, 0)
ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_key, 0)
ZEND_ARG_INFO(0, arg)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_min, 0, 0, 1)
ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_max, 0, 0, 1)
ZEND_ARG_VARIADIC_INFO(0, args)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_walk, 0, 0, 2)
ZEND_ARG_INFO(1, input) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, funcname)
ZEND_ARG_INFO(0, userdata)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_walk_recursive, 0, 0, 2)
ZEND_ARG_INFO(1, input) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, funcname)
ZEND_ARG_INFO(0, userdata)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_in_array, 0, 0, 2)
ZEND_ARG_INFO(0, needle)
ZEND_ARG_INFO(0, haystack) /* ARRAY_INFO(0, haystack, 0) */
ZEND_ARG_INFO(0, strict)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_search, 0, 0, 2)
ZEND_ARG_INFO(0, needle)
ZEND_ARG_INFO(0, haystack) /* ARRAY_INFO(0, haystack, 0) */
ZEND_ARG_INFO(0, strict)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_extract, 0, 0, 1)
ZEND_ARG_INFO(ZEND_SEND_PREFER_REF, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, extract_type)
ZEND_ARG_INFO(0, prefix)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_compact, 0, 0, 1)
ZEND_ARG_VARIADIC_INFO(0, var_names)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_fill, 0)
ZEND_ARG_INFO(0, start_key)
ZEND_ARG_INFO(0, num)
ZEND_ARG_INFO(0, val)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_fill_keys, 0)
ZEND_ARG_INFO(0, keys) /* ARRAY_INFO(0, keys, 0) */
ZEND_ARG_INFO(0, val)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_range, 0, 0, 2)
ZEND_ARG_INFO(0, low)
ZEND_ARG_INFO(0, high)
ZEND_ARG_INFO(0, step)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_shuffle, 0)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_push, 0, 0, 1)
ZEND_ARG_INFO(1, stack) /* ARRAY_INFO(1, stack, 0) */
ZEND_ARG_VARIADIC_INFO(0, vars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_pop, 0)
ZEND_ARG_INFO(1, stack) /* ARRAY_INFO(1, stack, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_shift, 0)
ZEND_ARG_INFO(1, stack) /* ARRAY_INFO(1, stack, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_unshift, 0, 0, 1)
ZEND_ARG_INFO(1, stack) /* ARRAY_INFO(1, stack, 0) */
ZEND_ARG_VARIADIC_INFO(0, vars)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_splice, 0, 0, 2)
ZEND_ARG_INFO(1, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, offset)
ZEND_ARG_INFO(0, length)
ZEND_ARG_INFO(0, replacement) /* ARRAY_INFO(0, arg, 1) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_slice, 0, 0, 2)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(1, arg, 0) */
ZEND_ARG_INFO(0, offset)
ZEND_ARG_INFO(0, length)
ZEND_ARG_INFO(0, preserve_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_merge, 0, 0, 1)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_merge_recursive, 0, 0, 1)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_replace, 0, 0, 1)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_replace_recursive, 0, 0, 1)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_keys, 0, 0, 1)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, search_value)
ZEND_ARG_INFO(0, strict)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_key_first, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_key_last, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_END_ARG_INFO()


ZEND_BEGIN_ARG_INFO(arginfo_array_values, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_count_values, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_column, 0, 0, 2)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, column_key)
ZEND_ARG_INFO(0, index_key)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_reverse, 0, 0, 1)
ZEND_ARG_INFO(0, input) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, preserve_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_pad, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, pad_size)
ZEND_ARG_INFO(0, pad_value)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_flip, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_change_key_case, 0, 0, 1)
ZEND_ARG_INFO(0, input) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, case)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_unique, 0, 0, 1)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, flags)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_intersect_key, 0, 0, 2)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_intersect_ukey, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_key_compare_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_intersect, 0, 0, 2)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_uintersect, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_data_compare_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_intersect_assoc, 0, 0, 2)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_uintersect_assoc, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_data_compare_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_intersect_uassoc, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_key_compare_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_uintersect_uassoc, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_data_compare_func)
ZEND_ARG_INFO(0, callback_key_compare_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_diff_key, 0, 0, 2)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_diff_ukey, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_key_comp_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_diff, 0, 0, 2)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_udiff, 0)
ZEND_ARG_INFO(0, arr1)
ZEND_ARG_INFO(0, arr2)
ZEND_ARG_INFO(0, callback_data_comp_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_diff_assoc, 0, 0, 2)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_diff_uassoc, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_data_comp_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_udiff_assoc, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_key_comp_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_udiff_uassoc, 0)
ZEND_ARG_INFO(0, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(0, arr2) /* ARRAY_INFO(0, arg2, 0) */
ZEND_ARG_INFO(0, callback_data_comp_func)
ZEND_ARG_INFO(0, callback_key_comp_func)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_multisort, 0, 0, 1)
ZEND_ARG_INFO(ZEND_SEND_PREFER_REF, arr1) /* ARRAY_INFO(0, arg1, 0) */
ZEND_ARG_INFO(ZEND_SEND_PREFER_REF, sort_order)
ZEND_ARG_INFO(ZEND_SEND_PREFER_REF, sort_flags)
ZEND_ARG_VARIADIC_INFO(ZEND_SEND_PREFER_REF, arr2)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_rand, 0, 0, 1)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, num_req)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_sum, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_product, 0)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_reduce, 0, 0, 2)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, callback)
ZEND_ARG_INFO(0, initial)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_filter, 0, 0, 1)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, callback)
ZEND_ARG_INFO(0, use_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_map, 0, 0, 2)
ZEND_ARG_INFO(0, callback)
ZEND_ARG_VARIADIC_INFO(0, arrays)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_key_exists, 0)
ZEND_ARG_INFO(0, key)
ZEND_ARG_INFO(0, search)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_array_chunk, 0, 0, 2)
ZEND_ARG_INFO(0, arg) /* ARRAY_INFO(0, arg, 0) */
ZEND_ARG_INFO(0, size)
ZEND_ARG_INFO(0, preserve_keys)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO(arginfo_array_combine, 0)
ZEND_ARG_INFO(0, keys)   /* ARRAY_INFO(0, keys, 0) */
ZEND_ARG_INFO(0, values) /* ARRAY_INFO(0, values, 0) */
ZEND_END_ARG_INFO()

/// args for assert
ZEND_BEGIN_ARG_INFO_EX(arginfo_assert, 0, 0, 1)
   ZEND_ARG_INFO(0, assertion)
   ZEND_ARG_INFO(0, description)
ZEND_END_ARG_INFO()

ZEND_BEGIN_ARG_INFO_EX(arginfo_assert_options, 0, 0, 1)
   ZEND_ARG_INFO(0, what)
   ZEND_ARG_INFO(0, value)
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
   RUNTIME_MINIT_SUBMODULE(assert);
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
