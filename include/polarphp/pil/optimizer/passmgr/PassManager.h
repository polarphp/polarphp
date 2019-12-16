//===--- PassManager.h  - Swift Pass Manager --------------------*- C++ -*-===//
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

#include "polarphp/pil/lang/Notifications.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/passmgr/PassPipeline.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"
#include <vector>

#ifndef POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PASSMANAGER_H
#define POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PASSMANAGER_H

namespace polar {

class PILFunction;
class PILFunctionTransform;
class PILModule;
class PILModuleTransform;
class PILOptions;
class PILTransform;

namespace irgen {
class IRGenModule;
}

/// The PIL pass manager.
class PILPassManager {
   /// The module that the pass manager will transform.
   PILModule *Mod;

   /// An optional IRGenModule associated with this PassManager.
   irgen::IRGenModule *IRMod;

   /// The list of transformations to run.
   llvm::SmallVector<PILTransform *, 16> Transformations;

   /// A list of registered analysis.
   llvm::SmallVector<PILAnalysis *, 16> Analyses;

   /// An entry in the FunctionWorkList.
   struct WorklistEntry {
      WorklistEntry(PILFunction *F) : F(F) { }

      PILFunction *F;

      /// The current position in the transform-list.
      unsigned PipelineIdx = 0;

      /// How many times the pipeline was restarted for the function.
      unsigned NumRestarts = 0;
   };

   /// The worklist of functions to be processed by function passes.
   std::vector<WorklistEntry> FunctionWorklist;

   // Name of the current optimization stage for diagnostics.
   std::string StageName;

   /// The number of passes run so far.
   unsigned NumPassesRun = 0;

   /// A mask which has one bit for each pass. A one for a pass-bit means that
   /// the pass doesn't need to run, because nothing has changed since the
   /// previous run of that pass.
   typedef std::bitset<(size_t)PassKind::AllPasses_Last + 1> CompletedPasses;

   /// A completed-passes mask for each function.
   llvm::DenseMap<PILFunction *, CompletedPasses> CompletedPassesMap;

   /// Stores for each function the number of levels of specializations it is
   /// derived from an original function. E.g. if a function is a signature
   /// optimized specialization of a generic specialization, it has level 2.
   /// This is used to avoid an infinite amount of functions pushed on the
   /// worklist (e.g. caused by a bug in a specializing optimization).
   llvm::DenseMap<PILFunction *, int> DerivationLevels;

   /// Set to true when a pass invalidates an analysis.
   bool CurrentPassHasInvalidated = false;

   /// True if we need to stop running passes and restart again on the
   /// same function.
   bool RestartPipeline = false;

   /// If true, passes are also run for functions which have
   /// OptimizationMode::NoOptimization.
   bool isMandatory = false;

   /// The IRGen PIL passes. These have to be dynamically added by IRGen.
   llvm::DenseMap<unsigned, PILTransform *> IRGenPasses;

   /// The notification handler for this specific PILPassManager.
   ///
   /// This is not owned by the pass manager, it is owned by the PILModule which
   /// is guaranteed to outlive any pass manager associated with it. We keep this
   /// bare pointer to ensure that we can deregister the notification after this
   /// pass manager is destroyed.
   DeserializationNotificationHandler *deserializationNotificationHandler;

public:
   /// C'tor. It creates and registers all analysis passes, which are defined
   /// in Analysis.def.
   ///
   /// If \p isMandatory is true, passes are also run for functions
   /// which have OptimizationMode::NoOptimization.
   PILPassManager(PILModule *M, llvm::StringRef Stage = "",
                  bool isMandatory = false);

   /// C'tor. It creates an IRGen pass manager. Passes can query for the
   /// IRGenModule.
   PILPassManager(PILModule *M, irgen::IRGenModule *IRMod,
                  llvm::StringRef Stage = "", bool isMandatory = false);

   const PILOptions &getOptions() const;

   /// Searches for an analysis of type T in the list of registered
   /// analysis. If the analysis is not found, the program terminates.
   template<typename T>
   T *getAnalysis() {
      for (PILAnalysis *A : Analyses)
         if (auto *R = llvm::dyn_cast<T>(A))
            return R;

      llvm_unreachable("Unable to find analysis for requested type.");
   }

   /// \returns the module that the pass manager owns.
   PILModule *getModule() { return Mod; }

   /// \returns the associated IGenModule or null if this is not an IRGen
   /// pass manager.
   irgen::IRGenModule *getIRGenModule() { return IRMod; }

   /// Restart the function pass pipeline on the same function
   /// that is currently being processed.
   void restartWithCurrentFunction(PILTransform *T);
   void clearRestartPipeline() { RestartPipeline = false; }
   bool shouldRestartPipeline() { return RestartPipeline; }

   /// Iterate over all analysis and invalidate them.
   void invalidateAllAnalysis() {
      // Invalidate the analysis (unless they are locked)
      for (auto AP : Analyses)
         if (!AP->isLocked())
            AP->invalidate();

      CurrentPassHasInvalidated = true;

      // Assume that all functions have changed. Clear all masks of all functions.
      CompletedPassesMap.clear();
   }

   /// Notify the pass manager of a newly create function for tracing.
   void notifyOfNewFunction(PILFunction *F, PILTransform *T);

