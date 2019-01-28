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

#include "polarphp/basic/adt/IntrusiveList.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/IntrusiveListNode.h"
#include "gtest/gtest.h"
#include <ostream>

using namespace polar::basic;

namespace {

struct Node : IntrusiveListNode<Node>
{
   int Value;

   Node() {}
   Node(int Value) : Value(Value) {}
   Node(const Node&) = default;
   ~Node() { Value = -1; }
};

TEST(IntrusiveListTest, testBasic)
{
   IntrusiveList<Node> list;
   list.push_back(new Node(1));
   EXPECT_EQ(1, list.back().Value);
   EXPECT_EQ(nullptr, list.getPrevNode(list.back()));
   EXPECT_EQ(nullptr, list.getNextNode(list.back()));

   list.push_back(new Node(2));
   EXPECT_EQ(2, list.back().Value);
   EXPECT_EQ(2, list.getNextNode(list.front())->Value);
   EXPECT_EQ(1, list.getPrevNode(list.back())->Value);

   const IntrusiveList<Node> &ConstList = list;
   EXPECT_EQ(2, ConstList.back().Value);
   EXPECT_EQ(2, ConstList.getNextNode(ConstList.front())->Value);
   EXPECT_EQ(1, ConstList.getPrevNode(ConstList.back())->Value);
}

TEST(IntrusiveListTest, testCloneFrom)
{
   Node L1Nodes[] = {Node(0), Node(1)};
   Node L2Nodes[] = {Node(0), Node(1)};
   IntrusiveList<Node> l1, l2, L3;

   // Build l1 from L1Nodes.
   l1.push_back(&L1Nodes[0]);
   l1.push_back(&L1Nodes[1]);

   // Build l2 from L2Nodes, based on l1 nodes.
   l2.cloneFrom(l1, [&](const Node &N) { return &L2Nodes[N.Value]; });

   // Add a node to L3 to be deleted, and then rebuild L3 by copying l1.
   L3.push_back(new Node(7));
   L3.cloneFrom(l1, [](const Node &N) { return new Node(N); });

   EXPECT_EQ(2u, l1.size());
   EXPECT_EQ(&L1Nodes[0], &l1.front());
   EXPECT_EQ(&L1Nodes[1], &l1.back());
   EXPECT_EQ(2u, l2.size());
   EXPECT_EQ(&L2Nodes[0], &l2.front());
   EXPECT_EQ(&L2Nodes[1], &l2.back());
   EXPECT_EQ(2u, L3.size());
   EXPECT_EQ(0, L3.front().Value);
   EXPECT_EQ(1, L3.back().Value);

   // Don't free nodes on the stack.
   l1.clearAndLeakNodesUnsafely();
   l2.clearAndLeakNodesUnsafely();
}

TEST(IntrusiveListTest, testSpliceOne)
{
   IntrusiveList<Node> list;
   list.push_back(new Node(1));

   // The single-element splice operation supports noops.
   list.splice(list.begin(), list, list.begin());
   EXPECT_EQ(1u, list.size());
   EXPECT_EQ(1, list.front().Value);
   EXPECT_TRUE(std::next(list.begin()) == list.end());

   // Altenative noop. Move the first element behind itself.
   list.push_back(new Node(2));
   list.push_back(new Node(3));
   list.splice(std::next(list.begin()), list, list.begin());
   EXPECT_EQ(3u, list.size());
   EXPECT_EQ(1, list.front().Value);
   EXPECT_EQ(2, std::next(list.begin())->Value);
   EXPECT_EQ(3, list.back().Value);
}

TEST(IntrusiveListTest, testSpliceSwap)
{
   IntrusiveList<Node> L;
   Node N0(0);
   Node N1(1);
   L.insert(L.end(), &N0);
   L.insert(L.end(), &N1);
   EXPECT_EQ(0, L.front().Value);
   EXPECT_EQ(1, L.back().Value);

   L.splice(L.begin(), L, ++L.begin());
   EXPECT_EQ(1, L.front().Value);
   EXPECT_EQ(0, L.back().Value);

   L.clearAndLeakNodesUnsafely();
}

TEST(IntrusiveListTest, testSpliceSwapOtherWay)
{
   IntrusiveList<Node> L;
   Node N0(0);
   Node N1(1);
   L.insert(L.end(), &N0);
   L.insert(L.end(), &N1);
   EXPECT_EQ(0, L.front().Value);
   EXPECT_EQ(1, L.back().Value);

   L.splice(L.end(), L, L.begin());
   EXPECT_EQ(1, L.front().Value);
   EXPECT_EQ(0, L.back().Value);

   L.clearAndLeakNodesUnsafely();
}

TEST(IntrusiveListTest, testUnsafeClear)
{
   IntrusiveList<Node> list;
   // Before even allocating a sentinel.
   list.clearAndLeakNodesUnsafely();
   EXPECT_EQ(0u, list.size());

   // Empty list with sentinel.
   IntrusiveList<Node>::iterator E = list.end();
   list.clearAndLeakNodesUnsafely();
   EXPECT_EQ(0u, list.size());
   // The sentinel shouldn't change.
   EXPECT_TRUE(E == list.end());

   // list with contents.
   list.push_back(new Node(1));
   ASSERT_EQ(1u, list.size());
   Node *N = &*list.begin();
   EXPECT_EQ(1, N->Value);
   list.clearAndLeakNodesUnsafely();
   EXPECT_EQ(0u, list.size());
   ASSERT_EQ(1, N->Value);
   delete N;

   // list is still functional.
   list.push_back(new Node(5));
   list.push_back(new Node(6));
   ASSERT_EQ(2u, list.size());
   EXPECT_EQ(5, list.front().Value);
   EXPECT_EQ(6, list.back().Value);
}

struct Empty {};
TEST(IntrusiveListTest, testHasObsoleteCustomizationTrait)
{
   // Negative test for HasObsoleteCustomization.
   static_assert(!ilist_internal::HasObsoleteCustomization<Empty, Node>::value,
                 "Empty has no customizations");
}

struct GetNext
{
   Node *getNext(Node *);
};

TEST(IntrusiveListTest, testHasGetNextTrait)
{
   static_assert(ilist_internal::HasGetNext<GetNext, Node>::value,
                 "GetNext has a getNext(Node*)");
   static_assert(ilist_internal::HasObsoleteCustomization<GetNext, Node>::value,
                 "Empty should be obsolete because of getNext()");

   // Negative test for HasGetNext.
   static_assert(!ilist_internal::HasGetNext<Empty, Node>::value,
                 "Empty does not have a getNext(Node*)");
}

struct CreateSentinel
{
   Node *createSentinel();
};

TEST(IntrusiveListTest, testHasCreateSentinelTrait)
{
   static_assert(ilist_internal::HasCreateSentinel<CreateSentinel>::value,
                 "CreateSentinel has a getNext(Node*)");
   static_assert(
            ilist_internal::HasObsoleteCustomization<CreateSentinel, Node>::value,
            "Empty should be obsolete because of createSentinel()");

   // Negative test for HasCreateSentinel.
   static_assert(!ilist_internal::HasCreateSentinel<Empty>::value,
                 "Empty does not have a createSentinel()");
}

struct NodeWithCallback : IntrusiveListNode<NodeWithCallback>
{
   int Value = 0;
   bool IsInList = false;
   bool WasTransferred = false;

