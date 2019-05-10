//===--- InstrumenterSupport.cpp - Instrumenter Support -------------------===//
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
//
//===----------------------------------------------------------------------===//
//
//  This file implements the supporting functions for writing instrumenters of
//  the Swift AST.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_AST_DIAGNOSTIC_SUPPRESSION_H
#define POLARPHP_AST_DIAGNOSTIC_SUPPRESSION_H

#include <vector>

namespace polar::ast {

class DiagnosticConsumer;
class DiagnosticEngine;

/// RAII class that suppresses diagnostics by temporarily disabling all of
/// the diagnostic consumers.
class DiagnosticSuppression
{
   DiagnosticEngine &m_diags;
   std::vector<DiagnosticConsumer *> m_consumers;

   DiagnosticSuppression(const DiagnosticSuppression &) = delete;
   DiagnosticSuppression &operator=(const DiagnosticSuppression &) = delete;

public:
   explicit DiagnosticSuppression(DiagnosticEngine &diags);
   ~DiagnosticSuppression();
};

} // polar::ast

#endif // POLARPHP_AST_DIAGNOSTIC_SUPPRESSION_H
