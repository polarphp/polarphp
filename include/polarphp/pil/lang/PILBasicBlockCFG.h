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

#ifndef POLARPHP_PIL_PILBASIC_BLOCK_CFG_H
#define POLARPHP_PIL_PILBASIC_BLOCK_CFG_H

#include "polarphp/pil/lang/PILBasicBlock.h"
#include "llvm/ADT/GraphTraits.h"

namespace llvm {

//===----------------------------------------------------------------------===//
// GraphTraits for PILBasicBlock
//===----------------------------------------------------------------------===//

template <> struct GraphTraits<::polar::PILBasicBlock *> {
   using ChildIteratorType = polar::PILBasicBlock::succblock_iterator;
   using Node = polar::PILBasicBlock;
   using NodeRef = Node *;

   static NodeRef getEntryNode(NodeRef BB) { return BB; }

   static ChildIteratorType child_begin(NodeRef N) {
      return N->succblock_begin();
   }
   static ChildIteratorType child_end(NodeRef N) { return N->succblock_end(); }
};

template <> struct GraphTraits<const ::polar::PILBasicBlock*> {
   using ChildIteratorType = polar::PILBasicBlock::const_succblock_iterator;
   using Node = const polar::PILBasicBlock;
   using NodeRef = Node *;

   static NodeRef getEntryNode(NodeRef BB) { return BB; }

   static ChildIteratorType child_begin(NodeRef N) {
      return N->succblock_begin();
   }
   static ChildIteratorType child_end(NodeRef N) { return N->succblock_end(); }
};

template <> struct GraphTraits<Inverse<::polar::PILBasicBlock*> > {
   using ChildIteratorType = polar::PILBasicBlock::pred_iterator;
   using Node = polar::PILBasicBlock;
   using NodeRef = Node *;
   static NodeRef getEntryNode(Inverse<polar::PILBasicBlock *> G) {
      return G.Graph;
   }
   static inline ChildIteratorType child_begin(NodeRef N) {
      return N->pred_begin();
   }
   static inline ChildIteratorType child_end(NodeRef N) {
      return N->pred_end();
   }
};

template <> struct GraphTraits<Inverse<const ::polar::PILBasicBlock*> > {
   using ChildIteratorType = polar::PILBasicBlock::pred_iterator;
   using Node = const polar::PILBasicBlock;
   using NodeRef = Node *;

   static NodeRef getEntryNode(Inverse<const polar::PILBasicBlock *> G) {
      return G.Graph;
   }
   static inline ChildIteratorType child_begin(NodeRef N) {
      return N->pred_begin();
   }
   static inline ChildIteratorType child_end(NodeRef N) {
      return N->pred_end();
   }
};

} // end llvm namespace

#endif // POLARPHP_PIL_PILBASIC_BLOCK_CFG_H

