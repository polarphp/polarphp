//===--- SerializedDiagnosticConsumer.h - Serialize Diagnostics -*- C++ -*-===//
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
// Created by polarboy on 2019/12/04.
//
//  This file defines the SerializedDiagnosticConsumer class, which
//  serializes diagnostics to Clang's serialized diagnostic bitcode format.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_FRONTEND_DIAGNOSTIC_VERIFIER_H
#define POLARPHP_FRONTEND_DIAGNOSTIC_VERIFIER_H

#include "polarphp/basic/LLVM.h"

namespace polar::basic {
class SourceManager;
} // polar::basic

namespace polar::frontend {

using polar::basic::SourceManager;

/// Set up the specified source manager so that diagnostics are captured
/// instead of being printed.
void enable_diagnostic_verifier(SourceManager &sourceMgr);

/// Verify that captured diagnostics meet with the expectations of the source
/// files corresponding to the specified \p bufferIds and tear down our
/// support for capturing and verifying diagnostics.
///
/// This returns true if there are any mismatches found.
bool verify_diagnostics(SourceManager &sourceMgr, ArrayRef<unsigned> bufferIds,
                        bool autoApplyFixes, bool ignoreUnknown);

} // polar::frontend

#endif // POLARPHP_FRONTEND_DIAGNOSTIC_VERIFIER_H
