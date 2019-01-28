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
//===----------------------------------------------------------------------===//
//
// This file defines some helpful functions for dealing with the possibility of
// Unix signals occurring while your program is running.
//
//===----------------------------------------------------------------------===//
//
// This file is extremely careful to only do signal-safe things while in a
// signal handler. In particular, memory allocation and acquiring a mutex
// while in a signal handler should never occur. ManagedStatic isn't usable from
// a signal handler for 2 reasons:
//
//  1. Creating a new one allocates.
//  2. The signal handler could fire while llvm_shutdown is being processed, in
//     which case the ManagedStatic is in an unknown state because it could
//     already have been destroyed, or be in the process of being destroyed.
//
// Modifying the behavior of the signal handlers (such as registering new ones)
// can acquire a mutex, but all this guarantees is that the signal handler
// behavior is only modified by one thread at a time. A signal handler can still
// fire while this occurs!
//
// Adding work to a signal handler requires lock-freedom (and assume atomics are
// always lock-free) because the signal handler could fire while new work is
// being added.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/MemoryAlloc.h"
#include "polarphp/utils/MemoryBuffer.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/global/Config.h"
#include "polarphp/utils/unix/Unix.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/utils/Signals.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/utils/RawOutStream.h"
#include "polarphp/utils/Format.h"

#include <filesystem>
#include <shared_mutex>
#include <algorithm>
#include <string>
#include <sysexits.h>
#include <iostream>
#include <iomanip>

#ifdef HAVE_BACKTRACE
# include BACKTRACE_HEADER
#endif
#ifdef POLAR_HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef POLAR_HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef POLAR_HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#ifdef HAVE_MACH_MACH_H
#include <mach/mach.h>
#endif
#include <stdio.h>
#ifdef HAVE_LINK_H
#include <link.h>
#endif
#ifdef HAVE__UNWIND_BACKTRACE
// FIXME: We should be able to use <unwind.h> for any target that has an
// _Unwind_Backtrace function, but on FreeBSD the configure test passes
// despite the function not existing, and on Android, <unwind.h> conflicts
// with <link.h>.
#if defined(__GLIBC__) || defined(__GLIBCXX__)
#include <unwind.h>
#else
#undef HAVE__UNWIND_BACKTRACE
#endif
#endif

using polar::utils::ManagedStatic;
using polar::utils::StringRef;

static void signal_handler(int sig);  // defined below.
/// The function to call if ctrl-c is pressed.
using InterruptFunctionType = void (*)();
static std::atomic<InterruptFunctionType> sg_interruptFunction =
      ATOMIC_VAR_INIT(nullptr);

namespace {
/// Signal-safe removal of files.
/// Inserting and erasing from the list isn't signal-safe, but removal of files
/// themselves is signal-safe. Memory is freed when the head is freed, deletion
/// is therefore not signal-safe either.
class FileToRemoveList
{
   std::atomic<char *> m_filename = ATOMIC_VAR_INIT(nullptr);
   std::atomic<FileToRemoveList *> m_next = ATOMIC_VAR_INIT(nullptr);

   FileToRemoveList() = default;
   // Not signal-safe.
   FileToRemoveList(const std::string &str) : m_filename(strdup(str.c_str()))
   {}

public:
   // Not signal-safe.
   ~FileToRemoveList()
   {
      if (FileToRemoveList *next = m_next.exchange(nullptr)) {
         delete next;
      }
      if (char *filename = m_filename.exchange(nullptr)) {
         free(filename);
      }
   }

   // Not signal-safe.
   static void insert(std::atomic<FileToRemoveList *> &head,
                      const std::string &filename)
   {
      // Insert the new file at the end of the list.
      FileToRemoveList *newHead = new FileToRemoveList(filename);
      std::atomic<FileToRemoveList *> *insertionPoint = &head;
      FileToRemoveList *oldHead = nullptr;
      while (!insertionPoint->compare_exchange_strong(oldHead, newHead)) {
         insertionPoint = &oldHead->m_next;
         oldHead = nullptr;
      }
   }

