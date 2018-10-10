// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/10.

#include "ErrorHandling.h"
#include <cassert>
#include <cstdlib>
#include <mutex>
#include <new>
#if defined(HAVE_UNISTD_H)
# include <unistd.h>
#endif
#if defined(_MSC_VER)
# include <io.h>
# include <fcntl.h>
#endif

namespace polar {
namespace utils {

static FatalErrorHandlerFunc sg_errorHandler = nullptr;
static void *sg_errorHandlerUserData = nullptr;

static FatalErrorHandlerFunc sg_badAllocErrorHandler = nullptr;
static void *sg_badAllocErrorHandlerUserData = nullptr;

// Mutexes to synchronize installing error handlers and calling error handlers.
// Do not use ManagedStatic, or that may allocate memory while attempting to
// report an OOM.
//
// This usage of std::mutex has to be conditionalized behind ifdefs because
// of this script:
//   compiler-rt/lib/sanitizer_common/symbolizer/scripts/build_symbolizer.sh
// That script attempts to statically link the LLVM symbolizer library with the
// STL and hide all of its symbols with 'opt -internalize'. To reduce size, it
// cuts out the threading portions of the hermetic copy of libc++ that it
// builds. We can remove these ifdefs if that script goes away.
static std::mutex sg_errorHandlerMutex;
static std::mutex sg_badAllocErrorHandlerMutex;

void install_fatal_error_handler(FatalErrorHandlerFunc handler, void *userData)
{
   std::lock_guard locker(sg_errorHandlerMutex);
   assert(!sg_errorHandler && "Error handler already registered!\n");
   sg_errorHandler = handler;
   sg_errorHandlerUserData = userData;
}

void remove_fatal_error_handler()
{
   std::lock_guard locker(sg_errorHandlerMutex);
   sg_errorHandler = nullptr;
   sg_errorHandlerUserData = nullptr;
}



} // utils
} // polar
