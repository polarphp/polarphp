// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/21.


//===----------------------------------------------------------------------===//
//
//  Defines Valgrind communication methods, if HAVE_VALGRIND_VALGRIND_H is
//  defined.  If we have valgrind.h but valgrind isn't running, its macros are
//  no-ops.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/Valgrind.h"
#include "polarphp/global/Config.h"
#include <cstddef>

#if HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
namespace polar {
namespace sys {

namespace {

bool init_not_under_valgrind()
{
   return !RUNNING_ON_VALGRIND;
}

} // anonymous namespace

// This bool is negated from what we'd expect because code may run before it
// gets initialized.  If that happens, it will appear to be 0 (false), and we
// want that to cause the rest of the code in this file to run the
// Valgrind-provided macros.
static const bool sg_notUnderValgrind = init_not_under_valgrind();

bool running_on_valgrind()
{
   if (sg_notUnderValgrind) {
      return false;
   }
   return RUNNING_ON_VALGRIND;
}

void valgrind_discard_translations(const void *addr, size_t len)
{
   if (sg_notUnderValgrind) {
      return;
   }
   VALGRIND_DISCARD_TRANSLATIONS(addr, len);
}
} // sys
} // polar
#else  // !HAVE_VALGRIND_VALGRIND_H

namespace polar {
namespace sys {

bool running_on_valgrind()
{
   return false;
}

void valgrind_discard_translations(const void *addr, size_t len)
{}

} // sys
} // polar

#endif