   // Not signal-safe.
   static void erase(std::atomic<FileToRemoveList *> &head,
                     const std::string &filename)
   {
      // Use a lock to avoid concurrent erase: the comparison would access
      // free'd memory.
      static ManagedStatic<std::shared_mutex> mutex;
      std::unique_lock<std::shared_mutex> writeLocker(*mutex);
      for (FileToRemoveList *current = head.load(); current;
           current = current->m_next.load()) {
         if (char *oldFilename = current->m_filename.load()) {
            if (oldFilename != filename) {
               continue;
            }
            // Leave an empty filename.
            oldFilename = current->m_filename.exchange(nullptr);
            // The filename might have become null between the time we
            // compared it and we exchanged it.
            if (oldFilename) {
               free(oldFilename);
            }
         }
      }
   }

   // Signal-safe.
   static void removeAllFiles(std::atomic<FileToRemoveList *> &head)
   {
      // If cleanup were to occur while we're removing files we'd have a bad time.
      // Make sure we're OK by preventing cleanup from doing anything while we're
      // removing files. If cleanup races with us and we win we'll have a leak,
      // but we won't crash.
      FileToRemoveList *oldHead = head.exchange(nullptr);
      for (FileToRemoveList *currentFile = oldHead; currentFile;
           currentFile = currentFile->m_next.load()) {
         // If erasing was occuring while we're trying to remove files we'd look
         // at free'd data. Take away the path and put it back when done.
         if (char *path = currentFile->m_filename.exchange(nullptr)) {
            // Get the status so we can determine if it's a file or directory. If we
            // can't stat the file, ignore it.
            struct stat buf;
            if (stat(path, &buf) != 0) {
               continue;
            }

            // If this is not a regular file, ignore it. We want to prevent removal
            // of special files like /dev/null, even if the compiler is being run
            // with the super-user permissions.
            if (!S_ISREG(buf.st_mode)) {
               continue;
            }
            // Otherwise, remove the file. We ignore any errors here as there is
            // nothing else we can do.
            unlink(path);
            // We're done removing the file, erasing can safely proceed.
            currentFile->m_filename.exchange(path);
         }
      }
      // We're done removing files, cleanup can safely proceed.
      head.exchange(oldHead);
   }
};

static std::atomic<FileToRemoveList *> sg_filesToRemove = ATOMIC_VAR_INIT(nullptr);

/// Clean up the list in a signal-friendly manner.
/// Recall that signals can fire during llvm_shutdown. If this occurs we should
/// either clean something up or nothing at all, but we shouldn't crash!
struct FilesToRemoveCleanup
{
   // Not signal-safe.
   ~FilesToRemoveCleanup()
   {
      FileToRemoveList *head = sg_filesToRemove.exchange(nullptr);
      if (head) {
         delete head;
      }
   }
};

} // anonymous namespace

static std::string sg_argv0;

// Signals that represent requested termination. There's no bug or failure, or
// if there is, it's not our direct responsibility. For whatever reason, our
// continued execution is no longer desirable.
static const int sg_intSigs[] =
{
   SIGHUP, SIGINT, SIGPIPE, SIGTERM, SIGUSR1, SIGUSR2
};

// Signals that represent that we have a bug, and our prompt termination has
// been ordered.
static const int sg_killSigs[] =
{
   SIGILL, SIGTRAP, SIGABRT, SIGFPE, SIGBUS, SIGSEGV, SIGQUIT
   #ifdef SIGSYS
   , SIGSYS
   #endif
   #ifdef SIGXCPU
   , SIGXCPU
   #endif
   #ifdef SIGXFSZ
   , SIGXFSZ
   #endif
   #ifdef SIGEMT
   , SIGEMT
   #endif
};