   NodeWithCallback() = default;
   NodeWithCallback(int Value) : Value(Value) {}
   NodeWithCallback(const NodeWithCallback &) = delete;
};

} // anonymous namespace

namespace polar {
namespace basic {

template <>
struct IntrusiveListCallbackTraits<NodeWithCallback>
{
   void addNodeToList(NodeWithCallback *N) { N->IsInList = true; }
   void removeNodeFromList(NodeWithCallback *N) { N->IsInList = false; }
   template <class Iterator>
   void transferNodesFromList(IntrusiveListCallbackTraits &Other, Iterator First,
                              Iterator Last)
   {
      for (; First != Last; ++First) {
         First->WasTransferred = true;
         Other.removeNodeFromList(&*First);
         addNodeToList(&*First);
      }
   }
};

} // basic
} // polar

namespace {

TEST(IntrusiveListTest, testAddNodeToList)
{
   IntrusiveList<NodeWithCallback> l1, l2;
   NodeWithCallback N(7);
   ASSERT_FALSE(N.IsInList);
   ASSERT_FALSE(N.WasTransferred);

   l1.insert(l1.begin(), &N);
   ASSERT_EQ(1u, l1.size());
   ASSERT_EQ(&N, &l1.front());
   ASSERT_TRUE(N.IsInList);
   ASSERT_FALSE(N.WasTransferred);

   l2.splice(l2.end(), l1);
   ASSERT_EQ(&N, &l2.front());
   ASSERT_TRUE(N.IsInList);
   ASSERT_TRUE(N.WasTransferred);

   l1.remove(&N);
   ASSERT_EQ(0u, l1.size());
   ASSERT_FALSE(N.IsInList);
   ASSERT_TRUE(N.WasTransferred);
}

struct PrivateNode : private IntrusiveListNode<PrivateNode>
{
   friend struct ilist_internal::NodeAccess;

   int Value = 0;

   PrivateNode() = default;
   PrivateNode(int Value) : Value(Value) {}
   PrivateNode(const PrivateNode &) = delete;
};

TEST(IntrusiveListTest, testPrivateNode)
{
   // Instantiate various APIs to be sure they're callable when ilist_node is
   // inherited privately.
   IntrusiveList<PrivateNode> L;
   PrivateNode N(7);
   L.insert(L.begin(), &N);
   ++L.begin();
   (void)*L.begin();
   (void)(L.begin() == L.end());

   IntrusiveList<PrivateNode> l2;
   l2.splice(l2.end(), L);
   l2.remove(&N);
}

} // end namespace

