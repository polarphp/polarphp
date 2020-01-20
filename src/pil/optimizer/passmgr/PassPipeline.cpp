//===--- PassPipeline.cpp - Swift Compiler PIL Pass Entrypoints -----------===//
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
///  \file
///  This file provides implementations of a few helper functions
///  which provide abstracted entrypoints to the PILPasses stage.
///
///  \note The actual PIL passes should be implemented in per-pass source files,
///  not in this file.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "pil-passpipeline-plan"

#include "polarphp/pil/optimizer/passmgr/PassPipeline.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/optimizer/analysis/Analysis.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/YAMLParser.h"
#include "llvm/Support/YAMLTraits.h"

using namespace polar;

static llvm::cl::opt<bool>
   PILViewCFG("pil-view-cfg", llvm::cl::init(false),
              llvm::cl::desc("Enable the pil cfg viewer pass"));

static llvm::cl::opt<bool> PILViewGuaranteedCFG(
   "pil-view-guaranteed-cfg", llvm::cl::init(false),
   llvm::cl::desc("Enable the pil cfg viewer pass after diagnostics"));

static llvm::cl::opt<bool> PILViewPILGenCFG(
   "pil-view-pilgen-cfg", llvm::cl::init(false),
   llvm::cl::desc("Enable the pil cfg viewer pass before diagnostics"));

//===----------------------------------------------------------------------===//
//                          Diagnostic Pass Pipeline
//===----------------------------------------------------------------------===//

static void addCFGPrinterPipeline(PILPassPipelinePlan &P, StringRef Name) {
   P.startPipeline(Name);
   P.addCFGPrinter();
}

static void addMandatoryDebugSerialization(PILPassPipelinePlan &P) {
   P.startPipeline("Mandatory Debug Serialization");
   P.addOwnershipModelEliminator();
   P.addMandatoryInlining();
}

static void addOwnershipModelEliminatorPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("Ownership Model Eliminator");
   P.addOwnershipModelEliminator();
}

/// Passes for performing definite initialization. Must be run together in this
/// order.
static void addDefiniteInitialization(PILPassPipelinePlan &P) {
   P.addMarkUninitializedFixup();
   P.addDefiniteInitialization();
   P.addRawPILInstLowering();
}

static void addMandatoryOptPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("Guaranteed Passes");
   P.addPILGenCleanup();
   P.addDiagnoseInvalidEscapingCaptures();
   P.addDiagnoseStaticExclusivity();
   P.addCapturePromotion();

   // Select access kind after capture promotion and before stack promotion.
   // This guarantees that stack-promotable boxes have [static] enforcement.
   P.addAccessEnforcementSelection();

   P.addAllocBoxToStack();
   P.addNoReturnFolding();
   addDefiniteInitialization(P);
   // Only run semantic arc opts if we are optimizing and if mandatory semantic
   // arc opts is explicitly enabled.
   //
   // NOTE: Eventually this pass will be split into a mandatory/more aggressive
   // pass. This will happen when OSSA is no longer eliminated before the
   // optimizer pipeline is run implying we can put a pass that requires OSSA
   // there.
   const auto &Options = P.getOptions();
   P.addClosureLifetimeFixup();

#ifndef NDEBUG
   // Add a verification pass to check our work when skipping non-inlinable
   // function bodies.
   if (Options.SkipNonInlinableFunctionBodies)
      P.addNonInlinableFunctionSkippingChecker();
