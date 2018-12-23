// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/08.

#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/IntrusiveList.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

template <int> struct Shadow;

struct WeirdIter : std::iterator<std::input_iterator_tag, Shadow<0>, Shadow<1>,
      Shadow<2>, Shadow<3>> {};

struct AdaptedIter : IteratorAdaptorBase<AdaptedIter, WeirdIter>
{};

// Test that iterator_adaptor_base forwards typedefs, if value_type is
// unchanged.
static_assert(std::is_same<typename AdaptedIter::value_type, Shadow<0>>::value,
              "");
static_assert(
      std::is_same<typename AdaptedIter::difference_type, Shadow<1>>::value, "");
static_assert(std::is_same<typename AdaptedIter::pointer, Shadow<2>>::value,
              "");
static_assert(std::is_same<typename AdaptedIter::reference, Shadow<3>>::value,
              "");

// Ensure that pointe{e,r}_iterator adaptors correctly forward the category of
// the underlying iterator.

using RandomAccessIter = SmallVectorImpl<int*>::iterator;
using BidiIter = IntrusiveList<int*>::iterator;

template<class T>
using pointee_iterator_defaulted = PointeeIterator<T>;
template<class T>
using pointer_iterator_defaulted = PointeeIterator<T>;

// Ensures that an iterator and its adaptation have the same iterator_category.
template<template<typename> class A, typename It>
using IsAdaptedIterCategorySame =
std::is_same<typename std::iterator_traits<It>::iterator_category,
typename std::iterator_traits<A<It>>::iterator_category>;

// pointeE_iterator
static_assert(IsAdaptedIterCategorySame<pointee_iterator_defaulted,
              RandomAccessIter>::value, "");
static_assert(IsAdaptedIterCategorySame<pointee_iterator_defaulted,
              BidiIter>::value, "");
// pointeR_iterator
static_assert(IsAdaptedIterCategorySame<pointer_iterator_defaulted,
              RandomAccessIter>::value, "");
static_assert(IsAdaptedIterCategorySame<pointer_iterator_defaulted,
              BidiIter>::value, "");

TEST(IteratorTest, testBasic)
{
   int arr[4] = {1, 2, 3, 4};
   SmallVector<int *, 4> V;
   V.push_back(&arr[0]);
   V.push_back(&arr[1]);
   V.push_back(&arr[2]);
   V.push_back(&arr[3]);

   typedef PointeeIterator<SmallVectorImpl<int *>::const_iterator>
         test_iterator;

   test_iterator begin, end;
   begin = V.begin();
   end = test_iterator(V.end());

   test_iterator I = begin;
   for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(*V[i], *I);

      EXPECT_EQ(I, begin + i);
      EXPECT_EQ(I, std::next(begin, i));
      test_iterator J = begin;
      J += i;
      EXPECT_EQ(I, J);
      EXPECT_EQ(*V[i], begin[i]);

      EXPECT_NE(I, end);
      EXPECT_GT(end, I);
      EXPECT_LT(I, end);
      EXPECT_GE(I, begin);
      EXPECT_LE(begin, I);

      EXPECT_EQ(i, I - begin);
      EXPECT_EQ(i, std::distance(begin, I));
      EXPECT_EQ(begin, I - i);

      test_iterator K = I++;
      EXPECT_EQ(K, std::prev(I));
   }
   EXPECT_EQ(end, I);
}

TEST(IteratorTest, testSmartPointer)
{
   SmallVector<std::unique_ptr<int>, 4> V;
   V.push_back(make_unique<int>(1));
   V.push_back(make_unique<int>(2));
   V.push_back(make_unique<int>(3));
   V.push_back(make_unique<int>(4));

   typedef PointeeIterator<
         SmallVectorImpl<std::unique_ptr<int>>::const_iterator>
         test_iterator;

   test_iterator begin, end;
   begin = V.begin();
   end = test_iterator(V.end());

   test_iterator I = begin;
   for (int i = 0; i < 4; ++i) {
      EXPECT_EQ(*V[i], *I);

      EXPECT_EQ(I, begin + i);
      EXPECT_EQ(I, std::next(begin, i));
      test_iterator J = begin;
      J += i;
      EXPECT_EQ(I, J);
      EXPECT_EQ(*V[i], begin[i]);

      EXPECT_NE(I, end);
      EXPECT_GT(end, I);
      EXPECT_LT(I, end);
      EXPECT_GE(I, begin);
      EXPECT_LE(begin, I);

      EXPECT_EQ(i, I - begin);
      EXPECT_EQ(i, std::distance(begin, I));
      EXPECT_EQ(begin, I - i);

      test_iterator K = I++;
      EXPECT_EQ(K, std::prev(I));
   }
   EXPECT_EQ(end, I);
}

TEST(IteratorTest, testRange)
{
   int A[] = {1, 2, 3, 4};
   SmallVector<int *, 4> V{&A[0], &A[1], &A[2], &A[3]};

   int I = 0;
   for (int II : make_pointee_range(V))
      EXPECT_EQ(A[I++], II);
}

