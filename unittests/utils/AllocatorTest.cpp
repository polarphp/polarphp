// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/06/03.

#include "gtest/gtest.h"
#include "polarphp/utils/Allocator.h"
#include <cstdlib>

using polar::utils::BumpPtrAllocator;

TEST(AllocatorTest, testBasics)
{
   BumpPtrAllocator alloc;
   int *a = (int*)alloc.allocate(sizeof(int), alignof(int));
   int *b = (int*)alloc.allocate(sizeof(int) * 10, alignof(int));
   int *c = (int*)alloc.allocate(sizeof(int), alignof(int));
   *a = 1;
   b[0] = 2;
   b[9] = 2;
   *c = 3;
   EXPECT_EQ(1, *a);
   EXPECT_EQ(2, b[0]);
   EXPECT_EQ(2, b[9]);
   EXPECT_EQ(3, *c);
   EXPECT_EQ(1U, alloc.getNumSlabs());

   BumpPtrAllocator alloc2 = std::move(alloc);
   EXPECT_EQ(0U, alloc.getNumSlabs());
   EXPECT_EQ(1U, alloc2.getNumSlabs());

   // Make sure the old pointers still work. These are especially interesting
   // under ASan or Valgrind.
   EXPECT_EQ(1, *a);
   EXPECT_EQ(2, b[0]);
   EXPECT_EQ(2, b[9]);
   EXPECT_EQ(3, *c);

   alloc = std::move(alloc2);
   EXPECT_EQ(0U, alloc2.getNumSlabs());
   EXPECT_EQ(1U, alloc.getNumSlabs());
}

// Allocate enough bytes to create three slabs.
TEST(AllocatorTest, testThreeSlabs)
{
   BumpPtrAllocator alloc;
   alloc.allocate(3000, 1);
   EXPECT_EQ(1U, alloc.getNumSlabs());
   alloc.allocate(3000, 1);
   EXPECT_EQ(2U, alloc.getNumSlabs());
   alloc.allocate(3000, 1);
   EXPECT_EQ(3U, alloc.getNumSlabs());
}

// Allocate enough bytes to create two slabs, reset the allocator, and do it
// again.
TEST(AllocatorTest, testReset)
{
   BumpPtrAllocator alloc;

   // Allocate something larger than the SizeThreshold=4096.
   (void)alloc.allocate(5000, 1);
   alloc.reset();
   // Calling Reset should free all CustomSizedSlabs.
   EXPECT_EQ(0u, alloc.getNumSlabs());

   alloc.allocate(3000, 1);
   EXPECT_EQ(1U, alloc.getNumSlabs());
   alloc.allocate(3000, 1);
   EXPECT_EQ(2U, alloc.getNumSlabs());
   alloc.reset();
   EXPECT_EQ(1U, alloc.getNumSlabs());
   alloc.allocate(3000, 1);
   EXPECT_EQ(1U, alloc.getNumSlabs());
   alloc.allocate(3000, 1);
   EXPECT_EQ(2U, alloc.getNumSlabs());
}

// Test some allocations at varying alignments.
TEST(AllocatorTest, testAlignment)
{
   BumpPtrAllocator alloc;
   uintptr_t a;
   a = (uintptr_t)alloc.allocate(1, 2);
   EXPECT_EQ(0U, a & 1);
   a = (uintptr_t)alloc.allocate(1, 4);
   EXPECT_EQ(0U, a & 3);
   a = (uintptr_t)alloc.allocate(1, 8);
   EXPECT_EQ(0U, a & 7);
   a = (uintptr_t)alloc.allocate(1, 16);
   EXPECT_EQ(0U, a & 15);
   a = (uintptr_t)alloc.allocate(1, 32);
   EXPECT_EQ(0U, a & 31);
   a = (uintptr_t)alloc.allocate(1, 64);
   EXPECT_EQ(0U, a & 63);
   a = (uintptr_t)alloc.allocate(1, 128);
   EXPECT_EQ(0U, a & 127);
}


// Test allocating just over the slab size.  This tests a bug where before the
// allocator incorrectly calculated the buffer end pointer.
TEST(AllocatorTest, testOverflow)
{
   BumpPtrAllocator alloc;

   // Fill the slab right up until the end pointer.
   alloc.allocate(4096, 1);
   EXPECT_EQ(1U, alloc.getNumSlabs());

   // If we don't allocate a new slab, then we will have overflowed.
   alloc.allocate(1, 1);
   EXPECT_EQ(2U, alloc.getNumSlabs());
}

// Test allocating with a size larger than the initial slab size.
TEST(AllocatorTest, testSmallSlabSize)
{
   BumpPtrAllocator alloc;

   alloc.allocate(8000, 1);
   EXPECT_EQ(1U, alloc.getNumSlabs());
}

// Test requesting alignment that goes past the end of the current slab.
TEST(AllocatorTest, testAlignmentPastSlab)
{
   BumpPtrAllocator alloc;
   alloc.allocate(4095, 1);

   // Aligning the current slab pointer is likely to move it past the end of the
   // slab, which would confuse any unsigned comparisons with the difference of
   // the end pointer and the aligned pointer.
   alloc.allocate(1024, 8192);

   EXPECT_EQ(2U, alloc.getNumSlabs());
}

namespace {

// Mock slab allocator that returns slabs aligned on 4096 bytes.  There is no
// easy portable way to do this, so this is kind of a hack.
class MockSlabAllocator
{
   static size_t m_lastSlabSize;

public:
   ~MockSlabAllocator() { }

   void *allocate(size_t size, size_t /*Alignment*/) {
      // Allocate space for the alignment, the slab, and a void* that goes right
      // before the slab.
      size_t alignment = 4096;
      void *memBase = polar::utils::safe_malloc(size + alignment - 1 + sizeof(void*));

      // Find the slab start.
      void *slab = (void *)polar::utils::align_addr((char*)memBase + sizeof(void *), alignment);

      // Hold a pointer to the base so we can free the whole malloced block.
      ((void**)slab)[-1] = memBase;

      m_lastSlabSize = size;
      return slab;
   }

   void deallocate(void *slab, size_t size)
   {
      free(((void**)slab)[-1]);
   }

   static size_t getLastSlabSize()
   {
      return m_lastSlabSize;
   }
};

size_t MockSlabAllocator::m_lastSlabSize = 0;


} // anonymous namespace


// Allocate a large-ish block with a really large alignment so that the
// allocator will think that it has space, but after it does the alignment it
// will not.
TEST(AllocatorTest, testBigAlignment)
{
   polar::utils::BumpPtrAllocatorImpl<MockSlabAllocator> alloc;

   // First allocate a tiny bit to ensure we have to re-align things.
   (void)alloc.allocate(1, 1);

   // Now the big chunk with a big alignment.
   (void)alloc.allocate(3000, 2048);

   // We test that the last slab size is not the default 4096 byte slab, but
   // rather a custom sized slab that is larger.
   EXPECT_GT(MockSlabAllocator::getLastSlabSize(), 4096u);
}

