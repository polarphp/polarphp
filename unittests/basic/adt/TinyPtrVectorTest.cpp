// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/09.

#include "polarphp/basic/adt/TinyPtrVector.h"
#include "polarphp/basic/adt/ArrayRef.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/utils/TypeTraits.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <random>
#include <vector>

using namespace polar::basic;

namespace {

template <typename VectorT>
class TinyPtrVectorTest : public testing::Test
{
protected:
   typedef typename VectorT::value_type PtrT;
   typedef typename std::remove_pointer<PtrT>::type ValueT;

   VectorT V;
   VectorT V2;

   ValueT TestValues[1024];
   std::vector<PtrT> TestPtrs;

   TinyPtrVectorTest()
   {
      for (size_t i = 0, e = array_lengthof(TestValues); i != e; ++i) {
         TestPtrs.push_back(&TestValues[i]);
      }
      std::shuffle(TestPtrs.begin(), TestPtrs.end(), std::mt19937{});
   }

   ArrayRef<PtrT> testArray(size_t N)
   {
      return make_array_ref(&TestPtrs[0], N);
   }

   void appendValues(VectorT &V, ArrayRef<PtrT> values)
   {
      for (size_t i = 0, e = values.getSize(); i != e; ++i) {
         V.push_back(values[i]);
      }
   }

   void setVectors(ArrayRef<PtrT> Values1, ArrayRef<PtrT> Values2)
   {
      V.clear();
      appendValues(V, Values1);
      V2.clear();
      appendValues(V2, Values2);
   }

