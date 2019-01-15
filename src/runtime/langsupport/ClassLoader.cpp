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

#include "polarphp/runtime/langsupport/ClassLoader.h"

namespace polar {
namespace runtime {

static zend_function *sg_autoloadFunc = nullptr;
static zend_function *sg_autoloadCallFunc = nullptr;

#define CLASS_LOADER_DEFAULT_FILE_EXTENSIONS ".inc,.php"

ClassLoaderModuleData &retrieve_classloader_module_data()
{
   thread_local ClassLoaderModuleData classLoaderModuleData{
      nullptr,
            nullptr,
            false
   };
   return classLoaderModuleData;
}

namespace {
zend_class_entry * spl_find_ce_by_name(zend_string *name, zend_bool autoload)
{
   zend_class_entry *ce;
   if (!autoload) {
      zend_string *lc_name = zend_string_tolower(name);
      ce = reinterpret_cast<zend_class_entry *>(zend_hash_find_ptr(EG(class_table), lc_name));
      zend_string_free(lc_name);
   } else {
      ce = zend_lookup_class(name);
   }
   if (ce == nullptr) {
      php_error_docref(nullptr, E_WARNING, "Class %s does not exist%s", ZSTR_VAL(name), autoload ? " and could not be loaded" : "");
      return nullptr;
   }
   return ce;
}

} // anonymous namespace

void register_interface(zend_class_entry **ppce, char *className, const zend_function_entry * functions)
{
   zend_class_entry ce;
   INIT_CLASS_ENTRY_EX(ce, className, strlen(className), functions);
   *ppce = zend_register_internal_interface(&ce);
}

void spl_register_std_class(zend_class_entry **ppce, char *className, void *objCtor, const zend_function_entry *functionList)
{
   zend_class_entry ce;

   INIT_CLASS_ENTRY_EX(ce, className, strlen(className), functionList);
   *ppce = zend_register_internal_class(&ce);
   /* entries changed by initialize */
   if (objCtor) {
      (*ppce)->create_object = reinterpret_cast<CreateObjectFuncType>(objCtor);
   }
}

void register_sub_class(zend_class_entry **ppce, zend_class_entry *parentClassEntry,
                        char *className, void *objCtor,
                        const zend_function_entry *functionList)
{
   zend_class_entry ce;
   INIT_CLASS_ENTRY_EX(ce, className, strlen(className), functionList);
   *ppce = zend_register_internal_class_ex(&ce, parentClassEntry);
   /* entries changed by initialize */
   if (objCtor) {
      (*ppce)->create_object = reinterpret_cast<CreateObjectFuncType>(objCtor);
   } else {
      (*ppce)->create_object = reinterpret_cast<CreateObjectFuncType>(parentClassEntry->create_object);
   }
}

void register_property( zend_class_entry *classEntry, char *propPame, int propNameLen, int propFlags)
{
   zend_declare_property_null(classEntry, propPame, propNameLen, propFlags);
}

void add_class_name(zval *list, zend_class_entry *pce, int allow, int classEntryFlags)
{
   if (!allow || (allow > 0 && pce->ce_flags & classEntryFlags) || (allow < 0 && !(pce->ce_flags & classEntryFlags))) {
      zval *tmp;
      if ((tmp = zend_hash_find(Z_ARRVAL_P(list), pce->name)) == nullptr) {
         zval t;
         ZVAL_STR_COPY(&t, pce->name);
         zend_hash_add(Z_ARRVAL_P(list), pce->name, &t);
      }
   }
}

void add_interfaces(zval *list, zend_class_entry *pce, int allow, int classEntryflags)
{
   uint32_t num_interfaces;

   for (num_interfaces = 0; num_interfaces < pce->num_interfaces; num_interfaces++) {
      add_class_name(list, pce->interfaces[num_interfaces], allow, classEntryflags);
   }
}

void add_traits(zval *list, zend_class_entry * pce, int allow, int classEntryflags)
{
   uint32_t num_traits;

   for (num_traits = 0; num_traits < pce->num_traits; num_traits++) {
      add_class_name(list, pce->traits[num_traits], allow, classEntryflags);
   }
}

int add_classes(zend_class_entry *pce, zval *list, int sub, int allow, int classEntryflags)
{
   if (!pce) {
      return 0;
   }
   add_class_name(list, pce, allow, classEntryflags);
   if (sub) {
      add_interfaces(list, pce, allow, classEntryflags);
      while (pce->parent) {
         pce = pce->parent;
         add_classes(pce, list, sub, allow, classEntryflags);
      }
   }
   return 0;
}

///
/// proto array class_parents(object instance [, bool autoload = true])
/// Return an array containing the names of all parent classes
///

PHP_FUNCTION(class_parents)
{
   zval *obj;
   zend_class_entry *parent_class, *ce;
   zend_bool autoload = 1;
   if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &obj, &autoload) == FAILURE) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING) {
      php_error_docref(nullptr, E_WARNING, "object or string expected");
      RETURN_FALSE;
   }
   if (Z_TYPE_P(obj) == IS_STRING) {
      if (nullptr == (ce = spl_find_ce_by_name(Z_STR_P(obj), autoload))) {
         RETURN_FALSE;
      }
   } else {
      ce = Z_OBJCE_P(obj);
   }
   array_init(return_value);
   parent_class = ce->parent;
//   while (parent_class) {
//      spl_add_class_name(return_value, parent_class, 0, 0);
//      parent_class = parent_class->parent;
//   }
}

PHP_FUNCTION(class_implements)
{

}

PHP_FUNCTION(class_uses)
{

}

PHP_FUNCTION(set_autoload_file_extensions)
{

}

PHP_FUNCTION(default_class_loader)
{

}

PHP_FUNCTION(retrieve_registered_class_loaders)
{

}

PHP_FUNCTION(load_class)
{

}

PHP_FUNCTION(register_class_loader)
{

}

PHP_FUNCTION(unregister_class_loader)
{

}

PHP_MINIT_FUNCTION(classloader)
{
   return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(classloader)
{
   return SUCCESS;
}

PHP_RINIT_FUNCTION(classloader)
{
   return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(classloader)
{
   return SUCCESS;
}

} // runtime
} // polar
