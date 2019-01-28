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

//===- llvm/Support/Unix/Unix.h - Common Unix Include File -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines things specific to Unix implementations.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_UNIX_UNIX_H
#define POLARPHP_UTILS_UNIX_UNIX_H

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only generic UNIX code that
//===          is guaranteed to work on all UNIX variants.
//===----------------------------------------------------------------------===//

#include "polarphp/global/Config.h"
#include "polarphp/utils/ErrorNumber.h"
#include "polarphp/utils/Chrono.h"
#include <chrono>
#include <algorithm>
#include <assert.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef POLAR_HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef POLAR_HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef POLAR_HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <time.h>

#ifdef POLAR_HAVE_DLFCN_H
# include <dlfcn.h>
#endif

#ifdef POLAR_HAVE_FCNTL_H
# include <fcntl.h>
#endif

/// This function builds an error message into \p ErrMsg using the \p prefix
/// string and the Unix error number given by \p errnum. If errnum is -1, the
/// default then the value of errno is used.
/// Make an error message
///
/// If the error number can be converted to a string, it will be
/// separated from prefix by ": ".
static inline bool make_error_msg(
      std::string *errorMsg, const std::string &prefix, int errnum = -1) {
   if (!errorMsg) {
      return true;
   }
   if (errnum == -1) {
      errnum = errno;
   }

   *errorMsg = prefix + ": " + polar::utils::get_str_error(errnum);
   return true;
}

namespace polar {
namespace utils {
/// Convert a struct timeval to a duration. Note that timeval can be used both
/// as a time point and a duration. Be sure to check what the input represents.
inline std::chrono::microseconds to_duration(const struct timeval &tv)
{
  return std::chrono::seconds(tv.tv_sec) +
         std::chrono::microseconds(tv.tv_usec);
}

/// Convert a time point to struct timespec.
inline struct timespec to_time_spec(TimePoint<> tp)
{
  using namespace std::chrono;

  struct timespec retVal;
  retVal.tv_sec = to_time_t(tp);
  retVal.tv_nsec = (tp.time_since_epoch() % seconds(1)).count();
  return retVal;
}

/// Convert a time point to struct timeval.
inline struct timeval to_time_val(TimePoint<std::chrono::microseconds> tp)
{
  using namespace std::chrono;
  struct timeval retVal;
  retVal.tv_sec = to_time_t(tp);
  retVal.tv_usec = (tp.time_since_epoch() % seconds(1)).count();
  return retVal;
}


} // unix
} // polar

#endif // POLARPHP_UTILS_UNIX_UNIX_H
