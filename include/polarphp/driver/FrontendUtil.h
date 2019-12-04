//===--- FrontendUtil.h - Driver Utilities for Frontend ---------*- C++ -*-===//
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
// Created by polarboy on 2019/11/26.

#ifndef POLARPHP_DRIVER_FRONTEND_UTIL_H
#define POLARPHP_DRIVER_FRONTEND_UTIL_H

#include "polarphp/basic/LLVM.h"
#include "llvm/ADT/STLExtras.h"

#include <memory>

namespace polar::ast {
class DiagnosticEngine;
} // polar::driver

namespace polar::driver {

using polar::ast::DiagnosticEngine;

/// Generates the list of arguments that would be passed to the compiler
/// frontend from the given driver arguments.
///
/// \param ArgList The driver arguments (i.e. normal arguments for \c swiftc).
/// \param Diags The DiagnosticEngine used to report any errors parsing the
/// arguments.
/// \param Action Called with the list of frontend arguments if there were no
/// errors in processing \p ArgList. This is a callback rather than a return
/// value to avoid copying the arguments more than necessary.
///
/// \returns True on error, or if \p Action returns true.
///
/// \note This function is not intended to create invocations which are
/// suitable for use in REPL or immediate modes.
bool get_single_frontend_invocation_from_driver_arguments(
      ArrayRef<const char *> argList, DiagnosticEngine &diags,
      llvm::function_ref<bool(ArrayRef<const char *> FrontendArgs)> action);

} // polar::frontend

#endif // POLARPHP_DRIVER_FRONTEND_UTIL_H
