//===--- ClosureScope.cpp - Closure Scope Analysis ------------------------===//
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
///
/// Implementation of ClosureScopeAnalysis.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "closure-scope"

#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/optimizer/analysis/ClosureScope.h"

namespace polar {

class ClosureScopeData {
   using IndexRange = ClosureScopeAnalysis::IndexRange;
   using IndexLookupFunc = ClosureScopeAnalysis::IndexLookupFunc;
   using ScopeRange = ClosureScopeAnalysis::ScopeRange;

private:
   // Map an index to each PILFunction with a closure scope.
   std::vector<PILFunction *> indexedScopes;

   // Map each PILFunction with a closure scope to an index.
   llvm::DenseMap<PILFunction *, int> scopeToIndexMap;

   // A list of all indices for the PILFunctions that partially apply this
   // closure. The indices index into the `indexedScopes` vector. If the indexed
   // scope is nullptr, then that function has been deleted.
   using ClosureScopes = llvm::SmallVector<int, 1>;

   // Map each closure to its parent scopes.
   llvm::DenseMap<PILFunction *, ClosureScopes> closureToScopesMap;

public:
   void reset() {
      indexedScopes.clear();
      scopeToIndexMap.clear();
      closureToScopesMap.clear();
   }

   void erase(PILFunction *F) {
      // If this function is a mapped closure scope, remove it, leaving a nullptr
      // sentinel.
      auto indexPos = scopeToIndexMap.find(F);
      if (indexPos != scopeToIndexMap.end()) {
         indexedScopes[indexPos->second] = nullptr;
         scopeToIndexMap.erase(F);
      }
      // If this function is a closure, remove it.
      closureToScopesMap.erase(F);
   }

   // Record all closure scopes in this module.
   void compute(PILModule *M);

   bool isClosureScope(PILFunction *F) { return scopeToIndexMap.count(F); }

   // Return a range of scopes for the given closure. The elements of the
   // returned range have type `PILFunction *` and are non-null. Return an empty
   // range for a PILFunction that is not a closure or is a dead closure.
   ScopeRange getClosureScopes(PILFunction *ClosureF) {
      IndexRange indexRange(nullptr, nullptr);
      auto closureScopesPos = closureToScopesMap.find(ClosureF);
      if (closureScopesPos != closureToScopesMap.end()) {
         auto &indexedScopes = closureScopesPos->second;
         indexRange = IndexRange(indexedScopes.begin(), indexedScopes.end());
      }
      return makeOptionalTransformRange(indexRange,
                                        IndexLookupFunc(indexedScopes));
   }

   void recordScope(PartialApplyInst *PAI) {
      // Only track scopes of non-escaping closures.
      auto closureTy = PAI->getCallee()->getType().castTo<PILFunctionType>();
      // FIXME: isCalleeDynamicallyReplaceable should not be true but can today
      // because local functions can be marked dynamic.
      if (!isNonEscapingClosure(closureTy) ||
          PAI->isCalleeDynamicallyReplaceable())
         return;

      auto closureFunc = PAI->getCalleeFunction();
      assert(closureFunc && "non-escaping closure needs a direct partial_apply.");

      auto scopeFunc = PAI->getFunction();
      int scopeIdx = lookupScopeIndex(scopeFunc);

      // Passes may assume that a deserialized function can only refer to
      // deserialized closures. For example, AccessEnforcementSelection skips
      // deserialized functions but assumes all a closure's parent scope have been
      // processed.
      assert(scopeFunc->wasDeserializedCanonical()
             == closureFunc->wasDeserializedCanonical() &&
             "A closure cannot be serialized in a different module than its "
             "parent context");

      auto &indices = closureToScopesMap[closureFunc];
      if (std::find(indices.begin(), indices.end(), scopeIdx) != indices.end())
         return;

      indices.push_back(scopeIdx);
   }

protected:
   int lookupScopeIndex(PILFunction *scopeFunc) {
      auto indexPos = scopeToIndexMap.find(scopeFunc);
      if (indexPos != scopeToIndexMap.end())
         return indexPos->second;

      int scopeIdx = indexedScopes.size();
      scopeToIndexMap[scopeFunc] = scopeIdx;
      indexedScopes.push_back(scopeFunc);
      return scopeIdx;
   }
};

void ClosureScopeData::compute(PILModule *M) {
   for (auto &F : *M) {
      for (auto &BB : F) {
         for (auto &I : BB) {
            if (auto *PAI = dyn_cast<PartialApplyInst>(&I)) {
               recordScope(PAI);
            }
         }
      }
   }
}

ClosureScopeAnalysis::ClosureScopeAnalysis(PILModule *M)
   : PILAnalysis(PILAnalysisKind::ClosureScope), M(M), scopeData(nullptr) {}

ClosureScopeAnalysis::~ClosureScopeAnalysis() = default;

bool ClosureScopeAnalysis::isClosureScope(PILFunction *scopeFunc) {
   return getOrComputeScopeData()->isClosureScope(scopeFunc);
}

ClosureScopeAnalysis::ScopeRange
ClosureScopeAnalysis::getClosureScopes(PILFunction *closureFunc) {
   return getOrComputeScopeData()->getClosureScopes(closureFunc);
}

void ClosureScopeAnalysis::invalidate() {
   if (scopeData) scopeData->reset();
}

void ClosureScopeAnalysis::notifyWillDeleteFunction(PILFunction *F) {
   if (scopeData) scopeData->erase(F);
}

ClosureScopeData *ClosureScopeAnalysis::getOrComputeScopeData() {
   if (!scopeData) {
      scopeData = std::make_unique<ClosureScopeData>();
      scopeData->compute(M);
   }
   return scopeData.get();
}

PILAnalysis *createClosureScopeAnalysis(PILModule *M) {
   return new ClosureScopeAnalysis(M);
}

void TopDownClosureFunctionOrder::visitFunctions(
   llvm::function_ref<void(PILFunction *)> visitor) {
   auto markVisited = [&](PILFunction *F) {
      bool visitOnce = visited.insert(F).second;
      assert(visitOnce);
      (void)visitOnce;
   };
   auto allScopesVisited = [&](PILFunction *closureF) {
      return llvm::all_of(CSA->getClosureScopes(closureF),
                          [this](PILFunction *F) { return visited.count(F); });
   };
   for (auto &F : *CSA->getModule()) {
      if (!allScopesVisited(&F)) {
         closureWorklist.insert(&F);
         continue;
      }
      markVisited(&F);
      visitor(&F);
   }
   unsigned numClosures = closureWorklist.size();
   while (numClosures) {
      unsigned prevNumClosures = numClosures;
      for (auto &closureNode : closureWorklist) {
         // skip erased closures.
         if (!closureNode)
            continue;

         auto closureF = closureNode.getValue();
         if (!allScopesVisited(closureF))
            continue;

         markVisited(closureF);
         visitor(closureF);
         closureWorklist.erase(closureF);
         --numClosures;
      }
      assert(numClosures < prevNumClosures && "Cyclic closures scopes");
      (void)prevNumClosures;
   }
}

} // namespace polar
