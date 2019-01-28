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

#ifndef POLARPHP_RUNTIME_LIFE_CYCLE_H
#define POLARPHP_RUNTIME_LIFE_CYCLE_H

#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

#include <list>

namespace polar {
namespace runtime {

const char HARDCODED_INI[] =
      "html_errors=0\n"
      "register_argc_argv=1\n"
      "implicit_flush=1\n"
      "output_buffering=0\n"
      "max_execution_time=0\n"
      "max_input_time=-1\n\0";

POLAR_DECL_EXPORT bool php_module_startup();
POLAR_DECL_EXPORT void php_module_shutdown();
POLAR_DECL_EXPORT void php_module_shutdown_for_exec();
POLAR_DECL_EXPORT bool php_exec_env_startup();
POLAR_DECL_EXPORT void php_exec_env_shutdown();
POLAR_DECL_EXPORT bool php_register_extensions(std::list<zend_module_entry *> extensions);
POLAR_DECL_EXPORT int php_execute_script(zend_file_handle *primary_file);
POLAR_DECL_EXPORT int php_execute_simple_script(zend_file_handle *primary_file, zval *ret);

/* environment module */
int php_init_environ();
int php_shutdown_environ();
void cli_ini_defaults(HashTable *configuration_hash);
POLAR_DECL_EXPORT int php_during_module_startup();
POLAR_DECL_EXPORT int php_during_module_shutdown();
POLAR_DECL_EXPORT int php_get_module_initialized();

void php_free_cli_exec_globals();
void php_disable_functions();
void php_disable_classes();

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LIFE_CYCLE_H
