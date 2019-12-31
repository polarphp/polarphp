//===--- SanitizerOptions.cpp - Swift Sanitizer options -------------------===//
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
// Created by polarboy on 2019/12/02.
//
// \file
// This file implements the parsing of sanitizer arguments.
//
//===----------------------------------------------------------------------===//

#include "polarphp/option/SanitizerOptions.h"
#include "polarphp/basic/Platform.h"
#include "polarphp/basic/OptionSet.h"
#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/ast/DiagnosticsFrontend.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Triple.h"

namespace polar {

using namespace llvm::opt;

static StringRef to_string_ref(const SanitizerKind kind)
{
   switch (kind) {
#define SANITIZER(_, kind, name, file) \
   case SanitizerKind::kind: return #name;
#include "polarphp/basic/SanitizersDef.h"
   }
   llvm_unreachable("Unknown sanitizer");
}

static StringRef to_file_name(const SanitizerKind kind)
{
   switch (kind) {
#define SANITIZER(_, kind, name, file) \
   case SanitizerKind::kind: return #file;
#include "polarphp/basic/SanitizersDef.h"
   }
   llvm_unreachable("Unknown sanitizer");
}

static Optional<SanitizerKind> parse(const char* arg)
{
   return llvm::StringSwitch<Optional<SanitizerKind>>(arg)
#define SANITIZER(_, kind, name, file) .Case(#name, SanitizerKind::kind)
#include "polarphp/basic/SanitizersDef.h"
      .Default(None);
}

llvm::SanitizerCoverageOptions parse_sanitizer_coverage_arg_value(
   const llvm::opt::Arg *arg, const llvm::Triple &triple,
   DiagnosticEngine &diags, OptionSet<SanitizerKind> sanitizers)
{

   llvm::SanitizerCoverageOptions opts;
   // The coverage names here follow the names used by clang's
   // ``-fsanitize-coverage=`` flag.
   for (int i = 0, n = arg->getNumValues(); i != n; ++i) {
      if (opts.CoverageType == llvm::SanitizerCoverageOptions::SCK_None) {
         opts.CoverageType =
            llvm::StringSwitch<llvm::SanitizerCoverageOptions::Type>(
               arg->getValue(i))
               .Case("func", llvm::SanitizerCoverageOptions::SCK_Function)
               .Case("bb", llvm::SanitizerCoverageOptions::SCK_BB)
               .Case("edge", llvm::SanitizerCoverageOptions::SCK_Edge)
               .Default(llvm::SanitizerCoverageOptions::SCK_None);
         if (opts.CoverageType != llvm::SanitizerCoverageOptions::SCK_None)
            continue;
      }

      if (StringRef(arg->getValue(i)) == "indirect-calls") {
         opts.IndirectCalls = true;
         continue;
      } else if (StringRef(arg->getValue(i)) == "trace-bb") {
         opts.TraceBB = true;
         continue;
      } else if (StringRef(arg->getValue(i)) == "trace-cmp") {
         opts.TraceCmp = true;
         continue;
      } else if (StringRef(arg->getValue(i)) == "8bit-counters") {
         opts.Use8bitCounters = true;
         continue;
      } else if (StringRef(arg->getValue(i)) == "trace-pc") {
         opts.TracePC = true;
         continue;
      } else if (StringRef(arg->getValue(i)) == "trace-pc-guard") {
         opts.TracePCGuard = true;
         continue;
      }

      // Argument is not supported.
      diags.diagnose(SourceLoc(), diag::error_unsupported_option_argument,
                     arg->getOption().getPrefixedName(), arg->getValue(i));
      return llvm::SanitizerCoverageOptions();
   }

   if (opts.CoverageType == llvm::SanitizerCoverageOptions::SCK_None) {
      diags.diagnose(SourceLoc(), diag::error_option_missing_required_argument,
                     arg->getSpelling(), "\"func\", \"bb\", \"edge\"");
      return llvm::SanitizerCoverageOptions();
   }

   // Running the sanitizer coverage pass will add undefined symbols to
   // functions in compiler-rt's "sanitizer_common". "sanitizer_common" isn't
   // shipped as a separate library we can link with. However those are defined
   // in the various sanitizer runtime libraries so we require that we are
   // doing a sanitized build so we pick up the required functions during
   // linking.
   if (opts.CoverageType != llvm::SanitizerCoverageOptions::SCK_None &&
       !sanitizers) {
      diags.diagnose(SourceLoc(), diag::error_option_requires_sanitizer,
                     arg->getSpelling());
      return llvm::SanitizerCoverageOptions();
   }
   return opts;
}

OptionSet<SanitizerKind> parse_sanitizer_arg_values(
   const llvm::opt::ArgList &Args,
   const llvm::opt::Arg *arg,
   const llvm::Triple &triple,
   DiagnosticEngine &diags,
   llvm::function_ref<bool(llvm::StringRef, bool)> sanitizerRuntimeLibExists)
{
   OptionSet<SanitizerKind> sanitizerSet;

   // Find the sanitizer kind.
   for (const char *argValue : arg->getValues()) {
      Optional<SanitizerKind> optKind = parse(argValue);

      // Unrecognized sanitizer option
      if (!optKind.hasValue()) {
         diags.diagnose(SourceLoc(), diag::error_unsupported_option_argument,
                        arg->getOption().getPrefixedName(), argValue);
         continue;
      }
      SanitizerKind kind = optKind.getValue();

      // Support is determined by existance of the sanitizer library.
      auto fileName = to_file_name(kind);
      bool isShared = (kind != SanitizerKind::Fuzzer);
      bool sanitizerSupported = sanitizerRuntimeLibExists(fileName, isShared);

      // TSan is explicitly not supported for 32 bits.
      if (kind == SanitizerKind::Thread && !triple.isArch64Bit())
         sanitizerSupported = false;

      if (!sanitizerSupported) {
         SmallString<128> b;
         diags.diagnose(SourceLoc(), diag::error_unsupported_opt_for_target,
                        (arg->getOption().getPrefixedName() + to_string_ref(kind))
                           .toStringRef(b),
                        triple.getTriple());
      } else {
         sanitizerSet |= kind;
      }
   }

   // Check that we're one of the known supported targets for sanitizers.
   if (!(triple.isOSDarwin() || triple.isOSLinux() || triple.isOSWindows())) {
      SmallString<128> b;
      diags.diagnose(SourceLoc(), diag::error_unsupported_opt_for_target,
                     (arg->getOption().getPrefixedName() +
                      StringRef(arg->getAsString(Args))).toStringRef(b),
                     triple.getTriple());
   }

   // Address and thread sanitizers can not be enabled concurrently.
   if ((sanitizerSet & SanitizerKind::Thread)
       && (sanitizerSet & SanitizerKind::Address)) {
      SmallString<128> b1;
      SmallString<128> b2;
      diags.diagnose(SourceLoc(), diag::error_argument_not_allowed_with,
                     (arg->getOption().getPrefixedName()
                      + to_string_ref(SanitizerKind::Address)).toStringRef(b1),
                     (arg->getOption().getPrefixedName()
                      + to_string_ref(SanitizerKind::Thread)).toStringRef(b2));
   }

   return sanitizerSet;
}

OptionSet<SanitizerKind> parse_sanitizer_recover_arg_values(
   const llvm::opt::Arg *A, const OptionSet<SanitizerKind> &enabledSanitizers,
   DiagnosticEngine &Diags, bool emitWarnings) {
   OptionSet<SanitizerKind> sanitizerRecoverSet;

   // Find the sanitizer kind.
   for (const char *arg : A->getValues()) {
      Optional<SanitizerKind> optKind = parse(arg);

      // Unrecognized sanitizer option
      if (!optKind.hasValue()) {
         Diags.diagnose(SourceLoc(), diag::error_unsupported_option_argument,
                        A->getOption().getPrefixedName(), arg);
         continue;
      }
      SanitizerKind kind = optKind.getValue();

      // Only support ASan for now.
      if (kind != SanitizerKind::Address) {
         Diags.diagnose(SourceLoc(), diag::error_unsupported_option_argument,
                        A->getOption().getPrefixedName(), arg);
         continue;
      }

      // Check that the sanitizer is enabled.
      if (!(enabledSanitizers & kind)) {
         SmallString<128> b;
         if (emitWarnings) {
            Diags.diagnose(SourceLoc(),
                           diag::warning_option_requires_specific_sanitizer,
                           (A->getOption().getPrefixedName() + to_string_ref(kind))
                              .toStringRef(b),
                           to_string_ref(kind));
         }
         continue;
      }

      sanitizerRecoverSet |= kind;
   }

   return sanitizerRecoverSet;
}


std::string get_sanitizer_list(const OptionSet<SanitizerKind> &set)
{
   std::string list;
#define SANITIZER(_, kind, name, file) \
   if (set & SanitizerKind::kind) list += #name ",";
#include "polarphp/basic/SanitizersDef.h"
   if (!list.empty()) {
      list.pop_back(); // Remove last comma
   }
   return list;
}

} // polar

