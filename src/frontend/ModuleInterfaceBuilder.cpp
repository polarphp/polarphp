//===----- ModuleInterfaceBuilder.cpp - Compiles .polarinterface files ----===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2019 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polar.org/LICENSE.txt for license information
// See https://polar.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "textual-module-interface"

#include "polarphp/frontend/internal/ModuleInterfaceBuilder.h"
#include "polarphp/ast/AstContext.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "polarphp/ast/DiagnosticsSema.h"
#include "polarphp/ast/FileSystem.h"
#include "polarphp/ast/Module.h"
#include "polarphp/basic/Defer.h"
#include "polarphp/frontend/Frontend.h"
#include "polarphp/frontend/ModuleInterfaceSupport.h"
#include "polarphp/pil/optimizer/passmgr/Passes.h"
#include "polarphp/serialization/SerializationOptions.h"

#include "llvm/ADT/StringSet.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/xxhash.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/CrashRecoveryContext.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/Regex.h"
#include "llvm/Support/StringSaver.h"

using namespace polar;
using FileDependency = SerializationOptions::FileDependency;
namespace path = llvm::sys::path;

/// If the file dependency in \p FullDepPath is inside the \p Base directory,
/// this returns its path relative to \p Base. Otherwise it returns None.
static Optional<StringRef> getRelativeDepPath(StringRef DepPath,
                                              StringRef Base) {
   // If Base is the root directory, or DepPath does not start with Base, bail.
   if (Base.size() <= 1 || !DepPath.startswith(Base)) {
      return None;
   }

   assert(DepPath.size() > Base.size() &&
          "should never depend on a directory");

   // Is the DepName something like ${Base}/foo.h"?
   if (path::is_separator(DepPath[Base.size()]))
      return DepPath.substr(Base.size() + 1);

   // Is the DepName something like "${Base}foo.h", where Base
   // itself contains a trailing slash?
   if (path::is_separator(Base.back()))
      return DepPath.substr(Base.size());

   // We have something next to Base, like "Base.h", that's somehow
   // become a dependency.
   return None;
}

void ModuleInterfaceBuilder::configureSubInvocationInputsAndOutputs(
   StringRef OutPath) {
   auto &SubFEOpts = subInvocation.getFrontendOptions();
   SubFEOpts.RequestedAction = FrontendOptions::ActionType::EmitModuleOnly;
   SubFEOpts.InputsAndOutputs.addPrimaryInputFile(interfacePath);
   SupplementaryOutputPaths SOPs;
   SOPs.ModuleOutputPath = OutPath.str();

   // Pick a primary output path that will cause problems to use.
   StringRef MainOut = "/<unused>";
   SubFEOpts.InputsAndOutputs
      .setMainAndSupplementaryOutputs({MainOut}, {SOPs});
}

