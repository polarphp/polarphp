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
//
// This source file is part of the polarphp.org open source project
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_AST_SEARCHPATH_OPTIONS_H
#define POLARPHP_AST_SEARCHPATH_OPTIONS_H

#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/StringRef.h"
#include <string>
#include <vector>

namespace polar::ast {

using llvm::StringRef;

/// Options for controlling search path behavior.
class SearchPathOptions
{
public:
   /// path to the SDK which is being built against.
   std::string sdkPath;

   /// path(s) which should be searched for modules.
   ///
   /// Do not add values to this directly. Instead, use
   /// \c ASTContext::addSearchPath.
   std::vector<std::string> importSearchPaths;

   /// path(s) to virtual filesystem overlay YAML files.
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
                              const FrameworkSearchPath &rhs)
      {
         return !(lhs == rhs);
      }
   };
   /// path(s) which should be searched for frameworks.
   ///
   /// Do not add values to this directly. Instead, use
   /// \c ASTContext::addSearchPath.
   std::vector<FrameworkSearchPath> frameworkSearchPaths;

   /// path(s) which should be searched for libraries.
   ///
   /// This is used in immediate modes. It is safe to add paths to this directly.
   std::vector<std::string> librarySearchPaths;

   /// path to search for compiler-relative header files.
   std::string runtimeResourcePath;

   /// Paths to search for compiler-relative stdlib dylibs, in order of
   /// preference.
   std::vector<std::string> runtimeLibraryPaths;

   /// Paths to search for stdlib modules. One of these will be compiler-relative.
   std::vector<std::string> runtimeLibraryImportPaths;

   /// Don't look in for compiler-provided modules.
   bool skipRuntimeLibraryImportPaths = false;

   /// Return a hash code of any components from these options that should
   /// contribute to a Swift Bridging PCH hash.
   llvm::hash_code getPCHHashComponents() const
   {
      using llvm::hash_value;
      using llvm::hash_combine;
      auto code = hash_value(sdkPath);
      for (auto importItem : importSearchPaths) {
         code = hash_combine(code, importItem);
      }
      for (auto vfsFile : vfsOverlayFiles) {
         code = hash_combine(code, vfsFile);
      }
      for (const auto &frameworkPath : frameworkSearchPaths) {
         code = hash_combine(code, frameworkPath.path);
      }
      for (auto libraryPath : librarySearchPaths) {
         code = hash_combine(code, libraryPath);
      }
      code = hash_combine(code, runtimeResourcePath);
      for (auto runtimeLibraryImportPath : runtimeLibraryImportPaths) {
         code = hash_combine(code, runtimeLibraryImportPath);
      }
      return code;
   }
};


} // polar::ast

#endif // POLARPHP_AST_SEARCHPATH_OPTIONS_H
