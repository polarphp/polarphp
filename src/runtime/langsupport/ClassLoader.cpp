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

struct AutoloadFuncInfo
{
   zend_function *funcPtr;
   zval obj;
   zval closure;
   zend_class_entry *ce;
};

namespace {
zend_class_entry * find_ce_by_name(zend_string *name, zend_bool autoload)
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

void register_std_class(zend_class_entry **ppce, char *className, void *objCtor, const zend_function_entry *functionList)
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
                        char *className, CreateObjectFuncType objCtor,
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
      if (nullptr == (ce = find_ce_by_name(Z_STR_P(obj), autoload))) {
         RETURN_FALSE;
      }
   } else {
      ce = Z_OBJCE_P(obj);
   }
   array_init(return_value);
   parent_class = ce->parent;
   while (parent_class) {
      add_class_name(return_value, parent_class, 0, 0);
      parent_class = parent_class->parent;
   }
}

PHP_FUNCTION(class_implements)
{
   zval *obj;
   zend_bool autoload = 1;
   zend_class_entry *ce;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &obj, &autoload) == FAILURE) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING) {
      php_error_docref(nullptr, E_WARNING, "object or string expected");
      RETURN_FALSE;
   }

   if (Z_TYPE_P(obj) == IS_STRING) {
      if (nullptr == (ce = find_ce_by_name(Z_STR_P(obj), autoload))) {
         RETURN_FALSE;
      }
   } else {
      ce = Z_OBJCE_P(obj);
   }
   array_init(return_value);
   add_interfaces(return_value, ce, 1, ZEND_ACC_INTERFACE);
}

PHP_FUNCTION(class_uses)
{
   zval *obj;
   zend_bool autoload = 1;
   zend_class_entry *ce;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "z|b", &obj, &autoload) == FAILURE) {
      RETURN_FALSE;
   }
   if (Z_TYPE_P(obj) != IS_OBJECT && Z_TYPE_P(obj) != IS_STRING) {
      php_error_docref(nullptr, E_WARNING, "object or string expected");
      RETURN_FALSE;
   }

   if (Z_TYPE_P(obj) == IS_STRING) {
      if (nullptr == (ce = find_ce_by_name(Z_STR_P(obj), autoload))) {
         RETURN_FALSE;
      }
   } else {
      ce = Z_OBJCE_P(obj);
   }

   array_init(return_value);
   add_traits(return_value, ce, 1, ZEND_ACC_TRAIT);
}

