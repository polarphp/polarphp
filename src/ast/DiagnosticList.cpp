//===--- DiagnosticList.cpp - Diagnostic Definitions ----------------------===//
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
//  This file defines all of the diagnostics emitted by Swift.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/DiagnosticsCommon.h"

namespace polar {

enum class DiagID : uint32_t {
#define DIAG(KIND,ID,Options,Text,Signature) ID,
#include "polarphp/ast/DiagnosticsAllDefs.h"
};
static_assert(static_cast<uint32_t>(DiagID::invalid_diagnostic) == 0,
              "0 is not the invalid diagnostic ID");

enum class FixItID : uint32_t {
#define DIAG(KIND, ID, Options, Text, Signature)
#define FIXIT(ID, Text, Signature) ID,
#include "polarphp/ast/DiagnosticsAllDefs.h"
};

// Define all of the diagnostic objects and initialize them with their
// diagnostic IDs.

namespace diag {
#define DIAG(KIND,ID,Options,Text,Signature) \
    internal::DiagWithArguments<void Signature>::type ID = { DiagID::ID };
#define FIXIT(ID, Text, Signature) \
    internal::StructuredFixItWithArguments<void Signature>::type ID = {FixItID::ID};
#include "polarphp/ast/DiagnosticsAllDefs.h"
} // end namespace diag

} // end namespace polar
