//===--- Malloc.h - Aligned malloc interface --------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//===----------------------------------------------------------------------===//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/28.
//===----------------------------------------------------------------------===//
//
//  This file provides an implementation of C11 aligned_alloc(3) for platforms
//  that don't have it yet, using posix_memalign(3).
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_MALLOC_H
#define POLARPHP_BASIC_MALLOC_H

#include <cassert>
#if defined(_WIN32)
#include <malloc.h>
#else
#include <cstdlib>
#endif

namespace polar {

// FIXME: Use C11 aligned_alloc if available.
inline void *aligned_alloc(size_t size, size_t align)
{
   // posix_memalign only accepts alignments greater than sizeof(void*).
   //
   if (align < sizeof(void*)) {
      align = sizeof(void*);
   }
   void *r;
#if defined(_WIN32)
   r = _aligned_malloc(size, align);
   assert(r && "_aligned_malloc failed");
#else
   int res = posix_memalign(&r, align, size);
   assert(res == 0 && "posix_memalign failed");
   (void)res; // Silence the unused variable warning.
#endif
   return r;
}

inline void aligned_free(void *p)
{
#if defined(_WIN32)
   _aligned_free(p);
#else
   free(p);
#endif
}

} // polar

#endif // POLARPHP_BASIC_MALLOC_H