void ModuleInterfaceBuilder::configureSubInvocation(
   const SearchPathOptions &SearchPathOpts,
   const LangOptions &LangOpts,
   ClangModuleLoader *ClangLoader) {
   // Start with a SubInvocation that copies various state from our
   // invoking ASTContext.
   subInvocation.setImportSearchPaths(SearchPathOpts.ImportSearchPaths);
   subInvocation.setFrameworkSearchPaths(SearchPathOpts.FrameworkSearchPaths);
   subInvocation.setSDKPath(SearchPathOpts.SDKPath);
   subInvocation.setInputKind(InputFileKind::PHPModuleInterface);
   subInvocation.setRuntimeResourcePath(SearchPathOpts.RuntimeResourcePath);
   subInvocation.setTargetTriple(LangOpts.Target);

   subInvocation.setModuleName(moduleName);
   subInvocation.setClangModuleCachePath(moduleCachePath);
   subInvocation.getFrontendOptions().PrebuiltModuleCachePath =
      prebuiltCachePath;
   subInvocation.getFrontendOptions().TrackSystemDeps = trackSystemDependencies;

   // Respect the detailed-record preprocessor setting of the parent context.
   // This, and the "raw" clang module format it implicitly enables, are
   // required by sourcekitd.
   if (ClangLoader) {
      auto &Opts = ClangLoader->getClangInstance().getPreprocessorOpts();
      if (Opts.DetailedRecord) {
         subInvocation.getClangImporterOptions().DetailedPreprocessingRecord = true;
      }
   }

   // Inhibit warnings from the SubInvocation since we are assuming the user
   // is not in a position to fix them.
   subInvocation.getDiagnosticOptions().SuppressWarnings = true;

   // Inherit this setting down so that it can affect error diagnostics (mostly
   // by making them non-fatal).
   subInvocation.getLangOptions().DebuggerSupport = LangOpts.DebuggerSupport;

   // Disable this; deinitializers always get printed with `@objc` even in
   // modules that don't import Foundation.
   subInvocation.getLangOptions().EnableObjCAttrRequiresFoundation = false;

   // Tell the subinvocation to serialize dependency hashes if asked to do so.
   auto &frontendOpts = subInvocation.getFrontendOptions();
   frontendOpts.SerializeModuleInterfaceDependencyHashes =
      serializeDependencyHashes;

   // Tell the subinvocation to remark on rebuilds from an interface if asked
   // to do so.
   frontendOpts.RemarkOnRebuildFromModuleInterface =
      remarkOnRebuildFromInterface;
}

bool ModuleInterfaceBuilder::extractSwiftInterfaceVersionAndArgs(
   polar::version::Version &Vers, llvm::StringSaver &SubArgSaver,
   SmallVectorImpl<const char *> &SubArgs) {
   auto FileOrError = polar::vfs::get_file_or_stdin(fs, interfacePath);
   if (!FileOrError) {
      diags.diagnose(diagnosticLoc, diag::error_open_input_file,
                     interfacePath, FileOrError.getError().message());
      return true;
   }
   auto SB = FileOrError.get()->getBuffer();
   auto VersRe = getPHPInterfaceFormatVersionRegex();
   auto FlagRe = getPHPInterfaceModuleFlagsRegex();
   SmallVector<StringRef, 1> VersMatches, FlagMatches;
   if (!VersRe.match(SB, &VersMatches)) {
      diags.diagnose(diagnosticLoc,
                     diag::error_extracting_version_from_module_interface);
      return true;
   }
   if (!FlagRe.match(SB, &FlagMatches)) {
      diags.diagnose(diagnosticLoc,
                     diag::error_extracting_flags_from_module_interface);
      return true;
   }
   assert(VersMatches.size() == 2);
   assert(FlagMatches.size() == 2);
   Vers = polar::version::Version(VersMatches[1], SourceLoc(), &diags);
   llvm::cl::TokenizeGNUCommandLine(FlagMatches[1], SubArgSaver, SubArgs);
   return false;
}