   /// Add the function \p F to the function pass worklist.
   /// If not null, the function \p DerivedFrom is the function from which \p F
   /// is derived. This is used to avoid an infinite amount of functions pushed
   /// on the worklist (e.g. caused by a bug in a specializing optimization).
   void addFunctionToWorklist(PILFunction *F, PILFunction *DerivedFrom);

   /// Iterate over all analysis and notify them of the function.
   ///
   /// This function does not necessarily have to be newly created function. It
   /// is the job of the analysis to make sure no extra work is done if the
   /// particular analysis has been done on the function.
   void notifyAnalysisOfFunction(PILFunction *F) {
      for (auto AP : Analyses) {
         AP->notifyAddedOrModifiedFunction(F);
      }
   }

   /// Broadcast the invalidation of the function to all analysis.
   void invalidateAnalysis(PILFunction *F,
                           PILAnalysis::InvalidationKind K) {
      // Invalidate the analysis (unless they are locked)
      for (auto AP : Analyses)
         if (!AP->isLocked())
            AP->invalidate(F, K);

      CurrentPassHasInvalidated = true;
      // Any change let all passes run again.
      CompletedPassesMap[F].reset();
   }

   /// Iterate over all analysis and notify them of a change in witness-
   /// or vtables.
   void invalidateFunctionTables() {
      // Invalidate the analysis (unless they are locked)
      for (auto AP : Analyses)
         if (!AP->isLocked())
            AP->invalidateFunctionTables();

      CurrentPassHasInvalidated = true;

      // Assume that all functions have changed. Clear all masks of all functions.
      CompletedPassesMap.clear();
   }

   /// Iterate over all analysis and notify them of a deleted function.
   void notifyWillDeleteFunction(PILFunction *F) {
      // Invalidate the analysis (unless they are locked)
      for (auto AP : Analyses)
         if (!AP->isLocked())
            AP->notifyWillDeleteFunction(F);

      CurrentPassHasInvalidated = true;
      // Any change let all passes run again.
      CompletedPassesMap[F].reset();
   }

   /// Reset the state of the pass manager and remove all transformation
   /// owned by the pass manager. Analysis passes will be kept.
   void resetAndRemoveTransformations();

   /// Set the name of the current optimization stage.
   ///
   /// This is useful for debugging.
   void setStageName(llvm::StringRef NextStage = "");

   /// Get the name of the current optimization stage.
   ///
   /// This is useful for debugging.
   StringRef getStageName() const;

   /// D'tor.
   ~PILPassManager();

   /// Verify all analyses.
   void verifyAnalyses() const {
      for (auto *A : Analyses) {
         A->verify();
      }
   }

   /// Verify all analyses, limiting the verification to just this one function
   /// if possible.
   ///
   /// Discussion: We leave it up to the analyses to decide how to implement
   /// this. If no override is provided the PILAnalysis should just call the
   /// normal verify method.
   void verifyAnalyses(PILFunction *F) const {
      for (auto *A : Analyses) {
         A->verify(F);
      }
   }

   void executePassPipelinePlan(const PILPassPipelinePlan &Plan) {
      for (const PILPassPipeline &Pipeline : Plan.getPipelines()) {
         setStageName(Pipeline.Name);
         resetAndRemoveTransformations();
         for (PassKind Kind : Plan.getPipelinePasses(Pipeline)) {
            addPass(Kind);
         }
         execute();
      }
   }

   void registerIRGenPass(PassKind Kind, PILTransform *Transform) {
      assert(IRGenPasses.find(unsigned(Kind)) == IRGenPasses.end() &&
             "Pass already registered");
      assert(
         IRMod &&
         "Attempting to register an IRGen pass with a non-IRGen pass manager");
      IRGenPasses[unsigned(Kind)] = Transform;
   }

private:
   void execute();

   /// Add a pass of a specific kind.
   void addPass(PassKind Kind);

   /// Add a pass with a given name.
   void addPassForName(StringRef Name);

   /// Run the \p TransIdx'th PIL module transform over all the functions in
   /// the module.
   void runModulePass(unsigned TransIdx);

   /// Run the \p TransIdx'th pass on the function \p F.
   void runPassOnFunction(unsigned TransIdx, PILFunction *F);

   /// Run the passes in Transform from \p FromTransIdx to \p ToTransIdx.
   void runFunctionPasses(unsigned FromTransIdx, unsigned ToTransIdx);

   /// A helper function that returns (based on PIL stage and debug
   /// options) whether we should continue running passes.
   bool continueTransforming();

   /// Return true if all analyses are unlocked.
   bool analysesUnlocked();

   /// Dumps information about the pass with index \p TransIdx to llvm::dbgs().
   void dumpPassInfo(const char *Title, PILTransform *Tr, PILFunction *F);

   /// Dumps information about the pass with index \p TransIdx to llvm::dbgs().
   void dumpPassInfo(const char *Title, unsigned TransIdx,
                     PILFunction *F = nullptr);

   /// Displays the call graph in an external dot-viewer.
   /// This function is meant for use from the debugger.
   /// When asserts are disabled, this is a NoOp.
   void viewCallGraph();
};

} // end namespace polar

#endif // POLARPHP_PIL_OPTIMIZER_PASSMANAGER_PASSMANAGER_H
