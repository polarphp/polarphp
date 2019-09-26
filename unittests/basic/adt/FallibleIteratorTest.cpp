//===- unittests/ADT/FallibleIteratorTest.cpp - fallible_iterator.h tests -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/09/26.

#include "polarphp/basic/adt/FallibleIterator.h"
#include "../../support/Error.h"

#include "gtest/gtest-spi.h"
#include "gtest/gtest.h"

#include <utility>
#include <vector>

using namespace polar::basic;
using namespace polar::utils;
using namespace polar::unittest;

namespace {

using ItemValid = enum { ValidItem, InvalidItem };
using LinkValid = enum { ValidLink, InvalidLink };

class Item {
public:
   Item(ItemValid V) : V(V) {}
   bool isValid() const { return V == ValidItem; }

private:
   ItemValid V;
};

// A utility to mock "bad collections". It supports both invalid items,
// where the dereference operator may return an Error, and bad links
// where the inc/dec operations may return an Error.
// Each element of the mock collection contains a pair of a (possibly broken)
// item and link.
using FallibleCollection = std::vector<std::pair<Item, LinkValid>>;

class FallibleCollectionWalker
{
public:
   FallibleCollectionWalker(FallibleCollection &C, unsigned idx)
      : C(C), idx(idx) {}

   Item &operator*()
   {
      return C[idx].first;
   }

   const Item &operator*() const
   {
      return C[idx].first;
   }

   Error inc()
   {
      assert(idx != C.size() && "Walking off end of (mock) collection");
      if (C[idx].second == ValidLink) {
         ++idx;
         return Error::getSuccess();
      }
      return make_error<StringError>("cant get next object in (mock) collection",
                                     inconvertible_error_code());
   }

   Error dec()
   {
      assert(idx != 0 && "Walking off start of (mock) collection");
      --idx;
      if (C[idx].second == ValidLink)
         return Error::getSuccess();
      return make_error<StringError>("cant get prev object in (mock) collection",
                                     inconvertible_error_code());
   }

