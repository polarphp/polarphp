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

#ifndef POLARPHP_RUNTIME_BUILDIN_INI_MODIFY_HANDLER_H
#define POLARPHP_RUNTIME_BUILDIN_INI_MODIFY_HANDLER_H

#include "polarphp/runtime/internal/DepsZendVmHeaders.h"

namespace polar {
namespace runtime {

///
/// because polarphp use thread_local mechanism instead of TSRM
/// so we need rewrite ini modify callbacks
/// here we use update_xxx_handler name schema
///
POLAR_DECL_EXPORT ZEND_INI_MH(update_bool_handler);
POLAR_DECL_EXPORT ZEND_INI_MH(update_long_handler);
POLAR_DECL_EXPORT ZEND_INI_MH(update_long_ge_zero_handler);
POLAR_DECL_EXPORT ZEND_INI_MH(update_real_handler);
POLAR_DECL_EXPORT ZEND_INI_MH(update_string_handler);
POLAR_DECL_EXPORT ZEND_INI_MH(update_string_unempty_handler);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_BUILDIN_INI_MODIFY_HANDLER_H
