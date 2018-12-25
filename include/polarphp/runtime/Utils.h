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

#include "polarphp/global/CompilerFeature.h"

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

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_UTILS_H

