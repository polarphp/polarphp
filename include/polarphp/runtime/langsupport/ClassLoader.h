// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/14.

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_CLASS_LOADER_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_CLASS_LOADER_H

#include "polarphp/runtime/RtDefs.h"
#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

namespace polar {
namespace runtime {

#define CLASS_LOADER_G(v) retrieve_classloader_module_data().v

#define RT_REGISTER_STD_CLASS(class_name, obj_ctor) \
   register_std_class(&g_ ## class_name, const_cast<char *>(# class_name), obj_ctor, NULL);

#define RT_REGISTER_STD_CLASS_EX(class_name, obj_ctor, funcs) \
   register_std_class(&g_ ## class_name,const_cast<char *>(# class_name), obj_ctor, funcs);

#define RT_REGISTER_SUB_CLASS_EX(class_name, parent_class_name, obj_ctor, funcs) \
   register_sub_class(&g_ ## class_name, g_ ## parent_class_name, const_cast<char *>(# class_name), obj_ctor, funcs);

#define RT_REGISTER_INTERFACE(class_name) \
   register_interface(&g_ ## class_name, const_cast<char *>(# class_name), spl_funcs_ ## class_name);

#define RT_REGISTER_IMPLEMENTS(class_name, interface_name) \
   zend_class_implements(g_ ## class_name, 1, spl_ce_ ## interface_name);

#define RT_REGISTER_ITERATOR(class_name) \
   zend_class_implements(g_ ## class_name, 1, zend_ce_iterator);

#define RT_REGISTER_PROPERTY(class_name, prop_name, prop_flags) \
   register_property(g_ ## class_name, prop_name, sizeof(prop_name)-1, prop_flags);

#define RT_REGISTER_CLASS_CONST_LONG(class_name, const_name, value) \
   zend_declare_class_constant_long(g_ ## class_name, const_name, sizeof(const_name)-1, (zend_long)value);

struct ClassLoaderModuleData
{
   bool         hashMaskInit;
   bool         autoloadRunning;
   intptr_t     hashMaskHandle;
   intptr_t     hashMaskHandlers;
   zend_string  *autoloadExtensions;
   HashTable    *autoloadFunctions;
};

using CreateObjectFuncType = zend_object* (*)(zend_class_entry *classType);

extern zend_module_entry g_classLoaderModuleEntry;

POLAR_DECL_EXPORT void register_std_class(zend_class_entry ** ppce, char * className, CreateObjectFuncType ctor,
                                          const zend_function_entry * functionList);
POLAR_DECL_EXPORT void register_sub_class(zend_class_entry ** ppce, zend_class_entry * parentClassEntry, char * className,
                                          CreateObjectFuncType ctor, const zend_function_entry * functionList);
POLAR_DECL_EXPORT void register_interface(zend_class_entry ** ppce, char * className, const zend_function_entry *functions);
POLAR_DECL_EXPORT void register_property( zend_class_entry * classEntry, char *propName, int propNameLen, int propFlags);
POLAR_DECL_EXPORT void add_class_name(zval * list, zend_class_entry * pce, int allow, int classEntryFlags);
POLAR_DECL_EXPORT void add_interfaces(zval * list, zend_class_entry * pce, int allow, int classEntryFlags);
POLAR_DECL_EXPORT void add_traits(zval * list, zend_class_entry * pce, int allow, int classEntryFlags);
POLAR_DECL_EXPORT int add_classes(zend_class_entry *pce, zval *list, int sub, int allow, int classEntryFlags);
POLAR_DECL_EXPORT _zend_string *php_object_hash(zval *obj);

PHP_MINIT_FUNCTION(classloader);
PHP_RINIT_FUNCTION(classloader);
PHP_RSHUTDOWN_FUNCTION(classloader);

PHP_FUNCTION(class_parents);
PHP_FUNCTION(class_implements);
PHP_FUNCTION(class_uses);
PHP_FUNCTION(set_autoload_file_extensions);
PHP_FUNCTION(default_class_loader);
PHP_FUNCTION(retrieve_registered_class_loaders);
PHP_FUNCTION(load_class);
PHP_FUNCTION(register_class_loader);
PHP_FUNCTION(unregister_class_loader);
PHP_FUNCTION(object_hash);
PHP_FUNCTION(object_id);

ClassLoaderModuleData &retrieve_classloader_module_data();

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_CLASS_LOADER_H
