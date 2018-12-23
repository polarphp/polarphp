// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/08.

#include "polarphp/basic/adt/IntervalMap.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

typedef IntervalMap<unsigned, unsigned, 4> UUMap;
typedef IntervalMap<unsigned, unsigned, 4,
IntervalMapHalfOpenInfo<unsigned>> UUHalfOpenMap;

// Empty map tests
TEST(IntervalMapTest, testEmptyMap)
{
   UUMap::Allocator allocator;
   UUMap map(allocator);
   EXPECT_TRUE(map.empty());

   // Lookup on empty map.
   EXPECT_EQ(0u, map.lookup(0));
   EXPECT_EQ(7u, map.lookup(0, 7));
   EXPECT_EQ(0u, map.lookup(~0u-1));
   EXPECT_EQ(7u, map.lookup(~0u-1, 7));

   // Iterators.
   EXPECT_TRUE(map.begin() == map.begin());
   EXPECT_TRUE(map.begin() == map.end());
   EXPECT_TRUE(map.end() == map.end());
   EXPECT_FALSE(map.begin() != map.begin());
   EXPECT_FALSE(map.begin() != map.end());
   EXPECT_FALSE(map.end() != map.end());
   EXPECT_FALSE(map.begin().valid());
   EXPECT_FALSE(map.end().valid());
   UUMap::Iterator iter = map.begin();
   EXPECT_FALSE(iter.valid());
   EXPECT_TRUE(iter == map.end());

   // Default constructor and cross-constness compares.
   UUMap::ConstIterator CI;
   CI = map.begin();
   EXPECT_TRUE(CI == iter);
   UUMap::Iterator I2;
   I2 = map.end();
   EXPECT_TRUE(I2 == CI);
}

