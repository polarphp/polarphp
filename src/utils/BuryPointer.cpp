// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/05.

#include "polarphp/utils/BuryPointer.h"
#include "polarphp/global/CompilerFeature.h"
#include <atomic>

namespace polar {
namespace utils {

void bury_pointer(const void *ptr)
{
   // This function may be called only a small fixed amount of times per each
   // invocation, otherwise we do actually have a leak which we want to report.
   // If this function is called more than kGraveYardMaxSize times, the pointers
   // will not be properly buried and a leak detector will report a leak, which
   // is what we want in such case.
   static const size_t kGraveYardMaxSize = 16;
   POLAR_ATTRIBUTE_UNUSED static const void *graveYard[kGraveYardMaxSize];
   static std::atomic<unsigned> graveYardSize;
   unsigned idx = graveYardSize++;
   if (idx >= kGraveYardMaxSize) {
      return;
   }
   graveYard[idx] = ptr;
}

} // utils
} // polar
