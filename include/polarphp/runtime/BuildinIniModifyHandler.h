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
ZEND_INI_MH(update_bool_handler);
ZEND_INI_MH(update_long_handler);
ZEND_INI_MH(update_long_ge_zero_handler);
ZEND_INI_MH(update_real_handler);
ZEND_INI_MH(update_string_handler);
ZEND_INI_MH(update_string_unempty_handler);

///
/// custom ini modify handlers
///
ZEND_INI_MH(set_serialize_precision_handler);
ZEND_INI_MH(update_display_errors_handler);
ZEND_INI_MH(update_internal_encoding_handler);
ZEND_INI_MH(update_error_log_handler);
ZEND_INI_MH(update_timeout_handler);
ZEND_INI_MH(update_base_dir_handler);
ZEND_INI_MH(change_memory_limit_handler);
ZEND_INI_MH(set_precision_handler);
ZEND_INI_MH(set_facility_handler);
ZEND_INI_MH(set_log_filter_handler);

///
/// custom ini displayer handlers
///
ZEND_INI_DISP(display_errors_mode);

} // runtime
} // polar

#endif // POLARPHP_RUNTIME_BUILDIN_INI_MODIFY_HANDLER_H
