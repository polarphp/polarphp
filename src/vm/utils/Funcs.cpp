// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/23.

#include "polarphp/vm/utils/Funcs.h"
#include "polarphp/vm/ds/Variant.h"
#include "polarphp/vm/lang/Type.h"

#include <string>
#include <cstring>
#include <cctype>

namespace polar {
namespace vmapi {

void std_php_memory_deleter(void *ptr)
{
   efree(ptr);
}

void std_zend_string_deleter(zend_string *str)
{
   zend_string_release(str);
}

void std_zend_string_force_deleter(zend_string *str)
{
   zend_string_free(str);
}

char *str_toupper(char *str) noexcept
{
   return str_tolower(str, std::strlen(str));
}

char *str_toupper(char *str, size_t length) noexcept
{
   char *ptr = str;
   while (length--) {
      *ptr = std::toupper(*ptr);
      ptr++;
   }
   return str;
}

std::string &str_toupper(std::string &str) noexcept
{
   str_toupper(const_cast<char *>(str.data()), str.length());
   return str;
}

char *str_tolower(char *str) noexcept
{
   return str_tolower(str, std::strlen(str));
}

char *str_tolower(char *str, size_t length) noexcept
{
   char *ptr = str;
   while (length--) {
      *ptr = std::tolower(*ptr);
      ptr++;
   }
   return str;
}

std::string &str_tolower(std::string &str) noexcept
{
   str_tolower(const_cast<char *>(str.data()), str.length());
   return str;
}

std::string get_zval_type_str(const zval *valuePtr) noexcept
{
   switch (Z_TYPE_P(valuePtr)) {
   case IS_ARRAY:
      return "Array";
   case IS_TRUE:
   case IS_FALSE:
   case _IS_BOOL:
      return "Boolean";
   case IS_CALLABLE:
      return "Callable";
   case IS_DOUBLE:
      return "Double";
   case IS_LONG:
      return "Numeric";
   case IS_OBJECT:
      return "Object";
   case IS_REFERENCE:
      return "Reference";
   case IS_RESOURCE:
      return "Resource";
   case IS_STRING:
      return "String";
   case IS_PTR:
      return "Pointer";
   default:
      return "Unknow";
   }
}

bool zval_type_is_valid(const zval *valuePtr) noexcept
{
   switch (Z_TYPE_P(valuePtr)) {
   case IS_UNDEF:
   case IS_ARRAY:
   case IS_TRUE:
   case IS_FALSE:
   case _IS_BOOL:
   case IS_CALLABLE:
   case IS_DOUBLE:
   case IS_LONG:
   case IS_OBJECT:
   case IS_REFERENCE:
   case IS_RESOURCE:
   case IS_STRING:
   case IS_PTR:
      return true;
   default:
      return false;
   }
}

bool VariantKeyLess::operator ()(const Variant &lhs, const Variant &rhs) const
{
   Type ltype = lhs.getType();
   Type rtype = rhs.getType();
   bool result;
   if (ltype == Type::String && rtype == Type::String) {
      result = std::strcmp(Z_STRVAL_P(lhs.getZvalPtr()), Z_STRVAL_P(rhs.getZvalPtr())) < 0;
   } else if (ltype == Type::String && rtype == Type::Long) {
      zval temp;
      ZVAL_COPY_VALUE(&temp, rhs.getZvalPtr());
      convert_to_string(&temp);
      result = std::strcmp(Z_STRVAL_P(lhs.getZvalPtr()), Z_STRVAL_P(&temp)) < 0;
      zval_dtor(&temp);
   } else if (ltype == Type::Long && rtype == Type::String) {
      zval temp;
      ZVAL_COPY_VALUE(&temp, lhs.getZvalPtr());
      convert_to_string(&temp);
      result = std::strcmp(Z_STRVAL_P(&temp), Z_STRVAL_P(rhs.getZvalPtr())) < 0;
      zval_dtor(&temp);
   } else {
      result = Z_LVAL_P(lhs.getZvalPtr()) < Z_LVAL_P(rhs.getZvalPtr());
   }
   return result;
}


} // vmapi
} // polar
