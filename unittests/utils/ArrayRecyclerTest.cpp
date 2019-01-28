// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/12.

#include "polarphp/utils/ArrayRecycler.h"
#include "polarphp/utils/Allocator.h"
#include "gtest/gtest.h"
#include <cstdlib>

using namespace polar::utils;

namespace {

struct Object {
   int Num;
   Object *Other;
};
typedef ArrayRecycler<Object> ARO;

TEST(ArrayRecyclerTest, testCapacity)
{
   // Capacity size should never be 0.
   ARO::Capacity cap = ARO::Capacity::get(0);
   EXPECT_LT(0u, cap.getSize());

   size_t prevSize = cap.getSize();
   for (unsigned N = 1; N != 100; ++N) {
      cap = ARO::Capacity::get(N);
      EXPECT_LE(N, cap.getSize());
      if (prevSize >= N)
         EXPECT_EQ(prevSize, cap.getSize());
      else
         EXPECT_LT(prevSize, cap.getSize());
      prevSize = cap.getSize();
   }

   // Check that the buckets are monotonically increasing.
   cap = ARO::Capacity::get(0);
   prevSize = cap.getSize();
   for (unsigned N = 0; N != 20; ++N) {
      cap = cap.getNext();
      EXPECT_LT(prevSize, cap.getSize());
      prevSize = cap.getSize();
   }
}

TEST(ArrayRecyclerTest, testBasics)
{
   BumpPtrAllocator allocator;
   ArrayRecycler<Object> DUT;

   ARO::Capacity cap = ARO::Capacity::get(8);
   Object *A1 = DUT.allocate(cap, allocator);
   A1[0].Num = 21;
   A1[7].Num = 17;

   Object *A2 = DUT.allocate(cap, allocator);
   A2[0].Num = 121;
   A2[7].Num = 117;

   Object *A3 = DUT.allocate(cap, allocator);
   A3[0].Num = 221;
   A3[7].Num = 217;

   EXPECT_EQ(21, A1[0].Num);
   EXPECT_EQ(17, A1[7].Num);
   EXPECT_EQ(121, A2[0].Num);
   EXPECT_EQ(117, A2[7].Num);
   EXPECT_EQ(221, A3[0].Num);
   EXPECT_EQ(217, A3[7].Num);

   DUT.deallocate(cap, A2);

   // Check that deallocation didn't clobber anything.
   EXPECT_EQ(21, A1[0].Num);
   EXPECT_EQ(17, A1[7].Num);
   EXPECT_EQ(221, A3[0].Num);
   EXPECT_EQ(217, A3[7].Num);

   // Verify recycling.
   Object *A2x = DUT.allocate(cap, allocator);
   EXPECT_EQ(A2, A2x);

   DUT.deallocate(cap, A2x);
   DUT.deallocate(cap, A1);
   DUT.deallocate(cap, A3);

   // Objects are not required to be recycled in reverse deallocation order, but
   // that is what the current implementation does.
   Object *A3x = DUT.allocate(cap, allocator);
   EXPECT_EQ(A3, A3x);
   Object *A1x = DUT.allocate(cap, allocator);
   EXPECT_EQ(A1, A1x);
   Object *A2y = DUT.allocate(cap, allocator);
   EXPECT_EQ(A2, A2y);

   // Back to allocation from the BumpPtrAllocator.
   Object *A4 = DUT.allocate(cap, allocator);
   EXPECT_NE(A1, A4);
   EXPECT_NE(A2, A4);
   EXPECT_NE(A3, A4);

   DUT.clear(allocator);
}

} // anonymous namespace
