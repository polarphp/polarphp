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

#ifndef POLARPHP_UTILS_INTERNAL_DYNAMIC_LIBRARY_HANDLE_SET_H
#define POLARPHP_UTILS_INTERNAL_DYNAMIC_LIBRARY_HANDLE_SET_H

#include <vector>
#include <string>
#include "polarphp/global/CompilerDetection.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/utils/DynamicLibrary.h"

namespace polar {
namespace sys {
namespace internal {

// All methods for HandleSet should be used holding sg_symbolsMutex.
class HandleSet
{
   typedef std::vector<void *> HandleList;
   HandleList m_handles;
   void *m_process;

public:
   static void *dllOpen(const char *filename, std::string *errorMsg);
   static void dllClose(void *handle);
   static void *dllSym(void *handle, const char *symbol);

   HandleSet() : m_process(nullptr)
   {}
   ~HandleSet();

   HandleList::iterator find(void *handle)
   {
      return std::find(m_handles.begin(), m_handles.end(), handle);
   }

   bool contains(void *handle)
   {
      return handle == m_process || find(handle) != m_handles.end();
   }

   bool addLibrary(void *handle, bool isProcess = false, bool canClose = true) {
#ifdef POLAR_ON_WIN32
      assert((handle == this ? isProcess : !isProcess) && "Bad handle.");
#endif

      if (POLAR_LIKELY(!isProcess)) {
         if (find(handle) != m_handles.end()) {
            if (canClose) {
               dllClose(handle);
            }
            return false;
         }
         m_handles.push_back(handle);
      } else {
#ifndef POLAR_ON_WIN32
         if (m_process) {
            if (canClose) {
               dllClose(m_process);
            }
            if (m_process == handle) {
               return false;
            }
         }
#endif
         m_process = handle;
      }
      return true;
   }

   void *libLookup(const char *symbol, DynamicLibrary::SearchOrdering order) {
      if (order & DynamicLibrary::SearchOrdering::SO_LoadOrder) {
         for (void *handle : m_handles) {
            if (void *ptr = dllSym(handle, symbol)) {
               return ptr;
            }
         }
      } else {
         for (void *handle : polar::basic::reverse(m_handles)) {
            if (void *ptr = dllSym(handle, symbol)) {
               return ptr;
            }
         }
      }
      return nullptr;
   }

   void *lookup(const char *symbol, DynamicLibrary::SearchOrdering order)
   {
      assert(!((order & DynamicLibrary::SearchOrdering::SO_LoadedFirst) &&
               (order & DynamicLibrary::SearchOrdering::SO_LoadedLast)) &&
             "Invalid Ordering");

      if (!m_process || (order & DynamicLibrary::SearchOrdering::SO_LoadedFirst)) {
         if (void *ptr = libLookup(symbol, order)) {
            return ptr;
         }
      }
      if (m_process) {
         // Use OS facilities to search the current binary and all loaded libs.
         if (void *ptr = dllSym(m_process, symbol)) {
            return ptr;
         }
         // Search any libs that might have been skipped because of RTLD_LOCAL.
         if (order & DynamicLibrary::SearchOrdering::SO_LoadedLast) {
            if (void *ptr = libLookup(symbol, order)) {
               return ptr;
            }
         }
      }
      return nullptr;
   }
};

} // internal
} // utils
} // polar

#endif // POLARPHP_UTILS_INTERNAL_DYNAMIC_LIBRARY_HANDLE_SET_H
