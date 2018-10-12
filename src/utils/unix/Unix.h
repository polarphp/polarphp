// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
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

#ifndef POLAR_DEVLTOOLS_UTILS_UNIX_UNIX_H
#define POLAR_DEVLTOOLS_UTILS_UNIX_UNIX_H

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only generic UNIX code that
//===          is guaranteed to work on all UNIX variants.
//===----------------------------------------------------------------------===//

#include "polarphp/global/Config.h"
#include <chrono>

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <time.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace polar {
namespace utils {

/// Convert a struct timeval to a duration. Note that timeval can be used both
/// as a time point and a duration. Be sure to check what the input represents.
///
inline std::chrono::microseconds convert_time_to_duration(const struct timeval &timeValue)
{
   return std::chrono::seconds(timeValue.tv_sec) + std::chrono::microseconds(timeValue.tv_usec);
}

} // unix
} // polar

#endif // POLAR_DEVLTOOLS_UTILS_UNIX_UNIX_H
