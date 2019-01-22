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
#include "polarphp/runtime/langsupport/StdExceptions.h"
#include "polarphp/runtime/Spprintf.h"
#include "polarphp/runtime/Utils.h"

namespace polar {
namespace runtime {

static zend_function *sg_autoloadFunc = nullptr;
static zend_function *sg_autoloadCallFunc = nullptr;

#define CLASS_LOADER_DEFAULT_FILE_EXTENSIONS const_cast<char *>(".inc,.php")

ClassLoaderModuleData &retrieve_classloader_module_data()
{
   thread_local ClassLoaderModuleData classLoaderModuleData{
      false,
      false,
      0,
      0,
      nullptr,
            nullptr
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

void autoload_func_info_dtor(zval *element)
{
   AutoloadFuncInfo *alfi = (AutoloadFuncInfo*)Z_PTR_P(element);
   if (!Z_ISUNDEF(alfi->obj)) {
      zval_ptr_dtor(&alfi->obj);
   }
   if (alfi->funcPtr &&
       UNEXPECTED(alfi->funcPtr->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
      zend_string_release_ex(alfi->funcPtr->common.function_name, 0);
      zend_free_trampoline(alfi->funcPtr);
   }
   if (!Z_ISUNDEF(alfi->closure)) {
      zval_ptr_dtor(&alfi->closure);
   }
   efree(alfi);
}

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

void add_className(zval *list, zend_class_entry *pce, int allow, int classEntryFlags)
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
      add_className(list, pce->interfaces[num_interfaces], allow, classEntryflags);
   }
}

void add_traits(zval *list, zend_class_entry * pce, int allow, int classEntryflags)
{
   uint32_t num_traits;

   for (num_traits = 0; num_traits < pce->num_traits; num_traits++) {
      add_className(list, pce->traits[num_traits], allow, classEntryflags);
   }
}

int add_classes(zend_class_entry *pce, zval *list, int sub, int allow, int classEntryflags)
{
   if (!pce) {
      return 0;
   }
   add_className(list, pce, allow, classEntryflags);
   if (sub) {
      add_interfaces(list, pce, allow, classEntryflags);
      while (pce->parent) {
         pce = pce->parent;
         add_classes(pce, list, sub, allow, classEntryflags);
      }
   }
   return 0;
}

zend_string *php_object_hash(zval *obj) /* {{{*/
{
   intptr_t hashHandle, hashHandlers;

   if (!CLASS_LOADER_G(hashMaskInit)) {
      CLASS_LOADER_G(hashMaskHandle)   = (intptr_t)(mt_rand_32() >> 1);
      CLASS_LOADER_G(hashMaskHandlers) = (intptr_t)(mt_rand_32() >> 1);
      CLASS_LOADER_G(hashMaskInit) = 1;
   }

   hashHandle   = CLASS_LOADER_G(hashMaskHandle) ^ (intptr_t) Z_OBJ_HANDLE_P(obj);
   hashHandlers = CLASS_LOADER_G(hashMaskHandlers);

   return polar_strpprintf(32, "%016zx%016zx", hashHandle, hashHandlers);
}

PHP_FUNCTION(object_hash)
{
   zval *obj;
   if (zend_parse_parameters(ZEND_NUM_ARGS(), "o", &obj) == FAILURE) {
      return;
   }
   RETURN_NEW_STR(php_object_hash(obj));
}

PHP_FUNCTION(object_id)
{
   zval *obj;
   ZEND_PARSE_PARAMETERS_START(1, 1)
      Z_PARAM_OBJECT(obj)
   ZEND_PARSE_PARAMETERS_END();
   RETURN_LONG((zend_long)Z_OBJ_HANDLE_P(obj));
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
      add_className(return_value, parent_class, 0, 0);
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

namespace {
int default_autoload_handler(zend_string *className, zend_string *lc_name, const char *ext, int ext_len)
{
   char *classFile;
   int classFileLen;
   zval dummy;
   zend_file_handle file_handle;
   zend_op_array *new_op_array;
   zval result;
   int ret;

   classFileLen = (int)polar_spprintf(&classFile, 0, "%s%.*s", ZSTR_VAL(lc_name), ext_len, ext);

#if DEFAULT_SLASH != '\\'
   {
      char *ptr = classFile;
      char *end = ptr + classFileLen;

      while ((ptr = reinterpret_cast<char *>(memchr(ptr, '\\', (end - ptr)))) != nullptr) {
         *ptr = DEFAULT_SLASH;
      }
   }
#endif
   /// review here, we use zend_stream_open instead of php_stream_open_for_zend_ex
   /// because polarphp does not use php stream
   ret = zend_stream_open(classFile, &file_handle);
   //   ret = php_stream_open_for_zend_ex(classFile, &file_handle, USE_PATH|STREAM_OPEN_FOR_INCLUDE);

   if (ret == SUCCESS) {
      zend_string *opened_path;
      if (!file_handle.opened_path) {
         file_handle.opened_path = zend_string_init(classFile, classFileLen, 0);
      }
      opened_path = zend_string_copy(file_handle.opened_path);
      ZVAL_NULL(&dummy);
      if (zend_hash_add(&EG(included_files), opened_path, &dummy)) {
         new_op_array = zend_compile_file(&file_handle, ZEND_REQUIRE);
         zend_destroy_file_handle(&file_handle);
      } else {
         new_op_array = nullptr;
         zend_file_handle_dtor(&file_handle);
      }
      zend_string_release_ex(opened_path, 0);
      if (new_op_array) {
         ZVAL_UNDEF(&result);
         zend_execute(new_op_array, &result);

         destroy_op_array(new_op_array);
         efree(new_op_array);
         if (!EG(exception)) {
            zval_ptr_dtor(&result);
         }

         efree(classFile);
         return zend_hash_exists(EG(class_table), lc_name);
      }
   }
   efree(classFile);
   return 0;
}
} // anonymous namespace

PHP_FUNCTION(default_class_loader)
{
   int pos_len, pos1_len;
   char *pos, *pos1;
   zend_string *className, *lc_name, *file_exts = CLASS_LOADER_G(autoloadExtensions);

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "S|S", &className, &file_exts) == FAILURE) {
      RETURN_FALSE;
   }

   if (file_exts == nullptr) { /* autoloadExtensions is not initialized, set to defaults */
      pos = CLASS_LOADER_DEFAULT_FILE_EXTENSIONS;
      pos_len = sizeof(CLASS_LOADER_DEFAULT_FILE_EXTENSIONS) - 1;
   } else {
      pos = ZSTR_VAL(file_exts);
      pos_len = (int)ZSTR_LEN(file_exts);
   }

   lc_name = zend_string_tolower(className);
   while (pos && *pos && !EG(exception)) {
      pos1 = strchr(pos, ',');
      if (pos1) {
         pos1_len = (int)(pos1 - pos);
      } else {
         pos1_len = pos_len;
      }
      if (default_autoload_handler(className, lc_name, pos, pos1_len)) {
         break; /* loaded */
      }
      pos = pos1 ? pos1 + 1 : nullptr;
      pos_len = pos1? pos_len - pos1_len - 1 : 0;
   }
   zend_string_free(lc_name);
}

PHP_FUNCTION(retrieve_registered_class_loaders)
{

}

PHP_FUNCTION(load_class)
{
   zval *className, retval;
   zend_string *lc_name, *func_name;
   AutoloadFuncInfo *alfi;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &className) == FAILURE || Z_TYPE_P(className) != IS_STRING) {
      return;
   }

