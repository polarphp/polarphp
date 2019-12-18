//===--- AccessMarkerElimination.cpp - Eliminate access markers. ----------===//
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
/// This pass eliminates the instructions that demarcate memory access regions.
/// If no memory access markers exist, then the pass does nothing. Otherwise, it
/// unconditionally eliminates all non-dynamic markers (plus any dynamic markers
/// if dynamic exclusivity checking is disabled).
///
/// This is an always-on pass for temporary bootstrapping. It allows running
/// test cases through the pipeline and exercising PIL verification before all
/// passes support access markers.
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "access-marker-elim"

#include "polarphp/basic/Range.h"
#include "polarphp/pil/lang/MemAccessUtils.h"
#include "polarphp/pil/lang/PILFunction.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "llvm/Support/CommandLine.h"

using namespace polar;

// This temporary option allows markers during optimization passes. Enabling
// this flag causes this pass to preserve all access markers. Otherwise, it only
// preserved "dynamic" markers.
llvm::cl::opt<bool> EnableOptimizedAccessMarkers(
   "sil-optimized-access-markers", llvm::cl::init(false),
   llvm::cl::desc("Enable memory access markers during optimization passes."));

namespace {

struct AccessMarkerElimination {
   PILModule *Mod;
   PILFunction *F;

   bool removedAny = false;

   AccessMarkerElimination(PILFunction *F)
      : Mod(&F->getModule()), F(F) {}

   void notifyErased(PILInstruction *inst) {
      LLVM_DEBUG(llvm::dbgs() << "Erasing access marker: " << *inst);
      removedAny = true;
   }

   PILBasicBlock::iterator eraseInst(PILInstruction *inst) {
      notifyErased(inst);
      return inst->getParent()->erase(inst);
   };

   bool shouldPreserveAccess(PILAccessEnforcement enforcement);

   // Check if the instruction is a marker that should be eliminated. If so,
   // updated the PIL, short of erasing the marker itself, and return true.
   PILBasicBlock::iterator checkAndEliminateMarker(PILInstruction *inst);

   // Entry point called either by the pass by the same name
   // or as a utility (e.g. during deserialization).
   bool stripMarkers();
};

bool AccessMarkerElimination::shouldPreserveAccess(
   PILAccessEnforcement enforcement) {
   if (EnableOptimizedAccessMarkers || Mod->getOptions().VerifyExclusivity)
      return true;

   switch (enforcement) {
      case PILAccessEnforcement::Static:
      case PILAccessEnforcement::Unsafe:
         return false;
      case PILAccessEnforcement::Unknown:
      case PILAccessEnforcement::Dynamic:
         return Mod->getOptions().EnforceExclusivityDynamic;
   }
   llvm_unreachable("unhandled enforcement");
}

// Check if the instruction is a marker that should be eliminated. If so, delete
// the begin_access along with all associated end_access and a valid instruction
// iterator pointing to the first remaining instruction following the
// begin_access. If the marker is not eliminated, return an iterator pointing to
// the marker.
PILBasicBlock::iterator
AccessMarkerElimination::checkAndEliminateMarker(PILInstruction *inst) {
   if (auto beginAccess = dyn_cast<BeginAccessInst>(inst)) {
      // Builtins used by the standard library must emit markers regardless of the
      // current compiler options so that any user code that initiates access via
      // the standard library is fully enforced.
      if (beginAccess->isFromBuiltin())
         return inst->getIterator();

      // Leave dynamic accesses in place, but delete all others.
      if (shouldPreserveAccess(beginAccess->getEnforcement()))
         return inst->getIterator();

      notifyErased(beginAccess);
      return removeBeginAccess(beginAccess);
   }

   // end_access instructions will be handled when we process the
   // begin_access.

   // begin_unpaired_access instructions will be directly removed and
   // simply replaced with their operand.
   if (auto BUA = dyn_cast<BeginUnpairedAccessInst>(inst)) {
      // Builtins used by the standard library must emit markers regardless of the
      // current compiler options.
      if (BUA->isFromBuiltin())
         return inst->getIterator();

      if (shouldPreserveAccess(BUA->getEnforcement()))
         return inst->getIterator();

      return eraseInst(BUA);
   }
   // end_unpaired_access instructions will be directly removed and
   // simply replaced with their operand.
   if (auto EUA = dyn_cast<EndUnpairedAccessInst>(inst)) {
      // Builtins used by the standard library must emit markers regardless of the
      // current compiler options.
      if (EUA->isFromBuiltin())
         return inst->getIterator();

      if (shouldPreserveAccess(EUA->getEnforcement()))
         return inst->getIterator();

      return eraseInst(EUA);
   }
   return inst->getIterator();
}

// Top-level per-function entry-point.
// Return `true` if any markers were removed.
bool AccessMarkerElimination::stripMarkers() {
   // Iterating in reverse eliminates more begin_access users before they
   // need to be replaced.
   for (auto &BB : llvm::reverse(*F)) {
      // Don't cache the begin iterator since we're reverse iterating.
      for (auto II = BB.end(); II != BB.begin();) {
         PILInstruction *inst = &*(--II);
         // checkAndEliminateMarker returns the next non-deleted instruction. The
         // following iteration moves the iterator backward.
         II = checkAndEliminateMarker(inst);
      }
   }
   return removedAny;
}

} // end anonymous namespace

// Implement a PILModule::PILFunctionBodyCallback that strips all access
// markers from newly deserialized function bodies.
static void preparePILFunctionForOptimization(ModuleDecl *, PILFunction *F) {
   LLVM_DEBUG(llvm::dbgs() << "Stripping all markers in: " << F->getName()
                           << "\n");

   AccessMarkerElimination(F).stripMarkers();
}

namespace {

struct AccessMarkerEliminationPass : PILModuleTransform {
   void run() override {
      auto &M = *getModule();
      for (auto &F : M) {
         bool removedAny = AccessMarkerElimination(&F).stripMarkers();

         // Only invalidate analyses if we removed some markers.
         if (removedAny) {
            auto InvalidKind = PILAnalysis::InvalidationKind::Instructions;
            invalidateAnalysis(&F, InvalidKind);
         }

         // Markers from all current PIL functions are stripped. Register a
         // callback to strip an subsequently loaded functions on-the-fly.
         if (!EnableOptimizedAccessMarkers) {
            using NotificationHandlerTy =
            FunctionBodyDeserializationNotificationHandler;
            auto *n = new NotificationHandlerTy(preparePILFunctionForOptimization);
            std::unique_ptr<DeserializationNotificationHandler> ptr(n);
            M.registerDeserializationNotificationHandler(std::move(ptr));
         }
      }
   }
};

} // end anonymous namespace

PILTransform *polar::createAccessMarkerElimination() {
   return new AccessMarkerEliminationPass();
}
