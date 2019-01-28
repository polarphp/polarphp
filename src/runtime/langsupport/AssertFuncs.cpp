// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/02/10.

#include "polarphp/runtime/langsupport/AssertFuncs.h"
#include "polarphp/runtime/Ini.h"
#include "polarphp/runtime/BuildinIniModifyHandler.h"

namespace polar {
namespace runtime {

struct AssertModuleData
{
   zval callback;
   char *cb;
   zend_bool active;
   zend_bool bail;
   zend_bool warning;
   zend_bool quietEval;
   zend_bool exception;
};

#define ASSERTG(v) sg_assertModuleData.v

#define SAFE_STRING(s) ((s)?(s):"")

enum {
   ASSERT_ACTIVE=1,
   ASSERT_CALLBACK,
   ASSERT_BAIL,
   ASSERT_WARNING,
   ASSERT_QUIET_EVAL,
   ASSERT_EXCEPTION
};

thread_local AssertModuleData sg_assertModuleData;
static zend_class_entry *sg_assertionErrorCe;

namespace {
POLAR_INI_MH(assert_cfg_change_handler) /* {{{ */
{
   if (EG(current_execute_data)) {
      if (Z_TYPE(ASSERTG(callback)) != IS_UNDEF) {
         zval_ptr_dtor(&ASSERTG(callback));
         ZVAL_UNDEF(&ASSERTG(callback));
      }
      if (new_value && (Z_TYPE(ASSERTG(callback)) != IS_UNDEF || ZSTR_LEN(new_value))) {
         ZVAL_STR_COPY(&ASSERTG(callback), new_value);
      }
   } else {
      if (ASSERTG(cb)) {
         pefree(ASSERTG(cb), 1);
      }
      if (new_value && ZSTR_LEN(new_value)) {
         ASSERTG(cb) = reinterpret_cast<char *>(pemalloc(ZSTR_LEN(new_value) + 1, 1));
         memcpy(ASSERTG(cb), ZSTR_VAL(new_value), ZSTR_LEN(new_value));
         ASSERTG(cb)[ZSTR_LEN(new_value)] = '\0';
      } else {
         ASSERTG(cb) = NULL;
      }
   }
   return SUCCESS;
}

POLAR_INI_BEGIN()
   POLAR_STD_INI_ENTRY("assert.active",     "1", POLAR_INI_ALL, update_bool_handler, active,    AssertModuleData, sg_assertModuleData)
   POLAR_STD_INI_ENTRY("assert.bail",       "0", POLAR_INI_ALL, update_bool_handler, bail,      AssertModuleData, sg_assertModuleData)
   POLAR_STD_INI_ENTRY("assert.warning",    "1", POLAR_INI_ALL, update_bool_handler, warning,   AssertModuleData, sg_assertModuleData)
   POLAR_INI_ENTRY("assert.callback",       "",  POLAR_INI_ALL, assert_cfg_change_handler)
   POLAR_STD_INI_ENTRY("assert.quiet_eval", "0", POLAR_INI_ALL, update_bool_handler, quietEval,  AssertModuleData, sg_assertModuleData)
   POLAR_STD_INI_ENTRY("assert.exception",  "0", POLAR_INI_ALL, update_bool_handler, exception,  AssertModuleData, sg_assertModuleData)
POLAR_INI_END()

} // anonymous namespace

PHP_MINIT_FUNCTION(assert)
{
   zend_class_entry ce;
   ZVAL_UNDEF(&sg_assertModuleData.callback);
   sg_assertModuleData.cb = nullptr;
   REGISTER_INI_ENTRIES();
   REGISTER_LONG_CONSTANT("ASSERT_ACTIVE", ASSERT_ACTIVE, CONST_CS|CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("ASSERT_CALLBACK", ASSERT_CALLBACK, CONST_CS|CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("ASSERT_BAIL", ASSERT_BAIL, CONST_CS|CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("ASSERT_WARNING", ASSERT_WARNING, CONST_CS|CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("ASSERT_QUIET_EVAL", ASSERT_QUIET_EVAL, CONST_CS|CONST_PERSISTENT);
   REGISTER_LONG_CONSTANT("ASSERT_EXCEPTION", ASSERT_EXCEPTION, CONST_CS|CONST_PERSISTENT);
   INIT_CLASS_ENTRY(ce, "AssertionError", NULL);
   sg_assertionErrorCe = zend_register_internal_class_ex(&ce, zend_ce_error);
   return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(assert)
{
   if (ASSERTG(cb)) {
      pefree(ASSERTG(cb), 1);
      ASSERTG(cb) = NULL;
   }
   return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(assert)
{
   if (Z_TYPE(ASSERTG(callback)) != IS_UNDEF) {
      zval_ptr_dtor(&ASSERTG(callback));
      ZVAL_UNDEF(&ASSERTG(callback));
   }
   return SUCCESS;
}

PHP_FUNCTION(assert)
{
   zval *assertion;
   zval *description = NULL;
   int val;
   char *myeval = NULL;
   char *compiled_string_description;

   if (! ASSERTG(active)) {
      RETURN_TRUE;
   }

   ZEND_PARSE_PARAMETERS_START(1, 2)
         Z_PARAM_ZVAL(assertion)
         Z_PARAM_OPTIONAL
         Z_PARAM_ZVAL(description)
         ZEND_PARSE_PARAMETERS_END();

   if (Z_TYPE_P(assertion) == IS_STRING) {
      zval retval;
      int old_error_reporting = 0; /* shut up gcc! */

      if (zend_forbid_dynamic_call("assert() with string argument") == FAILURE) {
         RETURN_FALSE;
      }

      php_error_docref(NULL, E_DEPRECATED, "Calling assert() with a string argument is deprecated");

      myeval = Z_STRVAL_P(assertion);

      if (ASSERTG(quietEval)) {
         old_error_reporting = EG(error_reporting);
         EG(error_reporting) = 0;
      }

      compiled_string_description = zend_make_compiled_string_description("assert code");
      if (zend_eval_stringl(myeval, Z_STRLEN_P(assertion), &retval, compiled_string_description) == FAILURE) {
         efree(compiled_string_description);
         if (!description) {
            zend_throw_error(NULL, "Failure evaluating code: %s%s", PHP_EOL, myeval);
         } else {
            zend_string *str = zval_get_string(description);
            zend_throw_error(NULL, "Failure evaluating code: %s%s:\"%s\"", PHP_EOL, ZSTR_VAL(str), myeval);
            zend_string_release_ex(str, 0);
         }
         if (ASSERTG(bail)) {
            zend_bailout();
         }
         RETURN_FALSE;
      }
      efree(compiled_string_description);

      if (ASSERTG(quietEval)) {
         EG(error_reporting) = old_error_reporting;
      }

      convert_to_boolean(&retval);
      val = Z_TYPE(retval) == IS_TRUE;
   } else {
      val = zend_is_true(assertion);
   }

   if (val) {
      RETURN_TRUE;
   }

   if (Z_TYPE(ASSERTG(callback)) == IS_UNDEF && ASSERTG(cb)) {
      ZVAL_STRING(&ASSERTG(callback), ASSERTG(cb));
   }

   if (Z_TYPE(ASSERTG(callback)) != IS_UNDEF) {
      zval args[4];
      zval retval;
      uint32_t lineno = zend_get_executed_lineno();
      const char *filename = zend_get_executed_filename();

      ZVAL_STRING(&args[0], SAFE_STRING(filename));
      ZVAL_LONG (&args[1], lineno);
      ZVAL_STRING(&args[2], SAFE_STRING(myeval));

      ZVAL_FALSE(&retval);

      /* XXX do we want to check for error here? */
      if (!description) {
         call_user_function(CG(function_table), NULL, &ASSERTG(callback), &retval, 3, args);
         zval_ptr_dtor(&(args[2]));
         zval_ptr_dtor(&(args[0]));
      } else {
         ZVAL_STR(&args[3], zval_get_string(description));
         call_user_function(CG(function_table), NULL, &ASSERTG(callback), &retval, 4, args);
         zval_ptr_dtor(&(args[3]));
         zval_ptr_dtor(&(args[2]));
         zval_ptr_dtor(&(args[0]));
      }

      zval_ptr_dtor(&retval);
   }

   if (ASSERTG(exception)) {
      if (!description) {
         zend_throw_exception(sg_assertionErrorCe, NULL, E_ERROR);
      } else if (Z_TYPE_P(description) == IS_OBJECT &&
                 instanceof_function(Z_OBJCE_P(description), zend_ce_throwable)) {
         Z_ADDREF_P(description);
         zend_throw_exception_object(description);
      } else {
         zend_string *str = zval_get_string(description);
         zend_throw_exception(sg_assertionErrorCe, ZSTR_VAL(str), E_ERROR);
         zend_string_release_ex(str, 0);
      }
   } else if (ASSERTG(warning)) {
      if (!description) {
         if (myeval) {
            php_error_docref(NULL, E_WARNING, "Assertion \"%s\" failed", myeval);
         } else {
            php_error_docref(NULL, E_WARNING, "Assertion failed");
         }
      } else {
         zend_string *str = zval_get_string(description);
         if (myeval) {
            php_error_docref(NULL, E_WARNING, "%s: \"%s\" failed", ZSTR_VAL(str), myeval);
         } else {
            php_error_docref(NULL, E_WARNING, "%s failed", ZSTR_VAL(str));
         }
         zend_string_release_ex(str, 0);
      }
   }
   if (ASSERTG(bail)) {
      zend_bailout();
   }
   RETURN_FALSE;
}

PHP_FUNCTION(assert_options)
{
   zval *value = NULL;
   zend_long what;
   zend_bool oldint;
   int ac = ZEND_NUM_ARGS();
   zend_string *key;

   ZEND_PARSE_PARAMETERS_START(1, 2)
         Z_PARAM_LONG(what)
         Z_PARAM_OPTIONAL
         Z_PARAM_ZVAL(value)
         ZEND_PARSE_PARAMETERS_END();

   switch (what) {
   case ASSERT_ACTIVE:
      oldint = ASSERTG(active);
      if (ac == 2) {
         zend_string *value_str = zval_get_string(value);
         key = zend_string_init("assert.active", sizeof("assert.active")-1, 0);
         zend_alter_ini_entry_ex(key, value_str, POLAR_INI_USER, POLAR_INI_STAGE_RUNTIME, 0);
         zend_string_release_ex(key, 0);
         zend_string_release_ex(value_str, 0);
      }
      RETURN_LONG(oldint);
      break;

   case ASSERT_BAIL:
      oldint = ASSERTG(bail);
      if (ac == 2) {
         zend_string *value_str = zval_get_string(value);
         key = zend_string_init("assert.bail", sizeof("assert.bail")-1, 0);
         zend_alter_ini_entry_ex(key, value_str, POLAR_INI_USER, POLAR_INI_STAGE_RUNTIME, 0);
         zend_string_release_ex(key, 0);
         zend_string_release_ex(value_str, 0);
      }
      RETURN_LONG(oldint);
      break;

   case ASSERT_QUIET_EVAL:
      oldint = ASSERTG(quietEval);
      if (ac == 2) {
         zend_string *value_str = zval_get_string(value);
         key = zend_string_init("assert.quiet_eval", sizeof("assert.quiet_eval")-1, 0);
         zend_alter_ini_entry_ex(key, value_str, POLAR_INI_USER, POLAR_INI_STAGE_RUNTIME, 0);
         zend_string_release_ex(key, 0);
         zend_string_release_ex(value_str, 0);
      }
      RETURN_LONG(oldint);
      break;

   case ASSERT_WARNING:
      oldint = ASSERTG(warning);
      if (ac == 2) {
         zend_string *value_str = zval_get_string(value);
         key = zend_string_init("assert.warning", sizeof("assert.warning")-1, 0);
         zend_alter_ini_entry_ex(key, value_str, POLAR_INI_USER, POLAR_INI_STAGE_RUNTIME, 0);
         zend_string_release_ex(key, 0);
         zend_string_release_ex(value_str, 0);
      }
      RETURN_LONG(oldint);
      break;

   case ASSERT_CALLBACK:
      if (Z_TYPE(ASSERTG(callback)) != IS_UNDEF) {
         ZVAL_COPY(return_value, &ASSERTG(callback));
      } else if (ASSERTG(cb)) {
         RETVAL_STRING(ASSERTG(cb));
      } else {
         RETVAL_NULL();
      }
      if (ac == 2) {
         zval_ptr_dtor(&ASSERTG(callback));
         ZVAL_COPY(&ASSERTG(callback), value);
      }
      return;

   case ASSERT_EXCEPTION:
      oldint = ASSERTG(exception);
      if (ac == 2) {
         zend_string *key = zend_string_init("assert.exception", sizeof("assert.exception")-1, 0);
         zend_string *val = zval_get_string(value);
         zend_alter_ini_entry_ex(key, val, POLAR_INI_USER, POLAR_INI_STAGE_RUNTIME, 0);
         zend_string_release_ex(val, 0);
         zend_string_release_ex(key, 0);
      }
      RETURN_LONG(oldint);
      break;

   default:
      php_error_docref(NULL, E_WARNING, "Unknown value " ZEND_LONG_FMT, what);
      break;
   }

   RETURN_FALSE;
}

} // runtime
} // polar
