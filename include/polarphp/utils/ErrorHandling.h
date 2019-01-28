// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/09.

//===- llvm/Support/ErrorHandling.h - Fatal error handling ------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines an API used to indicate fatal error conditions.  Non-fatal
// errors (most of them) should be handled through LLVMContext.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_ERROR_HANDLING_H
#define POLARPHP_UTILS_ERROR_HANDLING_H

#include <string>
#include "polarphp/global/CompilerFeature.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*PolarFatalErrorHandler)(const char *reason);

/**
 * Install a fatal error handler. By default, if polarphp detects a fatal error, it
 * will call exit(1). This may not be appropriate in many contexts. For example,
 * doing exit(1) will bypass many crash reporting/tracing system tools. This
 * function allows you to install a callback that will be invoked prior to the
 * call to exit(1).
 */
void polar_install_fatal_error_handler(PolarFatalErrorHandler handler);

/**
 * Reset the fatal error handler. This resets polarphp's fatal error handling
 * behavior to the default.
 */
void polar_reset_fatal_error_handler(void);

/**
 * Enable polarphp's built-in stack trace code. This intercepts the OS's crash
 * signals and prints which component of polarphp you were in at the time if the
 * crash.
 */
void polar_enable_pretty_stack_trace(void);

#ifdef __cplusplus
}
#endif

namespace polar {
namespace utils {

/// An error handler callback.
using FatalErrorHandlerFunc = void (*)(void *userData, const std::string &reason, bool genCrashDiag);

/// install_fatal_error_handler - Installs a new error handler to be used
/// whenever a serious (non-recoverable) error is encountered by polarphp.
///
/// If no error handler is installed the default is to print the error message
/// to stderr, and call exit(1).  If an error handler is installed then it is
/// the handler's responsibility to log the message, it will no longer be
/// printed to stderr.  If the error handler returns, then exit(1) will be
/// called.
///
/// It is dangerous to naively use an error handler which throws an exception.
/// Even though some applications desire to gracefully recover from arbitrary
/// faults, blindly throwing exceptions through unfamiliar code isn't a way to
/// achieve this.
///
/// \param user_data - An argument which will be passed to the install error
/// handler.
void install_fatal_error_handler(FatalErrorHandlerFunc handler,
                                 void *userData = nullptr);

/// Restores default error handling behaviour.
void remove_fatal_error_handler();

/// ScopedFatalErrorHandler - This is a simple helper class which just
/// calls install_fatal_error_handler in its constructor and
/// remove_fatal_error_handler in its destructor.
struct ScopedFatalErrorHandler
{
   explicit ScopedFatalErrorHandler(FatalErrorHandlerFunc handler,
                                    void *userData = nullptr) {
      install_fatal_error_handler(handler, userData);
   }

   ~ScopedFatalErrorHandler()
   {
      remove_fatal_error_handler();
   }
};

/// Reports a serious error, calling any installed error handler. These
/// functions are intended to be used for error conditions which are outside
/// the control of the compiler (I/O errors, invalid user input, etc.)
///
/// If no error handler is installed the default is to print the message to
/// standard error, followed by a newline.
/// After the error handler is called this function will call exit(1), it
/// does not return.
POLAR_ATTRIBUTE_NORETURN void report_fatal_error(const char *reason,
                                                 bool genCrashDiag = true);
POLAR_ATTRIBUTE_NORETURN void report_fatal_error(const std::string &reason,
                                                 bool genCrashDiag = true);
POLAR_ATTRIBUTE_NORETURN void report_fatal_error(std::string_view reason,
                                                 bool genCrashDiag = true);

/// Installs a new bad alloc error handler that should be used whenever a
/// bad alloc error, e.g. failing malloc/calloc, is encountered by LLVM.
///
/// The user can install a bad alloc handler, in order to define the behavior
/// in case of failing allocations, e.g. throwing an exception. Note that this
/// handler must not trigger any additional allocations itself.
///
/// If no error handler is installed the default is to print the error message
/// to stderr, and call exit(1).  If an error handler is installed then it is
/// the handler's responsibility to log the message, it will no longer be
/// printed to stderr.  If the error handler returns, then exit(1) will be
/// called.
///
///
/// \param user_data - An argument which will be passed to the installed error
/// handler.
void install_bad_alloc_error_handler(FatalErrorHandlerFunc handler,
                                     void *userData = nullptr);

/// Restores default bad alloc error handling behavior.
void remove_bad_alloc_error_handler();

/// Reports a bad alloc error, calling any user defined bad alloc
/// error handler. In contrast to the generic 'report_fatal_error'
/// functions, this function is expected to return, e.g. the user
/// defined error handler throws an exception.
///
/// Note: When throwing an exception in the bad alloc handler, make sure that
/// the following unwind succeeds, e.g. do not trigger additional allocations
/// in the unwind chain.
///
/// If no error handler is installed (default), then a bad_alloc exception
/// is thrown, if LLVM is compiled with exception support, otherwise an
/// assertion is called..
void report_bad_alloc_error(const char *reason, bool genCrashDiag = true);
void report_bad_alloc_error(std::string_view reason, bool genCrashDiag = true);

/// This function calls abort(), and prints the optional message to stderr.
/// Use the llvm_unreachable macro (that adds location info), instead of
/// calling this function directly.
POLAR_ATTRIBUTE_NORETURN void
polar_unreachable_internal(const char *msg = nullptr, const char *file = nullptr,
                           unsigned line = 0);

} // utils
} // polar

/// Marks that the current location is not supposed to be reachable.
/// In !NDEBUG builds, prints the message and location info to stderr.
/// In NDEBUG builds, becomes an optimizer hint that the current location
/// is not supposed to be reachable.  On compilers that don't support
/// such hints, prints a reduced message instead.
///
/// Use this instead of assert(0).  It conveys intent more clearly and
/// allows compilers to omit some unnecessary code.
#ifndef NDEBUG
#define polar_unreachable(msg) \
   ::polar::utils::polar_unreachable_internal(msg, __FILE__, __LINE__)
#elif defined(POLAR_BUILTIN_UNREACHABLE)
#define polar_unreachable(msg) POLAR_BUILTIN_UNREACHABLE
#else
#define polar_unreachable(msg) ::polar::utils::polar_unreachable_internal()
#endif

#endif // POLARPHP_UTILS_ERROR_HANDLING_H