#endif

   if (Options.shouldOptimize()) {
      P.addSemanticARCOpts();
      P.addDestroyHoisting();
   }
   if (!Options.StripOwnershipAfterSerialization)
      P.addOwnershipModelEliminator();
   P.addMandatoryInlining();
   P.addMandatoryPILLinker();

   // Promote loads as necessary to ensure we have enough SSA formation to emit
   // SSA based diagnostics.
   P.addPredictableMemoryAccessOptimizations();

   // This phase performs optimizations necessary for correct interoperation of
   // Swift os log APIs with C os_log ABIs.
   // Pass dependencies: this pass depends on MandatoryInlining and Mandatory
   // Linking happening before this pass and ConstantPropagation happening after
   // this pass.
   P.addOSLogOptimization();

   // Diagnostic ConstantPropagation must be rerun on deserialized functions
   // because it is sensitive to the assert configuration.
   // Consequently, certain optimization passes beyond this point will also rerun.
   P.addDiagnosticConstantPropagation();

   // Now that we have emitted constant propagation diagnostics, try to eliminate
   // dead allocations.
   P.addPredictableDeadAllocationElimination();

   P.addGuaranteedARCOpts();
   P.addDiagnoseUnreachable();
   P.addDiagnoseInfiniteRecursion();
   P.addYieldOnceCheck();
   P.addEmitDFDiagnostics();

   // Canonical swift requires all non cond_br critical edges to be split.
   P.addSplitNonCondBrCriticalEdges();
}

PILPassPipelinePlan
PILPassPipelinePlan::getDiagnosticPassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);

   if (PILViewPILGenCFG) {
      addCFGPrinterPipeline(P, "PIL View PILGen CFG");
   }

   // If we are asked do debug serialization, instead of running all diagnostic
   // passes, just run mandatory inlining with dead transparent function cleanup
   // disabled.
   if (Options.DebugSerialization) {
      addMandatoryDebugSerialization(P);
      return P;
   }

   // Otherwise run the rest of diagnostics.
   addMandatoryOptPipeline(P);

   if (PILViewGuaranteedCFG) {
      addCFGPrinterPipeline(P, "PIL View Guaranteed CFG");
   }
   return P;
}

//===----------------------------------------------------------------------===//
//                       Ownership Eliminator Pipeline
//===----------------------------------------------------------------------===//

PILPassPipelinePlan PILPassPipelinePlan::getOwnershipEliminatorPassPipeline(
   const PILOptions &Options) {
   PILPassPipelinePlan P(Options);
   addOwnershipModelEliminatorPipeline(P);
   return P;
}

//===----------------------------------------------------------------------===//
//                         Performance Pass Pipeline
//===----------------------------------------------------------------------===//

namespace {

// Enumerates the optimization kinds that we do in PIL.
enum OptimizationLevelKind {
   LowLevel,
   MidLevel,
   HighLevel,
};

} // end anonymous namespace

void addSimplifyCFGPILCombinePasses(PILPassPipelinePlan &P) {
   P.addSimplifyCFG();
   P.addConditionForwarding();
   // Jump threading can expose opportunity for pilcombine (enum -> is_enum_tag->
   // cond_br).
   P.addPILCombine();
   // Which can expose opportunity for simplifcfg.
   P.addSimplifyCFG();
}

/// Perform semantic annotation/loop base optimizations.
void addHighLevelLoopOptPasses(PILPassPipelinePlan &P) {
   // Perform classic SSA optimizations for cleanup.
   P.addLowerAggregateInstrs();
   P.addPILCombine();
   P.addSROA();
   P.addMem2Reg();
   P.addDCE();
   P.addPILCombine();
   addSimplifyCFGPILCombinePasses(P);

   // Run high-level loop opts.
   P.addLoopRotate();

   // Cleanup.
   P.addDCE();
   // Also CSE semantic calls.
   P.addHighLevelCSE();
   P.addPILCombine();
   P.addSimplifyCFG();
   // Optimize access markers for better LICM: might merge accesses
   // It will also set the no_nested_conflict for dynamic accesses
   P.addAccessEnforcementReleaseSinking();
   P.addAccessEnforcementOpts();
   P.addHighLevelLICM();
   // Simplify CFG after LICM that creates new exit blocks
   P.addSimplifyCFG();
   // LICM might have added new merging potential by hoisting
   // we don't want to restart the pipeline - ignore the
   // potential of merging out of two loops
   P.addAccessEnforcementReleaseSinking();
   P.addAccessEnforcementOpts();
   // Start of loop unrolling passes.
   P.addArrayCountPropagation();
   // To simplify induction variable.
   P.addPILCombine();
   P.addLoopUnroll();
   P.addSimplifyCFG();
   P.addPerformanceConstantPropagation();
   P.addSimplifyCFG();
   P.addArrayElementPropagation();
   // End of unrolling passes.
   P.addABCOpt();
   // Cleanup.
   P.addDCE();
   P.addCOWArrayOpts();
   // Cleanup.
   P.addDCE();
   P.addTypePHPArrayPropertyOpt();
}

