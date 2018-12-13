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

#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/ErrorCode.h"
#include "polarphp/utils/Error.h"
#include "polarphp/utils/Signals.h"
#include "polarphp/utils/WindowsError.h"
#include "polarphp/global/Config.h"

#include <cstring>
#include <cassert>
#include <cstdlib>
#include <mutex>
#include <new>
#if defined(POLAR_HAVE_UNISTD_H)
# include <unistd.h>
#endif
#if defined(_MSC_VER)
# include <io.h>
# include <fcntl.h>
#endif

namespace polar {
namespace utils {

static FatalErrorHandlerFunc sg_errorHandler = nullptr;
static void *sg_errorHandlerUserData = nullptr;

static FatalErrorHandlerFunc sg_badAllocErrorHandler = nullptr;
static void *sg_badAllocErrorHandlerUserData = nullptr;

// Mutexes to synchronize installing error handlers and calling error handlers.
// Do not use ManagedStatic, or that may allocate memory while attempting to
// report an OOM.
//
// This usage of std::mutex has to be conditionalized behind ifdefs because
// of this script:
//   compiler-rt/lib/sanitizer_common/symbolizer/scripts/build_symbolizer.sh
// That script attempts to statically link the LLVM symbolizer library with the
// STL and hide all of its symbols with 'opt -internalize'. To reduce size, it
// cuts out the threading portions of the hermetic copy of libc++ that it
// builds. We can remove these ifdefs if that script goes away.
static std::mutex sg_errorHandlerMutex;
static std::mutex sg_badAllocErrorHandlerMutex;

void install_fatal_error_handler(FatalErrorHandlerFunc handler, void *userData)
{
   std::lock_guard locker(sg_errorHandlerMutex);
   assert(!sg_errorHandler && "Error handler already registered!\n");
   sg_errorHandler = handler;
   sg_errorHandlerUserData = userData;
}

void remove_fatal_error_handler()
{
   std::lock_guard locker(sg_errorHandlerMutex);
   sg_errorHandler = nullptr;
   sg_errorHandlerUserData = nullptr;
}

void report_fatal_error(const char *reason, bool genCrashDiag)
{
   report_fatal_error(std::string_view(reason, std::strlen(reason)), genCrashDiag);
}

void report_fatal_error(const std::string &reason, bool genCrashDiag)
{
   report_fatal_error(reason.c_str(), reason.size());
}

void report_fatal_error(std::string_view reason, bool genCrashDiag)
{
   FatalErrorHandlerFunc handler = nullptr;
   void *handlerData = nullptr;
   {
      // Only acquire the mutex while reading the handler, so as not to invoke a
      // user-supplied callback under a lock.
      std::lock_guard locker(sg_errorHandlerMutex);
      handler = sg_errorHandler;
      handlerData = sg_errorHandlerUserData;
   }

   if (handler) {
      handler(handlerData, std::string(reason), genCrashDiag);
   } else {
      // Blast the result out to stderr.  We don't try hard to make sure this
      // succeeds (e.g. handling EINTR) and we can't use errs() here because
      // raw ostreams can call report_fatal_error.
      std::string buffer;
      std::ostringstream out(buffer);
      out << "polarphp error: " << reason << "\n";
      std::string messageStr = out.str();
      ssize_t written = ::write(2, messageStr.data(), messageStr.size());
      (void)written; // If something went wrong, we deliberately just give up.
   }
   // If we reached here, we are failing ungracefully. Run the interrupt handlers
   // to make sure any special cleanups get done, in particular that we remove
   // files registered with RemoveFileOnSignal.
   run_interrupt_handlers();
   exit(1);
}

void install_bad_alloc_error_handler(FatalErrorHandlerFunc handler, void *userData)
{
   std::lock_guard locker(sg_badAllocErrorHandlerMutex);
   assert(!sg_errorHandler && "Bad alloc error handler already registered!\n");
   sg_badAllocErrorHandler = handler;
   sg_badAllocErrorHandlerUserData = userData;
}

void remove_bad_alloc_error_handler()
{
   std::lock_guard locker(sg_badAllocErrorHandlerMutex);
   sg_badAllocErrorHandler = nullptr;
   sg_badAllocErrorHandlerUserData = nullptr;
}

void report_bad_alloc_error(const char *reason, bool genCrashDiag)
{
   report_bad_alloc_error(std::string_view(reason, std::strlen(reason)));
}

void report_bad_alloc_error(std::string_view reason, bool genCrashDiag)
{
   FatalErrorHandlerFunc handler = nullptr;
   void *handlerData = nullptr;
   {
      // Only acquire the mutex while reading the handler, so as not to invoke a
      // user-supplied callback under a lock.
      std::lock_guard locker(sg_badAllocErrorHandlerMutex);
      handler = sg_badAllocErrorHandler;
      handlerData = sg_badAllocErrorHandlerUserData;
   }
   if (handler) {
      handler(handlerData, std::string(reason), genCrashDiag);
      polar_unreachable("bad alloc handler should not return");
   }
   // If exceptions are enabled, make OOM in malloc look like OOM in new.
   throw std::bad_alloc();
}

void polar_unreachable_internal(const char *msg, const char *file, unsigned line)
{
   // This code intentionally doesn't call the ErrorHandler callback, because
   // llvm_unreachable is intended to be used to indicate "impossible"
   // situations, and not legitimate runtime errors.
   debug_stream() << msg << "\n";
   debug_stream() << "UNREACHABLE executed";
   if (file)
      debug_stream() << " at " << file << ":" << line;
   debug_stream() << "!\n";
   abort();
#ifdef POLAR_BUILTIN_UNREACHABLE
   // Windows systems and possibly others don't declare abort() to be noreturn,
   // so use the unreachable builtin to avoid a Clang self-host warning.
   POLAR_BUILTIN_UNREACHABLE;
#endif

}

} // utils
} // polar

