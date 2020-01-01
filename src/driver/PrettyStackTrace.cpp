//===--- PrettyStackTrace.cpp - Defines Driver crash prettifiers ----------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "polarphp/driver/PrettyStackTrace.h"

#include "polarphp/basic/FileTypes.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Job.h"

#include "llvm/Option/Arg.h"
#include "llvm/Support/raw_ostream.h"

using namespace polar::driver;

void PrettyStackTraceDriverAction::print(llvm::raw_ostream &out) const {
   out << "While " << Description << " for driver Action "
       << TheAction->getClassName() << " of type "
       << filetypes::get_type_name(TheAction->getType());
   if (auto *IA = dyn_cast<InputAction>(TheAction)) {
      out << " = ";
      IA->getInputArg().print(out);
   }
   out << '\n';
}

void PrettyStackTraceDriverJob::print(llvm::raw_ostream &out) const {
   out << "While " << Description << " for driver Job ";
   TheJob->printSummary(out);
   out << '\n';
}

void PrettyStackTraceDriverCommandOutput::print(llvm::raw_ostream &out) const {
   out << "While " << Description << " for driver CommandOutput\n";
   TheCommandOutput->print(out);
   out << '\n';
}

void PrettyStackTraceDriverCommandOutputAddition::print(
   llvm::raw_ostream &out) const {
   out << "While adding " << Description << " output named " << NewOutputName
       << " of type " << filetypes::get_type_name(NewOutputType) << " for input "
       << PrimaryInput << " to driver CommandOutput\n";
   TheCommandOutput->print(out);
   out << '\n';
}
