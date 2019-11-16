//===--- Statistic.h - Helpers for llvm::Statistic --------------*- C++ -*-===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2018 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of Swift project authors
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

#ifndef POLARPHP_BASIC_LANG_STATISTIC_H
#define POLARPHP_BASIC_LANG_STATISTIC_H

#include "llvm/ADTSmallString.h"
#include "llvm/ADTStatistic.h"
#include "polarphp/basic/LangTimer.h"
#include "llvm/ADTPointerUnion.h"

#include <thread>
#include <tuple>

#define POLARPHP_FUNC_STAT SWIFT_FUNC_STAT_NAMED(DEBUG_TYPE)

#define POLARPHP_FUNC_STAT_NAMED(DEBUG_TYPE)                               \
   do {                                                                  \
   static polar::basic::Statistic FStat =                                      \
{DEBUG_TYPE, __func__, __func__, {0}, {false}};                   \
   ++FStat;                                                            \
   } while (0)

namespace polar::basic {

using polar::utils::raw_ostream;

// Helper class designed to consolidate reporting of LLVM statistics and timers
// across polarphp compilations that typically invoke many drivers, each running
// many frontends. Additionally collects some cheap "always-on" statistics,
// beyond those that are (compile-time) parameterized by -DLLVM_ENABLE_STATS
// (LLVM's stats are global and involve some amount of locking and mfences).
//
// Assumes it's given a process name and target name (the latter used as
// decoration for its self-timer), and a directory to collect stats into, then:
//
//  - On construction:
//    - Calls polar::enable_statistics(/*PrintOnExit=*/false)
//    - Calls polar::enable_compilation_timers()
//    - Starts an polar::NamedRegionTimer for this process
//
//  - On destruction:
//    - Add any standard always-enabled stats about the process as a whole
//    - Opens $dir/stats-$timestamp-$name-$random.json for writing
//    - Calls polar::print_statistics_json(ostream) and/or its own writer
//
// Generally we make one of these per-process: either early in the life of the
// driver, or early in the life of the frontend.

class Decl;
class Expr;
class FrontendStatsTracer;
class SourceFile;
class SourceManager;
class Stmt;
class TypeRepr;

// There are a handful of cases where the polarphp compiler can introduce
// counter-measurement noise via nondeterminism, especially via
// parallelism; inhibiting all such cases reliably using existing avenues
// is a bit tricky and depends both on delicate build-setting management
// and some build-system support that is still pending (see
// rdar://39528362); in the meantime we support an environment variable
// ourselves to request blanket suppression of parallelism (and anything
// else nondeterministic we find).

bool environment_variable_requested_maximum_determinism();

class UnifiedStatsReporter
{

public:
   struct AlwaysOnDriverCounters
   {
#define DRIVER_STATISTIC(ID) int64_t ID;
#include "polarphp/basic/LangStatisticDefs.h"
#undef DRIVER_STATISTIC
   };

   struct AlwaysOnFrontendCounters
   {
#define FRONTEND_STATISTIC(NAME, ID) int64_t ID;
#include "polarphp/basic/LangStatisticDefs.h"
#undef FRONTEND_STATISTIC
   };

   // To trace an entity, you have to provide a TraceFormatter for it. This is a
   // separate type since we do not have retroactive conformances in C++, and it
   // is a type that takes void* arguments since we do not have existentials
   // separate from objects in C++. Pity us.
   struct TraceFormatter
   {
      virtual void traceName(const void *entity, raw_ostream &outStream) const = 0;
      virtual void traceLoc(const void *entity,
                            SourceManager *sourceMgr,
                            raw_ostream &outStream) const = 0;
      virtual ~TraceFormatter();
   };

   struct FrontendStatsEvent
   {
      uint64_t timeUSec;
      uint64_t liveUSec;
      bool isEntry;
      StringRef eventName;
      StringRef counterName;
      int64_t counterDelta;
      int64_t counterValue;
      const void *entity;
      const TraceFormatter *formatter;
   };

   // We only write fine-grained trace entries when the user passed
   // -trace-stats-events, but we recycle the same FrontendStatsTracers to give
   // us some free recursion-save phase timings whenever -trace-stats-dir is
   // active at all. Reduces redundant machinery.
   class RecursionSafeTimers;

   // We also keep a few banks of optional hierarchical profilers for times and
   // statistics, activated with -profile-stats-events and
   // -profile-stats-entities, which are part way between the detail level of the
   // aggregate statistic JSON files and the fine-grained CSV traces. Naturally
   // these are written in yet a different file format: the input format for
   // flamegraphs.
   struct StatsProfilers;

private:
   bool m_currentProcessExitStatusSet;
   int m_currentProcessExitStatus;
   SmallString<128> m_statsFilename;
   SmallString<128> m_traceFilename;
   SmallString<128> m_profileDirname;
   polar::utils::TimeRecord m_startedTime;
   std::thread::id m_mainThreadID;

