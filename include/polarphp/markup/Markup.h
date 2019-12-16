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

#ifndef POLARPHP_MARKUP_MARKUP_H
#define POLARPHP_MARKUP_MARKUP_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Allocator.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/basic/SourceLoc.h"
#include "polarphp/markup/Ast.h"
#include "polarphp/markup/LineList.h"

namespace polar {
struct RawComment;
} // polar

namespace polar::markup {

using polar::RawComment;
class LineList;

class MarkupContext final {
   llvm::BumpPtrAllocator Allocator;

public:
   void *allocate(unsigned long Bytes, unsigned Alignment) {
      return Allocator.Allocate(Bytes, Alignment);
   }

   template <typename T, typename It>
   T *allocateCopy(It Begin, It End) {
      T *Res =
         static_cast<T *>(allocate(sizeof(T) * (End - Begin), alignof(T)));
      for (unsigned i = 0; Begin != End; ++Begin, ++i)
         new (Res + i) T(*Begin);
      return Res;
   }

   template <typename T>
   MutableArrayRef<T> allocateCopy(ArrayRef<T> Array) {
      return MutableArrayRef<T>(allocateCopy<T>(Array.begin(), Array.end()),
                                Array.size());
   }

   StringRef allocateCopy(StringRef Str) {
      ArrayRef<char> Result =
         allocateCopy(llvm::makeArrayRef(Str.data(), Str.size()));
      return StringRef(Result.data(), Result.size());
   }

   LineList getLineList(RawComment RC);
};

Document *parseDocument(MarkupContext &MC, LineList &LL);

} // namespace polar::markup

#endif // POLARPHP_MARKUP_MARKUP_H
