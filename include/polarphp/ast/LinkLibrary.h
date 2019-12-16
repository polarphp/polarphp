//===--- LinkLibrary.h - A module-level linker dependency -------*- C++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_AST_LINK_LIBRARY_H
#define POLARPHP_AST_LINK_LIBRARY_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/SmallString.h"
#include <string>

namespace polar {

// Must be kept in sync with diag::error_immediate_mode_missing_library.
enum class LibraryKind
{
   Library = 0,
   Framework
};

/// Represents a linker dependency for an imported module.
// FIXME: This is basically a slightly more generic version of Clang's
// Module::LinkLibrary.
class LinkLibrary
{
public:
   LinkLibrary(StringRef name, LibraryKind kind, bool forceLoad = false)
      : m_name(name),
        m_kind(static_cast<unsigned>(kind)),
        m_forceLoad(forceLoad)
   {
      assert(getKind() == kind && "not enough bits for the kind");
   }

   LibraryKind getKind() const
   {
      return static_cast<LibraryKind>(m_kind);
   }

   StringRef getName() const
   {
      return m_name;
   }

   bool shouldForceLoad() const
   {
      return m_forceLoad;
   }

private:
   std::string m_name;
   unsigned m_kind : 1;
   unsigned m_forceLoad : 1;
};

} // polar

#endif // POLARPHP_AST_LINK_LIBRARY_H