   if (CLASS_LOADER_G(autoloadFunctions)) {
      HashPosition pos;
      zend_ulong num_idx;
      zend_function *func;
      zend_fcall_info fci;
      zend_fcall_info_cache fcic;
      zend_class_entry *called_scope = zend_get_called_scope(execute_data);
      int l_autoloadRunning = CLASS_LOADER_G(autoloadRunning);

      CLASS_LOADER_G(autoloadRunning) = 1;
      lc_name = zend_string_tolower(Z_STR_P(className));

      fci.size = sizeof(fci);
      fci.retval = &retval;
      fci.param_count = 1;
      fci.params = className;
      fci.no_separation = 1;

      ZVAL_UNDEF(&fci.function_name); /* Unused */

      zend_hash_internal_pointer_reset_ex(CLASS_LOADER_G(autoloadFunctions), &pos);
      while (zend_hash_get_current_key_ex(CLASS_LOADER_G(autoloadFunctions), &func_name, &num_idx, &pos) == HASH_KEY_IS_STRING) {
         alfi = reinterpret_cast<AutoloadFuncInfo *>(zend_hash_get_current_data_ptr_ex(CLASS_LOADER_G(autoloadFunctions), &pos));
         func = alfi->funcPtr;
         if (UNEXPECTED(func->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
            func = reinterpret_cast<zend_function *>(emalloc(sizeof(zend_op_array)));
            memcpy(func, alfi->funcPtr, sizeof(zend_op_array));
            zend_string_addref(func->op_array.function_name);
         }
         ZVAL_UNDEF(&retval);
         fcic.function_handler = func;
         if (Z_ISUNDEF(alfi->obj)) {
            fci.object = nullptr;
            fcic.object = nullptr;
            if (alfi->ce &&
                (!called_scope ||
                 !instanceof_function(called_scope, alfi->ce))) {
               fcic.called_scope = alfi->ce;
            } else {
               fcic.called_scope = called_scope;
            }
         } else {
            fci.object = Z_OBJ(alfi->obj);
            fcic.object = Z_OBJ(alfi->obj);
            fcic.called_scope = Z_OBJCE(alfi->obj);
         }

         zend_call_function(&fci, &fcic);
         zval_ptr_dtor(&retval);

         if (EG(exception)) {
            break;
         }

         if (pos + 1 == CLASS_LOADER_G(autoloadFunctions)->nNumUsed ||
             zend_hash_exists(EG(class_table), lc_name)) {
            break;
         }
         zend_hash_move_forward_ex(CLASS_LOADER_G(autoloadFunctions), &pos);
      }
      zend_string_release_ex(lc_name, 0);
      CLASS_LOADER_G(autoloadRunning) = l_autoloadRunning;
   } else {
      /* do not use or overwrite &EG(autoload_func) here */
      zend_fcall_info fcall_info;
      zend_fcall_info_cache fcall_cache;

      ZVAL_UNDEF(&retval);

      fcall_info.size = sizeof(fcall_info);
      ZVAL_UNDEF(&fcall_info.function_name);
      fcall_info.retval = &retval;
      fcall_info.param_count = 1;
      fcall_info.params = className;
      fcall_info.object = nullptr;
      fcall_info.no_separation = 1;

      fcall_cache.function_handler = sg_autoloadFunc;
      fcall_cache.called_scope = nullptr;
      fcall_cache.object = nullptr;

      zend_call_function(&fcall_info, &fcall_cache);
      zval_ptr_dtor(&retval);
   }
}

#define HT_MOVE_TAIL_TO_HEAD(ht)						        \
   do {												        \
   ::Bucket tmp = (ht)->arData[(ht)->nNumUsed-1];				\
   memmove((ht)->arData + 1, (ht)->arData,					\
   sizeof(Bucket) * ((ht)->nNumUsed - 1));				\
   (ht)->arData[0] = tmp;									\
   zend_hash_rehash(ht);						        	\
} while (0)

PHP_FUNCTION(register_class_loader)
{
   zend_string *func_name;
   char *error = nullptr;
   zend_string *lc_name;
   zval *zcallable = nullptr;
   zend_bool do_throw = 1;
   zend_bool prepend  = 0;
   zend_function *funcPtr;
   AutoloadFuncInfo alfi;
   zend_object *obj_ptr;
   zend_fcall_info_cache fcc;

   if (zend_parse_parameters_ex(ZEND_PARSE_PARAMS_QUIET, ZEND_NUM_ARGS(), "|zbb", &zcallable, &do_throw, &prepend) == FAILURE) {
      return;
   }

   if (ZEND_NUM_ARGS()) {
      if (!zend_is_callable_ex(zcallable, nullptr, IS_CALLABLE_STRICT, &func_name, &fcc, &error)) {
         alfi.ce = fcc.calling_scope;
         alfi.funcPtr = fcc.function_handler;
         obj_ptr = fcc.object;
         if (Z_TYPE_P(zcallable) == IS_ARRAY) {
            if (!obj_ptr && alfi.funcPtr && !(alfi.funcPtr->common.fn_flags & ZEND_ACC_STATIC)) {
               if (do_throw) {
                  zend_throw_exception_ex(g_LogicException, 0, "Passed array specifies a non static method but no object (%s)", error);
               }
               if (error) {
                  efree(error);
               }
               zend_string_release_ex(func_name, 0);
               RETURN_FALSE;
            } else if (do_throw) {
               zend_throw_exception_ex(g_LogicException, 0, "Passed array does not specify %s %smethod (%s)", alfi.funcPtr ? "a callable" : "an existing", !obj_ptr ? "static " : "", error);
            }
            if (error) {
               efree(error);
            }
            zend_string_release_ex(func_name, 0);
            RETURN_FALSE;
         } else if (Z_TYPE_P(zcallable) == IS_STRING) {
            if (do_throw) {
               zend_throw_exception_ex(g_LogicException, 0, "Function '%s' not %s (%s)", ZSTR_VAL(func_name), alfi.funcPtr ? "callable" : "found", error);
            }
            if (error) {
               efree(error);
            }
            zend_string_release_ex(func_name, 0);
            RETURN_FALSE;
         } else {
            if (do_throw) {
               zend_throw_exception_ex(g_LogicException, 0, "Illegal value passed (%s)", error);
            }
            if (error) {
               efree(error);
            }
            zend_string_release_ex(func_name, 0);
            RETURN_FALSE;
         }
      } else if (fcc.function_handler->type == ZEND_INTERNAL_FUNCTION &&
                 fcc.function_handler->internal_function.handler == zif_load_class) {
         if (do_throw) {
            zend_throw_exception_ex(g_LogicException, 0, "Function spl_autoload_call() cannot be registered");
         }
         if (error) {
            efree(error);
         }
         zend_string_release_ex(func_name, 0);
         RETURN_FALSE;
      }
      alfi.ce = fcc.calling_scope;
      alfi.funcPtr = fcc.function_handler;
      obj_ptr = fcc.object;
      if (error) {
         efree(error);
      }

      if (Z_TYPE_P(zcallable) == IS_OBJECT) {
         ZVAL_COPY(&alfi.closure, zcallable);

         lc_name = zend_string_alloc(ZSTR_LEN(func_name) + sizeof(uint32_t), 0);
         zend_str_tolower_copy(ZSTR_VAL(lc_name), ZSTR_VAL(func_name), ZSTR_LEN(func_name));
         memcpy(ZSTR_VAL(lc_name) + ZSTR_LEN(func_name), &Z_OBJ_HANDLE_P(zcallable), sizeof(uint32_t));
         ZSTR_VAL(lc_name)[ZSTR_LEN(lc_name)] = '\0';
      } else {
         ZVAL_UNDEF(&alfi.closure);
         /* Skip leading \ */
         if (ZSTR_VAL(func_name)[0] == '\\') {
            lc_name = zend_string_alloc(ZSTR_LEN(func_name) - 1, 0);
            zend_str_tolower_copy(ZSTR_VAL(lc_name), ZSTR_VAL(func_name) + 1, ZSTR_LEN(func_name) - 1);
         } else {
            lc_name = zend_string_tolower(func_name);
         }
      }
      zend_string_release_ex(func_name, 0);

      if (CLASS_LOADER_G(autoloadFunctions) && zend_hash_exists(CLASS_LOADER_G(autoloadFunctions), lc_name)) {
         if (!Z_ISUNDEF(alfi.closure)) {
            Z_DELREF_P(&alfi.closure);
         }
         goto skip;
      }

      if (obj_ptr && !(alfi.funcPtr->common.fn_flags & ZEND_ACC_STATIC)) {
         /* add object id to the hash to ensure uniqueness, for more reference look at bug #40091 */
         lc_name = zend_string_extend(lc_name, ZSTR_LEN(lc_name) + sizeof(uint32_t), 0);
         memcpy(ZSTR_VAL(lc_name) + ZSTR_LEN(lc_name) - sizeof(uint32_t), &obj_ptr->handle, sizeof(uint32_t));
         ZSTR_VAL(lc_name)[ZSTR_LEN(lc_name)] = '\0';
         ZVAL_OBJ(&alfi.obj, obj_ptr);
         Z_ADDREF(alfi.obj);
      } else {
         ZVAL_UNDEF(&alfi.obj);
      }

      if (!CLASS_LOADER_G(autoloadFunctions)) {
         ALLOC_HASHTABLE(CLASS_LOADER_G(autoloadFunctions));
         zend_hash_init(CLASS_LOADER_G(autoloadFunctions), 1, nullptr, autoload_func_info_dtor, 0);
      }

      funcPtr = sg_autoloadFunc;

      if (EG(autoload_func) == funcPtr) { /* registered already, so we insert that first */
         AutoloadFuncInfo alfi;

         alfi.funcPtr = funcPtr;
         ZVAL_UNDEF(&alfi.obj);
         ZVAL_UNDEF(&alfi.closure);
         alfi.ce = nullptr;
         zend_hash_add_mem(CLASS_LOADER_G(autoloadFunctions), sg_autoloadFunc->common.function_name,
                           &alfi, sizeof(AutoloadFuncInfo));
         if (prepend && CLASS_LOADER_G(autoloadFunctions)->nNumOfElements > 1) {
            /* Move the newly created element to the head of the hashtable */
            HT_MOVE_TAIL_TO_HEAD(CLASS_LOADER_G(autoloadFunctions));
         }
      }

      if (UNEXPECTED(alfi.funcPtr == &EG(trampoline))) {
         zend_function *copy = reinterpret_cast<zend_function *>(emalloc(sizeof(zend_op_array)));

         memcpy(copy, alfi.funcPtr, sizeof(zend_op_array));
         alfi.funcPtr->common.function_name = nullptr;
         alfi.funcPtr = copy;
      }
      if (zend_hash_add_mem(CLASS_LOADER_G(autoloadFunctions), lc_name, &alfi, sizeof(AutoloadFuncInfo)) == nullptr) {
         if (obj_ptr && !(alfi.funcPtr->common.fn_flags & ZEND_ACC_STATIC)) {
            Z_DELREF(alfi.obj);
         }
         if (!Z_ISUNDEF(alfi.closure)) {
            Z_DELREF(alfi.closure);
         }
         if (UNEXPECTED(alfi.funcPtr->common.fn_flags & ZEND_ACC_CALL_VIA_TRAMPOLINE)) {
            zend_string_release_ex(alfi.funcPtr->common.function_name, 0);
            zend_free_trampoline(alfi.funcPtr);
         }
      }
      if (prepend && CLASS_LOADER_G(autoloadFunctions)->nNumOfElements > 1) {
         /* Move the newly created element to the head of the hashtable */
         HT_MOVE_TAIL_TO_HEAD(CLASS_LOADER_G(autoloadFunctions));
      }
skip:
      zend_string_release_ex(lc_name, 0);
   }

   if (CLASS_LOADER_G(autoloadFunctions)) {
      EG(autoload_func) = sg_autoloadCallFunc;
   } else {
      EG(autoload_func) = sg_autoloadFunc;
   }

   RETURN_TRUE;
}

PHP_FUNCTION(unregister_class_loader)
{
   zend_string *func_name = nullptr;
   char *error = nullptr;
   zend_string *lc_name;
   zval *zcallable;
   int success = FAILURE;
   zend_function *funcPtr;
   zend_object *obj_ptr;
   zend_fcall_info_cache fcc;

   if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &zcallable) == FAILURE) {
      return;
   }

   if (!zend_is_callable_ex(zcallable, nullptr, IS_CALLABLE_CHECK_SYNTAX_ONLY, &func_name, &fcc, &error)) {
      zend_throw_exception_ex(g_LogicException, 0, "Unable to unregister invalid function (%s)", error);
      if (error) {
         efree(error);
      }
      if (func_name) {
         zend_string_release_ex(func_name, 0);
      }
      RETURN_FALSE;
   }
   obj_ptr = fcc.object;
   if (error) {
      efree(error);
   }

   if (Z_TYPE_P(zcallable) == IS_OBJECT) {
      lc_name = zend_string_alloc(ZSTR_LEN(func_name) + sizeof(uint32_t), 0);
      zend_str_tolower_copy(ZSTR_VAL(lc_name), ZSTR_VAL(func_name), ZSTR_LEN(func_name));
      memcpy(ZSTR_VAL(lc_name) + ZSTR_LEN(func_name), &Z_OBJ_HANDLE_P(zcallable), sizeof(uint32_t));
      ZSTR_VAL(lc_name)[ZSTR_LEN(lc_name)] = '\0';
   } else {
      /* Skip leading \ */
      if (ZSTR_VAL(func_name)[0] == '\\') {
         lc_name = zend_string_alloc(ZSTR_LEN(func_name) - 1, 0);
         zend_str_tolower_copy(ZSTR_VAL(lc_name), ZSTR_VAL(func_name) + 1, ZSTR_LEN(func_name) - 1);
      } else {
         lc_name = zend_string_tolower(func_name);
      }
   }
   zend_string_release_ex(func_name, 0);

   if (CLASS_LOADER_G(autoloadFunctions)) {
      if (zend_string_equals(lc_name, sg_autoloadCallFunc->common.function_name)) {
         /* remove all */
         if (!CLASS_LOADER_G(autoloadRunning)) {
            zend_hash_destroy(CLASS_LOADER_G(autoloadFunctions));
            FREE_HASHTABLE(CLASS_LOADER_G(autoloadFunctions));
            CLASS_LOADER_G(autoloadFunctions) = nullptr;
            EG(autoload_func) = nullptr;
         } else {
            zend_hash_clean(CLASS_LOADER_G(autoloadFunctions));
         }
         success = SUCCESS;
      } else {
         /* remove specific */
         success = zend_hash_del(CLASS_LOADER_G(autoloadFunctions), lc_name);
         if (success != SUCCESS && obj_ptr) {
            lc_name = zend_string_extend(lc_name, ZSTR_LEN(lc_name) + sizeof(uint32_t), 0);
            memcpy(ZSTR_VAL(lc_name) + ZSTR_LEN(lc_name) - sizeof(uint32_t), &obj_ptr->handle, sizeof(uint32_t));
            ZSTR_VAL(lc_name)[ZSTR_LEN(lc_name)] = '\0';
            success = zend_hash_del(CLASS_LOADER_G(autoloadFunctions), lc_name);
         }
      }
   } else if (zend_string_equals(lc_name, sg_autoloadFunc->common.function_name)) {
      /* register single spl_autoload() */
      funcPtr = sg_autoloadFunc;

      if (EG(autoload_func) == funcPtr) {
         success = SUCCESS;
         EG(autoload_func) = nullptr;
      }
   }

   zend_string_release_ex(lc_name, 0);
   RETURN_BOOL(success == SUCCESS);
}

