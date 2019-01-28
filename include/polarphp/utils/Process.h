// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/08.

#ifndef POLARPHP_UTILS_PROCESS_H
#define POLARPHP_UTILS_PROCESS_H

#include "polarphp/utils/Allocator.h"
#include "polarphp/utils/Chrono.h"
#include "polarphp/global/DataTypes.h"
#include <system_error>
#include <optional>

namespace polar {

// forward declare class with namespace
namespace basic {
template <typename T>
class ArrayRef;
class StringRef;
} // basic

namespace sys {

using polar::basic::ArrayRef;
using polar::basic::StringRef;
using polar::utils::TimePoint;

/// A collection of legacy interfaces for querying information about the
/// current executing process.
class Process
{
public:
   static unsigned getPageSize();

   /// Return process memory usage.
   /// This static function will return the total amount of memory allocated
   /// by the process. This only counts the memory allocated via the malloc,
   /// calloc and realloc functions and includes any "free" holes in the
   /// allocated space.
   static size_t getMallocUsage();

   /// This static function will set \p user_time to the amount of CPU time
   /// spent in user (non-kernel) mode and \p sys_time to the amount of CPU
   /// time spent in system (kernel) mode.  If the operating system does not
   /// support collection of these metrics, a zero duration will be for both
   /// values.
   /// \param elapsed Returns the system_clock::now() giving current time
   /// \param user_time Returns the current amount of user time for the process
   /// \param sys_time Returns the current amount of system time for the process
   static void getTimeUsage(TimePoint<> &elapsed,
                            std::chrono::nanoseconds &userTime,
                            std::chrono::nanoseconds &sysTime);

   /// This function makes the necessary calls to the operating system to
   /// prevent core files or any other kind of large memory dumps that can
   /// occur when a program fails.
   /// Prevent core file generation.
   static void preventCoreFiles();

   /// true if preventCoreFiles has been called, false otherwise.
   static bool areCoreFilesPrevented();

   // This function returns the environment variable \arg name's value as a UTF-8
   // string. \arg Name is assumed to be in UTF-8 encoding too.
   static std::optional<std::string> getEnv(StringRef name);

   /// This function searches for an existing file in the list of directories
   /// in a PATH like environment variable, and returns the first file found,
   /// according to the order of the entries in the PATH like environment
   /// variable.  If an ignore list is specified, then any folder which is in
   /// the PATH like environment variable but is also in IgnoreList is not
   /// considered.
   static std::optional<std::string> findInEnvPath(StringRef envName,
                                                   StringRef fileName,
                                                   ArrayRef<std::string> ignoreList);

   static std::optional<std::string> findInEnvPath(StringRef envName,
                                                   StringRef fileName);

   // This functions ensures that the standard file descriptors (input, output,
   // and error) are properly mapped to a file descriptor before we use any of
   // them.  This should only be called by standalone programs, library
   // components should not call this.
   static std::error_code fixupStandardFileDescriptors();

   // This function safely closes a file descriptor.  It is not safe to retry
   // close(2) when it returns with errno equivalent to EINTR; this is because
   // *nixen cannot agree if the file descriptor is, in fact, closed when this
   // occurs.
   //
   // N.B. Some operating systems, due to thread cancellation, cannot properly
   // guarantee that it will or will not be closed one way or the other!
   static std::error_code safelyCloseFileDescriptor(int fd);

   /// This function determines if the standard input is connected directly
   /// to a user's input (keyboard probably), rather than coming from a file
   /// or pipe.
   static bool standardInIsUserInput();

   /// This function determines if the standard output is connected to a
   /// "tty" or "console" window. That is, the output would be displayed to
   /// the user rather than being put on a pipe or stored in a file.
   static bool standardOutIsDisplayed();

   /// This function determines if the standard error is connected to a
   /// "tty" or "console" window. That is, the output would be displayed to
   /// the user rather than being put on a pipe or stored in a file.
   static bool standardErrIsDisplayed();

   /// This function determines if the given file descriptor is connected to
   /// a "tty" or "console" window. That is, the output would be displayed to
   /// the user rather than being put on a pipe or stored in a file.
   static bool fileDescriptorIsDisplayed(int fd);

   /// This function determines if the given file descriptor is displayd and
   /// supports colors.
   static bool fileDescriptorHasColors(int fd);

   /// This function determines the number of columns in the window
   /// if standard output is connected to a "tty" or "console"
   /// window. If standard output is not connected to a tty or
   /// console, or if the number of columns cannot be determined,
   /// this routine returns zero.
   static unsigned standardOutColumns();

   /// This function determines the number of columns in the window
   /// if standard error is connected to a "tty" or "console"
   /// window. If standard error is not connected to a tty or
   /// console, or if the number of columns cannot be determined,
   /// this routine returns zero.
   static unsigned standardErrColumns();

   /// This function determines whether the terminal connected to standard
   /// output supports colors. If standard output is not connected to a
   /// terminal, this function returns false.
   static bool standardOutHasColors();

   /// This function determines whether the terminal connected to standard
   /// error supports colors. If standard error is not connected to a
   /// terminal, this function returns false.
   static bool standardErrHasColors();

   /// Enables or disables whether ANSI escape sequences are used to output
   /// colors. This only has an effect on Windows.
   /// Note: Setting this option is not thread-safe and should only be done
   /// during initialization.
   static void useANSIEscapeCodes(bool enable);

   /// Whether changing colors requires the output to be flushed.
   /// This is needed on systems that don't support escape sequences for
   /// changing colors.
   static bool colorNeedsFlush();

   /// This function returns the colorcode escape sequences.
   /// If colorNeedsFlush() is true then this function will change the colors
   /// and return an empty escape sequence. In that case it is the
   /// responsibility of the client to flush the output stream prior to
   /// calling this function.
   static const char *outputColor(char c, bool bold, bool bg);

   /// Same as outputColor, but only enables the bold attribute.
   static const char *outputBold(bool bg);

   /// This function returns the escape sequence to reverse forground and
   /// background colors.
   static const char *outputReverse();

   /// Resets the terminals colors, or returns an escape sequence to do so.
   static const char *resetColor();

   /// Get the result of a process wide random number generator. The
   /// generator will be automatically seeded in non-deterministic fashion.
   static unsigned getRandomNumber();
};

} // sys
} // polar

#endif // POLARPHP_UTILS_PROCESS_H
