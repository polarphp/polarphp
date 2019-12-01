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
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2019/12/01.

#include "polarphp/driver/PrettyStackTrace.h"
#include "polarphp/basic/FileTypes.h"
#include "polarphp/driver/Action.h"
#include "polarphp/driver/Job.h"
#include "llvm/Option/Arg.h"
#include "llvm/Support/raw_ostream.h"

namespace polar::driver {

void PrettyStackTraceDriverAction::print(llvm::raw_ostream &out) const
{
   out << "While " << m_description << " for driver Action "
       << m_theAction->getClassName() << " of type "
       << filetypes::get_type_name(m_theAction->getType());
   if (auto *inputAction = dyn_cast<InputAction>(m_theAction)) {
      out << " = ";
      inputAction->getInputArg().print(out);
   }
   out << '\n';
}

void PrettyStackTraceDriverJob::print(llvm::raw_ostream &out) const
{
   out << "While " << m_description << " for driver Job ";
   m_theJob->printSummary(out);
   out << '\n';
}

void PrettyStackTraceDriverCommandOutput::print(llvm::raw_ostream &out) const
{
   out << "While " << m_description << " for driver CommandOutput\n";
   m_theCommandOutput->print(out);
   out << '\n';
}

void PrettyStackTraceDriverCommandOutputAddition::print(
      llvm::raw_ostream &out) const
{
   out << "While adding " << m_description << " output named " << m_newOutputName
       << " of type " << filetypes::get_type_name(m_newOutputType) << " for input "
       << m_primaryInput << " to driver CommandOutput\n";
   m_theCommandOutput->print(out);
   out << '\n';
}

} // polar::driver
