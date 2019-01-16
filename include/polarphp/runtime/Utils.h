// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/25.

#ifndef POLARPHP_RUNTIME_UTILS_H
#define POLARPHP_RUNTIME_UTILS_H

#include <ctime>
#include <cstdio>
#include <random>

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/vm/zend/zend_long.h"

struct _zend_string;

namespace polar {
namespace runtime {

std::size_t php_format_date(char* str, std::size_t count, const char* format,
                            time_t ts, bool localtime);
POLAR_DECL_EXPORT char *php_strtoupper(char *s, size_t len);
POLAR_DECL_EXPORT char *php_strtolower(char *s, size_t len);
POLAR_DECL_EXPORT char *php_strip_url_passwd(char *url);
POLAR_DECL_EXPORT char *expand_filepath(const char *filePath, char *realPath);
POLAR_DECL_EXPORT char *expand_filepath(const char *filePath, char *realPath,
                                        const char *relativeTo, size_t relativeToLen);
POLAR_DECL_EXPORT char *expand_filepath(const char *filePath, char *realPath,
                                        const char *relativeTo, size_t relativeToLen,
                                        int realPathMode);
POLAR_DECL_EXPORT FILE *php_fopen_with_path(const char *filename, const char *mode,
                                            const char *path, _zend_string **openedPath);
POLAR_DECL_EXPORT int php_check_open_basedir(const char *path);
POLAR_DECL_EXPORT int php_check_open_basedir(const char *path, int warn);
POLAR_DECL_EXPORT int php_check_specific_open_basedir(const char *basedir, const char *path);
POLAR_DECL_EXPORT _zend_string *php_addcslashes_str(const char *str, size_t len, char *what, size_t wlength);
POLAR_DECL_EXPORT _zend_string *php_addcslashes(_zend_string *str, char *what, size_t wlength);
POLAR_DECL_EXPORT _zend_string *php_str_to_str(const char *haystack, size_t length, const char *needle,
                                               size_t needle_len, const char *str, size_t str_len);
POLAR_DECL_EXPORT _zend_string *php_string_toupper(_zend_string *s);
POLAR_DECL_EXPORT _zend_string *php_string_tolower(_zend_string *s);
POLAR_DECL_EXPORT int strnatcmp_ex(char const *a, size_t a_len, char const *b, size_t b_len, int fold_case);
POLAR_DECL_EXPORT zend_long mt_rand_range(zend_long min, zend_long max);
POLAR_DECL_EXPORT std::uint32_t mt_rand_32();
POLAR_DECL_EXPORT std::uint64_t mt_rand_64();

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_UTILS_H

