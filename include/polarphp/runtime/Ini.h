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

#ifndef POLARPHP_RUNTIME_INI_H
#define POLARPHP_RUNTIME_INI_H

#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

#define POLAR_INI_USER	ZEND_INI_USER
#define POLAR_INI_PERDIR	ZEND_INI_PERDIR
#define POLAR_INI_SYSTEM	ZEND_INI_SYSTEM

#define POLAR_INI_ALL 	ZEND_INI_ALL

#define php_ini_entry	zend_ini_entry

#define POLAR_INI_MH		ZEND_INI_MH
#define POLAR_INI_DISP	ZEND_INI_DISP

#define POLAR_INI_BEGIN()	 zend_ini_entry_def ini_entries[] = {
#define POLAR_INI_END			ZEND_INI_END

#define POLAR_INI_ENTRY3_EX	ZEND_INI_ENTRY3_EX
#define POLAR_INI_ENTRY3		ZEND_INI_ENTRY3
#define POLAR_INI_ENTRY2_EX	ZEND_INI_ENTRY2_EX
#define POLAR_INI_ENTRY2		ZEND_INI_ENTRY2
#define POLAR_INI_ENTRY1_EX	ZEND_INI_ENTRY1_EX
#define POLAR_INI_ENTRY1		ZEND_INI_ENTRY1
#define POLAR_INI_ENTRY_EX	ZEND_INI_ENTRY_EX
#define POLAR_INI_ENTRY		ZEND_INI_ENTRY

#define POLAR_INI_DISPLAY_ORIG	ZEND_INI_DISPLAY_ORIG
#define POLAR_INI_DISPLAY_ACTIVE	ZEND_INI_DISPLAY_ACTIVE

#define POLAR_INI_STAGE_STARTUP		ZEND_INI_STAGE_STARTUP
#define POLAR_INI_STAGE_SHUTDOWN		ZEND_INI_STAGE_SHUTDOWN
#define POLAR_INI_STAGE_ACTIVATE		ZEND_INI_STAGE_ACTIVATE
#define POLAR_INI_STAGE_DEACTIVATE	ZEND_INI_STAGE_DEACTIVATE
#define POLAR_INI_STAGE_RUNTIME		ZEND_INI_STAGE_RUNTIME
#define POLAR_INI_STAGE_HTACCESS		ZEND_INI_STAGE_HTACCESS

#define POLAR_STD_INI_ENTRY(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr) \
   ZEND_INI_ENTRY2(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr)
#define POLAR_STD_INI_ENTRY_EX(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr, displayer) \
   ZEND_INI_ENTRY2_EX(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr, displayer)
#define POLAR_STD_INI_BOOLEAN(name, default_value, modifiable, on_modify, property_name, struct_type, struct_ptr) \
   ZEND_INI_ENTRY3_EX(name, default_value, modifiable, on_modify, (void *) XtOffsetOf(struct_type, property_name), (void *) &struct_ptr, NULL, zend_ini_boolean_displayer_cb)

#define polar_ini_boolean_displayer_cb	zend_ini_boolean_displayer_cb
#define polar_ini_color_displayer_cb		zend_ini_color_displayer_cb

#define polar_alter_ini_entry		zend_alter_ini_entry

#define polar_ini_long	zend_ini_long
#define polar_ini_double	zend_ini_double
#define polar_ini_string	zend_ini_string

namespace polar {
namespace runtime {

POLAR_DECL_EXPORT void config_zval_dtor(zval *zvalue);
bool php_init_config();
int php_shutdown_config();
void php_ini_register_extensions();
POLAR_DECL_EXPORT zval *cfg_get_entry_ex(zend_string *name);
POLAR_DECL_EXPORT zval *cfg_get_entry(const char *name, size_t nameLength);
POLAR_DECL_EXPORT int cfg_get_long(const char *varname, zend_long *result);
POLAR_DECL_EXPORT int cfg_get_double(const char *varname, double *result);
POLAR_DECL_EXPORT int cfg_get_string(const char *varname, char **result);
POLAR_DECL_EXPORT int php_parse_user_ini_file(const char *dirname, char *iniFilename, HashTable *targetHash);
POLAR_DECL_EXPORT void php_ini_activate_config(HashTable *sourceHash, int modifyType, int stage);
POLAR_DECL_EXPORT int php_ini_has_per_dir_config(void);
POLAR_DECL_EXPORT int php_ini_has_per_host_config(void);
POLAR_DECL_EXPORT void php_ini_activate_per_dir_config(char *path, size_t pathLen);
POLAR_DECL_EXPORT void php_ini_activate_per_host_config(const char *host, size_t hostLen);
POLAR_DECL_EXPORT HashTable *php_ini_get_configuration_hash(void);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_INI_H
