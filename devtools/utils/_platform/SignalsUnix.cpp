// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
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

#include "MemoryBuffer.h"
#include "ManagedStatics.h"
#include "UtilsConfig.h"
#include "unix/Unix.h"

#include <filesystem>
#include <shared_mutex>
#include <algorithm>
#include <string>
#include <iostream>
#ifdef HAVE_BACKTRACE
# include BACKTRACE_HEADER
#endif
#if HAVE_SIGNAL_H
#include <signal.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_DLFCN_H
#include <dlfcn.h>
#endif
#if HAVE_MACH_MACH_H
#include <mach/mach.h>
#endif
#include <stdio.h>
#if HAVE_LINK_H
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

void signal_handler(int sig);  // defined below.
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
} sg_registeredSignalInfo[array_lengthof(sg_intSigs) + array_lengthof(sg_killSigs)];

namespace polar {
namespace utils {



} // utils
} // polar



