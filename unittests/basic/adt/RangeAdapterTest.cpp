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

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include "gtest/gtest.h"

#include <iterator>
#include <list>
#include <vector>

using namespace polar::basic;

namespace {

// A wrapper around vector which exposes rbegin(), rend().
class ReverseOnlyVector
{
   std::vector<int> m_vector;

public:
   ReverseOnlyVector(std::initializer_list<int> list) : m_vector(list) {}

   typedef std::vector<int>::reverse_iterator reverse_iterator;
   typedef std::vector<int>::const_reverse_iterator const_reverse_iterator;
   reverse_iterator rbegin() { return m_vector.rbegin(); }
   reverse_iterator rend() { return m_vector.rend(); }
   const_reverse_iterator rbegin() const { return m_vector.rbegin(); }
   const_reverse_iterator rend() const { return m_vector.rend(); }
};

// A wrapper around vector which exposes begin(), end(), rbegin() and rend().
// begin() and end() don't have implementations as this ensures that we will
// get a linker error if reverse() chooses begin()/end() over rbegin(), rend().
class BidirectionalVector
{
   mutable std::vector<int> m_vector;

public:
   BidirectionalVector(std::initializer_list<int> list) : m_vector(list) {}

   typedef std::vector<int>::iterator iterator;
   iterator begin() const;
   iterator end() const;

   typedef std::vector<int>::reverse_iterator reverse_iterator;
   reverse_iterator rbegin() const { return m_vector.rbegin(); }
   reverse_iterator rend() const { return m_vector.rend(); }
};

/// This is the same as BidirectionalVector but with the addition of const
/// begin/rbegin methods to ensure that the type traits for has_rbegin works.
class BidirectionalVectorConsts
{
   std::vector<int> m_vector;

public:
   BidirectionalVectorConsts(std::initializer_list<int> list) : m_vector(list) {}

   typedef std::vector<int>::iterator iterator;
   typedef std::vector<int>::const_iterator const_iterator;
   iterator begin();
   iterator end();
   const_iterator begin() const;
   const_iterator end() const;

   typedef std::vector<int>::reverse_iterator reverse_iterator;
   typedef std::vector<int>::const_reverse_iterator const_reverse_iterator;
   reverse_iterator rbegin() { return m_vector.rbegin(); }
   reverse_iterator rend() { return m_vector.rend(); }
   const_reverse_iterator rbegin() const { return m_vector.rbegin(); }
   const_reverse_iterator rend() const { return m_vector.rend(); }
};

/// Check that types with custom iterators work.
class CustomIteratorVector
{
   mutable std::vector<int> m_vector;

public:
   CustomIteratorVector(std::initializer_list<int> list) : m_vector(list) {}

   typedef std::vector<int>::iterator iterator;
   class reverse_iterator {
      std::vector<int>::iterator I;

   public:
      reverse_iterator() = default;
      reverse_iterator(const reverse_iterator &) = default;
      reverse_iterator &operator=(const reverse_iterator &) = default;

      explicit reverse_iterator(std::vector<int>::iterator I) : I(I) {}

      reverse_iterator &operator++() {
         --I;
         return *this;
      }
      reverse_iterator &operator--() {
         ++I;
         return *this;
      }
      int &operator*() const { return *std::prev(I); }
      int *operator->() const { return &*std::prev(I); }
      friend bool operator==(const reverse_iterator &L,
                             const reverse_iterator &R) {
         return L.I == R.I;
      }
      friend bool operator!=(const reverse_iterator &L,
                             const reverse_iterator &R) {
         return !(L == R);
      }
   };

   iterator begin() const { return m_vector.begin(); }
   iterator end()  const { return m_vector.end(); }
   reverse_iterator rbegin() const { return reverse_iterator(m_vector.end()); }
   reverse_iterator rend() const { return reverse_iterator(m_vector.begin()); }
};

template <typename R>
void TestRev(const R &r)
{
   int counter = 3;
   for (int i : r)
      EXPECT_EQ(i, counter--);
}

// Test fixture
template <typename T>
class RangeAdapterLValueTest : public ::testing::Test {};

typedef ::testing::Types<std::vector<int>, std::list<int>, int[4]>
RangeAdapterLValueTestTypes;
TYPED_TEST_CASE(RangeAdapterLValueTest, RangeAdapterLValueTestTypes);

TYPED_TEST(RangeAdapterLValueTest, testTrivialOperation)
{
   TypeParam v = {0, 1, 2, 3};
   TestRev(reverse(v));

   const TypeParam c = {0, 1, 2, 3};
   TestRev(reverse(c));
}

template <typename T> struct RangeAdapterRValueTest : testing::Test {};

typedef ::testing::Types<std::vector<int>, std::list<int>, CustomIteratorVector,
ReverseOnlyVector, BidirectionalVector,
BidirectionalVectorConsts>
RangeAdapterRValueTestTypes;
TYPED_TEST_CASE(RangeAdapterRValueTest, RangeAdapterRValueTestTypes);

TYPED_TEST(RangeAdapterRValueTest, testTrivialOperation)
{
   TestRev(reverse(TypeParam({0, 1, 2, 3})));
}

TYPED_TEST(RangeAdapterRValueTest, testHasRbegin)
{
   static_assert(has_rbegin<TypeParam>::value, "rbegin() should be defined");
}

TYPED_TEST(RangeAdapterRValueTest, testRangeType)
{
   static_assert(
            std::is_same<
            decltype(reverse(*static_cast<TypeParam *>(nullptr)).begin()),
            decltype(static_cast<TypeParam *>(nullptr)->rbegin())>::value,
            "reverse().begin() should have the same type as rbegin()");
   static_assert(
            std::is_same<
            decltype(reverse(*static_cast<const TypeParam *>(nullptr)).begin()),
            decltype(static_cast<const TypeParam *>(nullptr)->rbegin())>::value,
            "reverse().begin() should have the same type as rbegin() [const]");
}

}