PHP_FUNCTION(set_autoload_file_extensions)
{
   zend_string *file_exts = nullptr;
   if (zend_parse_parameters(ZEND_NUM_ARGS(), "|S", &file_exts) == FAILURE) {
      return;
   }
   if (file_exts) {
      if (CLASS_LOADER_G(autoloadExtensions)) {
         zend_string_release_ex(CLASS_LOADER_G(autoloadExtensions), 0);
      }
      CLASS_LOADER_G(autoloadExtensions) = zend_string_copy(file_exts);
   }
   if (CLASS_LOADER_G(autoloadExtensions) == nullptr) {
      RETURN_STRINGL(CLASS_LOADER_DEFAULT_FILE_EXTENSIONS, sizeof(CLASS_LOADER_DEFAULT_FILE_EXTENSIONS) - 1);
   } else {
      zend_string_addref(CLASS_LOADER_G(autoloadExtensions));
      RETURN_STR(CLASS_LOADER_G(autoloadExtensions));
   }
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
//   zend_string *func_name;
//   char *error = nullptr;
//   zend_string *lc_name;
//   zval *zcallable = nullptr;
//   zend_bool do_throw = 1;
//   zend_bool prepend  = 0;
//   zend_function *spl_func_ptr;
//   AutoloadFuncInfo alfi;
//   zend_object *obj_ptr;
//   zend_fcall_info_cache fcc;

//   if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "|zbb", &zcallable, &do_throw, &prepend) == FAILURE) {
//      return;
//   }

//   if (ZEND_NUM_ARGS()) {
//      if (!zend_is_callable_ex(zcallable, nullptr, IS_CALLABLE_STRICT, &func_name, &fcc, &error)) {
//         alfi.ce = fcc.calling_scope;
//         alfi.funcPtr = fcc.function_handler;
//         obj_ptr = fcc.object;
//         if (Z_TYPE_P(zcallable) == IS_ARRAY) {
//            if (!obj_ptr && alfi.func_ptr && !(alfi.func_ptr->common.fn_flags & ZEND_ACC_STATIC)) {
//               if (do_throw) {
//                  zend_throw_exception_ex(g_logicException, 0, "Passed array specifies a non static method but no object (%s)", error);
//               }
//               if (error) {
//                  efree(error);
//               }
//               zend_string_release_ex(func_name, 0);
//               RETURN_FALSE;
//            } else if (do_throw) {
//               zend_throw_exception_ex(spl_ce_LogicException, 0, "Passed array does not specify %s %smethod (%s)", alfi.func_ptr ? "a callable" : "an existing", !obj_ptr ? "static " : "", error);
//            }
//            if (error) {
//               efree(error);
//            }
//            zend_string_release_ex(func_name, 0);
//            RETURN_FALSE;
//         } else if (Z_TYPE_P(zcallable) == IS_STRING) {
//            if (do_throw) {
//               zend_throw_exception_ex(spl_ce_LogicException, 0, "Function '%s' not %s (%s)", ZSTR_VAL(func_name), alfi.func_ptr ? "callable" : "found", error);
//            }
//            if (error) {
//               efree(error);
//            }
//            zend_string_release_ex(func_name, 0);
//            RETURN_FALSE;
//         } else {
//            if (do_throw) {
//               zend_throw_exception_ex(spl_ce_LogicException, 0, "Illegal value passed (%s)", error);
//            }
//            if (error) {
//               efree(error);
//            }
//            zend_string_release_ex(func_name, 0);
//            RETURN_FALSE;
//         }
//      } else if (fcc.function_handler->type == ZEND_INTERNAL_FUNCTION &&
//                 fcc.function_handler->internal_function.handler == zif_spl_autoload_call) {
//         if (do_throw) {
//            zend_throw_exception_ex(spl_ce_LogicException, 0, "Function spl_autoload_call() cannot be registered");
//         }
//         if (error) {
//            efree(error);
//         }
//         zend_string_release_ex(func_name, 0);
//         RETURN_FALSE;
//      }
//      alfi.ce = fcc.calling_scope;
//      alfi.func_ptr = fcc.function_handler;
//      obj_ptr = fcc.object;
//      if (error) {
//         efree(error);
//      }

//      if (Z_TYPE_P(zcallable) == IS_OBJECT) {
//         ZVAL_COPY(&alfi.closure, zcallable);

//         lc_name = zend_string_alloc(ZSTR_LEN(func_name) + sizeof(uint32_t), 0);
//         zend_str_tolower_copy(ZSTR_VAL(lc_name), ZSTR_VAL(func_name), ZSTR_LEN(func_name));
//         memcpy(ZSTR_VAL(lc_name) + ZSTR_LEN(func_name), &Z_OBJ_HANDLE_P(zcallable), sizeof(uint32_t));
//         ZSTR_VAL(lc_name)[ZSTR_LEN(lc_name)] = '\0';
//      } else {
//         ZVAL_UNDEF(&alfi.closure);
//         /* Skip leading \ */
//         if (ZSTR_VAL(func_name)[0] == '\\') {
//            lc_name = zend_string_alloc(ZSTR_LEN(func_name) - 1, 0);
//            zend_str_tolower_copy(ZSTR_VAL(lc_name), ZSTR_VAL(func_name) + 1, ZSTR_LEN(func_name) - 1);
//         } else {
//            lc_name = zend_string_tolower(func_name);
//         }
//      }
//      zend_string_release_ex(func_name, 0);

//      if (SPL_G(autoload_functions) && zend_hash_exists(SPL_G(autoload_functions), lc_name)) {
//         if (!Z_ISUNDEF(alfi.closure)) {
//            Z_DELREF_P(&alfi.closure);
//         }
//         goto skip;
//      }

//      if (obj_ptr && !(alfi.func_ptr->common.fn_flags & ZEND_ACC_STATIC)) {
//         /* add object id to the hash to ensure uniqueness, for more reference look at bug #40091 */
//         lc_name = zend_string_extend(lc_name, ZSTR_LEN(lc_name) + sizeof(uint32_t), 0);
//         memcpy(ZSTR_VAL(lc_name) + ZSTR_LEN(lc_name) - sizeof(uint32_t), &obj_ptr->handle, sizeof(uint32_t));
//         ZSTR_VAL(lc_name)[ZSTR_LEN(lc_name)] = '\0';
//         ZVAL_OBJ(&alfi.obj, obj_ptr);
//         Z_ADDREF(alfi.obj);
//      } else {
//         ZVAL_UNDEF(&alfi.obj);
//      }

//      if (!SPL_G(autoload_functions)) {
//         ALLOC_HASHTABLE(SPL_G(autoload_functions));
//         zend_hash_init(SPL_G(autoload_functions), 1, nullptr, autoload_func_info_dtor, 0);
//      }

//      spl_func_ptr = spl_autoload_fn;

//      if (EG(autoload_func) == spl_func_ptr) { /* registered already, so we insert that first */
//         autoload_func_info spl_alfi;

//         spl_alfi.func_ptr = spl_func_ptr;
//         ZVAL_UNDEF(&spl_alfi.obj);
//         ZVAL_UNDEF(&spl_alfi.closure);
//         spl_alfi.ce = nullptr;
//         zend_hash_add_mem(SPL_G(autoload_functions), spl_autoload_fn->common.function_name,
//                           &spl_alfi, sizeof(autoload_func_info));
//         if (prepend && SPL_G(autoload_functions)->nNumOfElements > 1) {
//            /* Move the newly created element to the head of the hashtable */
//            HT_MOVE_TAIL_TO_HEAD(SPL_G(autoload_functions));
//         }
//      }

//      if (UNEXPECTED(alfi.func_ptr == &EG(trampoline))) {
//         zend_function *copy = emalloc(sizeof(zend_op_array));

//         memcpy(copy, alfi.func_ptr, sizeof(zend_op_array));
//         alfi.func_ptr->common.function_name = nullptr;
//         alfi.func_ptr = copy;
//      }
//      if (zend_hash_add_mem(SPL_G(autoload_functions), lc_name, &alfi, sizeof(autoload_func_info)) == nullptr) {
//         if (obj_ptr && !(alfi.func_ptr->common.fn_flags & ZEND_ACC_STATIC)) {
//            Z_DELREF(alfi.obj);
//         }
//         if (!Z_ISUNDEF(alfi.closure)) {
//            Z_DELREF(alfi.closure);
//         }
//         if (UNEXPECTED(alfi.func_ptr->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
//            zend_string_release_ex(alfi.func_ptr->common.function_name, 0);
//            zend_free_trampoline(alfi.func_ptr);
//         }
//      }
//      if (prepend && SPL_G(autoload_functions)->nNumOfElements > 1) {
//         /* Move the newly created element to the head of the hashtable */
//         HT_MOVE_TAIL_TO_HEAD(SPL_G(autoload_functions));
//      }
//skip:
//      zend_string_release_ex(lc_name, 0);
//   }

//   if (SPL_G(autoload_functions)) {
//      EG(autoload_func) = spl_autoload_call_fn;
//   } else {
//      EG(autoload_func) = spl_autoload_fn;
//   }

//   RETURN_TRUE;
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
