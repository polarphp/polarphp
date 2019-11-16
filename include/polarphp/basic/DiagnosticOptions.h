//===--- DiagnosticOptions.h ------------------------------------*- C++ -*-===//
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
// Created by polarboy on 2018/06/29.
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_DIAGNOSTICOPTIONS_H
#define POLARPHP_BASIC_DIAGNOSTICOPTIONS_H

#include "llvm/ADTHashing.h"

namespace polar::basic {

/// Options for controlling diagnostics.
class DiagnosticOptions
{
public:
   /// Indicates whether textual diagnostics should use color.
   bool useColor = false;

   /// Indicates whether the diagnostics produced during compilation should be
   /// checked against expected diagnostics, indicated by markers in the
   /// input source file.
   enum {
      NoVerify,
      Verify,
      VerifyAndApplyFixes
   } verifyMode = NoVerify;

   /// Indicates whether to allow diagnostics for \c <unknown> locations if
   /// \c VerifyMode is not \c NoVerify.
   bool verifyIgnoreUnknown = false;

   /// Indicates whether diagnostic passes should be skipped.
   bool skipDiagnosticPasses = false;

   /// Keep emitting subsequent diagnostics after a fatal error.
   bool showDiagnosticsAfterFatalError = false;

   /// When emitting fixits as code edits, apply all fixits from diagnostics
   /// without any filtering.
   bool fixitCodeForAllDiagnostics = false;

   /// Suppress all warnings
   bool suppressWarnings = false;

   /// Treat all warnings as errors
   bool warningsAsErrors = false;
};

} // polar::basic

#endif // POLARPHP_BASIC_DIAGNOSTICOPTIONS_H
