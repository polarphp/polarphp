//===--- LangTimer.h - Shared timers for compilation phases ---------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.

#ifndef POLARPHP_BASIC_LANG_TIMER_H
#define POLARPHP_BASIC_LANG_TIMER_H

#include "llvm/Support/Timer.h"

namespace polar::basic {

using llvm::Timer;
using llvm::NamedRegionTimer;
using llvm::StringRef;

/// A convenience class for declaring a timer that's part of the Swift
/// compilation timers group.
///
/// Please don't use this class directly for anything other than the flat,
/// top-level compilation-phase timing numbers; unadorned SharedTimers are
/// enabled, summed and reported via -debug-time-compilation, using LLVM's
/// built-in logic for timer groups, and that logic doesn't work right if
/// there's any nesting or reentry in timers at all (crashes on reentry,
/// simply mis-reports nesting). Additional SharedTimers also confuse users
/// who are expecting to see only top-level phase timings when they pass
/// -debug-time-compilation.
///
/// Instead, please use FrontendStatsTracer objects and the -stats-output-dir
/// subsystem in include/swift/Basic/Statistic.h. In addition to not
/// interfering with users passing -debug-time-compilation, the
/// FrontendStatsTracer objects automatically instantiate nesting-safe and
/// reentry-safe SharedTimers themselves, as well as supporting event and
/// source-entity tracing and profiling.
class SharedTimer
{
private:
   enum class State
   {
      Initial,
      Skipped,
      Enabled
   };

public:
   explicit SharedTimer(StringRef name)
   {
      if (m_compilationTimersEnabled == State::Enabled) {
          m_timer.emplace(name, name, "swift", "Swift compilation");
      } else {
          m_compilationTimersEnabled = State::Skipped;
      }
   }

   /// Must be called before any SharedTimers have been created.
   static void enableCompilationTimers()
   {
      assert(m_compilationTimersEnabled != State::Skipped &&
            "a timer has already been created");
      m_compilationTimersEnabled = State::Enabled;
   }
private:
   static State m_compilationTimersEnabled;
   std::optional<NamedRegionTimer> m_timer;
};

} // polar::basic

#endif // POLARPHP_BASIC_LANG_TIMER_H