static std::atomic<unsigned> sg_numRegisteredSignals = ATOMIC_VAR_INIT(0);
static struct {
   struct sigaction m_sa;
   int m_sigNo;
} sg_registeredSignalInfo[polar::basic::array_lengthof(sg_intSigs) + polar::basic::array_lengthof(sg_killSigs)];

#if defined(HAVE_SIGALTSTACK)
// Hold onto both the old and new alternate signal stack so that it's not
// reported as a leak. We don't make any attempt to remove our alt signal
// stack if we remove our signal handlers; that can't be done reliably if
// someone else is also trying to do the same thing.
static stack_t sg_oldAltStack;
static void* sg_newAltStackPointer;

static void create_sig_alt_stack()
{
   const size_t altStackSize = MINSIGSTKSZ + 64 * 1024;

   // If we're executing on the alternate stack, or we already have an alternate
   // signal stack that we're happy with, there's nothing for us to do. Don't
   // reduce the size, some other part of the process might need a larger stack
   // than we do.
   if (sigaltstack(nullptr, &sg_oldAltStack) != 0 ||
       sg_oldAltStack.ss_flags & SS_ONSTACK ||
       (sg_oldAltStack.ss_sp && sg_oldAltStack.ss_size >= altStackSize)) {
      return;
   }
   stack_t altStack = {};
   altStack.ss_sp = static_cast<char *>(polar::utils::safe_malloc(altStackSize));
   sg_newAltStackPointer = altStack.ss_sp; // Save to avoid reporting a leak.
   altStack.ss_size = altStackSize;
   if (sigaltstack(&altStack, &sg_oldAltStack) != 0) {
      free(altStack.ss_sp);
   }
}
#else
static void create_sig_alt_stack() {}
#endif

static void register_handlers()
{ // Not signal-safe.
   // The mutex prevents other threads from registering handlers while we're
   // doing it. We also have to protect the handlers and their count because
   // a signal handler could fire while we're registeting handlers.
   static ManagedStatic<std::shared_mutex> signalHandlerRegistrationMutex;
   std::shared_lock<std::shared_mutex> writeLocker(*signalHandlerRegistrationMutex);

   // If the handlers are already registered, we're done.
   if (sg_numRegisteredSignals.load() != 0) {
      return;
   }
   // Create an alternate stack for signal handling. This is necessary for us to
   // be able to reliably handle signals due to stack overflow.
   create_sig_alt_stack();

   auto registerHandler = [&](int signal) {
      unsigned index = sg_numRegisteredSignals.load();
      assert(index < polar::basic::array_lengthof(sg_registeredSignalInfo) &&
             "Out of space for signal handlers!");

      struct sigaction newHandler;

      newHandler.sa_handler = signal_handler;
      newHandler.sa_flags = SA_NODEFER | SA_RESETHAND | SA_ONSTACK;
      sigemptyset(&newHandler.sa_mask);

      // Install the new handler, save the old one in RegisteredSignalInfo.
      sigaction(signal, &newHandler, &sg_registeredSignalInfo[index].m_sa);
      sg_registeredSignalInfo[index].m_sigNo = signal;
      ++sg_numRegisteredSignals;
   };

   for (auto sig : sg_intSigs) {
      registerHandler(sig);
   }
   for (auto sig : sg_killSigs) {
      registerHandler(sig);
   }
}

static void unregister_handlers()
{
   // Restore all of the signal handlers to how they were before we showed up.
   for (unsigned i = 0, e = sg_numRegisteredSignals.load(); i != e; ++i) {
      sigaction(sg_registeredSignalInfo[i].m_sigNo,
                &sg_registeredSignalInfo[i].m_sa, nullptr);
      --sg_numRegisteredSignals;
   }
}

/// Process the FilesToRemove list.
static void remove_files_to_remove()
{
   FileToRemoveList::removeAllFiles(sg_filesToRemove);
}

