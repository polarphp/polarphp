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

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_ARRAY_FUNCS_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_ARRAY_FUNCS_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

PHP_MINIT_FUNCTION(array);
PHP_MSHUTDOWN_FUNCTION(array);

PHP_FUNCTION(ksort);
PHP_FUNCTION(krsort);
PHP_FUNCTION(natsort);
PHP_FUNCTION(natcasesort);
PHP_FUNCTION(asort);
PHP_FUNCTION(arsort);
PHP_FUNCTION(sort);
PHP_FUNCTION(rsort);
PHP_FUNCTION(usort);
PHP_FUNCTION(uasort);
PHP_FUNCTION(uksort);
PHP_FUNCTION(array_walk);
PHP_FUNCTION(array_walk_recursive);
PHP_FUNCTION(count);
PHP_FUNCTION(array_count);
PHP_FUNCTION(end);
PHP_FUNCTION(prev);
PHP_FUNCTION(next);
PHP_FUNCTION(reset);
PHP_FUNCTION(current);
PHP_FUNCTION(key);
PHP_FUNCTION(min);
PHP_FUNCTION(max);
PHP_FUNCTION(in_array);
PHP_FUNCTION(array_search);
PHP_FUNCTION(extract);
PHP_FUNCTION(compact);
PHP_FUNCTION(array_fill);
PHP_FUNCTION(array_fill_keys);
PHP_FUNCTION(range);
PHP_FUNCTION(shuffle);
PHP_FUNCTION(array_multisort);
PHP_FUNCTION(array_push);
PHP_FUNCTION(array_pop);
PHP_FUNCTION(array_shift);
PHP_FUNCTION(array_unshift);
PHP_FUNCTION(array_splice);
PHP_FUNCTION(array_slice);
PHP_FUNCTION(array_merge);
PHP_FUNCTION(array_merge_recursive);
PHP_FUNCTION(array_replace);
PHP_FUNCTION(array_replace_recursive);
PHP_FUNCTION(array_keys);
PHP_FUNCTION(array_key_first);
PHP_FUNCTION(array_key_last);
PHP_FUNCTION(array_values);
PHP_FUNCTION(array_count_values);
PHP_FUNCTION(array_column);
PHP_FUNCTION(array_reverse);
PHP_FUNCTION(array_reduce);
PHP_FUNCTION(array_pad);
PHP_FUNCTION(array_flip);
PHP_FUNCTION(array_change_key_case);
PHP_FUNCTION(array_rand);
PHP_FUNCTION(array_unique);
PHP_FUNCTION(array_intersect);
PHP_FUNCTION(array_intersect_key);
PHP_FUNCTION(array_intersect_ukey);
PHP_FUNCTION(array_uintersect);
PHP_FUNCTION(array_intersect_assoc);
PHP_FUNCTION(array_uintersect_assoc);
PHP_FUNCTION(array_intersect_uassoc);
PHP_FUNCTION(array_uintersect_uassoc);
PHP_FUNCTION(array_diff);
PHP_FUNCTION(array_diff_key);
PHP_FUNCTION(array_diff_ukey);
PHP_FUNCTION(array_udiff);
PHP_FUNCTION(array_diff_assoc);
PHP_FUNCTION(array_udiff_assoc);
PHP_FUNCTION(array_diff_uassoc);
PHP_FUNCTION(array_udiff_uassoc);
PHP_FUNCTION(array_sum);
PHP_FUNCTION(array_product);
PHP_FUNCTION(array_filter);
PHP_FUNCTION(array_map);
PHP_FUNCTION(array_key_exists);
PHP_FUNCTION(array_chunk);
PHP_FUNCTION(array_combine);

POLAR_DECL_EXPORT int array_merge(HashTable *dest, HashTable *src);
POLAR_DECL_EXPORT int array_merge_recursive(HashTable *dest, HashTable *src);
POLAR_DECL_EXPORT int array_replace_recursive(HashTable *dest, HashTable *src);
POLAR_DECL_EXPORT int array_multisort_compare(const void *a, const void *b);
POLAR_DECL_EXPORT zend_long array_count_recursive(HashTable *ht);

#define PHP_SORT_REGULAR            0
#define PHP_SORT_NUMERIC            1
#define PHP_SORT_STRING             2
#define PHP_SORT_DESC               3
#define PHP_SORT_ASC                4
#define PHP_SORT_LOCALE_STRING      5
#define PHP_SORT_NATURAL            6
#define PHP_SORT_FLAG_CASE          8

#define COUNT_NORMAL      0
#define COUNT_RECURSIVE   1

#define ARRAY_FILTER_USE_BOTH	1
#define ARRAY_FILTER_USE_KEY	2

struct ArrayModuleData
{
   compare_func_t *multisortFunc = nullptr;
};

ArrayModuleData &retrieve_array_module_data();

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_ARRAY_FUNCS_H
