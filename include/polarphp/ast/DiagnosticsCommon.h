//===--- DiagnosticsCommon.h - Shared Diagnostic Definitions ----*- C++ -*-===//
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
// Created by polarboy on 2019/04/25.
//===----------------------------------------------------------------------===//
//
/// \file
/// This file defines common diagnostics for the whole compiler, as well
/// as some diagnostic infrastructure.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DIAGNOSTIC_COMMON_H
#define POLARPHP_AST_DIAGNOSTIC_COMMON_H

#include "polarphp/ast/DiagnosticEngine.h"

namespace polar::ast {

template<typename ...ArgTypes>
struct Diag;

namespace internal
{
  template<typename T>
  struct DiagWithArguments;

  template<typename ...ArgTypes>
  struct DiagWithArguments<void(ArgTypes...)>
  {
    typedef Diag<ArgTypes...> type;
  };
}

enum class StaticSpellingKind : uint8_t;

namespace diag {

  enum class RequirementKind : uint8_t;

// Declare common diagnostics objects with their appropriate types.
#define DIAG(KIND, ID, Options, Text, Signature) \
  extern internal::DiagWithArguments<void Signature>::type ID;
#include "polarphp/ast/DiagnosticsCommonDefs.h"
}

} // polar::ast

#endif // POLARPHP_AST_DIAGNOSTIC_COMMON_H