// Perform classic SSA optimizations.
void addSSAPasses(PILPassPipelinePlan &P, OptimizationLevelKind OpLevel) {
   // Promote box allocations to stack allocations.
   P.addAllocBoxToStack();

   // Propagate copies through stack locations.  Should run after
   // box-to-stack promotion since it is limited to propagating through
   // stack locations. Should run before aggregate lowering since that
   // splits up copy_addr.
   P.addCopyForwarding();

   // Split up opaque operations (copy_addr, retain_value, etc.).
   P.addLowerAggregateInstrs();

   // Split up operations on stack-allocated aggregates (struct, tuple).
   P.addSROA();

   // Promote stack allocations to values.
   P.addMem2Reg();

   // Run the existential specializer Pass.
   P.addExistentialSpecializer();

   // Cleanup, which is important if the inliner has restarted the pass pipeline.
   P.addPerformanceConstantPropagation();
   P.addSimplifyCFG();
   P.addPILCombine();

   // Mainly for Array.append(contentsOf) optimization.
   P.addArrayElementPropagation();

   // Run the devirtualizer, specializer, and inliner. If any of these
   // makes a change we'll end up restarting the function passes on the
   // current function (after optimizing any new callees).
   P.addDevirtualizer();
   P.addGenericSpecializer();
   // Run devirtualizer after the specializer, because many
   // class_method/witness_method instructions may use concrete types now.
   P.addDevirtualizer();

   switch (OpLevel) {
      case OptimizationLevelKind::HighLevel:
         // Does not inline functions with defined semantics.
         P.addEarlyInliner();
         break;
      case OptimizationLevelKind::MidLevel:
         P.addGlobalOpt();
         P.addLetPropertiesOpt();
         // It is important to serialize before any of the @_semantics
         // functions are inlined, because otherwise the information about
         // uses of such functions inside the module is lost,
         // which reduces the ability of the compiler to optimize clients
         // importing this module.
         P.addSerializePILPass();

         // Now strip any transparent functions that still have ownership.
         if (P.getOptions().StripOwnershipAfterSerialization)
            P.addOwnershipModelEliminator();

         if (P.getOptions().StopOptimizationAfterSerialization)
            return;

         // Does inline semantics-functions (except "availability"), but not
         // global-init functions.
         P.addPerfInliner();
         break;
      case OptimizationLevelKind::LowLevel:
         // Inlines everything
         P.addLateInliner();
         break;
   }

   // Promote stack allocations to values and eliminate redundant
   // loads.
   P.addMem2Reg();
   P.addPerformanceConstantPropagation();
   //  Do a round of CFG simplification, followed by peepholes, then
   //  more CFG simplification.

   // Jump threading can expose opportunity for PILCombine (enum -> is_enum_tag->
   // cond_br).
   P.addJumpThreadSimplifyCFG();
   P.addPILCombine();
   // PILCombine can expose further opportunities for SimplifyCFG.
   P.addSimplifyCFG();

   P.addCSE();
   if (OpLevel == OptimizationLevelKind::HighLevel) {
      // Early RLE does not touch loads from Arrays. This is important because
      // later array optimizations, like ABCOpt, get confused if an array load in
      // a loop is converted to a pattern with a phi argument.
      P.addEarlyRedundantLoadElimination();
   } else {
      P.addRedundantLoadElimination();
   }

   P.addPerformanceConstantPropagation();
   P.addCSE();
   P.addDCE();

   // Perform retain/release code motion and run the first ARC optimizer.
   P.addEarlyCodeMotion();
   P.addReleaseHoisting();
   P.addARCSequenceOpts();

   P.addSimplifyCFG();
   if (OpLevel == OptimizationLevelKind::LowLevel) {
      // Remove retain/releases based on Builtin.unsafeGuaranteed
      P.addUnsafeGuaranteedPeephole();
      // Only hoist releases very late.
      P.addLateCodeMotion();
   } else
      P.addEarlyCodeMotion();

   P.addRetainSinking();
   // Retain sinking does not sink all retains in one round.
   // Let it run one more time time, because it can be beneficial.
   // FIXME: Improve the RetainSinking pass to sink more/all
   // retains in one go.
   P.addRetainSinking();
   P.addReleaseHoisting();
   P.addARCSequenceOpts();
}

