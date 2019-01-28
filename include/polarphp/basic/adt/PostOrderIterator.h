// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.

#ifndef POLARPHP_BASIC_ADT_POST_ORDER_ITERATOR_H
#define POLARPHP_BASIC_ADT_POST_ORDER_ITERATOR_H

#include "polarphp/basic/adt/GraphTraits.h"
#include "polarphp/basic/adt/SmallPtrSet.h"
#include "polarphp/basic/adt/IteratorRange.h"
#include <iterator>
#include <set>
#include <utility>
#include <vector>
#include <optional>

namespace polar {
namespace basic {

// The PostOrderIteratorStorage template provides access to the set of already
// visited nodes during the PostOrderIterator's depth-first traversal.
//
// The default implementation simply contains a set of visited nodes, while
// the External=true version uses a reference to an external set.
//
// It is possible to prune the depth-first traversal in several ways:
//
// - When providing an external set that already contains some graph nodes,
//   those nodes won't be visited again. This is useful for restarting a
//   post-order traversal on a graph with nodes that aren't dominated by a
//   single node.
//
// - By providing a custom SetType class, unwanted graph nodes can be excluded
//   by having the insert() function return false. This could for example
//   confine a CFG traversal to blocks in a specific loop.
//
// - Finally, by specializing the PostOrderIteratorStorage template itself, graph
//   edges can be pruned by returning false in the insertEdge() function. This
//   could be used to remove loop back-edges from the CFG seen by PostOrderIterator.
//
// A specialized PostOrderIteratorStorage class can observe both the pre-order and
// the post-order. The insertEdge() function is called in a pre-order, while
// the finishPostorder() function is called just before the PostOrderIterator moves
// on to the next node.

/// Default PostOrderIteratorStorage implementation with an internal set object.
template <typename SetType, bool External>
class PostOrderIteratorStorage
{
   SetType m_visited;

public:
   // Return true if edge destination should be visited.
   template <typename NodeRef>
   bool insertEdge(std::optional<NodeRef> from, NodeRef to)
   {
      return m_visited.insert(to).second;
   }

   // Called after all children of node have been visited.
   template <typename NodeRef>
   void finishPostorder(NodeRef node)
   {}
};

/// Specialization of PostOrderIteratorStorage that references an external set.
template <typename SetType>
class PostOrderIteratorStorage<SetType, true> {
   SetType &m_visited;

public:
   PostOrderIteratorStorage(SetType &vset) : m_visited(vset)
   {}

   PostOrderIteratorStorage(const PostOrderIteratorStorage &storage) : m_visited(storage.m_visited)
   {}

   // Return true if edge destination should be visited, called with from = 0 for
   // the root node.
   // Graph edges can be pruned by specializing this function.
   template <typename NodeRef>
   bool insertEdge(std::optional<NodeRef> from, NodeRef to)
   {
      return m_visited.insert(to).second;
   }

