//===--- LangOptions.h - Language & configuration options -------*- C++ -*-===//
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
//
//  This file defines the LangOptions class, which provides various
//  language and configuration flags.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_KERNEL_LANGOPTIONS_H
#define POLARPHP_KERNEL_LANGOPTIONS_H

#include "polarphp/basic/CycleDiagnosticKind.h"
#include "polarphp/kernel/Version.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Hashing.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/VersionTuple.h"
#include <string>
#include <vector>

namespace polar::kernel {

using llvm::VersionTuple;
using llvm::Triple;
using llvm::StringRef;
using llvm::ArrayRef;
using llvm::SmallVector;
using polar::basic::CycleDiagnosticKind;

/// Kind of implicit platform conditions.
enum class PlatformConditionKind
{
   /// The active os target (OSX, iOS, Linux, etc.)
   OS,
   /// The active arch target (x86_64, i386, arm, arm64, etc.)
   Arch,
   /// The active endianness target (big or little)
   Endianness,
   /// Runtime support (_ObjC or _Native)
   Runtime,
   /// Conditional import of module
   CanImport,
   /// target Environment (currently just 'simulator' or absent)
   TargetEnvironment,
};

/// A collection of options that affect the language dialect and
/// provide compiler debugging facilities.
class LangOptions
{
public:

   /// The target we are building for.
   ///
   /// This represents the minimum deployment target.
   Triple target;

   ///
   /// Language features
   ///

   /// User-overridable language version to compile for.
   version::Version effectiveLanguageVersion = version::Version::getCurrentLanguageVersion();

   /// PackageDescription version to compile for.
   version::Version packageDescriptionVersion;

   /// Disable API availability checking.
   bool disableAvailabilityChecking = false;

   /// Maximum number of typo corrections we are allowed to perform.
   unsigned typoCorrectionLimit = 10;

   /// Should access control be respected?
   bool enableAccessControl = true;

   /// Enable 'availability' restrictions for App Extensions.
   bool enableAppExtensionRestrictions = false;

   ///
   /// Support for alternate usage modes
   ///

   /// Enable features useful for running in the debugger.
   bool debuggerSupport = false;

   /// Allows using identifiers with a leading dollar.
   bool enableDollarIdentifiers = false;

   /// Allow throwing call expressions without annotation with 'try'.
   bool enableThrowWithoutTry = false;

   /// Keep comments during lexing and attach them to declarations.
   bool attachCommentsToDecls = false;

   /// Whether to include initializers when code-completing a postfix
   /// expression.
   bool codeCompleteInitsInPostfixExpr = false;

   /// Whether to use heuristics to decide whether to show call-pattern
   /// completions.
   bool codeCompleteCallPatternHeuristics = false;

   /// If true, <code>@testable import Foo</code> produces an error if \c Foo
   /// was not compiled with -enable-testing.
   bool enableTestableAttrRequiresTestableModule = true;

   ///
   /// Flags for developers
   ///

   /// Whether we are debugging the constraint solver.
   ///
   /// This option enables verbose debugging output from the constraint
   /// solver.
   bool eebugConstraintSolver = false;

   /// Specific solution attempt for which the constraint
   /// solver should be debugged.
   unsigned eebugConstraintSolverAttempt = 0;

   /// Enable named lazy member loading.
   bool namedLazyMemberLoading = true;

   /// Debug the generic signatures computed by the generic signature builder.
   bool debugGenericSignatures = false;

   /// Triggers llvm fatal_error if typechecker tries to typecheck a decl or an
   /// identifier reference with the provided prefix name.
   /// This is for testing purposes.
   std::string debugForbidTypecheckPrefix;

   /// How to diagnose cycles encountered
   CycleDiagnosticKind evaluatorCycleDiagnostics =
         CycleDiagnosticKind::NoDiagnose;

   /// The path to which we should emit GraphViz output for the complete
   /// request-evaluator graph.
   std::string requestEvaluatorGraphVizPath;

   /// The upper bound, in bytes, of temporary data that can be
   /// allocated by the constraint solver.
   unsigned solverMemoryThreshold = 512 * 1024 * 1024;

   unsigned solverBindingThreshold = 1024 * 1024;

   /// The upper bound to number of sub-expressions unsolved
   /// before termination of the shrink phrase of the constraint solver.
   unsigned solverShrinkUnsolvedThreshold = 10;

   /// Disable the shrink phase of the expression type checker.
   bool solverDisableShrink = false;

   /// Disable constraint system performance hacks.
   bool disableConstraintSolverPerformanceHacks = false;

   /// Enable experimental operator designated types feature.
   bool enableOperatorDesignatedTypes = false;

   /// Enable constraint solver support for experimental
   ///        operator protocol designator feature.
   bool solverEnableOperatorDesignatedTypes = false;

   /// The maximum depth to which to test decl circularity.
   unsigned maxCircularityDepth = 500;

   /// Perform all dynamic allocations using malloc/free instead of
   /// optimized custom allocator, so that memory debugging tools can be used.
   bool useMalloc = false;

   /// Enable experimental #assert feature.
   bool enableExperimentalStaticAssert = false;

   /// Staging flag for treating inout parameters as Thread Sanitizer
   /// accesses.
   bool disableTsanInoutInstrumentation = false;

   /// Should we check the target OSs of serialized modules to see that they're
   /// new enough?
   bool enableTargetOSChecking = true;