static void addPerfDebugSerializationPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("Performance Debug Serialization");
   P.addPerformancePILLinker();
}

static void addPerfEarlyModulePassPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("EarlyModulePasses");

   // Get rid of apparently dead functions as soon as possible so that
   // we do not spend time optimizing them.
   P.addDeadFunctionElimination();

   P.addSemanticARCOpts();

   // Strip ownership from non-transparent functions.
   if (P.getOptions().StripOwnershipAfterSerialization)
      P.addNonTransparentFunctionOwnershipModelEliminator();

   // Start by cloning functions from stdlib.
   P.addPerformancePILLinker();

   // Cleanup after PILGen: remove trivial copies to temporaries.
   P.addTempRValueOpt();

   // Add the outliner pass (Osize).
   P.addOutliner();

   P.addCrossModuleSerializationSetup();
}

static void addHighLevelEarlyLoopOptPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("HighLevel+EarlyLoopOpt");
   // FIXME: update this to be a function pass.
   P.addEagerSpecializer();
   addSSAPasses(P, OptimizationLevelKind::HighLevel);
   addHighLevelLoopOptPasses(P);
}

static void addMidModulePassesStackPromotePassPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("MidModulePasses+StackPromote");
   P.addDeadFunctionElimination();
   P.addPerformancePILLinker();
   P.addDeadObjectElimination();
   P.addGlobalPropertyOpt();

   // Do the first stack promotion on high-level PIL.
   P.addStackPromotion();
}

static bool addMidLevelPassPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("MidLevel");
   addSSAPasses(P, OptimizationLevelKind::MidLevel);
   if (P.getOptions().StopOptimizationAfterSerialization)
      return true;

   // Specialize partially applied functions with dead arguments as a preparation
   // for CapturePropagation.
   P.addDeadArgSignatureOpt();

   // Run loop unrolling after inlining and constant propagation, because loop
   // trip counts may have became constant.
   P.addLoopUnroll();
   return false;
}

static void addClosureSpecializePassPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("ClosureSpecialize");
   P.addDeadFunctionElimination();
   P.addDeadStoreElimination();
   P.addDeadObjectElimination();

   // These few passes are needed to cleanup between loop unrolling and GlobalOpt.
   // This is needed to fully optimize static small String constants.
   P.addSimplifyCFG();
   P.addPILCombine();
   P.addPerformanceConstantPropagation();
   P.addSimplifyCFG();

   // Hoist globals out of loops.
   // Global-init functions should not be inlined GlobalOpt is done.
   P.addGlobalOpt();
   P.addLetPropertiesOpt();

   // Propagate constants into closures and convert to static dispatch.  This
   // should run after specialization and inlining because we don't want to
   // specialize a call that can be inlined. It should run before
   // ClosureSpecialization, because constant propagation is more effective.  At
   // least one round of SSA optimization and inlining should run after this to
   // take advantage of static dispatch.
   P.addCapturePropagation();

   // Specialize closure.
   P.addClosureSpecializer();

   // Do the second stack promotion on low-level PIL.
   P.addStackPromotion();

   // Speculate virtual call targets.
   P.addSpeculativeDevirtualization();

   // There should be at least one PILCombine+SimplifyCFG between the
   // ClosureSpecializer, etc. and the last inliner. Cleaning up after these
   // passes can expose more inlining opportunities.
   addSimplifyCFGPILCombinePasses(P);

   // We do this late since it is a pass like the inline caches that we only want
   // to run once very late. Make sure to run at least one round of the ARC
   // optimizer after this.
}

