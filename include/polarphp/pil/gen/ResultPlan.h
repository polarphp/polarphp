//===--- ResultPlan.h -------------------------------------------*- C++ -*-===//
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

#ifndef POLARPHP_PIL_GEN_RESULTPLAN_H
#define POLARPHP_PIL_GEN_RESULTPLAN_H

#include "polarphp/pil/gen/Callee.h"
#include "polarphp/pil/gen/ManagedValue.h"
#include "polarphp/ast/Types.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/pil/lang/PILLocation.h"
#include <memory>

namespace polar {

class CanType;
class PILValue;

namespace lowering {

class AbstractionPattern;
class Initialization;
class RValue;
class PILGenFunction;
class SGFContext;
class CalleeTypeInfo;

/// An abstract class for working with results.of applies.
class ResultPlan {
public:
   virtual RValue finish(PILGenFunction &SGF, PILLocation loc, CanType substType,
                         ArrayRef<ManagedValue> &directResults) = 0;
   virtual ~ResultPlan() = default;

   virtual void
   gatherIndirectResultAddrs(PILGenFunction &SGF, PILLocation loc,
                             SmallVectorImpl<PILValue> &outList) const = 0;

   virtual Optional<std::pair<ManagedValue, ManagedValue>>
   emitForeignErrorArgument(PILGenFunction &SGF, PILLocation loc) {
      return None;
   }
};

using ResultPlanPtr = std::unique_ptr<ResultPlan>;

/// The class for building result plans.
struct ResultPlanBuilder {
   PILGenFunction &SGF;
   PILLocation loc;
   const CalleeTypeInfo &calleeTypeInfo;

   /// A list of all of the results that we are tracking in reverse order. The
   /// reason that it is in reverse order is to allow us to simply traverse the
   /// list by popping values off the back.
   SmallVector<PILResultInfo, 8> allResults;

   ResultPlanBuilder(PILGenFunction &SGF, PILLocation loc,
                     const CalleeTypeInfo &calleeTypeInfo)
      : SGF(SGF), loc(loc), calleeTypeInfo(calleeTypeInfo),
      // We reverse the order so we can pop values off the back.
        allResults(llvm::reverse(calleeTypeInfo.substFnType->getResults())) {}

   ResultPlanPtr build(Initialization *emitInto, AbstractionPattern origType,
                       CanType substType);
   ResultPlanPtr buildForTuple(Initialization *emitInto,
                               AbstractionPattern origType,
                               CanTupleType substType);

   static ResultPlanPtr computeResultPlan(PILGenFunction &SGF,
                                          const CalleeTypeInfo &calleeTypeInfo,
                                          PILLocation loc,
                                          SGFContext evalContext);

   ~ResultPlanBuilder() {
      assert(allResults.empty() && "didn't consume all results!");
   }

private:
   ResultPlanPtr buildTopLevelResult(Initialization *init, PILLocation loc);
};

} // end namespace lowering
} // end namespace polar

#endif
