// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/06/02.

#include "gtest/gtest.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/StringRef.h"
#include <list>
#include <vector>

namespace {
using polar::basic::rank;
using polar::basic::StringRef;
using polar::basic::is_splat;
using polar::basic::make_range;

int f(rank<0>) { return 0; }
int f(rank<1>) { return 1; }
int f(rank<2>) { return 2; }
int f(rank<4>) { return 4; }

}

TEST(StlExtrasTest, testRank)
{
   // We shouldn't get ambiguities and should select the overload of the same
   // rank as the argument.
   EXPECT_EQ(0, f(rank<0>()));
   EXPECT_EQ(1, f(rank<1>()));
   EXPECT_EQ(2, f(rank<2>()));
   // This overload is missing so we end up back at 2.
   EXPECT_EQ(2, f(rank<3>()));

   // But going past 3 should work fine.
   EXPECT_EQ(4, f(rank<4>()));

   // And we can even go higher and just fall back to the last overload.
   EXPECT_EQ(4, f(rank<5>()));
   EXPECT_EQ(4, f(rank<6>()));
}

TEST(StlExtrasTest, testNumerateLValue)
{
   // Test that a simple LValue can be enumerated and gives correct results with
   // multiple types, including the empty container.
   std::vector<char> foo = {'a', 'b', 'c'};
   typedef std::pair<std::size_t, char> CharPairType;
   std::vector<CharPairType> charResults;
   for (auto iter : polar::basic::enumerate(foo)) {
      charResults.emplace_back(iter.getIndex(), iter.getValue());
   }
   ASSERT_EQ(3u, charResults.size());
   EXPECT_EQ(CharPairType(0u, 'a'), charResults[0]);
   EXPECT_EQ(CharPairType(1u, 'b'), charResults[1]);
   EXPECT_EQ(CharPairType(2u, 'c'), charResults[2]);

   // Test a const range of a different type.
   typedef std::pair<std::size_t, int> IntPairType;
   std::vector<IntPairType> intResults;
   const std::vector<int> bar = {1, 2, 3};
   for (auto iter : polar::basic::enumerate(bar)) {
      intResults.emplace_back(iter.getIndex(), iter.getValue());
   }
   ASSERT_EQ(3u, intResults.size());
   EXPECT_EQ(IntPairType(0u, 1), intResults[0]);
   EXPECT_EQ(IntPairType(1u, 2), intResults[1]);
   EXPECT_EQ(IntPairType(2u, 3), intResults[2]);

   // Test an empty range.
   intResults.clear();
   const std::vector<int> baz{};
   for (auto iter : polar::basic::enumerate(baz)) {
      intResults.emplace_back(iter.getIndex(), iter.getValue());
   }
   EXPECT_TRUE(intResults.empty());
}


TEST(StlExtrasTest, testEnumerateModifyLValue) {
   // Test that you can modify the underlying entries of an lvalue range through
   // the enumeration iterator.
   std::vector<char> foo = {'a', 'b', 'c'};

   for (auto iter : polar::basic::enumerate(foo)) {
      ++iter.getValue();
   }
   EXPECT_EQ('b', foo[0]);
   EXPECT_EQ('c', foo[1]);
   EXPECT_EQ('d', foo[2]);
}

TEST(StlExtrasTest, testEnumerateRValueRef) {
   // Test that an rvalue can be enumerated.
   typedef std::pair<std::size_t, int> PairType;
   std::vector<PairType> results;

   auto enumerator = polar::basic::enumerate(std::vector<int>{1, 2, 3});

   for (auto iter : polar::basic::enumerate(std::vector<int>{1, 2, 3})) {
      results.emplace_back(iter.getIndex(), iter.getValue());
   }

   ASSERT_EQ(3u, results.size());
   EXPECT_EQ(PairType(0u, 1), results[0]);
   EXPECT_EQ(PairType(1u, 2), results[1]);
   EXPECT_EQ(PairType(2u, 3), results[2]);
}

