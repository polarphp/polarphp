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

#include "polarphp/basic/adt/FuncExtras.h"
#include "gtest/gtest.h"

#include <memory>

namespace {

using polar::unique_function;

TEST(UniqueFunctionTest, Basic) {
   unique_function<int(int, int)> sum = [](int A, int B) { return A + B; };
   EXPECT_EQ(sum(1, 2), 3);

   unique_function<int(int, int)> sum2 = std::move(sum);
   EXPECT_EQ(sum2(1, 2), 3);

   unique_function<int(int, int)> Sum3 = [](int A, int B) { return A + B; };
   sum2 = std::move(Sum3);
   EXPECT_EQ(sum2(1, 2), 3);

   sum2 = unique_function<int(int, int)>([](int A, int B) { return A + B; });
   EXPECT_EQ(sum2(1, 2), 3);

   // Explicit self-move test.
   *&sum2 = std::move(sum2);
   EXPECT_EQ(sum2(1, 2), 3);

   sum2 = unique_function<int(int, int)>();
   EXPECT_FALSE(sum2);

   // Make sure we can forward through l-value reference parameters.
   unique_function<void(int &)> inc = [](int &X) { ++X; };
   int X = 42;
   inc(X);
   EXPECT_EQ(X, 43);

   // Make sure we can forward through r-value reference parameters with
   // move-only types.
   unique_function<int(std::unique_ptr<int> &&)> readAndDeallocByRef =
         [](std::unique_ptr<int> &&ptr) {
      int V = *ptr;
      ptr.reset();
      return V;
   };
   std::unique_ptr<int> ptr{new int(13)};
   EXPECT_EQ(readAndDeallocByRef(std::move(ptr)), 13);
   EXPECT_FALSE((bool)ptr);

   // Make sure we can pass a move-only temporary as opposed to a local variable.
   EXPECT_EQ(readAndDeallocByRef(std::unique_ptr<int>(new int(42))), 42);

   // Make sure we can pass a move-only type by-value.
   unique_function<int(std::unique_ptr<int>)> readAndDeallocByVal =
         [](std::unique_ptr<int> ptr) {
      int V = *ptr;
      ptr.reset();
      return V;
   };
   ptr.reset(new int(13));
   EXPECT_EQ(readAndDeallocByVal(std::move(ptr)), 13);
   EXPECT_FALSE((bool)ptr);

   EXPECT_EQ(readAndDeallocByVal(std::unique_ptr<int>(new int(42))), 42);
}

TEST(UniqueFunctionTest, Captures) {
   long A = 1, B = 2, C = 3, D = 4, E = 5;

   unique_function<long()> temp;

   unique_function<long()> C1 = [A]() { return A; };
   EXPECT_EQ(C1(), 1);
   temp = std::move(C1);
   EXPECT_EQ(temp(), 1);

   unique_function<long()> C2 = [A, B]() { return A + B; };
   EXPECT_EQ(C2(), 3);
   temp = std::move(C2);
   EXPECT_EQ(temp(), 3);

   unique_function<long()> C3 = [A, B, C]() { return A + B + C; };
   EXPECT_EQ(C3(), 6);
   temp = std::move(C3);
   EXPECT_EQ(temp(), 6);

   unique_function<long()> C4 = [A, B, C, D]() { return A + B + C + D; };
   EXPECT_EQ(C4(), 10);
   temp = std::move(C4);
   EXPECT_EQ(temp(), 10);

   unique_function<long()> C5 = [A, B, C, D, E]() { return A + B + C + D + E; };
   EXPECT_EQ(C5(), 15);
   temp = std::move(C5);
   EXPECT_EQ(temp(), 15);
}

TEST(UniqueFunctionTest, MoveOnly) {
   struct SmallCallable {
      std::unique_ptr<int> A{new int(1)};

      int operator()(int B) { return *A + B; }
   };
   unique_function<int(int)> Small = SmallCallable();
   EXPECT_EQ(Small(2), 3);
   unique_function<int(int)> Small2 = std::move(Small);
   EXPECT_EQ(Small2(2), 3);

   struct LargeCallable {
      std::unique_ptr<int> A{new int(1)};
      std::unique_ptr<int> B{new int(2)};
      std::unique_ptr<int> C{new int(3)};
      std::unique_ptr<int> D{new int(4)};
      std::unique_ptr<int> E{new int(5)};

      int operator()() { return *A + *B + *C + *D + *E; }
   };
   unique_function<int()> large = LargeCallable();
   EXPECT_EQ(large(), 15);
   unique_function<int()> Large2 = std::move(large);
   EXPECT_EQ(Large2(), 15);
}

TEST(UniqueFunctionTest, CountForwardingCopies) {
   struct CopyCounter {
      int &copyCount;

      CopyCounter(int &copyCount) : copyCount(copyCount) {}
      CopyCounter(const CopyCounter &Arg) : copyCount(Arg.copyCount) {
         ++copyCount;
      }
   };

   unique_function<void(CopyCounter)> byValF = [](CopyCounter) {};
   int copyCount = 0;
   byValF(CopyCounter(copyCount));
   EXPECT_EQ(1, copyCount);

   copyCount = 0;
   {
      CopyCounter counter{copyCount};
      byValF(counter);
   }
   EXPECT_EQ(2, copyCount);

   // Check that we don't generate a copy at all when we can bind a reference all
   // the way down, even if that reference could *in theory* allow copies.
   unique_function<void(const CopyCounter &)> byRefF = [](const CopyCounter &) {
   };
   copyCount = 0;
   byRefF(CopyCounter(copyCount));
   EXPECT_EQ(0, copyCount);

   copyCount = 0;
   {
      CopyCounter counter{copyCount};
      byRefF(counter);
   }
   EXPECT_EQ(0, copyCount);

   // If we use a reference, we can make a stronger guarantee that *no* copy
   // occurs.
   struct Uncopyable {
      Uncopyable() = default;
      Uncopyable(const Uncopyable &) = delete;
   };
   unique_function<void(const Uncopyable &)> UncopyableF =
         [](const Uncopyable &) {};
   UncopyableF(Uncopyable());
   Uncopyable X;
   UncopyableF(X);
}

TEST(UniqueFunctionTest, CountForwardingMoves) {
   struct MoveCounter {
      int &moveCount;

      MoveCounter(int &moveCount) : moveCount(moveCount) {}
      MoveCounter(MoveCounter &&Arg) : moveCount(Arg.moveCount) { ++moveCount; }
   };

   unique_function<void(MoveCounter)> byValF = [](MoveCounter) {};
   int moveCount = 0;
   byValF(MoveCounter(moveCount));
   EXPECT_EQ(1, moveCount);

   moveCount = 0;
   {
      MoveCounter counter{moveCount};
      byValF(std::move(counter));
   }
   EXPECT_EQ(2, moveCount);

   // Check that when we use an r-value reference we get no spurious copies.
   unique_function<void(MoveCounter &&)> byRefF = [](MoveCounter &&) {};
   moveCount = 0;
   byRefF(MoveCounter(moveCount));
   EXPECT_EQ(0, moveCount);

   moveCount = 0;
   {
      MoveCounter counter{moveCount};
      byRefF(std::move(counter));
   }
   EXPECT_EQ(0, moveCount);

   // If we use an r-value reference we can in fact make a stronger guarantee
   // with an unmovable type.
   struct Unmovable {
      Unmovable() = default;
      Unmovable(Unmovable &&) = delete;
   };
   unique_function<void(const Unmovable &)> unmovableF = [](const Unmovable &) {
   };
   unmovableF(Unmovable());
   Unmovable X;
   unmovableF(X);
}

}
