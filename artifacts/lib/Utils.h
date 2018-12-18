// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/18.

#ifndef POLARPHP_ARTIFACTS_UTILS_H
#define POLARPHP_ARTIFACTS_UTILS_H

#include <ctime>

namespace polar {

std::size_t php_format_date(char* str, std::size_t count, const char* format,
                            time_t ts, bool localtime);

} // polar

#endif // POLARPHP_ARTIFACTS_UTILS_H