TEST(StlExtrasTest, testEnumerateModifyRValue) {
   // Test that when enumerating an rvalue, modification still works (even if
   // this isn't terribly useful, it at least shows that we haven't snuck an
   // extra const in there somewhere.
   typedef std::pair<std::size_t, char> PairType;
   std::vector<PairType> results;

   for (auto iter : polar::basic::enumerate(std::vector<char>{'1', '2', '3'})) {
      ++iter.getValue();
      results.emplace_back(iter.getIndex(), iter.getValue());
   }

   ASSERT_EQ(3u, results.size());
   EXPECT_EQ(PairType(0u, '2'), results[0]);
   EXPECT_EQ(PairType(1u, '3'), results[1]);
   EXPECT_EQ(PairType(2u, '4'), results[2]);
}

namespace {

template <bool B> struct CanMove
{};

template <> struct CanMove<false>
{
   CanMove(CanMove &&) = delete;

   CanMove() = default;
   CanMove(const CanMove &) = default;
};

template <bool B> struct CanCopy
{};

template <> struct CanCopy<false>
{
   CanCopy(const CanCopy &) = delete;

   CanCopy() = default;
   CanCopy(CanCopy &&) = default;
};

template <bool Moveable, bool Copyable>
struct Range : CanMove<Moveable>, CanCopy<Copyable>
{
   explicit Range(int &C, int &M, int &D) : C(C), M(M), D(D)
   {}
   Range(const Range &R) : CanCopy<Copyable>(R), C(R.C), M(R.M), D(R.D) { ++C; }
   Range(Range &&R) : CanMove<Moveable>(std::move(R)), C(R.C), M(R.M), D(R.D) {
      ++M;
   }
   ~Range() { ++D; }

   int &C;
   int &M;
   int &D;

   int *begin() { return nullptr; }
   int *end() { return nullptr; }
};

} // anonymous namespace

TEST(StlExtrasTest, testEnumerateLifetimeSemantics)
{
   // Test that when enumerating lvalues and rvalues, there are no surprise
   // copies or moves.

   // With an rvalue, it should not be destroyed until the end of the scope.
   int copies = 0;
   int moves = 0;
   int destructors = 0;
   {
      auto E1 = polar::basic::enumerate(Range<true, false>(copies, moves, destructors));
      // Doesn't compile.  rvalue ranges must be moveable.
      // auto E2 = enumerate(Range<false, true>(Copies, Moves, Destructors));
      EXPECT_EQ(0, copies);
      EXPECT_EQ(1, moves);
      EXPECT_EQ(1, destructors);
   }
   EXPECT_EQ(0, copies);
   EXPECT_EQ(1, moves);
   EXPECT_EQ(2, destructors);

   copies = moves = destructors = 0;
   // With an lvalue, it should not be destroyed even after the end of the scope.
   // lvalue ranges need be neither copyable nor moveable.
   Range<false, false> range(copies, moves, destructors);
   {
      auto enumerator = polar::basic::enumerate(range);
      (void)enumerator;
      EXPECT_EQ(0, copies);
      EXPECT_EQ(0, moves);
      EXPECT_EQ(0, destructors);
   }
   EXPECT_EQ(0, copies);
   EXPECT_EQ(0, moves);
   EXPECT_EQ(0, destructors);
}

TEST(StlExtrasTest, testApplyTuple) {
   auto T = std::make_tuple(1, 3, 7);
   auto U = polar::basic::apply_tuple(
            [](int A, int B, int C) { return std::make_tuple(A - B, B - C, C - A); },
   T);

   EXPECT_EQ(-2, std::get<0>(U));
   EXPECT_EQ(-4, std::get<1>(U));
   EXPECT_EQ(6, std::get<2>(U));

   auto V = polar::basic::apply_tuple(
            [](int A, int B, int C) {
      return std::make_tuple(std::make_pair(A, char('A' + A)),
                             std::make_pair(B, char('A' + B)),
                             std::make_pair(C, char('A' + C)));
   },
   T);

   EXPECT_EQ(std::make_pair(1, 'B'), std::get<0>(V));
   EXPECT_EQ(std::make_pair(3, 'D'), std::get<1>(V));
   EXPECT_EQ(std::make_pair(7, 'H'), std::get<2>(V));
}