// The signal handler that runs.
static void signal_handler(int sig)
{
   // Restore the signal behavior to default, so that the program actually
   // crashes when we return and the signal reissues.  This also ensures that if
   // we crash in our signal handler that the program will terminate immediately
   // instead of recursing in the signal handler.
   unregister_handlers();

   // Unmask all potentially blocked kill signals.
   sigset_t sigMask;
   sigfillset(&sigMask);
   sigprocmask(SIG_UNBLOCK, &sigMask, nullptr);

   {
      remove_files_to_remove();
      if (std::find(std::begin(sg_intSigs), std::end(sg_intSigs), sig)
          != std::end(sg_intSigs)) {
         if (auto oldInterruptFunction = sg_interruptFunction.exchange(nullptr)) {
            return oldInterruptFunction();
         }
         // Send a special return code that drivers can check for, from sysexits.h.
         if (sig == SIGPIPE) {
            exit(EX_IOERR);
         }
         raise(sig);   // Execute the default handler.
         return;
      }
   }
   // Otherwise if it is a fault (like SEGV) run any handler.
   polar::utils::run_signal_handlers();
#ifdef __s390__
   // On S/390, certain signals are delivered with PSW Address pointing to
   // *after* the faulting instruction.  Simply returning from the signal
   // handler would continue execution after that point, instead of
   // re-raising the signal.  Raise the signal manually in those cases.
   if (sig == SIGILL || sig == SIGFPE || sig == SIGTRAP) {
      raise(sig);
   }
#endif
}

