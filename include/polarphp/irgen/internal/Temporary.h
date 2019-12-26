//===--- Temporary.h - A temporary allocation -------------------*- C++ -*-===//
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
//  This file defines the Temporary and TemporarySet classes.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_IRGEN_INTERNAL_TEMPORARY_H
#define POLARPHP_IRGEN_INTERNAL_TEMPORARY_H

#include "polarphp/irgen/internal/Address.h"
#include "polarphp/pil/lang/PILType.h"
#include <vector>

namespace polar::irgen {

class IRGenFunction;

/// A temporary allocation.
class Temporary {
public:
   StackAddress Addr;
   PILType Type;

   void destroy(IRGenFunction &IGF) const;
};

class TemporarySet {
   std::vector<Temporary> Stack;
   bool HasBeenCleared = false;

public:
   TemporarySet() = default;

   TemporarySet(TemporarySet &&) = default;
   TemporarySet &operator=(TemporarySet &&) = default;

   // Make this move-only to reduce chances of double-destroys.  We can't
   // get too strict with this, though, because we may need to destroy
   // the same set of temporaries along multiple control-flow paths.
   TemporarySet(const TemporarySet &) = delete;
   TemporarySet &operator=(const TemporarySet &) = delete;

   void add(Temporary temp) {
      Stack.push_back(temp);
   }

   /// Destroy all the temporaries.
   void destroyAll(IRGenFunction &IGF) const;

   /// Remove all the temporaries from this set.  This does not destroy
   /// the temporaries.
   void clear() {
      assert(!HasBeenCleared && "already cleared");
      HasBeenCleared = true;
      Stack.clear();
   }

   /// Has clear() been called on this set?
   bool hasBeenCleared() const {
      return HasBeenCleared;
   }
};

} // polar::irgen

#endif // POLARPHP_IRGEN_INTERNAL_TEMPORARY_H
