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

#ifndef POLARPHP_VMAPI_UTILS_FUNCS_H
#define POLARPHP_VMAPI_UTILS_FUNCS_H

#include "polarphp/vm/ZendApi.h"

#include <string>
#include <cstring>
#include <cctype>

namespace polar {
namespace vmapi {

class Variant;

VMAPI_DECL_EXPORT void std_php_memory_deleter(void *ptr);
VMAPI_DECL_EXPORT void std_zend_string_deleter(zend_string *str);
VMAPI_DECL_EXPORT void std_zend_string_force_deleter(zend_string *str);

VMAPI_DECL_EXPORT char *str_toupper(char *str) noexcept;
VMAPI_DECL_EXPORT char *str_toupper(char *str, size_t length) noexcept;
VMAPI_DECL_EXPORT std::string &str_toupper(std::string &str) noexcept;
VMAPI_DECL_EXPORT char *str_tolower(char *str) noexcept;
VMAPI_DECL_EXPORT char *str_tolower(char *str, size_t length) noexcept;
VMAPI_DECL_EXPORT std::string &str_tolower(std::string &str) noexcept;
VMAPI_DECL_EXPORT std::string get_zval_type_str(const zval *valuePtr) noexcept;
VMAPI_DECL_EXPORT bool zval_type_is_valid(const zval *valuePtr) noexcept;

struct VMAPI_DECL_EXPORT VariantKeyLess
{
   bool operator ()(const Variant &lhs, const Variant &rhs) const;
};

} // vmapi
} // polar

#endif // POLARPHP_VMAPI_UTILS_FUNCS_H
