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
#include "polarphp/basic/adt/IntrusiveListNodeBase.h"

using polar::basic::IntrusiveListNodeBase;

namespace {
using RawNode = IntrusiveListNodeBase<false> ;
using TrackingNode = IntrusiveListNodeBase<true>;
} // anonymous namespace

TEST(IntrusiveListNodeBaseTest, testDefaultConstructor)
{
   RawNode rawNode;
   EXPECT_EQ(nullptr, rawNode.getPrev());
   EXPECT_EQ(nullptr, rawNode.getNext());
   EXPECT_FALSE(rawNode.isKnownSentinel());

   TrackingNode trackNode;
   EXPECT_EQ(nullptr, trackNode.getPrev());
   EXPECT_EQ(nullptr, trackNode.getNext());
   EXPECT_FALSE(trackNode.isKnownSentinel());
   EXPECT_FALSE(trackNode.isSentinel());
}

TEST(IntrusiveListNodeBaseTest, testSetPrevAndNext)
{
   RawNode A, B, C;
   A.setPrev(&B);
   EXPECT_EQ(&B, A.getPrev());
   EXPECT_EQ(nullptr, A.getNext());
   EXPECT_EQ(nullptr, B.getPrev());
   EXPECT_EQ(nullptr, B.getNext());
   EXPECT_EQ(nullptr, C.getPrev());
   EXPECT_EQ(nullptr, C.getNext());

   A.setNext(&C);
   EXPECT_EQ(&B, A.getPrev());
   EXPECT_EQ(&C, A.getNext());
   EXPECT_EQ(nullptr, B.getPrev());
   EXPECT_EQ(nullptr, B.getNext());
   EXPECT_EQ(nullptr, C.getPrev());
   EXPECT_EQ(nullptr, C.getNext());

   TrackingNode TA, TB, TC;
   TA.setPrev(&TB);
   EXPECT_EQ(&TB, TA.getPrev());
   EXPECT_EQ(nullptr, TA.getNext());
   EXPECT_EQ(nullptr, TB.getPrev());
   EXPECT_EQ(nullptr, TB.getNext());
   EXPECT_EQ(nullptr, TC.getPrev());
   EXPECT_EQ(nullptr, TC.getNext());

   TA.setNext(&TC);
   EXPECT_EQ(&TB, TA.getPrev());
   EXPECT_EQ(&TC, TA.getNext());
   EXPECT_EQ(nullptr, TB.getPrev());
   EXPECT_EQ(nullptr, TB.getNext());
   EXPECT_EQ(nullptr, TC.getPrev());
   EXPECT_EQ(nullptr, TC.getNext());
}

TEST(IntrusiveListNodeBaseTest, testIsKnownSentinel)
{
   // Without sentinel tracking.
   RawNode A, B;
   EXPECT_FALSE(A.isKnownSentinel());
   A.setPrev(&B);
   A.setNext(&B);
   EXPECT_EQ(&B, A.getPrev());
   EXPECT_EQ(&B, A.getNext());
   EXPECT_FALSE(A.isKnownSentinel());
   A.initializeSentinel();
   EXPECT_FALSE(A.isKnownSentinel());
   EXPECT_EQ(&B, A.getPrev());
   EXPECT_EQ(&B, A.getNext());

   // With sentinel tracking.
   TrackingNode TA, TB;
   EXPECT_FALSE(TA.isKnownSentinel());
   EXPECT_FALSE(TA.isSentinel());
   TA.setPrev(&TB);
   TA.setNext(&TB);
   EXPECT_EQ(&TB, TA.getPrev());
   EXPECT_EQ(&TB, TA.getNext());
   EXPECT_FALSE(TA.isKnownSentinel());
   EXPECT_FALSE(TA.isSentinel());
   TA.initializeSentinel();
   EXPECT_TRUE(TA.isKnownSentinel());
   EXPECT_TRUE(TA.isSentinel());
   EXPECT_EQ(&TB, TA.getPrev());
   EXPECT_EQ(&TB, TA.getNext());
}
