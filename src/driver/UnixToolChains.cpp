//===------ UnixToolChains.cpp - Job invocations (non-Darwin Unix) --------===//
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
#include "polarphp/basic/TaskQueue.h"
#include "polarphp/global/Config.h"
#include "polarphp/driver/Compilation.h"
#include "polarphp/driver/Driver.h"
#include "polarphp/driver/Job.h"
#include "polarphp/option/Options.h"
#include "polarphp/option/SanitizerOptions.h"
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

namespace polar::driver::toolchains {

using namespace llvm::opt;

std::string
GenericUnix::sanitizerRuntimeLibName(StringRef Sanitizer,
                                     bool shared) const
{
   return (Twine("libclang_rt.") + Sanitizer + "-" +
           this->getTriple().getArchName() + ".a")
         .str();
}

ToolChain::InvocationInfo
GenericUnix::constructInvocation(const InterpretJobAction &job,
                                 const JobContext &context) const
{
   InvocationInfo invocationInfo = ToolChain::constructInvocation(job, context);

   SmallVector<std::string, 4> runtimeLibraryPaths;
   getRuntimeLibraryPaths(runtimeLibraryPaths, context.args, context.outputInfo.sdkPath,
                          /*Shared=*/true);

   addPathEnvironmentVariableIfNeeded(invocationInfo.extraEnvironment, "LD_LIBRARY_PATH",
                                      ":", options::OPT_L, context.args,
                                      runtimeLibraryPaths);
   return invocationInfo;
}

ToolChain::InvocationInfo GenericUnix::constructInvocation(
      const AutolinkExtractJobAction &job, const JobContext &context) const
{
   assert(context.output.getPrimaryOutputType() == filetypes::TY_AutolinkFile);

   InvocationInfo invocationInfo{"swift-autolink-extract"};
   ArgStringList &arguments = invocationInfo.arguments;
   invocationInfo.allowsResponseFiles = true;

   addPrimaryInputsOfType(arguments, context.inputs, context.args,
                          filetypes::TY_Object);
   addInputsOfType(arguments, context.inputActions, filetypes::TY_Object);

   arguments.push_back("-o");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));

   return invocationInfo;
}

std::string GenericUnix::getDefaultLinker() const
{
   switch (getTriple().getArch()) {
   case llvm::Triple::arm:
   case llvm::Triple::aarch64:
   case llvm::Triple::armeb:
   case llvm::Triple::thumb:
   case llvm::Triple::thumbeb:
      // BFD linker has issues wrt relocation of the protocol conformance
      // section on these targets, it also generates COPY relocations for
      // final executables, as such, unless specified, we default to gold
      // linker.
      return "gold";
   case llvm::Triple::x86:
   case llvm::Triple::x86_64:
   case llvm::Triple::ppc64:
   case llvm::Triple::ppc64le:
   case llvm::Triple::systemz:
      // BFD linker has issues wrt relocations against protected symbols.
      return "gold";
   default:
      // Otherwise, use the default BFD linker.
      return "";
   }
}

std::string GenericUnix::getTargetForLinker() const
{
   return getTriple().str();
}

bool GenericUnix::shouldProvideRPathToLinker() const
{
   return true;
}

