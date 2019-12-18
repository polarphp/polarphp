//===--- Passes.cpp - Swift Compiler PIL Pass Entrypoints -----------------===//
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

#define DEBUG_TYPE "pil-optimizer"

#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/pil/lang/PILModule.h"
#include "polarphp/pil/optimizer/passmgr/PassManager.h"
#include "polarphp/pil/optimizer/passmgr/Transforms.h"
#include "polarphp/pil/optimizer/utils/InstOptUtils.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/YAMLParser.h"

using namespace polar;

bool polar::runPILDiagnosticPasses(PILModule &Module) {
   // Verify the module, if required.
   if (Module.getOptions().VerifyAll)
      Module.verify();

   // If we parsed a .sil file that is already in canonical form, don't rerun
   // the diagnostic passes.
   if (Module.getStage() != PILStage::Raw)
      return false;

   auto &Ctx = Module.getAstContext();

   PILPassManager PM(&Module, "", /*isMandatoryPipeline=*/ true);
   PM.executePassPipelinePlan(
      PILPassPipelinePlan::getDiagnosticPassPipeline(Module.getOptions()));

   // If we were asked to debug serialization, exit now.
   if (Module.getOptions().DebugSerialization)
      return Ctx.hadError();

   // Generate diagnostics.
   Module.setStage(PILStage::Canonical);

   // Verify the module, if required.
   if (Module.getOptions().VerifyAll)
      Module.verify();
   else {
      LLVM_DEBUG(Module.verify());
   }

   // If errors were produced during PIL analysis, return true.
   return Ctx.hadError();
}

bool polar::runPILOwnershipEliminatorPass(PILModule &Module) {
   auto &Ctx = Module.getAstContext();

   PILPassManager PM(&Module);
   PM.executePassPipelinePlan(
      PILPassPipelinePlan::getOwnershipEliminatorPassPipeline(
         Module.getOptions()));

   return Ctx.hadError();
}

// Prepare PIL for the -O pipeline.
void polar::runPILOptPreparePasses(PILModule &Module) {
   PILPassManager PM(&Module);
   PM.executePassPipelinePlan(
      PILPassPipelinePlan::getPILOptPreparePassPipeline(Module.getOptions()));
}

void polar::runPILOptimizationPasses(PILModule &Module) {
   // Verify the module, if required.
   if (Module.getOptions().VerifyAll)
      Module.verify();

   if (Module.getOptions().DisablePILPerfOptimizations) {
      // If we are not supposed to run PIL perf optzns, we may still need to
      // serialize. So serialize now.
      PILPassManager PM(&Module, "" /*stage*/, true /*isMandatory*/);
      PM.executePassPipelinePlan(
         PILPassPipelinePlan::getSerializePILPassPipeline(Module.getOptions()));
      return;
   }

   {
      PILPassManager PM(&Module);
      PM.executePassPipelinePlan(
         PILPassPipelinePlan::getPerformancePassPipeline(Module.getOptions()));
   }

   // Check if we actually serialized our module. If we did not, serialize now.
   if (!Module.isSerialized()) {
      PILPassManager PM(&Module, "" /*stage*/, true /*isMandatory*/);
      PM.executePassPipelinePlan(
         PILPassPipelinePlan::getSerializePILPassPipeline(Module.getOptions()));
   }

   // If we were asked to debug serialization, exit now.
   if (Module.getOptions().DebugSerialization)
      return;

   // Verify the module, if required.
   if (Module.getOptions().VerifyAll)
      Module.verify();
   else {
      LLVM_DEBUG(Module.verify());
   }
}

void polar::runPILPassesForOnone(PILModule &Module) {
   // Verify the module, if required.
   if (Module.getOptions().VerifyAll)
      Module.verify();

   // We want to run the Onone passes also for function which have an explicit
   // Onone attribute.
   PILPassManager PM(&Module, "Onone", /*isMandatoryPipeline=*/ true);
   PM.executePassPipelinePlan(
      PILPassPipelinePlan::getOnonePassPipeline(Module.getOptions()));

   // Verify the module, if required.
   if (Module.getOptions().VerifyAll)
      Module.verify();
   else {
      LLVM_DEBUG(Module.verify());
   }
}

void polar::runPILOptimizationPassesWithFileSpecification(PILModule &M,
                                                          StringRef Filename) {
   PILPassManager PM(&M);
   PM.executePassPipelinePlan(
      PILPassPipelinePlan::getPassPipelineFromFile(M.getOptions(), Filename));
}

/// Get the Pass ID enum value from an ID string.
PassKind polar::PassKindFromString(StringRef IDString) {
   return llvm::StringSwitch<PassKind>(IDString)
#define PASS(ID, TAG, DESCRIPTION) .Case(#ID, PassKind::ID)
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"
      .Default(PassKind::invalidPassKind);
}

/// Get an ID string for the given pass Kind.
/// This is useful for tools that identify a pass
/// by its type name.
StringRef polar::PassKindID(PassKind Kind) {
   switch (Kind) {
#define PASS(ID, TAG, DESCRIPTION)                                             \
  case PassKind::ID:                                                           \
    return #ID;
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"
      case PassKind::invalidPassKind:
         llvm_unreachable("Invalid pass kind?!");
   }

   llvm_unreachable("Unhandled PassKind in switch.");
}

/// Get a tag string for the given pass Kind.
/// This format is useful for command line options.
StringRef polar::PassKindTag(PassKind Kind) {
   switch (Kind) {
#define PASS(ID, TAG, DESCRIPTION)                                             \
  case PassKind::ID:                                                           \
    return TAG;
#include "polarphp/pil/optimizer/passmgr/PassesDef.h"
      case PassKind::invalidPassKind:
         llvm_unreachable("Invalid pass kind?!");
   }

   llvm_unreachable("Unhandled PassKind in switch.");
}

// During PIL Lowering, passes may see partially lowered PIL, which is
// inconsistent with the current (canonical) stage. We don't change the PIL
// stage until lowering is complete. Consequently, any pass added to this
// PassManager needs to be able to handle the output of the previous pass. If
// the function pass needs to read PIL from other functions, it may be best to
// convert it to a module pass to ensure that the PIL input is always at the
// same stage of lowering.
void polar::runPILLoweringPasses(PILModule &Module) {
   PILPassManager PM(&Module, "LoweringPasses", /*isMandatoryPipeline=*/ true);
   PM.executePassPipelinePlan(
      PILPassPipelinePlan::getLoweringPassPipeline(Module.getOptions()));
   assert(Module.getStage() == PILStage::Lowered);
}
