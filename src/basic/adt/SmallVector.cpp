// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/06.

#include "polarphp/basic/adt/SmallVector.h"

namespace polar {
namespace basic {

// Check that no bytes are wasted and everything is well-aligned.
namespace {
struct Struct16Byte
{
   alignas(16) void *m_data;
};

struct Struct32Byte
{
   alignas(32) void *m_data;
};
}

static_assert(sizeof(SmallVector<void *, 0>) ==
              sizeof(unsigned) * 2 + sizeof(void *),
              "wasted space in SmallVector size 0");
static_assert(alignof(SmallVector<Struct16Byte, 0>) >= alignof(Struct16Byte),
              "wrong alignment for 16-byte aligned T");
static_assert(alignof(SmallVector<Struct32Byte, 0>) >= alignof(Struct32Byte),
              "wrong alignment for 32-byte aligned T");
static_assert(sizeof(SmallVector<Struct16Byte, 0>) >= alignof(Struct16Byte),
              "missing padding for 16-byte aligned T");
static_assert(sizeof(SmallVector<Struct32Byte, 0>) >= alignof(Struct32Byte),
              "missing padding for 32-byte aligned T");
static_assert(sizeof(SmallVector<void *, 1>) ==
              sizeof(unsigned) * 2 + sizeof(void *) * 2,
              "wasted space in SmallVector size 1");

/// growPod - This is an implementation of the grow() method which only works
/// on POD-like datatypes and is out of line to reduce code duplication.
void SmallVectorBase::growPod(void *firstEl, size_t minCapacity,
                              size_t tsize)
{
   // Ensure we can fit the new capacity in 32 bits.
   if (minCapacity > UINT32_MAX) {
      polar::utils::report_bad_alloc_error("SmallVector capacity overflow during allocation");
   }
   size_t newCapacity = 2 * getCapacity() + 1; // Always grow.
   newCapacity =
         std::min(std::max(newCapacity, minCapacity), size_t(UINT32_MAX));

   void *newElts;
   if (m_beginX == firstEl) {
      newElts = polar::utils::safe_malloc(newCapacity * tsize);

      // Copy the elements over.  No need to run dtors on PODs.
      memcpy(newElts, this->m_beginX, getSize() * tsize);
   } else {
      // If this wasn't grown from the inline copy, grow the allocated space.
      newElts = polar::utils::safe_realloc(this->m_beginX, newCapacity * tsize);
   }

   this->m_beginX = newElts;
   this->m_capacity = newCapacity;
}

} // utils
} // polar
