//===------ DarwinToolChains.cpp - Job invocations (Darwin-specific) ------===//
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
#include "llvm/Support/VersionTuple.h"

namespace polar::driver::toolchains {

using namespace llvm::opt;
using namespace polar::basic;

std::string
Darwin::findProgramRelativeToPolarphpImpl(StringRef name) const
{
   StringRef polarphpPath = getDriver().getPolarphpProgramPath();
   StringRef polarphpBinDir = llvm::sys::path::parent_path(polarphpPath);
   // See if we're in an Xcode toolchain.
   bool hasToolchain = false;
   llvm::SmallString<128> path{polarphpBinDir};
   llvm::sys::path::remove_filename(path); // bin
   llvm::sys::path::remove_filename(path); // usr
   if (llvm::sys::path::extension(path) == ".xctoolchain") {
      hasToolchain = true;
      llvm::sys::path::remove_filename(path); // *.xctoolchain
      llvm::sys::path::remove_filename(path); // Toolchains
      llvm::sys::path::append(path, "usr", "bin");
   }

   StringRef paths[] = {polarphpBinDir, path};
   auto pathsRef = llvm::makeArrayRef(paths);
   if (!hasToolchain) {
      pathsRef = pathsRef.drop_back();
   }
   auto result = llvm::sys::findProgramByName(name, pathsRef);
   if (result) {
      return result.get();
   }
   return {};
}

ToolChain::InvocationInfo
Darwin::constructInvocation(const InterpretJobAction &job,
                            const JobContext &context) const
{
   InvocationInfo invocationInfo = ToolChain::constructInvocation(job, context);

   SmallVector<std::string, 4> runtimeLibraryPaths;
   getRuntimeLibraryPaths(runtimeLibraryPaths, context.args, context.outputInfo.sdkPath,
                          /*Shared=*/true);

   addPathEnvironmentVariableIfNeeded(invocationInfo.extraEnvironment, "DYLD_LIBRARY_PATH",
                                      ":", options::OPT_L, context.args,
                                      runtimeLibraryPaths);
   addPathEnvironmentVariableIfNeeded(invocationInfo.extraEnvironment, "DYLD_FRAMEWORK_PATH",
                                      ":", options::OPT_F, context.args);
   // FIXME: Add options::OPT_Fsystem paths to DYLD_FRAMEWORK_PATH as well.
   return invocationInfo;
}

static StringRef
get_darwin_library_name_suffix_for_triple(const llvm::Triple &triple,
                                          bool distinguishSimulator = true)
{
   const DarwinPlatformKind kind = get_darwin_platform_kind(triple);
   const DarwinPlatformKind effectiveKind =
         distinguishSimulator ? kind : get_non_simulator_platform(kind);
   switch (effectiveKind) {
   case DarwinPlatformKind::MacOS:
      return "osx";
   case DarwinPlatformKind::IPhoneOS:
      return "ios";
   case DarwinPlatformKind::IPhoneOSSimulator:
      return "iossim";
   case DarwinPlatformKind::TvOS:
      return "tvos";
   case DarwinPlatformKind::TvOSSimulator:
      return "tvossim";
   case DarwinPlatformKind::WatchOS:
      return "watchos";
   case DarwinPlatformKind::WatchOSSimulator:
      return "watchossim";
   }
   llvm_unreachable("Unsupported Darwin platform");
}

std::string Darwin::sanitizerRuntimeLibName(StringRef Sanitizer,
                                            bool shared) const
{
   return (Twine("libclang_rt.") + Sanitizer + "_" +
           get_darwin_library_name_suffix_for_triple(this->getTriple()) +
           (shared ? "_dynamic.dylib" : ".a"))
         .str();
}

static void add_link_runtime_lib_rpath(const ArgList &args,
                                       ArgStringList &arguments,
                                       StringRef darwinLibName,
                                       const ToolChain &toolchain)
{
   // Adding the rpaths might negatively interact when other rpaths are involved,
   // so we should make sure we add the rpaths last, after all user-specified
   // rpaths. This is currently true from this place, but we need to be
   // careful if this function is ever called before user's rpaths are emitted.
   assert(darwinLibName.endswith(".dylib") && "must be a dynamic library");

   // Add @executable_path to rpath to support having the dylib copied with
   // the executable.
   arguments.push_back("-rpath");
   arguments.push_back("@executable_path");

   // Add the path to the resource dir to rpath to support using the dylib
   // from the default location without copying.

   SmallString<128> clangLibraryPath;
   toolchain.getClangLibraryPath(args, clangLibraryPath);

   arguments.push_back("-rpath");
   arguments.push_back(args.MakeArgString(clangLibraryPath));
}

static void addLinkSanitizerLibArgsForDarwin(const ArgList &args,
                                             ArgStringList &arguments,
                                             StringRef Sanitizer,
                                             const ToolChain &toolchain,
                                             bool shared = true)
{
   // Sanitizer runtime libraries requires C++.
   arguments.push_back("-lc++");
   // Add explicit dependency on -lc++abi, as -lc++ doesn't re-export
   // all RTTI-related symbols that are used.
   arguments.push_back("-lc++abi");
   auto libName = toolchain.sanitizerRuntimeLibName(Sanitizer, shared);
   toolchain.addLinkRuntimeLib(args, arguments, libName);
   if (shared) {
      add_link_runtime_lib_rpath(args, arguments, libName, toolchain);
   }
}

/// Runs <code>xcrun -f clang</code> in order to find the location of Clang for
/// the currently active Xcode.
///
/// We get the "currently active" part by passing through the DEVELOPER_DIR
/// environment variable (along with the rest of the environment).
static bool find_xcode_clang_path(llvm::SmallVectorImpl<char> &path)
{
   assert(path.empty());
   auto xcrunPath = llvm::sys::findProgramByName("xcrun");
   if (!xcrunPath.getError()) {
      // Explicitly ask for the default toolchain so that we don't find a Clang
      // included with an open-source toolchain.
      const char *args[] = {"-toolchain", "default", "-f", "clang", nullptr};
      sys::TaskQueue queue;
      queue.addTask(xcrunPath->c_str(), args, /*Env=*/llvm::None,
                    /*Context=*/nullptr,
                    /*SeparateErrors=*/true);
      queue.execute(nullptr,
                    [&path](sys::ProcessId pid, int returnCode, StringRef output,
                    StringRef errors,
                    sys::TaskProcessInformation procInfo,
                    void *unused) -> sys::TaskFinishedResponse
      {
         if (returnCode == 0) {
            output = output.rtrim();
            path.append(output.begin(), output.end());
         }
         return sys::TaskFinishedResponse::ContinueExecution;
      });
   }
   return !path.empty();
}

static void add_version_string(const ArgList &inputArgs, ArgStringList &arguments,
                               unsigned major, unsigned minor, unsigned micro)
{
   llvm::SmallString<8> buf;
   llvm::raw_svector_ostream os{buf};
   os << major << '.' << minor << '.' << micro;
   arguments.push_back(inputArgs.MakeArgString(os.str()));
}

ToolChain::InvocationInfo
Darwin::constructInvocation(const DynamicLinkJobAction &job,
                            const JobContext &context) const
{
   assert(context.output.getPrimaryOutputType() == filetypes::TY_Image &&
          "Invalid linker output type.");

   if (context.args.hasFlag(options::OPT_static_executable,
                            options::OPT_no_static_executable, false)) {
      llvm::report_fatal_error("-static-executable is not supported on Darwin");
   }

   const Driver &driver = getDriver();
   const llvm::Triple &triple = getTriple();

   // Configure the toolchain.
   // By default, use the system `ld` to link.
   const char *ld = "ld";
   if (const Arg *arg = context.args.getLastArg(options::OPT_tools_directory)) {
      StringRef toolchainPath(arg->getValue());
      // If there is a 'ld' in the toolchain folder, use that instead.
      if (auto toolchainLD =
          llvm::sys::findProgramByName("ld", {toolchainPath})) {
         ld = context.args.MakeArgString(toolchainLD.get());
      }
   }

   InvocationInfo invocationInfo = {ld};
   ArgStringList &arguments = invocationInfo.arguments;

   if (context.shouldUseInputFileList()) {
      arguments.push_back("-filelist");
      arguments.push_back(context.getTemporaryFilePath("inputs", "LinkFileList"));
      invocationInfo.filelistInfos.push_back({arguments.back(), filetypes::TY_Object,
                                              FilelistInfo::WhichFiles::Input});
   } else {
      addPrimaryInputsOfType(arguments, context.inputs, context.args,
                             filetypes::TY_Object);
   }

   addInputsOfType(arguments, context.inputActions, filetypes::TY_Object);
   if (context.outputInfo.compilerMode == OutputInfo::Mode::SingleCompile) {
      addInputsOfType(arguments, context.inputs, context.args,
                      filetypes::TY_PolarModuleFile, "-add_ast_path");
   } else {
      addPrimaryInputsOfType(arguments, context.inputs, context.args,
                             filetypes::TY_PolarModuleFile, "-add_ast_path");
   }

   // Add all .polarmodule file inputs as arguments, preceded by the
   // "-add_ast_path" linker option.
   addInputsOfType(arguments, context.inputActions,
                   filetypes::TY_PolarModuleFile, "-add_ast_path");

   switch (job.getKind()) {
   case LinkKind::None:
      llvm_unreachable("invalid link kind");
   case LinkKind::Executable:
      // The default for ld; no extra flags necessary.
      break;
   case LinkKind::DynamicLibrary:
      arguments.push_back("-dylib");
      break;
   case LinkKind::StaticLibrary:
      llvm_unreachable("the dynamic linker cannot build static libraries");
   }

   assert(triple.isOSDarwin());

   // FIXME: If we used Clang as a linker instead of going straight to ld,
   // we wouldn't have to replicate a bunch of Clang's logic here.

   // Always link the regular compiler_rt if it's present.
   //
   // Note: Normally we'd just add this unconditionally, but it's valid to build
   // Swift and use it as a linker without building compiler_rt.
   SmallString<128> compilerRTPath;
   getClangLibraryPath(context.args, compilerRTPath);
   llvm::sys::path::append(
            compilerRTPath,
            Twine("libclang_rt.") +
            get_darwin_library_name_suffix_for_triple(triple, /*simulator*/false) +
            ".a");
   if (llvm::sys::fs::exists(compilerRTPath)) {
      arguments.push_back(context.args.MakeArgString(compilerRTPath));
   }
   for (const Arg *arg :
        context.args.filtered(options::OPT_F, options::OPT_Fsystem)) {
      arguments.push_back("-F");
      arguments.push_back(arg->getValue());
   }
   if (context.args.hasArg(options::OPT_enable_app_extension)) {
      // Keep this string fixed in case the option used by the
      // compiler itself changes.
      arguments.push_back("-application_extension");
   }
   // Linking sanitizers will add rpaths, which might negatively interact when
   // other rpaths are involved, so we should make sure we add the rpaths after
   // all user-specified rpaths.
   if (context.outputInfo.selectedSanitizers & SanitizerKind::Address) {
      addLinkSanitizerLibArgsForDarwin(context.args, arguments, "asan", *this);
   }
   if (context.outputInfo.selectedSanitizers & SanitizerKind::Thread)
      addLinkSanitizerLibArgsForDarwin(context.args, arguments, "tsan", *this);

   if (context.outputInfo.selectedSanitizers & SanitizerKind::Undefined) {
      addLinkSanitizerLibArgsForDarwin(context.args, arguments, "ubsan", *this);
   }
   // Only link in libFuzzer for executables.
   if (job.getKind() == LinkKind::Executable &&
       (context.outputInfo.selectedSanitizers & SanitizerKind::Fuzzer)) {
      addLinkSanitizerLibArgsForDarwin(context.args, arguments, "fuzzer", *this,
                                       /*shared=*/false);
   }
   if (context.args.hasArg(options::OPT_embed_bitcode,
                           options::OPT_embed_bitcode_marker)) {
      arguments.push_back("-bitcode_bundle");
   }
   if (!context.outputInfo.sdkPath.empty()) {
      arguments.push_back("-syslibroot");
      arguments.push_back(context.args.MakeArgString(context.outputInfo.sdkPath));
   }
   arguments.push_back("-lobjc");
   arguments.push_back("-lSystem");

   arguments.push_back("-arch");
   arguments.push_back(context.args.MakeArgString(getTriple().getArchName()));

   // Link compatibility libraries, if we're deploying back to OSes that
   // have an older Polarphp runtime.
   SmallString<128> SharedResourceDirPath;
   getResourceDirPath(SharedResourceDirPath, context.args, /*Shared=*/true);
   Optional<llvm::VersionTuple> runtimeCompatibilityVersion;
   if (context.args.hasArg(options::OPT_runtime_compatibility_version)) {
      auto value = context.args.getLastArgValue(
               options::OPT_runtime_compatibility_version);
      if (value.equals("5.0")) {
         runtimeCompatibilityVersion = llvm::VersionTuple(5, 0);
      } else if (value.equals("none")) {
         runtimeCompatibilityVersion = None;
      } else {
         // TODO: diagnose unknown runtime compatibility version?
      }
   } else if (job.getKind() == LinkKind::Executable) {
      runtimeCompatibilityVersion
            = get_runtime_compatibility_version_for_target(triple);
   }
   if (runtimeCompatibilityVersion) {
      if (*runtimeCompatibilityVersion <= llvm::VersionTuple(5, 0)) {
         // Polarphp 5.0 compatibility library
         SmallString<128> backDeployLib;
         backDeployLib.append(SharedResourceDirPath);
         llvm::sys::path::append(backDeployLib, "libswiftCompatibility50.a");
         if (llvm::sys::fs::exists(backDeployLib)) {
            arguments.push_back("-force_load");
            arguments.push_back(context.args.MakeArgString(backDeployLib));
         }
      }
   }

   if (job.getKind() == LinkKind::Executable) {
      if (runtimeCompatibilityVersion) {
         if (*runtimeCompatibilityVersion <= llvm::VersionTuple(5, 0)) {
            // Polarphp 5.0 dynamic replacement compatibility library.
            SmallString<128> backDeployLib;
            backDeployLib.append(SharedResourceDirPath);
            llvm::sys::path::append(backDeployLib,
                                    "libPolarphpCompatibilityDynamicReplacements.a");
            if (llvm::sys::fs::exists(backDeployLib)) {
               arguments.push_back("-force_load");
               arguments.push_back(context.args.MakeArgString(backDeployLib));
            }
         }
      }
   }

   bool wantsStaticStdlib =
         context.args.hasFlag(options::OPT_static_stdlib,
                              options::OPT_no_static_stdlib, false);

   SmallVector<std::string, 4> runtimeLibPaths;
   getRuntimeLibraryPaths(runtimeLibPaths, context.args,
                          context.outputInfo.sdkPath, /*Shared=*/!wantsStaticStdlib);

   // Add the runtime library link path, which is platform-specific and found
   // relative to the compiler.
   for (auto path : runtimeLibPaths) {
      arguments.push_back("-L");
      arguments.push_back(context.args.MakeArgString(path));
   }

   // Link the standard library.
   if (wantsStaticStdlib) {
      arguments.push_back("-lc++");
      arguments.push_back("-framework");
      arguments.push_back("Foundation");
      arguments.push_back("-force_load_polarphp_libs");
   } else {
      // FIXME: We probably shouldn't be adding an rpath here unless we know ahead
      // of time the standard library won't be copied. SR-1967
      for (auto path : runtimeLibPaths) {
         arguments.push_back("-rpath");
         arguments.push_back(context.args.MakeArgString(path));
      }
   }

   if (context.args.hasArg(options::OPT_profile_generate)) {
      SmallString<128> libProfile;
      getClangLibraryPath(context.args, libProfile);

      StringRef rt;
      if (triple.isiOS()) {
         if (triple.isTvOS()) {
             rt = "tvos";
         } else {
            rt = "ios";
         }
      } else if (triple.isWatchOS()) {
         rt = "watchos";
      } else {
         assert(triple.isMacOSX());
         rt = "osx";
      }

      StringRef simulator;
      if (triple_is_any_simulator(triple)) {
         simulator = "sim";
      }

      llvm::sys::path::append(libProfile,
                              "libclang_rt.profile_" + rt + simulator + ".a");

      // FIXME: Continue accepting the old path for simulator libraries for now.
      if (!simulator.empty() && !llvm::sys::fs::exists(libProfile)) {
         llvm::sys::path::remove_filename(libProfile);
         llvm::sys::path::append(libProfile, "libclang_rt.profile_" + rt + ".a");
      }

      arguments.push_back(context.args.MakeArgString(libProfile));
   }

   // FIXME: Properly handle deployment targets.
   assert(triple.isiOS() || triple.isWatchOS() || triple.isMacOSX());
   if (triple.isiOS()) {
      bool isiOSSimulator = triple_is_ios_simulator(triple);
      if (triple.isTvOS()) {
         if (isiOSSimulator)
            arguments.push_back("-tvos_simulator_version_min");
         else
            arguments.push_back("-tvos_version_min");
      } else {
         if (isiOSSimulator)
            arguments.push_back("-ios_simulator_version_min");
         else
            arguments.push_back("-iphoneos_version_min");
      }
      unsigned major, minor, micro;
      triple.getiOSVersion(major, minor, micro);
      add_version_string(context.args, arguments, major, minor, micro);
   } else if (triple.isWatchOS()) {
      if (triple_is_watch_simulator(triple)) {
         arguments.push_back("-watchos_simulator_version_min");
      } else {
         arguments.push_back("-watchos_version_min");
      }
      unsigned major, minor, micro;
      triple.getOSVersion(major, minor, micro);
      add_version_string(context.args, arguments, major, minor, micro);
   } else {
      arguments.push_back("-macosx_version_min");
      unsigned major, minor, micro;
      triple.getMacOSXVersion(major, minor, micro);
      add_version_string(context.args, arguments, major, minor, micro);
   }

   arguments.push_back("-no_objc_category_merging");

   // These custom arguments should be right before the object file at the end.
   context.args.AddAllArgs(arguments, options::OPT_linker_option_Group);
   context.args.AddAllArgValues(arguments, options::OPT_Xlinker);

   // This should be the last option, for convenience in checking output.
   arguments.push_back("-o");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));

   return invocationInfo;
}


