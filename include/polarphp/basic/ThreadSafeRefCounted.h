//===--- ThreadSafeRefCounted.h - Thread-safe Refcounting Base --*- C++ -*-===//
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
// Created by polarboy on 2019/12/02.


#ifndef POLARPHP_BASIC_THREADSAFE_REFCOUNTED_H
#define POLARPHP_BASIC_THREADSAFE_REFCOUNTED_H

#include <atomic>
#include <cassert>
#include "llvm/ADT/IntrusiveRefCntPtr.h"

namespace polar {

/// A class that has the same function as \c ThreadSafeRefCountedBase, but with
/// a virtual destructor.
///
/// Should be used instead of \c ThreadSafeRefCountedBase for classes that
/// already have virtual methods to enforce dynamic allocation via 'new'.
/// FIXME: This should eventually move to llvm.
class ThreadSafeRefCountedBaseVPTR
{
   mutable std::atomic<unsigned> m_refCnt;
   virtual void anchor();

protected:
   ThreadSafeRefCountedBaseVPTR()
      : m_refCnt(0) {}

   virtual ~ThreadSafeRefCountedBaseVPTR() {}

public:
   void retain() const
   {
      m_refCnt += 1;
   }

   void release() const
   {
      int refCount = static_cast<int>(--m_refCnt);
      assert(refCount >= 0 && "Reference count was already zero.");
      if (refCount == 0) delete this;
   }
};

} // polar

#endif // POLARPHP_BASIC_THREADSAFE_REFCOUNTED_H