TEST(IteratorTest, testLambda)
{
   auto IsOdd = [](int N) { return N % 2 == 1; };
   int A[] = {0, 1, 2, 3, 4, 5, 6};
   auto Range = make_filter_range(A, IsOdd);
   SmallVector<int, 3> Actual(Range.begin(), Range.end());
   EXPECT_EQ((SmallVector<int, 3>{1, 3, 5}), Actual);
}

TEST(IteratorTest, testCallableObject) {
   int Counter = 0;
   struct Callable {
      int &Counter;

      Callable(int &Counter) : Counter(Counter) {}

      bool operator()(int N) {
         Counter++;
         return N % 2 == 1;
      }
   };
   Callable IsOdd(Counter);
   int A[] = {0, 1, 2, 3, 4, 5, 6};
   auto Range = make_filter_range(A, IsOdd);
   EXPECT_EQ(2, Counter);
   SmallVector<int, 3> Actual(Range.begin(), Range.end());
   EXPECT_GE(Counter, 7);
   EXPECT_EQ((SmallVector<int, 3>{1, 3, 5}), Actual);
}

TEST(IteratorTest, testFunctionPointer)
{
   bool (*IsOdd)(int) = [](int N) { return N % 2 == 1; };
   int A[] = {0, 1, 2, 3, 4, 5, 6};
   auto Range = make_filter_range(A, IsOdd);
   SmallVector<int, 3> Actual(Range.begin(), Range.end());
   EXPECT_EQ((SmallVector<int, 3>{1, 3, 5}), Actual);
}

TEST(IteratorTest, testComposition)
{
   auto IsOdd = [](int N) { return N % 2 == 1; };
   std::unique_ptr<int> A[] = {make_unique<int>(0), make_unique<int>(1),
                               make_unique<int>(2), make_unique<int>(3),
                               make_unique<int>(4), make_unique<int>(5),
                               make_unique<int>(6)};
   using PointeeIterator = PointeeIterator<std::unique_ptr<int> *>;
   auto Range = make_filter_range(
            make_range(PointeeIterator(std::begin(A)), PointeeIterator(std::end(A))),
            IsOdd);
   SmallVector<int, 3> Actual(Range.begin(), Range.end());
   EXPECT_EQ((SmallVector<int, 3>{1, 3, 5}), Actual);
}

TEST(IteratorTest, testInputIterator)
{
   struct InputIterator
         : IteratorAdaptorBase<InputIterator, int *, std::input_iterator_tag> {
      using BaseT =
      IteratorAdaptorBase<InputIterator, int *, std::input_iterator_tag>;

      InputIterator(int *It) : BaseT(It) {}
   };

   auto IsOdd = [](int N) { return N % 2 == 1; };
   int A[] = {0, 1, 2, 3, 4, 5, 6};
   auto Range = make_filter_range(
            make_range(InputIterator(std::begin(A)), InputIterator(std::end(A))),
            IsOdd);
   SmallVector<int, 3> Actual(Range.begin(), Range.end());
   EXPECT_EQ((SmallVector<int, 3>{1, 3, 5}), Actual);
}

TEST(FilterIteratorTest, testReverseFilterRange)
{
   auto IsOdd = [](int N) { return N % 2 == 1; };
   int A[] = {0, 1, 2, 3, 4, 5, 6};

   // Check basic reversal.
   auto Range = reverse(make_filter_range(A, IsOdd));
   SmallVector<int, 3> Actual(Range.begin(), Range.end());
   EXPECT_EQ((SmallVector<int, 3>{5, 3, 1}), Actual);

   // Check that the reverse of the reverse is the original.
   auto Range2 = reverse(reverse(make_filter_range(A, IsOdd)));
   SmallVector<int, 3> Actual2(Range2.begin(), Range2.end());
   EXPECT_EQ((SmallVector<int, 3>{1, 3, 5}), Actual2);

   // Check empty ranges.
   auto Range3 = reverse(make_filter_range(ArrayRef<int>(), IsOdd));
   SmallVector<int, 0> Actual3(Range3.begin(), Range3.end());
   EXPECT_EQ((SmallVector<int, 0>{}), Actual3);

   // Check that we don't skip the first element, provided it isn't filtered
   // away.
   auto IsEven = [](int N) { return N % 2 == 0; };
   auto Range4 = reverse(make_filter_range(A, IsEven));
   SmallVector<int, 4> Actual4(Range4.begin(), Range4.end());
   EXPECT_EQ((SmallVector<int, 4>{6, 4, 2, 0}), Actual4);
}

TEST(IteratorTest, testPointerIteratorBasic)
{
   int A[] = {1, 2, 3, 4};
   PointerIterator<int *> begin(std::begin(A)), end(std::end(A));
   EXPECT_EQ(A, *begin);
   ++begin;
   EXPECT_EQ(A + 1, *begin);
   ++begin;
   EXPECT_EQ(A + 2, *begin);
   ++begin;
   EXPECT_EQ(A + 3, *begin);
   ++begin;
   EXPECT_EQ(begin, end);
}

