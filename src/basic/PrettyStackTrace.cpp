//===--- PrettyStackTrace.cpp - Generic PrettyStackTraceEntries -----------===//
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
// Created by polarboy on 2019/12/01.
//===----------------------------------------------------------------------===//
//
//  This file implements several PrettyStackTraceEntries that probably
//  ought to be in LLVM.
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/PrettyStackTrace.h"
#include "polarphp/basic/QuotedString.h"
#include "polarphp/basic/Version.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

namespace polar {

void PrettyStackTraceStringAction::print(llvm::raw_ostream &out) const
{
   out << "While " << m_action << ' ' << QuotedString(m_theString) << '\n';
}

void PrettyStackTraceFileContents::print(llvm::raw_ostream &out) const
{
   out << "Contents of " << m_buffer.getBufferIdentifier() << ":\n---\n"
       << m_buffer.getBuffer();
   if (!m_buffer.getBuffer().endswith("\n")) {
      out << '\n';
   }
   out << "---\n";
}

void PrettyStackTracePolarphpVersion::print(llvm::raw_ostream &out) const
{
   out << version::retrieve_polarphp_full_version() << '\n';
}

} // polar
