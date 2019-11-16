//===--- Markup.h - Markup --------------------------------------*- C++ -*-===//
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
// Created by polarboy on 2019/04/27.

#ifndef POLARPHP_MARKUP_MARKUP_H
#define POLARPHP_MARKUP_MARKUP_H

#include "llvm/ADTArrayRef.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/parser/SourceLoc.h"
#include "polarphp/markup/AST.h"
#include "polarphp/markup/LineList.h"

/// forward declare class with namespace
namespace polar::ast {
struct RawComment;
}

namespace polar::markup {

using polar::utils::BumpPtrAllocator;
using polar::ast::RawComment;
class LineList;

class MarkupContext final
{
   BumpPtrAllocator m_allocator;

public:
   void *allocate(unsigned long bytes, unsigned alignment)
   {
      return m_allocator.allocate(bytes, alignment);
   }

   template <typename T, typename Iter>
   T *allocateCopy(Iter begin, Iter end)
   {
      T *result =
            static_cast<T *>(allocate(sizeof(T) * (end - begin), alignof(T)));
      for (unsigned i = 0; begin != end; ++begin, ++i) {
         new (result + i) T(*begin);
      }
      return result;
   }

   template <typename T>
   MutableArrayRef<T> allocateCopy(ArrayRef<T> array)
   {
      return MutableArrayRef<T>(allocateCopy<T>(array.begin(), array.end()),
                                array.size());
   }

   StringRef allocateCopy(StringRef str)
   {
      ArrayRef<char> result =
            allocateCopy(polar::basic::make_array_ref(str.data(), str.size()));
      return StringRef(result.data(), result.size());
   }

   LineList getLineList(RawComment rawComment);
};

Document *parse_document(MarkupContext &markupContext, LineList &lineList);

} // polar::markup

#endif // POLARPHP_MARKUP_MARKUP_H
