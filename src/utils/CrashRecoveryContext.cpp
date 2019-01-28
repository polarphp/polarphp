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

#include "polarphp/utils/CrashRecoveryContext.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/ManagedStatics.h"
#include <setjmp.h>
#include <thread>
#include <mutex>

namespace polar {
namespace utils {

namespace {

struct CrashRecoveryContextImpl;

thread_local ManagedStatic<const CrashRecoveryContextImpl *> sg_currentContext;

struct CrashRecoveryContextImpl
{
   // When threads are disabled, this links up all active
   // CrashRecoveryContextImpls.  When threads are enabled there's one thread
   // per CrashRecoveryContext and sg_currentContext is a thread-local, so only one
   // CrashRecoveryContextImpl is active per thread and this is always null.
   const CrashRecoveryContextImpl *m_next;

   CrashRecoveryContext *m_crc;
   ::jmp_buf m_jumpBuffer;
   volatile unsigned m_failed : 1;
   unsigned m_switchedThread : 1;

public:
   CrashRecoveryContextImpl(CrashRecoveryContext *crc) : m_crc(crc),
      m_failed(false),
      m_switchedThread(false)
   {
      m_next = *sg_currentContext;
      *sg_currentContext = this;
   }

   ~CrashRecoveryContextImpl()
   {
      if (!m_switchedThread) {
         *sg_currentContext = m_next;
      }
   }

   /// \brief Called when the separate crash-recovery thread was finished, to
   /// indicate that we don't need to clear the thread-local sg_currentContext.
   void setSwitchedThread()
   {
      m_switchedThread = true;
   }

