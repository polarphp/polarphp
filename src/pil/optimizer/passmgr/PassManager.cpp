//===--- PassManager.cpp - Swift Pass Manager -----------------------------===//
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

#define DEBUG_TYPE "pil-passmanager"

#include "polarphp/pil/optimizer/passmgr/PassManager.h"
#include "polarphp/demangling/Demangle.h"
#include "polarphp/pil/lang/ApplySite.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/optimizer/analysis/BasicCalleeAnalysis.h"
#include "polarphp/pil/optimizer/analysis/FunctionOrder.h"
#include "polarphp/pil/optimizer/passmgr/PrettyStackTrace.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/OptimizerStatsUtils.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/Chrono.h"

using namespace polar;

llvm::cl::opt<bool> PILPrintAll(
   "sil-print-all", llvm::cl::init(false),
   llvm::cl::desc("Print PIL after each pass"));

llvm::cl::opt<bool> PILPrintPassName(
   "sil-print-pass-name", llvm::cl::init(false),
   llvm::cl::desc("Print the name of each PIL pass before it runs"));

llvm::cl::opt<bool> PILPrintPassTime(
   "sil-print-pass-time", llvm::cl::init(false),
   llvm::cl::desc("Print the execution time of each PIL pass"));

llvm::cl::opt<unsigned> PILNumOptPassesToRun(
   "sil-opt-pass-count", llvm::cl::init(UINT_MAX),
   llvm::cl::desc("Stop optimizing after <N> optimization passes"));

llvm::cl::opt<std::string> PILBreakOnFun(
   "sil-break-on-function", llvm::cl::init(""),
   llvm::cl::desc(
      "Break before running each function pass on a particular function"));

llvm::cl::opt<std::string> PILBreakOnPass(
   "sil-break-on-pass", llvm::cl::init(""),
   llvm::cl::desc("Break before running a particular function pass"));

llvm::cl::opt<std::string>
   PILPrintOnlyFun("sil-print-only-function", llvm::cl::init(""),
                   llvm::cl::desc("Only print out the sil for this function"));

llvm::cl::opt<std::string>
   PILPrintOnlyFuns("sil-print-only-functions", llvm::cl::init(""),
                    llvm::cl::desc("Only print out the sil for the functions "
                                   "whose name contains this substring"));

llvm::cl::list<std::string>
   PILPrintBefore("sil-print-before",
                  llvm::cl::desc("Print out the sil before passes which "
                                 "contain a string from this list."));

llvm::cl::list<std::string>
   PILPrintAfter("sil-print-after",
                 llvm::cl::desc("Print out the sil after passes which contain "
                                "a string from this list."));

llvm::cl::list<std::string>
   PILPrintAround("sil-print-around",
                  llvm::cl::desc("Print out the sil before and after passes "
                                 "which contain a string from this list"));

llvm::cl::list<std::string>
   PILDisablePass("sil-disable-pass",
                  llvm::cl::desc("Disable passes "
                                 "which contain a string from this list"));

llvm::cl::list<std::string> PILVerifyBeforePass(
   "sil-verify-before-pass",
   llvm::cl::desc("Verify the module/analyses before we run "
                  "a pass from this list"));

llvm::cl::list<std::string> PILVerifyAroundPass(
   "sil-verify-around-pass",
   llvm::cl::desc("Verify the module/analyses before/after we run "
                  "a pass from this list"));

llvm::cl::list<std::string>
   PILVerifyAfterPass("sil-verify-after-pass",
                      llvm::cl::desc("Verify the module/analyses after we run "
                                     "a pass from this list"));

llvm::cl::opt<bool> PILVerifyWithoutInvalidation(
   "sil-verify-without-invalidation", llvm::cl::init(false),
   llvm::cl::desc("Verify after passes even if the pass has not invalidated"));

llvm::cl::opt<bool> PILDisableSkippingPasses(
   "sil-disable-skipping-passes", llvm::cl::init(false),
   llvm::cl::desc("Do not skip passes even if nothing was changed"));

static llvm::ManagedStatic<std::vector<unsigned>> DebugPassNumbers;

namespace {

struct DebugOnlyPassNumberOpt {
   void operator=(const std::string &Val) const {
      if (Val.empty())
         return;
      SmallVector<StringRef, 8> dbgPassNumbers;
      StringRef(Val).split(dbgPassNumbers, ',', -1, false);
      for (auto dbgPassNumber : dbgPassNumbers) {
         int PassNumber;
         if (dbgPassNumber.getAsInteger(10, PassNumber) || PassNumber < 0)
            llvm_unreachable("The pass number should be an integer number >= 0");
         DebugPassNumbers->push_back(static_cast<unsigned>(PassNumber));
      }
   }
};

} // end anonymous namespace

