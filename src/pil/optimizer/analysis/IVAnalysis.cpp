//===--- IVAnalysis.cpp - PIL IV Analysis ---------------------------------===//
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

#include "polarphp/pil/optimizer/analysis/IVAnalysis.h"
#include "polarphp/pil/lang/PatternMatch.h"
#include "polarphp/pil/lang/PILInstruction.h"
#include "polarphp/pil/lang/PILValue.h"

using namespace polar;
using namespace polar::patternmatch;

#if !defined(NDEBUG)
static bool inSCC(ValueBase *value, IVInfo::SCCType &SCC) {
   PILNode *valueNode = value->getRepresentativePILNodeInObject();
   for (PILNode *node : SCC) {
      if (node->getRepresentativePILNodeInObject() == valueNode)
         return true;
   }
   return false;
}
#endif

// For now, we'll consider only the simplest induction variables:
// - Exactly one element in the cycle must be a PILArgument.
// - Only a single increment by a literal.
//
// In other words many valid things that could be considered induction
// variables are disallowed at this point.
PILArgument *IVInfo::isInductionSequence(SCCType &SCC) {
   // Ignore SCCs of size 1 for now. Some of these are derived IVs
   // like i+1 or i*4, which we will eventually want to handle.
   if (SCC.size() == 1)
      return nullptr;

   BuiltinInst *FoundBuiltin = nullptr;
   PILArgument *FoundArgument = nullptr;
   IntegerLiteralInst *IncValue = nullptr;
   for (unsigned long i = 0, e = SCC.size(); i != e; ++i) {
      if (auto IV = dyn_cast<PILArgument>(SCC[i])) {
         if (FoundArgument)
            return nullptr;

         FoundArgument = IV;
         continue;
      }

      // TODO: MultiValueInstruction

      auto *I = cast<PILInstruction>(SCC[i]);
      switch (I->getKind()) {
         case PILInstructionKind::BuiltinInst: {
            if (FoundBuiltin)
               return nullptr;

            FoundBuiltin = cast<BuiltinInst>(I);

            PILValue L, R;
            if (!match(FoundBuiltin, m_ApplyInst(BuiltinValueKind::SAddOver,
                                                 m_PILValue(L), m_PILValue(R))))
               return nullptr;

            if (match(L, m_IntegerLiteralInst(IncValue)))
               std::swap(L, R);

            if (!match(R, m_IntegerLiteralInst(IncValue)))
               return nullptr;
            break;
         }

         case PILInstructionKind::TupleExtractInst: {
            assert(inSCC(cast<TupleExtractInst>(I)->getOperand(), SCC) &&
                   "TupleExtract operand not an induction var");
            break;
         }

         default:
            return nullptr;
      }
   }
   if (!FoundBuiltin || !FoundArgument || !IncValue)
      return nullptr;

   InductionInfoMap[FoundArgument] = IVDesc(FoundBuiltin, IncValue);
   return FoundArgument;
}

void IVInfo::visit(SCCType &SCC) {
   assert(SCC.size() && "SCCs should have an element!!");

   PILArgument *IV;
   if (!(IV = isInductionSequence(SCC)))
      return;

   for (auto node : SCC) {
      if (auto value = dyn_cast<ValueBase>(node))
         InductionVariableMap[value] = IV;
   }
}
