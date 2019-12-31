//===--- Immediate.cpp - the swift immediate mode -------------------------===//
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
//
// This is the implementation of the swift interpreter, which takes a
// source file and JITs it.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "polarphp-immediate"

#include "polarphp/immediate/Immediate.h"
#include "polarphp/immediate/internal/ImmediateImpl.h"

#include "polarphp/global/Subsystems.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/ast/IRGenOptions.h"
#include "polarphp/ast/Module.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/basic/LLVMContext.h"
#include "polarphp/frontend/Frontend.h"
#include "polarphp/irgen/IRGenPublic.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/ExecutionEngine/MCJIT.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/Linker/Linker.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/Path.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <dlfcn.h>
#endif

using namespace polar;
using namespace polar::immediate;

static void *loadRuntimeLib(StringRef runtimeLibPathWithName) {
#if defined(_WIN32)
   return LoadLibraryA(runtimeLibPathWithName.str().c_str());
#else
   return dlopen(runtimeLibPathWithName.str().c_str(), RTLD_LAZY | RTLD_GLOBAL);
#endif
}

static void *loadRuntimeLibAtPath(StringRef sharedLibName,
                                  StringRef runtimeLibPath) {
   // FIXME: Need error-checking.
   llvm::SmallString<128> Path = runtimeLibPath;
   llvm::sys::path::append(Path, sharedLibName);
   return loadRuntimeLib(Path);
}

static void *loadRuntimeLib(StringRef sharedLibName,
                            ArrayRef<std::string> runtimeLibPaths) {
   for (auto &runtimeLibPath : runtimeLibPaths) {
      if (void *handle = loadRuntimeLibAtPath(sharedLibName, runtimeLibPath))
         return handle;
   }
   return nullptr;
}

void *polar::immediate::loadPHPRuntime(ArrayRef<std::string>
                                       runtimeLibPaths) {
#if defined(_WIN32)
   return loadRuntimeLib("swiftCore" LTDL_SHLIB_EXT, runtimeLibPaths);
#else
   // @todo
   return loadRuntimeLib("libPHPCore"/* LTDL_SHLIB_EXT*/, runtimeLibPaths);
#endif
}

static bool tryLoadLibrary(LinkLibrary linkLib,
                           SearchPathOptions searchPathOpts) {
   llvm::SmallString<128> path = linkLib.getName();

   // If we have an absolute or relative path, just try to load it now.
   if (llvm::sys::path::has_parent_path(path.str())) {
      return loadRuntimeLib(path);
   }

   bool success = false;
   switch (linkLib.getKind()) {
      case LibraryKind::Library: {
         llvm::SmallString<32> stem;
         if (llvm::sys::path::has_extension(path.str())) {
            stem = std::move(path);
         } else {
            // FIXME: Try the appropriate extension for the current platform?
            stem = "lib";
            stem += path;
            // @todo
            /*stem += LTDL_SHLIB_EXT;*/
         }

         // Try user-provided library search paths first.
         for (auto &libDir : searchPathOpts.LibrarySearchPaths) {
            path = libDir;
            llvm::sys::path::append(path, stem.str());
            success = loadRuntimeLib(path);
            if (success)
               break;
         }

         // Let loadRuntimeLib determine the best search paths.
         if (!success)
            success = loadRuntimeLib(stem);

         // If that fails, try our runtime library paths.
         if (!success)
            success = loadRuntimeLib(stem, searchPathOpts.RuntimeLibraryPaths);
         break;
      }
      case LibraryKind::Framework: {
         // If we have a framework, mangle the name to point to the framework
         // binary.
         llvm::SmallString<64> frameworkPart{std::move(path)};
         frameworkPart += ".framework";
         llvm::sys::path::append(frameworkPart, linkLib.getName());

         // Try user-provided framework search paths first; frameworks contain
         // binaries as well as modules.
         for (auto &frameworkDir : searchPathOpts.FrameworkSearchPaths) {
            path = frameworkDir.Path;
            llvm::sys::path::append(path, frameworkPart.str());
            success = loadRuntimeLib(path);
            if (success)
               break;
         }

         // If that fails, let loadRuntimeLib search for system frameworks.
         if (!success)
            success = loadRuntimeLib(frameworkPart);
         break;
      }
   }

   return success;
}