   void expectValues(const VectorT &V, ArrayRef<PtrT> values)
   {
      EXPECT_EQ(values.empty(), V.empty());
      EXPECT_EQ(values.getSize(), V.size());
      for (size_t i = 0, e = values.getSize(); i != e; ++i) {
         EXPECT_EQ(values[i], V[i]);
         EXPECT_EQ(values[i], *std::next(V.begin(), i));
      }
      EXPECT_EQ(V.end(), std::next(V.begin(), values.getSize()));
   }
};

typedef ::testing::Types<TinyPtrVector<int*>,
TinyPtrVector<double*>
> TinyPtrVectorTestTypes;
TYPED_TEST_CASE(TinyPtrVectorTest, TinyPtrVectorTestTypes);

TYPED_TEST(TinyPtrVectorTest, EmptyTest)
{
   this->expectValues(this->V, this->testArray(0));
}

TYPED_TEST(TinyPtrVectorTest, testPushPopBack)
{
   this->V.push_back(this->TestPtrs[0]);
   this->expectValues(this->V, this->testArray(1));
   this->V.push_back(this->TestPtrs[1]);
   this->expectValues(this->V, this->testArray(2));
   this->V.push_back(this->TestPtrs[2]);
   this->expectValues(this->V, this->testArray(3));
   this->V.push_back(this->TestPtrs[3]);
   this->expectValues(this->V, this->testArray(4));
   this->V.push_back(this->TestPtrs[4]);
   this->expectValues(this->V, this->testArray(5));

   // Pop and clobber a few values to keep things interesting.
   this->V.pop_back();
   this->expectValues(this->V, this->testArray(4));
   this->V.pop_back();
   this->expectValues(this->V, this->testArray(3));
   this->TestPtrs[3] = &this->TestValues[42];
   this->TestPtrs[4] = &this->TestValues[43];
   this->V.push_back(this->TestPtrs[3]);
   this->expectValues(this->V, this->testArray(4));
   this->V.push_back(this->TestPtrs[4]);
   this->expectValues(this->V, this->testArray(5));

   this->V.pop_back();
   this->expectValues(this->V, this->testArray(4));
   this->V.pop_back();
   this->expectValues(this->V, this->testArray(3));
   this->V.pop_back();
   this->expectValues(this->V, this->testArray(2));
   this->V.pop_back();
   this->expectValues(this->V, this->testArray(1));
   this->V.pop_back();
   this->expectValues(this->V, this->testArray(0));

   this->appendValues(this->V, this->testArray(42));
   this->expectValues(this->V, this->testArray(42));
}

TYPED_TEST(TinyPtrVectorTest, testClearTest)
{
   this->expectValues(this->V, this->testArray(0));
   this->V.clear();
   this->expectValues(this->V, this->testArray(0));

   this->appendValues(this->V, this->testArray(1));
   this->expectValues(this->V, this->testArray(1));
   this->V.clear();
   this->expectValues(this->V, this->testArray(0));

   this->appendValues(this->V, this->testArray(42));
   this->expectValues(this->V, this->testArray(42));
   this->V.clear();
   this->expectValues(this->V, this->testArray(0));
}

TYPED_TEST(TinyPtrVectorTest, testCopyAndMoveCtorTest)
{
   this->appendValues(this->V, this->testArray(42));
   TypeParam Copy(this->V);
   this->expectValues(Copy, this->testArray(42));

   // This is a separate copy, and so it shouldn't destroy the original.
   Copy.clear();
   this->expectValues(Copy, this->testArray(0));
   this->expectValues(this->V, this->testArray(42));

   TypeParam Copy2(this->V2);
   this->appendValues(Copy2, this->testArray(42));
   this->expectValues(Copy2, this->testArray(42));
   this->expectValues(this->V2, this->testArray(0));

   TypeParam Move(std::move(Copy2));
   this->expectValues(Move, this->testArray(42));
   this->expectValues(Copy2, this->testArray(0));

   TypeParam MultipleElements(this->testArray(2));
   TypeParam SingleElement(this->testArray(1));
   MultipleElements = std::move(SingleElement);
   this->expectValues(MultipleElements, this->testArray(1));
   this->expectValues(SingleElement, this->testArray(0));
}

TYPED_TEST(TinyPtrVectorTest, testCopyAndMoveTest)
{
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(0));
   this->expectValues(this->V2, this->testArray(0));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(0));

   this->setVectors(this->testArray(1), this->testArray(0));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(0));
   this->expectValues(this->V2, this->testArray(0));
   this->setVectors(this->testArray(1), this->testArray(0));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(0));

   this->setVectors(this->testArray(2), this->testArray(0));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(0));
   this->expectValues(this->V2, this->testArray(0));
   this->setVectors(this->testArray(2), this->testArray(0));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(0));

   this->setVectors(this->testArray(42), this->testArray(0));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(0));
   this->expectValues(this->V2, this->testArray(0));
   this->setVectors(this->testArray(42), this->testArray(0));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(0));

   this->setVectors(this->testArray(0), this->testArray(1));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(1));
   this->expectValues(this->V2, this->testArray(1));
   this->setVectors(this->testArray(0), this->testArray(1));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(1));

   this->setVectors(this->testArray(0), this->testArray(2));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(2));
   this->expectValues(this->V2, this->testArray(2));
   this->setVectors(this->testArray(0), this->testArray(2));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(2));

   this->setVectors(this->testArray(0), this->testArray(42));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(42));
   this->expectValues(this->V2, this->testArray(42));
   this->setVectors(this->testArray(0), this->testArray(42));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(42));

   this->setVectors(this->testArray(1), this->testArray(1));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(1));
   this->expectValues(this->V2, this->testArray(1));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(1));

   this->setVectors(this->testArray(1), this->testArray(2));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(2));
   this->expectValues(this->V2, this->testArray(2));
   this->setVectors(this->testArray(1), this->testArray(2));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(2));

   this->setVectors(this->testArray(1), this->testArray(42));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(42));
   this->expectValues(this->V2, this->testArray(42));
   this->setVectors(this->testArray(1), this->testArray(42));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(42));

   this->setVectors(this->testArray(2), this->testArray(1));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(1));
   this->expectValues(this->V2, this->testArray(1));
   this->setVectors(this->testArray(2), this->testArray(1));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(1));

   this->setVectors(this->testArray(2), this->testArray(2));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(2));
   this->expectValues(this->V2, this->testArray(2));
   this->setVectors(this->testArray(2), this->testArray(2));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(2));

   this->setVectors(this->testArray(2), this->testArray(42));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(42));
   this->expectValues(this->V2, this->testArray(42));
   this->setVectors(this->testArray(2), this->testArray(42));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(42));

   this->setVectors(this->testArray(42), this->testArray(1));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(1));
   this->expectValues(this->V2, this->testArray(1));
   this->setVectors(this->testArray(42), this->testArray(1));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(1));

   this->setVectors(this->testArray(42), this->testArray(2));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(2));
   this->expectValues(this->V2, this->testArray(2));
   this->setVectors(this->testArray(42), this->testArray(2));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(2));

   this->setVectors(this->testArray(42), this->testArray(42));
   this->V = this->V2;
   this->expectValues(this->V, this->testArray(42));
   this->expectValues(this->V2, this->testArray(42));
   this->setVectors(this->testArray(42), this->testArray(42));
   this->V = std::move(this->V2);
   this->expectValues(this->V, this->testArray(42));
}