   // Called after all children of node have been visited.
   template <typename NodeRef> void finishPostorder(NodeRef node)
   {}
};

template <typename GraphT,
          typename SetType =
          SmallPtrSet<typename GraphTraits<GraphT>::NodeRef, 8>,
          bool ExtStorage = false, typename GT = GraphTraits<GraphT>>
class PostOrderIterator
      : public std::iterator<std::forward_iterator_tag, typename GT::NodeRef>,
      public PostOrderIteratorStorage<SetType, ExtStorage>
{
   using super = std::iterator<std::forward_iterator_tag, typename GT::NodeRef>;
   using NodeRef = typename GT::NodeRef;
   using ChildItTy = typename GT::ChildIteratorType;

   // m_visitStack - Used to maintain the ordering.  Top = current block
   // First element is basic block pointer, second is the 'next child' to visit
   std::vector<std::pair<NodeRef, ChildItTy>> m_visitStack;

   PostOrderIterator(NodeRef node)
   {
      this->insertEdge(std::optional<NodeRef>(), node);
      m_visitStack.push_back(std::make_pair(node, GT::child_begin(node)));
      traverseChild();
   }

   PostOrderIterator() = default; // End is when stack is empty.

   PostOrderIterator(NodeRef node, SetType &set)
      : PostOrderIteratorStorage<SetType, ExtStorage>(set)
   {
      if (this->insertEdge(std::optional<NodeRef>(), node)) {
         m_visitStack.push_back(std::make_pair(node, GT::child_begin(node)));
         traverseChild();
      }
   }

   PostOrderIterator(SetType &set)
      : PostOrderIteratorStorage<SetType, ExtStorage>(set)
   {
   } // End is when stack is empty.

   void traverseChild()
   {
      while (m_visitStack.back().second != GT::child_end(m_visitStack.back().first)) {
         NodeRef node = *m_visitStack.back().second++;
         if (this->insertEdge(std::optional<NodeRef>(m_visitStack.back().first), node)) {
            // If the block is not visited...
            m_visitStack.push_back(std::make_pair(node, GT::child_begin(node)));
         }
      }
   }

public:
   using pointer = typename super::pointer;

   // Provide static "constructors"...
   static PostOrderIterator begin(GraphT graph)
   {
      return PostOrderIterator(GT::getEntryNode(graph));
   }

   static PostOrderIterator end(GraphT graph)
   {
      return PostOrderIterator();
   }

   static PostOrderIterator begin(GraphT graph, SetType &set)
   {
      return PostOrderIterator(GT::getEntryNode(graph), set);
   }

   static PostOrderIterator end(GraphT graph, SetType &set)
   {
      return PostOrderIterator(set);
   }

   bool operator==(const PostOrderIterator &iter) const
   {
      return m_visitStack == iter.m_visitStack;
   }

   bool operator!=(const PostOrderIterator &iter) const
   {
      return !(*this == iter);
   }

   const NodeRef &operator*() const
   {
      return m_visitStack.back().first;
   }

   // This is a nonstandard operator-> that dereferences the pointer an extra
   // time... so that you can actually call methods ON the BasicBlock, because
   // the contained type is a pointer.  This allows BBIt->getTerminator() f.e.
   //
   NodeRef operator->() const
   {
      return **this;
   }

   PostOrderIterator &operator++()
   {
      // Preincrement
      this->finishPostorder(m_visitStack.back().first);
      m_visitStack.pop_back();
      if (!m_visitStack.empty()) {
         traverseChild();
      }
      return *this;
   }

   PostOrderIterator operator++(int)
   {
      // Postincrement
      PostOrderIterator tmp = *this;
      ++*this;
      return tmp;
   }
};

// Provide global constructors that automatically figure out correct types...
//
template <typename T>
PostOrderIterator<T> po_begin(const T &graph)
{
   return PostOrderIterator<T>::begin(graph);
}

template <typename T>
PostOrderIterator<T> po_end(const T &graph)
{
   return PostOrderIterator<T>::end(graph);
}

template <typename T> IteratorRange<PostOrderIterator<T>> post_order(const T &graph)
{
   return make_range(po_begin(graph), po_end(graph));
}

// Provide global definitions of external postorder iterators...
template <typename T, typename SetType = std::set<typename GraphTraits<T>::NodeRef>>
struct po_ext_iterator : public PostOrderIterator<T, SetType, true>
{
   po_ext_iterator(const PostOrderIterator<T, SetType, true> &iter) :
      PostOrderIterator<T, SetType, true>(iter) {}
};

template <typename T, typename SetType>
po_ext_iterator<T, SetType> po_ext_begin(T graph, SetType &set)
{
   return po_ext_iterator<T, SetType>::begin(graph, set);
}

template <typename T, typename SetType>
po_ext_iterator<T, SetType> po_ext_end(T graph, SetType &set)
{
   return po_ext_iterator<T, SetType>::end(graph, set);
}

template <typename T, typename SetType>
iterator_range<po_ext_iterator<T, SetType>> post_order_ext(const T &graph, SetType &set)
{
   return make_range(po_ext_begin(graph, set), po_ext_end(graph, set));
}

// Provide global definitions of inverse post order iterators...
template <typename T, typename SetType = std::set<typename GraphTraits<T>::NodeRef>,
          bool External = false>
struct ipo_iterator : public PostOrderIterator<Inverse<T>, SetType, External>
{
   ipo_iterator(const PostOrderIterator<Inverse<T>, SetType, External> &iter) :
      PostOrderIterator<Inverse<T>, SetType, External> (iter)
   {}
};

template <typename T>
ipo_iterator<T> ipo_begin(const T &graph)
{
   return ipo_iterator<T>::begin(graph);
}

template <typename T>
ipo_iterator<T> ipo_end(const T &graph)
{
   return ipo_iterator<T>::end(graph);
}

template <typename T>
iterator_range<ipo_iterator<T>> inverse_post_order(const T &graph)
{
   return make_range(ipo_begin(graph), ipo_end(graph));
}

// Provide global definitions of external inverse postorder iterators...
template <typename T, typename SetType = std::set<typename GraphTraits<T>::NodeRef>>
struct ipo_ext_iterator : public ipo_iterator<T, SetType, true>
{
   ipo_ext_iterator(const ipo_iterator<T, SetType, true> &iter) :
      ipo_iterator<T, SetType, true>(iter)
   {}

