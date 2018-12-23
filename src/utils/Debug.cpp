// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/11.

//===----------------------------------------------------------------------===//
//
// This file implements a handy way of adding debugging information to your
// code, without it being enabled all of the time, and without having to add
// command line options to enable it.
//
// In particular, just wrap your code with the DEBUG() macro, and it will be
// enabled automatically if you specify '-debug' on the command-line.
// Alternatively, you can also use the SET_DEBUG_TYPE("foo") macro to specify
// that your debug code belongs to class "foo".  Then, on the command line, you
// can specify '-debug-only=foo' to enable JUST the debug information for the
// foo class.
//
// When compiling without assertions, the -debug-* options and all code in
// DEBUG() statements disappears, so it does not affect the runtime of the code.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/Debug.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/Signals.h"
#include "polarphp/utils/CircularRawOutStream.h"
#include "polarphp/utils/RawOutStream.h"

#undef is_current_debug_type
#undef set_current_debug_type
#undef set_current_debug_types

namespace polar {

using polar::utils::ManagedStatic;
using polar::utils::SmallVector;
using polar::utils::StringRef;
using polar::utils::CircularRawOutStream;
using polar::utils::RawOutStream;
using polar::utils::error_stream;

// Even though polarVM might be built with NDEBUG, define symbols that the code
// built without NDEBUG can depend on via the poalr/utils/Debug.h header.
/// Exported boolean set by the -debug option.
bool sg_debugFlag = false;

static ManagedStatic<std::vector<std::string>> sg_currentDebugType;

/// Return true if the specified string is the debug type
/// specified on the command line, or if none was specified on the command line
/// with the -debug-only=X option.
bool is_current_debug_type(const char *debugType)
{
   if (sg_currentDebugType->empty()) {
      return true;
   }
   // See if DebugType is in list. Note: do not use find() as that forces us to
   // unnecessarily create an std::string instance.
   for (auto &d : *sg_currentDebugType) {
      if (d == debugType) {
         return true;
      }
   }
   return false;
}

/// Set the current debug type, as if the -debug-only=X
/// option were specified.  Note that sg_debugFlag also needs to be set to true for
/// debug output to be produced.
///
void set_current_debug_types(const char **types, unsigned count);

void set_current_debug_type(const char *type)
{
   set_current_debug_types(&type, 1);
}

void set_current_debug_types(const char **types, unsigned count)
{
   sg_currentDebugType->clear();
   for (size_t index = 0; index < count; ++index)
   {
      sg_currentDebugType->push_back(types[index]);
   }
}

// All Debug.h functionality is a no-op in NDEBUG mode.
#ifndef NDEBUG

// -debug - Command line option to enable the DEBUG statements in the passes.
// This flag may only be enabled in debug builds.
static cmd::Opt<bool, true>
sg_debug("debug", cmd::Desc("Enable debug output"), cmd::Hidden,
         cmd::location(sg_debugFlag));

// -debug-buffer-size - Buffer the last N characters of debug output
//until program termination.
static cmd::Opt<unsigned>
sg_debugBufferSize("debug-buffer-size",
                   cmd::Desc("Buffer the last N characters of debug output "
                             "until program termination. "
                             "[default 0 -- immediate print-out]"),
                   cmd::Hidden,
                   cmd::init(0));

namespace {

struct DebugOnlyOpt
{
   void operator=(const std::string &value) const
   {
      if (value.empty()) {
         return;
      }
      sg_debugFlag = true;
      SmallVector<StringRef,8> dbgTypes;
      StringRef(value).split(dbgTypes, ',', -1, false);
      for (auto dbgType : dbgTypes) {
         sg_currentDebugType->push_back(dbgType);
      }
   }
};

} // anonymous namespace

static DebugOnlyOpt sg_debugOnlyOptLoc;

static cmd::Opt<DebugOnlyOpt, true, cmd::Parser<std::string>>
sg_debugOnly("debug-only", cmd::Desc("Enable a specific type of debug output (comma separated list of types)"),
             cmd::Hidden, cmd::ZeroOrMore, cmd::ValueDesc("debug string"),
             cmd::location(sg_debugOnlyOptLoc), cmd::ValueRequired);
// Signal handlers - dump debug output on termination.

namespace {
void debug_user_sig_handler(void *cookie)
{
   // This is a bit sneaky.  Since this is under #ifndef NDEBUG, we
   // know that debug mode is enabled and dbgs() really is a
   // circular_RawOutStream.  If NDEBUG is defined, then dbgs() ==
   // errs() but this will never be invoked.
   CircularRawOutStream &dbgout =
         static_cast<CircularRawOutStream &>(debug_stream());
   dbgout.flushBufferWithBanner();
}
} // anonymous namespace

/// dbgs - Return a circular-buffered debug stream.
RawOutStream &debug_stream()
{
   // Do one-time initialization in a thread-safe way.
   static struct DbgStream
   {
      CircularRawOutStream m_stream;

      DbgStream() :
         m_stream(error_stream(), "*** Debug Log Output ***\n",
                  (!sg_enableDebugBuffering || !sg_debugFlag) ? 0 : sg_debugBufferSize) {
         if (sg_enableDebugBuffering && sg_debugFlag && sg_debugBufferSize != 0)
            // TODO: Add a handler for SIGUSER1-type signals so the user can
            // force a debug dump.
            polar::utils::add_signal_handler(&debug_user_sig_handler, nullptr);
         // Otherwise we've already set the debug stream buffer size to
         // zero, disabling buffering so it will output directly to errs().
      }
   } thestrm;

   return thestrm.m_stream;
}

#else
// Avoid "has no symbols" warning.
/// dbgs - Return errs().
RawOutStream &debug_stream()
{
   return error_stream();
}
#endif

/// EnableDebugBuffering - Turn on signal handler installation.
///
bool sg_enableDebugBuffering = false;

} // polar