ToolChain::InvocationInfo
GenericUnix::constructInvocation(const DynamicLinkJobAction &job,
                                 const JobContext &context) const
{
   assert(context.output.getPrimaryOutputType() == filetypes::TY_Image &&
          "Invalid linker output type.");
   ArgStringList arguments;
   switch (job.getKind()) {
   case LinkKind::None:
      llvm_unreachable("invalid link kind");
   case LinkKind::Executable:
      // Default case, nothing extra needed.
      break;
   case LinkKind::DynamicLibrary:
      arguments.push_back("-shared");
      break;
   case LinkKind::StaticLibrary:
      llvm_unreachable("the dynamic linker cannot build static libraries");
   }

   // Select the linker to use.
   std::string linker;
   if (const Arg *arg = context.args.getLastArg(options::OPT_use_ld)) {
      linker = arg->getValue();
   } else {
      linker = getDefaultLinker();
   }
   if (!linker.empty()) {
#if defined(__HAIKU__)
      // For now, passing -fuse-ld on Haiku doesn't work as swiftc doesn't
      // recognise it. Passing -use-ld= as the argument works fine.
      arguments.push_back(context.args.MakeArgString("-use-ld=" + linker));
#else
      arguments.push_back(context.args.MakeArgString("-fuse-ld=" + linker));
#endif
   }

   // Configure the toolchain.
   // By default, use the system clang++ to link.
   const char *clang = "clang++";
   if (const Arg *arg = context.args.getLastArg(options::OPT_tools_directory)) {
      StringRef toolchainPath(arg->getValue());

      // If there is a clang in the toolchain folder, use that instead.
      if (auto toolchainClang =
          llvm::sys::findProgramByName("clang++", {toolchainPath})) {
         clang = context.args.MakeArgString(toolchainClang.get());
      }

      // Look for binutils in the toolchain folder.
      arguments.push_back("-B");
      arguments.push_back(context.args.MakeArgString(arg->getValue()));
   }

   if (getTriple().getOS() == llvm::Triple::Linux &&
       job.getKind() == LinkKind::Executable) {
      arguments.push_back("-pie");
   }

   std::string target = getTargetForLinker();
   if (!target.empty()) {
      arguments.push_back("-target");
      arguments.push_back(context.args.MakeArgString(target));
   }

   bool staticExecutable = false;
   bool staticStdlib = false;

   if (context.args.hasFlag(options::OPT_static_executable,
                            options::OPT_no_static_executable, false)) {
      staticExecutable = true;
   } else if (context.args.hasFlag(options::OPT_static_stdlib,
                                   options::OPT_no_static_stdlib, false)) {
      staticStdlib = true;
   }

   SmallVector<std::string, 4> runtimeLibPaths;
   getRuntimeLibraryPaths(runtimeLibPaths, context.args, context.outputInfo.sdkPath,
                          /*Shared=*/!(staticExecutable || staticStdlib));

   if (!(staticExecutable || staticStdlib) && shouldProvideRPathToLinker()) {
      // FIXME: We probably shouldn't be adding an rpath here unless we know
      //        ahead of time the standard library won't be copied.
      for (auto path : runtimeLibPaths) {
         arguments.push_back("-Xlinker");
         arguments.push_back("-rpath");
         arguments.push_back("-Xlinker");
         arguments.push_back(context.args.MakeArgString(path));
      }
   }

   SmallString<128> sharedResourceDirPath;
   getResourceDirPath(sharedResourceDirPath, context.args, /*Shared=*/true);

   SmallString<128> polarphprtPath = sharedResourceDirPath;
   llvm::sys::path::append(polarphprtPath,
                           polar::basic::get_major_architecture_name(getTriple()));
   llvm::sys::path::append(polarphprtPath, "swiftrt.o");
   arguments.push_back(context.args.MakeArgString(polarphprtPath));

   addPrimaryInputsOfType(arguments, context.inputs, context.args,
                          filetypes::TY_Object);
   addInputsOfType(arguments, context.inputActions, filetypes::TY_Object);

   for (const Arg *arg :
        context.args.filtered(options::OPT_F, options::OPT_Fsystem)) {
      if (arg->getOption().matches(options::OPT_Fsystem)) {
         arguments.push_back("-iframework");
      } else {
         arguments.push_back(context.args.MakeArgString(arg->getSpelling()));
      }
      arguments.push_back(arg->getValue());
   }

   if (!context.outputInfo.sdkPath.empty()) {
      arguments.push_back("--sysroot");
      arguments.push_back(context.args.MakeArgString(context.outputInfo.sdkPath));
   }

   // Add any autolinking scripts to the arguments
   for (const Job *cmd : context.inputs) {
      auto &outputInfo = cmd->getOutput();
      if (outputInfo.getPrimaryOutputType() == filetypes::TY_AutolinkFile) {
         arguments.push_back(context.args.MakeArgString(
                                Twine("@") + outputInfo.getPrimaryOutputFilename()));
      }
   }

   // Add the runtime library link paths.
   for (auto path : runtimeLibPaths) {
      arguments.push_back("-L");
      arguments.push_back(context.args.MakeArgString(path));
   }

   // Link the standard library. In two paths, we do this using a .lnk file;
   // if we're going that route, we'll set `linkFilePath` to the path to that
   // file.
   SmallString<128> linkFilePath;
   getResourceDirPath(linkFilePath, context.args, /*Shared=*/false);

   if (staticExecutable) {
      llvm::sys::path::append(linkFilePath, "static-executable-args.lnk");
   } else if (staticStdlib) {
      llvm::sys::path::append(linkFilePath, "static-stdlib-args.lnk");
   } else {
      linkFilePath.clear();
      arguments.push_back("-lswiftCore");
   }

   if (!linkFilePath.empty()) {
      auto linkFile = linkFilePath.str();
      if (llvm::sys::fs::is_regular_file(linkFile)) {
         arguments.push_back(context.args.MakeArgString(Twine("@") + linkFile));
      } else {
         llvm::report_fatal_error(linkFile + " not found");
      }
   }

   // Explicitly pass the target to the linker
   arguments.push_back(
            context.args.MakeArgString("--target=" + getTriple().str()));

   // Delegate to clang for sanitizers. It will figure out the correct linker
   // options.
   if (job.getKind() == LinkKind::Executable && context.outputInfo.selectedSanitizers) {
      arguments.push_back(context.args.MakeArgString(
                             "-fsanitize=" + get_sanitizer_list(context.outputInfo.selectedSanitizers)));

      // The TSan runtime depends on the blocks runtime and libdispatch.
      if (context.outputInfo.selectedSanitizers & SanitizerKind::Thread) {
         arguments.push_back("-lBlocksRuntime");
         arguments.push_back("-ldispatch");
      }
   }

   if (context.args.hasArg(options::OPT_profile_generate)) {
      SmallString<128> LibProfile(sharedResourceDirPath);
      llvm::sys::path::remove_filename(LibProfile); // remove platform name
      llvm::sys::path::append(LibProfile, "clang", "lib");

      llvm::sys::path::append(LibProfile, getTriple().getOSName(),
                              Twine("libclang_rt.profile-") +
                              getTriple().getArchName() + ".a");
      arguments.push_back(context.args.MakeArgString(LibProfile));
      arguments.push_back(context.args.MakeArgString(
                             Twine("-u", llvm::getInstrProfRuntimeHookVarName())));
   }

   // Run clang++ in verbose mode if "-v" is set
   if (context.args.hasArg(options::OPT_v)) {
      arguments.push_back("-v");
   }

   // These custom arguments should be right before the object file at the end.
   context.args.AddAllArgs(arguments, options::OPT_linker_option_Group);
   context.args.AddAllArgs(arguments, options::OPT_Xlinker);
   context.args.AddAllArgValues(arguments, options::OPT_Xclang_linker);

   // This should be the last option, for convenience in checking output.
   arguments.push_back("-o");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));

   InvocationInfo invocationInfo{clang, arguments};
   invocationInfo.allowsResponseFiles = true;

   return invocationInfo;
}