TEST(IteratorTest, testPointerIteratorConst)
{
   int A[] = {1, 2, 3, 4};
   const PointerIterator<int *> begin(std::begin(A));
   EXPECT_EQ(A, *begin);
   EXPECT_EQ(A + 1, std::next(*begin, 1));
   EXPECT_EQ(A + 2, std::next(*begin, 2));
   EXPECT_EQ(A + 3, std::next(*begin, 3));
   EXPECT_EQ(A + 4, std::next(*begin, 4));
}

TEST(IteratorTest, testPointerIteratorRange)
{
   int A[] = {1, 2, 3, 4};
   int I = 0;
   for (int *P : make_pointer_range(A)) {
      EXPECT_EQ(A + I++, P);
   }
}

TEST(IteratorTest, testZipBasic)
{
   using namespace std;
   const SmallVector<unsigned, 6> pi{3, 1, 4, 1, 5, 9};
   SmallVector<bool, 6> odd{1, 1, 0, 1, 1, 1};
   const char message[] = "yynyyy\0";

   for (auto tup : zip(pi, odd, message)) {
      EXPECT_EQ(get<0>(tup) & 0x01, get<1>(tup));
      EXPECT_EQ(get<0>(tup) & 0x01 ? 'y' : 'n', get<2>(tup));
   }

   // note the rvalue
   for (auto tup : zip(pi, SmallVector<bool, 0>{1, 1, 0, 1, 1})) {
      EXPECT_EQ(get<0>(tup) & 0x01, get<1>(tup));
   }
}

TEST(IteratorTest, testZipFirstBasic)
{
   using namespace std;
   const SmallVector<unsigned, 6> pi{3, 1, 4, 1, 5, 9};
   unsigned iters = 0;

   for (auto tup : zip_first(SmallVector<bool, 0>{1, 1, 0, 1}, pi)) {
      EXPECT_EQ(get<0>(tup), get<1>(tup) & 0x01);
      iters += 1;
   }

   EXPECT_EQ(iters, 4u);
}

TEST(IteratorTest, testMutability)
{
   using namespace std;
   const SmallVector<unsigned, 4> pi{3, 1, 4, 1, 5, 9};
   char message[] = "hello zip\0";

   for (auto tup : zip(pi, message, message)) {
      EXPECT_EQ(get<1>(tup), get<2>(tup));
      get<2>(tup) = get<0>(tup) & 0x01 ? 'y' : 'n';
   }

   // note the rvalue
   for (auto tup : zip(message, "yynyyyzip\0")) {
      EXPECT_EQ(get<0>(tup), get<1>(tup));
   }
}

TEST(IteratorTest, testZipFirstMutability) {
   using namespace std;
   vector<unsigned> pi{3, 1, 4, 1, 5, 9};
   unsigned iters = 0;

   for (auto tup : zip_first(SmallVector<bool, 0>{1, 1, 0, 1}, pi)) {
      get<1>(tup) = get<0>(tup);
      iters += 1;
   }

   EXPECT_EQ(iters, 4u);

   for (auto tup : zip_first(SmallVector<bool, 0>{1, 1, 0, 1}, pi)) {
      EXPECT_EQ(get<0>(tup), get<1>(tup));
   }
}

TEST(IteratorTest, testFilter)
{
   using namespace std;
   vector<unsigned> pi{3, 1, 4, 1, 5, 9};

   unsigned iters = 0;
   // pi is length 6, but the zip RHS is length 7.
   auto zipped = zip_first(pi, vector<bool>{1, 1, 0, 1, 1, 1, 0});
   for (auto tup : make_filter_range(
           zipped, [](decltype(zipped)::value_type t) { return get<1>(t); })) {
      EXPECT_EQ(get<0>(tup) & 0x01, get<1>(tup));
      get<0>(tup) += 1;
      iters += 1;
   }

   // Should have skipped pi[2].
   EXPECT_EQ(iters, 5u);

   // Ensure that in-place mutation works.
   EXPECT_TRUE(all_of(pi, [](unsigned n) { return (n & 0x01) == 0; }));
}

TEST(IteratorTest, testReverse)
{
   using namespace std;
   vector<unsigned> ascending{0, 1, 2, 3, 4, 5};

   auto zipped = zip_first(ascending, vector<bool>{0, 1, 0, 1, 0, 1});
   unsigned last = 6;
   for (auto tup : reverse(zipped)) {
      // Check that this is in reverse.
      EXPECT_LT(get<0>(tup), last);
      last = get<0>(tup);
      EXPECT_EQ(get<0>(tup) & 0x01, get<1>(tup));
   }

   auto odds = [](decltype(zipped)::value_type tup) { return get<1>(tup); };
   last = 6;
   for (auto tup : make_filter_range(reverse(zipped), odds)) {
      EXPECT_LT(get<0>(tup), last);
      last = get<0>(tup);
      EXPECT_TRUE(get<0>(tup) & 0x01);
      get<0>(tup) += 1;
   }

   // Ensure that in-place mutation works.
   EXPECT_TRUE(all_of(ascending, [](unsigned n) { return (n & 0x01) == 0; }));
}

} // anonymous namespace