bool ModuleInterfaceBuilder::collectDepsForSerialization(
   CompilerInstance &SubInstance, SmallVectorImpl<FileDependency> &Deps,
   bool IsHashBased) {
   auto &Opts = SubInstance.getAstContext().SearchPathOpts;
   SmallString<128> SDKPath(Opts.SDKPath);
   path::native(SDKPath);
   SmallString<128> ResourcePath(Opts.RuntimeResourcePath);
   path::native(ResourcePath);

   auto DTDeps = SubInstance.getDependencyTracker()->getDependencies();
   SmallVector<StringRef, 16> InitialDepNames(DTDeps.begin(), DTDeps.end());
   InitialDepNames.push_back(interfacePath);
   InitialDepNames.insert(InitialDepNames.end(),
                          extraDependencies.begin(), extraDependencies.end());
   llvm::StringSet<> AllDepNames;
   SmallString<128> Scratch;

   for (const auto &InitialDepName : InitialDepNames) {
      path::native(InitialDepName, Scratch);
      StringRef DepName = Scratch.str();

      assert(moduleCachePath.empty() || !DepName.startswith(moduleCachePath));

      // Serialize the paths of dependencies in the SDK relative to it.
      Optional<StringRef> SDKRelativePath = getRelativeDepPath(DepName, SDKPath);
      StringRef DepNameToStore = SDKRelativePath.getValueOr(DepName);
      bool IsSDKRelative = SDKRelativePath.hasValue();

      // Forwarding modules add the underlying prebuilt module to their
      // dependency list -- don't serialize that.
      if (!prebuiltCachePath.empty() && DepName.startswith(prebuiltCachePath))
         continue;

      if (AllDepNames.insert(DepName).second && dependencyTracker) {
         dependencyTracker->addDependency(DepName, /*isSystem*/IsSDKRelative);
      }

      // Don't serialize compiler-relative deps so the cache is relocatable.
      if (DepName.startswith(ResourcePath))
         continue;

      auto Status = fs.status(DepName);
      if (!Status)
         return true;

      /// Lazily load the dependency buffer if we need it. If we're not
      /// dealing with a hash-based dependencies, and if the dependency is
      /// not a .polarmodule, we can avoid opening the buffer.
      std::unique_ptr<llvm::MemoryBuffer> DepBuf = nullptr;
      auto getDepBuf = [&]() -> llvm::MemoryBuffer * {
         if (DepBuf) return DepBuf.get();
         if (auto Buf = fs.getBufferForFile(DepName, /*FileSize=*/-1,
            /*RequiresNullTerminator=*/false)) {
            DepBuf = std::move(Buf.get());
            return DepBuf.get();
         }
         return nullptr;
      };

      if (IsHashBased) {
         auto buf = getDepBuf();
         if (!buf) return true;
         uint64_t hash = xxHash64(buf->getBuffer());
         Deps.push_back(
            FileDependency::hashBased(DepNameToStore, IsSDKRelative,
                                      Status->getSize(), hash));
      } else {
         uint64_t mtime =
            Status->getLastModificationTime().time_since_epoch().count();
         Deps.push_back(
            FileDependency::modTimeBased(DepNameToStore, IsSDKRelative,
                                         Status->getSize(), mtime));
      }
   }
   return false;
}

