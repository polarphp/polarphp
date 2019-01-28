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

#include "gtest/gtest.h"
#include "polarphp/basic/adt/IntrusiveListBase.h"

namespace {

using polar::basic::IntrusiveListBase;

// Test fixture.
template <typename T>
class IntrusiveListBaseTest : public ::testing::Test
{};

// Test variants with the same test.
typedef ::testing::Types<IntrusiveListBase<false>, IntrusiveListBase<true>>
IntrusiveListBaseTestTypes;

TYPED_TEST_CASE(IntrusiveListBaseTest, IntrusiveListBaseTestTypes);

} // anonymous namespace

TYPED_TEST(IntrusiveListBaseTest, testInsertBeforeImpl)
{
   typedef TypeParam ListBaseType;
   typedef typename ListBaseType::NodeBaseType NodeBaseType;

   NodeBaseType S, A, B;

   // [S] <-> [S]
   S.setPrev(&S);
   S.setNext(&S);

   // [S] <-> A <-> [S]
   ListBaseType::insertBeforeImpl(S, A);
   EXPECT_EQ(&A, S.getPrev());
   EXPECT_EQ(&S, A.getPrev());
   EXPECT_EQ(&A, S.getNext());
   EXPECT_EQ(&S, A.getNext());

   // [S] <-> A <-> B <-> [S]
   ListBaseType::insertBeforeImpl(S, B);
   EXPECT_EQ(&B, S.getPrev());
   EXPECT_EQ(&A, B.getPrev());
   EXPECT_EQ(&S, A.getPrev());
   EXPECT_EQ(&A, S.getNext());
   EXPECT_EQ(&B, A.getNext());
   EXPECT_EQ(&S, B.getNext());
}

TYPED_TEST(IntrusiveListBaseTest, testRemoveImpl)
{
   typedef TypeParam ListBaseType;
   typedef typename ListBaseType::NodeBaseType NodeBaseType;

   NodeBaseType S, A, B;

   // [S] <-> A <-> B <-> [S]
   S.setPrev(&S);
   S.setNext(&S);
   ListBaseType::insertBeforeImpl(S, A);
   ListBaseType::insertBeforeImpl(S, B);

   // [S] <-> B <-> [S]
   ListBaseType::removeImpl(A);
   EXPECT_EQ(&B, S.getPrev());
   EXPECT_EQ(&S, B.getPrev());
   EXPECT_EQ(&B, S.getNext());
   EXPECT_EQ(&S, B.getNext());
   EXPECT_EQ(nullptr, A.getPrev());
   EXPECT_EQ(nullptr, A.getNext());

   // [S] <-> [S]
   ListBaseType::removeImpl(B);
   EXPECT_EQ(&S, S.getPrev());
   EXPECT_EQ(&S, S.getNext());
   EXPECT_EQ(nullptr, B.getPrev());
   EXPECT_EQ(nullptr, B.getNext());
}

TYPED_TEST(IntrusiveListBaseTest, testRemoveRangeImpl)
{
   typedef TypeParam ListBaseType;
   typedef typename ListBaseType::NodeBaseType NodeBaseType;

   NodeBaseType S, A, B, C, D;

   // [S] <-> A <-> B <-> C <-> D <-> [S]
   S.setPrev(&S);
   S.setNext(&S);
   ListBaseType::insertBeforeImpl(S, A);
   ListBaseType::insertBeforeImpl(S, B);
   ListBaseType::insertBeforeImpl(S, C);
   ListBaseType::insertBeforeImpl(S, D);

   // [S] <-> A <-> D <-> [S]
   ListBaseType::removeRangeImpl(B, D);
   EXPECT_EQ(&D, S.getPrev());
   EXPECT_EQ(&A, D.getPrev());
   EXPECT_EQ(&S, A.getPrev());
   EXPECT_EQ(&A, S.getNext());
   EXPECT_EQ(&D, A.getNext());
   EXPECT_EQ(&S, D.getNext());
   EXPECT_EQ(nullptr, B.getPrev());
   EXPECT_EQ(nullptr, C.getNext());
}

TYPED_TEST(IntrusiveListBaseTest, testRemoveRangeImplAllButSentinel)
{
   typedef TypeParam ListBaseType;
   typedef typename ListBaseType::NodeBaseType NodeBaseType;

   NodeBaseType S, A, B;

   // [S] <-> A <-> B <-> [S]
   S.setPrev(&S);
   S.setNext(&S);
   ListBaseType::insertBeforeImpl(S, A);
   ListBaseType::insertBeforeImpl(S, B);

   // [S] <-> [S]
   ListBaseType::removeRangeImpl(A, S);
   EXPECT_EQ(&S, S.getPrev());
   EXPECT_EQ(&S, S.getNext());
   EXPECT_EQ(nullptr, A.getPrev());
   EXPECT_EQ(nullptr, B.getNext());
}

TYPED_TEST(IntrusiveListBaseTest, testTransferBeforeImpl)
{
   typedef TypeParam ListBaseType;
   typedef typename ListBaseType::NodeBaseType NodeBaseType;

   NodeBaseType S1, S2, A, B, C, D, E;

   // [S1] <-> A <-> B <-> C <-> [S1]
   S1.setPrev(&S1);
   S1.setNext(&S1);
   ListBaseType::insertBeforeImpl(S1, A);
   ListBaseType::insertBeforeImpl(S1, B);
   ListBaseType::insertBeforeImpl(S1, C);

   // [S2] <-> D <-> E <-> [S2]
   S2.setPrev(&S2);
   S2.setNext(&S2);
   ListBaseType::insertBeforeImpl(S2, D);
   ListBaseType::insertBeforeImpl(S2, E);

   // [S1] <-> C <-> [S1]
   ListBaseType::transferBeforeImpl(D, A, C);
   EXPECT_EQ(&C, S1.getPrev());
   EXPECT_EQ(&S1, C.getPrev());
   EXPECT_EQ(&C, S1.getNext());
   EXPECT_EQ(&S1, C.getNext());

   // [S2] <-> A <-> B <-> D <-> E <-> [S2]
   EXPECT_EQ(&E, S2.getPrev());
   EXPECT_EQ(&D, E.getPrev());
   EXPECT_EQ(&B, D.getPrev());
   EXPECT_EQ(&A, B.getPrev());
   EXPECT_EQ(&S2, A.getPrev());
   EXPECT_EQ(&A, S2.getNext());
   EXPECT_EQ(&B, A.getNext());
   EXPECT_EQ(&D, B.getNext());
   EXPECT_EQ(&E, D.getNext());
   EXPECT_EQ(&S2, E.getNext());
}
