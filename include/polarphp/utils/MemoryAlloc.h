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

#ifndef POLARPHP_UTILS_MEMORY_ALLOC_H
#define POLARPHP_UTILS_MEMORY_ALLOC_H

#include "polarphp/global/CompilerFeature.h"
#include "polarphp/utils/ErrorHandling.h"
#include <cstdlib>

namespace polar {
namespace utils {

POLAR_ATTRIBUTE_RETURNS_NONNULL inline void *safe_malloc(size_t size)
{
   void *result = std::malloc(size);
   if (result == nullptr) {
      report_bad_alloc_error("Allocation failed");
   }
   return result;
}

POLAR_ATTRIBUTE_RETURNS_NONNULL inline void *safe_calloc(size_t count,
                                                         size_t size)
{
   void *result = std::calloc(count, size);
   if (result == nullptr) {
      report_bad_alloc_error("Allocation failed");
   }
   return result;
}

POLAR_ATTRIBUTE_RETURNS_NONNULL inline void *safe_realloc(void *ptr, size_t size)
{
   void *result = std::realloc(ptr, size);
   if (result == nullptr) {
      report_bad_alloc_error("Allocation failed");
   }
   return result;
}

} // utils
} // polar

#endif // POLARPHP_UTILS_MEMORY_ALLOC_H
