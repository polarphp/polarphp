//===--- SearchPathOptions.h ------------------------------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2018/06/29.


#ifndef POLARPHP_AST_SEARCHPATHOPTIONS_H
#define POLARPHP_AST_SEARCHPATHOPTIONS_H

#include "llvm/ADT/Hashing.h"
#include <string>
#include <vector>

namespace llvm {
class StringRef;
}

namespace polar {

using llvm::StringRef;

/// Options for controlling search path behavior.
class SearchPathOptions
{
public:
   /// Path(s) which should be searched for modules.
   ///
   /// Do not add values to this directly. Instead, use
   /// \c AstContext::addSearchPath.
   std::vector<std::string> importSearchPaths;

   /// Path(s) to virtual filesystem overlay YAML files.
   std::vector<std::string> vfsOverlayFiles;

   struct FrameworkSearchPath
   {
      std::string path;
      bool isSystem = false;
      FrameworkSearchPath(StringRef path, bool isSystem)
         : path(path),
           isSystem(isSystem)
      {}

      friend bool operator ==(const FrameworkSearchPath &lhs,
                              const FrameworkSearchPath &rhs)
      {
         return lhs.path == rhs.path && lhs.isSystem == rhs.isSystem;
      }

      friend bool operator !=(const FrameworkSearchPath &lhs,
                              const FrameworkSearchPath &rhs) {
         return !(lhs == rhs);
      }
   };
   /// Path(s) which should be searched for frameworks.
   ///
   /// Do not add values to this directly. Instead, use
   /// \c AstContext::addSearchPath.
   std::vector<FrameworkSearchPath> frameworkSearchPaths;

   /// Path(s) which should be searched for libraries.
   ///
   /// This is used in immediate modes. It is safe to add paths to this directly.
   std::vector<std::string> librarySearchPaths;

   /// Path to search for compiler-relative header files.
   std::string runtimeResourcePath;

   /// Path to search for compiler-relative stdlib dylibs.
   std::string runtimeLibraryPath;

   /// Path to search for compiler-relative stdlib modules.
   std::string runtimeLibraryImportPath;

   /// Don't look in for compiler-provided modules.
   bool skipRuntimeLibraryImportPath = false;
};

} // polar

#endif // POLARPHP_AST_SEARCHPATHOPTIONS_H
