// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/10.

#ifndef POLARPHP_UTILS_SIGNALS_H
#define POLARPHP_UTILS_SIGNALS_H

#include <string>

namespace polar {

// forward declare with namespace
namespace basic {
class StringRef;
} // basic

namespace utils {

class RawOutStream;
using polar::basic::StringRef;

/// This function runs all the registered interrupt handlers, including the
/// removal of files registered by RemoveFileOnSignal.
void run_interrupt_handlers();

/// This function registers signal handlers to ensure that if a signal gets
/// delivered that the named file is removed.
/// Remove a file if a fatal signal occurs.
bool remove_file_on_signal(StringRef filename, std::string *errorMsg = nullptr);

/// This function removes a file from the list of files to be removed on
/// signal delivery.
void dont_remove_file_on_signal(StringRef filename);

/// When an error signal (such as SIGABRT or SIGSEGV) is delivered to the
/// process, print a stack trace and then exit.
/// Print a stack trace if a fatal signal occurs.
/// \param Argv0 the current binary name, used to find the symbolizer
///        relative to the current binary before searching $PATH; can be
///        StringRef(), in which case we will only search $PATH.
/// \param DisableCrashReporting if \c true, disable the normal crash
///        reporting mechanisms on the underlying operating system.
void print_stack_trace_on_error_signal(StringRef argv0,
                                       bool disableCrashReporting = false);

/// Disable all system dialog boxes that appear when the process crashes.
void disable_system_dialogs_on_crash();

/// Print the stack trace using the given \c raw_ostream object.
void print_stack_trace(RawOutStream &out);

// Run all registered signal handlers.
void run_signal_handlers();

using SignalHandlerCallback = void (*)(void *);

/// Add a function to be called when an abort/kill signal is delivered to the
/// process. The handler can have a cookie passed to it to identify what
/// instance of the handler it is.
void add_signal_handler(SignalHandlerCallback funcPtr, void *cookie);

/// This function registers a function to be called when the user "interrupts"
/// the program (typically by pressing ctrl-c).  When the user interrupts the
/// program, the specified interrupt function is called instead of the program
/// being killed, and the interrupt function automatically disabled.  Note
/// that interrupt functions are not allowed to call any non-reentrant
/// functions.  An null interrupt function pointer disables the current
/// installed function.  Note also that the handler may be executed on a
/// different thread on some platforms.
/// Register a function to be called when ctrl-c is pressed.
void set_interrupt_function(void (*ifunc)());

} // utils
} // polar

#endif // POLARPHP_UTILS_SIGNALS_H
