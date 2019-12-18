//===--- RCStateTransition.cpp --------------------------------------------===//
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

#define DEBUG_TYPE "arc-sequence-opts"

#include "polarphp/pil/optimizer/internal/arc/RCStateTransition.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Debug.h"

using namespace polar;

//===----------------------------------------------------------------------===//
//                                  Utility
//===----------------------------------------------------------------------===//

static bool isAutoreleasePoolCall(PILInstruction *I) {
   auto *AI = dyn_cast<ApplyInst>(I);
   if (!AI)
      return false;

   auto *Fn = AI->getReferencedFunctionOrNull();
   if (!Fn)
      return false;

   return llvm::StringSwitch<bool>(Fn->getName())
      .Case("objc_autoreleasePoolPush", true)
      .Case("objc_autoreleasePoolPop", true)
      .Default(false);
}

//===----------------------------------------------------------------------===//
//                           RCStateTransitionKind
//===----------------------------------------------------------------------===//

RCStateTransitionKind polar::getRCStateTransitionKind(PILNode *N) {
   switch (N->getKind()) {
      case PILNodeKind::StrongRetainInst:
      case PILNodeKind::RetainValueInst:
         return RCStateTransitionKind::StrongIncrement;

      case PILNodeKind::StrongReleaseInst:
      case PILNodeKind::ReleaseValueInst:
         return RCStateTransitionKind::StrongDecrement;

      case PILNodeKind::PILFunctionArgument: {
         auto *Arg = cast<PILFunctionArgument>(N);
         if (Arg->hasConvention(PILArgumentConvention::Direct_Owned))
            return RCStateTransitionKind::StrongEntrance;
         return RCStateTransitionKind::Unknown;
      }

      case PILNodeKind::ApplyInst: {
         auto *AI = cast<ApplyInst>(N);
         if (isAutoreleasePoolCall(AI))
            return RCStateTransitionKind::AutoreleasePoolCall;

         // If we have an @owned return value. This AI is a strong entrance for its
         // return value.
         //
         // TODO: When we support pairing retains with @owned parameters, we will
         // need to be able to handle the potential of multiple state transition
         // kinds.
         for (auto result : AI->getSubstCalleeConv().getDirectPILResults()) {
            if (result.getConvention() == ResultConvention::Owned)
               return RCStateTransitionKind::StrongEntrance;
         }

         return RCStateTransitionKind::Unknown;
      }

         // Alloc* are always allocating new classes so they are introducing new
         // values at +1.
      case PILNodeKind::AllocRefInst:
      case PILNodeKind::AllocRefDynamicInst:
      case PILNodeKind::AllocBoxInst:
         return RCStateTransitionKind::StrongEntrance;

      case PILNodeKind::PartialApplyInst:
         // Partial apply boxes are introduced at +1.
         return RCStateTransitionKind::StrongEntrance;

      default:
         return RCStateTransitionKind::Unknown;
   }
}

/// Define test functions for all of our abstract value kinds.
#define ABSTRACT_VALUE(Name, StartKind, EndKind)                           \
  bool polar::isRCStateTransition ## Name(RCStateTransitionKind Kind) {    \
    return unsigned(RCStateTransitionKind::StartKind) <= unsigned(Kind) && \
      unsigned(RCStateTransitionKind::EndKind) >= unsigned(Kind);          \
  }
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"

raw_ostream &llvm::operator<<(raw_ostream &os, RCStateTransitionKind Kind) {
   switch (Kind) {
#define KIND(K)                                 \
  case RCStateTransitionKind::K:                \
    return os << #K;
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"
   }
   llvm_unreachable("Covered switch isn't covered?!");
}

//===----------------------------------------------------------------------===//
//                             RCStateTransition
//===----------------------------------------------------------------------===//

#define ABSTRACT_VALUE(Name, Start, End)            \
  bool RCStateTransition::is ## Name() const {      \
    return isRCStateTransition ## Name(getKind());  \
  }
#include "polarphp/pil/optimizer/internal/arc/RCStateTransitionDef.h"

bool RCStateTransition::matchingInst(PILInstruction *Inst) const {
   // We only pair mutators for now.
   if (!isMutator())
      return false;

   if (Kind == RCStateTransitionKind::StrongIncrement) {
      auto InstTransKind = getRCStateTransitionKind(Inst);
      return InstTransKind == RCStateTransitionKind::StrongDecrement;
   }

   if (Kind == RCStateTransitionKind::StrongDecrement) {
      auto InstTransKind = getRCStateTransitionKind(Inst);
      return InstTransKind == RCStateTransitionKind::StrongIncrement;
   }

   return false;
}

bool RCStateTransition::merge(const RCStateTransition &Other) {
   // If our kinds do not match, bail. We don't cross the streams.
   if (Kind != Other.Kind)
      return false;

   // If we are not a mutator, there is nothing further to do here.
   if (!isMutator())
      return true;

   Mutators = Mutators->merge(Other.Mutators);

   return true;
}

