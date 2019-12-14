//===--- SwiftStddef.h ------------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_STDLIB_SHIMS_POLARPHP_STDDEF_H
#define POLARPHP_STDLIB_SHIMS_POLARPHP_STDDEF_H

// stddef.h is provided by Clang, but it dispatches to libc's stddef.h.  As a
// result, using stddef.h here would pull in Darwin module (which includes
// libc). This creates a dependency cycle, so we can't use stddef.h in
// SwiftShims.
// On Linux, the story is different. We get the error message
// "/usr/include/x86_64-linux-gnu/sys/types.h:146:10: error: 'stddef.h' file not
// found"
// This is a known Clang/Ubuntu bug.
#if !defined(__APPLE__) && !defined(__linux__)
#include <stddef.h>
typedef size_t __polarphp_size_t;
#else
typedef __SIZE_TYPE__ __polarphp_size_t;
#endif

// This selects the signed equivalent of the unsigned type chosen for size_t.
#if __STDC_VERSION__-0 >= 201112l || defined(__polarphp__)
typedef __typeof__(_Generic((__polarphp_size_t)0,                                 \
                            unsigned long long int : (long long int)0,         \
                            unsigned long int : (long int)0,                   \
                            unsigned int : (int)0,                             \
                            unsigned short : (short)0,                         \
                            unsigned char : (signed char)0)) __polarphp_ssize_t;
#elif defined(__cplusplus)
#include <type_traits>
using __polarphp_ssize_t = std::make_signed<__polarphp_size_t>::type;
#else
#error "do not have __polarphp_ssize_t defined"
#endif

#endif // POLARPHP_STDLIB_SHIMS_POLARPHP_STDDEF_H
