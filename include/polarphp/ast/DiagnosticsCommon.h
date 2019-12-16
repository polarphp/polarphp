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
//
/// \file
/// This file defines common diagnostics for the whole compiler, as well
/// as some diagnostic infrastructure.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DIAGNOSTICSCOMMON_H
#define POLARPHP_AST_DIAGNOSTICSCOMMON_H

#include "polarphp/ast/DiagnosticEngine.h"
#include "polarphp/basic/LLVM.h"
#include "polarphp/global/Config.h"

namespace polar {
template<typename ...ArgTypes>
struct Diag;

namespace internal {
// These templates are used to help extract the type arguments of the
// DIAG/ERROR/WARNING/NOTE/REMARK/FIXIT macros.
template<typename T>
struct DiagWithArguments;

template<typename ...ArgTypes>
struct DiagWithArguments<void(ArgTypes...)> {
   typedef Diag<ArgTypes...> type;
};

template <typename T>
struct StructuredFixItWithArguments;

template <typename... ArgTypes>
struct StructuredFixItWithArguments<void(ArgTypes...)> {
   typedef StructuredFixIt<ArgTypes...> type;
};
} // end namespace detail

enum class StaticSpellingKind : uint8_t;

namespace diag {

enum class RequirementKind : uint8_t;

using DeclAttribute = const DeclAttribute *;

// Declare common diagnostics objects with their appropriate types.
#define DIAG(KIND,ID,Options,Text,Signature) \
    extern internal::DiagWithArguments<void Signature>::type ID;
#define FIXIT(ID, Text, Signature) \
    extern internal::StructuredFixItWithArguments<void Signature>::type ID;
#include "polarphp/ast/DiagnosticsCommonDefs.h"
} // end namespace diag
} // end namespace polar

#endif // POLARPHP_AST_DIAGNOSTICSCOMMON_H