   /// Whether to attempt to recover from missing cross-references and other
   /// errors when deserializing from a Swift module.
   ///
   /// This is a staging flag; eventually it will be removed.
   bool enableDeserializationRecovery = true;

   /// Should we use \c ASTScope-based resolution for unqualified name lookup?
   bool enableASTScopeLookup = false;

   /// Whether to use the import as member inference system
   ///
   /// When importing a global, try to infer whether we can import it as a
   /// member of some type instead. This includes inits, computed properties,
   /// and methods.
   bool inferImportAsMember = false;

   /// If set to true, the diagnosis engine can assume the emitted diagnostics
   /// will be used in editor. This usually leads to more aggressive fixit.
   bool diagnosticsEditorMode = false;


   /// Diagnose implicit 'override'.
   bool warnImplicitOverrides = false;


   /// Diagnose switches over non-frozen enums that do not have catch-all
   /// cases.
   bool enableNonFrozenEnumExhaustivityDiagnostics = false;

   /// When a conversion from String to Substring fails, emit a fix-it to append
   /// the void subscript '[]'.
   /// FIXME: Remove this flag when void subscripts are implemented.
   /// This is used to guard preemptive testing for the fix-it.
   bool fixStringToSubstringConversions = false;

   /// Whether collect tokens during parsing for syntax coloring.
   bool collectParsedToken = false;

   /// Whether to parse syntax tree. If the syntax tree is built, the generated
   /// AST may not be correct when syntax nodes are reused as part of
   /// incrementals parsing.
   bool buildSyntaxTree = false;

   /// Whether to verify the parsed syntax tree and emit related diagnostics.
   bool verifySyntaxTree = false;

   /// Scaffolding to permit experimentation with finer-grained dependencies
   /// and faster rebuilds.
   bool enableExperimentalDependencies = false;

   /// To mimic existing system, set to false.
   /// To experiment with including file-private and private dependency info,
   /// set to true.
   bool experimentalDependenciesIncludeIntrafileOnes = false;

   /// Sets the target we are building for and updates platform conditions
   /// to match.
   ///
   /// \returns A pair - the first element is true if the OS was invalid.
   /// The second element is true if the Arch was invalid.
   std::pair<bool, bool> setTarget(Triple triple);

   /// Returns the minimum platform version to which code will be deployed.
   ///
   /// This is only implemented on certain OSs. If no target has been
   /// configured, returns v0.0.0.
   VersionTuple getMinPlatformVersion() const
   {
      unsigned major, minor, revision;
      if (target.isMacOSX()) {
         target.getMacOSXVersion(major, minor, revision);
      } else if (target.isiOS()) {
         target.getiOSVersion(major, minor, revision);
      } else if (target.isWatchOS()) {
         target.getOSVersion(major, minor, revision);
      } else if (target.isOSLinux() || target.isOSFreeBSD() ||
                 target.isAndroid() || target.isOSWindows() ||
                 target.isPS4() || target.isOSHaiku() ||
                 target.getTriple().empty()) {
         major = minor = revision = 0;
      } else {
         llvm_unreachable("Unsupported target OS");
      }
      return VersionTuple(major, minor, revision);
   }

   /// Sets an implicit platform condition.
   void addPlatformConditionValue(PlatformConditionKind kind, StringRef value)
   {
      assert(!value.empty());
      m_platformConditionValues.emplace_back(kind, value);
   }

   /// Removes all values added with addPlatformConditionValue.
   void clearAllPlatformConditionValues()
   {
      m_platformConditionValues.clear();
   }

   /// Returns the value for the given platform condition or an empty string.
   StringRef getPlatformConditionValue(PlatformConditionKind kind) const;

   /// Check whether the given platform condition matches the given value.
   bool checkPlatformCondition(PlatformConditionKind kind, StringRef value) const;

   /// Explicit conditional compilation flags, initialized via the '-D'
   /// compiler flag.
   void addCustomConditionalCompilationFlag(StringRef name)
   {
      assert(!name.empty());
      m_customConditionalCompilationFlags.push_back(name);
   }

   /// Determines if a given conditional compilation flag has been set.
   bool isCustomConditionalCompilationFlagSet(StringRef name) const;

   ArrayRef<std::pair<PlatformConditionKind, std::string>>
   getPlatformConditionValues() const
   {
      return m_platformConditionValues;
   }

   ArrayRef<std::string> getCustomConditionalCompilationFlags() const
   {
      return m_customConditionalCompilationFlags;
   }

   /// Whether our effective polarphp version is at least 'major'.
   ///
   /// This is usually the check you want; for example, when introducing
   /// a new language feature which is only visible in Swift 5, you would
   /// check for isPolarVersionAtLeast(5).
   bool isPolarVersionAtLeast(unsigned major, unsigned minor = 0) const
   {
      return effectiveLanguageVersion.isVersionAtLeast(major, minor);
   }

   /// Returns true if the given platform condition argument represents
   /// a supported target operating system.
   ///
   /// \param suggestions Populated with suggested replacements
   /// if a match is not found.
   static bool checkPlatformConditionSupported(
         PlatformConditionKind kind, StringRef Value,
         std::vector<StringRef> &suggestions);

private:
   SmallVector<std::pair<PlatformConditionKind, std::string>, 5>
   m_platformConditionValues;
   SmallVector<std::string, 2> m_customConditionalCompilationFlags;
};

} // polar::kernel

#endif // POLARPHP_KERNEL_LANGOPTIONS_H
