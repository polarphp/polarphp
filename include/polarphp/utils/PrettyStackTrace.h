// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/03.

#ifndef POLARPHP_UTILS_PRETTY_STACK_TRACE_H
#define POLARPHP_UTILS_PRETTY_STACK_TRACE_H

#include "polarphp/basic/adt/SmallVector.h"

namespace polar {

// forward declare class with namespace
namespace basic {
template <typename T, unsigned N>
class SmallVector;
} // basic

namespace utils {

class RawOutStream;
using polar::basic::SmallVector;

void enable_pretty_stack_trace();

/// PrettyStackTraceEntry - This class is used to represent a frame of the
/// "pretty" stack trace that is dumped when a program crashes. You can define
/// subclasses of this and declare them on the program stack: when they are
/// constructed and destructed, they will add their symbolic frames to a
/// virtual stack trace.  This gets dumped out if the program crashes.
class PrettyStackTraceEntry
{
   friend PrettyStackTraceEntry *reverse_stack_trace(PrettyStackTraceEntry *);

   PrettyStackTraceEntry *m_nextEntry;
   PrettyStackTraceEntry(const PrettyStackTraceEntry &) = delete;
   void operator=(const PrettyStackTraceEntry &) = delete;
public:
   PrettyStackTraceEntry();
   virtual ~PrettyStackTraceEntry();

   /// print - Emit information about this stack frame to outstream.
   virtual void print(RawOutStream &outstream) const = 0;

   /// getNextEntry - Return the next entry in the list of frames.
   const PrettyStackTraceEntry *getNextEntry() const
   {
      return m_nextEntry;
   }
};

/// PrettyStackTraceString - This object prints a specified string (which
/// should not contain newlines) to the stream as the stack trace when a crash
/// occurs.
class PrettyStackTraceString : public PrettyStackTraceEntry
{
   const char *m_str;
public:
   PrettyStackTraceString(const char *str) : m_str(str)
   {}
   void print(RawOutStream &outstream) const override;
};

/// PrettyStackTraceFormat - This object prints a string (which may use
/// printf-style formatting but should not contain newlines) to the stream
/// as the stack trace when a crash occurs.
class PrettyStackTraceFormat : public PrettyStackTraceEntry
{
   SmallVector<char, 32> m_str;
public:
   PrettyStackTraceFormat(const char *format, ...);
   void print(RawOutStream &outstream) const override;
};

/// PrettyStackTraceProgram - This object prints a specified program arguments
/// to the stream as the stack trace when a crash occurs.
class PrettyStackTraceProgram : public PrettyStackTraceEntry
{
   int m_argc;
   const char *const *m_argv;
public:
   PrettyStackTraceProgram(int argc, const char * const*argv)
      : m_argc(argc), m_argv(argv)
   {
      enable_pretty_stack_trace();
   }
   void print(RawOutStream &outstream) const override;
};

/// Returns the topmost element of the "pretty" stack state.
const void *save_pretty_stack_state();

/// Restores the topmost element of the "pretty" stack state to state, which
/// should come from a previous call to savePrettyStackState().  This is
/// useful when using a CrashRecoveryContext in code that also uses
/// PrettyStackTraceEntries, to make sure the stack that's printed if a crash
/// happens after a crash that's been recovered by CrashRecoveryContext
/// doesn't have frames on it that were added in code unwound by the
/// CrashRecoveryContext.
void restore_pretty_stack_state(const void *state);

} // utils
} // polar

#endif // POLARPHP_UTILS_PRETTY_STACK_TRACE_H
