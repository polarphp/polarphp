//===--- CycleDiagnosticKind.h - Cycle Diagnostic Kind ----------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
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
// Created by polarboy on 2018/06/28.
//===----------------------------------------------------------------------===//
//
// This file defines the CycleDiagnosticKind enum.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_BASIC_CYCLE_DIAGNOSTIC_KIND_H
#define POLARPHP_BASIC_CYCLE_DIAGNOSTIC_KIND_H

namespace polar {

/// How to diagnose cycles when they are encountered during evaluation.
enum class CycleDiagnosticKind
{
   /// Don't diagnose cycles at all.
   NoDiagnose,
   /// Diagnose cycles as full-fledged errors.
   FullDiagnose,
   /// Diagnose cycles via debugging dumps.
   DebugDiagnose,
};

} // polar

#endif // POLARPHP_BASIC_CYCLE_DIAGNOSTIC_KIND_H
