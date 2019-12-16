//===--- PrintingDiagnosticConsumer.h - Print Text Diagnostics --*- C++ -*-===//
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
//===----------------------------------------------------------------------===//
//
//  This file defines the PrintingDiagnosticConsumer class, which displays
//  diagnostics as text to a terminal.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_FRONTEND_PRINTING_DIAGNOSTIC_CONSUMER_H
#define POLARPHP_FRONTEND_PRINTING_DIAGNOSTIC_CONSUMER_H

#include "polarphp/basic/LLVM.h"
#include "polarphp/ast/DiagnosticConsumer.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Process.h"

namespace polar::frontend {

using polar::ast::DiagnosticConsumer;
using polar::ast::DiagnosticArgument;
using polar::ast::DiagnosticInfo;
using polar::ast::DiagnosticKind;
using polar::SourceManager;
using polar::SourceLoc;

/// Diagnostic consumer that displays diagnostics to standard error.
class PrintingDiagnosticConsumer : public DiagnosticConsumer
{
public:
   PrintingDiagnosticConsumer(llvm::raw_ostream &stream = llvm::errs()) :
      Stream(stream)
   {}

   virtual void handleDiagnostic(SourceManager &SM, SourceLoc Loc, DiagnosticKind Kind,
                                 StringRef FormatString,
                                 ArrayRef<DiagnosticArgument> FormatArgs,
                                 const DiagnosticInfo &Info,
                                 SourceLoc bufferIndirectlyCausingDiagnostic) override;

   void forceColors()
   {
      ForceColors = true;
      llvm::sys::Process::UseANSIEscapeCodes(true);
   }

   bool didErrorOccur()
   {
      return DidErrorOccur;
   }

private:
   llvm::raw_ostream &Stream;
   bool ForceColors = false;
   bool DidErrorOccur = false;
};

} // polar::frontend

#endif // POLARPHP_FRONTEND_PRINTING_DIAGNOSTIC_CONSUMER_H
