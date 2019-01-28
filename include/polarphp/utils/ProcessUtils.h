// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/08.

#ifndef POLARPHP_UTILS_PROCESS_UTILS_H
#define POLARPHP_UTILS_PROCESS_UTILS_H

#include "polarphp/global/DataTypes.h"
#include <chrono>

namespace polar {
namespace utils {

unsigned get_process_page_size();

/// Return process memory usage.
/// This static function will return the total amount of memory allocated
/// by the process. This only counts the memory allocated via the malloc,
/// calloc and realloc functions and includes any "free" holes in the
/// allocated space.
size_t get_process_malloc_usage();

/// This static function will set \p user_time to the amount of CPU time
/// spent in user (non-kernel) mode and \p sys_time to the amount of CPU
/// time spent in system (kernel) mode.  If the operating system does not
/// support collection of these metrics, a zero duration will be for both
/// values.
/// \param elapsed Returns the system_clock::now() giving current time
/// \param user_time Returns the current amount of user time for the process
/// \param sys_time Returns the current amount of system time for the process
void get_process_time_usage(std::chrono::time_point<> &elapsed,
                            std::chrono::nanoseconds &userTime,
                            std::chrono::nanoseconds &sysTime);


} // utils
} // polar

#endif // POLARPHP_UTILS_PROCESS_UTILS_H
