// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

//===----------------------------------------------------------------------===//
//
// This file provides the UNIX specific implementation of DynamicLibrary.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/DynamicLibrary.h"
#include "polarphp/utils/internal/DynamicLibraryHandleSetPrivate.h"

namespace polar {
namespace sys {

#if defined(POLAR_HAVE_DLFCN_H) && defined(POLAR_HAVE_DLOPEN)
#include <dlfcn.h>

namespace internal {
HandleSet::~HandleSet()
{
   // Close the libraries in reverse order.
   for (void *handle : polar::basic::reverse(m_handles)) {
      dlclose(handle);
   }

   if (m_process) {
      dlclose(m_process);
   }
   // polar_shutdown called, Return to default
   DynamicLibrary::sm_searchOrder = DynamicLibrary::SO_Linker;
}

void *HandleSet::dllOpen(const char *file, std::string *errorMsg)
{
   void *handle = dlopen(file, RTLD_LAZY|RTLD_GLOBAL);
   if (!handle) {
      if (errorMsg) *errorMsg = dlerror();
      return &DynamicLibrary::sm_invalid;
   }

#ifdef __CYGWIN__
   // Cygwin searches symbols only in the main
   // with the handle of dlopen(NULL, RTLD_GLOBAL).
   if (!file) {
      handle = RTLD_DEFAULT;
   }
#endif
   return handle;
}

void HandleSet::dllClose(void *handle)
{
   dlclose(handle);
}

void *HandleSet::dllSym(void *handle, const char *symbol)
{
   return dlsym(handle, symbol);
}

#else // !POLAR_HAVE_DLOPEN

HandleSet::~HandleSet()
{}

void *DynamicLibrary::HandleSet::dllOpen(const char *file, std::string *errorMsg)
{
   if (errorMsg) {
      *errorMsg = "dlopen() not supported on this platform";
   }
   return &sm_invalid;
}

void HandleSet::dllClose(void *handle)
{
}

void *HandleSet::dllSym(void *handle, const char *symbol)
{
   return nullptr;
}

#endif
} // internal

// Must declare the symbols in the global namespace.
void *do_search(const char* symbolName) {
#define EXPLICIT_SYMBOL(SYM) \
   extern void *SYM; if (!strcmp(symbolName, #SYM)) return (void*)&SYM

   // If this is darwin, it has some funky issues, try to solve them here.  Some
   // important symbols are marked 'private external' which doesn't allow
   // SearchForAddressOfSymbol to find them.  As such, we special case them here,
   // there is only a small handful of them.

#ifdef __APPLE__
   {
      // __eprintf is sometimes used for assert() handling on x86.
      //
      // FIXME: Currently disabled when using Clang, as we don't always have our
      // runtime support libraries available.
#ifndef __clang__
#ifdef __i386__
      EXPLICIT_SYMBOL(__eprintf);
#endif
#endif
   }
#endif

#ifdef __CYGWIN__
   {
      EXPLICIT_SYMBOL(_alloca);
      EXPLICIT_SYMBOL(__main);
   }
#endif

#undef EXPLICIT_SYMBOL

   // This macro returns the address of a well-known, explicit symbol
#define EXPLICIT_SYMBOL(SYM) \
   if (!strcmp(symbolName, #SYM)) return &SYM

   // Under glibc we have a weird situation. The stderr/out/in symbols are both
   // macros and global variables because of standards requirements. So, we
   // boldly use the EXPLICIT_SYMBOL macro without checking for a #define first.
#if defined(__GLIBC__)
   {
      EXPLICIT_SYMBOL(stderr);
      EXPLICIT_SYMBOL(stdout);
      EXPLICIT_SYMBOL(stdin);
   }
#else
   // For everything else, we want to check to make sure the symbol isn't defined
   // as a macro before using EXPLICIT_SYMBOL.
   {
#ifndef stdin
      EXPLICIT_SYMBOL(stdin);
#endif
#ifndef stdout
      EXPLICIT_SYMBOL(stdout);
#endif
#ifndef stderr
      EXPLICIT_SYMBOL(stderr);
#endif
   }
#endif
#undef EXPLICIT_SYMBOL

   return nullptr;
}

} // sys
} // polar