   ipo_ext_iterator(const PostOrderIterator<Inverse<T>, SetType, true> &iter) :
      ipo_iterator<T, SetType, true>(iter)
   {}
};

template <typename T, typename SetType>
ipo_ext_iterator<T, SetType> ipo_ext_begin(const T &graph, SetType &set)
{
   return ipo_ext_iterator<T, SetType>::begin(graph, set);
}

template <typename T, typename SetType>
ipo_ext_iterator<T, SetType> ipo_ext_end(const T &graph, SetType &set)
{
   return ipo_ext_iterator<T, SetType>::end(graph, set);
}

template <typename T, typename SetType>
IteratorRange<ipo_ext_iterator<T, SetType>>
inverse_post_order_ext(const T &graph, SetType &set)
{
   return make_range(ipo_ext_begin(graph, set), ipo_ext_end(graph, set));
}

//===--------------------------------------------------------------------===//
// Reverse Post Order CFG iterator code
//===--------------------------------------------------------------------===//
//
// This is used to visit basic blocks in a method in reverse post order.  This
// class is awkward to use because I don't know a good incremental algorithm to
// computer RPO from a graph.  Because of this, the construction of the
// ReversePostOrderTraversal object is expensive (it must walk the entire graph
// with a postorder iterator to build the data structures).  The moral of this
// story is: Don't create more ReversePostOrderTraversal classes than necessary.
//
// Because it does the traversal in its constructor, it won't invalidate when
// BasicBlocks are removed, *but* it may contain erased blocks. Some places
// rely on this behavior (i.e. GVN).
//
// This class should be used like this:
// {
//   ReversePostOrderTraversal<Function*> RPOT(FuncPtr); // Expensive to create
//   for (rpo_iterator I = RPOT.begin(); I != RPOT.end(); ++I) {
//      ...
//   }
//   for (rpo_iterator I = RPOT.begin(); I != RPOT.end(); ++I) {
//      ...
//   }
// }
//

template <typename GraphT, typename GT = GraphTraits<GraphT>>
class ReversePostOrderTraversal
{
   using NodeRef = typename GT::NodeRef;

   std::vector<NodeRef> m_blocks; // Block list in normal PO order

   void initialize(NodeRef node)
   {
      std::copy(po_begin(node), po_end(node), std::back_inserter(m_blocks));
   }

public:
   using rpo_iterator = typename std::vector<NodeRef>::reverse_iterator;
   using const_rpo_iterator = typename std::vector<NodeRef>::const_reverse_iterator;

   ReversePostOrderTraversal(GraphT graph)
   {
      initialize(GT::getEntryNode(graph));
   }

   // Because we want a reverse post order, use reverse iterators from the vector
   rpo_iterator begin()
   {
      return m_blocks.rbegin();
   }

   const_rpo_iterator begin() const
   {
      return m_blocks.crbegin();
   }

   rpo_iterator end()
   {
      return m_blocks.rend();
   }

   const_rpo_iterator end() const
   {
      return m_blocks.crend();
   }
};

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_POST_ORDER_ITERATOR_H