bool polar::immediate::tryLoadLibraries(ArrayRef<LinkLibrary> LinkLibraries,
                                        SearchPathOptions SearchPathOpts,
                                        DiagnosticEngine &Diags) {
   SmallVector<bool, 4> LoadedLibraries;
   LoadedLibraries.append(LinkLibraries.size(), false);

   // Libraries are not sorted in the topological order of dependencies, and we
   // don't know the dependencies in advance.  Try to load all libraries until
   // we stop making progress.
   bool HadProgress;
   do {
      HadProgress = false;
      for (unsigned i = 0; i != LinkLibraries.size(); ++i) {
         if (!LoadedLibraries[i] &&
             tryLoadLibrary(LinkLibraries[i], SearchPathOpts)) {
            LoadedLibraries[i] = true;
            HadProgress = true;
         }
      }
   } while (HadProgress);

   return std::all_of(LoadedLibraries.begin(), LoadedLibraries.end(),
                      [](bool Value) { return Value; });
}

static void linkerDiagnosticHandlerNoCtx(const llvm::DiagnosticInfo &DI) {
   if (DI.getSeverity() != llvm::DS_Error)
      return;

   std::string MsgStorage;
   {
      llvm::raw_string_ostream Stream(MsgStorage);
      llvm::DiagnosticPrinterRawOStream DP(Stream);
      DI.print(DP);
   }
   llvm::errs() << "Error linking swift modules\n";
   llvm::errs() << MsgStorage << "\n";
}



static void linkerDiagnosticHandler(const llvm::DiagnosticInfo &DI,
                                    void *Context) {
   // This assert self documents our precondition that Context is always
   // nullptr. It seems that parts of LLVM are using the flexibility of having a
   // context. We don't really care about this.
   assert(Context == nullptr && "We assume Context is always a nullptr");

   return linkerDiagnosticHandlerNoCtx(DI);
}

bool polar::immediate::linkLLVMModules(llvm::Module *Module,
                                       std::unique_ptr<llvm::Module> SubModule
   // TODO: reactivate the linker mode if it is
   // supported in llvm again. Otherwise remove the
   // commented code completely.
   /*, llvm::Linker::LinkerMode LinkerMode */)
{
   llvm::LLVMContext &Ctx = SubModule->getContext();
   auto OldHandler = Ctx.getDiagnosticHandlerCallBack();
   void *OldDiagnosticContext = Ctx.getDiagnosticContext();
   Ctx.setDiagnosticHandlerCallBack(linkerDiagnosticHandler, nullptr);
   bool Failed = llvm::Linker::linkModules(*Module, std::move(SubModule));
   Ctx.setDiagnosticHandlerCallBack(OldHandler, OldDiagnosticContext);
   return !Failed;
}

bool polar::immediate::autolinkImportedModules(ModuleDecl *M,
                                               IRGenOptions &IRGenOpts) {
   // Perform autolinking.
   SmallVector<LinkLibrary, 4> AllLinkLibraries(IRGenOpts.LinkLibraries);
   auto addLinkLibrary = [&](LinkLibrary linkLib) {
      AllLinkLibraries.push_back(linkLib);
   };

   M->collectLinkLibraries(addLinkLibrary);

   tryLoadLibraries(AllLinkLibraries, M->getAstContext().SearchPathOpts,
                    M->getAstContext().Diags);
   return false;
}

