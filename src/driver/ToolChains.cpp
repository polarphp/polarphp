//===--- ToolChains.cpp - Job invocations (general and per-platform) ------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
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
// Created by polarboy on 2019/12/03.

#include "polarphp/driver/internal/ToolChains.h"

#include "polarphp/basic/Dwarf.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/Platform.h"
#include "polarphp/basic/Range.h"
#include "polarphp/basic/StlExtras.h"
#include "polarphp/basic/TaskQueue.h"
#include "polarphp/global/Config.h"
#include "polarphp/driver/Compilation.h"
#include "polarphp/driver/Driver.h"
#include "polarphp/driver/Job.h"
#include "polarphp/option/Options.h"
#include "clang/Basic/Version.h"
#include "clang/Driver/Util.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/Program.h"

using namespace llvm::opt;
using namespace polar::driver;

namespace polar::driver {

using polar::basic::interleave;

bool ToolChain::JobContext::shouldUseInputFileList() const
{
   return getTopLevelInputFiles().size() > m_compilation.getFilelistThreshold();
}

bool ToolChain::JobContext::shouldUsePrimaryInputFileListInFrontendInvocation() const
{
   return inputActions.size() > m_compilation.getFilelistThreshold();
}

bool ToolChain::JobContext::shouldUseMainOutputFileListInFrontendInvocation() const
{
   return output.getPrimaryOutputFilenames().size() > m_compilation.getFilelistThreshold();
}

bool ToolChain::JobContext::shouldUseSupplementaryOutputFileMapInFrontendInvocation() const
{
   static const unsigned UpperBoundOnSupplementaryOutputFileTypes =
         filetypes::TY_INVALID;
   return inputActions.size() * UpperBoundOnSupplementaryOutputFileTypes >
         m_compilation.getFilelistThreshold();
}

bool ToolChain::JobContext::shouldFilterFrontendInputsByType() const
{
   // FIXME: SingleCompile has not filtered its inputs in the past and now people
   // rely upon that. But we would like the compilation modes to be consistent.
   return outputInfo.compilerMode != OutputInfo::Mode::SingleCompile;
}

void ToolChain::addInputsOfType(ArgStringList &arguments,
                                ArrayRef<const Action *> inputs,
                                filetypes::FileTypeId inputType,
                                const char *prefixArgument) const
{
   for (auto &input : inputs) {
      if (input->getType() != inputType) {
         continue;
      }
      if (prefixArgument) {
         arguments.push_back(prefixArgument);
      }
      arguments.push_back(cast<InputAction>(input)->getInputArg().getValue());
   }
}

void ToolChain::addInputsOfType(ArgStringList &arguments,
                                ArrayRef<const Job *> jobs,
                                const llvm::opt::ArgList &args,
                                filetypes::FileTypeId inputType,
                                const char *prefixArgument) const
{
   for (const Job *cmd : jobs) {
      auto output = cmd->getOutput().getAnyOutputForType(inputType);
      if (!output.empty()) {
         if (prefixArgument) {
            arguments.push_back(prefixArgument);
         }
         arguments.push_back(args.MakeArgString(output));
      }
   }
}

void ToolChain::addPrimaryInputsOfType(ArgStringList &arguments,
                                       ArrayRef<const Job *> jobs,
                                       const llvm::opt::ArgList &args,
                                       filetypes::FileTypeId inputType,
                                       const char *prefixArgument) const
{
   for (const Job *cmd : jobs) {
      auto &outputInfo = cmd->getOutput();
      if (outputInfo.getPrimaryOutputType() == inputType) {
         for (auto output : outputInfo.getPrimaryOutputFilenames()) {
            if (prefixArgument) {
               arguments.push_back(prefixArgument);
            }
            arguments.push_back(args.MakeArgString(output));
         }
      }
   }
}

static bool add_outputs_of_type(ArgStringList &arguments,
                                CommandOutput const &output,
                                const llvm::opt::ArgList &args,
                                filetypes::FileTypeId outputType,
                                const char *prefixArgument = nullptr)
{
   bool added = false;
   for (auto output : output.getAdditionalOutputsForType(outputType)) {
      assert(!output.empty());
      if (prefixArgument) {
         arguments.push_back(prefixArgument);
      }
      arguments.push_back(args.MakeArgString(output));
      added = true;
   }
   return added;
}

/// Handle arguments common to all invocations of the frontend (compilation,
/// module-merging, LLDB's REPL, etc).
static void add_common_frontend_args(const ToolChain &toolchain, const OutputInfo &outputInfo,
                                     const CommandOutput &output,
                                     const ArgList &inputArgs,
                                     ArgStringList &arguments) {
   const llvm::Triple &triple = toolchain.getTriple();

   // Only pass -target to the REPL or immediate modes if it was explicitly
   // specified on the command line.
   switch (outputInfo.compilerMode) {
   case OutputInfo::Mode::REPL:
   case OutputInfo::Mode::Immediate:
      if (!inputArgs.hasArg(options::OPT_target))
         break;
      LLVM_FALLTHROUGH;
   case OutputInfo::Mode::StandardCompile:
   case OutputInfo::Mode::SingleCompile:
   case OutputInfo::Mode::BatchModeCompile:
      arguments.push_back("-target");
      arguments.push_back(inputArgs.MakeArgString(triple.str()));
      break;
   }

   // Enable address top-byte ignored in the ARM64 backend.
   if (triple.getArch() == llvm::Triple::aarch64) {
      arguments.push_back("-Xllvm");
      arguments.push_back("-aarch64-use-tbi");
   }

   // Enable or disable ObjC interop appropriately for the platform
   if (triple.isOSDarwin()) {
      arguments.push_back("-enable-objc-interop");
   } else {
      arguments.push_back("-disable-objc-interop");
   }

   // Handle the CPU and its preferences.
   inputArgs.AddLastArg(arguments, options::OPT_target_cpu);

   if (!outputInfo.sdkPath.empty()) {
      arguments.push_back("-sdk");
      arguments.push_back(inputArgs.MakeArgString(outputInfo.sdkPath));
   }

   inputArgs.AddAllArgs(arguments, options::OPT_I);
   inputArgs.AddAllArgs(arguments, options::OPT_F, options::OPT_Fsystem);

   inputArgs.AddLastArg(arguments, options::OPT_AssertConfig);
   inputArgs.AddLastArg(arguments, options::OPT_autolink_force_load);
   inputArgs.AddLastArg(arguments, options::OPT_color_diagnostics);
   inputArgs.AddLastArg(arguments, options::OPT_fixit_all);
   inputArgs.AddLastArg(arguments, options::OPT_warn_implicit_overrides);
   inputArgs.AddLastArg(arguments, options::OPT_typo_correction_limit);
   inputArgs.AddLastArg(arguments, options::OPT_enable_app_extension);
   inputArgs.AddLastArg(arguments, options::OPT_enable_library_evolution);
   inputArgs.AddLastArg(arguments, options::OPT_enable_testing);
   inputArgs.AddLastArg(arguments, options::OPT_enable_private_imports);
   inputArgs.AddLastArg(arguments, options::OPT_g_Group);
   inputArgs.AddLastArg(arguments, options::OPT_debug_info_format);
   inputArgs.AddLastArg(arguments, options::OPT_import_underlying_module);
   inputArgs.AddLastArg(arguments, options::OPT_module_cache_path);
   inputArgs.AddLastArg(arguments, options::OPT_module_link_name);
   inputArgs.AddLastArg(arguments, options::OPT_nostdimport);
   inputArgs.AddLastArg(arguments, options::OPT_parse_stdlib);
   inputArgs.AddLastArg(arguments, options::OPT_resource_dir);
   inputArgs.AddLastArg(arguments, options::OPT_solver_memory_threshold);
   inputArgs.AddLastArg(arguments, options::OPT_value_recursion_threshold);
   inputArgs.AddLastArg(arguments, options::OPT_Rpass_EQ);
   inputArgs.AddLastArg(arguments, options::OPT_Rpass_missed_EQ);
   inputArgs.AddLastArg(arguments, options::OPT_suppress_warnings);
   inputArgs.AddLastArg(arguments, options::OPT_profile_generate);
   inputArgs.AddLastArg(arguments, options::OPT_profile_use);
   inputArgs.AddLastArg(arguments, options::OPT_profile_coverage_mapping);
   inputArgs.AddLastArg(arguments, options::OPT_warnings_as_errors);
   inputArgs.AddLastArg(arguments, options::OPT_sanitize_EQ);
   inputArgs.AddLastArg(arguments, options::OPT_sanitize_coverage_EQ);
   inputArgs.AddLastArg(arguments, options::OPT_static);
   inputArgs.AddLastArg(arguments, options::OPT_polarphp_version);
   inputArgs.AddLastArg(arguments, options::OPT_enforce_exclusivity_EQ);
   inputArgs.AddLastArg(arguments, options::OPT_stats_output_dir);
   inputArgs.AddLastArg(arguments, options::OPT_trace_stats_events);
   inputArgs.AddLastArg(arguments, options::OPT_profile_stats_events);
   inputArgs.AddLastArg(arguments, options::OPT_profile_stats_entities);
   inputArgs.AddLastArg(arguments,
                        options::OPT_solver_shrink_unsolved_threshold);
   inputArgs.AddLastArg(arguments, options::OPT_O_Group);
   inputArgs.AddLastArg(arguments, options::OPT_RemoveRuntimeAsserts);
   inputArgs.AddLastArg(arguments, options::OPT_AssumeSingleThreaded);
   inputArgs.AddLastArg(arguments,
                        options::OPT_enable_experimental_dependencies);
   inputArgs.AddLastArg(arguments,
                        options::OPT_experimental_dependency_include_intrafile);
   inputArgs.AddLastArg(arguments, options::OPT_package_description_version);
   inputArgs.AddLastArg(arguments, options::OPT_serialize_diagnostics_path);

   // Pass on any build config options
   inputArgs.AddAllArgs(arguments, options::OPT_D);

   // Pass on file paths that should be remapped in debug info.
   inputArgs.AddAllArgs(arguments, options::OPT_debug_prefix_map);

   // Pass through the values passed to -Xfrontend.
   inputArgs.AddAllArgValues(arguments, options::OPT_Xfrontend);

   if (auto *A = inputArgs.getLastArg(options::OPT_working_directory)) {
      // Add -Xcc -working-directory before any other -Xcc options to ensure it is
      // overridden by an explicit -Xcc -working-directory, although having a
      // different working directory is probably incorrect.
      SmallString<128> workingDirectory(A->getValue());
      llvm::sys::fs::make_absolute(workingDirectory);
      arguments.push_back("-Xcc");
      arguments.push_back("-working-directory");
      arguments.push_back("-Xcc");
      arguments.push_back(inputArgs.MakeArgString(workingDirectory));
   }

   // -g implies -enable-anonymous-context-mangled-names, because the extra
   // metadata aids debugging.
   if (inputArgs.hasArg(options::OPT_g)) {
      // But don't add the option in optimized builds: it would prevent dead code
      // stripping of unused metadata.
      auto OptArg = inputArgs.getLastArgNoClaim(options::OPT_O_Group);
      if (!OptArg || OptArg->getOption().matches(options::OPT_Onone))
         arguments.push_back("-enable-anonymous-context-mangled-names");
   }

   // Pass through any subsystem flags.
   inputArgs.AddAllArgs(arguments, options::OPT_Xllvm);
   inputArgs.AddAllArgs(arguments, options::OPT_Xcc);

   if (llvm::sys::Process::StandardErrHasColors()) {
      arguments.push_back("-color-diagnostics");
   }
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const CompileJobAction &job,
                               const JobContext &context) const
{
   InvocationInfo invocationInfo{POLARPHP_EXECUTABLE_NAME};
   ArgStringList &arguments = invocationInfo.arguments;
   invocationInfo.allowsResponseFiles = true;

   for (auto &s : getDriver().getPolarphpProgramArgs()) {
      arguments.push_back(s.c_str());
   }
   arguments.push_back("-frontend");
   {
      // Determine the frontend mode option.
      const char *frontendModeOption = context.computeFrontendModeForCompile();
      assert(frontendModeOption != nullptr &&
            "No frontend mode option specified!");
      arguments.push_back(frontendModeOption);
   }

   context.addFrontendInputAndOutputArguments(arguments, invocationInfo.filelistInfos);

   // Forward migrator flags.
   if (auto dataPath =
       context.args.getLastArg(options::OPT_api_diff_data_file)) {
      arguments.push_back("-api-diff-data-file");
      arguments.push_back(dataPath->getValue());
   }
   if (auto DataDir = context.args.getLastArg(options::OPT_api_diff_data_dir)) {
      arguments.push_back("-api-diff-data-dir");
      arguments.push_back(DataDir->getValue());
   }
   if (context.args.hasArg(options::OPT_dump_usr)) {
      arguments.push_back("-dump-usr");
   }

   if (context.args.hasArg(options::OPT_parse_stdlib)) {
      arguments.push_back("-disable-objc-attr-requires-foundation-module");
   }
   add_common_frontend_args(*this, context.outputInfo, context.output, context.args,
                            arguments);

   if (context.args.hasArg(options::OPT_parse_as_library) ||
       context.args.hasArg(options::OPT_emit_library)) {
      arguments.push_back("-parse-as-library");
   }

   context.args.AddLastArg(arguments, options::OPT_parse_sil);

   arguments.push_back("-module-name");
   arguments.push_back(context.args.MakeArgString(context.outputInfo.moduleName));

   add_outputs_of_type(arguments, context.output, context.args,
                       filetypes::TY_OptRecord, "-save-optimization-record-path");

   if (context.args.hasArg(options::OPT_migrate_keep_objc_visibility)) {
      arguments.push_back("-migrate-keep-objc-visibility");
   }

   add_outputs_of_type(arguments, context.output, context.args,
                       filetypes::TY_Remapping, "-emit-remap-file-path");

   if (context.outputInfo.numThreads > 0) {
      arguments.push_back("-num-threads");
      arguments.push_back(
               context.args.MakeArgString(Twine(context.outputInfo.numThreads)));
   }

   // Add the output file argument if necessary.
   if (context.output.getPrimaryOutputType() != filetypes::TY_Nothing) {
      if (context.shouldUseMainOutputFileListInFrontendInvocation()) {
         arguments.push_back("-output-filelist");
         arguments.push_back(context.getTemporaryFilePath("outputs", ""));
         invocationInfo.filelistInfos.push_back({arguments.back(),
                                                 context.output.getPrimaryOutputType(),
                                                 FilelistInfo::WhichFiles::Output});
      } else {
         for (auto FileName : context.output.getPrimaryOutputFilenames()) {
            arguments.push_back("-o");
            arguments.push_back(context.args.MakeArgString(FileName));
         }
      }
   }

   if (context.args.hasArg(options::OPT_embed_bitcode_marker)) {
      arguments.push_back("-embed-bitcode-marker");
   }
   // For `-index-file` mode add `-disable-typo-correction`, since the errors
   // will be ignored and it can be expensive to do typo-correction.
   if (job.getType() == filetypes::TY_IndexData) {
      arguments.push_back("-disable-typo-correction");
   }

   if (context.args.hasArg(options::OPT_index_store_path)) {
      context.args.AddLastArg(arguments, options::OPT_index_store_path);
      if (!context.args.hasArg(options::OPT_index_ignore_system_modules)) {
         arguments.push_back("-index-system-modules");
      }
   }

   if (context.args.hasArg(options::OPT_debug_info_store_invocation) ||
       shouldStoreInvocationInDebugInfo()) {
      arguments.push_back("-debug-info-store-invocation");
   }

   if (context.args.hasArg(
          options::OPT_disable_autolinking_runtime_compatibility)) {
      arguments.push_back("-disable-autolinking-runtime-compatibility");
   }

   if (auto arg = context.args.getLastArg(
          options::OPT_runtime_compatibility_version)) {
      arguments.push_back("-runtime-compatibility-version");
      arguments.push_back(arg->getValue());
   }

   context.args.AddLastArg(
            arguments,
            options::
            OPT_disable_autolinking_runtime_compatibility_dynamic_replacements);

   return invocationInfo;
}

const char *ToolChain::JobContext::computeFrontendModeForCompile() const {
   switch (outputInfo.compilerMode) {
   case OutputInfo::Mode::StandardCompile:
   case OutputInfo::Mode::SingleCompile:
   case OutputInfo::Mode::BatchModeCompile:
      break;
   case OutputInfo::Mode::Immediate:
   case OutputInfo::Mode::REPL:
      llvm_unreachable("REPL and immediate modes handled elsewhere");
   }
   switch (output.getPrimaryOutputType()) {
   case filetypes::TY_Object:
      return "-c";
   case filetypes::TY_PCH:
      return "-emit-pch";
   case filetypes::TY_ASTDump:
      return "-dump-ast";
   case filetypes::TY_RawPIL:
      return "-emit-pilgen";
   case filetypes::TY_PIL:
      return "-emit-pil";
   case filetypes::TY_RawPIB:
      return "-emit-pibgen";
   case filetypes::TY_PIB:
      return "-emit-pib";
   case filetypes::TY_LLVM_IR:
      return "-emit-ir";
   case filetypes::TY_LLVM_BC:
      return "-emit-bc";
   case filetypes::TY_Assembly:
      return "-S";
   case filetypes::TY_PolarModuleFile:
      // Since this is our primary output, we need to specify the option here.
      return "-emit-module";
   case filetypes::TY_ImportedModules:
      return "-emit-imported-modules";
   case filetypes::TY_IndexData:
      return "-typecheck";
   case filetypes::TY_Remapping:
      return "-update-code";
   case filetypes::TY_Nothing:
      // We were told to output nothing, so get the last mode option and use that.
      if (const Arg *A = args.getLastArg(options::OPT_modes_Group))
         return A->getSpelling().data();
      else
         llvm_unreachable("We were told to perform a standard compile, "
                          "but no mode option was passed to the driver.");
   case filetypes::TY_Polar:
   case filetypes::TY_dSYM:
   case filetypes::TY_AutolinkFile:
   case filetypes::TY_Dependencies:
   case filetypes::TY_PolarModuleDocFile:
   case filetypes::TY_ClangModuleFile:
   case filetypes::TY_SerializedDiagnostics:
   case filetypes::TY_ObjCHeader:
   case filetypes::TY_Image:
   case filetypes::TY_PolarDeps:
   case filetypes::TY_ModuleTrace:
   case filetypes::TY_TBD:
   case filetypes::TY_OptRecord:
   case filetypes::TY_PolarParseableInterfaceFile:
      llvm_unreachable("output type can never be primary output.");
   case filetypes::TY_INVALID:
      llvm_unreachable("Invalid type ID");
   }
   llvm_unreachable("unhandled output type");
}

void ToolChain::JobContext::addFrontendInputAndOutputArguments(
      ArgStringList &arguments, std::vector<FilelistInfo> &filelistInfos) const
{

   switch (outputInfo.compilerMode) {
   case OutputInfo::Mode::StandardCompile:
      assert(inputActions.size() == 1 &&
             "Standard-compile mode takes exactly one input (the primary file)");
      break;
   case OutputInfo::Mode::BatchModeCompile:
   case OutputInfo::Mode::SingleCompile:
      break;
   case OutputInfo::Mode::Immediate:
   case OutputInfo::Mode::REPL:
      llvm_unreachable("REPL and immediate modes handled elsewhere");
   }

   const bool useFileList = shouldUseInputFileList();
   const bool mayHavePrimaryInputs = outputInfo.mightHaveExplicitPrimaryInputs(output);
   const bool usePrimaryFileList =
         mayHavePrimaryInputs &&
         shouldUsePrimaryInputFileListInFrontendInvocation();
   const bool filterInputsByType = shouldFilterFrontendInputsByType();
   const bool useSupplementaryOutputFileList =
         shouldUseSupplementaryOutputFileMapInFrontendInvocation();

   assert(((m_compilation.getFilelistThreshold() != Compilation::NEVER_USE_FILELIST) ||
         (!useFileList && !usePrimaryFileList &&
          !useSupplementaryOutputFileList)) &&
          "No filelists are used if FilelistThreshold=NEVER_USE_FILELIST");

   if (useFileList) {
      arguments.push_back("-filelist");
      arguments.push_back(getAllSourcesPath());
   }
   if (usePrimaryFileList) {
      arguments.push_back("-primary-filelist");
      arguments.push_back(getTemporaryFilePath("primaryInputs", ""));
      filelistInfos.push_back({arguments.back(), filetypes::TY_Polar,
                               FilelistInfo::WhichFiles::PrimaryInputs});
   }
   if (!useFileList || !usePrimaryFileList) {
      addFrontendCommandLineInputArguments(mayHavePrimaryInputs, useFileList,
                                           usePrimaryFileList, filterInputsByType,
                                           arguments);
   }

   if (useSupplementaryOutputFileList) {
      arguments.push_back("-supplementary-output-file-map");
      arguments.push_back(getTemporaryFilePath("supplementaryOutputs", ""));
      filelistInfos.push_back({arguments.back(), filetypes::TY_INVALID,
                               FilelistInfo::WhichFiles::SupplementaryOutput});
   } else {
      addFrontendSupplementaryOutputArguments(arguments);
   }
}

void ToolChain::JobContext::addFrontendCommandLineInputArguments(
      const bool mayHavePrimaryInputs, const bool useFileList,
      const bool usePrimaryFileList, const bool filterByType,
      ArgStringList &arguments) const
{
   llvm::DenseSet<StringRef> primaries;

   if (mayHavePrimaryInputs) {
      for (const Action *action : inputActions) {
         const auto *inputAction = cast<InputAction>(action);
         const llvm::opt::Arg &InArg = inputAction->getInputArg();
         primaries.insert(InArg.getValue());
      }
   }
   // -index-file compilations are weird. They are processed as SingleCompiles
   // (WMO), but must indicate that there is one primary file, designated by
   // -index-file-path.
   if (Arg *arg = args.getLastArg(options::OPT_index_file_path)) {
      assert(primaries.empty() &&
             "index file jobs should be treated as single (WMO) compiles");
      primaries.insert(arg->getValue());
   }
   for (auto inputPair : getTopLevelInputFiles()) {
      if (filterByType && !filetypes::is_part_of_polarphp_compilation(inputPair.first)) {
         continue;
      }
      const char *inputName = inputPair.second->getValue();
      const bool isPrimary = primaries.count(inputName);
      if (isPrimary && !usePrimaryFileList) {
         arguments.push_back("-primary-file");
         arguments.push_back(inputName);
      }
      if ((!isPrimary || usePrimaryFileList) && !useFileList) {
         arguments.push_back(inputName);
      }
   }
}

void ToolChain::JobContext::addFrontendSupplementaryOutputArguments(
      ArgStringList &arguments) const
{
   // FIXME: Get these and other argument strings from the same place for both
   // driver and frontend.
   add_outputs_of_type(arguments, output, args, filetypes::FileTypeId::TY_PolarModuleFile,
                       "-emit-module-path");

   add_outputs_of_type(arguments, output, args, filetypes::TY_PolarModuleDocFile,
                       "-emit-module-doc-path");

   add_outputs_of_type(arguments, output, args,
                       filetypes::FileTypeId::TY_PolarParseableInterfaceFile,
                       "-emit-parseable-module-interface-path");

   add_outputs_of_type(arguments, output, args,
                       filetypes::TY_SerializedDiagnostics,
                       "-serialize-diagnostics-path");

   if (add_outputs_of_type(arguments, output, args, filetypes::FileTypeId::TY_ObjCHeader,
                           "-emit-objc-header-path")) {
      assert(outputInfo.compilerMode == OutputInfo::Mode::SingleCompile &&
             "The polarphp tool should only emit an Obj-C header in single compile"
             "mode!");
   }

   add_outputs_of_type(arguments, output, args, filetypes::TY_Dependencies,
                       "-emit-dependencies-path");
   add_outputs_of_type(arguments, output, args, filetypes::TY_PolarDeps,
                       "-emit-reference-dependencies-path");
   add_outputs_of_type(arguments, output, args, filetypes::TY_ModuleTrace,
                       "-emit-loaded-module-trace-path");
   add_outputs_of_type(arguments, output, args, filetypes::TY_TBD,
                       "-emit-tbd-path");
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const InterpretJobAction &job,
                               const JobContext &context) const {
   assert(context.outputInfo.compilerMode == OutputInfo::Mode::Immediate);

   InvocationInfo invocationInfo{POLARPHP_EXECUTABLE_NAME};
   ArgStringList &arguments = invocationInfo.arguments;
   invocationInfo.allowsResponseFiles = true;

   for (auto &s : getDriver().getPolarphpProgramArgs()) {
      arguments.push_back(s.c_str());
   }
   arguments.push_back("-frontend");
   arguments.push_back("-interpret");

   assert(context.inputs.empty() &&
          "The polarphp frontend does not expect to be fed any input jobs!");

   for (const Action *action : context.inputActions) {
      cast<InputAction>(action)->getInputArg().render(context.args, arguments);
   }

   if (context.args.hasArg(options::OPT_parse_stdlib)) {
      arguments.push_back("-disable-objc-attr-requires-foundation-module");
   }

   add_common_frontend_args(*this, context.outputInfo, context.output, context.args,
                            arguments);

   context.args.AddLastArg(arguments, options::OPT_parse_sil);

   arguments.push_back("-module-name");
   arguments.push_back(context.args.MakeArgString(context.outputInfo.moduleName));

   context.args.AddAllArgs(arguments, options::OPT_l, options::OPT_framework);
   // The immediate arguments must be last.
   context.args.AddLastArg(arguments, options::OPT__DASH_DASH);
   return invocationInfo;
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const BackendJobAction &job,
                               const JobContext &context) const
{
   assert(context.args.hasArg(options::OPT_embed_bitcode));
   ArgStringList arguments;

   for (auto &s : getDriver().getPolarphpProgramArgs()) {
      arguments.push_back(s.c_str());
   }
   arguments.push_back("-frontend");
   // Determine the frontend mode option.
   const char *frontendModeOption = nullptr;
   switch (context.outputInfo.compilerMode) {
   case OutputInfo::Mode::StandardCompile:
   case OutputInfo::Mode::SingleCompile: {
      switch (context.output.getPrimaryOutputType()) {
      case filetypes::TY_Object:
         frontendModeOption = "-c";
         break;
      case filetypes::TY_LLVM_IR:
         frontendModeOption = "-emit-ir";
         break;
      case filetypes::TY_LLVM_BC:
         frontendModeOption = "-emit-bc";
         break;
      case filetypes::TY_Assembly:
         frontendModeOption = "-S";
         break;
      case filetypes::TY_Nothing:
         // We were told to output nothing, so get the last mode option and use
         // that.
         if (const Arg *A = context.args.getLastArg(options::OPT_modes_Group))
            frontendModeOption = A->getSpelling().data();
         else
            llvm_unreachable("We were told to perform a standard compile, "
                             "but no mode option was passed to the driver.");
         break;

      case filetypes::TY_ImportedModules:
      case filetypes::TY_TBD:
      case filetypes::TY_PolarModuleFile:
      case filetypes::TY_ASTDump:
      case filetypes::TY_RawPIL:
      case filetypes::TY_RawPIB:
      case filetypes::TY_PIL:
      case filetypes::TY_PIB:
      case filetypes::TY_PCH:
      case filetypes::TY_IndexData:
         llvm_unreachable("Cannot be output from backend job");
      case filetypes::TY_Polar:
      case filetypes::TY_dSYM:
      case filetypes::TY_AutolinkFile:
      case filetypes::TY_Dependencies:
      case filetypes::TY_PolarModuleDocFile:
      case filetypes::TY_ClangModuleFile:
      case filetypes::TY_SerializedDiagnostics:
      case filetypes::TY_ObjCHeader:
      case filetypes::TY_Image:
      case filetypes::TY_PolarDeps:
      case filetypes::TY_Remapping:
      case filetypes::TY_ModuleTrace:
      case filetypes::TY_OptRecord:
      case filetypes::TY_PolarParseableInterfaceFile:
         llvm_unreachable("output type can never be primary output.");
      case filetypes::TY_INVALID:
         llvm_unreachable("Invalid type ID");
      }
      break;
   }
   case OutputInfo::Mode::BatchModeCompile:
   case OutputInfo::Mode::Immediate:
   case OutputInfo::Mode::REPL:
      llvm_unreachable("invalid mode for backend job");
   }

   assert(frontendModeOption != nullptr && "No frontend mode option specified!");

   arguments.push_back(frontendModeOption);

   // Add input arguments.
   switch (context.outputInfo.compilerMode) {
   case OutputInfo::Mode::StandardCompile: {
      assert(context.inputs.size() == 1 && "The backend expects one input!");
      arguments.push_back("-primary-file");
      const Job *cmd = context.inputs.front();
      arguments.push_back(context.args.MakeArgString(
                             cmd->getOutput().getPrimaryOutputFilename()));
      break;
   }
   case OutputInfo::Mode::SingleCompile: {
      assert(context.inputs.size() == 1 && "The backend expects one input!");
      arguments.push_back("-primary-file");
      const Job *cmd = context.inputs.front();

      // In multi-threaded compilation, the backend job must select the correct
      // output file of the compilation job.
      auto OutNames = cmd->getOutput().getPrimaryOutputFilenames();
      arguments.push_back(
               context.args.MakeArgString(OutNames[job.getInputIndex()]));
      break;
   }
   case OutputInfo::Mode::BatchModeCompile:
   case OutputInfo::Mode::Immediate:
   case OutputInfo::Mode::REPL:
      llvm_unreachable("invalid mode for backend job");
   }

   // Add flags implied by -embed-bitcode.
   arguments.push_back("-embed-bitcode");

   // -embed-bitcode only supports a restricted set of flags.
   arguments.push_back("-target");
   arguments.push_back(context.args.MakeArgString(getTriple().str()));

   // Enable address top-byte ignored in the ARM64 backend.
   if (getTriple().getArch() == llvm::Triple::aarch64) {
      arguments.push_back("-Xllvm");
      arguments.push_back("-aarch64-use-tbi");
   }

   // Handle the CPU and its preferences.
   context.args.AddLastArg(arguments, options::OPT_target_cpu);

   // Enable optimizations, but disable all LLVM-IR-level transformations.
   context.args.AddLastArg(arguments, options::OPT_O_Group);
   arguments.push_back("-disable-llvm-optzns");

   context.args.AddLastArg(arguments, options::OPT_parse_stdlib);

   arguments.push_back("-module-name");
   arguments.push_back(context.args.MakeArgString(context.outputInfo.moduleName));

   // Add the output file argument if necessary.
   if (context.output.getPrimaryOutputType() != filetypes::TY_Nothing) {
      for (auto FileName : context.output.getPrimaryOutputFilenames()) {
         arguments.push_back("-o");
         arguments.push_back(context.args.MakeArgString(FileName));
      }
   }
   return {POLARPHP_EXECUTABLE_NAME, arguments};
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const MergeModuleJobAction &job,
                               const JobContext &context) const {
   InvocationInfo invocationInfo{POLARPHP_EXECUTABLE_NAME};
   ArgStringList &arguments = invocationInfo.arguments;
   invocationInfo.allowsResponseFiles = true;

   for (auto &s : getDriver().getPolarphpProgramArgs()) {
      arguments.push_back(s.c_str());
   }
   arguments.push_back("-frontend");

   arguments.push_back("-merge-modules");
   arguments.push_back("-emit-module");

   if (context.shouldUseInputFileList()) {
      arguments.push_back("-filelist");
      arguments.push_back(context.getTemporaryFilePath("inputs", ""));
      invocationInfo.filelistInfos.push_back({arguments.back(),
                                              filetypes::TY_PolarModuleFile,
                                              FilelistInfo::WhichFiles::Input});

      addInputsOfType(arguments, context.inputActions,
                      filetypes::TY_PolarModuleFile);
   } else {
      size_t origLen = arguments.size();
      (void)origLen;
      addInputsOfType(arguments, context.inputs, context.args,
                      filetypes::TY_PolarModuleFile);
      addInputsOfType(arguments, context.inputActions,
                      filetypes::TY_PolarModuleFile);
      assert(arguments.size() - origLen >=
             context.inputs.size() + context.inputActions.size() ||
             context.outputInfo.compilerOutputType == filetypes::TY_Nothing);
      assert((arguments.size() - origLen == context.inputs.size() ||
              context.outputInfo.compilerOutputType == filetypes::TY_Nothing ||
              !context.inputActions.empty()) &&
             "every input to MergeModule must generate a swiftmodule");
   }

   // Tell all files to parse as library, which is necessary to load them as
   // serialized ASTs.
   arguments.push_back("-parse-as-library");

   // Merge serialized PIL from partial modules.
   arguments.push_back("-sil-merge-partial-modules");

   // Disable PIL optimization passes; we've already optimized the code in each
   // partial mode.
   arguments.push_back("-disable-diagnostic-passes");
   arguments.push_back("-disable-sil-perf-optzns");

   add_common_frontend_args(*this, context.outputInfo, context.output, context.args,
                            arguments);
   add_outputs_of_type(arguments, context.output, context.args,
                       filetypes::TY_PolarModuleDocFile, "-emit-module-doc-path");
   add_outputs_of_type(arguments, context.output, context.args,
                       filetypes::FileTypeId::TY_PolarParseableInterfaceFile,
                       "-emit-parseable-module-interface-path");
   add_outputs_of_type(arguments, context.output, context.args,
                       filetypes::TY_SerializedDiagnostics,
                       "-serialize-diagnostics-path");
   add_outputs_of_type(arguments, context.output, context.args,
                       filetypes::TY_ObjCHeader, "-emit-objc-header-path");
   add_outputs_of_type(arguments, context.output, context.args, filetypes::TY_TBD,
                       "-emit-tbd-path");
   arguments.push_back("-module-name");
   arguments.push_back(context.args.MakeArgString(context.outputInfo.moduleName));

   assert(context.output.getPrimaryOutputType() ==
          filetypes::TY_PolarModuleFile &&
          "The MergeModule tool only produces swiftmodule files!");
   arguments.push_back("-o");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));
   return invocationInfo;
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const ModuleWrapJobAction &job,
                               const JobContext &context) const
{
   InvocationInfo invocationInfo{POLARPHP_EXECUTABLE_NAME};
   ArgStringList &arguments = invocationInfo.arguments;
   invocationInfo.allowsResponseFiles = true;

   for (auto &s : getDriver().getPolarphpProgramArgs()) {
      arguments.push_back(s.c_str());
   }
   arguments.push_back("-modulewrap");
   addInputsOfType(arguments, context.inputs, context.args,
                   filetypes::TY_PolarModuleFile);
   addInputsOfType(arguments, context.inputActions,
                   filetypes::TY_PolarModuleFile);
   assert(arguments.size() == 2 &&
          "ModuleWrap expects exactly one merged swiftmodule as input");
   assert(context.output.getPrimaryOutputType() == filetypes::TY_Object &&
          "The -modulewrap mode only produces object files");
   arguments.push_back("-target");
   arguments.push_back(context.args.MakeArgString(getTriple().str()));
   arguments.push_back("-o");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));
   return invocationInfo;
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const REPLJobAction &job,
                               const JobContext &context) const
{
   assert(context.inputs.empty());
   assert(context.inputActions.empty());

   bool useLLDB;

   switch (job.getRequestedMode()) {
   case REPLJobAction::Mode::Integrated:
      useLLDB = false;
      break;
   case REPLJobAction::Mode::RequireLLDB:
      useLLDB = true;
      break;
   case REPLJobAction::Mode::PreferLLDB:
      useLLDB = !findProgramRelativeToSwift("lldb").empty();
      break;
   }

   ArgStringList FrontendArgs;
   for (auto &s : getDriver().getPolarphpProgramArgs())
      FrontendArgs.push_back(s.c_str());
   add_common_frontend_args(*this, context.outputInfo, context.output, context.args,
                            FrontendArgs);
   context.args.AddAllArgs(FrontendArgs, options::OPT_l, options::OPT_framework,
                           options::OPT_L);

   if (!useLLDB) {
      FrontendArgs.insert(FrontendArgs.begin(), {"-frontend", "-repl"});
      FrontendArgs.push_back("-module-name");
      FrontendArgs.push_back(context.args.MakeArgString(context.outputInfo.moduleName));
      return {POLARPHP_EXECUTABLE_NAME, FrontendArgs};
   }

   // Squash important frontend options into a single argument for LLDB.
   std::string SingleArg = "--repl=";
   {
      llvm::raw_string_ostream os(SingleArg);
      Job::printArguments(os, FrontendArgs);
   }

   ArgStringList arguments;
   arguments.push_back(context.args.MakeArgString(std::move(SingleArg)));
   return {"lldb", arguments};
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const GenerateDSYMJobAction &job,
                               const JobContext &context) const
{
   assert(context.inputs.size() == 1);
   assert(context.inputActions.empty());
   assert(context.output.getPrimaryOutputType() == filetypes::TY_dSYM);

   ArgStringList arguments;

   auto inputPath =
         context.inputs.front()->getOutput().getPrimaryOutputFilename();
   arguments.push_back(context.args.MakeArgString(inputPath));

   arguments.push_back("-o");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));

   return {"dsymutil", arguments};
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const VerifyDebugInfoJobAction &job,
                               const JobContext &context) const
{
   assert(context.inputs.size() == 1);
   assert(context.inputActions.empty());

   // This mirrors the clang driver's --verify-debug-info option.
   ArgStringList arguments;
   arguments.push_back("--verify");
   arguments.push_back("--debug-info");
   arguments.push_back("--eh-frame");
   arguments.push_back("--quiet");

   auto inputPath =
         context.inputs.front()->getOutput().getPrimaryOutputFilename();
   arguments.push_back(context.args.MakeArgString(inputPath));

   return {"dwarfdump", arguments};
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const GeneratePCHJobAction &job,
                               const JobContext &context) const
{
   assert(context.inputs.empty());
   assert(context.inputActions.size() == 1);
   assert((!job.isPersistentPCH() &&
           context.output.getPrimaryOutputType() == filetypes::TY_PCH) ||
          (job.isPersistentPCH() &&
           context.output.getPrimaryOutputType() == filetypes::TY_Nothing));

   InvocationInfo invocationInfo{POLARPHP_EXECUTABLE_NAME};
   ArgStringList &arguments = invocationInfo.arguments;
   invocationInfo.allowsResponseFiles = true;

   for (auto &s : getDriver().getPolarphpProgramArgs()) {
      arguments.push_back(s.c_str());
   }
   arguments.push_back("-frontend");
   add_common_frontend_args(*this, context.outputInfo, context.output, context.args,
                            arguments);
   add_outputs_of_type(arguments, context.output, context.args,
                       filetypes::TY_SerializedDiagnostics,
                       "-serialize-diagnostics-path");

   addInputsOfType(arguments, context.inputActions, filetypes::TY_ObjCHeader);
   context.args.AddLastArg(arguments, options::OPT_index_store_path);
   if (job.isPersistentPCH()) {
      arguments.push_back("-emit-pch");
      arguments.push_back("-pch-output-dir");
      arguments.push_back(context.args.MakeArgString(job.getPersistentPCHDir()));
   } else {
      arguments.push_back("-emit-pch");
      arguments.push_back("-o");
      arguments.push_back(
               context.args.MakeArgString(context.output.getPrimaryOutputFilename()));
   }
   return invocationInfo;
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const AutolinkExtractJobAction &job,
                               const JobContext &context) const
{
   llvm_unreachable("autolink extraction not implemented for this toolchain");
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const DynamicLinkJobAction &job,
                               const JobContext &context) const
{
   llvm_unreachable("linking not implemented for this toolchain");
}