// Single entry map tests
TEST(IntervalMapTest, testSingleEntryMap)
{
   UUMap::Allocator allocator;
   UUMap map(allocator);
   map.insert(100, 150, 1);
   EXPECT_FALSE(map.empty());

   // Lookup around interval.
   EXPECT_EQ(0u, map.lookup(0));
   EXPECT_EQ(0u, map.lookup(99));
   EXPECT_EQ(1u, map.lookup(100));
   EXPECT_EQ(1u, map.lookup(101));
   EXPECT_EQ(1u, map.lookup(125));
   EXPECT_EQ(1u, map.lookup(149));
   EXPECT_EQ(1u, map.lookup(150));
   EXPECT_EQ(0u, map.lookup(151));
   EXPECT_EQ(0u, map.lookup(200));
   EXPECT_EQ(0u, map.lookup(~0u-1));

   // Iterators.
   EXPECT_TRUE(map.begin() == map.begin());
   EXPECT_FALSE(map.begin() == map.end());
   EXPECT_TRUE(map.end() == map.end());
   EXPECT_TRUE(map.begin().valid());
   EXPECT_FALSE(map.end().valid());

   // Iter deref.
   UUMap::Iterator iter = map.begin();
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(100u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   EXPECT_EQ(1u, iter.value());

   // Preincrement.
   ++iter;
   EXPECT_FALSE(iter.valid());
   EXPECT_FALSE(iter == map.begin());
   EXPECT_TRUE(iter == map.end());

   // PreDecrement.
   --iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(100u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   EXPECT_EQ(1u, iter.value());
   EXPECT_TRUE(iter == map.begin());
   EXPECT_FALSE(iter == map.end());

   // Change the value.
   iter.setValue(2);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(100u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   // Grow the bounds.
   iter.setStart(0);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(0u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   iter.setStop(200);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(0u, iter.start());
   EXPECT_EQ(200u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   // Shrink the bounds.
   iter.setStart(150);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(150u, iter.start());
   EXPECT_EQ(200u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   // Shrink the interval to have a length of 1
   iter.setStop(150);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(150u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   iter.setStop(160);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(150u, iter.start());
   EXPECT_EQ(160u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   // Shrink the interval to have a length of 1
   iter.setStart(160);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(160u, iter.start());
   EXPECT_EQ(160u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   // Erase last elem.
   iter.erase();
   EXPECT_TRUE(map.empty());
   EXPECT_EQ(0, std::distance(map.begin(), map.end()));
}

// Single entry half-open map tests
TEST(IntervalMapTest, testSingleEntryHalfOpenMap)
{
   UUHalfOpenMap::Allocator allocator;
   UUHalfOpenMap map(allocator);
   map.insert(100, 150, 1);
   EXPECT_FALSE(map.empty());

   UUHalfOpenMap::Iterator iter = map.begin();
   ASSERT_TRUE(iter.valid());

   // Shrink the interval to have a length of 1
   iter.setStart(149);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(149u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   EXPECT_EQ(1u, iter.value());

   iter.setStop(160);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(149u, iter.start());
   EXPECT_EQ(160u, iter.stop());
   EXPECT_EQ(1u, iter.value());

   // Shrink the interval to have a length of 1
   iter.setStop(150);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(149u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   EXPECT_EQ(1u, iter.value());
}

// Flat coalescing tests.
TEST(IntervalMapTest, testRootCoalescing)
{
   UUMap::Allocator allocator;
   UUMap map(allocator);
   map.insert(100, 150, 1);

   // Coalesce from the left.
   map.insert(90, 99, 1);
   EXPECT_EQ(1, std::distance(map.begin(), map.end()));
   EXPECT_EQ(90u, map.start());
   EXPECT_EQ(150u, map.stop());

   // Coalesce from the right.
   map.insert(151, 200, 1);
   EXPECT_EQ(1, std::distance(map.begin(), map.end()));
   EXPECT_EQ(90u, map.start());
   EXPECT_EQ(200u, map.stop());

   // Non-coalesce from the left.
   map.insert(60, 89, 2);
   EXPECT_EQ(2, std::distance(map.begin(), map.end()));
   EXPECT_EQ(60u, map.start());
   EXPECT_EQ(200u, map.stop());
   EXPECT_EQ(2u, map.lookup(89));
   EXPECT_EQ(1u, map.lookup(90));

   UUMap::Iterator iter = map.begin();
   EXPECT_EQ(60u, iter.start());
   EXPECT_EQ(89u, iter.stop());
   EXPECT_EQ(2u, iter.value());
   ++iter;
   EXPECT_EQ(90u, iter.start());
   EXPECT_EQ(200u, iter.stop());
   EXPECT_EQ(1u, iter.value());
   ++iter;
   EXPECT_FALSE(iter.valid());

   // Non-coalesce from the right.
   map.insert(201, 210, 2);
   EXPECT_EQ(3, std::distance(map.begin(), map.end()));
   EXPECT_EQ(60u, map.start());
   EXPECT_EQ(210u, map.stop());
   EXPECT_EQ(2u, map.lookup(201));
   EXPECT_EQ(1u, map.lookup(200));

   // Erase from the left.
   map.begin().erase();
   EXPECT_EQ(2, std::distance(map.begin(), map.end()));
   EXPECT_EQ(90u, map.start());
   EXPECT_EQ(210u, map.stop());

   // Erase from the right.
   (--map.end()).erase();
   EXPECT_EQ(1, std::distance(map.begin(), map.end()));
   EXPECT_EQ(90u, map.start());
   EXPECT_EQ(200u, map.stop());

   // Add non-coalescing, then trigger coalescing with setValue.
   map.insert(80, 89, 2);
   map.insert(201, 210, 2);
   EXPECT_EQ(3, std::distance(map.begin(), map.end()));
   (++map.begin()).setValue(2);
   EXPECT_EQ(1, std::distance(map.begin(), map.end()));
   iter = map.begin();
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(80u, iter.start());
   EXPECT_EQ(210u, iter.stop());
   EXPECT_EQ(2u, iter.value());
}

// Flat multi-coalescing tests.
TEST(IntervalMapTest, testRootMultiCoalescing)
{
   UUMap::Allocator allocator;
   UUMap map(allocator);
   map.insert(140, 150, 1);
   map.insert(160, 170, 1);
   map.insert(100, 110, 1);
   map.insert(120, 130, 1);
   EXPECT_EQ(4, std::distance(map.begin(), map.end()));
   EXPECT_EQ(100u, map.start());
   EXPECT_EQ(170u, map.stop());

   // Verify inserts.
   UUMap::Iterator iter = map.begin();
   EXPECT_EQ(100u, iter.start());
   EXPECT_EQ(110u, iter.stop());
   ++iter;
   EXPECT_EQ(120u, iter.start());
   EXPECT_EQ(130u, iter.stop());
   ++iter;
   EXPECT_EQ(140u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   ++iter;
   EXPECT_EQ(160u, iter.start());
   EXPECT_EQ(170u, iter.stop());
   ++iter;
   EXPECT_FALSE(iter.valid());

   // Test advanceTo on flat tree.
   iter = map.begin();
   iter.advanceTo(135);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(140u, iter.start());
   EXPECT_EQ(150u, iter.stop());

   iter.advanceTo(145);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(140u, iter.start());
   EXPECT_EQ(150u, iter.stop());

   iter.advanceTo(200);
   EXPECT_FALSE(iter.valid());

   iter.advanceTo(300);
   EXPECT_FALSE(iter.valid());

   // Coalesce left with followers.
   // [100;110] [120;130] [140;150] [160;170]
   map.insert(111, 115, 1);
   iter = map.begin();
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(100u, iter.start());
   EXPECT_EQ(115u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(120u, iter.start());
   EXPECT_EQ(130u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(140u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(160u, iter.start());
   EXPECT_EQ(170u, iter.stop());
   ++iter;
   EXPECT_FALSE(iter.valid());

   // Coalesce right with followers.
   // [100;115] [120;130] [140;150] [160;170]
   map.insert(135, 139, 1);
   iter = map.begin();
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(100u, iter.start());
   EXPECT_EQ(115u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(120u, iter.start());
   EXPECT_EQ(130u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(135u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(160u, iter.start());
   EXPECT_EQ(170u, iter.stop());
   ++iter;
   EXPECT_FALSE(iter.valid());

   // Coalesce left and right with followers.
   // [100;115] [120;130] [135;150] [160;170]
   map.insert(131, 134, 1);
   iter = map.begin();
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(100u, iter.start());
   EXPECT_EQ(115u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(120u, iter.start());
   EXPECT_EQ(150u, iter.stop());
   ++iter;
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(160u, iter.start());
   EXPECT_EQ(170u, iter.stop());
   ++iter;
   EXPECT_FALSE(iter.valid());

   // Test clear() on non-branched map.
   map.clear();
   EXPECT_TRUE(map.empty());
   EXPECT_TRUE(map.begin() == map.end());
}

// Branched, non-coalescing tests.
TEST(IntervalMapTest, testBranched)
{
   UUMap::Allocator allocator;
   UUMap map(allocator);

   // Insert enough intervals to force a branched tree.
   // This creates 9 leaf nodes with 11 elements each, tree height = 1.
   for (unsigned i = 1; i < 100; ++i) {
      map.insert(10*i, 10*i+5, i);
      EXPECT_EQ(10u, map.start());
      EXPECT_EQ(10*i+5, map.stop());
   }

   // Tree limits.
   EXPECT_FALSE(map.empty());
   EXPECT_EQ(10u, map.start());
   EXPECT_EQ(995u, map.stop());

   // Tree lookup.
   for (unsigned i = 1; i < 100; ++i) {
      EXPECT_EQ(0u, map.lookup(10*i-1));
      EXPECT_EQ(i, map.lookup(10*i));
      EXPECT_EQ(i, map.lookup(10*i+5));
      EXPECT_EQ(0u, map.lookup(10*i+6));
   }

   // Forward iteration.
   UUMap::Iterator iter = map.begin();
   for (unsigned i = 1; i < 100; ++i) {
      ASSERT_TRUE(iter.valid());
      EXPECT_EQ(10*i, iter.start());
      EXPECT_EQ(10*i+5, iter.stop());
      EXPECT_EQ(i, *iter);
      ++iter;
   }
   EXPECT_FALSE(iter.valid());
   EXPECT_TRUE(iter == map.end());

   // Backwards iteration.
   for (unsigned i = 99; i; --i) {
      --iter;
      ASSERT_TRUE(iter.valid());
      EXPECT_EQ(10*i, iter.start());
      EXPECT_EQ(10*i+5, iter.stop());
      EXPECT_EQ(i, *iter);
   }
   EXPECT_TRUE(iter == map.begin());

   // Test advanceTo in same node.
   iter.advanceTo(20);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(20u, iter.start());
   EXPECT_EQ(25u, iter.stop());

   // Change value, no coalescing.
   iter.setValue(0);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(20u, iter.start());
   EXPECT_EQ(25u, iter.stop());
   EXPECT_EQ(0u, iter.value());

   // Close the gap right, no coalescing.
   iter.setStop(29);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(20u, iter.start());
   EXPECT_EQ(29u, iter.stop());
   EXPECT_EQ(0u, iter.value());

   // Change value, no coalescing.
   iter.setValue(2);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(20u, iter.start());
   EXPECT_EQ(29u, iter.stop());
   EXPECT_EQ(2u, iter.value());

   // Change value, now coalescing.
   iter.setValue(3);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(20u, iter.start());
   EXPECT_EQ(35u, iter.stop());
   EXPECT_EQ(3u, iter.value());

   // Close the gap, now coalescing.
   iter.setValue(4);
   ASSERT_TRUE(iter.valid());
   iter.setStop(39);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(20u, iter.start());
   EXPECT_EQ(45u, iter.stop());
   EXPECT_EQ(4u, iter.value());

   // advanceTo another node.
   iter.advanceTo(200);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(200u, iter.start());
   EXPECT_EQ(205u, iter.stop());

   // Close the gap left, no coalescing.
   iter.setStart(196);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(196u, iter.start());
   EXPECT_EQ(205u, iter.stop());
   EXPECT_EQ(20u, iter.value());

   // Change value, no coalescing.
   iter.setValue(0);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(196u, iter.start());
   EXPECT_EQ(205u, iter.stop());
   EXPECT_EQ(0u, iter.value());

   // Change value, now coalescing.
   iter.setValue(19);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(190u, iter.start());
   EXPECT_EQ(205u, iter.stop());
   EXPECT_EQ(19u, iter.value());

   // Close the gap, now coalescing.
   iter.setValue(18);
   ASSERT_TRUE(iter.valid());
   iter.setStart(186);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(180u, iter.start());
   EXPECT_EQ(205u, iter.stop());
   EXPECT_EQ(18u, iter.value());

   // Erase from the front.
   iter = map.begin();
   for (unsigned i = 0; i != 20; ++i) {
      iter.erase();
      EXPECT_TRUE(iter == map.begin());
      EXPECT_FALSE(map.empty());
      EXPECT_EQ(iter.start(), map.start());
      EXPECT_EQ(995u, map.stop());
   }

   // Test clear() on branched map.
   map.clear();
   EXPECT_TRUE(map.empty());
   EXPECT_TRUE(map.begin() == map.end());
}

// Branched, high, non-coalescing tests.
TEST(IntervalMapTest, testBranched2)
{
   UUMap::Allocator allocator;
   UUMap map(allocator);

   // Insert enough intervals to force a height >= 2 tree.
   for (unsigned i = 1; i < 1000; ++i)
      map.insert(10*i, 10*i+5, i);

   // Tree limits.
   EXPECT_FALSE(map.empty());
   EXPECT_EQ(10u, map.start());
   EXPECT_EQ(9995u, map.stop());

   // Tree lookup.
   for (unsigned i = 1; i < 1000; ++i) {
      EXPECT_EQ(0u, map.lookup(10*i-1));
      EXPECT_EQ(i, map.lookup(10*i));
      EXPECT_EQ(i, map.lookup(10*i+5));
      EXPECT_EQ(0u, map.lookup(10*i+6));
   }

   // Forward iteration.
   UUMap::Iterator iter = map.begin();
   for (unsigned i = 1; i < 1000; ++i) {
      ASSERT_TRUE(iter.valid());
      EXPECT_EQ(10*i, iter.start());
      EXPECT_EQ(10*i+5, iter.stop());
      EXPECT_EQ(i, *iter);
      ++iter;
   }
   EXPECT_FALSE(iter.valid());
   EXPECT_TRUE(iter == map.end());

   // Backwards iteration.
   for (unsigned i = 999; i; --i) {
      --iter;
      ASSERT_TRUE(iter.valid());
      EXPECT_EQ(10*i, iter.start());
      EXPECT_EQ(10*i+5, iter.stop());
      EXPECT_EQ(i, *iter);
   }
   EXPECT_TRUE(iter == map.begin());

   // Test advanceTo in same node.
   iter.advanceTo(20);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(20u, iter.start());
   EXPECT_EQ(25u, iter.stop());

   // advanceTo sibling leaf node.
   iter.advanceTo(200);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(200u, iter.start());
   EXPECT_EQ(205u, iter.stop());

   // advanceTo further.
   iter.advanceTo(2000);
   ASSERT_TRUE(iter.valid());
   EXPECT_EQ(2000u, iter.start());
   EXPECT_EQ(2005u, iter.stop());

   // advanceTo beyond end()
   iter.advanceTo(20000);
   EXPECT_FALSE(iter.valid());

   // end().advanceTo() is valid as long as x > map.stop()
   iter.advanceTo(30000);
   EXPECT_FALSE(iter.valid());

   // Test clear() on branched map.
   map.clear();
   EXPECT_TRUE(map.empty());
   EXPECT_TRUE(map.begin() == map.end());
}

// Random insertions, coalescing to a single interval.
TEST(IntervalMapTest, testRandomCoalescing)
{
   UUMap::Allocator allocator;
   UUMap map(allocator);

   // This is a poor PRNG with maximal period:
   // x_n = 5 x_{n-1} + 1 mod 2^N

   unsigned x = 100;
   for (unsigned i = 0; i != 4096; ++i) {
      map.insert(10*x, 10*x+9, 1);
      EXPECT_GE(10*x, map.start());
      EXPECT_LE(10*x+9, map.stop());
      x = (5*x+1)%4096;
   }

   // Map should be fully coalesced after that exercise.
   EXPECT_FALSE(map.empty());
   EXPECT_EQ(0u, map.start());
   EXPECT_EQ(40959u, map.stop());
   EXPECT_EQ(1, std::distance(map.begin(), map.end()));

}

TEST(IntervalMapOverlapsTest, testSmallMaps)
{
   typedef IntervalMapOverlaps<UUMap,UUMap> UUOverlaps;
   UUMap::Allocator allocator;
   UUMap mapA(allocator);
   UUMap mapB(allocator);

   // empty, empty.
   EXPECT_FALSE(UUOverlaps(mapA, mapB).valid());

   mapA.insert(1, 2, 3);

   // full, empty
   EXPECT_FALSE(UUOverlaps(mapA, mapB).valid());
   // empty, full
   EXPECT_FALSE(UUOverlaps(mapB, mapA).valid());

   mapB.insert(3, 4, 5);

   // full, full, non-overlapping
   EXPECT_FALSE(UUOverlaps(mapA, mapB).valid());
   EXPECT_FALSE(UUOverlaps(mapB, mapA).valid());

   // Add an overlapping segment.
   mapA.insert(4, 5, 6);

   UUOverlaps AB(mapA, mapB);
   ASSERT_TRUE(AB.valid());
   EXPECT_EQ(4u, AB.a().start());
   EXPECT_EQ(3u, AB.b().start());
   ++AB;
   EXPECT_FALSE(AB.valid());

   UUOverlaps BA(mapB, mapA);
   ASSERT_TRUE(BA.valid());
   EXPECT_EQ(3u, BA.a().start());
   EXPECT_EQ(4u, BA.b().start());
   // advance past end.
   BA.advanceTo(6);
   EXPECT_FALSE(BA.valid());
   // advance an invalid Iterator.
   BA.advanceTo(7);
   EXPECT_FALSE(BA.valid());
}

TEST(IntervalMapOverlapsTest, testBigMaps)
{
   typedef IntervalMapOverlaps<UUMap,UUMap> UUOverlaps;
   UUMap::Allocator allocator;
   UUMap mapA(allocator);
   UUMap mapB(allocator);

   // [0;4] [10;14] [20;24] ...
   for (unsigned n = 0; n != 100; ++n)
      mapA.insert(10*n, 10*n+4, n);

   // [5;6] [15;16] [25;26] ...
   for (unsigned n = 10; n != 20; ++n)
      mapB.insert(10*n+5, 10*n+6, n);

   // [208;209] [218;219] ...
   for (unsigned n = 20; n != 30; ++n)
      mapB.insert(10*n+8, 10*n+9, n);

   // insert some overlapping segments.
   mapB.insert(400, 400, 400);
   mapB.insert(401, 401, 401);
   mapB.insert(402, 500, 402);
   mapB.insert(600, 601, 402);

   UUOverlaps AB(mapA, mapB);
   ASSERT_TRUE(AB.valid());
   EXPECT_EQ(400u, AB.a().start());
   EXPECT_EQ(400u, AB.b().start());
   ++AB;
   ASSERT_TRUE(AB.valid());
   EXPECT_EQ(400u, AB.a().start());
   EXPECT_EQ(401u, AB.b().start());
   ++AB;
   ASSERT_TRUE(AB.valid());
   EXPECT_EQ(400u, AB.a().start());
   EXPECT_EQ(402u, AB.b().start());
   ++AB;
   ASSERT_TRUE(AB.valid());
   EXPECT_EQ(410u, AB.a().start());
   EXPECT_EQ(402u, AB.b().start());
   ++AB;
   ASSERT_TRUE(AB.valid());
   EXPECT_EQ(420u, AB.a().start());
   EXPECT_EQ(402u, AB.b().start());
   AB.skipB();
   ASSERT_TRUE(AB.valid());
   EXPECT_EQ(600u, AB.a().start());
   EXPECT_EQ(600u, AB.b().start());
   ++AB;
   EXPECT_FALSE(AB.valid());

   // Test advanceTo.
   UUOverlaps AB2(mapA, mapB);
   AB2.advanceTo(410);
   ASSERT_TRUE(AB2.valid());
   EXPECT_EQ(410u, AB2.a().start());
   EXPECT_EQ(402u, AB2.b().start());

   // It is valid to advanceTo with any monotonic sequence.
   AB2.advanceTo(411);
   ASSERT_TRUE(AB2.valid());
   EXPECT_EQ(410u, AB2.a().start());
   EXPECT_EQ(402u, AB2.b().start());

   // Check reversed maps.
   UUOverlaps BA(mapB, mapA);
   ASSERT_TRUE(BA.valid());
   EXPECT_EQ(400u, BA.b().start());
   EXPECT_EQ(400u, BA.a().start());
   ++BA;
   ASSERT_TRUE(BA.valid());
   EXPECT_EQ(400u, BA.b().start());
   EXPECT_EQ(401u, BA.a().start());
   ++BA;
   ASSERT_TRUE(BA.valid());
   EXPECT_EQ(400u, BA.b().start());
   EXPECT_EQ(402u, BA.a().start());
   ++BA;
   ASSERT_TRUE(BA.valid());
   EXPECT_EQ(410u, BA.b().start());
   EXPECT_EQ(402u, BA.a().start());
   ++BA;
   ASSERT_TRUE(BA.valid());
   EXPECT_EQ(420u, BA.b().start());
   EXPECT_EQ(402u, BA.a().start());
   BA.skipA();
   ASSERT_TRUE(BA.valid());
   EXPECT_EQ(600u, BA.b().start());
   EXPECT_EQ(600u, BA.a().start());
   ++BA;
   EXPECT_FALSE(BA.valid());

   // Test advanceTo.
   UUOverlaps BA2(mapB, mapA);
   BA2.advanceTo(410);
   ASSERT_TRUE(BA2.valid());
   EXPECT_EQ(410u, BA2.b().start());
   EXPECT_EQ(402u, BA2.a().start());

   BA2.advanceTo(411);
   ASSERT_TRUE(BA2.valid());
   EXPECT_EQ(410u, BA2.b().start());
   EXPECT_EQ(402u, BA2.a().start());
}

} // anonymous namespace