PHP_MINIT_FUNCTION(classloader)
{
   sg_autoloadFunc = reinterpret_cast<zend_function *>(zend_hash_str_find_ptr(CG(function_table), "default_class_loader", sizeof("default_class_loader") - 1));
   sg_autoloadCallFunc = reinterpret_cast<zend_function *>(zend_hash_str_find_ptr(CG(function_table), "load_class", sizeof("load_class") - 1));
   ZEND_ASSERT(sg_autoloadFunc != nullptr && sg_autoloadCallFunc != nullptr);
   return SUCCESS;
}

PHP_RINIT_FUNCTION(classloader)
{
   CLASS_LOADER_G(autoloadExtensions) = nullptr;
   CLASS_LOADER_G(autoloadExtensions) = nullptr;
   CLASS_LOADER_G(hashMaskInit) = false;
   return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(classloader)
{
   if (CLASS_LOADER_G(autoloadExtensions)) {
      zend_string_release_ex(CLASS_LOADER_G(autoloadExtensions), 0);
      CLASS_LOADER_G(autoloadExtensions) = nullptr;
   }
   if (CLASS_LOADER_G(autoloadFunctions)) {
      zend_hash_destroy(CLASS_LOADER_G(autoloadFunctions));
      FREE_HASHTABLE(CLASS_LOADER_G(autoloadFunctions));
      CLASS_LOADER_G(autoloadFunctions) = nullptr;
   }
   if (CLASS_LOADER_G(hashMaskInit)) {
      CLASS_LOADER_G(hashMaskInit) = false;
   }
   return SUCCESS;
}

} // runtime
} // polar