ToolChain::InvocationInfo
ToolChain::constructInvocation(const StaticLinkJobAction &job,
                               const JobContext &context) const
{
   llvm_unreachable("archiving not implemented for this toolchain");
}

void ToolChain::addPathEnvironmentVariableIfNeeded(
      Job::EnvironmentVector &env, const char *name, const char *separator,
      options::ID optionID, const ArgList &args,
      ArrayRef<std::string> extraEntries) const
{
   auto linkPathOptions = args.filtered(optionID);
   if (linkPathOptions.begin() == linkPathOptions.end() && extraEntries.empty()) {
      return;
   }

   std::string newPaths;
   interleave(linkPathOptions,
              [&](const Arg *arg) { newPaths.append(arg->getValue()); },
   [&] { newPaths.append(separator); });
   for (auto extraEntry : extraEntries) {
      if (!newPaths.empty()) {
         newPaths.append(separator);
      }
      newPaths.append(extraEntry.data(), extraEntry.size());
   }
   if (auto currentPaths = llvm::sys::Process::GetEnv(name)) {
      newPaths.append(separator);
      newPaths.append(currentPaths.getValue());
   }
   env.emplace_back(name, args.MakeArgString(newPaths));
}

void ToolChain::addLinkRuntimeLib(const ArgList &args, ArgStringList &arguments,
                                  StringRef LibName) const
{
   SmallString<128> P;
   getClangLibraryPath(args, P);
   llvm::sys::path::append(P, LibName);
   arguments.push_back(args.MakeArgString(P));
}

