// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/09.

//===----------------------------------------------------------------------===//
//
// This file provides the generic Unix implementation of the Process class.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
//=== WARNING: Implementation here must contain only generic UNIX code that
//===          is guaranteed to work on *all* UNIX variants.
//===----------------------------------------------------------------------===//

#include "polarphp/global/Config.h"
#include "polarphp/utils/unix/Unix.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/Process.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include "polarphp/basic/adt/Hashing.h"
#include "polarphp/basic/adt/StringRef.h"

#ifdef POLAR_HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef POLAR_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef POLAR_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef POLAR_HAVE_SIGNAL_H
#include <signal.h>
#endif
// DragonFlyBSD, and OpenBSD have deprecated <malloc.h> for
// <stdlib.h> instead. Unix.h includes this for us already.
#if defined(POLAR_HAVE_MALLOC_H) && !defined(__DragonFly__) && \
   !defined(__OpenBSD__)
#include <malloc.h>
#endif
#if defined(HAVE_MALLCTL)
#include <malloc_np.h>
#endif
#ifdef HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif
#ifdef POLAR_HAVE_SYS_IOCTL_H
#  include <sys/ioctl.h>
#endif
#ifdef HAVE_TERMIOS_H
#  include <termios.h>
#endif

#include <mutex>

namespace polar {
namespace sys {

using polar::basic::StringSwitch;
using polar::utils::ManagedStatic;

extern bool sg_coreFilesPrevented;
extern const char sg_colorcodes[][2][8][10];

namespace {
std::pair<std::chrono::microseconds, std::chrono::microseconds> get_resource_usage_times()
{
#if defined(HAVE_GETRUSAGE)
   struct rusage ru;
   ::getrusage(RUSAGE_SELF, &ru);
   return { polar::utils::to_duration(ru.ru_utime), polar::utils::to_duration(ru.ru_stime) };
#else
#warning Cannot get usage times on this platform
   return { std::chrono::microseconds::zero(), std::chrono::microseconds::zero() };
#endif
}
} // anonymous namespace

// On Cygwin, getpagesize() returns 64k(AllocationGranularity) and
// offset in mmap(3) should be aligned to the AllocationGranularity.
unsigned Process::getPageSize()
{
#if defined(HAVE_GETPAGESIZE)
   static const int pageSize = ::getpagesize();
#elif defined(HAVE_SYSCONF)
   static long pageSize = ::sysconf(_SC_pageSize);
#else
#warning Cannot get the page size on this machine
#endif
   return static_cast<unsigned>(pageSize);
}

size_t Process::getMallocUsage()
{
#if defined(HAVE_MALLINFO)
   struct mallinfo mi;
   mi = ::mallinfo();
   return mi.uordblks;
#elif defined(HAVE_MALLOC_ZONE_STATISTICS) && defined(HAVE_MALLOC_MALLOC_H)
   malloc_statistics_t stats;
   malloc_zone_statistics(malloc_default_zone(), &stats);
   return stats.size_in_use;   // darwin
#elif defined(HAVE_MALLCTL)
   size_t alloc, sz;
   sz = sizeof(size_t);
   if (mallctl("stats.allocated", &alloc, &sz, NULL, 0) == 0) {
      return alloc;
   }
   return 0;
#elif defined(HAVE_SBRK)
   // Note this is only an approximation and more closely resembles
   // the value returned by mallinfo in the arena field.
   static char *StartOfMemory = reinterpret_cast<char*>(::sbrk(0));
   char *EndOfMemory = (char*)sbrk(0);
   if (EndOfMemory != ((char*)-1) && StartOfMemory != ((char*)-1))
      return EndOfMemory - StartOfMemory;
   return 0;
#else
#warning Cannot get malloc info on this platform
   return 0;
#endif
}

void Process::getTimeUsage(TimePoint<> &elapsed, std::chrono::nanoseconds &userTime,
                           std::chrono::nanoseconds &sysTime)
{
   elapsed = std::chrono::system_clock::now();
   std::tie(userTime, sysTime) = get_resource_usage_times();
}

#if defined(HAVE_MACH_MACH_H) && !defined(__GNU__)
#include <mach/mach.h>
#endif

// Some LLVM programs such as bugpoint produce core files as a normal part of
// their operation. To prevent the disk from filling up, this function
// does what's necessary to prevent their generation.
void Process::preventCoreFiles()
{
#ifdef POLAR_HAVE_SETRLIMIT
   struct rlimit rlim;
   rlim.rlim_cur = rlim.rlim_max = 0;
   setrlimit(RLIMIT_CORE, &rlim);
#endif

#if defined(HAVE_MACH_MACH_H) && !defined(__GNU__)
   // Disable crash reporting on Mac OS X 10.0-10.4

   // get information about the original set of exception ports for the task
   mach_msg_type_number_t Count = 0;
   exception_mask_t originalMasks[EXC_TYPES_COUNT];
   exception_port_t OriginalPorts[EXC_TYPES_COUNT];
   exception_behavior_t originalBehaviors[EXC_TYPES_COUNT];
   thread_state_flavor_t OriginalFlavors[EXC_TYPES_COUNT];
   kern_return_t err =
         task_get_exception_ports(mach_task_self(), EXC_MASK_ALL, originalMasks,
                                  &Count, OriginalPorts, originalBehaviors,
                                  OriginalFlavors);
   if (err == KERN_SUCCESS) {
      // replace each with MACH_PORT_NULL.
      for (unsigned i = 0; i != Count; ++i) {
         task_set_exception_ports(mach_task_self(), originalMasks[i],
                                  MACH_PORT_NULL, originalBehaviors[i],
                                  OriginalFlavors[i]);
      }
   }

   // Disable crash reporting on Mac OS X 10.5
   signal(SIGABRT, _exit);
   signal(SIGILL,  _exit);
   signal(SIGFPE,  _exit);
   signal(SIGSEGV, _exit);
   signal(SIGBUS,  _exit);
#endif

   sg_coreFilesPrevented = true;
}

std::optional<std::string> Process::getEnv(StringRef name)
{
   std::string nameStr = name.getStr();
   const char *val = ::getenv(nameStr.c_str());
   if (!val) {
      return std::nullopt;
   }
   return std::string(val);
}

namespace {
class FdCloser {
public:
   FdCloser(int &fd) : m_fd(fd), m_keepOpen(false)
   {}
   void keepOpen()
   {
      m_keepOpen = true;
   }

