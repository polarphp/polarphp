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

#include "polarphp/utils/DynamicLibrary.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/basic/adt/DenseSet.h"
#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/utils/internal/DynamicLibraryHandleSetPrivate.h"
#include <cstdio>
#include <cstring>
#include <mutex>

namespace polar {
namespace sys {

using polar::basic::StringMap;
using polar::utils::ManagedStatic;

namespace {
// Collection of symbol name/value pairs to be searched prior to any libraries.
static ManagedStatic<StringMap<void *>> sg_explicitSymbols;
// Collection of known library handles.
static ManagedStatic<internal::HandleSet> sg_openedHandles;
// Lock for sg_explicitSymbols and sg_openedHandles.
static ManagedStatic<std::mutex> sg_symbolsMutex;
}

char DynamicLibrary::sm_invalid;
DynamicLibrary::SearchOrdering DynamicLibrary::sm_searchOrder =
      DynamicLibrary::SO_Linker;

extern void *do_search(const char *);

void *search_for_address_of_special_symbol(const char *symbolName)
{
   return do_search(symbolName); // DynamicLibrary.inc
}

void DynamicLibrary::addSymbol(StringRef symbolName, void *symbolValue)
{
   std::lock_guard locker(*sg_symbolsMutex);
   (*sg_explicitSymbols)[symbolName] = symbolValue;
}

DynamicLibrary DynamicLibrary::getPermanentLibrary(const char *filename,
                                                   std::string *errorMsg)
{
   // Force sg_openedHandles to be added into the ManagedStatic list before any
   // ManagedStatic can be added from static constructors in HandleSet::dllOpen.
   internal::HandleSet& handleSet = *sg_openedHandles;
   void *handle = internal::HandleSet::dllOpen(filename, errorMsg);
   if (handle != &sm_invalid) {
      std::lock_guard locker(*sg_symbolsMutex);
      handleSet.addLibrary(handle, /*isProcess*/ filename == nullptr);
   }
   return DynamicLibrary(handle);
}

DynamicLibrary DynamicLibrary::addPermanentLibrary(void *handle,
                                                   std::string *errorMsg)
{
   std::lock_guard locker(*sg_symbolsMutex);
   // If we've already loaded this library, tell the caller.
   if (!sg_openedHandles->addLibrary(handle, /*isProcess*/false, /*canClose*/false)) {
      *errorMsg = "Library already loaded";
   }
   return DynamicLibrary(handle);
}

void *DynamicLibrary::getAddressOfSymbol(const char *symbolName)
{
   if (!isValid()) {
      return nullptr;
   }
   return internal::HandleSet::dllSym(m_data, symbolName);
}

void *DynamicLibrary::searchForAddressOfSymbol(const char *symbolName)
{
   {
      std::lock_guard locker(*sg_symbolsMutex);
      // First check symbols added via addSymbol().
      if (sg_explicitSymbols.isConstructed()) {
         StringMap<void *>::iterator i = sg_explicitSymbols->find(symbolName);
         if (i != sg_explicitSymbols->end()) {
            return i->m_second;
         }
      }
      // Now search the libraries.
      if (sg_openedHandles.isConstructed()) {
         if (void *ptr = sg_openedHandles->lookup(symbolName, sm_searchOrder)) {
            return ptr;
         }
      }
   }

   return search_for_address_of_special_symbol(symbolName);
}

} // sys
} // polar
