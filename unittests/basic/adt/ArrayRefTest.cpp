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

#include "polarphp/basic/adt/ArrayRef.h"
#include "gtest/gtest.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/RawOutStream.h"
#include <limits>
#include <vector>

using polar::basic::ArrayRef;
using polar::utils::BumpPtrAllocator;
using polar::basic::make_array_ref;

// Check that the ArrayRef-of-pointer converting constructor only allows adding
// cv qualifiers (not removing them, or otherwise changing the type)
static_assert(
      std::is_convertible<ArrayRef<int *>, ArrayRef<const int *>>::value,
      "Adding const");
static_assert(
      std::is_convertible<ArrayRef<int *>, ArrayRef<volatile int *>>::value,
      "Adding volatile");
static_assert(!std::is_convertible<ArrayRef<int *>, ArrayRef<float *>>::value,
              "Changing pointer of one type to a pointer of another");
static_assert(
      !std::is_convertible<ArrayRef<const int *>, ArrayRef<int *>>::value,
      "Removing const");
static_assert(
      !std::is_convertible<ArrayRef<volatile int *>, ArrayRef<int *>>::value,
      "Removing volatile");

// Check that we can't accidentally assign a temporary location to an ArrayRef.
// (Unfortunately we can't make use of the same thing with constructors.)
//
// Disable this check under MSVC; even MSVC 2015 isn't inconsistent between
// std::is_assignable and actually writing such an assignment.
#if !defined(_MSC_VER)
static_assert(
      !std::is_assignable<ArrayRef<int *>, int *>::value,
      "Assigning from single prvalue element");
static_assert(
      !std::is_assignable<ArrayRef<int *>, int * &&>::value,
      "Assigning from single xvalue element");
static_assert(
      std::is_assignable<ArrayRef<int *>, int * &>::value,
      "Assigning from single lvalue element");
static_assert(
      !std::is_assignable<ArrayRef<int *>, std::initializer_list<int *>>::value,
      "Assigning from an initializer list");
#endif

TEST(ArrayRefTest, testAllocatorCopy)
{
   BumpPtrAllocator alloc;
   static const char16_t words1[] = { 1, 4, 200, 37 };
   ArrayRef<char16_t> array1 = make_array_ref(words1, 4);
   static const char16_t words2[] = { 11, 4003, 67, 64000, 13 };
   ArrayRef<char16_t> array2 = make_array_ref(words2, 5);
   ArrayRef<char16_t> array1c = array1.copy(alloc);
   ArrayRef<char16_t> array2c = array2.copy(alloc);
   EXPECT_TRUE(array1.equals(array1c));
   EXPECT_NE(array1.getData(), array1c.getData());
   EXPECT_TRUE(array2.equals(array2c));
   EXPECT_NE(array2.getData(), array2c.getData());

   // Check that copy can cope with uninitialized memory.
   struct NonAssignable {
      const char *m_ptr;

      NonAssignable(const char *ptr) : m_ptr(ptr) {}
      NonAssignable(const NonAssignable &rhs) = default;
      void operator=(const NonAssignable &rhs) { assert(rhs.m_ptr != nullptr); }
      bool operator==(const NonAssignable &rhs) const { return m_ptr == rhs.m_ptr; }
   } array3Src[] = {"hello", "world"};
   ArrayRef<NonAssignable> array3Copy = make_array_ref(array3Src).copy(alloc);
   EXPECT_EQ(make_array_ref(array3Src), array3Copy);
   EXPECT_NE(make_array_ref(array3Src).getData(), array3Copy.getData());
}

TEST(ArrayRefTest, testSizeTSizedOperations)
{
   ArrayRef<char> arrayRef(nullptr, std::numeric_limits<ptrdiff_t>::max());

   // Check that dropBack accepts size_t-sized numbers.
   EXPECT_EQ(1U, arrayRef.dropBack(arrayRef.getSize() - 1).getSize());

   // Check that dropFront accepts size_t-sized numbers.
   EXPECT_EQ(1U, arrayRef.dropFront(arrayRef.getSize() - 1).getSize());

   // Check that slice accepts size_t-sized numbers.
   EXPECT_EQ(1U, arrayRef.slice(arrayRef.getSize() - 1).getSize());
   EXPECT_EQ(arrayRef.getSize() - 1, arrayRef.slice(1, arrayRef.getSize() - 1).getSize());
}

