// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

#ifndef POLARPHP_UTILS_ERROR_NUMBER_H
#define POLARPHP_UTILS_ERROR_NUMBER_H

#include <cerrno>
#include <string>
#include <type_traits>

namespace polar {
namespace utils {

/// Returns a string representation of the errno value, using whatever
/// thread-safe variant of strerror() is available.  Be sure to call this
/// immediately after the function that set errno, or errno may have been
/// overwritten by an intervening call.
std::string get_str_error();

/// Like the no-argument version above, but uses \p errnum instead of errno.
std::string get_str_error(int errnum);

template <typename FailT, typename Fun, typename... Args>
inline auto retry_after_signal(const FailT &fail, const Fun &func,
                               const Args &... args) -> decltype(func(args...))
{
   decltype(func(args...)) res;
   do {
      errno = 0;
      res = func(args...);
   } while (res == fail && errno == EINTR);
   return res;
}

} // utils
} // polar

#endif // POLARPHP_UTILS_ERROR_NUMBER_H
