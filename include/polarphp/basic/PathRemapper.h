//===--- PathRemapper.h - Transforms path prefixes --------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
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
// Created by polarboy on 2019/11/27.

//===----------------------------------------------------------------------===//
//
//  This file defines a data structure that stores a string-to-string
//  mapping used to transform file paths based on a prefix mapping. It
//  is optimized for the common case, which is that there will be
//  extremely few mappings (i.e., one or two).
//
//  Remappings are stored such that they are applied in the order they
//  are passed on the command line. This would only matter if one
//  source mapping was a prefix of another.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_PATH_REMAPPER_H
#define POLARPHP_BASIC_PATH_REMAPPER_H

#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/Twine.h"

#include <string>
#include <utility>

namespace polar {

using llvm::SmallVector;
using llvm::StringRef;
using llvm::Twine;

class PathRemapper
{
public:
   /// Adds a mapping such that any paths starting with `FromPrefix` have that
   /// portion replaced with `ToPrefix`.
   void addMapping(StringRef fromPrefix, StringRef toPrefix)
   {
      m_pathMappings.emplace_back(fromPrefix, toPrefix);
   }

   /// Returns a remapped `Path` if it starts with a prefix in the map; otherwise
   /// the original path is returned.
   std::string remapPath(StringRef path) const
   {
      // Clang's implementation of this feature also compares the path string
      // directly instead of treating path segments as indivisible units. The
      // latter would arguably be more accurate, but we choose to preserve
      // compatibility with Clang (especially because we propagate the flag to
      // ClangImporter as well).
      for (const auto &mapping : m_pathMappings) {
         if (path.startswith(mapping.first)) {
            return (Twine(mapping.second) +
                    path.substr(mapping.first.size())).str();
         }
      }
      return path.str();
   }

private:
   SmallVector<std::pair<std::string, std::string>, 2> m_pathMappings;
};

} // polar

#endif // POLARPHP_BASIC_PATH_REMAPPER_H