ToolChain::InvocationInfo
GenericUnix::constructInvocation(const StaticLinkJobAction &job,
                                 const JobContext &context) const
{
   assert(context.output.getPrimaryOutputType() == filetypes::TY_Image &&
          "Invalid linker output type.");

   ArgStringList arguments;

   // Configure the toolchain.
   const char *AR = "ar";
   arguments.push_back("crs");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));
   addPrimaryInputsOfType(arguments, context.inputs, context.args,
                          filetypes::TY_Object);
   addInputsOfType(arguments, context.inputActions, filetypes::TY_Object);
   InvocationInfo invocationInfo{AR, arguments};
   return invocationInfo;
}

std::string Android::getTargetForLinker() const
{
   const llvm::Triple &T = getTriple();
   if (T.getArch() == llvm::Triple::arm &&
       T.getSubArch() == llvm::Triple::SubArchType::ARMSubArch_v7)
      // Explicitly set the linker target to "androideabi", as opposed to the
      // llvm::Triple representation of "armv7-none-linux-android".
      return "armv7-none-linux-androideabi";
   return T.str();
}

bool Android::shouldProvideRPathToLinker() const
{
   return false;
}

std::string Cygwin::getDefaultLinker() const
{
   // Cygwin uses the default BFD linker, even on ARM.
   return "";
}

std::string Cygwin::getTargetForLinker() const
{
   return "";
}

} // polar::driver::toolchains