ToolChain::InvocationInfo
Darwin::constructInvocation(const StaticLinkJobAction &job,
                            const JobContext &context) const
{
   assert(context.output.getPrimaryOutputType() == filetypes::TY_Image &&
          "Invalid linker output type.");

   // Configure the toolchain.
   const char *LibTool = "libtool";

   InvocationInfo invocationInfo = {LibTool};
   ArgStringList &arguments = invocationInfo.arguments;

   arguments.push_back("-static");

   if (context.shouldUseInputFileList()) {
      arguments.push_back("-filelist");
      arguments.push_back(context.getTemporaryFilePath("inputs", "LinkFileList"));
      invocationInfo.filelistInfos.push_back({arguments.back(), filetypes::TY_Object,
                                              FilelistInfo::WhichFiles::Input});
   } else {
      addPrimaryInputsOfType(arguments, context.inputs, context.args,
                             filetypes::TY_Object);
   }

   addInputsOfType(arguments, context.inputActions, filetypes::TY_Object);

   arguments.push_back("-o");

   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));

   return invocationInfo;
}

bool Darwin::shouldStoreInvocationInDebugInfo() const
{
   // This matches the behavior in Clang (see
   // clang/lib/driver/ToolChains/Darwin.cpp).
   if (const char *S = ::getenv("RC_DEBUG_OPTIONS")) {
      return S[0] != '\0';
   }
   return false;
}

} // polar::driver::toolchains
