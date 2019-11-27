//===--- IRGenOptions.h - Swift Language IR Generation Options --*- C++ -*-===//
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
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/11/27.

#ifndef POLARPHP_AST_IRGEN_OPTIONS_H
#define POLARPHP_AST_IRGEN_OPTIONS_H

#include "polarphp/ast/LinkLibrary.h"
#include "polarphp/basic/PathRemapper.h"
#include "polarphp/basic/Sanitizers.h"
#include "polarphp/basic/OptionSet.h"
#include "polarphp/basic/OptimizationMode.h"
// FIXME: This include is just for llvm::SanitizerCoverageOptions. We should
// split the header upstream so we don't include so much.
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Support/VersionTuple.h"
#include <string>
#include <vector>

namespace polar::ast {

using polar::basic::OptimizationMode;
using polar::basic::OptionSet;
using polar::basic::SanitizerKind;
using polar::basic::PathRemapper;

enum class IRGenOutputKind : unsigned
{
   /// Just generate an LLVM module and return it.
   Module,

   /// Generate an LLVM module and write it out as LLVM assembly.
   LLVMAssembly,

   /// Generate an LLVM module and write it out as LLVM bitcode.
   LLVMBitcode,

   /// Generate an LLVM module and compile it to assembly.
   NativeAssembly,

   /// Generate an LLVM module, compile it, and assemble into an object file.
   ObjectFile
};

enum class IRGenDebugInfoLevel : unsigned
{
   None,       ///< No debug info.
   LineTables, ///< Line tables only.
   ASTTypes,   ///< Line tables + AST type references.
   DwarfTypes, ///< Line tables + AST type references + DWARF types.
   Normal = ASTTypes ///< The setting LLDB prefers.
};

enum class IRGenDebugInfoFormat : unsigned
{
   None,
   DWARF,
   CodeView
};

enum class IRGenEmbedMode : unsigned
{
   None,
   EmbedMarker,
   EmbedBitcode
};

/// The set of options supported by IR generation.
class IRGenOptions
{
public:
   std::string moduleName;

   /// The compilation directory for the debug info.
   std::string debugCompilationDir;

   /// The DWARF version of debug info.
   unsigned DWARFVersion;

   /// The command line string that is to be stored in the debug info.
   std::string debugFlags;

   /// List of -Xcc -D macro definitions.
   std::vector<std::string> clangDefines;

   /// The libraries and frameworks specified on the command line.
   SmallVector<LinkLibrary, 4> linkLibraries;

   /// If non-empty, the (unmangled) name of a dummy symbol to emit that can be
   /// used to force-load this module.
   std::string forceLoadSymbolName;

   /// The kind of compilation we should do.
   IRGenOutputKind outputKind : 3;

   /// Should we spend time verifying that the IR we produce is
   /// well-formed?
   unsigned verify : 1;

   OptimizationMode optMode;

   /// Which sanitizer is turned on.
   OptionSet<SanitizerKind> sanitizers;

   /// Path prefixes that should be rewritten in debug info.
   PathRemapper debugPrefixMap;

   /// What level of debug info to generate.
   IRGenDebugInfoLevel debugInfoLevel : 2;

   /// What type of debug info to generate.
   IRGenDebugInfoFormat debugInfoFormat : 2;

   /// Whether to leave DWARF breadcrumbs pointing to imported Clang modules.
   unsigned disableClangModuleSkeletonCUs : 1;

   /// Whether we're generating IR for the JIT.
   unsigned useJIT : 1;

   /// Whether we're generating code for the integrated REPL.
   unsigned integratedREPL : 1;

   /// Whether we should run LLVM optimizations after IRGen.
   unsigned disableLLVMOptzns : 1;

   /// Whether we should run swift specific LLVM optimizations after IRGen.
   unsigned disablePolarphpSpecificLLVMOptzns : 1;

   /// Whether we should run LLVM SLP vectorizer.
   unsigned disableLLVMSLPVectorizer : 1;

   /// Disable frame pointer elimination?
   unsigned disableFPElim : 1;

   /// Special codegen for playgrounds.
   unsigned playground : 1;

   /// Emit runtime calls to check the end of the lifetime of stack promoted
   /// objects.
   unsigned emitStackPromotionChecks : 1;

   /// The maximum number of bytes used on a stack frame for stack promotion
   /// (includes alloc_stack allocations).
   unsigned stackPromotionSizeLimit = 1024;

   /// Emit code to verify that static and runtime type layout are consistent for
   /// the given type names.
   SmallVector<StringRef, 1> verifyTypeLayoutNames;

   /// Frameworks that we should not autolink against.
   SmallVector<std::string, 1> disableAutolinkFrameworks;

   /// Print the LLVM inline tree at the end of the LLVM pass pipeline.
   unsigned printInlineTree : 1;

   /// Whether we should embed the bitcode file.
   IRGenEmbedMode embedMode : 2;

   /// Add names to LLVM values.
   unsigned hasValueNamesSetting : 1;
   unsigned valueNames : 1;

   /// Emit nominal type field metadata.
   unsigned enableReflectionMetadata : 1;

