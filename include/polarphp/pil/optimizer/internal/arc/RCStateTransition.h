//===--- RCStateTransition.h ------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_RCSTATETRANSITION_H
#define POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_RCSTATETRANSITION_H

#include "polarphp/basic/ImmutablePointerSet.h"
#include "polarphp/pil/lang/PILArgument.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "llvm/ADT/SmallPtrSet.h"
#include <cstdint>

namespace polar {

class RCIdentityFunctionInfo;
class ConsumedArgToEpilogueReleaseMatcher;

} // end polar namespace

//===----------------------------------------------------------------------===//
//                           RCStateTransitionKind
//===----------------------------------------------------------------------===//

namespace polar {

/// The kind of an RCStateTransition.
enum class RCStateTransitionKind : uint8_t {
#define KIND(K) K,
#define ABSTRACT_VALUE(Name, StartKind, EndKind) \
  Name ## _Start = StartKind, Name ## _End = EndKind,
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"
};

/// \returns the RCStateTransitionKind corresponding to \p N.
RCStateTransitionKind getRCStateTransitionKind(PILNode *N);

/// Define predicates to test for RCStateTransition abstract value kinds.
#define ABSTRACT_VALUE(Name, Start, End)                              \
  bool isRCStateTransition ## Name(RCStateTransitionKind Kind);       \
  static inline bool isRCStateTransition ## Name(PILNode *N) {        \
    return isRCStateTransition ## Name(getRCStateTransitionKind(N));  \
  }
#define KIND(Name)                                                      \
  static inline bool isRCStateTransition ## Name(PILNode *N) {          \
    return RCStateTransitionKind::Name == getRCStateTransitionKind(N);  \
  }
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"

//===----------------------------------------------------------------------===//
//                             RCStateTransition
//===----------------------------------------------------------------------===//

class RefCountState;
class BottomUpRefCountState;
class TopDownRefCountState;

/// Represents a transition in the RC history of a ref count.
class RCStateTransition {
   friend class RefCountState;
   friend class BottomUpRefCountState;
   friend class TopDownRefCountState;

   /// An RCStateTransition can represent either an RC end point (i.e. an initial
   /// or terminal RC transition) or a ptr set of Mutators.
   PILNode *EndPoint;
   ImmutablePointerSet<PILInstruction> *Mutators =
      ImmutablePointerSetFactory<PILInstruction>::getEmptySet();
   RCStateTransitionKind Kind;

   // Should only be constructed be default RefCountState.
   RCStateTransition() = default;

public:
   ~RCStateTransition() = default;
   RCStateTransition(const RCStateTransition &R) = default;

   RCStateTransition(ImmutablePointerSet<PILInstruction> *I) {
      assert(I->size() == 1);
      PILInstruction *Inst = *I->begin();
      Kind = getRCStateTransitionKind(Inst);
      if (isRCStateTransitionEndPoint(Kind)) {
         EndPoint = Inst;
         return;
      }

      if (isRCStateTransitionMutator(Kind)) {
         Mutators = I;
         return;
      }

      // Unknown kind.
   }

   RCStateTransition(PILFunctionArgument *A)
      : EndPoint(A), Kind(RCStateTransitionKind::StrongEntrance) {
      assert(A->hasConvention(PILArgumentConvention::Direct_Owned) &&
             "Expected owned argument");
   }

   RCStateTransitionKind getKind() const { return Kind; }

/// Define test functions for the various abstract categorizations we have.
#define ABSTRACT_VALUE(Name, StartKind, EndKind) bool is ## Name() const;
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"

   /// Return true if this Transition is a mutator transition that contains I.
   bool containsMutator(PILInstruction *I) const {
      assert(isMutator() && "This should only be called if we are of mutator "
                            "kind");
      return Mutators->count(I);
   }

   using mutator_range =
   iterator_range<std::remove_pointer<decltype(Mutators)>::type::iterator>;

   /// Returns a Range of Mutators. Asserts if this transition is not a mutator
   /// transition.
   mutator_range getMutators() const {
      assert(isMutator() && "This should never be called given mutators");
      return {Mutators->begin(), Mutators->end()};
   }

   /// Return true if Inst is an instruction that causes a transition that can be
   /// paired with this transition.
   bool matchingInst(PILInstruction *Inst) const;

   /// Attempt to merge \p Other into \p this. Returns true if we succeeded,
   /// false otherwise.
   bool merge(const RCStateTransition &Other);

   /// Return true if the kind of this RCStateTransition is not 'Invalid'.
   bool isValid() const { return getKind() != RCStateTransitionKind::Invalid; }
};

// These static assert checks are here for performance reasons.
static_assert(std::is_trivially_copyable_v<RCStateTransition>,
              "RCStateTransitions must be trivially copyable");

} // end polar namespace

namespace llvm {
raw_ostream &operator<<(raw_ostream &os, polar::RCStateTransitionKind Kind);
} // end llvm namespace

#endif // POLARPHP_PIL_OPTIMIZER_INTERNAL_ARC_RCSTATETRANSITION_H
