// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/21.

#include "polarphp/basic/adt/DepthFirstIterator.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "TestGraph.h"
#include "gtest/gtest.h"

namespace polar {

using polar::basic::SmallPtrSet;
using polar::basic::Graph;
using polar::basic::DepthFirstIterator;
using polar::basic::DepthFirstIteratorStorage;

template <typename T>
struct CountedSet
{
  typedef typename SmallPtrSet<T, 4>::iterator iterator;

  SmallPtrSet<T, 4> m_set;
  int m_insertVisited = 0;

  std::pair<iterator, bool> insert(const T &item)
  {
    m_insertVisited++;
    return m_set.insert(item);
  }

  size_t count(const T &item) const
  {
     return m_set.count(item);
  }

  void completed(T) { }
};

template <typename T>
class DepthFirstIteratorStorage<CountedSet<T>, true>
{
public:
  DepthFirstIteratorStorage(CountedSet<T> &vset) : m_visited(vset) {}

  CountedSet<T> &m_visited;
};

TEST(DepthFirstIteratorTest, testActuallyUpdateIterator)
{
  typedef CountedSet<Graph<3>::NodeType *> StorageT;
  typedef DepthFirstIterator<Graph<3>, StorageT, true> DFIter;

  Graph<3> graph;
  graph.addEdge(0, 1);
  graph.addEdge(0, 2);
  StorageT m_set;
  for (auto N : make_range(DFIter::begin(graph, m_set), DFIter::end(graph, m_set)))
    (void)N;

  EXPECT_EQ(3, m_set.m_insertVisited);
}
}
