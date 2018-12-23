// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/09.

#ifndef POLARPHP_BASIC_ADT_EPOCH_TRACKER_H
#define POLARPHP_BASIC_ADT_EPOCH_TRACKER_H

#include "polarphp/global/AbiBreaking.h"
#include <cstdint>

namespace polar {
namespace basic {

#if POLAR_ENABLE_ABI_BREAKING_CHECKS

/// A base class for data structure classes wishing to make iterators
/// ("handles") pointing into themselves fail-fast.  When building without
/// asserts, this class is empty and does nothing.
///
/// DebugEpochBase does not by itself track handles pointing into itself.  The
/// expectation is that routines touching the handles will poll on
/// isHandleInSync at appropriate points to assert that the handle they're using
/// is still valid.
///
class DebugEpochBase
{
   uint64_t m_epoch;

public:
   DebugEpochBase() : m_epoch(0)
   {}

   /// Calling incrementEpoch invalidates all handles pointing into the
   /// calling instance.
   void incrementEpoch()
   {
      ++m_epoch;
   }

   /// The destructor calls incrementEpoch to make use-after-free bugs
   /// more likely to crash deterministically.
   ~DebugEpochBase()
   {
      incrementEpoch();
   }

   /// A base class for iterator classes ("handles") that wish to poll for
   /// iterator invalidating modifications in the underlying data structure.
   /// When LLVM is built without asserts, this class is empty and does nothing.
   ///
   /// HandleBase does not track the parent data structure by itself.  It expects
   /// the routines modifying the data structure to call incrementEpoch when they
   /// make an iterator-invalidating modification.
   ///
   class HandleBase
   {
      const uint64_t *m_epochAddress;
      uint64_t m_epochAtCreation;

   public:
      HandleBase() : m_epochAddress(nullptr), m_epochAtCreation(UINT64_MAX)
      {}

      explicit HandleBase(const DebugEpochBase *parent)
         : m_epochAddress(&parent->m_epoch),
           m_epochAtCreation(parent->m_epoch)
      {}

      /// Returns true if the DebugEpochBase this Handle is linked to has
      /// not called incrementEpoch on itself since the creation of this
      /// HandleBase instance.
      bool isHandleInSync() const
      {
         return *m_epochAddress == m_epochAtCreation;
      }

      /// Returns a pointer to the epoch word stored in the data structure
      /// this handle points into.  Can be used to check if two iterators point
      /// into the same data structure.
      const void *getEpochAddress() const
      {
         return m_epochAddress;
      }
   };
};

#else

class DebugEpochBase
{
public:
   void incrementEpoch()
   {}

   class HandleBase
   {
   public:
      HandleBase() = default;
      explicit HandleBase(const DebugEpochBase *)
      {}

      bool isHandleInSync() const
      {
         return true;
      }

      const void *getEpochAddress() const
      {
         return nullptr;
      }
   };
};

#endif // POLAR_ENABLE_ABI_BREAKING_CHECKS

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_EPOCH_TRACKER_H