namespace polar {
namespace utils {

void run_interrupt_handlers()
{
   remove_files_to_remove();
}

void set_interrupt_function(void (*ifunc)())
{
   sg_interruptFunction.exchange(ifunc);
   register_handlers();
}

// The public API
bool remove_file_on_signal(StringRef filename,
                           std::string *)
{
   // Ensure that cleanup will occur as soon as one file is added.
   static ManagedStatic<FilesToRemoveCleanup> filesToRemoveCleanup;
   *filesToRemoveCleanup;
   FileToRemoveList::insert(sg_filesToRemove, filename.getStr());
   register_handlers();
   return false;
}


// The public API
void dont_remove_file_on_signal(StringRef filename)
{
   FileToRemoveList::erase(sg_filesToRemove, filename.getStr());
}

void insert_signal_handler(SignalHandlerCallback funcPtr,
                           void *cookie);

/// Add a function to be called when a signal is delivered to the process. The
/// handler can have a cookie passed to it to identify what instance of the
/// handler it is.
void add_signal_handler(SignalHandlerCallback funcPtr,
                        void *cookie)
{ // Signal-safe.
   insert_signal_handler(funcPtr, cookie);
   register_handlers();
}

#if ENABLE_BACKTRACES && defined(HAVE__UNWIND_BACKTRACE)
namespace {
int unwind_backtrace(void **stackTrace, int maxEntries)
{
   if (maxEntries < 0) {
      return 0;
   }
   // Skip the first frame ('unwindBacktrace' itself).
   int entries = -1;

   auto handleFrame = [&](_Unwind_Context *context) -> _Unwind_Reason_Code {
      // Apparently we need to detect reaching the end of the stack ourselves.
      void *ip = (void *)_Unwind_GetIP(context);
      if (!ip) {
         return _URC_END_OF_STACK;
      }
      assert(entries < maxEntries && "recursively called after END_OF_STACK?");
      if (entries >= 0) {
         stackTrace[entries] = ip;
      }
      if (++entries == maxEntries) {
         return _URC_END_OF_STACK;
      }
      return _URC_NO_REASON;
   };

   _Unwind_Backtrace(
            [](_Unwind_Context *context, void *handler) {
      return (*static_cast<decltype(handleFrame) *>(handler))(context);
   },
   static_cast<void *>(&handleFrame));
   return std::max(entries, 0);
}
} // anonymous namespace
#endif

// In the case of a program crash or fault, print out a stack trace so that the
// user has an indication of why and where we died.
//
// On glibc systems we have the 'backtrace' function, which works nicely, but
// doesn't demangle symbols.
void print_stack_trace(RawOutStream &out)
{
#if ENABLE_BACKTRACES
   static void *stackTrace[256];
   int depth = 0;
#if defined(HAVE_BACKTRACE)
   // Use backtrace() to output a backtrace on Linux systems with glibc.
   if (!depth) {
      depth = backtrace(stackTrace, static_cast<int>(polar::basic::array_lengthof(stackTrace)));
   }
#endif
#if defined(HAVE__UNWIND_BACKTRACE)
   // Try _Unwind_Backtrace() if backtrace() failed.
   if (!depth) {
      depth = unwind_backtrace(stackTrace,
                               static_cast<int>(polar::basic::array_lengthof(stackTrace)));
   }

#endif
   if (!depth) {
      return;
   }

#if defined(POLAR_HAVE_DLFCN_H) && defined(POLAR_HAVE_DLADDR)
   int width = 0;
   for (int i = 0; i < depth; ++i) {
      Dl_info dlinfo;
      dladdr(stackTrace[i], &dlinfo);
      const char* name = strrchr(dlinfo.dli_fname, '/');
      int nwidth;
      if (!name) {
         nwidth = strlen(dlinfo.dli_fname);
      } else {
         nwidth = strlen(name) - 1;
      }
      if (nwidth > width) {
         width = nwidth;
      }
   }
   for (int i = 0; i < depth; ++i) {
      Dl_info dlinfo;
      dladdr(stackTrace[i], &dlinfo);
      out << format("%-2d", i);
      const char* name = strrchr(dlinfo.dli_fname, '/');
      if (!name) {
         out << format(" %-*s", width, dlinfo.dli_fname);
      } else {
         out << format(" %-*s", width, name+1);
      }
      out << format(" %#0*lx", (int)(sizeof(void*) * 2) + 2,
                    (unsigned long)stackTrace[i]);
      if (dlinfo.dli_sname != nullptr) {
         out << ' ';
         //         int res;
         // @TODO add itaniumDemangle
         //         char* d = itaniumDemangle(dlinfo.dli_sname, nullptr, nullptr, &res);
         //         if (!d) out << dlinfo.dli_sname;
         //         else    out << d;
         //         free(d);
         out << dlinfo.dli_sname;
         // FIXME: When we move to C++11, use %t length modifier. It's not in
         // C++03 and causes gcc to issue warnings. Losing the upper 32 bits of
         // the stack offset for a stack dump isn't likely to cause any problems.
         out << format(" + %u",(unsigned)((char*)stackTrace[i]-
                                          (char*)dlinfo.dli_saddr));
      }
      out << '\n';
   }
#elif defined(HAVE_BACKTRACE)
   backtrace_symbols_fd(stackTrace, depth, STDERR_FILENO);
#endif
#endif
}

namespace {
void print_stack_trace_signal_handler(void *)
{
   print_stack_trace(error_stream());
}
}

void disable_system_dialogs_on_crash()
{}

/// When an error signal (such as SIGABRT or SIGSEGV) is delivered to the
/// process, print a stack trace and then exit.
void print_stack_trace_on_error_signal(StringRef argv0,
                                       bool disableCrashReporting)
{
   ::sg_argv0 = argv0;

   add_signal_handler(print_stack_trace_signal_handler, nullptr);

#if defined(__APPLE__) && ENABLE_CRASH_OVERRIDES
   // Environment variable to disable any kind of crash dialog.
   if (disableCrashReporting || getenv("POLAR_DISABLE_CRASH_REPORT")) {
      mach_port_t self = mach_task_self();
      exception_mask_t mask = EXC_MASK_CRASH;
      kern_return_t ret = task_set_exception_ports(self,
                                                   mask,
                                                   MACH_PORT_NULL,
                                                   EXCEPTION_STATE_IDENTITY | MACH_EXCEPTION_CODES,
                                                   THREAD_STATE_NONE);
      (void)ret;
   }
#endif
}

} // utils
} // polar
