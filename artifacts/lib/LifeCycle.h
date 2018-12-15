// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/12.

#ifndef POLARPHP_ARTIFACTS_LIFE_CYCLE_H
#define POLARPHP_ARTIFACTS_LIFE_CYCLE_H

#include "ZendHeaders.h"

namespace polar {

POLAR_DECL_EXPORT bool php_module_startup(zend_module_entry *additionalModules, uint32_t numAdditionalModules);
POLAR_DECL_EXPORT void php_module_shutdown();
POLAR_DECL_EXPORT void php_module_shutdown_for_exec();
//POLAR_DECL_EXPORT int php_module_shutdown_wrapper(sapi_module_struct *sapi_globals);

POLAR_DECL_EXPORT int php_register_extensions(zend_module_entry * const * ptr, int count);

POLAR_DECL_EXPORT int php_execute_script(zend_file_handle *primary_file);
POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primary_file, zval *ret);

POLAR_DECL_EXPORT void php_html_puts(const char *str, size_t siz);
POLAR_DECL_EXPORT int php_stream_open_for_zend_ex(const char *filename, zend_file_handle *handle, int mode);

/* environment module */
extern int php_init_environ();
extern int php_shutdown_environ();

} // polar

#endif // POLARPHP_ARTIFACTS_LIFE_CYCLE_H
