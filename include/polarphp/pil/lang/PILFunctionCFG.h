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
// PIL function as a control-flow graph.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_PIL_PILFUNCTION_CFG_H
#define POLARPHP_PIL_PILFUNCTION_CFG_H

#include "polarphp/pil/lang/PILBasicBlockCFG.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "llvm/ADT/GraphTraits.h"

namespace llvm {

template <>
struct GraphTraits<::polar::PILFunction *>
   : public GraphTraits<polar::PILBasicBlock *> {
   using GraphType = polar::PILFunction *;
   using NodeRef = polar::PILBasicBlock *;

   static NodeRef getEntryNode(GraphType F) { return &F->front(); }

   using nodes_iterator = pointer_iterator<polar::PILFunction::iterator>;
   static nodes_iterator nodes_begin(GraphType F) {
      return nodes_iterator(F->begin());
   }
   static nodes_iterator nodes_end(GraphType F) {
      return nodes_iterator(F->end());
   }
   static unsigned size(GraphType F) { return F->size(); }
};

template <> struct GraphTraits<Inverse<::polar::PILFunction*> >
   : public GraphTraits<Inverse<polar::PILBasicBlock*> > {
   using GraphType = Inverse<polar::PILFunction *>;
   using NodeRef = NodeRef;

   static NodeRef getEntryNode(GraphType F) { return &F.Graph->front(); }

   using nodes_iterator = pointer_iterator<polar::PILFunction::iterator>;
   static nodes_iterator nodes_begin(GraphType F) {
      return nodes_iterator(F.Graph->begin());
   }
   static nodes_iterator nodes_end(GraphType F) {
      return nodes_iterator(F.Graph->end());
   }
   static unsigned size(GraphType F) { return F.Graph->size(); }
};

} // llvm

#endif //POLARPHP_PIL_PILFUNCTION_CFG_H
