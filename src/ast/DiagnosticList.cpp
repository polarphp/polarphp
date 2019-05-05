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
// This source file is part of the polarphp.org open source project
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/05/05.
//===----------------------------------------------------------------------===//
//
//  This file defines all of the diagnostics emitted by polarphp.
//
//===----------------------------------------------------------------------===//

#include "polarphp/ast/DiagnosticsCommon.h"

namespace polar::ast {

enum class DiagID : uint32_t
{
#define DIAG(KIND, ID, Options, Text, Signature) ID,
#include "polarphp/ast/DiagnosticsAllDefs.h"
};

static_assert(static_cast<uint32_t>(DiagID::invalid_diagnostic) == 0,
              "0 is not the invalid diagnostic ID");

namespace diag {
#define DIAG(KIND,ID,Options,Text,Signature) \
  internal::DiagWithArguments<void Signature>::type ID = { DiagID::ID };
#include "polarphp/ast/DiagnosticsAllDefs.h"
} // end namespace diag
} // polar::ast
