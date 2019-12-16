//===--- PostOrder.h --------------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_POSTORDER_H
#define POLARPHP_PIL_POSTORDER_H

#include "polarphp/basic/Range.h"
#include "polarphp/pil/lang/PILBasicBlockCFG.h"
#include "polarphp/pil/lang/PILFunctionCFG.h"
#include "polarphp/pil/lang/PILBasicBlock.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/optimizer/Analysis/analysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/iterator_range.h"
#include <vector>

namespace polar {

class PostOrderFunctionInfo {
   std::vector<PILBasicBlock *> PostOrder;
   llvm::DenseMap<PILBasicBlock *, unsigned> BBToPOMap;

public:
   PostOrderFunctionInfo(PILFunction *F) {
      for (auto *BB : make_range(po_begin(F), po_end(F))) {
         BBToPOMap[BB] = PostOrder.size();
         PostOrder.push_back(BB);
      }
   }

   using iterator = decltype(PostOrder)::iterator;
   using const_iterator = decltype(PostOrder)::const_iterator;
   using reverse_iterator = decltype(PostOrder)::reverse_iterator;
   using const_reverse_iterator = decltype(PostOrder)::const_reverse_iterator;

   using range = iterator_range<iterator>;
   using const_range = iterator_range<const_iterator>;
   using reverse_range = iterator_range<reverse_iterator>;
   using const_reverse_range = iterator_range<const_reverse_iterator>;

   range getPostOrder() {
      return make_range(PostOrder.begin(), PostOrder.end());
   }
   const_range getPostOrder() const {
      return make_range(PostOrder.begin(), PostOrder.end());
   }
   reverse_range getReversePostOrder() {
      return make_range(PostOrder.rbegin(), PostOrder.rend());
   }
   const_reverse_range getReversePostOrder() const {
      return make_range(PostOrder.rbegin(), PostOrder.rend());
   }

   const_reverse_range getReversePostOrder(PILBasicBlock *StartBlock) const {
      unsigned RPONumber = getRPONumber(StartBlock).getValue();
      return getReversePostOrder(RPONumber);
   }

   const_reverse_range getReversePostOrder(unsigned RPONumber) const {
      return make_range(std::next(PostOrder.rbegin(), RPONumber),
                        PostOrder.rend());
   }

   unsigned size() const { return PostOrder.size(); }

   Optional<unsigned> getPONumber(PILBasicBlock *BB) const {
      auto Iter = BBToPOMap.find(BB);
      if (Iter != BBToPOMap.end())
         return Iter->second;
      return None;
   }

   Optional<unsigned> getRPONumber(PILBasicBlock *BB) const {
      auto Iter = BBToPOMap.find(BB);
      if (Iter != BBToPOMap.end())
         return PostOrder.size() - Iter->second - 1;
      return None;
   }
};

} // end polar namespace

#endif