static DebugOnlyPassNumberOpt DebugOnlyPassNumberOptLoc;

static llvm::cl::opt<DebugOnlyPassNumberOpt, true,
   llvm::cl::parser<std::string>>
   DebugOnly("debug-only-pass-number",
             llvm::cl::desc("Enable a specific type of debug output (comma "
                            "separated list pass numbers)"),
             llvm::cl::Hidden, llvm::cl::ZeroOrMore,
             llvm::cl::value_desc("pass number"),
             llvm::cl::location(DebugOnlyPassNumberOptLoc),
             llvm::cl::ValueRequired);

static bool doPrintBefore(PILTransform *T, PILFunction *F) {
   if (!PILPrintOnlyFun.empty() && F && F->getName() != PILPrintOnlyFun)
      return false;

   if (!PILPrintOnlyFuns.empty() && F &&
       F->getName().find(PILPrintOnlyFuns, 0) == StringRef::npos)
      return false;

   auto MatchFun = [&](const std::string &Str) -> bool {
      return T->getTag().find(Str) != StringRef::npos
             || T->getID().find(Str) != StringRef::npos;
   };

   if (PILPrintBefore.end() !=
       std::find_if(PILPrintBefore.begin(), PILPrintBefore.end(), MatchFun))
      return true;

   if (PILPrintAround.end() !=
       std::find_if(PILPrintAround.begin(), PILPrintAround.end(), MatchFun))
      return true;

   return false;
}

static bool doPrintAfter(PILTransform *T, PILFunction *F, bool Default) {
   if (!PILPrintOnlyFun.empty() && F && F->getName() != PILPrintOnlyFun)
      return false;

   if (!PILPrintOnlyFuns.empty() && F &&
       F->getName().find(PILPrintOnlyFuns, 0) == StringRef::npos)
      return false;

   auto MatchFun = [&](const std::string &Str) -> bool {
      return T->getTag().find(Str) != StringRef::npos
             || T->getID().find(Str) != StringRef::npos;
   };

   if (PILPrintAfter.end() !=
       std::find_if(PILPrintAfter.begin(), PILPrintAfter.end(), MatchFun))
      return true;

   if (PILPrintAround.end() !=
       std::find_if(PILPrintAround.begin(), PILPrintAround.end(), MatchFun))
      return true;

   return Default;
}

static bool isDisabled(PILTransform *T) {
   for (const std::string &NamePattern : PILDisablePass) {
      if (T->getTag().find(NamePattern) != StringRef::npos
          || T->getID().find(NamePattern) != StringRef::npos) {
         return true;
      }
   }
   return false;
}

static void printModule(PILModule *Mod, bool EmitVerbosePIL) {
   if (PILPrintOnlyFun.empty() && PILPrintOnlyFuns.empty()) {
      Mod->dump();
      return;
   }
   for (auto &F : *Mod) {
      if (!PILPrintOnlyFun.empty() && F.getName().str() == PILPrintOnlyFun)
         F.dump(EmitVerbosePIL);

      if (!PILPrintOnlyFuns.empty() &&
          F.getName().find(PILPrintOnlyFuns, 0) != StringRef::npos)
         F.dump(EmitVerbosePIL);
   }
}

class DebugPrintEnabler {
#ifndef NDEBUG
   bool OldDebugFlag;
#endif
public:
   DebugPrintEnabler(unsigned PassNumber) {
#ifndef NDEBUG
      OldDebugFlag = llvm::DebugFlag;
      if (llvm::DebugFlag)
         return;
      if (DebugPassNumbers->empty())
         return;
      // Enable debug printing if the pass number matches
      // one of the pass numbers provided as a command line option.
      for (auto DebugPassNumber : *DebugPassNumbers) {
         if (DebugPassNumber == PassNumber) {
            llvm::DebugFlag = true;
            return;
         }
      }
#endif
   }

   ~DebugPrintEnabler() {
#ifndef NDEBUG
      llvm::DebugFlag = OldDebugFlag;
#endif
   }
};

//===----------------------------------------------------------------------===//
//                 Serialization Notification Implementation
//===----------------------------------------------------------------------===//

