//===- MemAlloc.h - Memory allocation functions -----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

#ifndef POLARPHP_UTILS_MEMORY_ALLOC_H
#define POLARPHP_UTILS_MEMORY_ALLOC_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/utils/ErrorHandling.h"
#include <cstdlib>

namespace polar::utils {

POLAR_ATTRIBUTE_RETURNS_NONNULL inline void *safe_malloc(size_t size)
{
   void *result = std::malloc(size);
   if (result == nullptr) {
      // It is implementation-defined whether allocation occurs if the space
      // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
      // non-zero, if the space requested was zero.
      if (0 == size) {
         return safe_malloc(1);
      }
      report_bad_alloc_error("Allocation failed");
   }
   return result;
}

POLAR_ATTRIBUTE_RETURNS_NONNULL inline void *safe_calloc(size_t count,
                                                         size_t size)
{
   void *result = std::calloc(count, size);
   if (result == nullptr) {
      // It is implementation-defined whether allocation occurs if the space
      // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
      // non-zero, if the space requested was zero.
      if (count == 0 || size == 0) {
         return safe_malloc(1);
      }
      report_bad_alloc_error("Allocation failed");
   }
   return result;
}

POLAR_ATTRIBUTE_RETURNS_NONNULL inline void *safe_realloc(void *ptr, size_t size)
{
   void *result = std::realloc(ptr, size);
   if (result == nullptr) {
      // It is implementation-defined whether allocation occurs if the space
      // requested is zero (ISO/IEC 9899:2018 7.22.3). Retry, requesting
      // non-zero, if the space requested was zero.
      if (size == 0) {
         return safe_malloc(1);
      }
      report_bad_alloc_error("Allocation failed");
   }
   return result;
}

} // polar::utils

#endif // POLARPHP_UTILS_MEMORY_ALLOC_H
