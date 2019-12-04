//===--- Driver.cpp - Swift compiler driver -------------------------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2019/12/02.
//
// This file contains implementations of parts of the compiler driver.
//
//===----------------------------------------------------------------------===//


#include "polarphp/driver/Driver.h"

#include "polarphp/driver/internal/ToolChains.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsDriver.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/OutputFileMap.h"
#include "polarphp/basic/Range.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/TaskQueue.h"
#include "polarphp/kernel/Version.h"
#include "polarphp/global/Config.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Compilation.h"
#include "polarphp/driver/Job.h"
#include "polarphp/driver/PrettyStackTrace.h"
#include "polarphp/driver/ToolChain.h"
#include "polarphp/option/Options.h"
#include "polarphp/option/SanitizerOptions.h"
#include "polarphp/parser/Lexer.h"
#include "polarphp/global/NameStrings.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Option/OptTable.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/MD5.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/raw_ostream.h"
#include "polarphp/driver/internal/CompilationRecord.h"

#include <memory>

namespace polar::driver {

using namespace polar::basic;
using namespace polar::ast;
using namespace polar::version;
using namespace llvm;
using namespace llvm::opt;
using polar::parser::Lexer;

Driver::Driver(StringRef driverExecutable,
               StringRef name,
               ArrayRef<const char *> args,
               DiagnosticEngine &diags)
   : m_opts(create_polarphp_opt_table()),
     m_diags(diags),
     m_name(name),
     m_driverExecutable(driverExecutable),
     m_defaultTargetTriple(llvm::sys::getDefaultTargetTriple())
{
   // The driver kind must be parsed prior to parsing arguments, since that
   // affects how arguments are parsed.
   parseDriverKind(args.slice(1));
}

Driver::~Driver() = default;

void Driver::parseDriverKind(ArrayRef<const char *> args)
{
   // The default driver kind is determined by name.
   StringRef driverName = m_name;

   std::string optName;
   // However, the driver kind may be overridden if the first argument is
   // --driver-mode.
   if (!args.empty()) {
      optName = getOpts().getOption(options::OPT_driver_mode).getPrefixedName();
      StringRef firstArg(args[0]);
      if (firstArg.startswith(optName)) {
         driverName = firstArg.drop_front(optName.size());
      }
   }

   Optional<DriverKind> kind =
         llvm::StringSwitch<Optional<DriverKind>>(driverName)
                                                 .Case("polarphp", DriverKind::Interactive)
                                                 .Case("polarphpc", DriverKind::Batch)
                                                 .Case("polarphp-autolink-extract", DriverKind::AutolinkExtract)
                                                 .Case("polarphp-format", DriverKind::PolarphpFormat)
                                                 .Default(None);

   if (kind.hasValue()) {
      m_driverKind = kind.getValue();
   } else if (!optName.empty()) {
      m_diags.diagnose({}, diag::error_invalid_arg_value, optName, driverName);
   }
}

ArrayRef<const char *> Driver::getArgsWithoutProgramNameAndDriverMode(
      ArrayRef<const char *> args) const
{
   args = args.slice(1);
   if (args.empty()) {
      return args;
   }
   const std::string optName =
         getOpts().getOption(options::OPT_driver_mode).getPrefixedName();
   if (StringRef(args[0]).startswith(optName)) {
      args = args.slice(1);
   }
   return args;
}

static void validate_bridging_header_args(DiagnosticEngine &diags,
                                          const ArgList &args)
{
   //   if (!args.hasArgNoClaim(options::OPT_import_objc_header))
   //      return;
   if (args.hasArgNoClaim(options::OPT_import_underlying_module)) {
      diags.diagnose({}, diag::error_framework_bridging_header);
   }
   if (args.hasArgNoClaim(options::OPT_emit_module_interface,
                          options::OPT_emit_module_interface_path)) {
      //      diags.diagnose({}, diag::error_bridging_header_module_interface);
   }
}

static void validate_deployment_target(DiagnosticEngine &diags,
                                       const ArgList &args)
{
   const Arg *arg = args.getLastArg(options::OPT_target);
   if (!arg)
      return;

   // Check minimum supported ostream versions.
   llvm::Triple triple(llvm::Triple::normalize(arg->getValue()));
   if (triple.isMacOSX()) {
      if (triple.isMacOSXVersionLT(10, 9)) {
         diags.diagnose(SourceLoc(), diag::error_os_minimum_deployment,
                        "ostream X 10.9");
      }
   } else if (triple.isiOS()) {
      if (triple.isTvOS()) {
         if (triple.isOSVersionLT(9, 0)) {
            diags.diagnose(SourceLoc(), diag::error_os_minimum_deployment,
                           "tvOS 9.0");
            return;
         }
      }
      if (triple.isOSVersionLT(7)) {
         diags.diagnose(SourceLoc(), diag::error_os_minimum_deployment,
                        "iOS 7");
      }
      if (triple.isArch32Bit() && !triple.isOSVersionLT(11)) {
         diags.diagnose(SourceLoc(), diag::error_ios_maximum_deployment_32,
                        triple.getOSMajorVersion());
      }
   } else if (triple.isWatchOS()) {
      if (triple.isOSVersionLT(2, 0)) {
         diags.diagnose(SourceLoc(), diag::error_os_minimum_deployment,
                        "watchOS 2.0");
         return;
      }
   }
}

static void validate_warning_control_args(DiagnosticEngine &diags,
                                          const ArgList &args)
{
   if (args.hasArg(options::OPT_suppress_warnings) &&
       args.hasArg(options::OPT_warnings_as_errors)) {
      diags.diagnose(SourceLoc(), diag::error_conflicting_options,
                     "-warnings-as-errors", "-suppress-warnings");
   }
}

static void validate_profiling_args(DiagnosticEngine &diags,
                                    const ArgList &args)
{
   const Arg *profileGenerate = args.getLastArg(options::OPT_profile_generate);
   const Arg *profileUse = args.getLastArg(options::OPT_profile_use);
   if (profileGenerate && profileUse) {
      diags.diagnose(SourceLoc(), diag::error_conflicting_options,
                     "-profile-generate", "-profile-use");
   }

   // Check if the profdata is missing
   if (profileUse && !llvm::sys::fs::exists(profileUse->getValue())) {
      diags.diagnose(SourceLoc(), diag::error_profile_missing,
                     profileUse->getValue());
   }
}

static void validate_debug_info_args(DiagnosticEngine &diags,
                                     const ArgList &args)
{
   // Check for missing debug option when verifying debug info.
   if (args.hasArg(options::OPT_verify_debug_info)) {
      Arg *debugOpt = args.getLastArg(polar::options::OPT_g_Group);
      if (!debugOpt || debugOpt->getOption().matches(polar::options::OPT_gnone)) {
         diags.diagnose(SourceLoc(),
                        diag::verify_debug_info_requires_debug_option);
      }
   }

   // Check for any -debug-prefix-map options that aren't of the form
   // 'original=remapped' (either side can be empty, however).
   for (auto arg : args.getAllArgValues(options::OPT_debug_prefix_map)) {
      if (arg.find('=') == StringRef::npos) {
         diags.diagnose(SourceLoc(), diag::error_invalid_debug_prefix_map, arg);
      }
   }
}

static void validate_compilation_condition_args(DiagnosticEngine &diags,
                                                const ArgList &args)
{
   for (const Arg *arg : args.filtered(options::OPT_D)) {
      StringRef name = arg->getValue();
      if (name.find('=') != StringRef::npos) {
         diags.diagnose(SourceLoc(),
                        diag::cannot_assign_value_to_conditional_compilation_flag,
                        name);
      } else if (name.startswith("-driver")) {
         diags.diagnose(SourceLoc(), diag::redundant_prefix_compilation_flag,
                        name);
      } else if (!Lexer::isIdentifier(name)) {
         diags.diagnose(SourceLoc(), diag::invalid_conditional_compilation_flag,
                        name);
      }
   }
}

static void validate_autolinking_args(DiagnosticEngine &diags,
                                      const ArgList &args)
{
   auto *forceLoadArg = args.getLastArg(options::OPT_autolink_force_load);
   if (!forceLoadArg) {
      return;
   }

   auto *incrementalArg = args.getLastArg(options::OPT_incremental);
   if (!incrementalArg) {
      return;
   }
   // Note: -incremental can itself be overridden by other arguments later
   // on, but since -autolink-force-load is a rare and not-really-recommended
   // option it's not worth modeling that complexity here (or moving the
   // check somewhere else).
   diags.diagnose(SourceLoc(), diag::error_option_not_supported,
                  forceLoadArg->getSpelling(), incrementalArg->getSpelling());
}

/// Perform miscellaneous early validation of arguments.
static void validate_args(DiagnosticEngine &diags, const ArgList &args)
{
   validate_bridging_header_args(diags, args);
   validate_deployment_target(diags, args);
   validate_warning_control_args(diags, args);
   validate_profiling_args(diags, args);
   validate_debug_info_args(diags, args);
   validate_compilation_condition_args(diags, args);
   validate_autolinking_args(diags, args);
}

std::unique_ptr<ToolChain>
Driver::buildToolChain(const llvm::opt::InputArgList &argList)
{

   if (const Arg *arg = argList.getLastArg(options::OPT_target)) {
      m_defaultTargetTriple = llvm::Triple::normalize(arg->getValue());
   }

   const llvm::Triple target(m_defaultTargetTriple);

   switch (target.getOS()) {
   case llvm::Triple::Darwin:
   case llvm::Triple::MacOSX:
   case llvm::Triple::IOS:
   case llvm::Triple::TvOS:
   case llvm::Triple::WatchOS:
      return std::make_unique<toolchains::Darwin>(*this, target);
   case llvm::Triple::Linux:
      if (target.isAndroid())
         return std::make_unique<toolchains::Android>(*this, target);
      return std::make_unique<toolchains::GenericUnix>(*this, target);
   case llvm::Triple::FreeBSD:
      return std::make_unique<toolchains::GenericUnix>(*this, target);
   case llvm::Triple::Win32:
      if (target.isWindowsCygwinEnvironment())
         return std::make_unique<toolchains::Cygwin>(*this, target);
      return std::make_unique<toolchains::Windows>(*this, target);
   case llvm::Triple::Haiku:
      return std::make_unique<toolchains::GenericUnix>(*this, target);
   default:
      m_diags.diagnose(SourceLoc(), diag::error_unknown_target,
                       argList.getLastArg(options::OPT_target)->getValue());
      break;
   }
   return nullptr;
}

std::unique_ptr<sys::TaskQueue> Driver::buildTaskQueue(const Compilation &compilation)
{
   const auto &argList = compilation.getArgs();
   unsigned numberOfParallelCommands = 1;
   if (const Arg *arg = argList.getLastArg(options::OPT_j)) {
      if (StringRef(arg->getValue()).getAsInteger(10, numberOfParallelCommands)) {
         m_diags.diagnose(SourceLoc(), diag::error_invalid_arg_value,
                          arg->getAsString(argList), arg->getValue());
         return nullptr;
      }
   }
   if (environment_variable_requested_maximum_determinism()) {
      numberOfParallelCommands = 1;
      m_diags.diagnose(SourceLoc(), diag::remark_max_determinism_overriding,
                       "-j");
   }

   const bool driverSkipExecution =
         argList.hasArg(options::OPT_driver_skip_execution,
                        options::OPT_driver_print_jobs);
   if (driverSkipExecution) {
      return std::make_unique<sys::DummyTaskQueue>(numberOfParallelCommands);
   } else {
      return std::make_unique<sys::TaskQueue>(numberOfParallelCommands,
                                              compilation.getStatsReporter());
   }
}

static void compute_args_hash(SmallString<32> &out, const DerivedArgList &args)
{
   SmallVector<const Arg *, 32> interestingArgs;
   interestingArgs.reserve(args.size());
   std::copy_if(args.begin(), args.end(), std::back_inserter(interestingArgs),
                [](const Arg *arg) {
      return !arg->getOption().hasFlag(options::DoesNotAffectIncrementalBuild) &&
            arg->getOption().getKind() != Option::InputClass;
   });

   llvm::array_pod_sort(interestingArgs.begin(), interestingArgs.end(),
                        [](const Arg * const *lhs, const Arg * const *rhs)->int {
      auto cmpID = (*lhs)->getOption().getID() - (*rhs)->getOption().getID();
      if (cmpID != 0) {
         return cmpID;
      }
      return (*lhs)->getIndex() - (*rhs)->getIndex();
   });

   llvm::MD5 hash;
   for (const Arg *arg : interestingArgs) {
      hash.update(arg->getOption().getID());
      for (const char *value : const_cast<Arg *>(arg)->getValues()) {
         hash.update(value);
      }
   }

   llvm::MD5::MD5Result hashBuf;
   hash.final(hashBuf);
   llvm::MD5::stringifyResult(hashBuf, out);
}

class Driver::InputInfoMap
      : public llvm::SmallDenseMap<const Arg *, CompileJobAction::InputInfo, 16>
{
};

using InputInfoMap = Driver::InputInfoMap;

/// Get the filename for build record. Returns true if failed.
/// Additionally, set 'outputBuildRecordForModuleOnlyBuild' to true if this is
/// full compilation with swiftmodule.
static bool get_compilationrecord_path(std::string &buildRecordPath,
                                       bool &outputBuildRecordForModuleOnlyBuild,
                                       const OutputInfo &outputInfo,
                                       const Optional<OutputFileMap> &optionalOutputInfo,
                                       DiagnosticEngine *diags)
{
   if (!optionalOutputInfo) {
      // FIXME: This should work without an output file map. We should have
      // another way to specify a build record and where to put intermediates.
      if (diags) {
         diags->diagnose(SourceLoc(), diag::incremental_requires_output_file_map);
      }
      return true;
   }

   if (auto *masterOutputMap = optionalOutputInfo->getOutputMapForSingleOutput()) {
      buildRecordPath = masterOutputMap->lookup(filetypes::TY_PolarDeps);
   }

   if (buildRecordPath.empty()) {
      if (diags) {
         diags->diagnose(SourceLoc(),
                         diag::incremental_requires_build_record_entry,
                         filetypes::get_type_name(filetypes::TY_PolarDeps));
      }
      return true;
   }

   // In 'emit-module' only mode, use build-record filename suffixed with
   // '~moduleonly'. So that module-only mode doesn't mess up build-record
   // file for full compilation.
   if (outputInfo.compilerOutputType == filetypes::TY_PolarModuleFile) {
      buildRecordPath = buildRecordPath.append("~moduleonly");
   } else if (outputInfo.shouldTreatModuleAsTopLevelOutput) {
      // If we emit module along with full compilation, emit build record
      // file for '-emit-module' only mode as well.
      outputBuildRecordForModuleOnlyBuild = true;
   }

   return false;
}

static bool failed_to_read_out_of_date_map(bool showIncrementalBuildDecisions,
                                           StringRef buildRecordPath,
                                           StringRef reason = "")
{
   if (showIncrementalBuildDecisions) {
      llvm::outs() << "incremental compilation has been disabled due to "
                   << "malformed build record file '" << buildRecordPath << "'.";
      if (!reason.empty()) {
         llvm::outs() << " " << reason;
      }
      llvm::outs() << "\n";
   }
   return true;
}

/// Returns true on error.
static bool populate_out_of_date_map(InputInfoMap &map,
                                     llvm::sys::TimePoint<> &lastBuildTime,
                                     StringRef argsHashStr,
                                     const InputFileList &inputs,
                                     StringRef buildRecordPath,
                                     bool showIncrementalBuildDecisions)
{
   // Treat a missing file as "no previous build".
   auto buffer = llvm::MemoryBuffer::getFile(buildRecordPath);
   if (!buffer) {
      if (showIncrementalBuildDecisions) {
         llvm::outs() << "incremental compilation could not read build record.\n";
      }
      return false;
   }

   namespace yaml = llvm::yaml;
   using InputInfo = CompileJobAction::InputInfo;

   llvm::SourceMgr SM;
   yaml::Stream stream(buffer.get()->getMemBufferRef(), SM);

   auto iter = stream.begin();
   if (iter == stream.end() || !iter->getRoot()) {
      return failed_to_read_out_of_date_map(showIncrementalBuildDecisions,
                                            buildRecordPath);
   }
   auto *topLevelMap = dyn_cast<yaml::MappingNode>(iter->getRoot());
   if (!topLevelMap) {
      return failed_to_read_out_of_date_map(showIncrementalBuildDecisions,
                                            buildRecordPath);
   }

   SmallString<64> scratch;

   llvm::StringMap<InputInfo> previousInputs;
   bool versionValid = false;
   bool optionsMatch = true;

   auto readTimeValue = [&scratch](yaml::Node *node,
         llvm::sys::TimePoint<> &timeValue) -> bool {
      auto *seq = dyn_cast<yaml::SequenceNode>(node);
      if (!seq) {
         return true;
      }

      auto seqI = seq->begin(), seqE = seq->end();
      if (seqI == seqE) {
         return true;
      }

      auto *secondsRaw = dyn_cast<yaml::ScalarNode>(&*seqI);
      if (!secondsRaw)
         return true;
      std::time_t parsedSeconds;
      if (secondsRaw->getValue(scratch).getAsInteger(10, parsedSeconds)) {
         return true;
      }

      ++seqI;
      if (seqI == seqE) {
         return true;
      }
      auto *nanosecondsRaw = dyn_cast<yaml::ScalarNode>(&*seqI);
      if (!nanosecondsRaw) {
         return true;
      }

      std::chrono::system_clock::rep parsedNanoseconds;
      if (nanosecondsRaw->getValue(scratch).getAsInteger(10, parsedNanoseconds)) {
         return true;
      }
      ++seqI;
      if (seqI != seqE) {
         return true;
      }

      timeValue = llvm::sys::TimePoint<>(std::chrono::seconds(parsedSeconds));
      timeValue += std::chrono::nanoseconds(parsedNanoseconds);
      return false;
   };

   // FIXME: LLVM's YAML support does incremental parsing in such a way that
   // for-range loops break.
   SmallString<64> CompilationRecordSwiftVersion;
   for (auto i = topLevelMap->begin(), e = topLevelMap->end(); i != e; ++i) {
      auto *key = cast<yaml::ScalarNode>(i->getKey());
      StringRef keyStr = key->getValue(scratch);
      using compilationrecord::TopLevelKey;
      if (keyStr == compilationrecord::get_name(TopLevelKey::Version)) {
         auto *value = dyn_cast<yaml::ScalarNode>(i->getValue());
         if (!value) {
            auto reason = ("Malformed value for key '" + keyStr + "'.")
                  .toStringRef(scratch);
            return failed_to_read_out_of_date_map(showIncrementalBuildDecisions,
                                                  buildRecordPath, reason);
         }

         // NB: We check against
         // swift::version::Version::getCurrentLanguageVersion() here because any
         // -polarphp-version argument is handled in the argsHashStr check that
         // follows.
         CompilationRecordSwiftVersion = value->getValue(scratch);
         versionValid = (CompilationRecordSwiftVersion
                         == version::retrieve_polarphp_full_version(
                            version::Version::getCurrentLanguageVersion()));

      } else if (keyStr == compilationrecord::get_name(TopLevelKey::Options)) {
         auto *value = dyn_cast<yaml::ScalarNode>(i->getValue());
         if (!value) {
            return true;
         }
         optionsMatch = (argsHashStr == value->getValue(scratch));

      } else if (keyStr == compilationrecord::get_name(TopLevelKey::BuildTime)) {
         auto *value = dyn_cast<yaml::SequenceNode>(i->getValue());
         if (!value) {
            auto reason = ("Malformed value for key '" + keyStr + "'.")
                  .toStringRef(scratch);
            return failed_to_read_out_of_date_map(showIncrementalBuildDecisions,
                                                  buildRecordPath, reason);
         }
         llvm::sys::TimePoint<> timeVal;
         if (readTimeValue(i->getValue(), timeVal)) {
            return true;
         }
         lastBuildTime = timeVal;

      } else if (keyStr == compilationrecord::get_name(TopLevelKey::Inputs)) {
         auto *inputMap = dyn_cast<yaml::MappingNode>(i->getValue());
         if (!inputMap) {
            auto reason = ("Malformed value for key '" + keyStr + "'.")
                  .toStringRef(scratch);
            return failed_to_read_out_of_date_map(showIncrementalBuildDecisions,
                                                  buildRecordPath, reason);
         }

         // FIXME: LLVM's YAML support does incremental parsing in such a way that
         // for-range loops break.
         for (auto i = inputMap->begin(), e = inputMap->end(); i != e; ++i) {
            auto *key = dyn_cast<yaml::ScalarNode>(i->getKey());
            if (!key) {
               return true;
            }

            auto *value = dyn_cast<yaml::SequenceNode>(i->getValue());
            if (!value) {
               return true;
            }
            using compilationrecord::get_info_statu_for_identifier;
            auto previousBuildState =
                  get_info_statu_for_identifier(value->getRawTag());
            if (!previousBuildState) {
               return true;
            }

            llvm::sys::TimePoint<> timeValue;
            if (readTimeValue(value, timeValue)) {
               return true;
            }
            auto inputName = key->getValue(scratch);
            previousInputs[inputName] = { *previousBuildState, timeValue };
         }
      }
   }

   if (!versionValid) {
      if (showIncrementalBuildDecisions) {
         auto v = version::retrieve_polarphp_full_version(
                  version::Version::getCurrentLanguageVersion());
         llvm::outs() << "incremental compilation has been disabled, due to a "
                      << "compiler version mismatch.\n"
                      << "\tCompiling with: " << v << "\n"
                      << "\tPreviously compiled with: "
                      << CompilationRecordSwiftVersion << "\n";
      }
      return true;
   }

   if (!optionsMatch) {
      if (showIncrementalBuildDecisions) {
         llvm::outs() << "incremental compilation has been disabled, because "
                      << "different arguments were passed to the compiler.\n";
      }
      return true;
   }

   size_t numInputsFromPrevious = 0;
   for (auto &inputPair : inputs) {
      auto iter = previousInputs.find(inputPair.second->getValue());
      if (iter == previousInputs.end()) {
         map[inputPair.second] = InputInfo::makeNewlyAdded();
         continue;
      }
      ++numInputsFromPrevious;
      map[inputPair.second] = iter->getValue();
   }

   if (numInputsFromPrevious == previousInputs.size()) {
      return false;
   } else {
      // If a file was removed, we've lost its dependency info. Rebuild everything.
      // FIXME: Can we do better?
      if (showIncrementalBuildDecisions) {
         llvm::DenseSet<StringRef> inputArgs;
         for (auto &inputPair : inputs) {
            inputArgs.insert(inputPair.second->getValue());
         }

         SmallVector<StringRef, 8> missingInputs;
         for (auto &previousInput : previousInputs) {
            auto previousInputArg = previousInput.getKey();
            if (inputArgs.find(previousInputArg) == inputArgs.end()) {
               missingInputs.push_back(previousInputArg);
            }
         }

         llvm::outs() << "incremental compilation has been disabled, because "
                      << "the following inputs were used in the previous "
                      << "compilation, but not in the current compilation:\n";
         for (auto &missing : missingInputs) {
            llvm::outs() << "\t" << missing << "\n";
         }
      }
      return true;
   }
}

// warn if -embed-bitcode is set and the output type is not an object
static void validate_embed_bitcode(DerivedArgList &args, const OutputInfo &outputInfo,
                                   DiagnosticEngine &diags)
{
   if (args.hasArg(options::OPT_embed_bitcode) &&
       outputInfo.compilerOutputType != filetypes::TY_Object) {
      diags.diagnose(SourceLoc(), diag::warn_ignore_embed_bitcode);
      args.eraseArg(options::OPT_embed_bitcode);
   }

   if (args.hasArg(options::OPT_embed_bitcode_marker) &&
       outputInfo.compilerOutputType != filetypes::TY_Object) {
      diags.diagnose(SourceLoc(), diag::warn_ignore_embed_bitcode_marker);
      args.eraseArg(options::OPT_embed_bitcode_marker);
   }
}

/// Gets the filelist threshold to use. Diagnoses and returns true on error.
static bool get_filelist_threshold(DerivedArgList &args, size_t &filelistThreshold,
                                   DiagnosticEngine &diags)
{
   filelistThreshold = 128;

   // claim and diagnose deprecated -driver-use-filelists
   bool hasUseFilelists = args.hasArg(options::OPT_driver_use_filelists);
   if (hasUseFilelists)
      diags.diagnose(SourceLoc(), diag::warn_use_filelists_deprecated);

   if (const Arg *arg = args.getLastArg(options::OPT_driver_filelist_threshold)) {
      // Use the supplied threshold
      if (StringRef(arg->getValue()).getAsInteger(10, filelistThreshold)) {
         diags.diagnose(SourceLoc(), diag::error_invalid_arg_value,
                        arg->getAsString(args), arg->getValue());
         return true;
      }
   } else if (hasUseFilelists) {
      // Treat -driver-use-filelists as -driver-filelist-threshold=0
      filelistThreshold = 0;
   } // else stick with the default

   return false;
}

static unsigned
get_driver_batch_seed(llvm::opt::InputArgList &argList,
                      DiagnosticEngine &diags)
{
   unsigned driverBatchSeed = 0;
   if (const Arg *arg = argList.getLastArg(options::OPT_driver_batch_seed)) {
      if (StringRef(arg->getValue()).getAsInteger(10, driverBatchSeed)) {
         diags.diagnose(SourceLoc(), diag::error_invalid_arg_value,
                        arg->getAsString(argList), arg->getValue());
      }
   }
   return driverBatchSeed;
}

static Optional<unsigned>
get_driver_batch_count(llvm::opt::InputArgList &argList,
                       DiagnosticEngine &diags)
{
   if (const Arg *arg = argList.getLastArg(options::OPT_driver_batch_count)) {
      unsigned Count = 0;
      if (StringRef(arg->getValue()).getAsInteger(10, Count)) {
         diags.diagnose(SourceLoc(), diag::error_invalid_arg_value,
                        arg->getAsString(argList), arg->getValue());
      } else {
         return Count;
      }
   }
   return None;
}

static bool compute_incremental(const llvm::opt::InputArgList *argList,
                                const bool showIncrementalBuildDecisions)
{
   if (!argList->hasArg(options::OPT_incremental)) {
      return false;
   }
   const char *reasonToDisable =
         argList->hasArg(options::OPT_whole_module_optimization)
         ? "is not compatible with whole module optimization."
         : argList->hasArg(options::OPT_embed_bitcode)
           ? "is not currently compatible with embedding LLVM IR bitcode."
           : nullptr;

   if (!reasonToDisable) {
      return true;
   }

   if (showIncrementalBuildDecisions) {
      llvm::outs() << "incremental compilation has been disabled, because it "
                   << reasonToDisable;
   }
   return false;
}

static std::string
compute_working_directory(const llvm::opt::InputArgList *argList)
{
   if (auto *arg = argList->getLastArg(options::OPT_working_directory)) {
      SmallString<128> workingDirectory;
      workingDirectory = arg->getValue();
      llvm::sys::fs::make_absolute(workingDirectory);
      std::string result = workingDirectory.str().str();
      return result;
   }
   return std::string();
}

static std::unique_ptr<UnifiedStatsReporter>
create_stats_reporter(const llvm::opt::InputArgList *argList,
                      const InputFileList &inputs, const OutputInfo outputInfo,
                      StringRef defaultTargetTriple)
{
   const Arg *arg = argList->getLastArgNoClaim(options::OPT_stats_output_dir);
   if (!arg)
      return nullptr;

   StringRef optType;
   if (const Arg *OptA = argList->getLastArgNoClaim(options::OPT_O_Group)) {
      optType = OptA->getSpelling();
   }
   StringRef inputName;
   if (inputs.size() == 1) {
      inputName = inputs[0].second->getSpelling();
   }
   StringRef outputType = filetypes::get_extension(outputInfo.compilerOutputType);
   return std::make_unique<UnifiedStatsReporter>("polarphp-driver",
                                                 outputInfo.moduleName,
                                                 inputName,
                                                 defaultTargetTriple,
                                                 outputType,
                                                 optType,
                                                 arg->getValue());
}

static bool
computeContinueBuildingAfterErrors(const bool batchMode,
                                   const llvm::opt::InputArgList *argList) {
   // Note: Batch mode handling of serialized diagnostics requires that all
   // batches get to run, in order to make sure that all diagnostics emitted
   // during the compilation end up in at least one serialized diagnostic file.
   // Therefore, treat batch mode as implying -continue-building-after-errors.
   // (This behavior could be limited to only when serialized diagnostics are
   // being emitted, but this seems more consistent and less surprising for
   // users.)
   // FIXME: We don't really need (or want) a full ContinueBuildingAfterErrors.
   // If we fail to precompile a bridging header, for example, there's no need
   // to go on to compilation of source files, and if compilation of source files
   // fails, we shouldn't try to link. Instead, we'd want to let all jobs finish
   // but not schedule any new ones.
   return batchMode ||
         argList->hasArg(options::OPT_continue_building_after_errors);

}

static Optional<unsigned>
getDriverBatchSizeLimit(llvm::opt::InputArgList &argList,
                        DiagnosticEngine &diags)
{
   if (const Arg *arg = argList.getLastArg(options::OPT_driver_batch_size_limit)) {
      unsigned Limit = 0;
      if (StringRef(arg->getValue()).getAsInteger(10, Limit)) {
         diags.diagnose(SourceLoc(), diag::error_invalid_arg_value,
                        arg->getAsString(argList), arg->getValue());
      } else {
         return Limit;
      }
   }
   return None;
}

std::unique_ptr<Compilation>
Driver::buildCompilation(const ToolChain &toolchain,
                         std::unique_ptr<llvm::opt::InputArgList> argList)
{
   llvm::PrettyStackTraceString crashInfo("Compilation construction");
   llvm::sys::TimePoint<> startTime = std::chrono::system_clock::now();
   // Claim --driver-mode here, since it's already been handled.
   (void) argList->hasArg(options::OPT_driver_mode);

   m_driverPrintBindings = argList->hasArg(options::OPT_driver_print_bindings);

   const std::string workingDirectory = compute_working_directory(argList.get());

   std::unique_ptr<DerivedArgList> translatedArgList(
            translateInputAndPathArgs(*argList, workingDirectory));

   validate_args(m_diags, *translatedArgList);

   if (m_diags.hadAnyError()) {
      return nullptr;
   }

   if (!handleImmediateArgs(*translatedArgList, toolchain)) {
      return nullptr;
   }

   // Construct the list of inputs.
   InputFileList inputs;
   buildInputs(toolchain, *translatedArgList, inputs);

   if (m_diags.hadAnyError()) {
      return nullptr;
   }
   // Determine the OutputInfo for the driver.
   OutputInfo outputInfo;
   bool batchMode = false;
   outputInfo.compilerMode = computeCompilerMode(*translatedArgList, inputs, batchMode);
   buildOutputInfo(toolchain, *translatedArgList, batchMode, inputs, outputInfo);

   if (m_diags.hadAnyError()) {
      return nullptr;
   }
   assert(outputInfo.compilerOutputType != filetypes::FileTypeId::TY_INVALID &&
         "buildOutputInfo() must set a valid output type!");

   validate_embed_bitcode(*translatedArgList, outputInfo, m_diags);

   if (outputInfo.compilerMode == OutputInfo::Mode::REPL) {
      // REPL mode expects no input files, so suppress the error.
      m_suppressNoInputFilesError = true;
   }


   Optional<OutputFileMap> optionalOutputInfo =
         buildOutputFileMap(*translatedArgList, workingDirectory);

   if (m_diags.hadAnyError()) {
      return nullptr;
   }
   if (argList->hasArg(options::OPT_driver_print_output_file_map)) {
      if (optionalOutputInfo) {
         optionalOutputInfo->dump(llvm::errs(), true);
      } else {
         m_diags.diagnose(SourceLoc(), diag::error_no_output_file_map_specified);
      }
      return nullptr;
   }

   const bool showIncrementalBuildDecisions =
         argList->hasArg(options::OPT_driver_show_incremental);
   const bool incremental =
         compute_incremental(argList.get(), showIncrementalBuildDecisions);

   std::string buildRecordPath;
   bool outputBuildRecordForModuleOnlyBuild = false;
   get_compilationrecord_path(buildRecordPath, outputBuildRecordForModuleOnlyBuild,
                              outputInfo, optionalOutputInfo, incremental ? &m_diags : nullptr);

   SmallString<32> argsHash;
   compute_args_hash(argsHash, *translatedArgList);
   llvm::sys::TimePoint<> lastBuildTime = llvm::sys::TimePoint<>::min();
   InputInfoMap outOfDateMap;
   bool rebuildEverything = true;
   if (incremental && !buildRecordPath.empty()) {
      if (populate_out_of_date_map(outOfDateMap, lastBuildTime, argsHash, inputs,
                                   buildRecordPath, showIncrementalBuildDecisions)) {
         // FIXME: Distinguish errors from "file removed", which is benign.
      } else {
         rebuildEverything = false;
      }
   }

   size_t driverFilelistThreshold;
   if (get_filelist_threshold(*translatedArgList, driverFilelistThreshold, m_diags)) {
      return nullptr;
   }

   OutputLevel level = OutputLevel::Normal;
   if (const Arg *arg =
       argList->getLastArg(options::OPT_driver_print_jobs, options::OPT_v,
                           options::OPT_parseable_output)) {
      if (arg->getOption().matches(options::OPT_driver_print_jobs)) {
         level = OutputLevel::PrintJobs;
      } else if (arg->getOption().matches(options::OPT_v)) {
         level = OutputLevel::Verbose;
      } else if (arg->getOption().matches(options::OPT_parseable_output)) {
         level = OutputLevel::Parseable;
      } else {
         llvm_unreachable("Unknown OutputLevel argument!");
      }
   }

   // About to move argument list, so capture some flags that will be needed
   // later.
   const bool driverPrintActions =
         argList->hasArg(options::OPT_driver_print_actions);
   const bool DriverPrintDerivedOutputFileMap =
         argList->hasArg(options::OPT_driver_print_derived_output_file_map);
   const bool ContinueBuildingAfterErrors =
         computeContinueBuildingAfterErrors(batchMode, argList.get());
   const bool ShowJobLifecycle =
         argList->hasArg(options::OPT_driver_show_job_lifecycle);

   // In order to confine the values below, while still moving the argument
   // list, and preserving the interface to Compilation, enclose the call to the
   // constructor in a block:
   std::unique_ptr<Compilation> compilation;
   {
      const unsigned driverBatchSeed = get_driver_batch_seed(*argList, m_diags);
      const Optional<unsigned> driverBatchCount =
            get_driver_batch_count(*argList, m_diags);
      const Optional<unsigned> driverBatchSizeLimit =
            getDriverBatchSizeLimit(*argList, m_diags);
      const bool saveTemps = argList->hasArg(options::OPT_save_temps);
      const bool showDriverTimeCompilation =
            argList->hasArg(options::OPT_driver_time_compilation);
      std::unique_ptr<UnifiedStatsReporter> StatsReporter =
            create_stats_reporter(argList.get(), inputs, outputInfo, m_defaultTargetTriple);
      const bool enableExperimentalDependencies =
            argList->hasArg(options::OPT_enable_experimental_dependencies);
      const bool verifyExperimentalDependencyGraphAfterEveryImport = argList->hasArg(
               options::
               OPT_driver_verify_experimental_dependency_graph_after_every_import);
      const bool emitExperimentalDependencyDotFileAfterEveryImport = argList->hasArg(
               options::
               OPT_driver_emit_experimental_dependency_dot_file_after_every_import);
      const bool experimentalDependenciesIncludeIntrafileOnes = argList->hasArg(
               options::OPT_experimental_dependency_include_intrafile);

      // clang-format off
      compilation = std::make_unique<Compilation>(
               m_diags, toolchain, outputInfo, level,
               std::move(argList),
               std::move(translatedArgList),
               std::move(inputs),
               buildRecordPath,
               outputBuildRecordForModuleOnlyBuild,
               argsHash,
               startTime,
               lastBuildTime,
               driverFilelistThreshold,
               incremental,
               batchMode,
               driverBatchSeed,
               driverBatchCount,
               driverBatchSizeLimit,
               saveTemps,
               showDriverTimeCompilation,
               std::move(StatsReporter),
               enableExperimentalDependencies,
               verifyExperimentalDependencyGraphAfterEveryImport,
               emitExperimentalDependencyDotFileAfterEveryImport,
               experimentalDependenciesIncludeIntrafileOnes);
      // clang-format on
   }

   // Construct the graph of Actions.
   SmallVector<const Action *, 8> topLevelActions;
   buildActions(topLevelActions, toolchain, outputInfo,
                rebuildEverything ? nullptr : &outOfDateMap, *compilation);

   if (m_diags.hadAnyError()) {
      return nullptr;
   }

   if (driverPrintActions) {
      printActions(*compilation);
      return nullptr;
   }

   buildJobs(topLevelActions, outputInfo, optionalOutputInfo ? optionalOutputInfo.getPointer() : nullptr,
             workingDirectory, toolchain, *compilation);

   if (DriverPrintDerivedOutputFileMap) {
      compilation->getDerivedOutputFileMap().dump(llvm::outs(), true);
      return nullptr;
   }
   // For getting bulk fixits, or for when users explicitly request to continue
   // building despite errors.
   if (ContinueBuildingAfterErrors) {
      compilation->setContinueBuildingAfterErrors();
   }
   if (showIncrementalBuildDecisions || ShowJobLifecycle) {
      compilation->setShowIncrementalBuildDecisions();
   }

   if (ShowJobLifecycle) {
      compilation->setShowJobLifecycle();
   }
   // This has to happen after building jobs, because otherwise we won't even
   // emit .swiftdeps files for the next build.
   if (rebuildEverything) {
      compilation->disableIncrementalBuild();
   }
   if (m_diags.hadAnyError()) {
      return nullptr;
   }
   if (m_driverPrintBindings) {
      return nullptr;
   }
   return compilation;
}

static Arg *make_input_arg(const DerivedArgList &args, OptTable &opts,
                           StringRef value)
{
   Arg *arg = new Arg(opts.getOption(options::OPT_INPUT), value,
                      args.getBaseArgs().MakeIndex(value), value.data());
   arg->claim();
   return arg;
}

using RemainingArgsHandler = llvm::function_ref<void(InputArgList &, unsigned)>;

std::unique_ptr<InputArgList> parse_args_until(const llvm::opt::OptTable& opts,
                                               const char *const *argBegin,
                                               const char *const *argEnd,
                                               unsigned &missingArgIndex,
                                               unsigned &missingArgCount,
                                               unsigned flagsToInclude,
                                               unsigned flagsToExclude,
                                               llvm::opt::OptSpecifier untilOption,
                                               RemainingArgsHandler remainingHandler)
{
   auto args = std::make_unique<InputArgList>(argBegin, argEnd);
   // FIXME: Handle '@' args (or at least error on them).
   bool checkUntil = untilOption != options::OPT_INVALID;
   missingArgIndex = missingArgCount = 0;
   unsigned index = 0, end = argEnd - argBegin;
   while (index < end) {
      // Ignore empty arguments (other things may still take them as arguments).
      StringRef str = args->getArgString(index);
      if (str == "") {
         ++index;
         continue;
      }
      unsigned Prev = index;
      Arg *arg = opts.ParseOneArg(*args, index, flagsToInclude, flagsToExclude);
      assert(index > Prev && "Parser failed to consume argument.");

      // Check for missing argument error.
      if (!arg) {
         assert(index >= end && "Unexpected parser error.");
         assert(index - Prev - 1 && "No missing arguments!");
         missingArgIndex = Prev;
         missingArgCount = index - Prev - 1;
         break;
      }
      args->append(arg);
      if (checkUntil && arg->getOption().matches(untilOption)) {
         if (index < end) {
            remainingHandler(*args, index);
         }
         return args;
      }
   }
   return args;
}

// Parse all args until we see an input, and then collect the remaining
// arguments into a synthesized "--" option.
static std::unique_ptr<InputArgList>
parse_Arg_strings_for_interactive_driver(const llvm::opt::OptTable& opts,
                                         ArrayRef<const char *> args,
                                         unsigned &missingArgIndex,
                                         unsigned &missingArgCount,
                                         unsigned flagsToInclude,
                                         unsigned flagsToExclude)
{
   return parse_args_until(opts, args.begin(), args.end(), missingArgIndex,
                           missingArgCount, flagsToInclude, flagsToExclude,
                           options::OPT_INPUT,
                           [&](InputArgList &args, unsigned nextIndex)
   {
      assert(nextIndex < args.getNumInputArgStrings());
      // Synthesize -- remaining args...
      Arg *remaining =
            new Arg(opts.getOption(options::OPT__DASH_DASH), "--", nextIndex);
      for (unsigned N = args.getNumInputArgStrings(); nextIndex != N;
           ++nextIndex) {
         remaining->getValues().push_back(args.getArgString(nextIndex));
      }
      args.append(remaining);
   });
}

std::unique_ptr<InputArgList>
Driver::parseArgStrings(ArrayRef<const char *> args)
{
   unsigned includedFlagsBitmask = 0;
   unsigned excludedFlagsBitmask = options::NoDriverOption;
   unsigned missingArgIndex, missingArgCount;
   std::unique_ptr<InputArgList> argList;

   if (m_driverKind == DriverKind::Interactive) {
      argList = parse_Arg_strings_for_interactive_driver(getOpts(), args,
                                                         missingArgIndex, missingArgCount, includedFlagsBitmask,
                                                         excludedFlagsBitmask);

   } else {
      argList = std::make_unique<InputArgList>(
               getOpts().ParseArgs(args, missingArgIndex, missingArgCount,
                                   includedFlagsBitmask, excludedFlagsBitmask));
   }

   assert(argList && "no argument list");

   // Check for missing argument error.
   if (missingArgCount) {
      m_diags.diagnose(SourceLoc(), diag::error_missing_arg_value,
                       argList->getArgString(missingArgIndex), missingArgCount);
      return nullptr;
   }

   // Check for unknown arguments.
   for (const Arg *arg :  argList->filtered(options::OPT_UNKNOWN)) {
      m_diags.diagnose(SourceLoc(), diag::error_unknown_arg,
                       arg->getAsString(*argList));
   }

   // Check for unsupported options
   unsigned unsupportedFlag = 0;
   if (m_driverKind == DriverKind::Interactive) {
      unsupportedFlag = options::NoInteractiveOption;
   } else if (m_driverKind == DriverKind::Batch) {
      unsupportedFlag = options::NoBatchOption;
   }

   if (unsupportedFlag) {
      for (const Arg *arg : *argList) {
         if (arg->getOption().hasFlag(unsupportedFlag)) {
            m_diags.diagnose(SourceLoc(), diag::error_unsupported_option,
                             argList->getArgString(arg->getIndex()), m_name,
                             unsupportedFlag == options::NoBatchOption ? "swift" : "swiftc");
         }
      }
   }
   return argList;
}

DerivedArgList *
Driver::translateInputAndPathArgs(const InputArgList &argList,
                                  StringRef workingDirectory) const {
   DerivedArgList *dal = new DerivedArgList(argList);

   auto addPath = [workingDirectory, dal](Arg *arg) {
      assert(arg->getNumValues() == 1 && "multiple values not handled");
      StringRef path = arg->getValue();
      if (workingDirectory.empty() || path == "-" ||
          llvm::sys::path::is_absolute(path)) {
         dal->append(arg);
         return;
      }

      SmallString<64> fullPath{workingDirectory};
      llvm::sys::path::append(fullPath, path);
      unsigned index = dal->getBaseArgs().MakeIndex(fullPath);
      Arg *newArg = new Arg(arg->getOption(), arg->getSpelling(), index,
                            dal->getBaseArgs().getArgString(index), arg);
      dal->AddSynthesizedArg(newArg);
      dal->append(newArg);
   };

   for (Arg *arg : argList) {
      if (arg->getOption().hasFlag(options::ArgumentIsPath) ||
          arg->getOption().matches(options::OPT_INPUT)) {
         addPath(arg);
         continue;
      }

      // If we're not in immediate mode, pick up inputs via the -- option.
      if (m_driverKind != DriverKind::Interactive && arg->getOption().matches(options::OPT__DASH_DASH)) {
         arg->claim();
         for (unsigned i = 0, e = arg->getNumValues(); i != e; ++i) {
            addPath(make_input_arg(*dal, *m_opts, arg->getValue(i)));
         }
         continue;
      }

      dal->append(arg);
   }

   return dal;
}

/// Check that the file referenced by \p input exists. If it doesn't,
/// issue a diagnostic and return false.
static bool check_input_existence(const Driver &driver, const DerivedArgList &args,
                                  DiagnosticEngine &diags, StringRef input)
{
   if (!driver.getCheckInputFilesExist()) {
      return true;
   }
   // stdin always exists.
   if (input == "-") {
      return true;
   }
   if (llvm::sys::fs::exists(input)) {
      return true;
   }

   diags.diagnose(SourceLoc(), diag::error_no_such_file_or_directory, input);
   return false;
}

void Driver::buildInputs(const ToolChain &toolchain,
                         const DerivedArgList &args,
                         InputFileList &inputs) const
{
   llvm::DenseMap<StringRef, StringRef> sourceFileNames;

   for (Arg *arg : args) {
      if (arg->getOption().getKind() == Option::InputClass) {
         StringRef value = arg->getValue();
         filetypes::FileTypeId type = filetypes::TY_INVALID;
         // stdin must be handled specially.
         if (value.equals("-")) {
            // By default, treat stdin as Swift input.
            type = filetypes::TY_Polar;
         } else {
            // Otherwise lookup by extension.
            type = toolchain.lookupTypeForExtension(llvm::sys::path::extension(value));
            if (type == filetypes::TY_INVALID) {
               // By default, treat inputs with no extension, or with an
               // extension that isn't recognized, as object files.
               type = filetypes::TY_Object;
            }
         }
         if (check_input_existence(*this, args, m_diags, value)) {
            inputs.push_back(std::make_pair(type, arg));
         }
         if (type == filetypes::TY_Polar) {
            StringRef Basename = llvm::sys::path::filename(value);
            if (!sourceFileNames.insert({Basename, value}).second) {
               m_diags.diagnose(SourceLoc(), diag::error_two_files_same_name,
                                Basename, sourceFileNames[Basename], value);
               m_diags.diagnose(SourceLoc(), diag::note_explain_two_files_same_name);
            }
         }
      }
   }
}

static bool maybe_building_executable(const OutputInfo &outputInfo,
                                      const DerivedArgList &args,
                                      const InputFileList &inputs)
{
   switch (outputInfo.linkAction) {
   case LinkKind::Executable:
      return true;
   case LinkKind::DynamicLibrary:
      return false;
   case LinkKind::StaticLibrary:
      return false;
   case LinkKind::None:
      break;
   }
   if (args.hasArg(options::OPT_parse_as_library, options::OPT_parse_stdlib)) {
      return false;
   }
   return inputs.size() == 1;
}

static void maybe_building_executable(DiagnosticEngine &diags, const Arg *arg,
                                      bool hasInputs, const DerivedArgList &args,
                                      bool isInteractiveDriver,
                                      StringRef driverName)
{
   switch (arg->getOption().getID()) {

   case options::OPT_i:
      diags.diagnose(SourceLoc(), diag::error_i_mode,
                     isInteractiveDriver ? driverName : "swift");
      break;

   case options::OPT_repl:
      if (isInteractiveDriver && !hasInputs)
         diags.diagnose(SourceLoc(), diag::warning_unnecessary_repl_mode,
                        args.getArgString(arg->getIndex()), driverName);
      break;

   default:
      break;
   }
}

static bool is_sdk_too_old(StringRef sdkPath, llvm::VersionTuple minVersion,
                           StringRef firstPrefix, StringRef secondPrefix = {})
{
   // FIXME: This is a hack.
   // We should be looking at the SDKSettings.plist.
   StringRef sdkDirName = llvm::sys::path::filename(sdkPath);

   size_t versionStart = sdkDirName.rfind(firstPrefix);
   if (versionStart != StringRef::npos) {
      versionStart += firstPrefix.size();
   } else if (!secondPrefix.empty()) {
      versionStart = sdkDirName.rfind(secondPrefix);
      if (versionStart != StringRef::npos) {
         versionStart += secondPrefix.size();
      }
   }
   if (versionStart == StringRef::npos) {
      return false;
   }
   size_t versionEnd = sdkDirName.rfind(".Internal");
   if (versionEnd == StringRef::npos) {
      versionEnd = sdkDirName.rfind(".sdk");
   }
   if (versionEnd == StringRef::npos) {
      return false;
   }
   llvm::VersionTuple version;
   if (version.tryParse(sdkDirName.slice(versionStart, versionEnd))) {
      return false;
   }
   return version < minVersion;
}

/// Returns true if the given SDK path points to an SDK that is too old for
/// the given target.
static bool is_sdk_too_old(StringRef sdkPath, const llvm::Triple &target) {
   if (target.isMacOSX()) {
      return is_sdk_too_old(sdkPath, llvm::VersionTuple(10, 15), "OSX");

   } else if (target.isiOS()) {
      // Includes both iOS and TVOS.
      return is_sdk_too_old(sdkPath, llvm::VersionTuple(13, 0), "Simulator", "ostream");

   } else if (target.isWatchOS()) {
      return is_sdk_too_old(sdkPath, llvm::VersionTuple(6, 0), "Simulator", "ostream");

   } else {
      return false;
   }
}

void Driver::buildOutputInfo(const ToolChain &toolchain, const DerivedArgList &args,
                             const bool batchMode, const InputFileList &inputs,
                             OutputInfo &outputInfo) const
{
   // By default, the driver does not link its output; this will be updated
   // appropriately below if linking is required.

   outputInfo.compilerOutputType = m_driverKind == DriverKind::Interactive
         ? filetypes::TY_Nothing
         : filetypes::TY_Object;

   if (const Arg *arg = args.getLastArg(options::OPT_num_threads)) {
      if (batchMode) {
         m_diags.diagnose(SourceLoc(), diag::warning_cannot_multithread_batch_mode);
      } else if (StringRef(arg->getValue()).getAsInteger(10, outputInfo.numThreads)) {
         m_diags.diagnose(SourceLoc(), diag::error_invalid_arg_value,
                          arg->getAsString(args), arg->getValue());
      }
   }

   const Arg *const outputModeArg = args.getLastArg(options::OPT_modes_Group);

   if (!outputModeArg) {
      if (args.hasArg(options::OPT_emit_module, options::OPT_emit_module_path)) {
         outputInfo.compilerOutputType = filetypes::TY_PolarModuleFile;
      } else if (m_driverKind != DriverKind::Interactive) {
         outputInfo.linkAction = LinkKind::Executable;
      }
   } else {
      maybe_building_executable(m_diags, outputModeArg, !inputs.empty(), args,
                                m_driverKind == DriverKind::Interactive, m_name);

      switch (outputModeArg->getOption().getID()) {
      case options::OPT_emit_executable:
         if (args.hasArg(options::OPT_static)) {
            m_diags.diagnose(SourceLoc(),
                             diag::error_static_emit_executable_disallowed);
         }
         outputInfo.linkAction = LinkKind::Executable;
         outputInfo.compilerOutputType = filetypes::TY_Object;
         break;

      case options::OPT_emit_library:
         outputInfo.linkAction = args.hasArg(options::OPT_static) ?
                  LinkKind::StaticLibrary :
                  LinkKind::DynamicLibrary;
         outputInfo.compilerOutputType = filetypes::TY_Object;
         break;

      case options::OPT_static:
         break;

      case options::OPT_emit_object:
         outputInfo.compilerOutputType = filetypes::TY_Object;
         break;

      case options::OPT_emit_assembly:
         outputInfo.compilerOutputType = filetypes::TY_Assembly;
         break;

      case options::OPT_emit_sil:
         outputInfo.compilerOutputType = filetypes::TY_PIL;
         break;

      case options::OPT_emit_silgen:
         outputInfo.compilerOutputType = filetypes::TY_RawPIL;
         break;

      case options::OPT_emit_sib:
         outputInfo.compilerOutputType = filetypes::TY_PIB;
         break;

      case options::OPT_emit_sibgen:
         outputInfo.compilerOutputType = filetypes::TY_RawPIB;
         break;

      case options::OPT_emit_ir:
         outputInfo.compilerOutputType = filetypes::TY_LLVM_IR;
         break;

      case options::OPT_emit_bc:
         outputInfo.compilerOutputType = filetypes::TY_LLVM_BC;
         break;

      case options::OPT_dump_ast:
         outputInfo.compilerOutputType = filetypes::TY_ASTDump;
         break;

      case options::OPT_emit_pch:
         outputInfo.compilerMode = OutputInfo::Mode::SingleCompile;
         outputInfo.compilerOutputType = filetypes::TY_PCH;
         break;

      case options::OPT_emit_imported_modules:
         outputInfo.compilerOutputType = filetypes::TY_ImportedModules;
         // We want the imported modules from the module as a whole, not individual
         // files, so let's do it in one invocation rather than having to collate
         // later.
         outputInfo.compilerMode = OutputInfo::Mode::SingleCompile;
         break;

      case options::OPT_index_file:
         outputInfo.compilerMode = OutputInfo::Mode::SingleCompile;
         outputInfo.compilerOutputType = filetypes::TY_IndexData;
         break;

      case options::OPT_update_code:
         outputInfo.compilerOutputType = filetypes::TY_Remapping;
         outputInfo.linkAction = LinkKind::None;
         break;

      case options::OPT_parse:
      case options::OPT_resolve_imports:
      case options::OPT_typecheck:
      case options::OPT_dump_parse:
      case options::OPT_emit_syntax:
      case options::OPT_print_ast:
      case options::OPT_dump_type_refinement_contexts:
      case options::OPT_dump_scope_maps:
      case options::OPT_dump_interface_hash:
      case options::OPT_dump_type_info:
      case options::OPT_verify_debug_info:
         outputInfo.compilerOutputType = filetypes::TY_Nothing;
         break;

      case options::OPT_i:
         // Keep the default output/mode; this flag was removed and should already
         // have been diagnosed above.
         assert(m_diags.hadAnyError() && "-i flag was removed");
         break;

      case options::OPT_repl:
      case options::OPT_deprecated_integrated_repl:
      case options::OPT_lldb_repl:
         outputInfo.compilerOutputType = filetypes::TY_Nothing;
         outputInfo.compilerMode = OutputInfo::Mode::REPL;
         break;

      default:
         llvm_unreachable("unknown mode");
      }
   }

   assert(outputInfo.compilerOutputType != filetypes::FileTypeId::TY_INVALID);

   if (const Arg *arg = args.getLastArg(options::OPT_g_Group)) {
      if (arg->getOption().matches(options::OPT_g)) {
         outputInfo.debugInfoLevel = IRGenDebugInfoLevel::Normal;
      } else if (arg->getOption().matches(options::OPT_gline_tables_only)) {
         outputInfo.debugInfoLevel = IRGenDebugInfoLevel::LineTables;
      } else if (arg->getOption().matches(options::OPT_gdwarf_types)) {
         outputInfo.debugInfoLevel = IRGenDebugInfoLevel::DwarfTypes;
      } else {
         assert(arg->getOption().matches(options::OPT_gnone) &&
                "unknown -g<kind> option");
      }
   }

   if (const Arg *arg = args.getLastArg(options::OPT_debug_info_format)) {
      if (strcmp(arg->getValue(), "dwarf") == 0) {
         outputInfo.debugInfoFormat = IRGenDebugInfoFormat::DWARF;
      } else if (strcmp(arg->getValue(), "codeview") == 0) {
         outputInfo.debugInfoFormat = IRGenDebugInfoFormat::CodeView;
      } else {
         m_diags.diagnose(SourceLoc(), diag::error_invalid_arg_value,
                          arg->getAsString(args), arg->getValue());
      }
   } else if (outputInfo.debugInfoLevel > IRGenDebugInfoLevel::None) {
      // If -g was specified but not -debug-info-format, DWARF is assumed.
      outputInfo.debugInfoFormat = IRGenDebugInfoFormat::DWARF;
   }
   if (args.hasArg(options::OPT_debug_info_format) &&
       !args.hasArg(options::OPT_g_Group)) {
      const Arg *debugFormatArg = args.getLastArg(options::OPT_debug_info_format);
      m_diags.diagnose(SourceLoc(), diag::error_option_missing_required_argument,
                       debugFormatArg->getAsString(args), "-g");
   }
   if (outputInfo.debugInfoFormat == IRGenDebugInfoFormat::CodeView &&
       (outputInfo.debugInfoLevel == IRGenDebugInfoLevel::LineTables ||
        outputInfo.debugInfoLevel == IRGenDebugInfoLevel::DwarfTypes)) {
      const Arg *debugFormatArg = args.getLastArg(options::OPT_debug_info_format);
      m_diags.diagnose(SourceLoc(), diag::error_argument_not_allowed_with,
                       debugFormatArg->getAsString(args),
                       outputInfo.debugInfoLevel == IRGenDebugInfoLevel::LineTables
                       ? "-gline-tables-only"
                       : "-gdwarf_types");
   }

   if (args.hasArg(options::OPT_emit_module, options::OPT_emit_module_path)) {
      // The user has requested a module, so generate one and treat it as
      // top-level output.
      outputInfo.shouldGenerateModule = true;
      outputInfo.shouldTreatModuleAsTopLevelOutput = true;
   } else if (outputInfo.debugInfoLevel > IRGenDebugInfoLevel::LineTables &&
              outputInfo.shouldLink()) {
      // An option has been passed which requires a module, but the user hasn't
      // requested one. Generate a module, but treat it as an intermediate output.
      outputInfo.shouldGenerateModule = true;
      outputInfo.shouldTreatModuleAsTopLevelOutput = false;
   } else if (args.hasArg(options::OPT_emit_objc_header,
                          options::OPT_emit_objc_header_path,
                          options::OPT_emit_module_interface,
                          options::OPT_emit_module_interface_path) &&
              outputInfo.compilerMode != OutputInfo::Mode::SingleCompile) {
      // An option has been passed which requires whole-module knowledge, but we
      // don't have that. Generate a module, but treat it as an intermediate
      // output.
      outputInfo.shouldGenerateModule = true;
      outputInfo.shouldTreatModuleAsTopLevelOutput = false;
   } else {
      // No options require a module, so don't generate one.
      outputInfo.shouldGenerateModule = false;
      outputInfo.shouldTreatModuleAsTopLevelOutput = false;
   }

   if (outputInfo.shouldGenerateModule &&
       (outputInfo.compilerMode == OutputInfo::Mode::REPL ||
        outputInfo.compilerMode == OutputInfo::Mode::Immediate)) {
      m_diags.diagnose(SourceLoc(), diag::error_mode_cannot_emit_module);
      return;
   }

   if (const Arg *arg = args.getLastArg(options::OPT_module_name)) {
      outputInfo.moduleName = arg->getValue();
   } else if (outputInfo.compilerMode == OutputInfo::Mode::REPL) {
      // REPL mode should always use the REPL module.
      outputInfo.moduleName = "REPL";
   } else if (const Arg *arg = args.getLastArg(options::OPT_o)) {
      outputInfo.moduleName = llvm::sys::path::stem(arg->getValue());
      if ((outputInfo.linkAction == LinkKind::DynamicLibrary ||
           outputInfo.linkAction == LinkKind::StaticLibrary) &&
          !llvm::sys::path::extension(arg->getValue()).empty() &&
          StringRef(outputInfo.moduleName).startswith("lib")) {
         // Chop off a "lib" prefix if we're building a library.
         outputInfo.moduleName.erase(0, strlen("lib"));
      }
   } else if (inputs.size() == 1) {
      outputInfo.moduleName = llvm::sys::path::stem(inputs.front().second->getValue());
   }

   if (!Lexer::isIdentifier(outputInfo.moduleName) ||
       (outputInfo.moduleName == STDLIB_NAME &&
        !args.hasArg(options::OPT_parse_stdlib))) {
      outputInfo.moduleNameIsFallback = true;
      if (outputInfo.compilerOutputType == filetypes::TY_Nothing ||
          maybe_building_executable(outputInfo, args, inputs))
         outputInfo.moduleName = "main";
      else if (!inputs.empty() || outputInfo.compilerMode == OutputInfo::Mode::REPL) {
         // Having an improper module name is only bad if we have inputs or if
         // we're in REPL mode.
         auto DID = (outputInfo.moduleName == STDLIB_NAME) ? diag::error_stdlib_module_name
                                                           : diag::error_bad_module_name;
         m_diags.diagnose(SourceLoc(), DID,
                          outputInfo.moduleName, !args.hasArg(options::OPT_module_name));
         outputInfo.moduleName = "__bad__";
      }
   }

   {
      if (const Arg *arg = args.getLastArg(options::OPT_sdk)) {
         outputInfo.sdkPath = arg->getValue();
      } else if (const char *SDKROOT = getenv("SDKROOT")) {
         outputInfo.sdkPath = SDKROOT;
      } else if (outputInfo.compilerMode == OutputInfo::Mode::Immediate ||
                 outputInfo.compilerMode == OutputInfo::Mode::REPL) {
         if (toolchain.getTriple().isMacOSX()) {
            // In immediate modes, use the SDK provided by xcrun.
            // This will prefer the SDK alongside the Swift found by "xcrun swift".
            // We don't do this in compilation modes because defaulting to the
            // latest SDK may not be intended.
            auto xcrunPath = llvm::sys::findProgramByName("xcrun");
            if (!xcrunPath.getError()) {
               const char *args[] = {
                  "--show-sdk-path", "--sdk", "macosx", nullptr
               };
               sys::TaskQueue queue;
               queue.addTask(xcrunPath->c_str(), args);
               queue.execute(nullptr,
                             [&outputInfo](sys::ProcessId pid, int returnCode,
                             StringRef output, StringRef errors,
                             sys::TaskProcessInformation procInfo,
                             void *unused) -> sys::TaskFinishedResponse {
                  if (returnCode == 0) {
                     output = output.rtrim();
                     auto lastLineStart = output.find_last_of("\n\r");
                     if (lastLineStart != StringRef::npos) {
                        output = output.substr(lastLineStart+1);
                     }
                     if (output.empty()) {
                        outputInfo.sdkPath = "/";
                     } else {
                        outputInfo.sdkPath = output.str();
                     }
                  }
                  return sys::TaskFinishedResponse::ContinueExecution;
               });
            }
         }
      }

      if (!outputInfo.sdkPath.empty()) {
         // Delete a trailing /.
         if (outputInfo.sdkPath.size() > 1 &&
             llvm::sys::path::is_separator(outputInfo.sdkPath.back())) {
            outputInfo.sdkPath.erase(outputInfo.sdkPath.end()-1);
         }

         if (!llvm::sys::fs::exists(outputInfo.sdkPath)) {
            m_diags.diagnose(SourceLoc(), diag::warning_no_such_sdk, outputInfo.sdkPath);
         } else if (is_sdk_too_old(outputInfo.sdkPath, toolchain.getTriple())) {
            m_diags.diagnose(SourceLoc(), diag::error_sdk_too_old,
                             llvm::sys::path::filename(outputInfo.sdkPath));
         }
      }
   }

   if (const Arg *arg = args.getLastArg(options::OPT_sanitize_EQ))
      outputInfo.selectedSanitizers = parse_sanitizer_arg_values(
               args, arg, toolchain.getTriple(), m_diags,
               [&](StringRef sanitizerName, bool shared) {
            return toolchain.sanitizerRuntimeLibExists(args, sanitizerName, shared);
});

   if (const Arg *arg = args.getLastArg(options::OPT_sanitize_coverage_EQ)) {

      // Check that the sanitizer coverage flags are supported if supplied.
      // Dismiss the output, as we will grab the value later.
      (void)parse_sanitizer_coverage_arg_value(arg, toolchain.getTriple(), m_diags,
                                               outputInfo.selectedSanitizers);

   }

}

OutputInfo::Mode
Driver::computeCompilerMode(const DerivedArgList &args,
                            const InputFileList &inputs,
                            bool &batchModeOut) const
{

   if (m_driverKind == Driver::DriverKind::Interactive) {
      return inputs.empty() ? OutputInfo::Mode::REPL
                            : OutputInfo::Mode::Immediate;
   }

   const Arg *argRequiringSingleCompile = args.getLastArg(
            options::OPT_whole_module_optimization, options::OPT_index_file);

   batchModeOut = args.hasFlag(options::OPT_enable_batch_mode,
                               options::OPT_disable_batch_mode,
                               false);

   if (!argRequiringSingleCompile) {
      return OutputInfo::Mode::StandardCompile;
   }
   // Override batch mode if given -wmo or -index-file.
   if (batchModeOut) {
      batchModeOut = false;
      // Emit a warning about such overriding (FIXME: we might conditionalize
      // this based on the user or xcode passing -disable-batch-mode).
      m_diags.diagnose(SourceLoc(), diag::warn_ignoring_batch_mode,
                       argRequiringSingleCompile->getOption().getPrefixedName());
   }
   return OutputInfo::Mode::SingleCompile;
}

void Driver::buildActions(SmallVectorImpl<const Action *> &topLevelActions,
                          const ToolChain &toolchain, const OutputInfo &outputInfo,
                          const InputInfoMap *outOfDateMap,
                          Compilation &compilation) const
{
   const DerivedArgList &args = compilation.getArgs();
   ArrayRef<InputPair> inputs = compilation.getInputFiles();

   if (!m_suppressNoInputFilesError && inputs.empty()) {
      m_diags.diagnose(SourceLoc(), diag::error_no_input_files);
      return;
   }

   SmallVector<const Action *, 2> allModuleInputs;
   SmallVector<const Action *, 2> allLinkerInputs;

   switch (outputInfo.compilerMode) {
   case OutputInfo::Mode::StandardCompile: {

      // If the user is importing a textual (.h) bridging header and we're in
      // standard-compile (non-WMO) mode, we take the opportunity to precompile
      // the header into a temporary pch, and replace the import argument with the
      // pch in the subsequent frontend jobs.
      JobAction *pch = nullptr;
      if (args.hasFlag(options::OPT_enable_bridging_pch,
                       options::OPT_disable_bridging_pch,
                       true)) {
         //         if (Arg *arg = args.getLastArg(options::OPT_import_objc_header)) {
         //            StringRef value = arg->getValue();
         //            auto type = toolchain.lookupTypeForExtension(llvm::sys::path::extension(value));
         //            if (type == filetypes::TY_ObjCHeader) {
         //               auto *HeaderInput = compilation.createAction<InputAction>(*arg, type);
         //               StringRef PersistentPCHDir;
         //               if (const Arg *arg = args.getLastArg(options::OPT_pch_output_dir)) {
         //                  PersistentPCHDir = arg->getValue();
         //               }
         //               pch = compilation.createAction<GeneratePCHJobAction>(HeaderInput,
         //                                                          PersistentPCHDir);
         //            }
         //         }
      }

      for (const InputPair &input : inputs) {
         filetypes::FileTypeId inputType = input.first;
         const Arg *inputArg = input.second;

         Action *current = compilation.createAction<InputAction>(*inputArg, inputType);
         switch (inputType) {
         case filetypes::TY_Polar:
         case filetypes::TY_PIL:
         case filetypes::TY_PIB: {
            // Source inputs always need to be compiled.
            assert(filetypes::is_part_of_polarphp_compilation(inputType));

            CompileJobAction::InputInfo previousBuildState = {
               CompileJobAction::InputInfo::NeedsCascadingBuild,
               llvm::sys::TimePoint<>::min()
            };
            if (outOfDateMap) {
               previousBuildState = outOfDateMap->lookup(inputArg);
            }

            if (args.hasArg(options::OPT_embed_bitcode)) {
               current = compilation.createAction<CompileJobAction>(
                        current, filetypes::TY_LLVM_BC, previousBuildState);
               if (pch) {
                  cast<JobAction>(current)->addInput(pch);
               }
               allModuleInputs.push_back(current);
               current = compilation.createAction<BackendJobAction>(current,
                                                                    outputInfo.compilerOutputType, 0);
            } else {
               current = compilation.createAction<CompileJobAction>(current,
                                                                    outputInfo.compilerOutputType,
                                                                    previousBuildState);
               if (pch) {
                  cast<JobAction>(current)->addInput(pch);
               }
               allModuleInputs.push_back(current);
            }
            allLinkerInputs.push_back(current);
            break;
         }
         case filetypes::TY_PolarModuleFile:
         case filetypes::TY_PolarModuleDocFile:
            if (outputInfo.shouldGenerateModule && !outputInfo.shouldLink()) {
               // When generating a .swiftmodule as a top-level output (as opposed
               // to, for example, linking an image), treat .swiftmodule files as
               // inputs to a MergeModule action.
               allModuleInputs.push_back(current);
               break;
            } else if (outputInfo.shouldLink()) {
               // Otherwise, if linking, pass .swiftmodule files as inputs to the
               // linker, so that their debug info is available.
               allLinkerInputs.push_back(current);
               break;
            } else {
               m_diags.diagnose(SourceLoc(), diag::error_unexpected_input_file,
                                inputArg->getValue());
               continue;
            }
         case filetypes::TY_AutolinkFile:
         case filetypes::TY_Object:
            // Object inputs are only okay if linking.
            if (outputInfo.shouldLink()) {
               allLinkerInputs.push_back(current);
               break;
            }
            LLVM_FALLTHROUGH;
         case filetypes::TY_ASTDump:
         case filetypes::TY_Image:
         case filetypes::TY_dSYM:
         case filetypes::TY_Dependencies:
         case filetypes::TY_Assembly:
         case filetypes::TY_LLVM_IR:
         case filetypes::TY_LLVM_BC:
         case filetypes::TY_SerializedDiagnostics:
         case filetypes::TY_ObjCHeader:
         case filetypes::TY_ClangModuleFile:
         case filetypes::TY_PolarDeps:
         case filetypes::TY_Remapping:
         case filetypes::TY_IndexData:
         case filetypes::TY_PCH:
         case filetypes::TY_ImportedModules:
         case filetypes::TY_TBD:
         case filetypes::TY_ModuleTrace:
         case filetypes::TY_OptRecord:
         case filetypes::TY_PolarParseableInterfaceFile:
            // We could in theory handle assembly or LLVM input, but let's not.
            // FIXME: What about LTO?
            m_diags.diagnose(SourceLoc(), diag::error_unexpected_input_file,
                             inputArg->getValue());
            continue;
         case filetypes::TY_RawPIB:
         case filetypes::TY_RawPIL:
         case filetypes::TY_Nothing:
         case filetypes::TY_INVALID:
            llvm_unreachable("these types should never be inferred");
         }
      }
      break;
   }
   case OutputInfo::Mode::SingleCompile: {
      if (inputs.empty()) break;
      if (args.hasArg(options::OPT_embed_bitcode)) {
         // Make sure we can handle the inputs.
         bool handledHere = true;
         for (const InputPair &input : inputs) {
            filetypes::FileTypeId inputType = input.first;
            if (!filetypes::is_part_of_polarphp_compilation(inputType)) {
               handledHere = false;
               break;
            }
         }
         if (handledHere) {
            // Create a single CompileJobAction and a single BackendJobAction.
            JobAction *jobAction =
                  compilation.createAction<CompileJobAction>(filetypes::TY_LLVM_BC);
            allModuleInputs.push_back(jobAction);

            int inputIndex = 0;
            for (const InputPair &input : inputs) {
               filetypes::FileTypeId inputType = input.first;
               const Arg *inputArg = input.second;

               jobAction->addInput(compilation.createAction<InputAction>(*inputArg, inputType));
               if (outputInfo.isMultiThreading()) {
                  // With multi-threading we need a backend job for each output file
                  // of the compilation.
                  auto *BJA = compilation.createAction<BackendJobAction>(jobAction,
                                                                         outputInfo.compilerOutputType,
                                                                         inputIndex);
                  allLinkerInputs.push_back(BJA);
               }
               inputIndex++;
            }
            if (!outputInfo.isMultiThreading()) {
               // No multi-threading: the compilation only produces a single output
               // file.
               jobAction = compilation.createAction<BackendJobAction>(jobAction, outputInfo.compilerOutputType, 0);
               allLinkerInputs.push_back(jobAction);
            }
            break;
         }
      }

      // Create a single CompileJobAction for all of the driver's inputs.
      auto *jobAction = compilation.createAction<CompileJobAction>(outputInfo.compilerOutputType);
      for (const InputPair &input : inputs) {
         filetypes::FileTypeId inputType = input.first;
         const Arg *inputArg = input.second;

         jobAction->addInput(compilation.createAction<InputAction>(*inputArg, inputType));
      }
      allModuleInputs.push_back(jobAction);
      allLinkerInputs.push_back(jobAction);
      break;
   }
   case OutputInfo::Mode::BatchModeCompile: {
      llvm_unreachable("Batch mode should not be used to build actions");
   }
   case OutputInfo::Mode::Immediate: {
      if (inputs.empty())
         return;

      assert(outputInfo.compilerOutputType == filetypes::TY_Nothing);
      auto *jobAction = compilation.createAction<InterpretJobAction>();
      for (const InputPair &input : inputs) {
         filetypes::FileTypeId inputType = input.first;
         const Arg *inputArg = input.second;

         jobAction->addInput(compilation.createAction<InputAction>(*inputArg, inputType));
      }
      topLevelActions.push_back(jobAction);
      return;
   }
   case OutputInfo::Mode::REPL: {
      if (!inputs.empty()) {
         // REPL mode requires no inputs.
         m_diags.diagnose(SourceLoc(), diag::error_repl_requires_no_input_files);
         return;
      }

      REPLJobAction::Mode Mode = REPLJobAction::Mode::PreferLLDB;
      if (const Arg *arg = args.getLastArg(options::OPT_lldb_repl,
                                           options::OPT_deprecated_integrated_repl)) {
         if (arg->getOption().matches(options::OPT_lldb_repl))
            Mode = REPLJobAction::Mode::RequireLLDB;
         else
            Mode = REPLJobAction::Mode::Integrated;
      }

      topLevelActions.push_back(compilation.createAction<REPLJobAction>(Mode));
      return;
   }
   }

   JobAction *mergeModuleAction = nullptr;
   if (outputInfo.shouldGenerateModule &&
       outputInfo.compilerMode != OutputInfo::Mode::SingleCompile &&
       !allModuleInputs.empty()) {
      // We're performing multiple compilations; set up a merge module step
      // so we generate a single swiftmodule as output.
      mergeModuleAction = compilation.createAction<MergeModuleJobAction>(allModuleInputs);
   }

   if (outputInfo.shouldLink() && !allLinkerInputs.empty()) {
      JobAction *linkAction = nullptr;

      if (outputInfo.linkAction == LinkKind::StaticLibrary) {
         linkAction = compilation.createAction<StaticLinkJobAction>(allLinkerInputs,
                                                                    outputInfo.linkAction);
      } else {
         linkAction = compilation.createAction<DynamicLinkJobAction>(allLinkerInputs,
                                                                     outputInfo.linkAction);
      }

      // On ELF platforms there's no built in autolinking mechanism, so we
      // pull the info we need from the .o files directly and pass them as an
      // argument input file to the linker.
      SmallVector<const Action *, 2> autolinkExtractInputs;
      for (const Action *arg : allLinkerInputs) {
         if (arg->getType() == filetypes::TY_Object) {
            autolinkExtractInputs.push_back(arg);
         }
      }

      if (!autolinkExtractInputs.empty() &&
          (toolchain.getTriple().getObjectFormat() == llvm::Triple::ELF ||
           toolchain.getTriple().isOSCygMing())) {
         auto *AutolinkExtractAction =
               compilation.createAction<AutolinkExtractJobAction>(autolinkExtractInputs);
         // Takes the same inputs as the linker...
         // ...and gives its output to the linker.
         linkAction->addInput(AutolinkExtractAction);
      }

      if (mergeModuleAction) {
         if (outputInfo.debugInfoLevel == IRGenDebugInfoLevel::Normal) {
            if (toolchain.getTriple().getObjectFormat() == llvm::Triple::ELF) {
               auto *ModuleWrapAction =
                     compilation.createAction<ModuleWrapJobAction>(mergeModuleAction);
               linkAction->addInput(ModuleWrapAction);
            } else {
               linkAction->addInput(mergeModuleAction);
            }
            // FIXME: Adding the mergeModuleAction as top-level regardless would
            // allow us to get rid of the special case flag for that.
         } else {
            topLevelActions.push_back(mergeModuleAction);
         }
      }
      topLevelActions.push_back(linkAction);

      if (toolchain.getTriple().isOSDarwin() &&
          outputInfo.debugInfoLevel > IRGenDebugInfoLevel::None) {
         auto *dSYMAction = compilation.createAction<GenerateDSYMJobAction>(linkAction);
         topLevelActions.push_back(dSYMAction);
         if (args.hasArg(options::OPT_verify_debug_info)) {
            topLevelActions.push_back(
                     compilation.createAction<VerifyDebugInfoJobAction>(dSYMAction));
         }
      }
   } else {
      // We can't rely on the merge module action being the only top-level
      // action that needs to run. There may be other actions (e.g.
      // BackendJobActions) that are not merge-module inputs but should be run
      // anyway.
      if (mergeModuleAction)
         topLevelActions.push_back(mergeModuleAction);
      topLevelActions.append(allLinkerInputs.begin(), allLinkerInputs.end());
   }
}

bool Driver::handleImmediateArgs(const ArgList &args, const ToolChain &toolchain)
{
   if (args.hasArg(options::OPT_help)) {
      printHelp(false);
      return false;
   }

   if (args.hasArg(options::OPT_help_hidden)) {
      printHelp(true);
      return false;
   }

   if (args.hasArg(options::OPT_version)) {
      // Follow gcc/clang behavior and use stdout for --version and stderr for -v.
      printVersion(toolchain, llvm::outs());
      return false;
   }

   if (args.hasArg(options::OPT_v)) {
      printVersion(toolchain, llvm::errs());
      m_suppressNoInputFilesError = true;
   }

   if (const Arg *arg = args.getLastArg(options::OPT_driver_use_frontend_path)) {
      m_driverExecutable = arg->getValue();
      std::string commandString =
            args.getLastArgValue(options::OPT_driver_use_frontend_path);
      SmallVector<StringRef, 10> commandArgs;
      StringRef(commandString).split(commandArgs, ';', -1, false);
      m_driverExecutable = commandArgs[0];
      m_driverExecutableArgs.assign(std::begin(commandArgs) + 1,
                                    std::end(commandArgs));
   }

   return true;
}

Optional<OutputFileMap>
Driver::buildOutputFileMap(const llvm::opt::DerivedArgList &args,
                           StringRef workingDirectory) const
{
   const Arg *arg = args.getLastArg(options::OPT_output_file_map);
   if (!arg)
      return None;

   // TODO: perform some preflight checks to ensure the file exists.
   llvm::Expected<OutputFileMap> optionalOutputInfo =
         OutputFileMap::loadFromPath(arg->getValue(), workingDirectory);
   if (auto Err = optionalOutputInfo.takeError()) {
      m_diags.diagnose(SourceLoc(), diag::error_unable_to_load_output_file_map,
                       llvm::toString(std::move(Err)), arg->getValue());
      return None;
   }
   return *optionalOutputInfo;
}

void Driver::buildJobs(ArrayRef<const Action *> topLevelActions,
                       const OutputInfo &outputInfo, const OutputFileMap *optionalOutputInfo,
                       StringRef workingDirectory, const ToolChain &toolchain,
                       Compilation &compilation) const
{
   llvm::PrettyStackTraceString crashInfo("Building compilation jobs");

   const DerivedArgList &args = compilation.getArgs();
   JobCacheMap jobCache;

   if (args.hasArg(options::OPT_o) && !outputInfo.shouldLink() &&
       !outputInfo.shouldTreatModuleAsTopLevelOutput) {
      bool shouldComplain;
      if (outputInfo.isMultiThreading()) {
         // Multi-threading compilation has multiple outputs unless there's only
         // one input.
         shouldComplain = compilation.getInputFiles().size() > 1;
      } else {
         // Single-threaded compilation is a problem if we're compiling more than
         // one file.
         shouldComplain = 1 < llvm::count_if(compilation.getActions(), [](const Action *arg) {
            return isa<CompileJobAction>(arg);
         });
      }

      if (shouldComplain) {
         m_diags.diagnose(SourceLoc(),
                          diag::error_cannot_specify__o_for_multiple_outputs);
      }
   }

   for (const Action *arg : topLevelActions) {
      if (auto *jobAction = dyn_cast<JobAction>(arg)) {
         (void)buildJobsForAction(compilation, jobAction, optionalOutputInfo, workingDirectory, /*TopLevel=*/true,
                                  jobCache);
      }
   }
}

/// Form a filename based on \p base in \p result, optionally setting its
/// extension to \p newExt and in \p workingDirectory.
static void form_filename_from_base_and_ext(StringRef base, StringRef newExt,
                                            StringRef workingDirectory,
                                            SmallVectorImpl<char> &result)
{
   if (workingDirectory.empty() || llvm::sys::path::is_absolute(base)) {
      result.assign(base.begin(), base.end());
   } else {
      assert(!base.empty() && base != "-" && "unexpected basename");
      result.assign(workingDirectory.begin(), workingDirectory.end());
      llvm::sys::path::append(result, base);
   }

   if (!newExt.empty()) {
      llvm::sys::path::replace_extension(result, newExt);
   }
}

static Optional<StringRef> getOutputFilenameFromPathArgOrAsTopLevel(
      const OutputInfo &outputInfo, const llvm::opt::DerivedArgList &args,
      llvm::opt::OptSpecifier pathArg, filetypes::FileTypeId expectedOutputType,
      bool treatAsTopLevelOutput, StringRef workingDirectory,
      llvm::SmallString<128> &buffer)
{
   if (const Arg *arg = args.getLastArg(pathArg)) {
      return StringRef(arg->getValue());
   }

   if (treatAsTopLevelOutput) {
      if (const Arg *arg = args.getLastArg(options::OPT_o)) {
         if (outputInfo.compilerOutputType == expectedOutputType) {
            return StringRef(arg->getValue());
         }

         // Otherwise, put the file next to the top-level output.
         buffer = arg->getValue();
         llvm::sys::path::remove_filename(buffer);
         llvm::sys::path::append(buffer, outputInfo.moduleName);
         llvm::sys::path::replace_extension(
                  buffer, filetypes::get_extension(expectedOutputType));
         return buffer.str();
      }

      // arg top-level output wasn't specified, so just output to
      // <ModuleName>.<ext>.
      form_filename_from_base_and_ext(outputInfo.moduleName,
                                      filetypes::get_extension(expectedOutputType),
                                      workingDirectory,
                                      buffer);
      return buffer.str();
   }

   return None;
}

static StringRef assign_output_name(Compilation &compilation, const JobAction *jobAction,
                                    llvm::SmallString<128> &buffer,
                                    StringRef baseName,
                                    PreserveOnSignal shouldPreserveOnSignal)
{
   // We should output to a temporary file, since we're not at the top level
   // (or are generating a bridging pch, which is currently always a temp).
   StringRef stem = llvm::sys::path::stem(baseName);
   StringRef suffix = filetypes::get_extension(jobAction->getType());
   std::error_code errorCode = llvm::sys::fs::createTemporaryFile(stem, suffix, buffer);
   if (errorCode) {
      compilation.getDiags().diagnose(SourceLoc(),
                                      diag::error_unable_to_make_temporary_file,
                                      errorCode.message());
      return {};
   }
   compilation.addTemporaryFile(buffer.str(), shouldPreserveOnSignal);

   return buffer.str();
}

static StringRef baseNameForImage(const JobAction *jobAction, const OutputInfo &outputInfo,
                                  const llvm::Triple &triple,
                                  llvm::SmallString<128> &buffer,
                                  StringRef baseInput, StringRef baseName)
{
   if (jobAction->size() == 1 && outputInfo.moduleNameIsFallback && baseInput != "-") {
      return llvm::sys::path::stem(baseInput);
   }
   if (auto link = dyn_cast<StaticLinkJobAction>(jobAction)) {
      buffer = "lib";
      buffer.append(baseName);
      buffer.append(triple.isOSWindows() ? ".lib" : ".a");
      return buffer.str();
   }

   auto link = dyn_cast<DynamicLinkJobAction>(jobAction);
   if (!link)
      return baseName;
   if (link->getKind() != LinkKind::DynamicLibrary) {
      return baseName;
   }

   buffer = triple.isOSWindows() ? "" : "lib";
   buffer.append(baseName);

   if (triple.isOSDarwin()) {
      buffer.append(".dylib");
   } else if (triple.isOSWindows()) {
      buffer.append(".dll");
   } else {
      buffer.append(".so");
   }

   return buffer.str();
}

static StringRef getOutputFilename(Compilation &compilation,
                                   const JobAction *jobAction,
                                   const TypeToPathMap *outputMap,
                                   StringRef workingDirectory,
                                   bool atTopLevel,
                                   StringRef baseInput,
                                   StringRef primaryInput,
                                   llvm::SmallString<128> &buffer)
{
   if (jobAction->getType() == filetypes::TY_Nothing) {
      return {};
   }
   // If available, check the outputMap first.
   if (outputMap) {
      auto iter = outputMap->find(jobAction->getType());
      if (iter != outputMap->end()) {
         return iter->second;
      }
   }

   // Process Action-specific output-specifying options next,
   // since we didn't find anything applicable in the outputMap.
   const OutputInfo &outputInfo = compilation.getOutputInfo();
   const llvm::opt::DerivedArgList &args = compilation.getArgs();

   if (isa<MergeModuleJobAction>(jobAction)) {
      auto optFilename = getOutputFilenameFromPathArgOrAsTopLevel(
               outputInfo, args, options::OPT_emit_module_path, filetypes::TY_PolarModuleFile,
               outputInfo.shouldTreatModuleAsTopLevelOutput, workingDirectory, buffer);
      if (optFilename) {
         return *optFilename;
      }
   }

   // dSYM actions are never treated as top-level.
   if (isa<GenerateDSYMJobAction>(jobAction)) {
      buffer = primaryInput;
      buffer.push_back('.');
      buffer.append(filetypes::get_extension(jobAction->getType()));
      return buffer.str();
   }

   auto shouldPreserveOnSignal = PreserveOnSignal::No;

   if (auto *PCHAct = dyn_cast<GeneratePCHJobAction>(jobAction)) {
      // For a persistent pch we don't use an output, the frontend determines
      // the filename to use for the pch.
      assert(!PCHAct->isPersistentPCH());
      shouldPreserveOnSignal = PreserveOnSignal::Yes;
   }

   // We don't have an output from an Action-specific command line option,
   // so figure one out using the defaults.
   if (atTopLevel) {
      if (Arg *FinalOutput = args.getLastArg(options::OPT_o))
         return FinalOutput->getValue();
      if (filetypes::is_textual(jobAction->getType()))
         return "-";
   }

   assert(!baseInput.empty() &&
          "arg Job which produces output must have a baseInput!");
   StringRef baseName(baseInput);
   if (isa<MergeModuleJobAction>(jobAction) ||
       (outputInfo.compilerMode == OutputInfo::Mode::SingleCompile &&
        !outputInfo.isMultiThreading()) ||
       jobAction->getType() == filetypes::TY_Image) {
      baseName = outputInfo.moduleName;
   }
   // We don't yet have a name, assign one.
   if (!atTopLevel) {
      return assign_output_name(compilation, jobAction, buffer, baseName, shouldPreserveOnSignal);
   }

   if (jobAction->getType() == filetypes::TY_Image) {
      const llvm::Triple &Triple = compilation.getToolChain().getTriple();
      SmallString<16> Base =
            baseNameForImage(jobAction, outputInfo, Triple, buffer, baseInput, baseName);
      form_filename_from_base_and_ext(Base, /*newExt=*/"", workingDirectory, buffer);
      return buffer.str();
   }

   StringRef suffix = filetypes::get_extension(jobAction->getType());
   assert(suffix.data() &&
          "All types used for output should have a suffix.");

   form_filename_from_base_and_ext(llvm::sys::path::filename(baseName), suffix,
                                   workingDirectory, buffer);
   return buffer.str();
}

static bool has_existing_additional_output(CommandOutput &output,
                                           filetypes::FileTypeId outputType,
                                           StringRef outputPath = StringRef())
{

   auto existing = output.getAdditionalOutputForType(outputType);
   if (!existing.empty()) {
      assert(outputPath.empty() || outputPath == existing);
      return true;
   }
   return false;
}

static void add_auxiliary_output(
      Compilation &compilation, CommandOutput &output, filetypes::FileTypeId outputType,
      const TypeToPathMap *outputMap, StringRef workingDirectory,
      StringRef outputPath = StringRef(),
      llvm::opt::OptSpecifier requireArg = llvm::opt::OptSpecifier())
{

   if (has_existing_additional_output(output, outputType, outputPath)) {
      return;
   }

   StringRef outputMapPath;
   if (outputMap) {
      auto iter = outputMap->find(outputType);
      if (iter != outputMap->end()) {
         outputMapPath = iter->second;
      }
   }

   if (!outputMapPath.empty()) {
      // Prefer a path from the outputMap.
      output.setAdditionalOutputForType(outputType, outputMapPath);
   } else if (!outputPath.empty()) {
      output.setAdditionalOutputForType(outputType, outputPath);
   } else if (requireArg.isValid() && !compilation.getArgs().getLastArg(requireArg)) {
      // This auxiliary output only exists if requireArg is passed, but it
      // wasn't this time.
      return;
   } else {
      // Put the auxiliary output file next to "the" primary output file.
      //
      // FIXME: when we're in WMO and have multiple primary outputs, we derive the
      // additional filename here from the _first_ primary output name, which
      // means that in the derived optionalOutputInfo (in Job.cpp) the additional output will
      // have a possibly-surprising name. But that's only half the problem: it
      // also get associated with the first primary _input_, even when there are
      // multiple primary inputs; really it should be associated with the build as
      // a whole -- derived optionalOutputInfo input "" -- but that's a more general thing to
      // fix.
      llvm::SmallString<128> path;
      if (output.getPrimaryOutputType() != filetypes::TY_Nothing) {
         path = output.getPrimaryOutputFilenames()[0];
      } else if (!output.getBaseInput(0).empty()) {
         path = llvm::sys::path::filename(output.getBaseInput(0));
      } else {
         form_filename_from_base_and_ext(compilation.getOutputInfo().moduleName, /*newExt=*/"",
                                         workingDirectory, path);
      }
      assert(!path.empty());

      bool isTempFile = compilation.isTemporaryFile(path);
      llvm::sys::path::replace_extension(
               path, filetypes::get_extension(outputType));
      output.setAdditionalOutputForType(outputType, path);
      if (isTempFile) {
         compilation.addTemporaryFile(path);
      }
   }
}

static void add_diag_file_output_for_persistent_pch_action(
      Compilation &compilation, const GeneratePCHJobAction *jobAction, CommandOutput &output,
      const TypeToPathMap *outputMap, StringRef workingDirectory)
{
   assert(jobAction->isPersistentPCH());

   // For a persistent pch we don't use an output, the frontend determines
   // the filename to use for the pch. For the diagnostics file, try to
   // determine an invocation-specific path inside the directory where the
   // pch is going to be written, and fallback to a temporary file if we
   // cannot determine such a path.

   StringRef pchOutDir = jobAction->getPersistentPCHDir();
   StringRef headerPath = output.getBaseInput(jobAction->getInputIndex());
   StringRef stem = llvm::sys::path::stem(headerPath);
   StringRef suffix =
         filetypes::get_extension(filetypes::TY_SerializedDiagnostics);
   SmallString<256> outPathBuf;

   if (const Arg *arg = compilation.getArgs().getLastArg(options::OPT_emit_module_path)) {
      // The module file path is unique for a specific module and architecture
      // (it won't be concurrently written to) so we can use the path as hash
      // for determining the filename to use for the diagnostic file.
      StringRef moduleOutPath = arg->getValue();
      outPathBuf = pchOutDir;
      llvm::sys::path::append(outPathBuf, stem);
      outPathBuf += '-';
      auto code = llvm::hash_value(moduleOutPath);
      outPathBuf += llvm::APInt(64, code).toString(36, /*Signed=*/false);
      llvm::sys::path::replace_extension(outPathBuf, suffix);
   }

   if (outPathBuf.empty()) {
      // Fallback to creating a temporary file.
      std::error_code errorCode =
            llvm::sys::fs::createTemporaryFile(stem, suffix, outPathBuf);
      if (errorCode) {
         compilation.getDiags().diagnose(SourceLoc(),
                                         diag::error_unable_to_make_temporary_file,
                                         errorCode.message());
         return;
      }
      compilation.addTemporaryFile(outPathBuf.str());
   }

   if (!outPathBuf.empty()) {
      add_auxiliary_output(compilation, output, filetypes::TY_SerializedDiagnostics,
                           outputMap, workingDirectory, outPathBuf.str());
   }
}

/// If the file at \p input has not been modified since the last build (i.e. its
/// mtime has not changed), adjust the Job's condition accordingly.
static void
handle_compile_job_condition(Job *job, CompileJobAction::InputInfo inputInfo,
                             StringRef input, bool alwaysRebuildDependents)
{
   if (inputInfo.status == CompileJobAction::InputInfo::NewlyAdded) {
      job->setCondition(Job::Condition::NewlyAdded);
      return;
   }

   bool hasValidModTime = false;
   llvm::sys::fs::file_status inputStatus;
   if (!llvm::sys::fs::status(input, inputStatus)) {
      job->setInputModTime(inputStatus.getLastModificationTime());
      hasValidModTime = true;
   }

   Job::Condition condition;
   if (hasValidModTime && job->getInputModTime() == inputInfo.previousModTime) {
      switch (inputInfo.status) {
      case CompileJobAction::InputInfo::UpToDate:
         if (llvm::sys::fs::exists(job->getOutput().getPrimaryOutputFilename())) {
            condition = Job::Condition::CheckDependencies;
         } else {
            condition = Job::Condition::RunWithoutCascading;
         }
         break;
      case CompileJobAction::InputInfo::NeedsCascadingBuild:
         condition = Job::Condition::Always;
         break;
      case CompileJobAction::InputInfo::NeedsNonCascadingBuild:
         condition = Job::Condition::RunWithoutCascading;
         break;
      case CompileJobAction::InputInfo::NewlyAdded:
         llvm_unreachable("handled above");
      }
   } else {
      if (alwaysRebuildDependents ||
          inputInfo.status == CompileJobAction::InputInfo::NeedsCascadingBuild) {
         condition = Job::Condition::Always;
      } else {
         condition = Job::Condition::RunWithoutCascading;
      }
   }

   job->setCondition(condition);
}

Job *Driver::buildJobsForAction(Compilation &compilation, const JobAction *jobAction,
                                const OutputFileMap *optionalOutputInfo,
                                StringRef workingDirectory,
                                bool atTopLevel, JobCacheMap &jobCache) const
{

   PrettyStackTraceDriverAction crashInfo("building jobs", jobAction);

   // 1. See if we've already got this cached.
   const ToolChain &toolchain = compilation.getToolChain();
   std::pair<const Action *, const ToolChain *> Key(jobAction, &toolchain);
   {
      auto CacheIter = jobCache.find(Key);
      if (CacheIter != jobCache.end()) {
         return CacheIter->second;
      }
   }

   // 2. Build up the list of input jobs.
   SmallVector<const Action *, 4> inputActions;
   SmallVector<const Job *, 4> inputJobs;
   for (const Action *input : *jobAction) {
      if (auto *inputJobAction = dyn_cast<JobAction>(input)) {
         inputJobs.push_back(buildJobsForAction(
                                compilation, inputJobAction, optionalOutputInfo, workingDirectory, false, jobCache));
      } else {
         inputActions.push_back(input);
      }
   }

   // 3. Determine the CommandOutput for the job.
   StringRef baseInput;
   StringRef primaryInput;
   if (!inputActions.empty()) {
      // Use the first InputAction as our baseInput and primaryInput.
      const InputAction *IA = cast<InputAction>(inputActions[0]);
      baseInput = IA->getInputArg().getValue();
      primaryInput = baseInput;
   } else if (!inputJobs.empty()) {
      const CommandOutput &Out = inputJobs.front()->getOutput();
      size_t i = jobAction->getInputIndex();
      // Use the first Job's baseInput as our baseInput.
      baseInput = Out.getBaseInput(i);
      // Use the first Job's Primary output as our primaryInput.
      primaryInput = Out.getPrimaryOutputFilenames()[i];
   }

   // With -index-file option, the primary input is the one passed with
   // -index-file-path.
   // FIXME: Figure out how this better fits within the driver infrastructure.
   if (jobAction->getType() == filetypes::TY_IndexData) {
      if (Arg *arg = compilation.getArgs().getLastArg(options::OPT_index_file_path)) {
         baseInput = arg->getValue();
         primaryInput = arg->getValue();
      }
   }

   const OutputInfo &outputInfo = compilation.getOutputInfo();
   const TypeToPathMap *outputMap = nullptr;
   if (optionalOutputInfo) {
      if (isa<CompileJobAction>(jobAction)) {
         if (outputInfo.compilerMode == OutputInfo::Mode::SingleCompile) {
            outputMap = optionalOutputInfo->getOutputMapForSingleOutput();
         } else {
            outputMap = optionalOutputInfo->getOutputMapForInput(baseInput);
         }
      } else if (isa<BackendJobAction>(jobAction)) {
         outputMap = optionalOutputInfo->getOutputMapForInput(baseInput);
      }
   }

   std::unique_ptr<CommandOutput> output(
            new CommandOutput(jobAction->getType(), compilation.getDerivedOutputFileMap()));

   PrettyStackTraceDriverCommandOutput CrashInfo2("determining output",
                                                  output.get());
   llvm::SmallString<128> buffer;
   computeMainOutput(compilation, jobAction, optionalOutputInfo, atTopLevel, inputActions, inputJobs, outputMap,
                     workingDirectory, baseInput, primaryInput, buffer,
                     output.get());

   if (outputInfo.shouldGenerateModule && isa<CompileJobAction>(jobAction)) {
      choosePolarphpModuleOutputPath(compilation, outputMap, workingDirectory, output.get());
   }

   if (outputInfo.shouldGenerateModule &&
       (isa<CompileJobAction>(jobAction) || isa<MergeModuleJobAction>(jobAction))) {
      choosePolarphpModuleDocOutputPath(compilation, outputMap, workingDirectory,
                                        output.get());
   }

   if (compilation.getArgs().hasArg(options::OPT_emit_module_interface,
                                    options::OPT_emit_module_interface_path)) {
      chooseParseableInterfacePath(compilation, jobAction, workingDirectory, buffer, output.get());
   }

   if (compilation.getArgs().hasArg(options::OPT_update_code) && isa<CompileJobAction>(jobAction)) {
      chooseRemappingOutputPath(compilation, outputMap, output.get());
   }
   if (isa<CompileJobAction>(jobAction) || isa<GeneratePCHJobAction>(jobAction)) {
      // Choose the serialized diagnostics output path.
      if (compilation.getArgs().hasArg(options::OPT_serialize_diagnostics)) {
         chooseSerializedDiagnosticsPath(compilation, jobAction, outputMap, workingDirectory,
                                         output.get());
      }
   }

   if (isa<MergeModuleJobAction>(jobAction) ||
       (isa<CompileJobAction>(jobAction) &&
        outputInfo.compilerMode == OutputInfo::Mode::SingleCompile)) {
      // An emit-tbd argument gets passed down to a job that sees the whole
      // module, either the -merge-modules job or a -wmo compiler invocation.
      chooseTBDPath(compilation, outputMap, workingDirectory, buffer, output.get());
   }

   if (isa<CompileJobAction>(jobAction)) {
      chooseDependenciesOutputPaths(compilation, outputMap, workingDirectory, buffer,
                                    output.get());
   }

   if (compilation.getArgs().hasArg(options::OPT_save_optimization_record,
                                    options::OPT_save_optimization_record_path)) {
      chooseOptimizationRecordPath(compilation, workingDirectory, buffer, output.get());
   }




   // 4. Construct a Job which produces the right CommandOutput.
   std::unique_ptr<Job> ownedJob = toolchain.constructJob(*jobAction, compilation, std::move(inputJobs),
                                                          inputActions,
                                                          std::move(output), outputInfo);
   Job *job = compilation.addJob(std::move(ownedJob));

   // If we track dependencies for this job, we may be able to avoid running it.
   if (!job->getOutput()
       .getAdditionalOutputForType(filetypes::TY_PolarDeps)
       .empty()) {
      if (inputActions.size() == 1) {
         auto compileJob = cast<CompileJobAction>(jobAction);
         bool alwaysRebuildDependents =
               compilation.getArgs().hasArg(options::OPT_driver_always_rebuild_dependents);
         handle_compile_job_condition(job, compileJob->getInputInfo(), baseInput,
                                      alwaysRebuildDependents);
      }
   }

   // 5. Add it to the jobCache, so we don't construct the same Job multiple
   // times.
   jobCache[Key] = job;

   if (m_driverPrintBindings) {
      llvm::outs() << "# \"" << toolchain.getTriple().str()
                   << "\" - \"" << llvm::sys::path::filename(job->getExecutable())
                   << "\", inputs: [";

      interleave(inputActions.begin(), inputActions.end(),
                 [](const Action *arg) {
         auto input = cast<InputAction>(arg);
         llvm::outs() << '"' << input->getInputArg().getValue() << '"';
      },
      [] { llvm::outs() << ", "; });
      if (!inputActions.empty() && !job->getInputs().empty())
         llvm::outs() << ", ";
      interleave(job->getInputs().begin(), job->getInputs().end(),
                 [](const Job *input) {
         auto FileNames = input->getOutput().getPrimaryOutputFilenames();
         interleave(FileNames.begin(), FileNames.end(),
                    [](const std::string &FileName) {
            llvm::outs() << '"' << FileName << '"';
         },
         [] { llvm::outs() << ", "; });
      },
      [] { llvm::outs() << ", "; });

      llvm::outs() << "], output: {";
      auto OutputFileNames = job->getOutput().getPrimaryOutputFilenames();
      StringRef TypeName =
            filetypes::get_type_name(job->getOutput().getPrimaryOutputType());
      interleave(OutputFileNames.begin(), OutputFileNames.end(),
                 [TypeName](const std::string &FileName) {
         llvm::outs() << TypeName << ": \"" << FileName << '"';
      },
      [] { llvm::outs() << ", "; });

      filetypes::for_all_types([&job](filetypes::FileTypeId type) {
         StringRef AdditionalOutput =
               job->getOutput().getAdditionalOutputForType(type);
         if (!AdditionalOutput.empty()) {
            llvm::outs() << ", " << filetypes::get_type_name(type) << ": \""
                         << AdditionalOutput << '"';
         }
      });
      llvm::outs() << '}';

      switch (job->getCondition()) {
      case Job::Condition::Always:
         break;
      case Job::Condition::RunWithoutCascading:
         llvm::outs() << ", condition: run-without-cascading";
         break;
      case Job::Condition::CheckDependencies:
         llvm::outs() << ", condition: check-dependencies";
         break;
      case Job::Condition::NewlyAdded:
         llvm::outs() << ", condition: newly-added";
         break;
      }

      llvm::outs() << '\n';
   }

   return job;
}

void Driver::computeMainOutput(
      Compilation &compilation, const JobAction *jobAction, const OutputFileMap *optionalOutputInfo,
      bool atTopLevel, SmallVectorImpl<const Action *> &inputActions,
      SmallVectorImpl<const Job *> &inputJobs, const TypeToPathMap *outputMap,
      StringRef workingDirectory, StringRef baseInput, StringRef primaryInput,
      llvm::SmallString<128> &buffer, CommandOutput *output) const {
   StringRef OutputFile;
   if (compilation.getOutputInfo().isMultiThreading() && isa<CompileJobAction>(jobAction) &&
       filetypes::is_after_llvm(jobAction->getType())) {
      // Multi-threaded compilation: arg single frontend command produces multiple
      // output file: one for each input files.

      // In batch mode, the driver will try to reserve multiple differing main
      // outputs to a bridging header. Another assertion will trip, but the cause
      // will be harder to track down. Since the driver now ignores -num-threads
      // in batch mode, the user should never be able to falsify this assertion.
      assert(!compilation.getBatchModeEnabled() && "Batch mode produces only one main "
                                                   "output per input action, cannot have "
                                                   "batch mode & num-threads");

      auto OutputFunc = [&](StringRef Base, StringRef Primary) {
         const TypeToPathMap *OMForInput = nullptr;
         if (optionalOutputInfo)
            OMForInput = optionalOutputInfo->getOutputMapForInput(Base);

         OutputFile = getOutputFilename(compilation, jobAction, OMForInput, workingDirectory,
                                        atTopLevel, Base, Primary, buffer);
         output->addPrimaryOutput(CommandInputPair(Base, Primary),
                                  OutputFile);
      };
      // Add an output file for each input action.
      for (const Action *arg : inputActions) {
         const InputAction *IA = cast<InputAction>(arg);
         StringRef IV = IA->getInputArg().getValue();
         OutputFunc(IV, IV);
      }
      // Add an output file for each primary output of each input job.
      for (const Job *IJ : inputJobs) {
         size_t i = 0;
         CommandOutput const &Out = IJ->getOutput();
         for (auto OutPrimary : Out.getPrimaryOutputFilenames()) {
            OutputFunc(Out.getBaseInput(i++), OutPrimary);
         }
      }
   } else {
      // The common case: there is a single output file.
      OutputFile = getOutputFilename(compilation, jobAction, outputMap, workingDirectory,
                                     atTopLevel, baseInput, primaryInput, buffer);
      output->addPrimaryOutput(CommandInputPair(baseInput, primaryInput),
                               OutputFile);
   }
}

void Driver::choosePolarphpModuleOutputPath(Compilation &compilation,
                                            const TypeToPathMap *outputMap,
                                            StringRef workingDirectory,
                                            CommandOutput *output) const {

   if (has_existing_additional_output(*output, filetypes::TY_PolarModuleFile)) {
      return;
   }
   StringRef ofmModuleOutputPath;
   if (outputMap) {
      auto iter = outputMap->find(filetypes::TY_PolarModuleFile);
      if (iter != outputMap->end()) {
          ofmModuleOutputPath = iter->second;
      }
   }

   const OutputInfo &outputInfo = compilation.getOutputInfo();
   const Arg *arg = compilation.getArgs().getLastArg(options::OPT_emit_module_path);
   using filetypes::TY_PolarModuleFile;

   if (!ofmModuleOutputPath.empty()) {
      // Prefer a path from the outputMap.
      output->setAdditionalOutputForType(TY_PolarModuleFile, ofmModuleOutputPath);
   } else if (arg && outputInfo.compilerMode == OutputInfo::Mode::SingleCompile) {
      // We're performing a single compilation (and thus no merge module step),
      // so prefer to use -emit-module-path, if present.
      output->setAdditionalOutputForType(TY_PolarModuleFile, arg->getValue());
   } else if (output->getPrimaryOutputType() == TY_PolarModuleFile) {
      // If the primary type is already a module type, we're out of
      // options for overriding the primary name choice: stop now.
      assert(!output->getPrimaryOutputFilename().empty());
      return;
   } else if (outputInfo.compilerMode == OutputInfo::Mode::SingleCompile &&
              outputInfo.shouldTreatModuleAsTopLevelOutput) {
      // We're performing a single compile and don't have -emit-module-path,
      // but have been told to treat the module as a top-level output.
      // Determine an appropriate path.
      llvm::SmallString<128> path;
      if (const Arg *arg = compilation.getArgs().getLastArg(options::OPT_o)) {
         // Put the module next to the top-level output.
         path = arg->getValue();
         llvm::sys::path::remove_filename(path);
      } else {
         // arg top-level output wasn't specified, so just output to
         // <ModuleName>.swiftmodule in the current directory.
      }
      llvm::sys::path::append(path, outputInfo.moduleName);
      llvm::sys::path::replace_extension(
               path, filetypes::get_extension(TY_PolarModuleFile));
      output->setAdditionalOutputForType(TY_PolarModuleFile, path);
   } else if (output->getPrimaryOutputType() != filetypes::TY_Nothing) {
      // We're only generating the module as an intermediate, so put it next
      // to the primary output of the compile command.
      llvm::SmallString<128> path(output->getPrimaryOutputFilenames()[0]);
      assert(!path.empty());
      bool isTempFile = compilation.isTemporaryFile(path);
      llvm::sys::path::replace_extension(
               path, filetypes::get_extension(TY_PolarModuleFile));
      output->setAdditionalOutputForType(TY_PolarModuleFile, path);
      if (isTempFile)
         compilation.addTemporaryFile(path);
   }
}

void Driver::choosePolarphpModuleDocOutputPath(Compilation &compilation,
                                               const TypeToPathMap *outputMap,
                                               StringRef workingDirectory,
                                               CommandOutput *output) const
{

   if (has_existing_additional_output(*output, filetypes::TY_PolarModuleDocFile)) {
      return;
   }

   StringRef ofmModuleDocOutputPath;
   if (outputMap) {
      auto iter = outputMap->find(filetypes::TY_PolarModuleDocFile);
      if (iter != outputMap->end()) {
         ofmModuleDocOutputPath = iter->second;
      }
   }
   if (!ofmModuleDocOutputPath.empty()) {
      // Prefer a path from the outputMap.
      output->setAdditionalOutputForType(filetypes::TY_PolarModuleDocFile,
                                         ofmModuleDocOutputPath);
   } else if (output->getPrimaryOutputType() != filetypes::TY_Nothing) {
      // Otherwise, put it next to the swiftmodule file.
      llvm::SmallString<128> path(
               output->getAnyOutputForType(filetypes::TY_PolarModuleFile));
      bool isTempFile = compilation.isTemporaryFile(path);
      llvm::sys::path::replace_extension(
               path, filetypes::get_extension(filetypes::TY_PolarModuleDocFile));
      output->setAdditionalOutputForType(filetypes::TY_PolarModuleDocFile, path);
      if (isTempFile) {
         compilation.addTemporaryFile(path);
      }
   }
}

void Driver::chooseRemappingOutputPath(Compilation &compilation,
                                       const TypeToPathMap *outputMap,
                                       CommandOutput *output) const
{

   if (has_existing_additional_output(*output, filetypes::TY_Remapping)) {
      return;
   }

   StringRef ofmFixitsOutputPath;
   if (outputMap) {
      auto iter = outputMap->find(filetypes::TY_Remapping);
      if (iter != outputMap->end()) {
         ofmFixitsOutputPath = iter->second;
      }
   }
   if (!ofmFixitsOutputPath.empty()) {
      output->setAdditionalOutputForType(filetypes::FileTypeId::TY_Remapping,
                                         ofmFixitsOutputPath);
   } else {
      llvm::SmallString<128> path(output->getPrimaryOutputFilenames()[0]);
      bool isTempFile = compilation.isTemporaryFile(path);
      llvm::sys::path::replace_extension(path,
                                         filetypes::get_extension(filetypes::FileTypeId::TY_Remapping));
      output->setAdditionalOutputForType(filetypes::FileTypeId::TY_Remapping, path);
      if (isTempFile) {
         compilation.addTemporaryFile(path);
      }
   }
}

void Driver::chooseParseableInterfacePath(Compilation &compilation, const JobAction *jobAction,
                                          StringRef workingDirectory,
                                          llvm::SmallString<128> &buffer,
                                          CommandOutput *output) const
{
   switch (compilation.getOutputInfo().compilerMode) {
   case OutputInfo::Mode::StandardCompile:
   case OutputInfo::Mode::BatchModeCompile:
      if (!isa<MergeModuleJobAction>(jobAction)) {
         return;
      }
      break;
   case OutputInfo::Mode::SingleCompile:
      if (!isa<CompileJobAction>(jobAction)) {
         return;
      }
      break;
   case OutputInfo::Mode::Immediate:
   case OutputInfo::Mode::REPL:
      llvm_unreachable("these modes aren't usable with 'swiftc'");
   }

   StringRef outputPath = *getOutputFilenameFromPathArgOrAsTopLevel(
            compilation.getOutputInfo(), compilation.getArgs(),
            options::OPT_emit_module_interface_path,
            filetypes::TY_PolarParseableInterfaceFile,
            /*treatAsTopLevelOutput*/true, workingDirectory, buffer);
   output->setAdditionalOutputForType(filetypes::TY_PolarParseableInterfaceFile,
                                      outputPath);
}

void Driver::chooseSerializedDiagnosticsPath(Compilation &compilation,
                                             const JobAction *jobAction,
                                             const TypeToPathMap *outputMap,
                                             StringRef workingDirectory,
                                             CommandOutput *output) const
{
   if (compilation.getArgs().hasArg(options::OPT_serialize_diagnostics)) {
      auto pchJA = dyn_cast<GeneratePCHJobAction>(jobAction);
      if (pchJA && pchJA->isPersistentPCH()) {
         add_diag_file_output_for_persistent_pch_action(compilation, pchJA, *output, outputMap,
                                                        workingDirectory);
      } else {
         add_auxiliary_output(compilation, *output, filetypes::TY_SerializedDiagnostics,
                              outputMap, workingDirectory);
      }

      // Remove any existing diagnostics files so that clients can detect their
      // presence to determine if a command was run.
      StringRef outputPath =
            output->getAnyOutputForType(filetypes::TY_SerializedDiagnostics);
      if (llvm::sys::fs::is_regular_file(outputPath)) {
         llvm::sys::fs::remove(outputPath);
      }
   }
}

void Driver::chooseDependenciesOutputPaths(Compilation &compilation,
                                           const TypeToPathMap *outputMap,
                                           StringRef workingDirectory,
                                           llvm::SmallString<128> &buffer,
                                           CommandOutput *output) const
{
   if (compilation.getArgs().hasArg(options::OPT_emit_dependencies)) {
      add_auxiliary_output(compilation, *output, filetypes::TY_Dependencies, outputMap,
                           workingDirectory);
   }
   if (compilation.getIncrementalBuildEnabled()) {
      add_auxiliary_output(compilation, *output, filetypes::TY_PolarDeps, outputMap,
                           workingDirectory);
   }
   chooseLoadedModuleTracePath(compilation, workingDirectory, buffer, output);
}

void Driver::chooseLoadedModuleTracePath(Compilation &compilation,
                                         StringRef workingDirectory,
                                         llvm::SmallString<128> &buffer,
                                         CommandOutput *output) const {
   // The loaded-module-trace is the same for all compile jobs: all `import`
   // statements are processed, even ones from non-primary files. Thus, only
   // one of those jobs needs to emit the file, and we can get it to write
   // straight to the desired final location.
   auto tracePathEnvVar = getenv("SWIFT_LOADED_MODULE_TRACE_FILE");
   auto shouldEmitTrace =
         tracePathEnvVar ||
         compilation.getArgs().hasArg(options::OPT_emit_loaded_module_trace,
                                      options::OPT_emit_loaded_module_trace_path);

   if (shouldEmitTrace &&
       compilation.requestPermissionForFrontendToEmitLoadedModuleTrace()) {
      StringRef filename;
      // Prefer the environment variable.
      if (tracePathEnvVar) {
         filename = StringRef(tracePathEnvVar);
      } else {
         // By treating this as a top-level output, the return value always
         // exists.
         filename = *getOutputFilenameFromPathArgOrAsTopLevel(
                  compilation.getOutputInfo(), compilation.getArgs(),
                  options::OPT_emit_loaded_module_trace_path,
                  filetypes::TY_ModuleTrace,
                  /*treatAsTopLevelOutput=*/true, workingDirectory, buffer);
      }

      output->setAdditionalOutputForType(filetypes::TY_ModuleTrace, filename);
   }
}

void Driver::chooseTBDPath(Compilation &compilation, const TypeToPathMap *outputMap,
                           StringRef workingDirectory,
                           llvm::SmallString<128> &buffer,
                           CommandOutput *output) const
{
   StringRef pathFromArgs;
   if (const Arg *arg = compilation.getArgs().getLastArg(options::OPT_emit_tbd_path)) {
      pathFromArgs = arg->getValue();
   }
   add_auxiliary_output(compilation, *output, filetypes::TY_TBD, outputMap,
                        workingDirectory, pathFromArgs,
                        /*requireArg=*/options::OPT_emit_tbd);
}

void Driver::chooseOptimizationRecordPath(Compilation &compilation,
                                          StringRef workingDirectory,
                                          llvm::SmallString<128> &buffer,
                                          CommandOutput *output) const
{
   const OutputInfo &outputInfo = compilation.getOutputInfo();
   if (outputInfo.compilerMode == OutputInfo::Mode::SingleCompile) {
      auto filename = *getOutputFilenameFromPathArgOrAsTopLevel(
               outputInfo, compilation.getArgs(), options::OPT_save_optimization_record_path,
               filetypes::TY_OptRecord, /*treatAsTopLevelOutput=*/true,
               workingDirectory, buffer);

      output->setAdditionalOutputForType(filetypes::TY_OptRecord, filename);
   } else {
      // FIXME: We should use the outputMap in this case.
      m_diags.diagnose({}, diag::warn_opt_remark_disabled);
   }
}

static unsigned print_actions(const Action *arg,
                              llvm::DenseMap<const Action *, unsigned> &ids)
{
   if (ids.count(arg)) {
      return ids[arg];
   }
   std::string str;
   llvm::raw_string_ostream os(str);

   os << Action::getClassName(arg->getKind()) << ", ";
   if (const auto *IA = dyn_cast<InputAction>(arg)) {
      os << "\"" << IA->getInputArg().getValue() << "\"";
   } else {
      os << "{";
      interleave(*cast<JobAction>(arg),
                 [&](const Action *input) { os << print_actions(input, ids); },
      [&] { os << ", "; });
      os << "}";
   }

   unsigned id = ids.size();
   ids[arg] = id;
   llvm::errs() << id << ": " << os.str() << ", "
                << filetypes::get_type_name(arg->getType()) << "\n";

   return id;
}

void Driver::printActions(const Compilation &compilation) const
{
   llvm::DenseMap<const Action *, unsigned> ids;
   for (const Action *arg : compilation.getActions()) {
      print_actions(arg, ids);
   }
}

void Driver::printVersion(const ToolChain &toolchain, raw_ostream &ostream) const
{
   ostream << version::retrieve_polarphp_full_version(
                 version::Version::getCurrentLanguageVersion()) << '\n';
   ostream << "Target: " << toolchain.getTriple().str() << '\n';
}

void Driver::printHelp(bool showHidden) const
{
   unsigned includedFlagsBitmask = 0;
   unsigned excludedFlagsBitmask = options::NoDriverOption;

   switch (m_driverKind) {
   case DriverKind::Interactive:
      excludedFlagsBitmask |= options::NoInteractiveOption;
      break;
   case DriverKind::Batch:
   case DriverKind::AutolinkExtract:
   case DriverKind::PolarphpFormat:
      excludedFlagsBitmask |= options::NoBatchOption;
      break;
   }

   if (!showHidden) {
      excludedFlagsBitmask |= HelpHidden;
   }
   getOpts().PrintHelp(llvm::outs(), m_name.c_str(), "polarphp compiler",
                       includedFlagsBitmask, excludedFlagsBitmask,
                       /*ShowAllAliases*/false);
}

bool OutputInfo::mightHaveExplicitPrimaryInputs(const CommandOutput &output) const
{
   switch (compilerMode) {
   case Mode::StandardCompile:
   case Mode::BatchModeCompile:
      return true;
   case Mode::SingleCompile:
      return false;
   case Mode::Immediate:
   case Mode::REPL:
      llvm_unreachable("REPL and immediate modes handled elsewhere");
   }
   llvm_unreachable("unhandled mode");
}

} // polar::driver