static void addLowLevelPassPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("LowLevel");

   // Should be after FunctionSignatureOpts and before the last inliner.
   P.addReleaseDevirtualizer();

   addSSAPasses(P, OptimizationLevelKind::LowLevel);

   P.addDeadObjectElimination();
   P.addObjectOutliner();
   P.addDeadStoreElimination();

   // We've done a lot of optimizations on this function, attempt to FSO.
   P.addFunctionSignatureOpts();
}

static void addLateLoopOptPassPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("LateLoopOpt");

   // Delete dead code and drop the bodies of shared functions.
   P.addDeadFunctionElimination();

   // Perform the final lowering transformations.
   P.addCodeSinking();
   // Optimize access markers for better LICM: might merge accesses
   // It will also set the no_nested_conflict for dynamic accesses
   P.addAccessEnforcementReleaseSinking();
   P.addAccessEnforcementOpts();
   P.addLICM();
   // Simplify CFG after LICM that creates new exit blocks
   P.addSimplifyCFG();
   // LICM might have added new merging potential by hoisting
   // we don't want to restart the pipeline - ignore the
   // potential of merging out of two loops
   P.addAccessEnforcementReleaseSinking();
   P.addAccessEnforcementOpts();

   // Optimize overflow checks.
   P.addRedundantOverflowCheckRemoval();
   P.addMergeCondFails();

   // Remove dead code.
   P.addDCE();
   P.addPILCombine();
   P.addSimplifyCFG();

   // Try to hoist all releases, including epilogue releases. This should be
   // after FSO.
   P.addLateReleaseHoisting();
}

// Run passes that
// - should only run after all general PIL transformations.
// - have no reason to run before any other PIL optimizations.
// - don't require IRGen information.
static void addLastChanceOptPassPipeline(PILPassPipelinePlan &P) {
   // Optimize access markers for improved IRGen after all other optimizations.
   P.addAccessEnforcementReleaseSinking();
   P.addAccessEnforcementOpts();
   P.addAccessEnforcementWMO();
   P.addAccessEnforcementDom();
   // addAccessEnforcementDom might provide potential for LICM:
   // A loop might have only one dynamic access now, i.e. hoistable
   P.addLICM();

   // Only has an effect if the -assume-single-thread option is specified.
   P.addAssumeSingleThreaded();
}

static void addPILDebugInfoGeneratorPipeline(PILPassPipelinePlan &P) {
   P.startPipeline("PIL Debug Info Generator");
   P.addPILDebugInfoGenerator();
}

/// Mandatory IRGen preparation. It is the caller's job to set the set stage to
/// "lowered" after running this pipeline.
PILPassPipelinePlan
PILPassPipelinePlan::getLoweringPassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);
   P.startPipeline("Address Lowering");
   P.addOwnershipModelEliminator();
   P.addIRGenPrepare();
   P.addAddressLowering();

   return P;
}

PILPassPipelinePlan
PILPassPipelinePlan::getIRGenPreparePassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);
   P.startPipeline("IRGen Preparation");
   // Insert PIL passes to run during IRGen.
   // Hoist generic alloc_stack instructions to the entry block to enable better
   // llvm-ir generation for dynamic alloca instructions.
   P.addAllocStackHoisting();
   if (Options.EnableLargeLoadableTypes) {
      P.addLoadableByAddress();
   }
   return P;
}

