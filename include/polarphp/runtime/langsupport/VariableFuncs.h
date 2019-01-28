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

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_VARIABLE_FUNCS_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_VARIABLE_FUNCS_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

PHP_FUNCTION(var_dump);
PHP_FUNCTION(var_export);
PHP_FUNCTION(debug_zval_dump);

POLAR_DECL_EXPORT void var_dump(zval *struc, int level);
POLAR_DECL_EXPORT void var_export(zval *struc, int level);
POLAR_DECL_EXPORT void var_export_ex(zval *struc, int level, smart_str *buf);
POLAR_DECL_EXPORT void debug_zval_dump(zval *struc, int level);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_VARIABLE_FUNCS_H