TEST(ArrayRefTest, testDropBack)
{
   static const int theNumbers[] = {4, 8, 15, 16, 23, 42};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> arrayRef2(theNumbers, arrayRef1.getSize() - 1);
   EXPECT_TRUE(arrayRef1.dropBack().equals(arrayRef2));
}

TEST(ArrayRefTest, testDropFront)
{
   static const int theNumbers[] = {4, 8, 15, 16, 23, 42};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> arrayRef2(&theNumbers[2], arrayRef1.getSize() - 2);
   EXPECT_TRUE(arrayRef1.dropFront(2).equals(arrayRef2));
}

TEST(ArrayRefTest, testDropWhile)
{
   static const int theNumbers[] = {1, 3, 5, 8, 10, 11};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> Expected = arrayRef1.dropFront(3);
   EXPECT_EQ(Expected, arrayRef1.dropWhile([](const int &N) { return N % 2 == 1; }));

   EXPECT_EQ(arrayRef1, arrayRef1.dropWhile([](const int &N) { return N < 0; }));
   EXPECT_EQ(ArrayRef<int>(),
             arrayRef1.dropWhile([](const int &N) { return N > 0; }));
}

TEST(ArrayRefTest, testDropUntil)
{
   static const int theNumbers[] = {1, 3, 5, 8, 10, 11};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> Expected = arrayRef1.dropFront(3);
   EXPECT_EQ(Expected, arrayRef1.dropUntil([](const int &N) { return N % 2 == 0; }));

   EXPECT_EQ(ArrayRef<int>(),
             arrayRef1.dropUntil([](const int &N) { return N < 0; }));
   EXPECT_EQ(arrayRef1, arrayRef1.dropUntil([](const int &N) { return N > 0; }));
}

TEST(ArrayRefTest, testTakeBack)
{
   static const int theNumbers[] = {4, 8, 15, 16, 23, 42};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> arrayRef2(arrayRef1.end() - 1, 1);
   EXPECT_TRUE(arrayRef1.takeBack().equals(arrayRef2));
}

TEST(ArrayRefTest, testTakeFront)
{
   static const int theNumbers[] = {4, 8, 15, 16, 23, 42};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> arrayRef2(arrayRef1.getData(), 2);
   EXPECT_TRUE(arrayRef1.takeFront(2).equals(arrayRef2));
}

TEST(ArrayRefTest, testTakeWhile)
{
   static const int theNumbers[] = {1, 3, 5, 8, 10, 11};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> Expected = arrayRef1.takeFront(3);
   EXPECT_EQ(Expected, arrayRef1.takeWhile([](const int &N) { return N % 2 == 1; }));

   EXPECT_EQ(ArrayRef<int>(),
             arrayRef1.takeWhile([](const int &N) { return N < 0; }));
   EXPECT_EQ(arrayRef1, arrayRef1.takeWhile([](const int &N) { return N > 0; }));
}

TEST(ArrayRefTest, testTakeUntil)
{
   static const int theNumbers[] = {1, 3, 5, 8, 10, 11};
   ArrayRef<int> arrayRef1(theNumbers);
   ArrayRef<int> Expected = arrayRef1.takeFront(3);
   EXPECT_EQ(Expected, arrayRef1.takeUntil([](const int &N) { return N % 2 == 0; }));

   EXPECT_EQ(arrayRef1, arrayRef1.takeUntil([](const int &N) { return N < 0; }));
   EXPECT_EQ(ArrayRef<int>(),
             arrayRef1.takeUntil([](const int &N) { return N > 0; }));
}

