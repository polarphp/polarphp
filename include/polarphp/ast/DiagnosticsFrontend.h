//===--- DiagnosticsFrontend.h - Diagnostic Definitions ---------*- C++ -*-===//
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
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
//
/// \file
/// This file defines diagnostics for the frontend.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DIAGNOSTIC_FRONTEND_H
#define POLARPHP_AST_DIAGNOSTIC_FRONTEND_H

#include "polarphp/ast/DiagnosticsCommon.h"

namespace polar::ast::diag {
// Declare common diagnostics objects with their appropriate types.
#define DIAG(KIND, ID, Options, Text, Signature) \
extern internal::DiagWithArguments<void Signature>::type ID;
#include "DiagnosticsFrontendDefs.h"
} // polar::ast::diag

#endif // POLARPHP_AST_DIAGNOSTIC_FRONTEND_H