TYPED_TEST(TinyPtrVectorTest, testEraseTest)
{
   this->appendValues(this->V, this->testArray(1));
   this->expectValues(this->V, this->testArray(1));
   this->V.erase(this->V.begin());
   this->expectValues(this->V, this->testArray(0));

   this->appendValues(this->V, this->testArray(42));
   this->expectValues(this->V, this->testArray(42));
   this->V.erase(this->V.begin());
   this->TestPtrs.erase(this->TestPtrs.begin());
   this->expectValues(this->V, this->testArray(41));
   this->V.erase(std::next(this->V.begin(), 1));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 1));
   this->expectValues(this->V, this->testArray(40));
   this->V.erase(std::next(this->V.begin(), 2));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 2));
   this->expectValues(this->V, this->testArray(39));
   this->V.erase(std::next(this->V.begin(), 5));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 5));
   this->expectValues(this->V, this->testArray(38));
   this->V.erase(std::next(this->V.begin(), 13));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 13));
   this->expectValues(this->V, this->testArray(37));

   typename TypeParam::iterator I = this->V.begin();
   do {
      I = this->V.erase(I);
   } while (I != this->V.end());
   this->expectValues(this->V, this->testArray(0));
}

TYPED_TEST(TinyPtrVectorTest, testEraseRangeTest)
{
   this->appendValues(this->V, this->testArray(1));
   this->expectValues(this->V, this->testArray(1));
   this->V.erase(this->V.begin(), this->V.begin());
   this->expectValues(this->V, this->testArray(1));
   this->V.erase(this->V.end(), this->V.end());
   this->expectValues(this->V, this->testArray(1));
   this->V.erase(this->V.begin(), this->V.end());
   this->expectValues(this->V, this->testArray(0));

   this->appendValues(this->V, this->testArray(42));
   this->expectValues(this->V, this->testArray(42));
   this->V.erase(this->V.begin(), std::next(this->V.begin(), 1));
   this->TestPtrs.erase(this->TestPtrs.begin(),
                        std::next(this->TestPtrs.begin(), 1));
   this->expectValues(this->V, this->testArray(41));
   this->V.erase(std::next(this->V.begin(), 1), std::next(this->V.begin(), 2));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 1),
                        std::next(this->TestPtrs.begin(), 2));
   this->expectValues(this->V, this->testArray(40));
   this->V.erase(std::next(this->V.begin(), 2), std::next(this->V.begin(), 4));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 2),
                        std::next(this->TestPtrs.begin(), 4));
   this->expectValues(this->V, this->testArray(38));
   this->V.erase(std::next(this->V.begin(), 5), std::next(this->V.begin(), 10));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 5),
                        std::next(this->TestPtrs.begin(), 10));
   this->expectValues(this->V, this->testArray(33));
   this->V.erase(std::next(this->V.begin(), 13), std::next(this->V.begin(), 26));
   this->TestPtrs.erase(std::next(this->TestPtrs.begin(), 13),
                        std::next(this->TestPtrs.begin(), 26));
   this->expectValues(this->V, this->testArray(20));
   this->V.erase(std::next(this->V.begin(), 7), this->V.end());
   this->expectValues(this->V, this->testArray(7));
   this->V.erase(this->V.begin(), this->V.end());
   this->expectValues(this->V, this->testArray(0));
}