   // This is unique_ptr because NamedRegionTimer is non-copy-constructable.
   std::unique_ptr<polar::utils::NamedRegionTimer> m_timer;

   SourceManager *m_sourceMgr;
   std::optional<AlwaysOnDriverCounters> m_driverCounters;
   std::optional<AlwaysOnFrontendCounters> m_frontendCounters;
   std::optional<AlwaysOnFrontendCounters> m_lastTracedFrontendCounters;
   std::optional<std::vector<FrontendStatsEvent>> m_frontendStatsEvents;

   // These are unique_ptr so we can use incomplete types here.
   std::unique_ptr<RecursionSafeTimers> m_recursiveTimers;
   std::unique_ptr<StatsProfilers> m_eventProfilers;
   std::unique_ptr<StatsProfilers> m_entityProfilers;

   void publishAlwaysOnStatsToKernelStatistic();
   void printAlwaysOnStatsAndTimers(raw_ostream &outStream);

   UnifiedStatsReporter(StringRef programName,
                        StringRef auxName,
                        StringRef directory,
                        SourceManager *sourceMgr,
                        bool traceEvents,
                        bool profileEvents,
                        bool profileEntities);
public:
   UnifiedStatsReporter(StringRef programName,
                        StringRef moduleName,
                        StringRef inputName,
                        StringRef tripleName,
                        StringRef outputType,
                        StringRef optType,
                        StringRef directory,
                        SourceManager *sourceMgr=nullptr,
                        bool traceEvents=false,
                        bool profileEvents=false,
                        bool profileEntities=false);
   ~UnifiedStatsReporter();

   AlwaysOnDriverCounters &getDriverCounters();
   AlwaysOnFrontendCounters &getFrontendCounters();
   void flushTracesAndProfiles();
   void noteCurrentProcessExitStatus(int);
   void saveAnyFrontendStatsEvents(FrontendStatsTracer const &tracer, bool isEntry);
};

// This is a non-nested type just to make it less work to write at call sites.
class FrontendStatsTracer
{
   FrontendStatsTracer(UnifiedStatsReporter *reporter,
                       StringRef eventName,
                       const void *entity,
                       const UnifiedStatsReporter::TraceFormatter *formatter);

   // In the general case we do not know how to format an entity for tracing.
   template<typename T> static
   const UnifiedStatsReporter::TraceFormatter *getTraceFormatter()
   {
      return nullptr;
   }

public:
   UnifiedStatsReporter *reporter;
   polar::utils::TimeRecord savedTime;
   StringRef eventName;
   const void *entity;
   const UnifiedStatsReporter::TraceFormatter *formatter;
   FrontendStatsTracer();
   FrontendStatsTracer(FrontendStatsTracer&& other);
   FrontendStatsTracer& operator=(FrontendStatsTracer&&);
   ~FrontendStatsTracer();
   FrontendStatsTracer(const FrontendStatsTracer&) = delete;
   FrontendStatsTracer& operator=(const FrontendStatsTracer&) = delete;