PILPassPipelinePlan
PILPassPipelinePlan::getPILOptPreparePassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);

   if (Options.DebugSerialization) {
      addPerfDebugSerializationPipeline(P);
      return P;
   }

   P.startPipeline("PILOpt Prepare Passes");
   P.addMandatoryCombine();
   P.addAccessMarkerElimination();

   return P;
}

PILPassPipelinePlan
PILPassPipelinePlan::getPerformancePassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);

   if (Options.DebugSerialization) {
      addPerfDebugSerializationPipeline(P);
      return P;
   }

   // Eliminate immediately dead functions and then clone functions from the
   // stdlib.
   addPerfEarlyModulePassPipeline(P);

   // Then run an iteration of the high-level SSA passes.
   addHighLevelEarlyLoopOptPipeline(P);
   addMidModulePassesStackPromotePassPipeline(P);

   // Run an iteration of the mid-level SSA passes.
   if (addMidLevelPassPipeline(P))
      return P;

   // Perform optimizations that specialize.
   addClosureSpecializePassPipeline(P);

   // Run another iteration of the SSA optimizations to optimize the
   // devirtualized inline caches and constants propagated into closures
   // (CapturePropagation).
   addLowLevelPassPipeline(P);

   addLateLoopOptPassPipeline(P);

   addLastChanceOptPassPipeline(P);

   // Has only an effect if the -gpil option is specified.
   addPILDebugInfoGeneratorPipeline(P);

   // Call the CFG viewer.
   if (PILViewCFG) {
      addCFGPrinterPipeline(P, "PIL Before IRGen View CFG");
   }

   return P;
}

//===----------------------------------------------------------------------===//
//                            Onone Pass Pipeline
//===----------------------------------------------------------------------===//

PILPassPipelinePlan
PILPassPipelinePlan::getOnonePassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);

   P.startPipeline("Mandatory Combines");
   P.addMandatoryCombine();

   // First serialize the PIL if we are asked to.
   P.startPipeline("Serialization");
   P.addSerializePILPass();

   // And then strip ownership...
   if (Options.StripOwnershipAfterSerialization)
      P.addOwnershipModelEliminator();

   // Finally perform some small transforms.
   P.startPipeline("Rest of Onone");
   P.addUsePrespecialized();

   // Has only an effect if the -assume-single-thread option is specified.
   P.addAssumeSingleThreaded();

   // Has only an effect if the -gpil option is specified.
   P.addPILDebugInfoGenerator();

   return P;
}

//===----------------------------------------------------------------------===//
//                        Serialize PIL Pass Pipeline
//===----------------------------------------------------------------------===//

// Add to P a new pipeline that just serializes PIL. Meant to be used in
// situations where perf optzns are disabled, but we may need to serialize.
PILPassPipelinePlan
PILPassPipelinePlan::getSerializePILPassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);
   P.startPipeline("Serialize PIL");
   P.addSerializePILPass();
   return P;
}

//===----------------------------------------------------------------------===//
//                          Inst Count Pass Pipeline
//===----------------------------------------------------------------------===//

PILPassPipelinePlan
PILPassPipelinePlan::getInstCountPassPipeline(const PILOptions &Options) {
   PILPassPipelinePlan P(Options);
   P.startPipeline("Inst Count");
   P.addInstCount();
   return P;
}

//===----------------------------------------------------------------------===//
//                          Pass Kind List Pipeline
//===----------------------------------------------------------------------===//

void PILPassPipelinePlan::addPasses(ArrayRef<PassKind> PassKinds) {
   for (auto K : PassKinds) {
      // We could add to the Kind list directly, but we want to allow for
      // additional code to be added to add* without this code needing to be
      // updated.
      switch (K) {
// Each pass gets its own add-function.
#define PASS(ID, TAG, NAME)                                                    \
  case PassKind::ID: {                                                         \
    add##ID();                                                                 \
    break;                                                                     \
  }
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"
         case PassKind::invalidPassKind:
            llvm_unreachable("Unhandled pass kind?!");
      }
   }
}

