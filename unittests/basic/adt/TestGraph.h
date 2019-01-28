// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/07.

#ifndef POLARPHP_UNITTESTS_ADT_TEST_GRAPH_H
#define POLARPHP_UNITTESTS_ADT_TEST_GRAPH_H

#include "polarphp/basic/adt/GraphTraits.h"
#include <cassert>
#include <climits>
#include <utility>

namespace polar {
namespace basic {

/// Graph<N> - A graph with N nodes.  Note that N can be at most 8.
template <unsigned N>
class Graph
{
private:
   // Disable copying.
   Graph(const Graph&);
   Graph& operator=(const Graph&);

   static void validateIndex(unsigned idx)
   {
      assert(idx < N && "Invalid node index!");
   }
public:

   /// NodeSubset - A subset of the graph's nodes.
   class NodeSubset
   {
      typedef unsigned char BitVector; // Where the limitation N <= 8 comes from.
      BitVector Elements;
      NodeSubset(BitVector e) : Elements(e) {}
   public:
      /// NodeSubset - Default constructor, creates an empty subset.
      NodeSubset() : Elements(0)
      {
         assert(N <= sizeof(BitVector)*CHAR_BIT && "Graph too big!");
      }

      /// Comparison operators.
      bool operator==(const NodeSubset &other) const
      {
         return other.Elements == this->Elements;
      }
      bool operator!=(const NodeSubset &other) const
      {
         return !(*this == other);
      }

      /// addNode - Add the node with the given index to the subset.
      void addNode(unsigned idx)
      {
         validateIndex(idx);
         Elements |= 1U << idx;
      }

      /// deleteNode - Remove the node with the given index from the subset.
      void deleteNode(unsigned idx)
      {
         validateIndex(idx);
         Elements &= ~(1U << idx);
      }

      /// count - Return true if the node with the given index is in the subset.
      bool count(unsigned idx)
      {
         validateIndex(idx);
         return (Elements & (1U << idx)) != 0;
      }

      /// isEmpty - Return true if this is the empty set.
      bool isEmpty() const
      {
         return Elements == 0;
      }

      /// isSubsetOf - Return true if this set is a subset of the given one.
      bool isSubsetOf(const NodeSubset &other) const
      {
         return (this->Elements | other.Elements) == other.Elements;
      }

      /// complement - Return the complement of this subset.
      NodeSubset complement() const
      {
         return ~(unsigned)this->Elements & ((1U << N) - 1);
      }

      /// join - Return the union of this subset and the given one.
      NodeSubset join(const NodeSubset &other) const
      {
         return this->Elements | other.Elements;
      }

      /// meet - Return the intersection of this subset and the given one.
      NodeSubset meet(const NodeSubset &other) const
      {
         return this->Elements & other.Elements;
      }
   };

   /// NodeType - Node index and set of children of the node.
   typedef std::pair<unsigned, NodeSubset> NodeType;

private:
   /// Nodes - The list of nodes for this graph.
   NodeType Nodes[N];
public:

   /// Graph - Default constructor.  Creates an empty graph.
   Graph()
   {
      // Let each node know which node it is.  This allows us to find the start of
      // the Nodes array given a pointer to any element of it.
      for (unsigned i = 0; i != N; ++i)
         Nodes[i].first = i;
   }

   /// addEdge - Add an edge from the node with index FromIdx to the node with
   /// index ToIdx.
   void addEdge(unsigned FromIdx, unsigned ToIdx)
   {
      validateIndex(FromIdx);
      Nodes[FromIdx].second.addNode(ToIdx);
   }

   /// deleteEdge - Remove the edge (if any) from the node with index FromIdx to
   /// the node with index ToIdx.
   void deleteEdge(unsigned FromIdx, unsigned ToIdx)
   {
      validateIndex(FromIdx);
      Nodes[FromIdx].second.deleteNode(ToIdx);
   }

   /// accessNode - Get a pointer to the node with the given index.
   NodeType *accessNode(unsigned idx) const
   {
      validateIndex(idx);
      // The constant cast is needed when working with GraphTraits, which insists
      // on taking a constant Graph.
      return const_cast<NodeType *>(&Nodes[idx]);
   }

   /// nodesReachableFrom - Return the set of all nodes reachable from the given
   /// node.
   NodeSubset nodesReachableFrom(unsigned idx) const
   {
      // This algorithm doesn't scale, but that doesn't matter given the small
      // size of our graphs.
      NodeSubset Reachable;

      // The initial node is reachable.
      Reachable.addNode(idx);
      do {
         NodeSubset Previous(Reachable);

         // Add in all nodes which are children of a reachable node.
         for (unsigned i = 0; i != N; ++i)
            if (Previous.count(i))
               Reachable = Reachable.join(Nodes[i].second);

         // If nothing changed then we have found all reachable nodes.
         if (Reachable == Previous)
            return Reachable;

         // Rinse and repeat.
      } while (1);
   }

   /// ChildIterator - Visit all children of a node.
   class ChildIterator
   {
      friend class Graph;

      /// FirstNode - Pointer to first node in the graph's Nodes array.
      NodeType *FirstNode;
      /// Children - Set of nodes which are children of this one and that haven't
      /// yet been visited.
      NodeSubset Children;

      ChildIterator(); // Disable default constructor.
   protected:
      ChildIterator(NodeType *F, NodeSubset C) : FirstNode(F), Children(C) {}

   public:
      /// ChildIterator - Copy constructor.
      ChildIterator(const ChildIterator& other) : FirstNode(other.FirstNode),
         Children(other.Children) {}

      /// Comparison operators.
      bool operator==(const ChildIterator &other) const
      {
         return other.FirstNode == this->FirstNode &&
               other.Children == this->Children;
      }

      bool operator!=(const ChildIterator &other) const
      {
         return !(*this == other);
      }

      /// Prefix increment operator.
      ChildIterator& operator++()
      {
         // Find the next unvisited child node.
         for (unsigned i = 0; i != N; ++i)
            if (Children.count(i)) {
               // Remove that child - it has been visited.  This is the increment!
               Children.deleteNode(i);
               return *this;
            }
         assert(false && "Incrementing end iterator!");
         return *this; // Avoid compiler warnings.
      }

      /// Postfix increment operator.
      ChildIterator operator++(int)
      {
         ChildIterator Result(*this);
         ++(*this);
         return Result;
      }

      /// Dereference operator.
      NodeType *operator*()
      {
         // Find the next unvisited child node.
         for (unsigned i = 0; i != N; ++i)
            if (Children.count(i))
               // Return a pointer to it.
               return FirstNode + i;
         assert(false && "Dereferencing end iterator!");
         return nullptr; // Avoid compiler warning.
      }
   };

   /// childBegin - Return an iterator pointing to the first child of the given
   /// node.
   static ChildIterator childBegin(NodeType *parent)
   {
      return ChildIterator(parent - parent->first, parent->second);
   }

   /// childEnd - Return the end iterator for children of the given node.
   static ChildIterator childEnd(NodeType *parent)
   {
      return ChildIterator(parent - parent->first, NodeSubset());
   }
};

template <unsigned N>
struct GraphTraits<Graph<N>>
{
   typedef typename Graph<N>::NodeType *NodeRef;
   typedef typename Graph<N>::ChildIterator ChildIteratorType;

   static NodeRef getEntryNode(const Graph<N> &G) { return G.accessNode(0); }
   static ChildIteratorType childBegin(NodeRef Node) {
      return Graph<N>::childBegin(Node);
   }
   static ChildIteratorType childEnd(NodeRef Node)
   {
      return Graph<N>::childEnd(Node);
   }
};

} // basic
} // polar

#endif