   /// These are the convenience constructors you want to be calling throughout
   /// the compiler: they select an appropriate trace formatter for the provided
   /// entity type, and produce a tracer that's either active or inert depending
   /// on whether the provided \p reporter is null (nullptr means "tracing is
   /// disabled").
   FrontendStatsTracer(UnifiedStatsReporter *reporter, StringRef eventName);
   FrontendStatsTracer(UnifiedStatsReporter *reporter, StringRef eventName,
                       const Decl *decl);
   FrontendStatsTracer(UnifiedStatsReporter *reporter, StringRef eventName,
                       const Expr *expr);
   FrontendStatsTracer(UnifiedStatsReporter *reporter, StringRef eventName,
                       const SourceFile *file);
   FrontendStatsTracer(UnifiedStatsReporter *reporter, StringRef eventName,
                       const Stmt *stmt);
   FrontendStatsTracer(UnifiedStatsReporter *reporter, StringRef eventName,
                       const TypeRepr *typeRepr);
};

// In particular cases, we do know how to format traced entities: we declare
// explicit specializations of getTraceFormatter() here, matching the overloaded
// constructors of FrontendStatsTracer above, where the _definitions_ live in
// the upper-level files (in libswiftAST or libswiftSIL), and provide tracing
// for those entity types. If you want to trace those types, it's assumed you're
// linking with the object files that define the tracer.

template <>
const UnifiedStatsReporter::TraceFormatter *
FrontendStatsTracer::getTraceFormatter<const Decl *>();

template <>
const UnifiedStatsReporter::TraceFormatter *
FrontendStatsTracer::getTraceFormatter<const Expr *>();

template<> const UnifiedStatsReporter::TraceFormatter*
FrontendStatsTracer::getTraceFormatter<const SourceFile *>();

template<> const UnifiedStatsReporter::TraceFormatter*
FrontendStatsTracer::getTraceFormatter<const Stmt *>();

template<> const UnifiedStatsReporter::TraceFormatter*
FrontendStatsTracer::getTraceFormatter<const TypeRepr *>();

// Provide inline definitions for the delegating constructors.  These avoid
// introducing a circular dependency between libParse and libSIL.  They are
// marked as `inline` explicitly to prevent ODR violations due to multiple
// emissions.  We cannot force the inlining by defining them in the declaration
// due to the explicit template specializations of the `getTraceFormatter`,
// which is declared in the `FrontendStatsTracer` scope (the nested name
// specifier scope cannot be used to declare them).

inline FrontendStatsTracer::FrontendStatsTracer(UnifiedStatsReporter *reporter,
                                                StringRef str)
   : FrontendStatsTracer(reporter, str, nullptr, nullptr) {}

inline FrontendStatsTracer::FrontendStatsTracer(UnifiedStatsReporter *reporter,
                                                StringRef str, const Decl *decl)
   : FrontendStatsTracer(reporter, str, decl, getTraceFormatter<const Decl *>())
{}

inline FrontendStatsTracer::FrontendStatsTracer(UnifiedStatsReporter *reporter,
                                                StringRef str, const Expr *expr)
   : FrontendStatsTracer(reporter, str, expr, getTraceFormatter<const Expr *>())
{}

inline FrontendStatsTracer::FrontendStatsTracer(UnifiedStatsReporter *reporter,
                                                StringRef str,
                                                const SourceFile *file)
   : FrontendStatsTracer(reporter, str, file, getTraceFormatter<const SourceFile *>())
{}

inline FrontendStatsTracer::FrontendStatsTracer(UnifiedStatsReporter *reporter,
                                                StringRef str,
                                                const Stmt *stmt)
   : FrontendStatsTracer(reporter, str, stmt, getTraceFormatter<const Stmt *>())
{}

inline FrontendStatsTracer::FrontendStatsTracer(UnifiedStatsReporter *reporter,
                                                StringRef str,
                                                const TypeRepr *typeRepr)
   : FrontendStatsTracer(reporter, str, typeRepr, getTraceFormatter<const TypeRepr *>())
{}

/// Utilities for constructing TraceFormatters from entities in the request-evaluator:

template <typename T>
typename std::enable_if<
std::is_constructible<FrontendStatsTracer, UnifiedStatsReporter *,
StringRef, const T *>::value,
FrontendStatsTracer>::type
make_tracer_direct(UnifiedStatsReporter *reporter, StringRef name, T *value)
{
   return FrontendStatsTracer(reporter, name, static_cast<const T *>(value));
}

template <typename T>
typename std::enable_if<
std::is_constructible<FrontendStatsTracer, UnifiedStatsReporter *,
StringRef, const T *>::value,
FrontendStatsTracer>::type
make_tracer_direct(UnifiedStatsReporter *reporter, StringRef name,
                   const T *value)
{
   return FrontendStatsTracer(reporter, name, value);
}

template <typename T>
typename std::enable_if<
!std::is_constructible<FrontendStatsTracer, UnifiedStatsReporter *,
StringRef, const T *>::value,
FrontendStatsTracer>::type
make_tracer_direct(UnifiedStatsReporter *reporter, StringRef name, T *value)
{
   return FrontendStatsTracer(reporter, name);
}

template <typename T>
typename std::enable_if<!std::is_pointer<T>::value, FrontendStatsTracer>::type
make_tracer_direct(UnifiedStatsReporter *reporter, StringRef name, T value)
{
   return FrontendStatsTracer(reporter, name);
}

template <typename T>
struct is_pointerunion : std::false_type
{};

template <typename T, typename U>
struct is_pointerunion<PointerUnion<T, U>> : std::true_type
{};

template <typename T, typename U>
FrontendStatsTracer make_tracer_pointerunion(UnifiedStatsReporter *reporter,
                                             StringRef name,
                                             PointerUnion<T, U> value)
{
   if (value.template is<T>()) {
      return make_tracer_direct(reporter, name, value.template get<T>());
   } else {
      return make_tracer_direct(reporter, name, value.template get<U>());
   }
}

template <typename T>
typename std::enable_if<!is_pointerunion<T>::value, FrontendStatsTracer>::type
make_tracer_pointerunion(UnifiedStatsReporter *reporter, StringRef name,
                         T value)
{
   return make_tracer_direct(reporter, name, value);
}

template <typename First, typename... Rest>
FrontendStatsTracer make_tracer(UnifiedStatsReporter *reporter, StringRef name,
                                std::tuple<First, Rest...> value)
{
   return make_tracer_pointerunion(reporter, name, std::get<0>(value));
}

} // polar::basic

#endif // POLARPHP_BASIC_STATISTIC_H
