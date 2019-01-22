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

#ifndef POLARPHP_RUNTIME_LANG_SUPPORT_ASSERT_FUNCS_H
#define POLARPHP_RUNTIME_LANG_SUPPORT_ASSERT_FUNCS_H

#include "polarphp/runtime/RtDefs.h"

namespace polar {
namespace runtime {

PHP_MINIT_FUNCTION(assert);
PHP_MSHUTDOWN_FUNCTION(assert);
PHP_RINIT_FUNCTION(assert);
PHP_RSHUTDOWN_FUNCTION(assert);
PHP_FUNCTION(assert);
PHP_FUNCTION(assert_options);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_LANG_SUPPORT_ASSERT_FUNCS_H
