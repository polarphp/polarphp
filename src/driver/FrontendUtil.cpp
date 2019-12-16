//===--- FrontendUtil.cpp - Driver Utilities for Frontend -----------------===//
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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/04.

#include "polarphp/driver/FrontendUtil.h"

#include "polarphp/ast/DiagnosticsDriver.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Compilation.h"
#include "polarphp/driver/Driver.h"
#include "polarphp/driver/Job.h"
#include "polarphp/driver/ToolChain.h"
#include "llvm/Option/ArgList.h"

namespace polar::driver {

using namespace polar;

bool get_single_frontend_invocation_from_driver_arguments(
      ArrayRef<const char *> argv, DiagnosticEngine &diags,
      llvm::function_ref<bool(ArrayRef<const char *> FrontendArgs)> action)
{
   SmallVector<const char *, 16> args;
   args.push_back("<swiftc>"); // FIXME: Remove dummy argument.
   args.insert(args.end(), argv.begin(), argv.end());

   // When creating a CompilerInvocation, ensure that the driver creates a single
   // frontend command.
   args.push_back("-force-single-frontend-invocation");

   // Explictly disable batch mode to avoid a spurious warning when combining
   // -enable-batch-mode with -force-single-frontend-invocation.  This is an
   // implementation detail.
   args.push_back("-disable-batch-mode");

   // Avoid using filelists
   std::string neverThreshold =
         std::to_string(Compilation::NEVER_USE_FILELIST);
   args.push_back("-driver-filelist-threshold");
   args.push_back(neverThreshold.c_str());

   // Force the driver into batch mode by specifying "swiftc" as the name.
   Driver theDriver("polarphpc", "polarphpc", args, diags);

   // Don't check for the existence of input files, since the user of the
   // CompilerInvocation may wish to remap inputs to source buffers.
   theDriver.setCheckInputFilesExist(false);

   std::unique_ptr<llvm::opt::InputArgList> argList =
         theDriver.parseArgStrings(ArrayRef<const char *>(args).slice(1));
   if (diags.hadAnyError()) {
      return true;
   }

   std::unique_ptr<ToolChain> toolchain = theDriver.buildToolChain(*argList);
   if (diags.hadAnyError()) {
      return true;
   }
   std::unique_ptr<Compilation> compilation =
         theDriver.buildCompilation(*toolchain, std::move(argList));
   if (!compilation || compilation->getJobs().empty()) {
      return true; // Don't emit an error; one should already have been emitted
   }
   SmallPtrSet<const Job *, 4> compileCommands;
   for (const Job *cmd : compilation->getJobs()) {
      if (isa<CompileJobAction>(cmd->getSource())) {
         compileCommands.insert(cmd);
      }
   }
   if (compileCommands.size() != 1) {
      // TODO: include Jobs in the diagnostic.
      diags.diagnose(SourceLoc(), diag::error_expected_one_frontend_job);
      return true;
   }
   const Job *cmd = *compileCommands.begin();
   if (StringRef("-frontend") != cmd->getArguments().front()) {
      diags.diagnose(SourceLoc(), diag::error_expected_frontend_command);
      return true;
   }
   const llvm::opt::ArgStringList &BaseFrontendArgs = cmd->getArguments();
   return action(llvm::makeArrayRef(BaseFrontendArgs).drop_front());
}


} // polar::driver