   ~FdCloser()
   {
      if (!m_keepOpen && m_fd >= 0) {
         ::close(m_fd);
      }
   }

private:
   FdCloser(const FdCloser &) = delete;
   void operator=(const FdCloser &) = delete;

   int &m_fd;
   bool m_keepOpen;
};
} // anonymous namespace

std::error_code Process::fixupStandardFileDescriptors()
{
   int nullFD = -1;
   FdCloser fdc(nullFD);
   const int standardFDs[] = {STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO};
   for (int standardFD : standardFDs) {
      struct stat st;
      errno = 0;
      if (polar::utils::retry_after_signal(-1, fstat, standardFD, &st) < 0) {
         assert(errno && "expected errno to be set if fstat failed!");
         // fstat should return EBADF if the file descriptor is closed.
         if (errno != EBADF) {
            return std::error_code(errno, std::generic_category());
         }
      }
      // if fstat succeeds, move on to the next FD.
      if (!errno) {
         continue;
      }
      assert(errno == EBADF && "expected errno to have EBADF at this point!");
      if (nullFD < 0) {
         if ((nullFD = polar::utils::retry_after_signal(-1, open, "/dev/null", O_RDWR)) < 0) {
            return std::error_code(errno, std::generic_category());
         }
      }

      if (nullFD == standardFD) {
         fdc.keepOpen();
      } else if (dup2(nullFD, standardFD) < 0) {
         return std::error_code(errno, std::generic_category());
      }
   }
   return std::error_code();
}

std::error_code Process::safelyCloseFileDescriptor(int fd)
{
   // Create a signal set filled with *all* signals.
   sigset_t fullSet;
   if (sigfillset(&fullSet) < 0) {
      return std::error_code(errno, std::generic_category());
   }
   // Atomically swap our current signal mask with a full mask.
   sigset_t savedSet;
   if (int errorCode = pthread_sigmask(SIG_SETMASK, &fullSet, &savedSet)) {
      return std::error_code(errorCode, std::generic_category());
   }

   // Attempt to close the file descriptor.
   // We need to save the error, if one occurs, because our subsequent call to
   // pthread_sigmask might tamper with errno.
   int errnoFromClose = 0;
   if (::close(fd) < 0) {
      errnoFromClose = errno;
   }

   // Restore the signal mask back to what we saved earlier.
   int errorCode = 0;
   errorCode = pthread_sigmask(SIG_SETMASK, &savedSet, nullptr);
   // The error code from close takes precedence over the one from
   // pthread_sigmask.
   if (errnoFromClose) {
      return std::error_code(errnoFromClose, std::generic_category());
   }
   return std::error_code(errorCode, std::generic_category());
}

bool Process::standardInIsUserInput()
{
   return fileDescriptorIsDisplayed(STDIN_FILENO);
}

bool Process::standardOutIsDisplayed()
{
   return fileDescriptorIsDisplayed(STDOUT_FILENO);
}

bool Process::standardErrIsDisplayed()
{
   return fileDescriptorIsDisplayed(STDERR_FILENO);
}

bool Process::fileDescriptorIsDisplayed(int fd)
{
#ifdef HAVE_ISATTY
   return isatty(fd);
#else
   // If we don't have isatty, just return false.
   return false;
#endif
}

static unsigned getColumns(int fileId)
{
   // If COLUMNS is defined in the environment, wrap to that many columns.
   if (const char *columnsStr = std::getenv("COLUMNS")) {
      int columns = std::atoi(columnsStr);
      if (columns > 0) {
         return columns;
      }
   }
   unsigned columns = 0;
#if defined(POLAR_HAVE_SYS_IOCTL_H) && defined(HAVE_TERMIOS_H)
   // Try to determine the width of the terminal.
   struct winsize ws;
   if (ioctl(fileId, TIOCGWINSZ, &ws) == 0) {
      columns = ws.ws_col;
   }
#endif
   return columns;
}

unsigned Process::standardOutColumns()
{
   if (!standardOutIsDisplayed()) {
      return 0;
   }
   return getColumns(1);
}

unsigned Process::standardErrColumns()
{
   if (!standardErrIsDisplayed()) {
      return 0;
   }
   return getColumns(2);
}

#ifdef HAVE_TERMINFO
// We manually declare these extern functions because finding the correct
// headers from various terminfo, curses, or other sources is harder than
// writing their specs down.
extern "C" int setupterm(char *term, int filedes, int *errret);
extern "C" struct term *set_curterm(struct term *termp);
extern "C" int del_curterm(struct term *termp);
extern "C" int tigetnum(char *capname);
#endif

#ifdef HAVE_TERMINFO
static ManagedStatic<std::mutex> sg_termColorMutex;
#endif

namespace {

static bool terminal_has_colors(int fd)
{
#ifdef HAVE_TERMINFO
   // First, acquire a global lock because these C routines are thread hostile.
   std::lock_guard G(*sg_termColorMutex);

   int errret = 0;
   if (setupterm(nullptr, fd, &errret) != 0) {
      // Regardless of why, if we can't get terminfo, we shouldn't try to print
      // colors.
      return false;
   }
   // Test whether the terminal as set up supports color output. How to do this
   // isn't entirely obvious. We can use the curses routine 'has_colors' but it
   // would be nice to avoid a dependency on curses proper when we can make do
   // with a minimal terminfo parsing library. Also, we don't really care whether
   // the terminal supports the curses-specific color changing routines, merely
   // if it will interpret ANSI color escape codes in a reasonable way. Thus, the
   // strategy here is just to query the baseline colors capability and if it
   // supports colors at all to assume it will translate the escape codes into
   // whatever range of colors it does support. We can add more detailed tests
   // here if users report them as necessary.
   //
   // The 'tigetnum' routine returns -2 or -1 on errors, and might return 0 if
   // the terminfo says that no colors are supported.
   bool hasColors = tigetnum(const_cast<char *>("colors")) > 0;

   // Now extract the structure allocated by setupterm and free its memory
   // through a really silly dance.
   struct term *termp = set_curterm(nullptr);
   (void)del_curterm(termp); // Drop any errors here.

   // Return true if we found a color capabilities for the current terminal.
   if (hasColors) {
       return true;
   }
#else
   // When the terminfo database is not available, check if the current terminal
   // is one of terminals that are known to support ANSI color escape codes.
   if (const char *termStr = std::getenv("TERM")) {
      return StringSwitch<bool>(termStr)
            .cond("ansi", true)
            .cond("cygwin", true)
            .cond("linux", true)
            .startsWith("screen", true)
            .startsWith("xterm", true)
            .startsWith("vt100", true)
            .startsWith("rxvt", true)
            .endsWith("color", true)
            .defaultCond(false);
   }
#endif
   // Otherwise, be conservative.
   return false;
}

} // anonymous namespace

bool Process::fileDescriptorHasColors(int fd)
{
   // A file descriptor has colors if it is displayed and the terminal has
   // colors.
   return fileDescriptorIsDisplayed(fd) && terminal_has_colors(fd);
}

bool Process::standardOutHasColors()
{
   return fileDescriptorHasColors(STDOUT_FILENO);
}

bool Process::standardErrHasColors()
{
   return fileDescriptorHasColors(STDERR_FILENO);
}

void Process::useANSIEscapeCodes(bool /*enable*/)
{
   // No effect.
}

bool Process::colorNeedsFlush()
{
   // No, we use ANSI escape sequences.
   return false;
}

const char *Process::outputColor(char code, bool bold, bool bg)
{
   return sg_colorcodes[bg?1:0][bold?1:0][code&7];
}

const char *Process::outputBold(bool bg)
{
   return "\033[1m";
}

const char *Process::outputReverse()
{
   return "\033[7m";
}

const char *Process::resetColor()
{
   return "\033[0m";
}

#if !HAVE_DECL_ARC4RANDOM
static unsigned get_random_number_seed()
{
   // Attempt to get the initial seed from /dev/urandom, if possible.
   int urandomFD = open("/dev/urandom", O_RDONLY);
   if (urandomFD != -1) {
      unsigned seed;
      // Don't use a buffered read to avoid reading more data
      // from /dev/urandom than we need.
      int count = read(urandomFD, (void *)&seed, sizeof(seed));
      close(urandomFD);
      // Return the seed if the read was successful.
      if (count == sizeof(seed)) {
         return seed;
      }
   }
   // Otherwise, swizzle the current time and the process ID to form a reasonable
   // seed.
   const auto nowValue = std::chrono::high_resolution_clock::now();
   return polar::basic::hash_combine(nowValue.time_since_epoch().count(), ::getpid());
}
#endif

unsigned Process::getRandomNumber()
{
#if HAVE_DECL_ARC4RANDOM
   return arc4random();
#else
   static int x = (static_cast<void>(::srand(get_random_number_seed())), 0);
   (void)x;
   return ::rand();
#endif
}

} // sys
} // polar
