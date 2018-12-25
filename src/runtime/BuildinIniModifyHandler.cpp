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

#include "polarphp/runtime/BuildinIniModifyHandler.h"
#include "polarphp/runtime/PhpDefs.h"

#include <string>

namespace polar {
namespace runtime {

ZEND_INI_MH(update_bool_handler)
{
   zend_bool *p;
   char *base = (char *) mh_arg2;
   p = (zend_bool *) (base+(size_t) mh_arg1);
   *p = zend_ini_parse_bool(new_value);
   return SUCCESS;
}

ZEND_INI_MH(update_long_handler)
{
   zend_long *p;
   char *base = (char *) mh_arg2;
   p = (zend_long *) (base+(size_t) mh_arg1);
   *p = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
   return SUCCESS;
}

ZEND_INI_MH(update_long_ge_zero_handler)
{
   zend_long *p, tmp;
   char *base = (char *) mh_arg2;
   tmp = zend_atol(ZSTR_VAL(new_value), ZSTR_LEN(new_value));
   if (tmp < 0) {
      return FAILURE;
   }
   p = (zend_long *) (base+(size_t) mh_arg1);
   *p = tmp;
   return SUCCESS;
}

ZEND_INI_MH(update_real_handler)
{
   double *p;
   char *base;
   base = (char *) ts_resource(*((int *) mh_arg2));
   p = (double *) (base+(size_t) mh_arg1);
   *p = zend_strtod(ZSTR_VAL(new_value), nullptr);
   return SUCCESS;
}

ZEND_INI_MH(update_string_handler)
{
   std::string *p;
   char *base = (char *) mh_arg2;
   p = (std::string *) (base+(size_t) mh_arg1);
   *p = new_value ? ZSTR_VAL(new_value) : nullptr;
   return SUCCESS;
}

ZEND_INI_MH(update_string_unempty_handler)
{
   std::string *p;
   char *base = (char *) mh_arg2;
   if (new_value && !ZSTR_VAL(new_value)[0]) {
      return FAILURE;
   }
   p = (std::string *) (base+(size_t) mh_arg1);
   *p = new_value ? ZSTR_VAL(new_value) : nullptr;
   return SUCCESS;
}

} // runtime
} // polar
