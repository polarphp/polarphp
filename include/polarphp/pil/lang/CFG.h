//===--- CFG.h - PIL control-flow graph utilities ---------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
//
// This file provides basic declarations and utilities for working with
// PIL basic blocks as a control-flow graph.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_CFG_H
#define POLARPHP_PIL_CFG_H

#include "polarphp/pil/lang/PILFunction.h"
#include "llvm/ADT/GraphTraits.h"

namespace llvm {

//===----------------------------------------------------------------------===//
// GraphTraits for PILBasicBlock
//===----------------------------------------------------------------------===//

template <> struct GraphTraits<::polar::pil::PILBasicBlock *> {
   using ChildIteratorType = polar::pil::PILBasicBlock::succblock_iterator;
   using Node = polar::pil::PILBasicBlock;
   using NodeRef = Node *;

   static NodeRef getEntryNode(NodeRef BB) { return BB; }

   static ChildIteratorType child_begin(NodeRef N) {
      return N->succblock_begin();
   }
   static ChildIteratorType child_end(NodeRef N) { return N->succblock_end(); }
};

template <> struct GraphTraits<const ::polar::pil::PILBasicBlock*> {
   using ChildIteratorType = polar::pil::PILBasicBlock::const_succblock_iterator;
   using Node = const polar::pil::PILBasicBlock;
   using NodeRef = Node *;

   static NodeRef getEntryNode(NodeRef BB) { return BB; }

   static ChildIteratorType child_begin(NodeRef N) {
      return N->succblock_begin();
   }
   static ChildIteratorType child_end(NodeRef N) { return N->succblock_end(); }
};

template <> struct GraphTraits<Inverse<::polar::pil::PILBasicBlock*> > {
   using ChildIteratorType = polar::pil::PILBasicBlock::pred_iterator;
   using Node = polar::pil::PILBasicBlock;
   using NodeRef = Node *;
   static NodeRef getEntryNode(Inverse<polar::pil::PILBasicBlock *> G) {
      return G.Graph;
   }
   static inline ChildIteratorType child_begin(NodeRef N) {
      return N->pred_begin();
   }
   static inline ChildIteratorType child_end(NodeRef N) {
      return N->pred_end();
   }
};

template <> struct GraphTraits<Inverse<const ::polar::pil::PILBasicBlock*> > {
   using ChildIteratorType = polar::pil::PILBasicBlock::pred_iterator;
   using Node = const polar::pil::PILBasicBlock;
   using NodeRef = Node *;

   static NodeRef getEntryNode(Inverse<const polar::pil::PILBasicBlock *> G) {
      return G.Graph;
   }
   static inline ChildIteratorType child_begin(NodeRef N) {
      return N->pred_begin();
   }
   static inline ChildIteratorType child_end(NodeRef N) {
      return N->pred_end();
   }
};

template <>
struct GraphTraits<::polar::pil::PILFunction *>
   : public GraphTraits<polar::pil::PILBasicBlock *> {
   using GraphType = polar::pil::PILFunction *;
   using NodeRef = polar::pil::PILBasicBlock *;

   static NodeRef getEntryNode(GraphType F) { return &F->front(); }

   using nodes_iterator = pointer_iterator<polar::pil::PILFunction::iterator>;
   static nodes_iterator nodes_begin(GraphType F) {
      return nodes_iterator(F->begin());
   }
   static nodes_iterator nodes_end(GraphType F) {
      return nodes_iterator(F->end());
   }
   static unsigned size(GraphType F) { return F->size(); }
};

template <> struct GraphTraits<Inverse<::polar::pil::PILFunction*> >
   : public GraphTraits<Inverse<polar::pil::PILBasicBlock*> > {
   using GraphType = Inverse<polar::pil::PILFunction *>;
   using NodeRef = NodeRef;

   static NodeRef getEntryNode(GraphType F) { return &F.Graph->front(); }

   using nodes_iterator = pointer_iterator<polar::pil::PILFunction::iterator>;
   static nodes_iterator nodes_begin(GraphType F) {
      return nodes_iterator(F.Graph->begin());
   }
   static nodes_iterator nodes_end(GraphType F) {
      return nodes_iterator(F.Graph->end());
   }
   static unsigned size(GraphType F) { return F.Graph->size(); }
};

} // end llvm namespace

#endif