static void bindings_error_handler(void *userData, const std::string &reason,
                                   bool)
{
   PolarFatalErrorHandler handler =
         POLAR_EXTENSION reinterpret_cast<PolarFatalErrorHandler>(userData);
   handler(reason.c_str());
}

void polar_install_fatal_error_handler(PolarFatalErrorHandler handler)
{
   polar::utils::install_fatal_error_handler(bindings_error_handler,
                                             POLAR_EXTENSION reinterpret_cast<void *>(handler));
}

void polar_reset_fatal_error_handler()
{
   polar::utils::remove_fatal_error_handler();
}

#ifdef _WIN32

#include <winerror.h>

// I'd rather not double the line count of the following.
#define MAP_ERR_TO_COND(x, y)                                                  \
   case x:                                                                      \
   return make_error_code(errc::y)

std::error_code polar::utils::map_windows_error(unsigned EV)
{
   switch (EV) {
   MAP_ERR_TO_COND(ERROR_ACCESS_DENIED, permission_denied);
   MAP_ERR_TO_COND(ERROR_ALREADY_EXISTS, file_exists);
   MAP_ERR_TO_COND(ERROR_BAD_UNIT, no_such_device);
   MAP_ERR_TO_COND(ERROR_BUFFER_OVERFLOW, filename_too_long);
   MAP_ERR_TO_COND(ERROR_BUSY, device_or_resource_busy);
   MAP_ERR_TO_COND(ERROR_BUSY_DRIVE, device_or_resource_busy);
   MAP_ERR_TO_COND(ERROR_CANNOT_MAKE, permission_denied);
   MAP_ERR_TO_COND(ERROR_CANTOPEN, io_error);
   MAP_ERR_TO_COND(ERROR_CANTREAD, io_error);
   MAP_ERR_TO_COND(ERROR_CANTWRITE, io_error);
   MAP_ERR_TO_COND(ERROR_CURRENT_DIRECTORY, permission_denied);
   MAP_ERR_TO_COND(ERROR_DEV_NOT_EXIST, no_such_device);
   MAP_ERR_TO_COND(ERROR_DEVICE_IN_USE, device_or_resource_busy);
   MAP_ERR_TO_COND(ERROR_DIR_NOT_EMPTY, directory_not_empty);
   MAP_ERR_TO_COND(ERROR_DIRECTORY, invalid_argument);
   MAP_ERR_TO_COND(ERROR_DISK_FULL, no_space_on_device);
   MAP_ERR_TO_COND(ERROR_FILE_EXISTS, file_exists);
   MAP_ERR_TO_COND(ERROR_FILE_NOT_FOUND, no_such_file_or_directory);
   MAP_ERR_TO_COND(ERROR_HANDLE_DISK_FULL, no_space_on_device);
   MAP_ERR_TO_COND(ERROR_INVALID_ACCESS, permission_denied);
   MAP_ERR_TO_COND(ERROR_INVALID_DRIVE, no_such_device);
   MAP_ERR_TO_COND(ERROR_INVALID_FUNCTION, function_not_supported);
   MAP_ERR_TO_COND(ERROR_INVALID_HANDLE, invalid_argument);
   MAP_ERR_TO_COND(ERROR_INVALID_NAME, invalid_argument);
   MAP_ERR_TO_COND(ERROR_LOCK_VIOLATION, no_lock_available);
   MAP_ERR_TO_COND(ERROR_LOCKED, no_lock_available);
   MAP_ERR_TO_COND(ERROR_NEGATIVE_SEEK, invalid_argument);
   MAP_ERR_TO_COND(ERROR_NOACCESS, permission_denied);
   MAP_ERR_TO_COND(ERROR_NOT_ENOUGH_MEMORY, not_enough_memory);
   MAP_ERR_TO_COND(ERROR_NOT_READY, resource_unavailable_try_again);
   MAP_ERR_TO_COND(ERROR_OPEN_FAILED, io_error);
   MAP_ERR_TO_COND(ERROR_OPEN_FILES, device_or_resource_busy);
   MAP_ERR_TO_COND(ERROR_OUTOFMEMORY, not_enough_memory);
   MAP_ERR_TO_COND(ERROR_PATH_NOT_FOUND, no_such_file_or_directory);
   MAP_ERR_TO_COND(ERROR_BAD_NETPATH, no_such_file_or_directory);
   MAP_ERR_TO_COND(ERROR_READ_FAULT, io_error);
   MAP_ERR_TO_COND(ERROR_RETRY, resource_unavailable_try_again);
   MAP_ERR_TO_COND(ERROR_SEEK, io_error);
   MAP_ERR_TO_COND(ERROR_SHARING_VIOLATION, permission_denied);
   MAP_ERR_TO_COND(ERROR_TOO_MANY_OPEN_FILES, too_many_files_open);
   MAP_ERR_TO_COND(ERROR_WRITE_FAULT, io_error);
   MAP_ERR_TO_COND(ERROR_WRITE_PROTECT, permission_denied);
   MAP_ERR_TO_COND(WSAEACCES, permission_denied);
   MAP_ERR_TO_COND(WSAEBADF, bad_file_descriptor);
   MAP_ERR_TO_COND(WSAEFAULT, bad_address);
   MAP_ERR_TO_COND(WSAEINTR, interrupted);
   MAP_ERR_TO_COND(WSAEINVAL, invalid_argument);
   MAP_ERR_TO_COND(WSAEMFILE, too_many_files_open);
   MAP_ERR_TO_COND(WSAENAMETOOLONG, filename_too_long);
   default:
      return std::error_code(EV, std::system_category());
   }
}

#endif