namespace {

class PassManagerDeserializationNotificationHandler final
   : public DeserializationNotificationHandler {
   NullablePtr<PILPassManager> pm;

public:
   PassManagerDeserializationNotificationHandler(PILPassManager *pm) : pm(pm) {}
   ~PassManagerDeserializationNotificationHandler() override = default;

   StringRef getName() const override {
      return "PassManagerDeserializationNotificationHandler";
   }

   /// Observe that we deserialized a function declaration.
   void didDeserialize(ModuleDecl *mod, PILFunction *fn) override {
      pm.get()->notifyAnalysisOfFunction(fn);
   }
};

} // end anonymous namespace

PILPassManager::PILPassManager(PILModule *M, llvm::StringRef Stage,
                               bool isMandatory)
   : Mod(M), StageName(Stage), isMandatory(isMandatory),
     deserializationNotificationHandler(nullptr) {
#define ANALYSIS(NAME) \
  Analyses.push_back(create##NAME##Analysis(Mod));
#include "polarphp/pil/optimizer/Analysis/AnalysisDef.h"

   for (PILAnalysis *A : Analyses) {
      A->initialize(this);
      M->registerDeleteNotificationHandler(A);
   }

   std::unique_ptr<DeserializationNotificationHandler> handler(
      new PassManagerDeserializationNotificationHandler(this));
   deserializationNotificationHandler = handler.get();
   M->registerDeserializationNotificationHandler(std::move(handler));
}

PILPassManager::PILPassManager(PILModule *M, irgen::IRGenModule *IRMod,
                               llvm::StringRef Stage, bool isMandatory)
   : PILPassManager(M, Stage, isMandatory) {
   this->IRMod = IRMod;
}

bool PILPassManager::continueTransforming() {
   if (isMandatory)
      return true;
   return NumPassesRun < PILNumOptPassesToRun;
}

bool PILPassManager::analysesUnlocked() {
   for (auto *A : Analyses)
      if (A->isLocked())
         return false;

   return true;
}

// Test the function and pass names we're given against the debug
// options that force us to break prior to a given pass and/or on a
// given function.
static bool breakBeforeRunning(StringRef fnName, PILFunctionTransform *SFT) {
   if (PILBreakOnFun.empty() && PILBreakOnPass.empty())
      return false;

   if (PILBreakOnFun.empty()
       && (SFT->getID() == PILBreakOnPass || SFT->getTag() == PILBreakOnPass))
      return true;

   if (PILBreakOnPass.empty() && fnName == PILBreakOnFun)
      return true;

   return fnName == PILBreakOnFun
          && (SFT->getID() == PILBreakOnPass || SFT->getTag() == PILBreakOnPass);
}

void PILPassManager::dumpPassInfo(const char *Title, PILTransform *Tr,
                                  PILFunction *F) {
   llvm::dbgs() << "  " << Title << " #" << NumPassesRun << ", stage "
                << StageName << ", pass : " << Tr->getID()
                << " (" << Tr->getTag() << ")";
   if (F)
      llvm::dbgs() << ", Function: " << F->getName();
   llvm::dbgs() << '\n';
}

void PILPassManager::dumpPassInfo(const char *Title, unsigned TransIdx,
                                  PILFunction *F) {
   PILTransform *Tr = Transformations[TransIdx];
   llvm::dbgs() << "  " << Title << " #" << NumPassesRun << ", stage "
                << StageName << ", pass " << TransIdx << ": " << Tr->getID()
                << " (" << Tr->getTag() << ")";
   if (F)
      llvm::dbgs() << ", Function: " << F->getName();
   llvm::dbgs() << '\n';
}

void PILPassManager::runPassOnFunction(unsigned TransIdx, PILFunction *F) {

   assert(analysesUnlocked() && "Expected all analyses to be unlocked!");

   auto *SFT = cast<PILFunctionTransform>(Transformations[TransIdx]);
   SFT->injectPassManager(this);
   SFT->injectFunction(F);

   PrettyStackTracePILFunctionTransform X(SFT, NumPassesRun);
   DebugPrintEnabler DebugPrint(NumPassesRun);

   // If nothing changed since the last run of this pass, we can skip this
   // pass.
   CompletedPasses &completedPasses = CompletedPassesMap[F];
   if (completedPasses.test((size_t)SFT->getPassKind()) &&
       !PILDisableSkippingPasses) {
      if (PILPrintPassName)
         dumpPassInfo("(Skip)", TransIdx, F);
      return;
   }

   if (isDisabled(SFT)) {
      if (PILPrintPassName)
         dumpPassInfo("(Disabled)", TransIdx, F);
      return;
   }

   updatePILModuleStatsBeforeTransform(F->getModule(), SFT, *this, NumPassesRun);

   CurrentPassHasInvalidated = false;

   auto MatchFun = [&](const std::string &Str) -> bool {
      return SFT->getTag().find(Str) != StringRef::npos ||
             SFT->getID().find(Str) != StringRef::npos;
   };
   if ((PILVerifyBeforePass.end() != std::find_if(PILVerifyBeforePass.begin(),
                                                  PILVerifyBeforePass.end(),
                                                  MatchFun)) ||
       (PILVerifyAroundPass.end() != std::find_if(PILVerifyAroundPass.begin(),
                                                  PILVerifyAroundPass.end(),
                                                  MatchFun))) {
      F->verify();
      verifyAnalyses();
   }

   if (PILPrintPassName)
      dumpPassInfo("Run", TransIdx, F);

   if (doPrintBefore(SFT, F)) {
      dumpPassInfo("*** PIL function before ", TransIdx);
      F->dump(getOptions().EmitVerbosePIL);
   }

   llvm::sys::TimePoint<> StartTime = std::chrono::system_clock::now();
   Mod->registerDeleteNotificationHandler(SFT);
   if (breakBeforeRunning(F->getName(), SFT))
      LLVM_BUILTIN_DEBUGTRAP;
   SFT->run();
   assert(analysesUnlocked() && "Expected all analyses to be unlocked!");
   Mod->removeDeleteNotificationHandler(SFT);

   auto Delta = (std::chrono::system_clock::now() - StartTime).count();
   if (PILPrintPassTime) {
      llvm::dbgs() << Delta << " (" << SFT->getID() << "," << F->getName()
                   << ")\n";
   }

   // If this pass invalidated anything, print and verify.
   if (doPrintAfter(SFT, F, CurrentPassHasInvalidated && PILPrintAll)) {
      dumpPassInfo("*** PIL function after ", TransIdx);
      F->dump(getOptions().EmitVerbosePIL);
   }

   updatePILModuleStatsAfterTransform(F->getModule(), SFT, *this, NumPassesRun,
                                      Delta);

   // Remember if this pass didn't change anything.
   if (!CurrentPassHasInvalidated)
      completedPasses.set((size_t)SFT->getPassKind());

   if (getOptions().VerifyAll &&
       (CurrentPassHasInvalidated || PILVerifyWithoutInvalidation)) {
      F->verify();
      verifyAnalyses(F);
   } else {
      if ((PILVerifyAfterPass.end() != std::find_if(PILVerifyAfterPass.begin(),
                                                    PILVerifyAfterPass.end(),
                                                    MatchFun)) ||
          (PILVerifyAroundPass.end() != std::find_if(PILVerifyAroundPass.begin(),
                                                     PILVerifyAroundPass.end(),
                                                     MatchFun))) {
         F->verify();
         verifyAnalyses();
      }
   }

   ++NumPassesRun;
}

void PILPassManager::
runFunctionPasses(unsigned FromTransIdx, unsigned ToTransIdx) {
   if (ToTransIdx <= FromTransIdx)
      return;

   BasicCalleeAnalysis *BCA = getAnalysis<BasicCalleeAnalysis>();
   BottomUpFunctionOrder BottomUpOrder(*Mod, BCA);
   auto BottomUpFunctions = BottomUpOrder.getFunctions();

   assert(FunctionWorklist.empty() && "Expected empty function worklist!");

   FunctionWorklist.reserve(BottomUpFunctions.size());
   for (auto I = BottomUpFunctions.rbegin(), E = BottomUpFunctions.rend();
        I != E; ++I) {
      auto &F = **I;

      // Only include functions that are definitions, and which have not
      // been intentionally excluded from optimization.
      if (F.isDefinition() && (isMandatory || F.shouldOptimize()))
         FunctionWorklist.push_back(*I);
   }

   DerivationLevels.clear();

   // The maximum number of times the pass pipeline can be restarted for a
   // function. This is used to ensure we are not going into an infinite loop in
   // cases where (for example) we have recursive type-based specialization
   // happening.
   const unsigned MaxNumRestarts = 20;

   if (PILPrintPassName)
      llvm::dbgs() << "Start function passes at stage: " << StageName << "\n";

   // Run all transforms for all functions, starting at the tail of the worklist.
   while (!FunctionWorklist.empty() && continueTransforming()) {
      unsigned TailIdx = FunctionWorklist.size() - 1;
      unsigned PipelineIdx = FunctionWorklist[TailIdx].PipelineIdx;
      PILFunction *F = FunctionWorklist[TailIdx].F;

      if (PipelineIdx >= (ToTransIdx - FromTransIdx)) {
         // All passes did already run for the function. Pop it off the worklist.
         FunctionWorklist.pop_back();
         continue;
      }
      assert(!shouldRestartPipeline() &&
             "Did not expect function pipeline set up to restart from beginning!");

      runPassOnFunction(FromTransIdx + PipelineIdx, F);

      // Note: Don't get entry reference prior to runPassOnFunction().
      // A pass can push a new function to the worklist which may cause a
      // reallocation of the buffer and that would invalidate the reference.
      WorklistEntry &Entry = FunctionWorklist[TailIdx];
      if (shouldRestartPipeline() && Entry.NumRestarts < MaxNumRestarts) {
         ++Entry.NumRestarts;
         Entry.PipelineIdx = 0;
      } else {
         ++Entry.PipelineIdx;
      }
      clearRestartPipeline();
   }
}

void PILPassManager::runModulePass(unsigned TransIdx) {
   auto *SMT = cast<PILModuleTransform>(Transformations[TransIdx]);
   if (isDisabled(SMT))
      return;

   const PILOptions &Options = getOptions();

   SMT->injectPassManager(this);
   SMT->injectModule(Mod);

   PrettyStackTracePILModuleTransform X(SMT, NumPassesRun);
   DebugPrintEnabler DebugPrint(NumPassesRun);

   updatePILModuleStatsBeforeTransform(*Mod, SMT, *this, NumPassesRun);

   CurrentPassHasInvalidated = false;

   if (PILPrintPassName)
      dumpPassInfo("Run module pass", TransIdx);

   if (doPrintBefore(SMT, nullptr)) {
      dumpPassInfo("*** PIL module before", TransIdx);
      printModule(Mod, Options.EmitVerbosePIL);
   }

   auto MatchFun = [&](const std::string &Str) -> bool {
      return SMT->getTag().find(Str) != StringRef::npos ||
             SMT->getID().find(Str) != StringRef::npos;
   };
   if ((PILVerifyBeforePass.end() != std::find_if(PILVerifyBeforePass.begin(),
                                                  PILVerifyBeforePass.end(),
                                                  MatchFun)) ||
       (PILVerifyAroundPass.end() != std::find_if(PILVerifyAroundPass.begin(),
                                                  PILVerifyAroundPass.end(),
                                                  MatchFun))) {
      Mod->verify();
      verifyAnalyses();
   }

   llvm::sys::TimePoint<> StartTime = std::chrono::system_clock::now();
   assert(analysesUnlocked() && "Expected all analyses to be unlocked!");
   Mod->registerDeleteNotificationHandler(SMT);
   SMT->run();
   Mod->removeDeleteNotificationHandler(SMT);
   assert(analysesUnlocked() && "Expected all analyses to be unlocked!");

   auto Delta = (std::chrono::system_clock::now() - StartTime).count();
   if (PILPrintPassTime) {
      llvm::dbgs() << Delta << " (" << SMT->getID() << ",Module)\n";
   }

   // If this pass invalidated anything, print and verify.
   if (doPrintAfter(SMT, nullptr,
                    CurrentPassHasInvalidated && PILPrintAll)) {
      dumpPassInfo("*** PIL module after", TransIdx);
      printModule(Mod, Options.EmitVerbosePIL);
   }

   updatePILModuleStatsAfterTransform(*Mod, SMT, *this, NumPassesRun, Delta);

   if (Options.VerifyAll &&
       (CurrentPassHasInvalidated || !PILVerifyWithoutInvalidation)) {
      Mod->verify();
      verifyAnalyses();
   } else {
      if ((PILVerifyAfterPass.end() != std::find_if(PILVerifyAfterPass.begin(),
                                                    PILVerifyAfterPass.end(),
                                                    MatchFun)) ||
          (PILVerifyAroundPass.end() != std::find_if(PILVerifyAroundPass.begin(),
                                                     PILVerifyAroundPass.end(),
                                                     MatchFun))) {
         Mod->verify();
         verifyAnalyses();
      }
   }
}

void PILPassManager::execute() {
   const PILOptions &Options = getOptions();

   LLVM_DEBUG(llvm::dbgs() << "*** Optimizing the module (" << StageName
                           << ") *** \n");
   if (PILPrintAll) {
      llvm::dbgs() << "*** PIL module before "  << StageName << " ***\n";
      printModule(Mod, Options.EmitVerbosePIL);
   }

   // Run the transforms by alternating between function transforms and
   // module transforms. We'll queue up all the function transforms
   // that we see in a row and then run the entire group of transforms
   // on each function in turn. Then we move on to running the next set
   // of consecutive module transforms.
   unsigned Idx = 0, NumTransforms = Transformations.size();

   while (Idx < NumTransforms && continueTransforming()) {
      PILTransform *Tr = Transformations[Idx];
      assert((isa<PILFunctionTransform>(Tr) || isa<PILModuleTransform>(Tr)) &&
             "Unexpected pass kind!");
      (void)Tr;

      unsigned FirstFuncTrans = Idx;
      while (Idx < NumTransforms && isa<PILFunctionTransform>(Transformations[Idx]))
         ++Idx;

      runFunctionPasses(FirstFuncTrans, Idx);

      while (Idx < NumTransforms && isa<PILModuleTransform>(Transformations[Idx])
             && continueTransforming()) {
         runModulePass(Idx);

         ++Idx;
         ++NumPassesRun;
      }
   }
}

/// D'tor.
PILPassManager::~PILPassManager() {
   assert(IRGenPasses.empty() && "Must add IRGen PIL passes that were "
                                 "registered to the list of transformations");
   // Before we do anything further, verify the module and our analyses. These
   // are natural points with which to verify.
   //
   // TODO: We currently do not verify the module here since the verifier asserts
   // in the normal build. This should be enabled and those problems resolved
   // either by changing the verifier or treating those asserts as signs of a
   // bug.
   for (auto *A : Analyses) {
      // We use verify full instead of just verify to ensure that passes that want
      // to run more expensive verification after a pass manager is destroyed
      // properly trigger.
      //
      // NOTE: verifyFull() has a default implementation that just calls
      // verify(). So functionally, there is no difference here.
      A->verifyFull();
   }

   // Remove our deserialization notification handler.
   Mod->removeDeserializationNotificationHandler(
      deserializationNotificationHandler);

   // Free all transformations.
   for (auto *T : Transformations)
      delete T;

   // delete the analysis.
   for (auto *A : Analyses) {
      Mod->removeDeleteNotificationHandler(A);
      assert(!A->isLocked() &&
             "Deleting a locked analysis. Did we forget to unlock ?");
      delete A;
   }
}

void PILPassManager::notifyOfNewFunction(PILFunction *F, PILTransform *T) {
   if (doPrintAfter(T, F, PILPrintAll)) {
      dumpPassInfo("*** New PIL function in ", T, F);
      F->dump(getOptions().EmitVerbosePIL);
   }
}

void PILPassManager::addFunctionToWorklist(PILFunction *F,
                                           PILFunction *DerivedFrom) {
   assert(F && F->isDefinition() && (isMandatory || F->shouldOptimize()) &&
          "Expected optimizable function definition!");

   constexpr int MaxDeriveLevels = 10;

   int NewLevel = 1;
   if (DerivedFrom) {
      // When PILVerifyAll is enabled, individual functions are verified after
      // function passes are run upon them. This means that any functions created
      // by a function pass will not be verified after the pass runs. Thus
      // specialization errors that cause the verifier to trip will be
      // misattributed to the first pass that makes a change to the specialized
      // function. This is very misleading and increases triage time.
      //
      // As a result, when PILVerifyAll is enabled, we always verify newly
      // specialized functions as they are added to the worklist.
      //
      // TODO: Currently, all specialized functions are added to the function
      // worklist in this manner. This is all well and good, but we should really
      // add support for verifying that all specialized functions are added via
      // this function to the pass manager to ensure that we perform this
      // verification.
      if (getOptions().VerifyAll) {
         F->verify();
      }

      NewLevel = DerivationLevels[DerivedFrom] + 1;
      // Limit the number of derivations, i.e. don't allow that a pass specializes
      // a specialized function which is itself a specialized function, and so on.
      if (NewLevel >= MaxDeriveLevels)
         return;
   }
   int &StoredLevel = DerivationLevels[F];

   // Only allow a function to be pushed on the worklist a single time
   // (not counting the initial population of the worklist with the bottom-up
   // function order).
   if (StoredLevel > 0)
      return;

   StoredLevel = NewLevel;
   FunctionWorklist.push_back(F);
}

void PILPassManager::restartWithCurrentFunction(PILTransform *T) {
   assert(isa<PILFunctionTransform>(T) &&
          "Can only restart the pipeline from function passes");
   RestartPipeline = true;
}

/// Reset the state of the pass manager and remove all transformation
/// owned by the pass manager. Analysis passes will be kept.
void PILPassManager::resetAndRemoveTransformations() {
   for (auto *T : Transformations)
      delete T;

   Transformations.clear();
}

void PILPassManager::setStageName(llvm::StringRef NextStage) {
   StageName = NextStage;
}

StringRef PILPassManager::getStageName() const {
   return StageName;
}

const PILOptions &PILPassManager::getOptions() const {
   return Mod->getOptions();
}

void PILPassManager::addPass(PassKind Kind) {
   assert(unsigned(PassKind::AllPasses_Last) >= unsigned(Kind) &&
             "Invalid pass kind");
   switch (Kind) {
#define PASS(ID, TAG, NAME)                                                    \
  case PassKind::ID: {                                                         \
    PILTransform *T = polar::create##ID();                                     \
    T->setPassKind(PassKind::ID);                                              \
    Transformations.push_back(T);                                              \
    break;                                                                     \
  }
#define IRGEN_PASS(ID, TAG, NAME)                                              \
  case PassKind::ID: {                                                         \
    PILTransform *T = IRGenPasses[unsigned(Kind)];                             \
    assert(T && "Missing IRGen pass?");                                        \
    T->setPassKind(PassKind::ID);                                              \
    Transformations.push_back(T);                                              \
    IRGenPasses.erase(unsigned(Kind));                                         \
    break;                                                                     \
  }
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"
      case PassKind::invalidPassKind:
         llvm_unreachable("invalid pass kind");
   }
}

void PILPassManager::addPassForName(StringRef Name) {
   PassKind P = llvm::StringSwitch<PassKind>(Name)
#define PASS(ID, TAG, NAME) .Case(#ID, PassKind::ID)
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"
   ;
   addPass(P);
}

//===----------------------------------------------------------------------===//
//                          View Call-Graph Implementation
//===----------------------------------------------------------------------===//

#ifndef NDEBUG

namespace {

/// An explicit graph data structure for the call graph.
/// Used for viewing the callgraph as dot file with llvm::ViewGraph.
struct CallGraph {

   struct Node;

   struct Edge {
      FullApplySite FAS;
      Node *Child;
      bool Incomplete;
   };

   struct Node {
      PILFunction *F;
      CallGraph *CG;
      int NumCallSites = 0;
      SmallVector<Edge, 8> Children;
   };

   struct child_iterator
      : public std::iterator<std::random_access_iterator_tag, Node *,
         ptrdiff_t> {
      SmallVectorImpl<Edge>::iterator baseIter;

      child_iterator(SmallVectorImpl<Edge>::iterator baseIter) :
         baseIter(baseIter)
      { }

      child_iterator &operator++() { baseIter++; return *this; }
      child_iterator operator++(int) {
         auto tmp = *this;
         baseIter++;
         return tmp;
      }
      Node *operator*() const { return baseIter->Child; }
      bool operator==(const child_iterator &RHS) const {
         return baseIter == RHS.baseIter;
      }
      bool operator!=(const child_iterator &RHS) const {
         return baseIter != RHS.baseIter;
      }
      difference_type operator-(const child_iterator &RHS) const {
         return baseIter - RHS.baseIter;
      }
   };

   CallGraph(PILModule *M, BasicCalleeAnalysis *BCA);

   std::vector<Node> Nodes;

   /// The PILValue IDs which are printed as edge source labels.
   llvm::DenseMap<const PILNode *, unsigned> InstToIDMap;

   typedef std::vector<Node>::iterator iterator;
};

CallGraph::CallGraph(PILModule *M, BasicCalleeAnalysis *BCA) {
   Nodes.resize(M->getFunctionList().size());
   llvm::DenseMap<PILFunction *, Node *> NodeMap;
   int idx = 0;
   for (PILFunction &F : *M) {
      Node &Nd = Nodes[idx++];
      Nd.F = &F;
      Nd.CG = this;
      NodeMap[&F] = &Nd;

      F.numberValues(InstToIDMap);
   }

   for (Node &Nd : Nodes) {
      for (PILBasicBlock &BB : *Nd.F) {
         for (PILInstruction &I : BB) {
            if (FullApplySite FAS = FullApplySite::isa(&I)) {
               auto CList = BCA->getCalleeList(FAS);
               for (PILFunction *Callee : CList) {
                  Node *CalleeNode = NodeMap[Callee];
                  Nd.Children.push_back({FAS, CalleeNode,CList.isIncomplete()});
               }
            }
         }
      }
   }
}

} // end anonymous namespace

namespace llvm {

/// Wraps a dot node label string to multiple lines. The \p NumEdgeLabels
/// gives an estimate on the minimum width of the node shape.
static void wrap(std::string &Str, int NumEdgeLabels) {
   unsigned ColNum = 0;
   unsigned LastSpace = 0;
   unsigned MaxColumns = std::max(60, NumEdgeLabels * 8);
   for (unsigned i = 0; i != Str.length(); ++i) {
      if (ColNum == MaxColumns) {
         if (!LastSpace)
            LastSpace = i;
         Str.insert(LastSpace + 1, "\\l");
         ColNum = i - LastSpace - 1;
         LastSpace = 0;
      } else
         ++ColNum;
      if (Str[i] == ' ' || Str[i] == '.')
         LastSpace = i;
   }
}

/// CallGraph GraphTraits specialization so the CallGraph can be
/// iterable by generic graph iterators.
template <> struct GraphTraits<CallGraph::Node *> {
   typedef CallGraph::child_iterator ChildIteratorType;
   typedef CallGraph::Node *NodeRef;

   static NodeRef getEntryNode(NodeRef N) { return N; }
   static inline ChildIteratorType child_begin(NodeRef N) {
      return N->Children.begin();
   }
   static inline ChildIteratorType child_end(NodeRef N) {
      return N->Children.end();
   }
};

template <> struct GraphTraits<CallGraph *>
   : public GraphTraits<CallGraph::Node *> {
   typedef CallGraph *GraphType;
   typedef CallGraph::Node *NodeRef;

   static NodeRef getEntryNode(GraphType F) { return nullptr; }

   typedef pointer_iterator<CallGraph::iterator> nodes_iterator;
   static nodes_iterator nodes_begin(GraphType CG) {
      return nodes_iterator(CG->Nodes.begin());
   }
   static nodes_iterator nodes_end(GraphType CG) {
      return nodes_iterator(CG->Nodes.end());
   }
   static unsigned size(GraphType CG) { return CG->Nodes.size(); }
};

/// This is everything the llvm::GraphWriter needs to write the call graph in
/// a dot file.
template <>
struct DOTGraphTraits<CallGraph *> : public DefaultDOTGraphTraits {

   DOTGraphTraits(bool isSimple = false) : DefaultDOTGraphTraits(isSimple) {}

   std::string getNodeLabel(const CallGraph::Node *Node,
                            const CallGraph *Graph) {
      std::string Label = Node->F->getName();
      wrap(Label, Node->NumCallSites);
      return Label;
   }

   std::string getNodeDescription(const CallGraph::Node *Node,
                                  const CallGraph *Graph) {
      std::string Label = demangling::
      demangleSymbolAsString(Node->F->getName());
      wrap(Label, Node->NumCallSites);
      return Label;
   }

   static std::string getEdgeSourceLabel(const CallGraph::Node *Node,
                                         CallGraph::child_iterator I) {
      std::string Label;
      raw_string_ostream O(Label);
      PILInstruction *Inst = I.baseIter->FAS.getInstruction();
      O << '%' << Node->CG->InstToIDMap[Inst];
      return Label;
   }

   static std::string getEdgeAttributes(const CallGraph::Node *Node,
                                        CallGraph::child_iterator I,
                                        const CallGraph *Graph) {
      CallGraph::Edge *Edge = I.baseIter;
      if (Edge->Incomplete)
         return "color=\"red\"";
      return "";
   }
};
} // namespace llvm
#endif

void PILPassManager::viewCallGraph() {
   /// When asserts are disabled, this should be a NoOp.
#ifndef NDEBUG
   CallGraph OCG(getModule(), getAnalysis<BasicCalleeAnalysis>());
   llvm::ViewGraph(&OCG, "callgraph");
#endif
}
