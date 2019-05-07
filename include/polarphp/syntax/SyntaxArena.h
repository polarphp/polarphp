//===--- SyntaxArena.h - Syntax Tree Memory Allocation ----------*- C++ -*-===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/07.
//
//===----------------------------------------------------------------------===//
//
// This file defines SyntaxArena that is Memory manager for Syntax nodes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_SYNTAX_SYNTAXARENA_H
#define POLARPHP_SYNTAX_SYNTAXARENA_H

#include "polarphp/basic/adt/IntrusiveRefCountPtr.h".h"
#include "polarphp/utils/Allocator.h"

namespace polar::syntax {

using polar::utils::BumpPtrAllocator;
using polar::basic::ThreadSafeRefCountedBase;

/// Memory manager for Syntax nodes.
class SyntaxArena : public ThreadSafeRefCountedBase<SyntaxArena>
{
public:
   SyntaxArena()
   {}

   polar::utils::BumpPtrAllocator &getAllocator()
   {
      return m_allocator;
   }

   void *allocate(size_t size, size_t alignment)
   {
      return m_allocator.allocate(size, alignment);
   }

private:
   SyntaxArena(const SyntaxArena &) = delete;
   void operator=(const SyntaxArena &) = delete;
   BumpPtrAllocator m_allocator;
};

} // polar::syntax

#endif // POLARPHP_SYNTAX_SYNTAXARENA_H