namespace {

class ApplyVariadic
{
   static int apply_one(int X) { return X + 1; }
   static char apply_one(char C) { return C + 1; }
   static StringRef apply_one(StringRef str) { return str.dropBack(); }

public:
   template <typename... Ts>
   auto operator()(Ts &&... Items)
   -> decltype(std::make_tuple(apply_one(Items)...)) {
      return std::make_tuple(apply_one(Items)...);
   }
};

} // anonymous namespace


TEST(StlExtrasTest, testApplyTupleVariadic)
{
   auto items = std::make_tuple(1, StringRef("Test"), 'X');
   auto values = polar::basic::apply_tuple(ApplyVariadic(), items);

   EXPECT_EQ(2, std::get<0>(values));
   EXPECT_EQ("Tes", std::get<1>(values));
   EXPECT_EQ('Y', std::get<2>(values));
}

TEST(StlExtrasTest, testCountAdaptor)
{
   std::vector<int> v;

   v.push_back(1);
   v.push_back(2);
   v.push_back(1);
   v.push_back(4);
   v.push_back(3);
   v.push_back(2);
   v.push_back(1);

   EXPECT_EQ(3, polar::basic::count(v, 1));
   EXPECT_EQ(2, polar::basic::count(v, 2));
   EXPECT_EQ(1, polar::basic::count(v, 3));
   EXPECT_EQ(1, polar::basic::count(v, 4));
}

TEST(StlExtrasTest, testForeach)
{
   std::vector<int> v{0, 1, 2, 3, 4};
   int count = 0;

   polar::basic::for_each(v, [&count](int) { ++count; });
   EXPECT_EQ(5, count);
}

TEST(StlExtrasTest, testToVector)
{
   std::vector<char> v = {'a', 'b', 'c'};
   auto enumerated = polar::basic::to_vector<4>(polar::basic::enumerate(v));
   ASSERT_EQ(3u, enumerated.getSize());
   for (size_t index = 0; index < v.size(); ++index) {
      EXPECT_EQ(index, enumerated[index].getIndex());
      EXPECT_EQ(v[index], enumerated[index].getValue());
   }
}

TEST(StlExtrasTest, testConcatRange)
{
   std::vector<int> expected = {1, 2, 3, 4, 5, 6, 7, 8};
   std::vector<int> test;

   std::vector<int> v1234 = {1, 2, 3, 4};
   std::list<int> l56 = {5, 6};
   polar::basic::SmallVector<int, 2> sv78 = {7, 8};

   // Use concat across different sized ranges of different types with different
   // iterators.
   for (int &i : polar::basic::concat<int>(v1234, l56, sv78)) {
      test.push_back(i);
   }

   EXPECT_EQ(expected, test);

   // Use concat between a temporary, an L-value, and an R-value to make sure
   // complex lifetimes work well.
   test.clear();
   for (int &i : polar::basic::concat<int>(std::vector<int>(v1234), l56, std::move(sv78))) {
      test.push_back(i);
   }

   EXPECT_EQ(expected, test);
}

TEST(StlExtrasTest, testPartitionAdaptor) {
   std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

   auto iter = polar::basic::partition(v, [](int i) { return i % 2 == 0; });
   ASSERT_EQ(v.begin() + 4, iter);

   // Sort the two halves as partition may have messed with the order.
   polar::basic::sort(v.begin(), iter);
   polar::basic::sort(iter, v.end());

   EXPECT_EQ(2, v[0]);
   EXPECT_EQ(4, v[1]);
   EXPECT_EQ(6, v[2]);
   EXPECT_EQ(8, v[3]);
   EXPECT_EQ(1, v[4]);
   EXPECT_EQ(3, v[5]);
   EXPECT_EQ(5, v[6]);
   EXPECT_EQ(7, v[7]);
}

TEST(StlExtrasTest, testEraseIf) {
   std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

   polar::basic::erase_if(v, [](int i) { return i % 2 == 0; });
   EXPECT_EQ(4u, v.size());
   EXPECT_EQ(1, v[0]);
   EXPECT_EQ(3, v[1]);
   EXPECT_EQ(5, v[2]);
   EXPECT_EQ(7, v[3]);
}