TYPED_TEST(TinyPtrVectorTest, testInsert)
{
   this->V.insert(this->V.end(), this->TestPtrs[0]);
   this->expectValues(this->V, this->testArray(1));
   this->V.clear();
   this->appendValues(this->V, this->testArray(4));
   this->expectValues(this->V, this->testArray(4));
   this->V.insert(this->V.end(), this->TestPtrs[4]);
   this->expectValues(this->V, this->testArray(5));
   this->V.insert(this->V.begin(), this->TestPtrs[42]);
   this->TestPtrs.insert(this->TestPtrs.begin(), this->TestPtrs[42]);
   this->expectValues(this->V, this->testArray(6));
   this->V.insert(std::next(this->V.begin(), 3), this->TestPtrs[43]);
   this->TestPtrs.insert(std::next(this->TestPtrs.begin(), 3),
                         this->TestPtrs[43]);
   this->expectValues(this->V, this->testArray(7));
}

TYPED_TEST(TinyPtrVectorTest, testInsertRange)
{
   this->V.insert(this->V.end(), this->TestPtrs.begin(), this->TestPtrs.begin());
   this->expectValues(this->V, this->testArray(0));
   this->V.insert(this->V.begin(), this->TestPtrs.begin(),
                  this->TestPtrs.begin());
   this->expectValues(this->V, this->testArray(0));
   this->V.insert(this->V.end(), this->TestPtrs.end(), this->TestPtrs.end());
   this->expectValues(this->V, this->testArray(0));
   this->V.insert(this->V.end(), this->TestPtrs.begin(),
                  std::next(this->TestPtrs.begin()));
   this->expectValues(this->V, this->testArray(1));
   this->V.clear();
   this->V.insert(this->V.end(), this->TestPtrs.begin(),
                  std::next(this->TestPtrs.begin(), 2));
   this->expectValues(this->V, this->testArray(2));
   this->V.clear();
   this->V.insert(this->V.end(), this->TestPtrs.begin(),
                  std::next(this->TestPtrs.begin(), 42));
   this->expectValues(this->V, this->testArray(42));
   this->V.clear();
   this->V.insert(this->V.end(),
                  std::next(this->TestPtrs.begin(), 5),
                  std::next(this->TestPtrs.begin(), 13));
   this->V.insert(this->V.begin(),
                  std::next(this->TestPtrs.begin(), 0),
                  std::next(this->TestPtrs.begin(), 3));
   this->V.insert(std::next(this->V.begin(), 2),
                  std::next(this->TestPtrs.begin(), 2),
                  std::next(this->TestPtrs.begin(), 4));
   this->V.erase(std::next(this->V.begin(), 4));
   this->V.insert(std::next(this->V.begin(), 4),
                  std::next(this->TestPtrs.begin(), 4),
                  std::next(this->TestPtrs.begin(), 5));
   this->expectValues(this->V, this->testArray(13));
}

}

TEST(TinyPtrVectorTest, testSingleEltCtorTest)
{
   int v = 55;
   TinyPtrVector<int *> V(&v);

   EXPECT_TRUE(V.size() == 1);
   EXPECT_FALSE(V.empty());
   EXPECT_TRUE(V.front() == &v);
}

TEST(TinyPtrVectorTest, testArrayRefCtorTest)
{
   int data_array[128];
   std::vector<int *> data;

   for (unsigned i = 0, e = 128; i != e; ++i) {
      data_array[i] = 324 - int(i);
      data.push_back(&data_array[i]);
   }

   TinyPtrVector<int *> V(data);
   EXPECT_TRUE(V.size() == 128);
   EXPECT_FALSE(V.empty());
   for (unsigned i = 0, e = 128; i != e; ++i) {
      EXPECT_TRUE(V[i] == data[i]);
   }
}

TEST(TinyPtrVectorTest, testMutableArrayRefTest)
{
   int data_array[128];
   std::vector<int *> data;

   for (unsigned i = 0, e = 128; i != e; ++i) {
      data_array[i] = 324 - int(i);
      data.push_back(&data_array[i]);
   }

   TinyPtrVector<int *> V(data);
   EXPECT_TRUE(V.size() == 128);
   EXPECT_FALSE(V.empty());

   MutableArrayRef<int *> mut_array = V;
   for (unsigned i = 0, e = 128; i != e; ++i) {
      EXPECT_TRUE(mut_array[i] == data[i]);
      mut_array[i] = 324 + mut_array[i];
      EXPECT_TRUE(mut_array[i] == (324 + data[i]));
   }
}