void ToolChain::getClangLibraryPath(const ArgList &args,
                                    SmallString<128> &libPath) const
{
   const llvm::Triple &triple = getTriple();

   getResourceDirPath(libPath, args, /*Shared=*/true);
   // Remove platform name.
   llvm::sys::path::remove_filename(libPath);
   llvm::sys::path::append(libPath, "clang", "lib",
                           triple.isOSDarwin() ? "darwin"
                                               : polar::basic::get_platform_name_for_triple(triple));
}

/// Get the runtime library link path, which is platform-specific and found
/// relative to the compiler.
void ToolChain::getResourceDirPath(SmallVectorImpl<char> &resourceDirPath,
                                   const llvm::opt::ArgList &args,
                                   bool shared) const
{
   // FIXME: Duplicated from CompilerInvocation, but in theory the runtime
   // library link path and the standard library module import path don't
   // need to be the same.
   if (const Arg *A = args.getLastArg(options::OPT_resource_dir)) {
      StringRef value = A->getValue();
      resourceDirPath.append(value.begin(), value.end());
   } else {
      auto programPath = getDriver().getPolarphpProgramPath();
      resourceDirPath.append(programPath.begin(), programPath.end());
      llvm::sys::path::remove_filename(resourceDirPath); // remove /swift
      llvm::sys::path::remove_filename(resourceDirPath); // remove /bin
      llvm::sys::path::append(resourceDirPath, "lib",
                              shared ? "swift" : "swift_static");
   }
   llvm::sys::path::append(resourceDirPath,
                           polar::basic::get_platform_name_for_triple(getTriple()));
}

void ToolChain::getRuntimeLibraryPaths(SmallVectorImpl<std::string> &runtimeLibPaths,
                                       const llvm::opt::ArgList &args,
                                       StringRef sdkPath, bool shared) const
{
   SmallString<128> scratchPath;
   getResourceDirPath(scratchPath, args, shared);
   runtimeLibPaths.push_back(scratchPath.str());
   if (!sdkPath.empty()) {
      scratchPath = sdkPath;
      llvm::sys::path::append(scratchPath, "usr", "lib", "polarphp");
      runtimeLibPaths.push_back(scratchPath.str());
   }
}

bool ToolChain::sanitizerRuntimeLibExists(const ArgList &args,
                                          StringRef sanitizerName,
                                          bool shared) const
{
   SmallString<128> sanitizerLibPath;
   getClangLibraryPath(args, sanitizerLibPath);
   llvm::sys::path::append(sanitizerLibPath,
                           sanitizerRuntimeLibName(sanitizerName, shared));
   return llvm::sys::fs::exists(sanitizerLibPath.str());
}

} // polar::driver
