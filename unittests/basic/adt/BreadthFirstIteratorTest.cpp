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

#include "polarphp/basic/adt/BreadthFirstIterator.h"
#include "TestGraph.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(BreadthFristIteratorTest, testBasic)
{
   typedef BreadthFirstIterator<Graph<4>> BFIter;

   Graph<4> G;
   G.addEdge(0, 1);
   G.addEdge(0, 2);
   G.addEdge(1, 3);

   auto iter = BFIter::begin(G);
   auto End = BFIter::end(G);
   EXPECT_EQ(iter.getLevel(), 0U);
   EXPECT_EQ(*iter, G.accessNode(0));
   ++iter;
   EXPECT_EQ(iter.getLevel(), 1U);
   EXPECT_EQ(*iter, G.accessNode(1));
   ++iter;
   EXPECT_EQ(iter.getLevel(), 1U);
   EXPECT_EQ(*iter, G.accessNode(2));
   ++iter;
   EXPECT_EQ(iter.getLevel(), 2U);
   EXPECT_EQ(*iter, G.accessNode(3));
   ++iter;
   EXPECT_EQ(iter, End);
}

TEST(BreadthFristIteratorTest, testCycle)
{
   typedef BreadthFirstIterator<Graph<4>> BFIter;

   Graph<4> G;
   G.addEdge(0, 1);
   G.addEdge(1, 0);
   G.addEdge(1, 2);
   G.addEdge(2, 1);
   G.addEdge(2, 1);
   G.addEdge(2, 3);
   G.addEdge(3, 2);
   G.addEdge(3, 1);
   G.addEdge(3, 0);

   auto iter = BFIter::begin(G);
   auto End = BFIter::end(G);
   EXPECT_EQ(iter.getLevel(), 0U);
   EXPECT_EQ(*iter, G.accessNode(0));
   ++iter;
   EXPECT_EQ(iter.getLevel(), 1U);
   EXPECT_EQ(*iter, G.accessNode(1));
   ++iter;
   EXPECT_EQ(iter.getLevel(), 2U);
   EXPECT_EQ(*iter, G.accessNode(2));
   ++iter;
   EXPECT_EQ(iter.getLevel(), 3U);
   EXPECT_EQ(*iter, G.accessNode(3));
   ++iter;
   EXPECT_EQ(iter, End);
}

} // anonymous namespace
