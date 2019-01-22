// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/01/20.

#include "polarphp/runtime/langsupport/SerializeFuncs.h"
#include "polarphp/runtime/langsupport/IncompeleteClass.h"
#include "polarphp/runtime/langsupport/LangSupportFuncs.h"
#include "polarphp/runtime/ExecEnv.h"
#include "polarphp/runtime/Snprintf.h"

namespace polar {
namespace runtime {

namespace {
void var_serialize_intern(smart_str *buf, zval *struc, SerializeData *var_hash);

inline zend_long add_var_hash(SerializeData *data, zval *var)
{
   zval *zv;
   zend_ulong key;
   zend_bool is_ref = Z_ISREF_P(var);

   data->n += 1;

   if (!is_ref && Z_TYPE_P(var) != IS_OBJECT) {
      return 0;
   }

   /* References to objects are treated as if the reference didn't exist */
   if (is_ref && Z_TYPE_P(Z_REFVAL_P(var)) == IS_OBJECT) {
      var = Z_REFVAL_P(var);
   }

   /* Index for the variable is stored using the numeric value of the pointer to
    * the zend_refcounted struct */
   key = (zend_ulong) (zend_uintptr_t) Z_COUNTED_P(var);
   zv = zend_hash_index_find(&data->ht, key);

   if (zv) {
      /* References are only counted once, undo the data->n increment above */
      if (is_ref) {
         data->n -= 1;
      }

      return Z_LVAL_P(zv);
   } else {
      zval zv_n;
      ZVAL_LONG(&zv_n, data->n);
      zend_hash_index_add_new(&data->ht, key, &zv_n);

      /* Additionally to the index, we also store the variable, to ensure that it is
       * not destroyed during serialization and its pointer reused. The variable is
       * stored at the numeric value of the pointer + 1, which cannot be the location
       * of another zend_refcounted structure. */
      zend_hash_index_add_new(&data->ht, key + 1, var);
      Z_ADDREF_P(var);

      return 0;
   }
}

inline void var_serialize_long(smart_str *buf, zend_long val)
{
   smart_str_appendl(buf, "i:", 2);
   smart_str_append_long(buf, val);
   smart_str_appendc(buf, ';');
}

inline void var_serialize_string(smart_str *buf, char *str, size_t len)
{
   smart_str_appendl(buf, "s:", 2);
   smart_str_append_unsigned(buf, len);
   smart_str_appendl(buf, ":\"", 2);
   smart_str_appendl(buf, str, len);
   smart_str_appendl(buf, "\";", 2);
}

inline zend_bool var_serialize_class_name(smart_str *buf, zval *struc)
{
   PHP_CLASS_ATTRIBUTES;
   PHP_SET_CLASS_ATTRIBUTES(struc);
   smart_str_appendl(buf, "O:", 2);
   smart_str_append_unsigned(buf, ZSTR_LEN(class_name));
   smart_str_appendl(buf, ":\"", 2);
   smart_str_append(buf, class_name);
   smart_str_appendl(buf, "\":", 2);
   PHP_CLEANUP_CLASS_ATTRIBUTES();
   return incomplete_class;
}

int var_serialize_call_sleep(zval *retval, zval *struc)
{
   zval fname;
   int res;

   ZVAL_STRINGL(&fname, "__sleep", sizeof("__sleep") - 1);
   retrieve_runtime_module_data().serializeLock++;
   res = call_user_function(CG(function_table), struc, &fname, retval, 0, 0);
   retrieve_runtime_module_data().serializeLock--;
   zval_ptr_dtor_str(&fname);

   if (res == FAILURE || Z_ISUNDEF_P(retval)) {
      zval_ptr_dtor(retval);
      return FAILURE;
   }

   if (!HASH_OF(retval)) {
      zval_ptr_dtor(retval);
      php_error_docref(nullptr, E_NOTICE, "__sleep should return an array only containing the names of instance-variables to serialize");
      return FAILURE;
   }

   return SUCCESS;
}

void var_serialize_collect_names(HashTable *ht, HashTable *src)
{
   zval *val;
   zend_string *name, *tmp_name;

   zend_hash_init(ht, zend_hash_num_elements(src), nullptr, nullptr, 0);
   ZEND_HASH_FOREACH_VAL(src, val) {
      if (Z_TYPE_P(val) != IS_STRING) {
         php_error_docref(nullptr, E_NOTICE,
                          "__sleep should return an array only containing the names of instance-variables to serialize.");
      }

      name = zval_get_tmp_string(val, &tmp_name);
      if (zend_hash_exists(ht, name)) {
         php_error_docref(nullptr, E_NOTICE,
                          "\"%s\" is returned from __sleep multiple times", ZSTR_VAL(name));
         zend_tmp_string_release(tmp_name);
         continue;
      }
      zend_hash_add_empty_element(ht, name);
      zend_tmp_string_release(tmp_name);
   } ZEND_HASH_FOREACH_END();
}

void var_serialize_class(smart_str *buf, zval *struc, zval *retval_ptr, SerializeData *var_hash)
{
   zend_class_entry *ce = Z_OBJCE_P(struc);
   HashTable names, *propers;
   zval nval;
   zend_string *name;

   var_serialize_class_name(buf, struc);
   var_serialize_collect_names(&names, HASH_OF(retval_ptr));

   smart_str_append_unsigned(buf, zend_hash_num_elements(&names));
   smart_str_appendl(buf, ":{", 2);

   ZVAL_NULL(&nval);
   propers = Z_OBJPROP_P(struc);

   ZEND_HASH_FOREACH_STR_KEY(&names, name) {
      zend_string *prot_name, *priv_name;

      zval *val = zend_hash_find_ex(propers, name, 1);
      if (val != nullptr) {
         if (Z_TYPE_P(val) == IS_INDIRECT) {
            val = Z_INDIRECT_P(val);
            if (Z_TYPE_P(val) == IS_UNDEF) {
               goto undef_prop;
            }
         }

         var_serialize_string(buf, ZSTR_VAL(name), ZSTR_LEN(name));
         var_serialize_intern(buf, val, var_hash);
         continue;
      }

      priv_name = zend_mangle_property_name(
               ZSTR_VAL(ce->name), ZSTR_LEN(ce->name), ZSTR_VAL(name), ZSTR_LEN(name), 0);
      val = zend_hash_find(propers, priv_name);
      if (val != nullptr) {
         if (Z_TYPE_P(val) == IS_INDIRECT) {
            val = Z_INDIRECT_P(val);
            if (Z_ISUNDEF_P(val)) {
               zend_string_free(priv_name);
               goto undef_prop;
            }
         }

         var_serialize_string(buf, ZSTR_VAL(priv_name), ZSTR_LEN(priv_name));
         zend_string_free(priv_name);
         var_serialize_intern(buf, val, var_hash);
         continue;
      }
      zend_string_free(priv_name);

      prot_name = zend_mangle_property_name(
               "*", 1, ZSTR_VAL(name), ZSTR_LEN(name), 0);
      val = zend_hash_find(propers, prot_name);
      if (val != nullptr) {
         if (Z_TYPE_P(val) == IS_INDIRECT) {
            val = Z_INDIRECT_P(val);
            if (Z_TYPE_P(val) == IS_UNDEF) {
               zend_string_free(prot_name);
               goto undef_prop;
            }
         }

         var_serialize_string(buf, ZSTR_VAL(prot_name), ZSTR_LEN(prot_name));
         zend_string_free(prot_name);
         var_serialize_intern(buf, val, var_hash);
         continue;
      }
      zend_string_free(prot_name);

undef_prop:
      var_serialize_string(buf, ZSTR_VAL(name), ZSTR_LEN(name));
      var_serialize_intern(buf, &nval, var_hash);
      php_error_docref(nullptr, E_NOTICE,
                       "\"%s\" returned as member variable from __sleep() but does not exist", ZSTR_VAL(name));
   } ZEND_HASH_FOREACH_END();
   smart_str_appendc(buf, '}');

   zend_hash_destroy(&names);
}

void var_serialize_intern(smart_str *buf, zval *struc, SerializeData *var_hash)
{
   zend_long var_already;
   HashTable *myht;

   if (EG(exception)) {
      return;
   }

   if (var_hash && (var_already = add_var_hash(var_hash, struc))) {
      if (Z_ISREF_P(struc)) {
         smart_str_appendl(buf, "R:", 2);
         smart_str_append_long(buf, var_already);
         smart_str_appendc(buf, ';');
         return;
      } else if (Z_TYPE_P(struc) == IS_OBJECT) {
         smart_str_appendl(buf, "r:", 2);
         smart_str_append_long(buf, var_already);
         smart_str_appendc(buf, ';');
         return;
      }
   }

again:
   switch (Z_TYPE_P(struc)) {
   case IS_FALSE:
      smart_str_appendl(buf, "b:0;", 4);
      return;

   case IS_TRUE:
      smart_str_appendl(buf, "b:1;", 4);
      return;

   case IS_NULL:
      smart_str_appendl(buf, "N;", 2);
      return;

   case IS_LONG:
      var_serialize_long(buf, Z_LVAL_P(struc));
      return;

   case IS_DOUBLE: {
      char tmp_str[PHP_DOUBLE_MAX_LENGTH];
      smart_str_appendl(buf, "d:", 2);
      php_gcvt(Z_DVAL_P(struc), (int)retrieve_global_execenv_runtime_info().serializePrecision, '.', 'E', tmp_str);
      smart_str_appends(buf, tmp_str);
      smart_str_appendc(buf, ';');
      return;
   }

   case IS_STRING:
      var_serialize_string(buf, Z_STRVAL_P(struc), Z_STRLEN_P(struc));
      return;

   case IS_OBJECT: {
      zend_class_entry *ce = Z_OBJCE_P(struc);

      if (ce->serialize != nullptr) {
         /* has custom handler */
         unsigned char *serialized_data = nullptr;
         size_t serialized_length;

         if (ce->serialize(struc, &serialized_data, &serialized_length, (zend_serialize_data *)var_hash) == SUCCESS) {
            smart_str_appendl(buf, "C:", 2);
            smart_str_append_unsigned(buf, ZSTR_LEN(Z_OBJCE_P(struc)->name));
            smart_str_appendl(buf, ":\"", 2);
            smart_str_append(buf, Z_OBJCE_P(struc)->name);
            smart_str_appendl(buf, "\":", 2);

            smart_str_append_unsigned(buf, serialized_length);
            smart_str_appendl(buf, ":{", 2);
            smart_str_appendl(buf, (char *) serialized_data, serialized_length);
            smart_str_appendc(buf, '}');
         } else {
            smart_str_appendl(buf, "N;", 2);
         }
         if (serialized_data) {
            efree(serialized_data);
         }
         return;
      }

      if (ce != PHP_IC_ENTRY && zend_hash_str_exists(&ce->function_table, "__sleep", sizeof("__sleep")-1)) {
         zval retval, tmp;
         ZVAL_COPY(&tmp, struc);

         if (var_serialize_call_sleep(&retval, &tmp) == FAILURE) {
            if (!EG(exception)) {
               /* we should still add element even if it's not OK,
                      * since we already wrote the length of the array before */
               smart_str_appendl(buf, "N;", 2);
            }
            zval_ptr_dtor(&tmp);
            return;
         }

         var_serialize_class(buf, &tmp, &retval, var_hash);
         zval_ptr_dtor(&retval);
         zval_ptr_dtor(&tmp);
         return;
      }
      POLAR_FALLTHROUGH;
      /* fall-through */
   }
   case IS_ARRAY: {
      uint32_t i;
      zend_bool incomplete_class = 0;
      if (Z_TYPE_P(struc) == IS_ARRAY) {
         smart_str_appendl(buf, "a:", 2);
         myht = Z_ARRVAL_P(struc);
         i = zend_array_count(myht);
      } else {
         incomplete_class = var_serialize_class_name(buf, struc);
         myht = Z_OBJPROP_P(struc);
         /* count after serializing name, since php_var_serialize_class_name
             * changes the count if the variable is incomplete class */
         i = zend_array_count(myht);
         if (i > 0 && incomplete_class) {
            --i;
         }
      }
      smart_str_append_unsigned(buf, i);
      smart_str_appendl(buf, ":{", 2);
      if (i > 0) {
         zend_string *key;
         zval *data;
         zend_ulong index;

         ZEND_HASH_FOREACH_KEY_VAL_IND(myht, index, key, data) {

            if (incomplete_class && strcmp(ZSTR_VAL(key), MAGIC_MEMBER) == 0) {
               continue;
            }

            if (!key) {
               var_serialize_long(buf, index);
            } else {
               var_serialize_string(buf, ZSTR_VAL(key), ZSTR_LEN(key));
            }

            if (Z_ISREF_P(data) && Z_REFCOUNT_P(data) == 1) {
               data = Z_REFVAL_P(data);
            }

            /* we should still add element even if it's not OK,
                * since we already wrote the length of the array before */
            if (Z_TYPE_P(data) == IS_ARRAY) {
               if (UNEXPECTED(Z_IS_RECURSIVE_P(data))
                   || UNEXPECTED(Z_TYPE_P(struc) == IS_ARRAY && Z_ARR_P(data) == Z_ARR_P(struc))) {
                  add_var_hash(var_hash, struc);
                  smart_str_appendl(buf, "N;", 2);
               } else {
                  if (Z_REFCOUNTED_P(data)) {
                     Z_PROTECT_RECURSION_P(data);
                  }
                  var_serialize_intern(buf, data, var_hash);
                  if (Z_REFCOUNTED_P(data)) {
                     Z_UNPROTECT_RECURSION_P(data);
                  }
               }
            } else {
               var_serialize_intern(buf, data, var_hash);
            }
         } ZEND_HASH_FOREACH_END();
      }
      smart_str_appendc(buf, '}');
      return;
   }
   case IS_REFERENCE:
      struc = Z_REFVAL_P(struc);
      goto again;
   default:
      smart_str_appendl(buf, "i:0;", 4);
      return;
   }
}

} // anonymous namespace

void var_serialize(smart_str *buf, zval *struc, SerializeData **data)
{
   var_serialize_intern(buf, struc, *data);
   smart_str_0(buf);
}

SerializeData *var_serialize_init()
{
   SerializeData *d;
   RuntimeModuleData &rtData = retrieve_runtime_module_data();
   /* fprintf(stderr, "SERIALIZE_INIT      == lock: %u, level: %u\n", rtData.serializeLock, rtData.serialize.level); */
   if (rtData.serializeLock || !rtData.serialize.level) {
      d = reinterpret_cast<SerializeData *>(emalloc(sizeof(SerializeData)));
      zend_hash_init(&d->ht, 16, nullptr, ZVAL_PTR_DTOR, 0);
      d->n = 0;
      if (!rtData.serializeLock) {
         rtData.serialize.data = d;
         rtData.serialize.level = 1;
      }
   } else {
      d = rtData.serialize.data;
      ++rtData.serialize.level;
   }
   return d;
}

void var_serialize_destroy(SerializeData *d)
{
   RuntimeModuleData &rtData = retrieve_runtime_module_data();
   /* fprintf(stderr, "SERIALIZE_DESTROY   == lock: %u, level: %u\n", rtData.serializeLock, rtData.serialize.level); */
   if (rtData.serializeLock || rtData.serialize.level == 1) {
      zend_hash_destroy(&d->ht);
      efree(d);
   }
   if (!rtData.serializeLock && !--rtData.serialize.level) {
      rtData.serialize.data = nullptr;
   }
}

PHP_FUNCTION(serialize)
{
   zval *struc;
   SerializeData *var_hash;
   smart_str buf = {0};

   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(struc)
         ZEND_PARSE_PARAMETERS_END();

   PHP_VAR_SERIALIZE_INIT(var_hash);
   var_serialize(&buf, struc, &var_hash);
   PHP_VAR_SERIALIZE_DESTROY(var_hash);

   if (EG(exception)) {
      smart_str_free(&buf);
      RETURN_FALSE;
   }

   if (buf.s) {
      RETURN_NEW_STR(buf.s);
   } else {
      RETURN_NULL();
   }
}

PHP_FUNCTION(unserialize)
{
   char *buf = nullptr;
   size_t buf_len;
   const unsigned char *p;
   UnserializeData *var_hash;
   zval *options = nullptr, *classes = nullptr;
   zval *retval;
   HashTable *class_hash = nullptr, *prev_class_hash;

   ZEND_PARSE_PARAMETERS_START(1, 2)
         Z_PARAM_STRING(buf, buf_len)
         Z_PARAM_OPTIONAL
         Z_PARAM_ARRAY(options)
         ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

   if (buf_len == 0) {
      RETURN_FALSE;
   }

   p = (const unsigned char*) buf;
   PHP_VAR_UNSERIALIZE_INIT(var_hash);

   prev_class_hash = var_unserialize_get_allowed_classes(var_hash);
   if (options != nullptr) {
      classes = zend_hash_str_find(Z_ARRVAL_P(options), "allowed_classes", sizeof("allowed_classes")-1);
      if (classes && Z_TYPE_P(classes) != IS_ARRAY && Z_TYPE_P(classes) != IS_TRUE && Z_TYPE_P(classes) != IS_FALSE) {
         php_error_docref(nullptr, E_WARNING, "allowed_classes option should be array or boolean");
         PHP_VAR_UNSERIALIZE_DESTROY(var_hash);
         RETURN_FALSE;
      }

      if(classes && (Z_TYPE_P(classes) == IS_ARRAY || !zend_is_true(classes))) {
         ALLOC_HASHTABLE(class_hash);
         zend_hash_init(class_hash, (Z_TYPE_P(classes) == IS_ARRAY)?zend_hash_num_elements(Z_ARRVAL_P(classes)):0, nullptr, nullptr, 0);
      }
      if(class_hash && Z_TYPE_P(classes) == IS_ARRAY) {
         zval *entry;
         zend_string *lcname;

         ZEND_HASH_FOREACH_VAL(Z_ARRVAL_P(classes), entry) {
            convert_to_string_ex(entry);
            lcname = zend_string_tolower(Z_STR_P(entry));
            zend_hash_add_empty_element(class_hash, lcname);
            zend_string_release_ex(lcname, 0);
         } ZEND_HASH_FOREACH_END();
      }
      var_unserialize_set_allowed_classes(var_hash, class_hash);
   }

   retval = var_tmp_var(&var_hash);
   if (!var_unserialize(retval, &p, p + buf_len, &var_hash)) {
      if (!EG(exception)) {
         php_error_docref(nullptr, E_NOTICE, "Error at offset " ZEND_LONG_FMT " of %zd bytes",
                          (zend_long)(const_cast<char *>(reinterpret_cast<const char *>(p)) - buf), buf_len);
      }
      RETVAL_FALSE;
   } else {
      ZVAL_COPY(return_value, retval);
   }

   if (class_hash) {
      zend_hash_destroy(class_hash);
      FREE_HASHTABLE(class_hash);
   }

   /* Reset to previous allowed_classes in case this is a nested call */
   var_unserialize_set_allowed_classes(var_hash, prev_class_hash);
   PHP_VAR_UNSERIALIZE_DESTROY(var_hash);

   /* Per calling convention we must not return a reference here, so unwrap. We're doing this at
    * the very end, because __wakeup() calls performed during UNSERIALIZE_DESTROY might affect
    * the value we unwrap here. This is compatible with behavior in PHP <=7.0. */
   if (Z_ISREF_P(return_value)) {
      zend_unwrap_reference(return_value);
   }
}

} // runtime
} // polar
