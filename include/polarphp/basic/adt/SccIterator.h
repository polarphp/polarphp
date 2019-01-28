// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/26.

#ifndef POLARPHP_BASIC_ADT_SCCITERATOR_H
#define POLARPHP_BASIC_ADT_SCCITERATOR_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/GraphTraits.h"
#include "polarphp/basic/adt/Iterator.h"
#include "polarphp/utils/Debug.h"
#include <cassert>
#include <cstddef>
#include <iterator>
#include <vector>

namespace polar {
namespace basic {

using polar::debug_stream;

/// \brief Enumerate the SCCs of a directed graph in reverse topological order
/// of the SCC DAG.
///
/// This is implemented using Tarjan's DFS algorithm using an internal stack to
/// build up a vector of nodes in a particular SCC. Note that it is a forward
/// iterator and thus you cannot backtrack or re-visit nodes.
template <typename  GraphT, typename GT = GraphTraits<GraphT>>
class SccIterator : public IteratorFacadeBase<
      SccIterator<GraphT, GT>, std::forward_iterator_tag,
      const std::vector<typename GT::NodeRef>, ptrdiff_t>
{
   using NodeRef = typename GT::NodeRef;
   using ChildItTy = typename GT::ChildIteratorType;
   using SccTy = std::vector<NodeRef>;
   using reference = typename SccIterator::reference;

   /// Element of m_visitStack during DFS.
   struct StackElement
   {
      NodeRef m_node;         ///< The current node pointer.
      ChildItTy m_nextChild;  ///< The next child, modified inplace during DFS.
      unsigned m_minVisited;  ///< Minimum uplink value of all children of m_node.

      StackElement(NodeRef node, const ChildItTy &child, unsigned min)
         : m_node(node), m_nextChild(child), m_minVisited(min)
      {}

      bool operator==(const StackElement &other) const
      {
         return m_node == other.m_node &&
               m_nextChild == other.m_nextChild &&
               m_minVisited == other.m_minVisited;
      }
   };

   /// The visit counters used to detect when a complete SCC is on the stack.
   /// m_visitNum is the global counter.
   ///
   /// m_nodeVisitNumbers are per-node visit numbers, also used as DFS flags.
   unsigned m_visitNum;
   DenseMap<NodeRef, unsigned> m_nodeVisitNumbers;

   /// Stack holding nodes of the SCC.
   std::vector<NodeRef> m_sccNodeStack;

   /// The current SCC, retrieved using operator*().
   SccTy m_currentScc;

   /// DFS stack, Used to maintain the ordering.  The top contains the current
   /// node, the next child to visit, and the minimum uplink value of all child
   std::vector<StackElement> m_visitStack;

   /// A single "visit" within the non-recursive DFS traversal.
   void dfsVisitOne(NodeRef node);

   /// The stack-based DFS traversal; defined below.
   void dfsVisitChildren();

   /// Compute the next SCC using the DFS traversal.
   void getNextScc();

   SccIterator(NodeRef entryN) : m_visitNum(0)
   {
      dfsVisitOne(entryN);
      getNextScc();
   }

   /// End is when the DFS stack is empty.
   SccIterator() = default;

public:
   static SccIterator begin(const GraphT &graph)
   {
      return SccIterator(GT::getEntryNode(graph));
   }

   static SccIterator end(const GraphT &)
   {
      return SccIterator();
   }

   /// \brief Direct loop termination test which is more efficient than
   /// comparison with \c end().
   bool isAtEnd() const
   {
      assert(!m_currentScc.empty() || m_visitStack.empty());
      return m_currentScc.empty();
   }

   bool operator==(const SccIterator &x) const
   {
      return m_visitStack == x.m_visitStack && m_currentScc == x.m_currentScc;
   }

   SccIterator &operator++()
   {
      getNextScc();
      return *this;
   }

   reference operator*() const
   {
      assert(!m_currentScc.empty() && "Dereferencing END SCC iterator!");
      return m_currentScc;
   }

   /// \brief Test if the current SCC has a loop.
   ///
   /// If the SCC has more than one node, this is trivially true.  If not, it may
   /// still contain a loop if the node has an edge back to itself.
   bool hasLoop() const;

   /// This informs the \c SccIterator that the specified \c oldNode node
   /// has been deleted, and \c newNode is to be used in its place.
   void replaceNode(NodeRef oldNode, NodeRef newNode)
   {
      assert(m_nodeVisitNumbers.count(oldNode) && "oldNode not in SccIterator?");
      m_nodeVisitNumbers[newNode] = m_nodeVisitNumbers[oldNode];
      m_nodeVisitNumbers.erase(oldNode);
   }
};

template <typename  GraphT, typename GT>
void SccIterator<GraphT, GT>::dfsVisitOne(NodeRef node)
{
   ++m_visitNum;
   m_nodeVisitNumbers[node] = m_visitNum;
   m_sccNodeStack.push_back(node);
   m_visitStack.push_back(StackElement(node, GT::childBegin(node), m_visitNum));
#if 0 // Enable if needed when debugging.
   dbgs() << "TarjanSCC: m_node " << node <<
             " : m_visitNum = " << m_visitNum << "\n";
#endif
}

template <typename  GraphT, typename GT>
void SccIterator<GraphT, GT>::dfsVisitChildren()
{
   assert(!m_visitStack.empty());
   while (m_visitStack.back().m_nextChild != GT::childEnd(m_visitStack.back().m_node)) {
      // TOS has at least one more child so continue DFS
      NodeRef childN = *m_visitStack.back().m_nextChild++;
      typename DenseMap<NodeRef, unsigned>::iterator Visited =
            m_nodeVisitNumbers.find(childN);
      if (Visited == m_nodeVisitNumbers.end()) {
         // this node has never been seen.
         dfsVisitOne(childN);
         continue;
      }

      unsigned childNum = Visited->second;
      if (m_visitStack.back().m_minVisited > childNum) {
         m_visitStack.back().m_minVisited = childNum;
      }
   }
}

template <typename  GraphT, typename GT>
void SccIterator<GraphT, GT>::getNextScc()
{
   m_currentScc.clear(); // Prepare to compute the next SCC
   while (!m_visitStack.empty()) {
      dfsVisitChildren();

      // Pop the leaf on top of the m_visitStack.
      NodeRef visitingN = m_visitStack.back().m_node;
      unsigned minVisitNum = m_visitStack.back().m_minVisited;
      assert(m_visitStack.back().m_nextChild == GT::childEnd(visitingN));
      m_visitStack.pop_back();

      // Propagate MinVisitNum to parent so we can detect the SCC starting node.
      if (!m_visitStack.empty() && m_visitStack.back().m_minVisited > minVisitNum) {
         m_visitStack.back().m_minVisited = minVisitNum;
      }
#if 0 // Enable if needed when debugging.
      debug_stream() << "TarjanSCC: Popped node " << visitingN <<
                        " : minVisitNum = " << minVisitNum << "; m_node visit num = " <<
                        m_nodeVisitNumbers[visitingN] << "\n";
#endif
      if (minVisitNum != m_nodeVisitNumbers[visitingN]) {
         continue;
      }
      // A full SCC is on the m_sccNodeStack!  It includes all nodes below
      // visitingN on the stack.  Copy those nodes to m_currentScc,
      // reset their minVisit values, and return (this suspends
      // the DFS traversal till the next ++).
      do {
         m_currentScc.push_back(m_sccNodeStack.back());
         m_sccNodeStack.pop_back();
         m_nodeVisitNumbers[m_currentScc.back()] = ~0U;
      } while (m_currentScc.back() != visitingN);
      return;
   }
}

template <typename  GraphT, typename GT>
bool SccIterator<GraphT, GT>::hasLoop() const
{
   assert(!m_currentScc.empty() && "Dereferencing END SCC iterator!");
   if (m_currentScc.size() > 1)
      return true;
   NodeRef node = m_currentScc.front();
   for (ChildItTy citer = GT::childBegin(node), cend = GT::childEnd(node); citer != cend;
        ++citer) {
      if (*citer == node) {
         return true;
      }
   }
   return false;
}

/// \brief Construct the begin iterator for a deduced graph type T.
template <typename T>
SccIterator<T> scc_begin(const T &graph)
{
   return SccIterator<T>::begin(graph);
}

/// \brief Construct the end iterator for a deduced graph type T.
template <typename T>
SccIterator<T> scc_end(const T &graph)
{
   return SccIterator<T>::end(graph);
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_SCCITERATOR_H
