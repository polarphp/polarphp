//===--- DiagnosticsSema.h - Diagnostic Definitions -------------*- C++ -*-===//
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
/// \file
/// This file defines diagnostics for semantic analysis.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DIAGNOSTICSSEMA_H
#define POLARPHP_AST_DIAGNOSTICSSEMA_H

#include "polarphp/ast/DiagnosticsCommon.h"

namespace polar {
class SwitchStmt;
namespace diag {

/// Describes the kind of requirement in a protocol.
enum class RequirementKind : uint8_t {
   Constructor,
   Func,
   Var,
   Subscript
};

// Declare common diagnostics objects with their appropriate types.
#define DIAG(KIND,ID,Options,Text,Signature) \
    extern internal::DiagWithArguments<void Signature>::type ID;
#define FIXIT(ID,Text,Signature) \
    extern internal::StructuredFixItWithArguments<void Signature>::type ID;
#include "polarphp/ast/DiagnosticsSemaDefs.h"
} //diag
} // polar

#endif // POLARPHP_AST_DIAGNOSTICSSEMA_H