   /// Emit names of struct stored properties and enum cases.
   unsigned enableReflectionNames : 1;

   /// Emit mangled names of anonymous context descriptors.
   unsigned enableAnonymousContextMangledNames : 1;

   /// Force public linkage for private symbols. Used only by the LLDB
   /// expression evaluator.
   unsigned forcePublicLinkage : 1;

   /// Force lazy initialization of class metadata
   /// Used on Windows to avoid cross-module references.
   unsigned lazyInitializeClassMetadata : 1;
   unsigned lazyInitializeProtocolConformances : 1;

   /// Normally if the -read-legacy-type-info flag is not specified, we look for
   /// a file named "legacy-<arch>.yaml" in SearchPathOpts.RuntimeLibraryPath.
   /// Passing this flag completely disables this behavior.
   unsigned disableLegacyTypeInfo : 1;

   /// The path to load legacy type layouts from.
   StringRef readLegacyTypeInfoPath;

   /// Should we try to build incrementally by not emitting an object file if it
   /// has the same IR hash as the module that we are preparing to emit?
   ///
   /// This is a debugging option meant to make it easier to perform compile time
   /// measurements on a non-clean build directory.
   unsigned useIncrementalLLVMCodeGen : 1;

   /// Enable use of the polarphpcall calling convention.
   unsigned usePolarphpCall : 1;

   /// Instrument code to generate profiling information.
   unsigned generateProfile : 1;

   /// Enable chaining of dynamic replacements.
   unsigned enableDynamicReplacementChaining : 1;

   /// Disable round-trip verification of mangled debug types.
   unsigned disableRoundTripDebugTypes : 1;

   /// Path to the profdata file to be used for PGO, or the empty string.
   std::string useProfile = "";

   /// List of backend command-line options for -embed-bitcode.
   std::vector<uint8_t> cmdArgs;

   /// Which sanitizer coverage is turned on.
   llvm::SanitizerCoverageOptions sanitizeCoverage;

   /// The different modes for dumping IRGen type info.
   enum class TypeInfoDumpFilter
   {
      All,
      Resilient,
      Fragile
   };

   TypeInfoDumpFilter typeInfoFilter;

   /// Pull in runtime compatibility shim libraries by autolinking.
   Optional<llvm::VersionTuple> autolinkRuntimeCompatibilityLibraryVersion;
   Optional<llvm::VersionTuple> autolinkRuntimeCompatibilityDynamicReplacementLibraryVersion;

   IRGenOptions()
      : DWARFVersion(2), outputKind(IRGenOutputKind::LLVMAssembly),
        verify(true), optMode(OptimizationMode::NotSet),
        sanitizers(OptionSet<SanitizerKind>()),
        debugInfoLevel(IRGenDebugInfoLevel::None),
        debugInfoFormat(IRGenDebugInfoFormat::None),
        disableClangModuleSkeletonCUs(false),
        useJIT(false), integratedREPL(false),
        disableLLVMOptzns(false), disablePolarphpSpecificLLVMOptzns(false),
        disableLLVMSLPVectorizer(false), disableFPElim(true), playground(false),
        emitStackPromotionChecks(false), printInlineTree(false),
        embedMode(IRGenEmbedMode::None), hasValueNamesSetting(false),
        valueNames(false), enableReflectionMetadata(true),
        enableReflectionNames(true), enableAnonymousContextMangledNames(false),
        forcePublicLinkage(false), lazyInitializeClassMetadata(false),
        lazyInitializeProtocolConformances(false), disableLegacyTypeInfo(false),
        useIncrementalLLVMCodeGen(true), usePolarphpCall(false),
        generateProfile(false), enableDynamicReplacementChaining(false),
        disableRoundTripDebugTypes(false), cmdArgs(),
        sanitizeCoverage(llvm::SanitizerCoverageOptions()),
        typeInfoFilter(TypeInfoDumpFilter::All) {}

   // Get a hash of all options which influence the llvm compilation but are not
   // reflected in the llvm module itself.
   unsigned getLLVMCodeGenOptionsHash() {
      unsigned Hash = (unsigned)optMode;
      Hash = (Hash << 1) | disableLLVMOptzns;
      Hash = (Hash << 1) | disablePolarphpSpecificLLVMOptzns;
      return Hash;
   }

   /// Should LLVM IR value names be emitted and preserved?
   bool shouldProvideValueNames() const
   {
      // If the command line contains an explicit request about whether to add
      // LLVM value names, honor it.  Otherwise, add value names only if the
      // final result is textual LLVM assembly.
      if (hasValueNamesSetting) {
         return valueNames;
      } else {
         return outputKind == IRGenOutputKind::LLVMAssembly;
      }
   }

   bool shouldOptimize() const
   {
      return optMode > OptimizationMode::NoOptimization;
   }

   bool optimizeForSize() const
   {
      return optMode == OptimizationMode::ForSize;
   }

   /// Return a hash code of any components from these options that should
   /// contribute to a Swift Bridging PCH hash.
   llvm::hash_code getPCHHashComponents() const
   {
      return llvm::hash_value(0);
   }
};

} // polar::ast

#endif // POLARPHP_AST_IRGEN_OPTIONS_H
