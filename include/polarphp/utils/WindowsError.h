// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/09.

#ifndef POLARPHP_UTILS_WINDOWS_ERROR_H
#define POLARPHP_UTILS_WINDOWS_ERROR_H

#include <system_error>

namespace polar {
namespace utils {

std::error_code map_windows_error(unsigned ev);

} // utils
} // polar

#endif // POLARPHP_UTILS_WINDOWS_ERROR_H