   friend bool operator==(const FallibleCollectionWalker &lhs,
                          const FallibleCollectionWalker &rhs) {
      assert(&lhs.C == &rhs.C && "Comparing iterators across collectionss.");
      return lhs.idx == rhs.idx;
   }

private:
   FallibleCollection &C;
   unsigned idx;
};

class FallibleCollectionWalkerWithStructDeref
      : public FallibleCollectionWalker
{
public:
   using FallibleCollectionWalker::FallibleCollectionWalker;

   Item *operator->() { return &this->operator*(); }

   const Item *operator->() const { return &this->operator*(); }
};

class FallibleCollectionWalkerWithFallibleDeref
      : public FallibleCollectionWalker
{
public:
   using FallibleCollectionWalker::FallibleCollectionWalker;

   Expected<Item &> operator*() {
      auto &I = FallibleCollectionWalker::operator*();
      if (!I.isValid())
         return make_error<StringError>("bad item", inconvertible_error_code());
      return I;
   }

   Expected<const Item &> operator*() const {
      const auto &I = FallibleCollectionWalker::operator*();
      if (!I.isValid())
         return make_error<StringError>("bad item", inconvertible_error_code());
      return I;
   }
};

TEST(FallibleIteratorTest, testBasicSuccess)
{

   // Check that a basic use-case involing successful iteration over a
   // "FallibleCollection" works.

   FallibleCollection C({{ValidItem, ValidLink}, {ValidItem, ValidLink}});

   FallibleCollectionWalker begin(C, 0);
   FallibleCollectionWalker end(C, 2);

   Error error = Error::getSuccess();
   for (auto &Elem :
        make_fallible_range<FallibleCollectionWalker>(begin, end, error))
      EXPECT_TRUE(Elem.isValid());
   cant_fail(std::move(error));
}

TEST(FallibleIteratorTest, testBasicFailure) {

   // Check that a iteration failure (due to the InvalidLink state on element one
   // of the fallible collection) breaks out of the loop and raises an Error.

   FallibleCollection C({{ValidItem, ValidLink}, {ValidItem, InvalidLink}});

   FallibleCollectionWalker begin(C, 0);
   FallibleCollectionWalker end(C, 2);

   Error error = Error::getSuccess();
   for (auto &Elem :
        make_fallible_range<FallibleCollectionWalker>(begin, end, error))
      EXPECT_TRUE(Elem.isValid());

   EXPECT_THAT_ERROR(std::move(error), Failed()) << "Expected failure value";
}

TEST(FallibleIteratorTest, testNoRedundantErrorCheckOnEarlyExit) {

   // Check that an early return from the loop body does not require a redundant
   // check of error.

   FallibleCollection C({{ValidItem, ValidLink}, {ValidItem, ValidLink}});

   FallibleCollectionWalker begin(C, 0);
   FallibleCollectionWalker end(C, 2);

   Error error = Error::getSuccess();
   for (auto &Elem :
        make_fallible_range<FallibleCollectionWalker>(begin, end, error)) {
      (void)Elem;
      return;
   }
   // error not checked, but should be ok because we exit from the loop
   // body.
}

#if POLAR_ENABLE_ABI_BREAKING_CHECKS
TEST(FallibleIteratorTest, testRegularLoopExitRequiresErrorCheck) {

   // Check that error must be checked after a normal (i.e. not early) loop exit
   // by failing to check and expecting program death (due to the unchecked
   // error).

   EXPECT_DEATH(
   {
               FallibleCollection C({{ValidItem, ValidLink}, {ValidItem, ValidLink}});

               FallibleCollectionWalker begin(C, 0);
               FallibleCollectionWalker end(C, 2);

               Error error = Error::getSuccess();
               for (auto &Elem :
               make_fallible_range<FallibleCollectionWalker>(begin, end, error))
               (void)Elem;
            },
            "Program aborted due to an unhandled Error:")
         << "Normal (i.e. not early) loop exit should require an error check";
}
#endif

TEST(FallibleIteratorTest, testRawIncrementAndDecrementBehavior) {

   // Check the exact behavior of increment / decrement.

   FallibleCollection C({{ValidItem, ValidLink},
                         {ValidItem, InvalidLink},
                         {ValidItem, ValidLink},
                         {ValidItem, InvalidLink}});

   {
      // One increment from begin succeeds.
      Error error = Error::getSuccess();
      auto I = make_fallible_iter(FallibleCollectionWalker(C, 0), error);
      ++I;
      EXPECT_THAT_ERROR(std::move(error), Succeeded());
   }

   {
      // Two increments from begin fail.
      Error error = Error::getSuccess();
      auto I = make_fallible_iter(FallibleCollectionWalker(C, 0), error);
      ++I;
      EXPECT_THAT_ERROR(std::move(error), Succeeded());
      ++I;
      EXPECT_THAT_ERROR(std::move(error), Failed()) << "Expected failure value";
   }

   {
      // One decement from element three succeeds.
      Error error = Error::getSuccess();
      auto I = make_fallible_iter(FallibleCollectionWalker(C, 3), error);
      --I;
      EXPECT_THAT_ERROR(std::move(error), Succeeded());
   }

   {
      // One decement from element three succeeds.
      Error error = Error::getSuccess();
      auto I = make_fallible_iter(FallibleCollectionWalker(C, 3), error);
      --I;
      EXPECT_THAT_ERROR(std::move(error), Succeeded());
      --I;
      EXPECT_THAT_ERROR(std::move(error), Failed());
   }
}

TEST(FallibleIteratorTest, testCheckStructDerefOperatorSupport) {
   // Check that the fallible_iterator wrapper forwards through to the
   // underlying iterator's structure dereference operator if present.

   FallibleCollection C({{ValidItem, ValidLink},
                         {ValidItem, ValidLink},
                         {InvalidItem, InvalidLink}});

   FallibleCollectionWalkerWithStructDeref begin(C, 0);

   {
      Error error = Error::getSuccess();
      auto I = make_fallible_iter(begin, error);
      EXPECT_TRUE(I->isValid());
      cant_fail(std::move(error));
   }

   {
      Error error = Error::getSuccess();
      const auto I = make_fallible_iter(begin, error);
      EXPECT_TRUE(I->isValid());
      cant_fail(std::move(error));
   }
}

TEST(FallibleIteratorTest, testCheckDerefToExpectedSupport) {

   // Check that the fallible_iterator wrapper forwards value types, in
   // particular llvm::Expected, correctly.

   FallibleCollection C({{ValidItem, ValidLink},
                         {InvalidItem, ValidLink},
                         {ValidItem, ValidLink}});

   FallibleCollectionWalkerWithFallibleDeref begin(C, 0);
   FallibleCollectionWalkerWithFallibleDeref end(C, 3);

   Error error = Error::getSuccess();
   auto I = make_fallible_iter(begin, error);
   auto E = make_fallible_end(end);

   Expected<Item> V1 = *I;
   EXPECT_THAT_ERROR(V1.takeError(), Succeeded());
   ++I;
   EXPECT_NE(I, E); // Implicitly check error.
   Expected<Item> V2 = *I;
   EXPECT_THAT_ERROR(V2.takeError(), Failed());
   ++I;
   EXPECT_NE(I, E); // Implicitly check error.
   Expected<Item> V3 = *I;
   EXPECT_THAT_ERROR(V3.takeError(), Succeeded());
   ++I;
   EXPECT_EQ(I, E);
   cant_fail(std::move(error));
}

} // namespace
