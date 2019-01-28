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
#include "polarphp/basic/adt/IntrusiveListNode.h"
#include "polarphp/basic/adt/IntrusiveListNodeOptions.h"

#include <type_traits>

namespace {

using polar::basic::IntrusiveListNode;
using polar::basic::ilist_internal::ComputeNodeOptions;
using namespace polar::basic::ilist_internal;
using polar::basic::IntrusiveListTag;
using polar::basic::IntrusiveListSentinelTracking;
struct Node;

struct TagA {};
struct TagB {};

} // end namespace

TEST(IntrusiveListNodeTest, testOptions)
{
   static_assert(
            std::is_same<ComputeNodeOptions<Node>::type,
            ComputeNodeOptions<Node, IntrusiveListTag<void>>::type>::value,
            "default tag is void");
   static_assert(
            !std::is_same<ComputeNodeOptions<Node, IntrusiveListTag<TagA>>::type,
            ComputeNodeOptions<Node, IntrusiveListTag<void>>::type>::value,
            "default tag is void, different from TagA");
   static_assert(
            !std::is_same<ComputeNodeOptions<Node, IntrusiveListTag<TagA>>::type,
            ComputeNodeOptions<Node, IntrusiveListTag<TagB>>::type>::value,
            "TagA is not TagB");
   static_assert(
            std::is_same<
            ComputeNodeOptions<Node, IntrusiveListSentinelTracking<false>>::type,
            ComputeNodeOptions<Node, IntrusiveListSentinelTracking<false>,
            IntrusiveListTag<void>>::type>::value,
            "default tag is void, even with sentinel tracking off");
   static_assert(
            std::is_same<
            ComputeNodeOptions<Node, IntrusiveListSentinelTracking<false>>::type,
            ComputeNodeOptions<Node, IntrusiveListTag<void>,
            IntrusiveListSentinelTracking<false>>::type>::value,
            "order shouldn't matter");
   static_assert(
            std::is_same<
            ComputeNodeOptions<Node, IntrusiveListSentinelTracking<true>>::type,
            ComputeNodeOptions<Node, IntrusiveListSentinelTracking<true>,
            IntrusiveListTag<void>>::type>::value,
            "default tag is void, even with sentinel tracking on");
   static_assert(
            std::is_same<
            ComputeNodeOptions<Node, IntrusiveListSentinelTracking<true>>::type,
            ComputeNodeOptions<Node, IntrusiveListTag<void>,
            IntrusiveListSentinelTracking<true>>::type>::value,
            "order shouldn't matter");
   static_assert(
            std::is_same<
            ComputeNodeOptions<Node, IntrusiveListSentinelTracking<true>,
            IntrusiveListTag<TagA>>::type,
            ComputeNodeOptions<Node, IntrusiveListTag<TagA>,
            IntrusiveListSentinelTracking<true>>::type>::value,
            "order shouldn't matter with real tags");
}
