//===--- PILSuccessor.h - Terminator Instruction Successor ------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_PILSUCCESSOR_H
#define POLARPHP_PIL_PILSUCCESSOR_H

#include "polarphp/basic/ProfileCounter.h"
#include <cassert>
#include <cstddef>
#include <iterator>

namespace polar {

class PILBasicBlock;
class TermInst;

using polar::ProfileCounter;

/// An edge in the control flow graph.
///
/// A PILSuccessor is stored in the terminator instruction of the tail block of
/// the CFG edge. Internally it has a back reference to the terminator that
/// contains it (ContainingInst). It also contains the SuccessorBlock that is
/// the "head" of the CFG edge. This makes it very simple to iterate over the
/// successors of a specific block.
///
/// PILSuccessor also enables given a "head" edge the ability to iterate over
/// predecessors. This is done by using an ilist that is embedded into
/// PILSuccessors.
class PILSuccessor {
  /// The terminator instruction that contains this PILSuccessor.
  TermInst *ContainingInst = nullptr;

  /// If non-null, this is the BasicBlock that the terminator branches to.
  PILBasicBlock *SuccessorBlock = nullptr;

  /// If hasValue, this is the profiled execution count of the edge
  ProfileCounter Count;

  /// A pointer to the PILSuccessor that represents the previous PILSuccessor in the
  /// predecessor list for SuccessorBlock.
  ///
  /// Must be nullptr if SuccessorBlock is.
  PILSuccessor **Prev = nullptr;

  /// A pointer to the PILSuccessor that represents the next PILSuccessor in the
  /// predecessor list for SuccessorBlock.
  ///
  /// Must be nullptr if SuccessorBlock is.
  PILSuccessor *Next = nullptr;

public:
  PILSuccessor(ProfileCounter Count = ProfileCounter()) : Count(Count) {}

  PILSuccessor(TermInst *CI, ProfileCounter Count = ProfileCounter())
      : ContainingInst(CI), Count(Count) {}

  PILSuccessor(TermInst *CI, PILBasicBlock *Succ,
               ProfileCounter Count = ProfileCounter())
      : ContainingInst(CI), Count(Count) {
    *this = Succ;
  }

  ~PILSuccessor() {
    *this = nullptr;
  }

  void operator=(PILBasicBlock *BB);

  operator PILBasicBlock*() const { return SuccessorBlock; }
  PILBasicBlock *getBB() const { return SuccessorBlock; }

  ProfileCounter getCount() const { return Count; }

  // Do not copy or move these.
    /// @todo
//  PILSuccessor(const PILSuccessor &) = delete;
//  PILSuccessor(PILSuccessor &&) = delete;

  /// This is an iterator for walking the predecessor list of a PILBasicBlock.
  class pred_iterator {
    PILSuccessor *Cur;

    // Cache the basic block to avoid repeated pointer chasing.
    PILBasicBlock *Block;

    void cacheBasicBlock();

  public:
    using difference_type = std::ptrdiff_t;
    using value_type = PILBasicBlock *;
    using pointer = PILBasicBlock **;
    using reference = PILBasicBlock *&;
    using iterator_category = std::forward_iterator_tag;

    pred_iterator(PILSuccessor *Cur = nullptr) : Cur(Cur), Block(nullptr) {
      cacheBasicBlock();
    }

    bool operator==(pred_iterator I2) const { return Cur == I2.Cur; }
    bool operator!=(pred_iterator I2) const { return Cur != I2.Cur; }

    pred_iterator &operator++() {
      assert(Cur != nullptr);
      Cur = Cur->Next;
      cacheBasicBlock();
      return *this;
    }

    pred_iterator operator++(int) {
      auto old = *this;
      ++*this;
      return old;
    }

    pred_iterator operator+(unsigned distance) const {
      auto copy = *this;
      if (distance == 0)
        return copy;
      do {
        copy.Cur = Cur->Next;
      } while (--distance > 0);
      copy.cacheBasicBlock();
      return copy;
    }

    PILSuccessor *getSuccessorRef() const { return Cur; }
    PILBasicBlock *operator*() const { return Block; }
  };
};

} // end polar namespace

#endif
