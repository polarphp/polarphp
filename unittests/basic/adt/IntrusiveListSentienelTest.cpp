// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/07.

#include "polarphp/basic/adt/IntrusiveListNode.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

template <class T, class... Options> struct PickSentinel
{
   typedef IntrusiveListSentinel<
   typename ilist_internal::ComputeNodeOptions<T, Options...>::type>
   type;
};

class Node : public IntrusiveListNode<Node> {};
class TrackingNode : public IntrusiveListNode<Node, IntrusiveListSentinelTracking<true>>
{};
typedef PickSentinel<Node>::type Sentinel;
typedef PickSentinel<Node, IntrusiveListSentinelTracking<true>>::type
TrackingSentinel;
typedef PickSentinel<Node, IntrusiveListSentinelTracking<false>>::type
NoTrackingSentinel;

struct LocalAccess : ilist_internal::NodeAccess
{
   using NodeAccess::getPrev;
   using NodeAccess::getNext;
};

TEST(IntrusiveListSentinelTest, testDefaultConstructor)
{
   Sentinel S;
   EXPECT_EQ(&S, LocalAccess::getPrev(S));
   EXPECT_EQ(&S, LocalAccess::getNext(S));
#if POLAR_ENABLE_ABI_BREAKING_CHECKS
   EXPECT_TRUE(S.isKnownSentinel());
#else
   EXPECT_FALSE(S.isKnownSentinel());
#endif

   TrackingSentinel TS;
   NoTrackingSentinel NTS;
   EXPECT_TRUE(TS.isSentinel());
   EXPECT_TRUE(TS.isKnownSentinel());
   EXPECT_FALSE(NTS.isKnownSentinel());
}

TEST(IntrusiveListSentinelTest, testNormalNodeIsNotKnownSentinel)
{
   Node N;
   EXPECT_EQ(nullptr, LocalAccess::getPrev(N));
   EXPECT_EQ(nullptr, LocalAccess::getNext(N));
   EXPECT_FALSE(N.isKnownSentinel());

   TrackingNode TN;
   EXPECT_FALSE(TN.isSentinel());
}

} // anonymous namespace