int polar::RunImmediately(CompilerInstance &CI, const ProcessCmdLine &CmdLine,
                          IRGenOptions &IRGenOpts, const PILOptions &PILOpts) {
   AstContext &Context = CI.getAstContext();

   // IRGen the main module.
   auto *phpModule = CI.getMainModule();
   const auto PSPs = CI.getPrimarySpecificPathsForAtMostOnePrimary();
   // FIXME: We shouldn't need to use the global context here, but
   // something is persisting across calls to performIRGeneration.
   auto ModuleOwner = performIRGeneration(
      IRGenOpts, phpModule, CI.takePILModule(), phpModule->getName().str(),
      PSPs, get_global_llvm_context(), ArrayRef<std::string>());
   auto *Module = ModuleOwner.get();

   if (Context.hadError())
      return -1;

   // Load libPHPCore to setup process arguments.
   //
   // This must be done here, before any library loading has been done, to avoid
   // racing with the static initializers in user code.
   auto stdlib = loadPHPRuntime(Context.SearchPathOpts.RuntimeLibraryPaths);
   if (!stdlib) {
      CI.getDiags().diagnose(SourceLoc(),
                             diag::error_immediate_mode_missing_stdlib);
      return -1;
   }

   // Setup interpreted process arguments.
   using ArgOverride = void (*)(const char **, int);
#if defined(_WIN32)
   auto module = static_cast<HMODULE>(stdlib);
  auto emplaceProcessArgs = reinterpret_cast<ArgOverride>(
    GetProcAddress(module, "_swift_stdlib_overrideUnsafeArgvArgc"));
  if (emplaceProcessArgs == nullptr)
    return -1;
#else
   auto emplaceProcessArgs
      = (ArgOverride)dlsym(stdlib, "_swift_stdlib_overrideUnsafeArgvArgc");
   if (dlerror())
      return -1;
#endif

   SmallVector<const char *, 32> argBuf;
   for (size_t i = 0; i < CmdLine.size(); ++i) {
      argBuf.push_back(CmdLine[i].c_str());
   }
   argBuf.push_back(nullptr);

   (*emplaceProcessArgs)(argBuf.data(), CmdLine.size());

   SmallVector<llvm::Function*, 8> InitFns;
   if (autolinkImportedModules(phpModule, IRGenOpts))
      return -1;

   llvm::PassManagerBuilder PMBuilder;
   PMBuilder.OptLevel = 2;
   PMBuilder.Inliner = llvm::createFunctionInliningPass(200);

   // Build the ExecutionEngine.
   llvm::EngineBuilder builder(std::move(ModuleOwner));
   std::string ErrorMsg;
   llvm::TargetOptions TargetOpt;
   std::string CPU;
   std::string Triple;
   std::vector<std::string> Features;
   std::tie(TargetOpt, CPU, Features, Triple)
      = getIRTargetOptions(IRGenOpts, phpModule->getAstContext());
   builder.setRelocationModel(llvm::Reloc::PIC_);
   builder.setTargetOptions(TargetOpt);
   builder.setMCPU(CPU);
   builder.setMAttrs(Features);
   builder.setErrorStr(&ErrorMsg);
   builder.setEngineKind(llvm::EngineKind::JIT);
   llvm::ExecutionEngine *EE = builder.create();
   if (!EE) {
      llvm::errs() << "Error loading JIT: " << ErrorMsg;
      return -1;
   }

   LLVM_DEBUG(llvm::dbgs() << "Module to be executed:\n";
                 Module->dump());

   EE->finalizeObject();

   // Run the generated program.
   for (auto InitFn : InitFns) {
      LLVM_DEBUG(llvm::dbgs() << "Running initialization function "
                              << InitFn->getName() << '\n');
      EE->runFunctionAsMain(InitFn, CmdLine, nullptr);
   }

   LLVM_DEBUG(llvm::dbgs() << "Running static constructors\n");
   EE->runStaticConstructorsDestructors(false);
   LLVM_DEBUG(llvm::dbgs() << "Running main\n");
   llvm::Function *EntryFn = Module->getFunction("main");
   return EE->runFunctionAsMain(EntryFn, CmdLine, nullptr);
}
