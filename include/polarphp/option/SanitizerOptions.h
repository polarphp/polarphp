//===--- SanitizerOptions.h - Helpers related to sanitizers -----*- C++ -*-===//
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

#ifndef POLARPHP_OPTIONS_SANITIZER_OPTIONS_H
#define POLARPHP_OPTIONS_SANITIZER_OPTIONS_H

#include "polarphp/basic/Sanitizers.h"
#include "polarphp/basic/OptionSet.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
// FIXME: This include is just for llvm::SanitizerCoverageOptions. We should
// split the header upstream so we don't include so much.
#include "llvm/Transforms/Instrumentation.h"

namespace polar {

class DiagnosticEngine;

/// Parses a -sanitize= argument's values.
///
/// \param Diag If non null, the argument is used to diagnose invalid values.
/// \param sanitizerRuntimeLibExists Function which checks for existence of a
//         sanitizer dylib with a given name.
/// \return Returns a SanitizerKind.
OptionSet<SanitizerKind> parse_sanitizer_arg_values(
      const llvm::opt::ArgList &args, const llvm::opt::Arg *arg,
      const llvm::Triple &triple, DiagnosticEngine &diag,
      llvm::function_ref<bool(llvm::StringRef, bool)> sanitizerRuntimeLibExists);

OptionSet<SanitizerKind> parse_sanitizer_recover_arg_values(
   const llvm::opt::Arg *A, const OptionSet<SanitizerKind> &enabledSanitizers,
   DiagnosticEngine &Diags, bool emitWarnings);

/// Parses a -sanitize-coverage= argument's value.
llvm::SanitizerCoverageOptions parse_sanitizer_coverage_arg_value(
      const llvm::opt::Arg *arg,
      const llvm::Triple &triple,
      DiagnosticEngine &diag,
      OptionSet<SanitizerKind> sanitizers);

/// Returns the active sanitizers as a comma-separated list.
std::string get_sanitizer_list(const OptionSet<SanitizerKind> &optSet);

} // polar

#endif // POLARPHP_OPTIONS_SANITIZER_OPTIONS_H