bool ModuleInterfaceBuilder::buildPHPModule(
   StringRef OutPath, bool ShouldSerializeDeps,
   std::unique_ptr<llvm::MemoryBuffer> *ModuleBuffer) {
   bool SubError = false;
   bool RunSuccess = llvm::CrashRecoveryContext().RunSafelyOnThread([&] {
      // Note that we don't assume cachePath is the same as the Clang
      // module cache path at this point.
      if (!moduleCachePath.empty())
         (void)llvm::sys::fs::create_directories(moduleCachePath);

      configureSubInvocationInputsAndOutputs(OutPath);

      FrontendOptions &FEOpts = subInvocation.getFrontendOptions();
      const auto &InputInfo = FEOpts.InputsAndOutputs.firstInput();
      StringRef InPath = InputInfo.file();
      const auto &OutputInfo =
         InputInfo.getPrimarySpecificPaths().supplementaryOutputs;
      StringRef OutPath = OutputInfo.ModuleOutputPath;

      llvm::BumpPtrAllocator SubArgsAlloc;
      llvm::StringSaver SubArgSaver(SubArgsAlloc);
      SmallVector<const char *, 16> SubArgs;
      polar::version::Version Vers;
      if (extractSwiftInterfaceVersionAndArgs(Vers, SubArgSaver, SubArgs)) {
         SubError = true;
         return;
      }

      // For now: we support anything with the same "major version" and assume
      // minor versions might be interesting for debugging, or special-casing a
      // compatible field variant.
      if (Vers.asMajorVersion() != InterfaceFormatVersion.asMajorVersion()) {
         diags.diagnose(diagnosticLoc,
                        diag::unsupported_version_of_module_interface,
                        interfacePath, Vers);
         SubError = true;
         return;
      }

      SmallString<32> ExpectedModuleName = subInvocation.getModuleName();
      if (subInvocation.parseArgs(SubArgs, diags)) {
         SubError = true;
         return;
      }

      if (subInvocation.getModuleName() != ExpectedModuleName) {
         auto DiagKind = diag::serialization_name_mismatch;
         if (subInvocation.getLangOptions().DebuggerSupport)
            DiagKind = diag::serialization_name_mismatch_repl;
         diags.diagnose(diagnosticLoc, DiagKind, subInvocation.getModuleName(),
                        ExpectedModuleName);
         SubError = true;
         return;
      }

      // Build the .polarmodule; this is a _very_ abridged version of the logic
      // in performCompile in libFrontendTool, specialized, to just the one
      // module-serialization task we're trying to do here.
      LLVM_DEBUG(llvm::dbgs() << "Setting up instance to compile "
                              << InPath << " to " << OutPath << "\n");
      CompilerInstance SubInstance;
      SubInstance.getSourceMgr().setFileSystem(&fs);

      ForwardingDiagnosticConsumer FDC(diags);
      SubInstance.addDiagnosticConsumer(&FDC);

      SubInstance.createDependencyTracker(FEOpts.TrackSystemDeps);

      POLAR_DEFER {
         // Make sure to emit a generic top-level error if a module fails to
         // load. This is not only good for users; it also makes sure that we've
         // emitted an error in the parent diagnostic engine, which is what
         // determines whether the process exits with a proper failure status.
         if (SubInstance.getAstContext().hadError()) {
            diags.diagnose(diagnosticLoc, diag::serialization_load_failed,
                           moduleName);
         }
      };

      if (SubInstance.setup(subInvocation)) {
         SubError = true;
         return;
      }

      LLVM_DEBUG(llvm::dbgs() << "Performing sema\n");
      SubInstance.performSema();
      if (SubInstance.getAstContext().hadError()) {
         LLVM_DEBUG(llvm::dbgs() << "encountered errors\n");
         SubError = true;
         return;
      }

      PILOptions &PILOpts = subInvocation.getPILOptions();
      auto Mod = SubInstance.getMainModule();
      auto &TC = SubInstance.getPILTypes();
      auto PILMod = performPILGeneration(Mod, TC, PILOpts);
      if (!PILMod) {
         LLVM_DEBUG(llvm::dbgs() << "PILGen did not produce a module\n");
         SubError = true;
         return;
      }

      // Setup the callbacks for serialization, which can occur during the
      // optimization pipeline.
      SerializationOptions SerializationOpts;
      std::string OutPathStr = OutPath;
      SerializationOpts.OutputPath = OutPathStr.c_str();
      SerializationOpts.ModuleLinkName = FEOpts.ModuleLinkName;

      // Record any non-SDK module interface files for the debug info.
      StringRef SDKPath = SubInstance.getAstContext().SearchPathOpts.SDKPath;
      if (!getRelativeDepPath(InPath, SDKPath))
         SerializationOpts.ModuleInterface = InPath;

      SmallVector<FileDependency, 16> Deps;
      bool serializeHashes = FEOpts.SerializeModuleInterfaceDependencyHashes;
      if (collectDepsForSerialization(SubInstance, Deps, serializeHashes)) {
         SubError = true;
         return;
      }
      if (ShouldSerializeDeps)
         SerializationOpts.Dependencies = Deps;
      PILMod->setSerializePILAction([&]() {
         // We don't want to serialize module docs in the cache -- they
         // will be serialized beside the interface file.
         /// TODO
//         serializeToBuffers(Mod, SerializationOpts, ModuleBuffer,
//            /*ModuleDocBuffer*/nullptr,
//            /*SourceInfoBuffer*/nullptr,
//                            PILMod.get());
      });

      LLVM_DEBUG(llvm::dbgs() << "Running PIL processing passes\n");
      if (SubInstance.performPILProcessing(PILMod.get())) {
         LLVM_DEBUG(llvm::dbgs() << "encountered errors\n");
         SubError = true;
         return;
      }

      SubError = SubInstance.getDiags().hadAnyError();
   });
   return !RunSuccess || SubError;
}