namespace some_namespace
{
struct SomeStruct {
   std::vector<int> data;
   std::string swap_val;
};

std::vector<int>::const_iterator begin(const SomeStruct &s)
{
   return s.data.begin();
}

std::vector<int>::const_iterator end(const SomeStruct &s)
{
   return s.data.end();
}

void swap(SomeStruct &lhs, SomeStruct &rhs)
{
   // make swap visible as non-adl swap would even seem to
   // work with std::swap which defaults to moving
   lhs.swap_val = "lhs";
   rhs.swap_val = "rhs";
}
} // namespace some_namespace


TEST(StlExtrasTest, testADL)
{
   some_namespace::SomeStruct s{{1, 2, 3, 4, 5}, ""};
   some_namespace::SomeStruct s2{{2, 4, 6, 8, 10}, ""};

   EXPECT_EQ(*polar::basic::adl_begin(s), 1);
   EXPECT_EQ(*(polar::basic::adl_end(s) - 1), 5);

   polar::basic::adl_swap(s, s2);
   EXPECT_EQ(s.swap_val, "lhs");
   EXPECT_EQ(s2.swap_val, "rhs");

   int count = 0;
   polar::basic::for_each(s, [&count](int) { ++count; });
   EXPECT_EQ(5, count);
}

TEST(StlExtrasTest, testEmpty)
{
   std::vector<void*> V;
   EXPECT_TRUE(polar::basic::empty(V));
   V.push_back(nullptr);
   EXPECT_FALSE(polar::basic::empty(V));

   std::initializer_list<int> E = {};
   std::initializer_list<int> NotE = {7, 13, 42};
   EXPECT_TRUE(polar::basic::empty(E));
   EXPECT_FALSE(polar::basic::empty(NotE));

   auto R0 = make_range(V.begin(), V.begin());
   EXPECT_TRUE(polar::basic::empty(R0));
   auto R1 = make_range(V.begin(), V.end());
   EXPECT_FALSE(polar::basic::empty(R1));
}

TEST(StlExtrasTest, testEarlyIncrement)
{
   std::list<int> L = {1, 2, 3, 4};

   auto EIR = polar::basic::make_early_inc_range(L);

   auto I = EIR.begin();
   auto EI = EIR.end();
   EXPECT_NE(I, EI);

   EXPECT_EQ(1, *I);
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
#ifndef NDEBUG
   // Repeated dereferences are not allowed.
   EXPECT_DEATH(*I, "Cannot dereference");
   // Comparison after dereference is not allowed.
   EXPECT_DEATH((void)(I == EI), "Cannot compare");
   EXPECT_DEATH((void)(I != EI), "Cannot compare");
#endif
#endif

   ++I;
   EXPECT_NE(I, EI);
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
#ifndef NDEBUG
   // You cannot increment prior to dereference.
   EXPECT_DEATH(++I, "Cannot increment");
#endif
#endif
   EXPECT_EQ(2, *I);
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
#ifndef NDEBUG
   // Repeated dereferences are not allowed.
   EXPECT_DEATH(*I, "Cannot dereference");
#endif
#endif

   // Inserting shouldn't break anything. We should be able to keep dereferencing
   // the currrent iterator and increment. The increment to go to the "next"
   // iterator from before we inserted.
   L.insert(std::next(L.begin(), 2), -1);
   ++I;
   EXPECT_EQ(3, *I);

   // Erasing the front including the current doesn't break incrementing.
   L.erase(L.begin(), std::prev(L.end()));
   ++I;
   EXPECT_EQ(4, *I);
   ++I;
   EXPECT_EQ(EIR.end(), I);
}

TEST(StlExtrasTest, testSplat)
{
   std::vector<int> V;
   EXPECT_FALSE(is_splat(V));

   V.push_back(1);
   EXPECT_TRUE(is_splat(V));

   V.push_back(1);
   V.push_back(1);
   EXPECT_TRUE(is_splat(V));

   V.push_back(2);
   EXPECT_FALSE(is_splat(V));
}

