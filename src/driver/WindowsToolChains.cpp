//===---- WindowsToolChains.cpp - Job invocations (Windows-specific) ------===//
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
using namespace polar::basic;

std::string toolchains::Windows::sanitizerRuntimeLibName(StringRef sanitizer,
                                                         bool shared) const
{
   return (Twine("clang_rt.") + sanitizer + "-" +
           this->getTriple().getArchName() + ".lib")
         .str();
}

ToolChain::InvocationInfo
toolchains::Windows::constructInvocation(const DynamicLinkJobAction &job,
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
      llvm_unreachable("invalid link kind");
   }

   // Select the linker to use.
   std::string linker;
   if (const Arg *arg = context.args.getLastArg(options::OPT_use_ld)) {
      linker = arg->getValue();
   }
   if (!linker.empty())
      arguments.push_back(context.args.MakeArgString("-fuse-ld=" + linker));

   if (context.outputInfo.debugInfoFormat == IRGenDebugInfoFormat::CodeView)
      arguments.push_back("-Wl,/DEBUG");

   // Configure the toolchain.
   // By default, use the system clang++ to link.
   const char *clang = nullptr;
   if (const Arg *arg = context.args.getLastArg(options::OPT_tools_directory)) {
      StringRef toolchainPath(arg->getValue());

      // If there is a clang in the toolchain folder, use that instead.
      if (auto toolchainClang =
          llvm::sys::findProgramByName("clang++", {toolchainPath})) {
         clang = context.args.MakeArgString(toolchainClang.get());
      }
   }
   if (clang == nullptr) {
      if (auto pathClang = llvm::sys::findProgramByName("clang++", None)) {
         clang = context.args.MakeArgString(pathClang.get());
      }
   }
   assert(clang &&
          "clang++ was not found in the toolchain directory or system path.");

   std::string target = getTriple().str();
   if (!target.empty()) {
      arguments.push_back("-target");
      arguments.push_back(context.args.MakeArgString(target));
   }

   bool wantsStaticStdlib =
         context.args.hasFlag(options::OPT_static_stdlib,
                              options::OPT_no_static_stdlib, false);

   SmallVector<std::string, 4> runtimeLibPaths;
   getRuntimeLibraryPaths(runtimeLibPaths, context.args, context.outputInfo.sdkPath,
                          /*Shared=*/!wantsStaticStdlib);

   for (auto path : runtimeLibPaths) {
      arguments.push_back("-L");
      // Since Windows has separate libraries per architecture, link against the
      // architecture specific version of the static library.
      arguments.push_back(context.args.MakeArgString(path + "/" +
                                                     getTriple().getArchName()));
   }

   SmallString<128> sharedResourceDirPath;
   getResourceDirPath(sharedResourceDirPath, context.args, /*Shared=*/true);

   SmallString<128> polarphpRunTimePath = sharedResourceDirPath;
   llvm::sys::path::append(polarphpRunTimePath,
                           get_major_architecture_name(getTriple()));
   llvm::sys::path::append(polarphpRunTimePath, "swiftrt.obj");
   arguments.push_back(context.args.MakeArgString(polarphpRunTimePath));

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
      arguments.push_back("-I");
      arguments.push_back(context.args.MakeArgString(context.outputInfo.sdkPath));
   }

   if (job.getKind() == LinkKind::Executable) {
      if (context.outputInfo.selectedSanitizers & SanitizerKind::Address) {
         addLinkRuntimeLib(context.args, arguments,
                           sanitizerRuntimeLibName("asan"));
      }
      if (context.outputInfo.selectedSanitizers & SanitizerKind::Undefined) {
         addLinkRuntimeLib(context.args, arguments,
                           sanitizerRuntimeLibName("ubsan"));
      }
   }

   if (context.args.hasArg(options::OPT_profile_generate)) {
      SmallString<128> libProfile(sharedResourceDirPath);
      llvm::sys::path::remove_filename(libProfile); // remove platform name
      llvm::sys::path::append(libProfile, "clang", "lib");

      llvm::sys::path::append(libProfile, getTriple().getOSName(),
                              Twine("clang_rt.profile-") +
                              getTriple().getArchName() + ".lib");
      arguments.push_back(context.args.MakeArgString(libProfile));
      arguments.push_back(context.args.MakeArgString(
                             Twine("-u", llvm::getInstrProfRuntimeHookVarName())));
   }

   context.args.AddAllArgs(arguments, options::OPT_Xlinker);
   context.args.AddAllArgs(arguments, options::OPT_linker_option_Group);
   context.args.AddAllArgValues(arguments, options::OPT_Xclang_linker);

   // Run clang++ in verbose mode if "-v" is set
   if (context.args.hasArg(options::OPT_v)) {
      arguments.push_back("-v");
   }

   // This should be the last option, for convenience in checking output.
   arguments.push_back("-o");
   arguments.push_back(
            context.args.MakeArgString(context.output.getPrimaryOutputFilename()));

   InvocationInfo invocationInfo{clang, arguments};
   invocationInfo.allowsResponseFiles = true;
   return invocationInfo;
}

ToolChain::InvocationInfo
toolchains::Windows::constructInvocation(const StaticLinkJobAction &job,
                                         const JobContext &context) const
{
   assert(context.output.getPrimaryOutputType() == filetypes::TY_Image &&
          "Invalid linker output type.");

   ArgStringList arguments;

   // Configure the toolchain.
   const char *link = "link";
   arguments.push_back("-lib");

   addPrimaryInputsOfType(arguments, context.inputs, context.args,
                          filetypes::TY_Object);
   addInputsOfType(arguments, context.inputActions, filetypes::TY_Object);

   arguments.push_back(
            context.args.MakeArgString(Twine("/OUT:") +
                                       context.output.getPrimaryOutputFilename()));

   InvocationInfo invocationInfo{link, arguments};
   invocationInfo.allowsResponseFiles = true;

   return invocationInfo;
}

} //  polar::driver::toolchains