PILPassPipelinePlan
PILPassPipelinePlan::getPassPipelineForKinds(const PILOptions &Options,
                                             ArrayRef<PassKind> PassKinds) {
   PILPassPipelinePlan P(Options);
   P.startPipeline("Pass List Pipeline");
   P.addPasses(PassKinds);
   return P;
}

//===----------------------------------------------------------------------===//
//                Dumping And Loading Pass Pipelines from Yaml
//===----------------------------------------------------------------------===//

void PILPassPipelinePlan::dump() {
   print(llvm::errs());
   llvm::errs() << '\n';
}

void PILPassPipelinePlan::print(llvm::raw_ostream &os) {
   // Our pipelines yaml representation is simple, we just output it ourselves
   // rather than use the yaml writer interface. We want to use the yaml reader
   // interface to be repilient against slightly different forms of yaml.
   os << "[\n";
   interleave(getPipelines(),
              [&](const PILPassPipeline &Pipeline) {
                 os << "    [\n";

                 os << "        \"" << Pipeline.Name << "\"";
                 for (PassKind Kind : getPipelinePasses(Pipeline)) {
                    os << ",\n        [\"" << PassKindID(Kind) << "\","
                       << "\"" << PassKindTag(Kind) << "\"]";
                 }
              },
              [&] { os << "\n    ],\n"; });
   os << "\n    ]\n";
   os << ']';
}

PILPassPipelinePlan
PILPassPipelinePlan::getPassPipelineFromFile(const PILOptions &Options,
                                             StringRef Filename) {
   namespace yaml = llvm::yaml;
   LLVM_DEBUG(llvm::dbgs() << "Parsing Pass Pipeline from " << Filename << "\n");

   // Load the input file.
   llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> FileBufOrErr =
      llvm::MemoryBuffer::getFileOrSTDIN(Filename);
   if (!FileBufOrErr) {
      llvm_unreachable("Failed to read yaml file");
   }

   StringRef Buffer = FileBufOrErr->get()->getBuffer();
   llvm::SourceMgr SM;
   yaml::Stream Stream(Buffer, SM);
   yaml::document_iterator DI = Stream.begin();
   assert(DI != Stream.end() && "Failed to read a document");
   yaml::Node *N = DI->getRoot();
   assert(N && "Failed to find a root");

   PILPassPipelinePlan P(Options);

   auto *RootList = cast<yaml::SequenceNode>(N);
   llvm::SmallVector<PassKind, 32> Passes;
   for (yaml::Node &PipelineNode :
      make_range(RootList->begin(), RootList->end())) {
      Passes.clear();
      LLVM_DEBUG(llvm::dbgs() << "New Pipeline:\n");

      auto *Desc = cast<yaml::SequenceNode>(&PipelineNode);
      yaml::SequenceNode::iterator DescIter = Desc->begin();
      StringRef Name = cast<yaml::ScalarNode>(&*DescIter)->getRawValue();
      LLVM_DEBUG(llvm::dbgs() << "    Name: \"" << Name << "\"\n");
      ++DescIter;

      for (auto DescEnd = Desc->end(); DescIter != DescEnd; ++DescIter) {
         auto *InnerPassList = cast<yaml::SequenceNode>(&*DescIter);
         auto *FirstNode = &*InnerPassList->begin();
         StringRef PassName = cast<yaml::ScalarNode>(FirstNode)->getRawValue();
         unsigned Size = PassName.size() - 2;
         PassName = PassName.substr(1, Size);
         LLVM_DEBUG(llvm::dbgs() << "    Pass: \"" << PassName << "\"\n");
         auto Kind = PassKindFromString(PassName);
         assert(Kind != PassKind::invalidPassKind && "Found invalid pass kind?!");
         Passes.push_back(Kind);
      }

      P.startPipeline(Name);
      P.addPasses(Passes);
   }

   return P;
}
