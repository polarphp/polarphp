//===--- PrettyStackTrace.h - Generic stack-trace prettifiers ---*- C++ -*-===//
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
// Created by polarboy on 2019/12/01.

#ifndef POLARPHP_BASIC_PRETTY_STACKTRACE_H
#define POLARPHP_BASIC_PRETTY_STACKTRACE_H

#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/ADT/StringRef.h"

namespace llvm {
class MemoryBuffer;
}

namespace polar {

/// A PrettyStackTraceEntry for performing an action involving a StringRef.
///
/// The message is:
///   While <action> "<string>"\n
class PrettyStackTraceStringAction : public llvm::PrettyStackTraceEntry
{
   const char *m_action;
   llvm::StringRef m_theString;
public:
   PrettyStackTraceStringAction(const char *action, llvm::StringRef string)
      : m_action(action),
        m_theString(string)
   {}

   void print(llvm::raw_ostream &ostream) const override;
};

/// A PrettyStackTraceEntry to dump the contents of a file.
class PrettyStackTraceFileContents : public llvm::PrettyStackTraceEntry
{
   const llvm::MemoryBuffer &m_buffer;
public:
   explicit PrettyStackTraceFileContents(const llvm::MemoryBuffer &buffer)
      : m_buffer(buffer)
   {}

   void print(llvm::raw_ostream &ostream) const override;
};

/// A PrettyStackTraceEntry to print the version of the compiler.
class PrettyStackTracePolarphpVersion : public llvm::PrettyStackTraceEntry {
public:
   void print(llvm::raw_ostream &OS) const override;
};

} // polar

#endif // POLARPHP_BASIC_PRETTY_STACKTRACE_H
