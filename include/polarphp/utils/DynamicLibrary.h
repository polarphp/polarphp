// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/01.

#ifndef POLARPHP_UTILS_DYNAMIC_LIBRARY_H
#define POLARPHP_UTILS_DYNAMIC_LIBRARY_H

#include <string>
#include <vector>
#include <algorithm>
#include "polarphp/global/Global.h"

namespace polar {

// forward declare class with namespace
namespace basic {
class StringRef;
} // basic

namespace sys {

using polar::basic::StringRef;

namespace internal {
class HandleSet;
}

using internal::HandleSet;

/// This class provides a portable interface to dynamic libraries which also
/// might be known as shared libraries, shared objects, dynamic shared
/// objects, or dynamic link libraries. Regardless of the terminology or the
/// operating system interface, this class provides a portable interface that
/// allows dynamic libraries to be loaded and searched for externally
/// defined symbols. This is typically used to provide "plug-in" support.
/// It also allows for symbols to be defined which don't live in any library,
/// but rather the main program itself, useful on Windows where the main
/// executable cannot be searched.
///
/// Note: there is currently no interface for temporarily loading a library,
/// or for unloading libraries when the LLVM library is unloaded.
class DynamicLibrary
{
public:
   explicit DynamicLibrary(void *data = &sm_invalid) : m_data(data)
   {}

   /// Returns true if the object refers to a valid library.
   bool isValid() const
   {
      return m_data != &sm_invalid;
   }

   /// Searches through the library for the symbol \p symbolName. If it is
   /// found, the address of that symbol is returned. If not, NULL is returned.
   /// Note that NULL will also be returned if the library failed to load.
   /// Use isValid() to distinguish these cases if it is important.
   /// Note that this will \e not search symbols explicitly registered by
   /// addSymbol().
   void *getAddressOfSymbol(const char *symbolName);

   /// This function permanently loads the dynamic library at the given path.
   /// The library will only be unloaded when llvm_shutdown() is called.
   /// This returns a valid DynamicLibrary instance on success and an invalid
   /// instance on failure (see isValid()). \p *errMsg will only be modified
   /// if the library fails to load.
   ///
   /// It is safe to call this function multiple times for the same library.
   /// @brief Open a dynamic library permanently.
   static DynamicLibrary getPermanentLibrary(const char *filename,
                                             std::string *errMsg = nullptr);

   /// Registers an externally loaded library. The library will be unloaded
   /// when the program terminates.
   ///
   /// It is safe to call this function multiple times for the same library,
   /// though ownership is only taken if there was no error.
   ///
   /// \returns An empty \p DynamicLibrary if the library was already loaded.
   static DynamicLibrary addPermanentLibrary(void *handle,
                                             std::string *errMsg = nullptr);

   /// This function permanently loads the dynamic library at the given path.
   /// Use this instead of getPermanentLibrary() when you won't need to get
   /// symbols from the library itself.
   ///
   /// It is safe to call this function multiple times for the same library.
   static bool loadLibraryPermanently(const char *filename,
                                      std::string *errorMsg = nullptr)
   {
      return !getPermanentLibrary(filename, errorMsg).isValid();
   }

   enum SearchOrdering
   {
      /// SO_Linker - Search as a call to dlsym(dlopen(NULL)) would when
      /// DynamicLibrary::getPermanentLibrary(NULL) has been called or
      /// search the list of explcitly loaded symbols if not.
      SO_Linker,
      /// SO_LoadedFirst - Search all loaded libraries, then as SO_Linker would.
      SO_LoadedFirst,
      /// SO_LoadedLast - Search as SO_Linker would, then loaded libraries.
      /// Only useful to search if libraries with RTLD_LOCAL have been added.
      SO_LoadedLast,
      /// SO_LoadOrder - Or this in to search libraries in the ordered loaded.
      /// The default bahaviour is to search loaded libraries in reverse.
      SO_LoadOrder = 4
   };
   static SearchOrdering sm_searchOrder; // = SO_Linker

   /// This function will search through all previously loaded dynamic
   /// libraries for the symbol \p symbolName. If it is found, the address of
   /// that symbol is returned. If not, null is returned. Note that this will
   /// search permanently loaded libraries (getPermanentLibrary()) as well
   /// as explicitly registered symbols (addSymbol()).
   /// @throws std::string on error.
   /// @brief Search through libraries for address of a symbol
   static void *searchForAddressOfSymbol(const char *symbolName);

   /// @brief Convenience function for C++ophiles.
   static void *searchForAddressOfSymbol(const std::string &symbolName)
   {
      return searchForAddressOfSymbol(symbolName.c_str());
   }

   /// This functions permanently adds the symbol \p symbolName with the
   /// value \p symbolValue.  These symbols are searched before any
   /// libraries.
   /// @brief Add searchable symbol/value pair.
   static void addSymbol(StringRef symbolName, void *symbolValue);
private:
   friend class HandleSet;
   // Placeholder whose address represents an invalid library.
   // We use this instead of NULL or a pointer-int pair because the OS library
   // might define 0 or 1 to be "special" handles, such as "search all".
   static char sm_invalid;

   // Opaque data used to interface with OS-specific dynamic library handling.
   void *m_data;
};

} // utils
} // polar

#endif // POLARPHP_UTILS_DYNAMIC_LIBRARY_H