   void handleCrash()
   {
      // Eliminate the current context entry, to avoid re-entering in case the
      // cleanup code crashes.
      *sg_currentContext = m_next;

      assert(!m_failed && "Crash recovery context already failed!");
      m_failed = true;
      // FIXME: Stash the backtrace.
      // Jump back to the runSafely we were called under.
      longjmp(m_jumpBuffer, 1);
   }
};

}

static ManagedStatic<std::mutex> sg_crashRecoveryContextMutex;
static bool sg_crashRecoveryEnabled = false;

thread_local ManagedStatic<const CrashRecoveryContext *> sg_tlIsRecoveringFromCrash;

static void install_exception_or_signal_handlers();
static void uninstall_exception_or_signal_handlers();

CrashRecoveryContextCleanup::~CrashRecoveryContextCleanup()
{}

CrashRecoveryContext::~CrashRecoveryContext()
{
   // Reclaim registered resources.
   CrashRecoveryContextCleanup *i = m_head;
   const CrashRecoveryContext *pc = *sg_tlIsRecoveringFromCrash;
   *sg_tlIsRecoveringFromCrash = this;
   while (i) {
      CrashRecoveryContextCleanup *tmp = i;
      i = tmp->m_next;
      tmp->m_cleanupFired = true;
      tmp->recoverResources();
      delete tmp;
   }
   *sg_tlIsRecoveringFromCrash = pc;
   CrashRecoveryContextImpl *crci = (CrashRecoveryContextImpl *) m_impl;
   delete crci;
}

bool CrashRecoveryContext::isRecoveringFromCrash()
{
   return *sg_tlIsRecoveringFromCrash != nullptr;
}

CrashRecoveryContext *CrashRecoveryContext::getCurrent()
{
   if (!sg_crashRecoveryEnabled) {
      return nullptr;
   }
   const CrashRecoveryContextImpl *crci = *sg_currentContext;
   if (!crci) {
      return nullptr;
   }
   return crci->m_crc;
}

void CrashRecoveryContext::enable()
{
   std::lock_guard locker(*sg_crashRecoveryContextMutex);
   // FIXME: Shouldn't this be a refcount or something?
   if (sg_crashRecoveryEnabled) {
      return;
   }
   sg_crashRecoveryEnabled = true;
   install_exception_or_signal_handlers();
}

void CrashRecoveryContext::disable()
{
   std::lock_guard locker(*sg_crashRecoveryContextMutex);
   if (!sg_crashRecoveryEnabled) {
      return;
   }
   sg_crashRecoveryEnabled = false;
   uninstall_exception_or_signal_handlers();
}

void CrashRecoveryContext::registerCleanup(CrashRecoveryContextCleanup *cleanup)
{
   if (!cleanup) {
      return;
   }
   if (m_head) {
      m_head->m_prev = cleanup;
   }
   cleanup->m_next = m_head;
   m_head = cleanup;
}

void
CrashRecoveryContext::unregisterCleanup(CrashRecoveryContextCleanup *cleanup)
{
   if (!cleanup)
      return;
   if (cleanup == m_head) {
      m_head = cleanup->m_next;
      if (m_head) {
         m_head->m_prev = nullptr;
      }
   }
   else {
      cleanup->m_prev->m_next = cleanup->m_next;
      if (cleanup->m_next) {
         cleanup->m_next->m_prev = cleanup->m_prev;
      }
   }
   delete cleanup;
}

#if defined(_MSC_VER)
// If _MSC_VER is defined, we must have SEH. Use it if it's available. It's way
// better than VEH. Vectored exception handling catches all exceptions happening
// on the thread with installed exception handlers, so it can interfere with
// internal exception handling of other libraries on that thread. SEH works
// exactly as you would expect normal exception handling to work: it only
// catches exceptions if they would bubble out from the stack frame with __try /
// __except.

static void install_exception_or_signal_handlers()
{}

static void uninstall_exception_or_signal_handlers()
{}

bool CrashRecoveryContext::runSafely(FunctionRef<void()> func)
{
   if (!sg_crashRecoveryEnabled) {
      func();
      return true;
   }

   bool result = true;
   __try {
      func();
   } __except (1) { // Catch any exception.
      result = false;
   }
   return result;
}

#else // !_MSC_VER

#if defined(POLAR_ON_WIN32)


#else // !POLAR_ON_WIN32

// Generic POSIX implementation.
//
// This implementation relies on synchronous signals being delivered to the
// current thread. We use a thread local object to keep track of the active
// crash recovery context, and install signal handlers to invoke handleCrash on
// the active object.
//
// This implementation does not to attempt to chain signal handlers in any
// reliable fashion -- if we get a signal outside of a crash recovery context we
// simply disable crash recovery and raise the signal again.

#include <signal.h>

static const int sg_signals[] =
{ SIGABRT, SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGTRAP };
static const unsigned sg_numSignals = polar::basic::array_lengthof(sg_signals);
static struct sigaction sg_prevActions[sg_numSignals];

static void crash_recovery_signal_handler(int signal) {
   // Lookup the current thread local recovery object.
   const CrashRecoveryContextImpl *crci = *sg_currentContext;

   if (!crci) {
      // We didn't find a crash recovery context -- this means either we got a
      // signal on a thread we didn't expect it on, the application got a signal
      // outside of a crash recovery context, or something else went horribly
      // wrong.
      //
      // disable crash recovery and raise the signal again. The assumption here is
      // that the enclosing application will terminate soon, and we won't want to
      // attempt crash recovery again.
      //
      // This call of disable isn't thread safe, but it doesn't actually matter.
      CrashRecoveryContext::disable();
      raise(signal);

      // The signal will be thrown once the signal mask is restored.
      return;
   }

   // Unblock the signal we received.
   sigset_t sigMask;
   sigemptyset(&sigMask);
   sigaddset(&sigMask, signal);
   sigprocmask(SIG_UNBLOCK, &sigMask, nullptr);

   if (crci) {
      const_cast<CrashRecoveryContextImpl*>(crci)->handleCrash();
   }
}

static void install_exception_or_signal_handlers()
{
   // Setup the signal handler.
   struct sigaction handler;
   handler.sa_handler = crash_recovery_signal_handler;
   handler.sa_flags = 0;
   sigemptyset(&handler.sa_mask);

   for (unsigned i = 0; i != sg_numSignals; ++i) {
      sigaction(sg_signals[i], &handler, &sg_prevActions[i]);
   }
}

static void uninstall_exception_or_signal_handlers()
{
   // Restore the previous signal handlers.
   for (unsigned i = 0; i != sg_numSignals; ++i) {
      sigaction(sg_signals[i], &sg_prevActions[i], nullptr);
   }
}

#endif // !POLAR_ON_WIN32

bool CrashRecoveryContext::runSafely(FunctionRef<void()> func)
{
   // If crash recovery is disabled, do nothing.
   if (sg_crashRecoveryEnabled) {
      assert(!m_impl && "Crash recovery context already initialized!");
      CrashRecoveryContextImpl *crci = new CrashRecoveryContextImpl(this);
      m_impl = crci;
      if (setjmp(crci->m_jumpBuffer) != 0) {
         return false;
      }
   }
   func();
   return true;
}

#endif // !_MSC_VER

void CrashRecoveryContext::handleCrash()
{
   CrashRecoveryContextImpl *crci = (CrashRecoveryContextImpl *) m_impl;
   assert(crci && "Crash recovery context never initialized!");
   crci->handleCrash();
}

// FIXME: Portability.
static void setThreadBackgroundPriority()
{
#ifdef __APPLE__
   setpriority(PRIO_DARWIN_THREAD, 0, PRIO_DARWIN_BG);
#endif
}

static bool hasThreadBackgroundPriority()
{
#ifdef __APPLE__
   return getpriority(PRIO_DARWIN_THREAD, 0) == 1;
#else
   return false;
#endif
}

namespace {
struct runSafelyOnThreadInfo
{
   FunctionRef<void()> m_func;
   CrashRecoveryContext *m_crc;
   bool m_useBackgroundPriority;
   bool m_result;
};
}

static void run_safely_on_thread_dispatch(runSafelyOnThreadInfo *info)
{
   if (info->m_useBackgroundPriority) {
      setThreadBackgroundPriority();
   }
   info->m_result = info->m_crc->runSafely(info->m_func);
}

bool CrashRecoveryContext::runSafelyOnThread(FunctionRef<void()> func,
                                             unsigned requestedStackSize)
{
   bool m_useBackgroundPriority = hasThreadBackgroundPriority();
   runSafelyOnThreadInfo info = { func, this, m_useBackgroundPriority, false };
   std::thread thread(run_safely_on_thread_dispatch, &info);
   thread.join();
   if (CrashRecoveryContextImpl *m_crc = (CrashRecoveryContextImpl *)m_impl) {
      m_crc->setSwitchedThread();
   }
   return info.m_result;
}

} // utils
} // polar
