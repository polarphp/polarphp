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

#ifndef POLARPHP_BASIC_ADT_GRAPH_TRAITS_H
#define POLARPHP_BASIC_ADT_GRAPH_TRAITS_H

#include "polarphp/basic/adt/IteratorRange.h"

namespace polar {
namespace basic {

// GraphTraits - This class should be specialized by different graph types...
// which is why the default version is empty.
//
// This template evolved from supporting `BasicBlock` to also later supporting
// more complex types (e.g. CFG and DomTree).
//
// GraphTraits can be used to create a view over a graph interpreting it
// differently without requiring a copy of the original graph. This could
// be achieved by carrying more data in NodeRef. See LoopBodyTraits for one
// example.
template<typename GraphType>
struct GraphTraits
{
   // Elements to provide:

   // typedef NodeRef           - Type of Node token in the graph, which should
   //                             be cheap to copy.
   // typedef ChildIteratorType - Type used to iterate over children in graph,
   //                             dereference to a NodeRef.

   // static NodeRef getEntryNode(const GraphType &)
   //    Return the entry node of the graph

   // static ChildIteratorType child_begin(NodeRef)
   // static ChildIteratorType child_end  (NodeRef)
   //    Return iterators that point to the beginning and ending of the child
   //    node list for the specified node.

   // typedef  ...iterator nodes_iterator; - dereference to a NodeRef
   // static nodes_iterator nodes_begin(GraphType *G)
   // static nodes_iterator nodes_end  (GraphType *G)
   //    nodes_iterator/begin/end - Allow iteration over all nodes in the graph

   // static unsigned       size       (GraphType *G)
   //    Return total number of nodes in the graph

   // If anyone tries to use this class without having an appropriate
   // specialization, make an error.  If you get this error, it's because you
   // need to include the appropriate specialization of GraphTraits<> for your
   // graph, or you need to define it for a new graph type. Either that or
   // your argument to XXX_begin(...) is unknown or needs to have the proper .h
   // file #include'd.
   using NodeRef = typename GraphType::UnknownGraphTypeError;
};

// Inverse - This class is used as a little marker class to tell the graph
// iterator to iterate over the graph in a graph defined "Inverse" ordering.
// Not all graphs define an inverse ordering, and if they do, it depends on
// the graph exactly what that is.  Here's an example of usage with the
// df_iterator:
//
// idf_iterator<Method*> I = idf_begin(M), E = idf_end(M);
// for (; I != E; ++I) { ... }
//
// Which is equivalent to:
// df_iterator<Inverse<Method*>> I = idf_begin(M), E = idf_end(M);
// for (; I != E; ++I) { ... }
//
template <typename GraphType>
struct Inverse
{
   const GraphType &m_graph;

   inline Inverse(const GraphType &graph) : m_graph(graph)
   {}
};

// Provide a partial specialization of GraphTraits so that the inverse of an
// inverse falls back to the original graph.
template <typename T>
struct GraphTraits<Inverse<Inverse<T>>> : GraphTraits<T>
{};

// Provide iterator ranges for the graph traits nodes and children
template <typename GraphType>
IteratorRange<typename GraphTraits<GraphType>::nodes_iterator>
nodes(const GraphType &graph)
{
   return make_range(GraphTraits<GraphType>::nodes_begin(graph),
                     GraphTraits<GraphType>::nodes_end(graph));
}
template <typename GraphType>
IteratorRange<typename GraphTraits<Inverse<GraphType>>::nodes_iterator>
inverse_nodes(const GraphType &graph)
{
   return make_range(GraphTraits<Inverse<GraphType>>::nodes_begin(graph),
                     GraphTraits<Inverse<GraphType>>::nodes_end(graph));
}

template <typename GraphType>
IteratorRange<typename GraphTraits<GraphType>::ChildIteratorType>
children(const typename GraphTraits<GraphType>::NodeRef &graph)
{
   return make_range(GraphTraits<GraphType>::child_begin(graph),
                     GraphTraits<GraphType>::child_end(graph));
}

template <typename GraphType>
IteratorRange<typename GraphTraits<Inverse<GraphType>>::ChildIteratorType>
inverse_children(const typename GraphTraits<GraphType>::NodeRef &graph)
{
   return make_range(GraphTraits<Inverse<GraphType>>::child_begin(graph),
                     GraphTraits<Inverse<GraphType>>::child_end(graph));
}

template <typename GraphType>
IteratorRange<typename GraphTraits<GraphType>::ChildEdgeIteratorType>
children_edges(const typename GraphTraits<GraphType>::NodeRef &graph)
{
   return make_range(GraphTraits<GraphType>::child_edge_begin(graph),
                     GraphTraits<GraphType>::child_edge_end(graph));
}

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_GRAPH_TRAITS_H