TEST(ArrayRefTest, testEquals)
{
   static const int A1[] = {1, 2, 3, 4, 5, 6, 7, 8};
   ArrayRef<int> arrayRef1(A1);
   EXPECT_TRUE(arrayRef1.equals({1, 2, 3, 4, 5, 6, 7, 8}));
   EXPECT_FALSE(arrayRef1.equals({8, 1, 2, 4, 5, 6, 6, 7}));
   EXPECT_FALSE(arrayRef1.equals({2, 4, 5, 6, 6, 7, 8, 1}));
   EXPECT_FALSE(arrayRef1.equals({0, 1, 2, 4, 5, 6, 6, 7}));
   EXPECT_FALSE(arrayRef1.equals({1, 2, 42, 4, 5, 6, 7, 8}));
   EXPECT_FALSE(arrayRef1.equals({42, 2, 3, 4, 5, 6, 7, 8}));
   EXPECT_FALSE(arrayRef1.equals({1, 2, 3, 4, 5, 6, 7, 42}));
   EXPECT_FALSE(arrayRef1.equals({1, 2, 3, 4, 5, 6, 7}));
   EXPECT_FALSE(arrayRef1.equals({1, 2, 3, 4, 5, 6, 7, 8, 9}));

   ArrayRef<int> arrayRef1a = arrayRef1.dropBack();
   EXPECT_TRUE(arrayRef1a.equals({1, 2, 3, 4, 5, 6, 7}));
   EXPECT_FALSE(arrayRef1a.equals({1, 2, 3, 4, 5, 6, 7, 8}));

   ArrayRef<int> arrayRef1b = arrayRef1a.slice(2, 4);
   EXPECT_TRUE(arrayRef1b.equals({3, 4, 5, 6}));
   EXPECT_FALSE(arrayRef1b.equals({2, 3, 4, 5, 6}));
   EXPECT_FALSE(arrayRef1b.equals({3, 4, 5, 6, 7}));
}

TEST(ArrayRefTest, testEmptyEquals)
{
   EXPECT_TRUE(ArrayRef<unsigned>() == ArrayRef<unsigned>());
}

TEST(ArrayRefTest, testConstConvert)
{
   int buf[4];
   for (int i = 0; i < 4; ++i)
      buf[i] = i;

   static int *A[] = {&buf[0], &buf[1], &buf[2], &buf[3]};
   ArrayRef<const int *> a((ArrayRef<int *>(A)));
   a = ArrayRef<int *>(A);
}

static std::vector<int> ReturnTest12()
{
   return {1, 2};
}

static void ArgTest12(ArrayRef<int> A)
{
   EXPECT_EQ(2U, A.getSize());
   EXPECT_EQ(1, A[0]);
   EXPECT_EQ(2, A[1]);
}

TEST(ArrayRefTest, testInitializerList)
{
   std::initializer_list<int> init_list = { 0, 1, 2, 3, 4 };
   ArrayRef<int> A = init_list;
   for (int i = 0; i < 5; ++i)
      EXPECT_EQ(i, A[i]);

   std::vector<int> B = ReturnTest12();
   A = B;
   EXPECT_EQ(1, A[0]);
   EXPECT_EQ(2, A[1]);

   ArgTest12({1, 2});
}

TEST(ArrayRefTest, testEmptyInitializerList)
{
   ArrayRef<int> A = {};
   EXPECT_TRUE(A.empty());

   A = {};
   EXPECT_TRUE(A.empty());
}

// Test that makeArrayRef works on ArrayRef (no-op)
TEST(ArrayRefTest, testMakeArrayRef)
{
   static const int A1[] = {1, 2, 3, 4, 5, 6, 7, 8};

   // No copy expected for non-const ArrayRef (true no-op)
   ArrayRef<int> arrayRef1(A1);
   ArrayRef<int> &array1Ref = make_array_ref(arrayRef1);
   EXPECT_EQ(&arrayRef1, &array1Ref);

   // A copy is expected for non-const ArrayRef (thin copy)
   const ArrayRef<int> arrayRef2(A1);
   const ArrayRef<int> &arrayRef2Ref = make_array_ref(arrayRef2);
   EXPECT_NE(&arrayRef2Ref, &arrayRef2);
   EXPECT_TRUE(arrayRef2.equals(arrayRef2Ref));
}
