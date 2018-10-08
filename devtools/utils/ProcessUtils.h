// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/08.

#ifndef POLAR_DEVLTOOLS_UTILS_PROCESS_UTILS_H
#define POLAR_DEVLTOOLS_UTILS_PROCESS_UTILS_H

namespace polar {
namespace utils {

unsigned get_process_page_size();

/// Return process memory usage.
/// This static function will return the total amount of memory allocated
/// by the process. This only counts the memory allocated via the malloc,
/// calloc and realloc functions and includes any "free" holes in the
/// allocated space.

} // utils
} // polar

#endif // POLAR_DEVLTOOLS_UTILS_PROCESS_UTILS_H
