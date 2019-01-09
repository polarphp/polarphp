// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/09.

#include "polarphp/runtime/langsupport/VariableFuncs.h"
#include "polarphp/runtime/PhpDefs.h"
#include "polarphp/runtime/Output.h"
#include "polarphp/runtime/ExecEnv.h"

#define COMMON (is_ref ? "&" : "")

namespace polar {
namespace runtime {

namespace {
void array_element_dump(zval *zv, zend_ulong index, zend_string *key, int level)
{
   if (key == NULL) { /* numeric key */
      php_printf("%*c[" ZEND_LONG_FMT "]=>\n", level + 1, ' ', index);
   } else { /* string key */
      php_printf("%*c[\"", level + 1, ' ');
      PHPWRITE(ZSTR_VAL(key), ZSTR_LEN(key));
      php_printf("\"]=>\n");
   }
   var_dump(zv, level + 2);
}

void object_property_dump(zval *zv, zend_ulong index, zend_string *key, int level) /* {{{ */
{
   const char *prop_name, *class_name;
   if (key == NULL) { /* numeric key */
      php_printf("%*c[" ZEND_LONG_FMT "]=>\n", level + 1, ' ', index);
   } else { /* string key */
      int unmangle = zend_unmangle_property_name(key, &class_name, &prop_name);
      php_printf("%*c[", level + 1, ' ');
      if (class_name && unmangle == SUCCESS) {
         if (class_name[0] == '*') {
            php_printf("\"%s\":protected", prop_name);
         } else {
            php_printf("\"%s\":\"%s\":private", prop_name, class_name);
         }
      } else {
         php_printf("\"");
         PHPWRITE(ZSTR_VAL(key), ZSTR_LEN(key));
         php_printf("\"");
      }
      ZEND_PUTS("]=>\n");
   }
   var_dump(zv, level + 2);
}

} // anonymous namespace

void var_dump(zval *struc, int level) /* {{{ */
{
   HashTable *myht;
   zend_string *class_name;
   int is_temp;
   int is_ref = 0;
   zend_ulong num;
   zend_string *key;
   zval *val;
   uint32_t count;

   if (level > 1) {
      php_printf("%*c", level - 1, ' ');
   }

again:
   switch (Z_TYPE_P(struc)) {
   case IS_FALSE:
      php_printf("%sbool(false)\n", COMMON);
      break;
   case IS_TRUE:
      php_printf("%sbool(true)\n", COMMON);
      break;
   case IS_NULL:
      php_printf("%sNULL\n", COMMON);
      break;
   case IS_LONG:
      php_printf("%sint(" ZEND_LONG_FMT ")\n", COMMON, Z_LVAL_P(struc));
      break;
   case IS_DOUBLE:
      php_printf("%sfloat(%.*G)\n", COMMON, (int) EG(precision), Z_DVAL_P(struc));
      break;
   case IS_STRING:
      php_printf("%sstring(%zd) \"", COMMON, Z_STRLEN_P(struc));
      PHPWRITE(Z_STRVAL_P(struc), Z_STRLEN_P(struc));
      PUTS("\"\n");
      break;
   case IS_ARRAY:
      myht = Z_ARRVAL_P(struc);
      if (level > 1 && !(GC_FLAGS(myht) & GC_IMMUTABLE)) {
         if (GC_IS_RECURSIVE(myht)) {
            PUTS("*RECURSION*\n");
            return;
         }
         GC_PROTECT_RECURSION(myht);
      }
      count = zend_array_count(myht);
      php_printf("%sarray(%d) {\n", COMMON, count);

      ZEND_HASH_FOREACH_KEY_VAL_IND(myht, num, key, val) {
         array_element_dump(val, num, key, level);
      } ZEND_HASH_FOREACH_END();
      if (level > 1 && !(GC_FLAGS(myht) & GC_IMMUTABLE)) {
         GC_UNPROTECT_RECURSION(myht);
      }
      if (level > 1) {
         php_printf("%*c", level-1, ' ');
      }
      PUTS("}\n");
      break;
   case IS_OBJECT:
      if (Z_IS_RECURSIVE_P(struc)) {
         PUTS("*RECURSION*\n");
         return;
      }
      Z_PROTECT_RECURSION_P(struc);

      myht = Z_OBJDEBUG_P(struc, is_temp);
      class_name = Z_OBJ_HANDLER_P(struc, get_class_name)(Z_OBJ_P(struc));
      php_printf("%sobject(%s)#%d (%d) {\n", COMMON, ZSTR_VAL(class_name), Z_OBJ_HANDLE_P(struc), myht ? zend_array_count(myht) : 0);
      zend_string_release_ex(class_name, 0);

      if (myht) {
         zend_ulong num;
         zend_string *key;
         zval *val;

         ZEND_HASH_FOREACH_KEY_VAL_IND(myht, num, key, val) {
            object_property_dump(val, num, key, level);
         } ZEND_HASH_FOREACH_END();
         if (is_temp) {
            zend_hash_destroy(myht);
            efree(myht);
         }
      }
      if (level > 1) {
         php_printf("%*c", level-1, ' ');
      }
      PUTS("}\n");
      Z_UNPROTECT_RECURSION_P(struc);
      break;
   case IS_RESOURCE: {
      const char *type_name = zend_rsrc_list_get_rsrc_type(Z_RES_P(struc));
      php_printf("%sresource(%d) of type (%s)\n", COMMON, Z_RES_P(struc)->handle, type_name ? type_name : "Unknown");
      break;
   }
   case IS_REFERENCE:
      //??? hide references with refcount==1 (for compatibility)
      if (Z_REFCOUNT_P(struc) > 1) {
         is_ref = 1;
      }
      struc = Z_REFVAL_P(struc);
      goto again;
      break;
   default:
      php_printf("%sUNKNOWN:0\n", COMMON);
      break;
   }
}

PHP_FUNCTION(var_dump)
{
   zval *args;
   int argc;
   int	i;
   ZEND_PARSE_PARAMETERS_START(1, -1)
         Z_PARAM_VARIADIC('+', args, argc)
         ZEND_PARSE_PARAMETERS_END();
   for (i = 0; i < argc; i++) {
      var_dump(&args[i], 1);
   }
}

namespace {
void zval_array_element_dump(zval *zv, zend_ulong index, zend_string *key, int level) /* {{{ */
{
   if (key == NULL) { /* numeric key */
      php_printf("%*c[" ZEND_LONG_FMT "]=>\n", level + 1, ' ', index);
   } else { /* string key */
      php_printf("%*c[\"", level + 1, ' ');
      PHPWRITE(ZSTR_VAL(key), ZSTR_LEN(key));
      php_printf("\"]=>\n");
   }
   debug_zval_dump(zv, level + 2);
}

void zval_object_property_dump(zval *zv, zend_ulong index, zend_string *key, int level) /* {{{ */
{
   const char *prop_name, *class_name;
   if (key == NULL) { /* numeric key */
      php_printf("%*c[" ZEND_LONG_FMT "]=>\n", level + 1, ' ', index);
   } else { /* string key */
      zend_unmangle_property_name(key, &class_name, &prop_name);
      php_printf("%*c[", level + 1, ' ');
      if (class_name) {
         if (class_name[0] == '*') {
            php_printf("\"%s\":protected", prop_name);
         } else {
            php_printf("\"%s\":\"%s\":private", prop_name, class_name);
         }
      } else {
         php_printf("\"%s\"", prop_name);
      }
      ZEND_PUTS("]=>\n");
   }
   debug_zval_dump(zv, level + 2);
}

} // anonymous namespace

void debug_zval_dump(zval *struc, int level) /* {{{ */
{
   HashTable *myht = NULL;
   zend_string *class_name;
   int is_temp = 0;
   int is_ref = 0;
   zend_ulong index;
   zend_string *key;
   zval *val;
   uint32_t count;

   if (level > 1) {
      php_printf("%*c", level - 1, ' ');
   }

again:
   switch (Z_TYPE_P(struc)) {
   case IS_FALSE:
      php_printf("%sbool(false)\n", COMMON);
      break;
   case IS_TRUE:
      php_printf("%sbool(true)\n", COMMON);
      break;
   case IS_NULL:
      php_printf("%sNULL\n", COMMON);
      break;
   case IS_LONG:
      php_printf("%sint(" ZEND_LONG_FMT ")\n", COMMON, Z_LVAL_P(struc));
      break;
   case IS_DOUBLE:
      php_printf("%sfloat(%.*G)\n", COMMON, (int) EG(precision), Z_DVAL_P(struc));
      break;
   case IS_STRING:
      php_printf("%sstring(%zd) \"", COMMON, Z_STRLEN_P(struc));
      PHPWRITE(Z_STRVAL_P(struc), Z_STRLEN_P(struc));
      php_printf("\" refcount(%u)\n", Z_REFCOUNTED_P(struc) ? Z_REFCOUNT_P(struc) : 1);
      break;
   case IS_ARRAY:
      myht = Z_ARRVAL_P(struc);
      if (level > 1 && !(GC_FLAGS(myht) & GC_IMMUTABLE)) {
         if (GC_IS_RECURSIVE(myht)) {
            PUTS("*RECURSION*\n");
            return;
         }
         GC_PROTECT_RECURSION(myht);
      }
      count = zend_array_count(myht);
      php_printf("%sarray(%d) refcount(%u){\n", COMMON, count, Z_REFCOUNTED_P(struc) ? Z_REFCOUNT_P(struc) : 1);
      ZEND_HASH_FOREACH_KEY_VAL_IND(myht, index, key, val) {
         zval_array_element_dump(val, index, key, level);
      } ZEND_HASH_FOREACH_END();
      if (level > 1 && !(GC_FLAGS(myht) & GC_IMMUTABLE)) {
         GC_UNPROTECT_RECURSION(myht);
      }
      if (is_temp) {
         zend_hash_destroy(myht);
         efree(myht);
      }
      if (level > 1) {
         php_printf("%*c", level - 1, ' ');
      }
      PUTS("}\n");
      break;
   case IS_OBJECT:
      myht = Z_OBJDEBUG_P(struc, is_temp);
      if (myht) {
         if (GC_IS_RECURSIVE(myht)) {
            PUTS("*RECURSION*\n");
            return;
         }
         GC_PROTECT_RECURSION(myht);
      }
      class_name = Z_OBJ_HANDLER_P(struc, get_class_name)(Z_OBJ_P(struc));
      php_printf("%sobject(%s)#%d (%d) refcount(%u){\n", COMMON, ZSTR_VAL(class_name), Z_OBJ_HANDLE_P(struc), myht ? zend_array_count(myht) : 0, Z_REFCOUNT_P(struc));
      zend_string_release_ex(class_name, 0);
      if (myht) {
         ZEND_HASH_FOREACH_KEY_VAL_IND(myht, index, key, val) {
            zval_object_property_dump(val, index, key, level);
         } ZEND_HASH_FOREACH_END();
         GC_UNPROTECT_RECURSION(myht);
         if (is_temp) {
            zend_hash_destroy(myht);
            efree(myht);
         }
      }
      if (level > 1) {
         php_printf("%*c", level - 1, ' ');
      }
      PUTS("}\n");
      break;
   case IS_RESOURCE: {
      const char *type_name = zend_rsrc_list_get_rsrc_type(Z_RES_P(struc));
      php_printf("%sresource(%d) of type (%s) refcount(%u)\n", COMMON, Z_RES_P(struc)->handle, type_name ? type_name : "Unknown", Z_REFCOUNT_P(struc));
      break;
   }
   case IS_REFERENCE:
      //??? hide references with refcount==1 (for compatibility)
      if (Z_REFCOUNT_P(struc) > 1) {
         is_ref = 1;
      }
      struc = Z_REFVAL_P(struc);
      goto again;
   default:
      php_printf("%sUNKNOWN:0\n", COMMON);
      break;
   }
}

PHP_FUNCTION(debug_zval_dump)
{
   zval *args;
   int argc;
   int	i;
   ZEND_PARSE_PARAMETERS_START(1, -1)
         Z_PARAM_VARIADIC('+', args, argc)
   ZEND_PARSE_PARAMETERS_END();
   for (i = 0; i < argc; i++) {
      debug_zval_dump(&args[i], 1);
   }
}

} // runtime
} // polar
