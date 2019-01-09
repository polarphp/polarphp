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

#include "polarphp/runtime/langsupport/TypeFuncs.h"

namespace polar {
namespace runtime {

PHP_FUNCTION(gettype)
{
   zval *arg;
   zend_string *type;

   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(arg)
   ZEND_PARSE_PARAMETERS_END();

   type = zend_zval_get_type(arg);
   if (EXPECTED(type)) {
      RETURN_INTERNED_STR(type);
   } else {
      RETURN_STRING("unknown type");
   }
}

PHP_FUNCTION(settype)
{
   zval *var;
   char *type;
   size_t type_len = 0;

   ZEND_PARSE_PARAMETERS_START(2, 2)
         Z_PARAM_ZVAL_DEREF(var)
         Z_PARAM_STRING(type, type_len)
   ZEND_PARSE_PARAMETERS_END();

   if (!strcasecmp(type, "integer")) {
      convert_to_long(var);
   } else if (!strcasecmp(type, "int")) {
      convert_to_long(var);
   } else if (!strcasecmp(type, "float")) {
      convert_to_double(var);
   } else if (!strcasecmp(type, "double")) { /* deprecated */
      convert_to_double(var);
   } else if (!strcasecmp(type, "string")) {
      convert_to_string(var);
   } else if (!strcasecmp(type, "array")) {
      convert_to_array(var);
   } else if (!strcasecmp(type, "object")) {
      convert_to_object(var);
   } else if (!strcasecmp(type, "bool")) {
      convert_to_boolean(var);
   } else if (!strcasecmp(type, "boolean")) {
      convert_to_boolean(var);
   } else if (!strcasecmp(type, "null")) {
      convert_to_null(var);
   } else if (!strcasecmp(type, "resource")) {
      php_error_docref(NULL, E_WARNING, "Cannot convert to resource type");
      RETURN_FALSE;
   } else {
      php_error_docref(NULL, E_WARNING, "Invalid type");
      RETURN_FALSE;
   }
   RETVAL_TRUE;
}

PHP_FUNCTION(intval)
{
   zval *num;
   zend_long base = 10;
   ZEND_PARSE_PARAMETERS_START(1, 2)
         Z_PARAM_ZVAL(num)
         Z_PARAM_OPTIONAL
         Z_PARAM_LONG(base)
   ZEND_PARSE_PARAMETERS_END();

   if (Z_TYPE_P(num) != IS_STRING || base == 10) {
      RETVAL_LONG(zval_get_long(num));
      return;
   }
   if (base == 0 || base == 2) {
      char *strval = Z_STRVAL_P(num);
      size_t strlen = Z_STRLEN_P(num);
      while (isspace(*strval) && strlen) {
         strval++;
         strlen--;
      }
      /* Length of 3+ covers "0b#" and "-0b" (which results in 0) */
      if (strlen > 2) {
         int offset = 0;
         if (strval[0] == '-' || strval[0] == '+') {
            offset = 1;
         }
         if (strval[offset] == '0' && (strval[offset + 1] == 'b' || strval[offset + 1] == 'B')) {
            char *tmpval;
            strlen -= 2; /* Removing "0b" */
            tmpval = reinterpret_cast<char *>(emalloc(strlen + 1));
            /* Place the unary symbol at pos 0 if there was one */
            if (offset) {
               tmpval[0] = strval[0];
            }
            /* Copy the data from after "0b" to the end of the buffer */
            memcpy(tmpval + offset, strval + offset + 2, strlen - offset);
            tmpval[strlen] = 0;
            RETVAL_LONG(ZEND_STRTOL(tmpval, NULL, 2));
            efree(tmpval);
            return;
         }
      }
   }

   RETVAL_LONG(ZEND_STRTOL(Z_STRVAL_P(num), NULL, base));
}

PHP_FUNCTION(floatval)
{
   zval *num;
   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(num)
   ZEND_PARSE_PARAMETERS_END();

   RETURN_DOUBLE(zval_get_double(num));
}

PHP_FUNCTION(boolval)
{
   zval *val;

   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(val)
   ZEND_PARSE_PARAMETERS_END();

   RETURN_BOOL(zend_is_true(val));
}

PHP_FUNCTION(strval)
{
   zval *num;

   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(num)
   ZEND_PARSE_PARAMETERS_END();

   RETVAL_STR(zval_get_string(num));
}

namespace {

inline void php_is_type(INTERNAL_FUNCTION_PARAMETERS, int type)
{
   zval *arg;

   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(arg)
   ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

   if (Z_TYPE_P(arg) == type) {
      if (type == IS_RESOURCE) {
         const char *type_name = zend_rsrc_list_get_rsrc_type(Z_RES_P(arg));
         if (!type_name) {
            RETURN_FALSE;
         }
      }
      RETURN_TRUE;
   } else {
      RETURN_FALSE;
   }
}

} // anonymous namespace

PHP_FUNCTION(is_null)
{
   php_is_type(INTERNAL_FUNCTION_PARAM_PASSTHRU, IS_NULL);
}

PHP_FUNCTION(is_resource)
{
   php_is_type(INTERNAL_FUNCTION_PARAM_PASSTHRU, IS_RESOURCE);
}

PHP_FUNCTION(is_bool)
{
   zval *arg;

   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(arg)
   ZEND_PARSE_PARAMETERS_END_EX(RETURN_FALSE);

   RETURN_BOOL(Z_TYPE_P(arg) == IS_FALSE || Z_TYPE_P(arg) == IS_TRUE);
}

PHP_FUNCTION(is_int)
{
   php_is_type(INTERNAL_FUNCTION_PARAM_PASSTHRU, IS_LONG);
}

PHP_FUNCTION(is_float)
{
   php_is_type(INTERNAL_FUNCTION_PARAM_PASSTHRU, IS_DOUBLE);
}

PHP_FUNCTION(is_string)
{
   php_is_type(INTERNAL_FUNCTION_PARAM_PASSTHRU, IS_STRING);
}

PHP_FUNCTION(is_array)
{
   php_is_type(INTERNAL_FUNCTION_PARAM_PASSTHRU, IS_ARRAY);
}

PHP_FUNCTION(is_object)
{
   php_is_type(INTERNAL_FUNCTION_PARAM_PASSTHRU, IS_OBJECT);
}

PHP_FUNCTION(is_numeric)
{
   zval *arg;

   ZEND_PARSE_PARAMETERS_START(1, 1)
         Z_PARAM_ZVAL(arg)
   ZEND_PARSE_PARAMETERS_END();

   switch (Z_TYPE_P(arg)) {
   case IS_LONG:
   case IS_DOUBLE:
      RETURN_TRUE;
      break;

   case IS_STRING:
      if (is_numeric_string(Z_STRVAL_P(arg), Z_STRLEN_P(arg), NULL, NULL, 0)) {
         RETURN_TRUE;
      } else {
         RETURN_FALSE;
      }
      break;

   default:
      RETURN_FALSE;
      break;
   }
}

PHP_FUNCTION(is_scalar)
{
   zval *arg;
   ZEND_PARSE_PARAMETERS_START(1, 1)
      Z_PARAM_ZVAL(arg)
   ZEND_PARSE_PARAMETERS_END();
   switch (Z_TYPE_P(arg)) {
      case IS_FALSE:
      case IS_TRUE:
      case IS_DOUBLE:
      case IS_LONG:
      case IS_STRING:
         RETURN_TRUE;
         break;

      default:
         RETURN_FALSE;
         break;
   }
}

PHP_FUNCTION(is_callable)
{
   zval *var, *callable_name = NULL;
   zend_string *name;
   char *error;
   zend_bool retval;
   zend_bool syntax_only = 0;
   int check_flags = 0;

   ZEND_PARSE_PARAMETERS_START(1, 3)
      Z_PARAM_ZVAL(var)
      Z_PARAM_OPTIONAL
      Z_PARAM_BOOL(syntax_only)
      Z_PARAM_ZVAL_DEREF(callable_name)
   ZEND_PARSE_PARAMETERS_END();
   if (syntax_only) {
      check_flags |= IS_CALLABLE_CHECK_SYNTAX_ONLY;
   }
   if (ZEND_NUM_ARGS() > 2) {
      retval = zend_is_callable_ex(var, NULL, check_flags, &name, NULL, &error);
      zval_ptr_dtor(callable_name);
      ZVAL_STR(callable_name, name);
   } else {
      retval = zend_is_callable_ex(var, NULL, check_flags, NULL, NULL, &error);
   }
   if (error) {
      /* ignore errors */
      efree(error);
   }
   RETURN_BOOL(retval);
}

PHP_FUNCTION(is_iterable)
{
   zval *var;
   ZEND_PARSE_PARAMETERS_START(1, 1)
      Z_PARAM_ZVAL(var)
   ZEND_PARSE_PARAMETERS_END();
   RETURN_BOOL(zend_is_iterable(var));
}

PHP_FUNCTION(is_countable)
{
   zval *var;
   ZEND_PARSE_PARAMETERS_START(1, 1)
      Z_PARAM_ZVAL(var)
   ZEND_PARSE_PARAMETERS_END();
   RETURN_BOOL(zend_is_countable(var));
}

} // runtime
} // polar
