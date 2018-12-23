// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/04.

#include "polarphp/utils/PrettyStackTrace.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/utils/Signals.h"
#include "polarphp/utils/WatchDog.h"
#include "polarphp/utils/RawOutStream.h"

#include <cstdarg>
#include <cstdio>
#include <tuple>

#ifdef HAVE_CRASHREPORTERCLIENT_H
#include <CrashReporterClient.h>
#endif

namespace polar {
namespace utils {

using polar::basic::SmallString;

// If backtrace support is not enabled, compile out support for pretty stack
// traces.  This has the secondary effect of not requiring thread local storage
// when backtrace support is disabled.
#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)

// We need a thread local pointer to manage the stack of our stack trace
// objects, but we *really* cannot tolerate destructors running and do not want
// to pay any overhead of synchronizing. As a consequence, we use a raw
// thread-local variable.
static thread_local PrettyStackTraceEntry *sg_prettyStackTraceHead = nullptr;

PrettyStackTraceEntry *reverse_stack_trace(PrettyStackTraceEntry *head)
{
   PrettyStackTraceEntry *prev = nullptr;
   while (head) {
      std::tie(prev, head, head->m_nextEntry) =
            std::make_tuple(head, head->m_nextEntry, prev);
   }
   return prev;
}

static void print_stack(RawOutStream &outstream)
{
   // Print out the stack in reverse order. To avoid recursion (which is likely
   // to fail if we crashed due to stack overflow), we do an up-front pass to
   // reverse the stack, then print it, then reverse it again.
   unsigned id = 0;
   PrettyStackTraceEntry *reversedStack =
         reverse_stack_trace(sg_prettyStackTraceHead);
   for (const PrettyStackTraceEntry *entry = reversedStack; entry;
        entry = entry->getNextEntry()) {
      outstream << id++ << ".\t";
      sys::WatchDog dog(5);
      entry->print(outstream);
   }
   reverse_stack_trace(reversedStack);
}

/// printCurStackTrace - Print the current stack trace to the specified stream.
static void printCurStackTrace(RawOutStream &outstream)
{
   // Don't print an empty trace.
   if (!sg_prettyStackTraceHead) {
      return;
   }
   // If there are pretty stack frames registered, walk and emit them.
   outstream << "Stack dump:\n";

   print_stack(outstream);
   outstream.flush();
}

// Integrate with crash reporter libraries.
#if defined (__APPLE__) && defined(HAVE_CRASHREPORTERCLIENT_H)
//  If any clients of llvm try to link to libCrashReporterClient.a themselves,
//  only one crash info struct will be used.
extern "C" {
CRASH_REPORTER_CLIENT_HIDDEN
struct crashreporter_annotations_t gCRAnnotations
      __attribute__((section("__DATA," CRASHREPORTER_ANNOTATIONS_SECTION)))
= { CRASHREPORTER_ANNOTATIONS_VERSION, 0, 0, 0, 0, 0, 0 };
}
#elif defined(__APPLE__) && HAVE_CRASHREPORTER_INFO
extern "C" const char *__crashreporter_info__
__attribute__((visibility("hidden"))) = 0;
asm(".desc ___crashreporter_info__, 0x10");
#endif

/// crash_handler - This callback is run if a fatal signal is delivered to the
/// process, it prints the pretty stack trace.
static void crash_handler(void *)
{
#ifndef __APPLE__
   // On non-apple systems, just emit the crash stack trace to stderr.
   printCurStackTrace(error_stream());
#else
   // Otherwise, emit to a smallvector of chars, send *that* to stderr, but also
   // put it into __crashreporter_info__.
   SmallString<2048> tmpStr;
   {
      RawSvectorOutStream stream(tmpStr);
      printCurStackTrace(stream);
   }

   if (!tmpStr.empty()) {
#ifdef HAVE_CRASHREPORTERCLIENT_H
      // Cast to void to avoid warning.
      (void)CRSetCrashLogMessage(std::string(tmpStr.str()).c_str());
#elif HAVE_CRASHREPORTER_INFO
      __crashreporter_info__ = strdup(std::string(tmpStr.getStr()).c_str());
#endif
      error_stream() << tmpStr.getStr();
   }

#endif
}

// defined(HAVE_BACKTRACE) && ENABLE_BACKTRACES
#endif

PrettyStackTraceEntry::PrettyStackTraceEntry()
{
#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)
   // Link ourselves.
   m_nextEntry = sg_prettyStackTraceHead;
   sg_prettyStackTraceHead = this;
#endif
}

PrettyStackTraceEntry::~PrettyStackTraceEntry()
{
#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)
   assert(sg_prettyStackTraceHead == this &&
          "Pretty stack trace entry destruction is out of order");
   sg_prettyStackTraceHead = m_nextEntry;
#endif
}

void PrettyStackTraceString::print(RawOutStream &outstream) const
{
   outstream << m_str << "\n";
}

PrettyStackTraceFormat::PrettyStackTraceFormat(const char *format, ...)
{
   va_list ap;
   va_start(ap, format);
   const int sizeOrError = vsnprintf(nullptr, 0, format, ap);
   va_end(ap);
   if (sizeOrError < 0) {
      return;
   }

   const int size = sizeOrError + 1; // '\0'
   m_str.resize(size);
   va_start(ap, format);
   vsnprintf(m_str.getData(), size, format, ap);
   va_end(ap);
}

void PrettyStackTraceFormat::print(RawOutStream &outstream) const
{
   outstream << m_str << "\n";
}

void PrettyStackTraceProgram::print(RawOutStream &outstream) const
{
   outstream << "Program arguments: ";
   // Print the argument list.
   for (unsigned i = 0, e = m_argc; i != e; ++i) {
      outstream << m_argv[i] << ' ';
   }
   outstream << '\n';
}

#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)
static bool register_crash_printer()
{
   utils::add_signal_handler(crash_handler, nullptr);
   return false;
}
#endif

void enable_pretty_stack_trace()
{
#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)
   // The first time this is called, we register the crash printer.
   static bool handlerRegistered = register_crash_printer();
   (void)handlerRegistered;
#endif
}

const void *save_pretty_stack_state()
{
#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)
   return sg_prettyStackTraceHead;
#else
   return nullptr;
#endif
}

void pestore_pretty_stack_state(const void *top)
{
#if defined(HAVE_BACKTRACE) && defined(ENABLE_BACKTRACES)
   sg_prettyStackTraceHead =
         static_cast<PrettyStackTraceEntry *>(const_cast<void *>(top));
#endif
}

} // utils
} // polar
