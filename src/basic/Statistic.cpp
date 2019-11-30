//===--- Statistic.cpp - Swift unified stats reporting --------------------===//
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
// Created by polarboy on 2019/11/30.

#include "clang/AST/Decl.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "polarphp/basic/Statistic.h"
#include "polarphp/basic/Timer.h"
#include "polarphp/global/Config.h"
//#include "polarphp/pil/PILFunction.h"
#include "polarphp/driver/DependencyGraph.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/Process.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Hashing.h"
#include <chrono>
#include <limits>

#ifdef LLVM_ON_UNIX
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#ifdef HAVE_PROC_PID_RUSAGE
#include <libproc.h>
#endif
#ifdef HAVE_MALLOC_MALLOC_H
#include <malloc/malloc.h>
#endif

namespace polar::basic {

using namespace llvm::sys;
using namespace llvm;

bool environment_variable_requested_maximum_determinism()
{
   if (const char *S = ::getenv("POLARPHPC_MAXIMUM_DETERMINISM")) {
      return (S[0] != '\0');
   }
   return false;
}

static int64_t
get_children_max_resident_set_size()
{
#if defined(HAVE_GETRUSAGE) && !defined(__HAIKU__)
   struct rusage RU;
   ::getrusage(RUSAGE_CHILDREN, &RU);
   int64_t M = static_cast<int64_t>(RU.ru_maxrss);
   if (M < 0) {
      M = std::numeric_limits<int64_t>::max();
   } else {
#ifndef __APPLE__
      // Apple systems report bytes; everything else appears to report KB.
      M <<= 10;
#endif
   }
   return M;
#else
   return 0;
#endif
}

static std::string make_filename(StringRef prefix,
                                 StringRef programName,
                                 StringRef aux_name,
                                 StringRef suffix)
{
   std::string tmp;
   raw_string_ostream stream(tmp);
   auto now = std::chrono::system_clock::now();
   auto dur = now.time_since_epoch();
   auto usec = std::chrono::duration_cast<std::chrono::microseconds>(dur);
   stream << prefix
          << "-" << usec.count()
          << "-" << programName
          << "-" << aux_name
          << "-" << Process::GetRandomNumber()
          << "." << suffix;
   return stream.str();
}

static std::string makeStatsFileName(StringRef programName,
                                     StringRef aux_name)
{
   return make_filename("stats", programName, aux_name, "json");
}

static std::string makeTraceFileName(StringRef programName,
                                     StringRef aux_name)
{
   return make_filename("trace", programName, aux_name, "csv");
}

static std::string makeProfileDirName(StringRef programName,
                                      StringRef aux_name)
{
   return make_filename("profile", programName, aux_name, "dir");
}

// LLVM's statistics-reporting machinery is sensitive to filenames containing
// YAML-quote-requiring characters, which occur surprisingly often in the wild;
// we only need a recognizable and likely-unique name for a target here, not an
// exact filename, so we go with a crude approximation. Furthermore, to avoid
// parse ambiguities when "demangling" counters and filenames we exclude hyphens
// and slashes.
static std::string clean_name(StringRef n)
{
   std::string tmp;
   for (auto c : n) {
      if (('a' <= c && c <= 'z') ||
          ('A' <= c && c <= 'Z') ||
          ('0' <= c && c <= '9') ||
          (c == '.'))
         tmp += c;
      else
         tmp += '_';
   }
   return tmp;
}

static std::string aux_name(StringRef moduleName,
                            StringRef inputName,
                            StringRef tripleName,
                            StringRef outputType,
                            StringRef optType)
{
   if (inputName.empty()) {
      inputName = "all";
   }
   // Dispose of path prefix, which might make composite name too long.
   inputName = path::filename(inputName);
   if (optType.empty()) {
      optType = "Onone";
   }
   if (!outputType.empty() && outputType.front() == '.') {
      outputType = outputType.substr(1);
   }
   if (!optType.empty() && optType.front() == '-') {
      optType = optType.substr(1);
   }
   return (clean_name(moduleName)
           + "-" + clean_name(inputName)
           + "-" + clean_name(tripleName)
           + "-" + clean_name(outputType)
           + "-" + clean_name(optType));
}

class UnifiedStatsReporter::RecursionSafeTimers
{

   struct RecursionSafeTimer
   {
      llvm::Optional<SharedTimer> timer;
      size_t recursionDepth;
   };

   llvm::StringMap<RecursionSafeTimer> m_timers;

public:

   void beginTimer(StringRef name)
   {
      RecursionSafeTimer &T = m_timers[name];
      if (T.recursionDepth == 0) {
         T.timer.emplace(name);
      }
      T.recursionDepth++;
   }

   void endTimer(StringRef name)
   {
      auto iter = m_timers.find(name);
      assert(iter != m_timers.end());
      RecursionSafeTimer &T = iter->getValue();
      assert(T.recursionDepth != 0);
      T.recursionDepth--;
      if (T.recursionDepth == 0) {
         T.timer.reset();
      }
   }
};

} // polar::basic

namespace llvm {
using polar::basic::UnifiedStatsReporter;
using NodeKey = std::tuple<StringRef, const void *, const UnifiedStatsReporter::TraceFormatter *>;
template<>
struct DenseMapInfo<NodeKey>
{
   using FirstInfo = DenseMapInfo<StringRef>;
   using SecondInfo = DenseMapInfo<const void *>;
   using ThirdInfo = DenseMapInfo<const UnifiedStatsReporter::TraceFormatter *>;
   static inline NodeKey getEmptyKey()
   {
      return std::make_tuple(FirstInfo::getEmptyKey(),
                             SecondInfo::getEmptyKey(),
                             ThirdInfo::getEmptyKey());
   }

   static inline NodeKey getTombstoneKey()
   {
      return std::make_tuple(FirstInfo::getTombstoneKey(),
                             SecondInfo::getTombstoneKey(),
                             ThirdInfo::getTombstoneKey());
   }

   static unsigned getHashValue(const NodeKey& p)
   {
      return hash_combine(FirstInfo::getHashValue(std::get<0>(p)),
                          SecondInfo::getHashValue(std::get<1>(p)),
                          ThirdInfo::getHashValue(std::get<2>(p)));
   }

   static bool isEqual(const NodeKey& lhs, const NodeKey& rhs)
   {
      return FirstInfo::isEqual(std::get<0>(lhs), std::get<0>(rhs)) &&
            SecondInfo::isEqual(std::get<1>(lhs), std::get<1>(rhs)) &&
            ThirdInfo::isEqual(std::get<2>(lhs), std::get<2>(rhs));
   }
};
} // llvm

namespace polar::basic {

class StatsProfiler
{
   struct Node
   {
      int64_t selfCount;
      using Key = std::tuple<llvm::StringRef, const void *,
      const UnifiedStatsReporter::TraceFormatter *>;
      Node *parent;
      llvm::DenseMap<Key, std::unique_ptr<Node>> children;

      Node(Node *P=nullptr) : selfCount(0), parent(P)
      {}

      void print(std::vector<Key> &context, raw_ostream &ostream) const
      {
         StringRef delim;
         if (!(selfCount == 0 || context.empty())) {
            for (auto const &key : context) {
               StringRef name;
               const void* entity;
               const UnifiedStatsReporter::TraceFormatter *formatter;
               std::tie(name, entity, formatter) = key;
               ostream << delim << name;
               if (formatter && entity) {
                  ostream << ' ';
                  formatter->traceName(entity, ostream);
               }
               delim = ";";
            }
            ostream << ' ' << selfCount << '\n';
         }
         for (auto const &iter : children) {
            context.push_back(iter.getFirst());
            iter.getSecond()->print(context, ostream);
            context.pop_back();
         }
      }

      Node *getChild(StringRef name,
                     const void *entity,
                     const UnifiedStatsReporter::TraceFormatter *traceFormatter)
      {
         Key key(name, entity, traceFormatter);
         auto iter = children.find(key);
         if (iter != children.end()) {
            return iter->getSecond().get();
         } else {
            auto N = std::make_unique<Node>(this);
            auto P = N.get();
            children.insert(std::make_pair(key, std::move(N)));
            return P;
         }
      }
   };
   Node root;
   Node *curr;
public:

   StatsProfiler()
      : curr(&root)
   {}
   StatsProfiler(StatsProfiler const &Other) = delete;
   StatsProfiler& operator=(const StatsProfiler&) = delete;

   void print(raw_ostream &ostream) const
   {
      std::vector<Node::Key> context;
      root.print(context, ostream);
   }

   void printToFile(StringRef dirname, StringRef filename) const
   {
      SmallString<256> Path(dirname);
      llvm::sys::path::append(Path, filename);
      std::error_code errorCode;
      raw_fd_ostream stream(Path, errorCode, fs::F_Append | fs::F_Text);
      if (errorCode) {
         llvm::errs() << "Error opening profile file '"
                      << Path << "' for writing\n";
         return;
      }
      print(stream);
   }

   void profileEvent(StringRef name,
                     double deltaSeconds,
                     bool isEntry,
                     const void *entity=nullptr,
                     const UnifiedStatsReporter::TraceFormatter *traceFormatter=nullptr)
   {
      int64_t deltaUSec = int64_t(1000000.0 * deltaSeconds);
      profileEvent(name, deltaUSec, isEntry, entity, traceFormatter);
   }

   void profileEvent(StringRef name,
                     int64_t delta,
                     bool isEntry,
                     const void *entity=nullptr,
                     const UnifiedStatsReporter::TraceFormatter *traceFormatter=nullptr)
   {
      assert(curr);
      curr->selfCount += delta;
      if (isEntry) {
         Node *child = curr->getChild(name, entity, traceFormatter);
         assert(child);
         assert(child->parent == curr);
         curr = child;
      } else {
         curr = curr->parent;
         assert(curr);
      }
   }
};

struct UnifiedStatsReporter::StatsProfilers
{
   // Timerecord of last update.
   llvm::TimeRecord lastUpdated;

   // One profiler for each time category.
   StatsProfiler userTime;
   StatsProfiler systemTime;
   StatsProfiler processTime;
   StatsProfiler wallTime;

   // Then one profiler for each frontend statistic.
#define FRONTEND_STATISTIC(TY, NAME) StatsProfiler NAME;
#include "polarphp/basic/StatisticsDef.h"
#undef FRONTEND_STATISTIC

   StatsProfilers()
      : lastUpdated(llvm::TimeRecord::getCurrentTime())
   {}
};

UnifiedStatsReporter::UnifiedStatsReporter(StringRef programName,
                                           StringRef moduleName,
                                           StringRef inputName,
                                           StringRef tripleName,
                                           StringRef outputType,
                                           StringRef optType,
                                           StringRef directory,
                                           SourceManager *sm,
                                           clang::SourceManager *csm,
                                           bool traceEvents,
                                           bool profileEvents,
                                           bool profileEntities)
   : UnifiedStatsReporter(programName,
                          aux_name(moduleName,
                                   inputName,
                                   tripleName,
                                   outputType,
                                   optType),
                          directory,
                          sm, csm,
                          traceEvents, profileEvents, profileEntities)
{
}

UnifiedStatsReporter::UnifiedStatsReporter(StringRef programName,
                                           StringRef auxname,
                                           StringRef directory,
                                           SourceManager *sm,
                                           clang::SourceManager *csm,
                                           bool traceEvents,
                                           bool profileEvents,
                                           bool profileEntities)
   : m_currentProcessExitStatusSet(false),
     m_currentProcessExitStatus(EXIT_FAILURE),
     m_statsFilename(directory),
     m_traceFilename(directory),
     m_profileDirname(directory),
     m_startedTime(llvm::TimeRecord::getCurrentTime()),
     m_mainThreadID(std::this_thread::get_id()),
     m_timer(std::make_unique<NamedRegionTimer>(auxname,
                                                "Building Target",
                                                programName, "Running Program")),
     m_sourceMgr(sm),
     m_clangSourceMgr(csm),
     m_recursiveTimers(std::make_unique<RecursionSafeTimers>())
{
   path::append(m_statsFilename, makeStatsFileName(programName, auxname));
   path::append(m_traceFilename, makeTraceFileName(programName, auxname));
   path::append(m_profileDirname, makeProfileDirName(programName, auxname));
   llvm::EnableStatistics(/*PrintOnExit=*/false);
   SharedTimer::enableCompilationTimers();
   if (traceEvents || profileEvents || profileEntities) {
      m_lastTracedFrontendCounters.emplace();
   }
   if (traceEvents) {
      m_frontendStatsEvents.emplace();
   }
   if (profileEvents) {
      m_eventProfilers = std::make_unique<StatsProfilers>();
   }
   if (profileEntities) {
      m_entityProfilers = std::make_unique<StatsProfilers>();
   }
}

UnifiedStatsReporter::AlwaysOnDriverCounters &
UnifiedStatsReporter::getDriverCounters()
{
   if (!m_driverCounters) {
      m_driverCounters.emplace();
   }
   return *m_driverCounters;
}

UnifiedStatsReporter::AlwaysOnFrontendCounters &
UnifiedStatsReporter::getFrontendCounters()
{
   if (!m_frontendCounters) {
      m_frontendCounters.emplace();
   }
   return *m_frontendCounters;
}

void
UnifiedStatsReporter::noteCurrentProcessExitStatus(int status)
{
   assert(m_mainThreadID == std::this_thread::get_id());
   assert(!m_currentProcessExitStatusSet);
   m_currentProcessExitStatusSet = true;
   m_currentProcessExitStatus = status;
}

void
UnifiedStatsReporter::publishAlwaysOnStatsToLLVM()
{
   if (m_frontendCounters) {
      auto &C = getFrontendCounters();
#define FRONTEND_STATISTIC(TY, NAME)                            \
   do {                                                        \
   static llvm::Statistic Stat(#TY, #NAME, #NAME);  \
   Stat += (C).NAME;                                         \
   } while (0);
#include "polarphp/basic/StatisticsDef.h"
#undef FRONTEND_STATISTIC
   }
   if (m_driverCounters) {
      auto &C = getDriverCounters();
#define DRIVER_STATISTIC(NAME)                                       \
   do {                                                             \
   static llvm::Statistic Stat("Driver", #NAME, #NAME);  \
   Stat += (C).NAME;                                              \
   } while (0);
#include "polarphp/basic/StatisticsDef.h"
#undef DRIVER_STATISTIC
   }
}

void
UnifiedStatsReporter::printAlwaysOnStatsAndTimers(raw_ostream &ostream)
{
   // Adapted from llvm::PrintStatisticsJSON
   ostream << "{\n";
   const char *delim = "";
   if (m_frontendCounters) {
      auto &C = getFrontendCounters();
#define FRONTEND_STATISTIC(TY, NAME)                        \
   do {                                                    \
   ostream << delim << "\t\"" #TY "." #NAME "\": " << C.NAME; \
   delim = ",\n";                                        \
   } while (0);
#include "polarphp/basic/StatisticsDef.h"
#undef FRONTEND_STATISTIC
   }
   if (m_driverCounters) {
      auto &C = getDriverCounters();
#define DRIVER_STATISTIC(NAME)                              \
   do {                                                    \
   ostream << delim << "\t\"Driver." #NAME "\": " << C.NAME;  \
   delim = ",\n";                                        \
   } while (0);
#include "polarphp/basic/StatisticsDef.h"
#undef DRIVER_STATISTIC
   }
   // Print timers.
   llvm::TimerGroup::printAllJSONValues(ostream, delim);
   llvm::TimerGroup::clearAll();
   ostream << "\n}\n";
   ostream.flush();
}

FrontendStatsTracer::FrontendStatsTracer(
      UnifiedStatsReporter *reporter, StringRef eventName, const void *entity,
      const UnifiedStatsReporter::TraceFormatter *formatter)
   : reporter(reporter), savedTime(), eventName(eventName), entity(entity),
     formatter(formatter)
{
   if (reporter) {
      savedTime = llvm::TimeRecord::getCurrentTime();
      reporter->saveAnyFrontendStatsEvents(*this, true);
   }
}

FrontendStatsTracer::FrontendStatsTracer() = default;

FrontendStatsTracer&
FrontendStatsTracer::operator=(FrontendStatsTracer&& other)
{
   reporter = other.reporter;
   savedTime = other.savedTime;
   eventName = other.eventName;
   entity = other.entity;
   formatter = other.formatter;
   other.reporter = nullptr;
   return *this;
}

FrontendStatsTracer::FrontendStatsTracer(FrontendStatsTracer&& other)
   : reporter(other.reporter),
     savedTime(other.savedTime),
     eventName(other.eventName),
     entity(other.entity),
     formatter(other.formatter)
{
   other.reporter = nullptr;
}

FrontendStatsTracer::~FrontendStatsTracer()
{
   if (reporter) {
      reporter->saveAnyFrontendStatsEvents(*this, false);
   }
}

// Copy any interesting process-wide resource accounting stats to
// associated fields in the provided AlwaysOnFrontendCounters.
void updateProcessWideFrontendCounters(
      UnifiedStatsReporter::AlwaysOnFrontendCounters &C)
{
#if defined(HAVE_PROC_PID_RUSAGE) && defined(RUSAGE_INFO_V4)
   struct rusage_info_v4 ru;
   if (0 == proc_pid_rusage(getpid(), RUSAGE_INFO_V4, reinterpret_cast<rusage_info_t *>(&ru))) {
      C.NumInstructionsExecuted = ru.ri_instructions;
   }
#endif

#if defined(HAVE_MALLOC_ZONE_STATISTICS) && defined(HAVE_MALLOC_MALLOC_H)
   // On Darwin we have a lifetime max that's maintained by malloc we can
   // just directly query, even if we only make one query on shutdown.
   malloc_statistics_t stats;
   malloc_zone_statistics(malloc_default_zone(), &stats);
   C.MaxMallocUsage = static_cast<int64_t>(stats.max_size_in_use);
#else
   // If we don't have a malloc-tracked max-usage counter, we have to rely
   // on taking the max over current-usage samples while running and hoping
   // we get called often enough. This will happen when profiling/tracing,
   // but not while doing single-query-on-shutdown collection.
   C.MaxMallocUsage = std::max(C.MaxMallocUsage,
                               (int64_t)llvm::sys::Process::GetMallocUsage());
#endif
}

static inline void
saveEvent(StringRef statName,
          int64_t curr, int64_t last,
          uint64_t NowUS, uint64_t liveUS,
          std::vector<UnifiedStatsReporter::FrontendStatsEvent> &Events,
          FrontendStatsTracer const& T,
          bool isEntry)
{
   int64_t delta = curr - last;
   if (delta != 0) {
      Events.emplace_back(UnifiedStatsReporter::FrontendStatsEvent{
                             NowUS, liveUS, isEntry, T.eventName, statName, delta, curr,
                             T.entity, T.formatter});
   }
}

void
UnifiedStatsReporter::saveAnyFrontendStatsEvents(
      FrontendStatsTracer const& T,
      bool isEntry)
{
   assert(m_mainThreadID == std::this_thread::get_id());
   // First make a note in the recursion-safe timers; these
   // are active anytime UnifiedStatsReporter is active.
   if (isEntry) {
      m_recursiveTimers->beginTimer(T.eventName);
   } else {
      m_recursiveTimers->endTimer(T.eventName);
   }

   // If we don't have a saved entry to form deltas against in the trace buffer
   // or profilers, we're not tracing or profiling: return early.
   if (!m_lastTracedFrontendCounters) {
      return;
   }

   auto now = llvm::TimeRecord::getCurrentTime();
   auto &curr = getFrontendCounters();
   auto &last = *m_lastTracedFrontendCounters;
   updateProcessWideFrontendCounters(curr);
   if (m_eventProfilers) {
      auto timeDelta = now;
      timeDelta -= m_eventProfilers->lastUpdated;
      m_eventProfilers->userTime.profileEvent(T.eventName,
                                              timeDelta.getUserTime(),
                                              isEntry);
      m_eventProfilers->systemTime.profileEvent(T.eventName,
                                                timeDelta.getSystemTime(),
                                                isEntry);
      m_eventProfilers->processTime.profileEvent(T.eventName,
                                                 timeDelta.getProcessTime(),
                                                 isEntry);
      m_eventProfilers->wallTime.profileEvent(T.eventName,
                                              timeDelta.getWallTime(),
                                              isEntry);
#define FRONTEND_STATISTIC(TY, N)                                       \
   m_eventProfilers->N.profileEvent(T.eventName, curr.N - last.N, isEntry);
#include "polarphp/basic/StatisticsDef.h"
#undef FRONTEND_STATISTIC
      m_eventProfilers->lastUpdated = now;
   }

   if (m_entityProfilers) {
      auto timeDelta = now;
      timeDelta -= m_entityProfilers->lastUpdated;
      m_entityProfilers->userTime.profileEvent(T.eventName,
                                               timeDelta.getUserTime(),
                                               isEntry, T.entity, T.formatter);
      m_entityProfilers->systemTime.profileEvent(T.eventName,
                                                 timeDelta.getSystemTime(),
                                                 isEntry, T.entity, T.formatter);
      m_entityProfilers->processTime.profileEvent(T.eventName,
                                                  timeDelta.getProcessTime(),
                                                  isEntry, T.entity, T.formatter);
      m_entityProfilers->wallTime.profileEvent(T.eventName,
                                               timeDelta.getWallTime(),
                                               isEntry, T.entity, T.formatter);
#define FRONTEND_STATISTIC(TY, N)                                          \
   m_entityProfilers->N.profileEvent(T.eventName, curr.N - last.N, isEntry, \
   T.entity, T.formatter);
#include "polarphp/basic/StatisticsDef.h"
#undef FRONTEND_STATISTIC
      m_entityProfilers->lastUpdated = now;
   }

   if (m_frontendStatsEvents) {
      auto StartUS = uint64_t(1000000.0 * T.savedTime.getProcessTime());
      auto NowUS = uint64_t(1000000.0 * now.getProcessTime());
      auto liveUS = isEntry ? 0 : NowUS - StartUS;
      auto &Events = *m_frontendStatsEvents;
#define FRONTEND_STATISTIC(TY, N)                                       \
   saveEvent(#TY "." #N, curr.N, last.N, NowUS, liveUS, Events, T, isEntry);
#include "polarphp/basic/StatisticsDef.h"
#undef FRONTEND_STATISTIC
   }

   // Save all counters (changed or otherwise).
   last = curr;
}

UnifiedStatsReporter::TraceFormatter::~TraceFormatter() {}

UnifiedStatsReporter::~UnifiedStatsReporter()
{
   assert(m_mainThreadID == std::this_thread::get_id());
   // If nobody's marked this process as successful yet,
   // mark it as failing.
   if (m_currentProcessExitStatus != EXIT_SUCCESS) {
      if (m_frontendCounters) {
         auto &C = getFrontendCounters();
         C.NumProcessFailures++;
      } else {
         auto &C = getDriverCounters();
         C.NumProcessFailures++;
      }
   }

   if (m_frontendCounters)
      updateProcessWideFrontendCounters(getFrontendCounters());

   // NB: timer needs to be Optional<> because it needs to be destructed early;
   // LLVM will complain about double-stopping a timer if you tear down a
   // NamedRegionTimer after printing all timers. The printing routines were
   // designed with more of a global-scope, run-at-process-exit in mind, which
   // we're repurposing a bit here.
   m_timer.reset();

   // We currently do this by manual TimeRecord keeping because LLVM has decided
   // not to allow access to the m_timers inside NamedRegionTimers.
   auto elapsedTime = llvm::TimeRecord::getCurrentTime();
   elapsedTime -= m_startedTime;

   if (m_driverCounters) {
      auto &C = getDriverCounters();
      C.ChildrenMaxRSS = get_children_max_resident_set_size();
   }

   if (m_frontendCounters) {
      auto &C = getFrontendCounters();
      // Convenience calculation for crude top-level "absolute speed".
      if (C.NumSourceLines != 0 && elapsedTime.getProcessTime() != 0.0)
         C.NumSourceLinesPerSecond = static_cast<int64_t>(((static_cast<double>(C.NumSourceLines)) /
                                                           elapsedTime.getProcessTime()));
   }

   std::error_code errorCode;
   raw_fd_ostream ostream(m_statsFilename, errorCode, fs::F_Append | fs::F_Text);
   if (errorCode) {
      llvm::errs() << "Error opening -stats-output-dir file '"
                   << m_statsFilename << "' for writing\n";
      return;
   }

   // We change behavior here depending on whether -DLLVM_ENABLE_STATS and/or
   // assertions were on in this build; this is somewhat subtle, but turning on
   // all stats for all of LLVM and clang is a bit more expensive and intrusive
   // than we want to be in release builds.
   //
   //  - If enabled: we copy all of our "always-on" local stats into LLVM's
   //    global statistics list, and ask LLVM to manage the printing of them.
   //
   //  - If disabled: we still have our "always-on" local stats to write, and
   //    LLVM's global _timers_ were still enabled (they're runtime-enabled, not
   //    compile-time) so we sequence printing our own stats and LLVM's timers
   //    manually.

#if !defined(NDEBUG) || defined(LLVM_ENABLE_STATS)
   publishAlwaysOnStatsToLLVM();
   PrintStatisticsJSON(ostream);
   llvm::TimerGroup::clearAll();
#else
   printAlwaysOnStatsAndTimers(ostream);
#endif
   flushTracesAndProfiles();
}

void
UnifiedStatsReporter::flushTracesAndProfiles()
{
   if (m_frontendStatsEvents && m_sourceMgr) {
      std::error_code errorCode;
      raw_fd_ostream tstream(m_traceFilename, errorCode, fs::F_Append | fs::F_Text);
      if (errorCode) {
         llvm::errs() << "Error opening -trace-stats-events file '"
                      << m_traceFilename << "' for writing\n";
         return;
      }
      tstream << "Time,Live,isEntry,eventName,counterName,"
              << "counterDelta,counterValue,EntityName,EntityRange\n";
      for (auto const &E : *m_frontendStatsEvents) {
         tstream << E.timeUSec << ','
                 << E.liveUSec << ','
                 << (E.isEntry ? "\"entry\"," : "\"exit\",")
                 << '"' << E.eventName << '"' << ','
                 << '"' << E.counterName << '"' << ','
                 << E.counterDelta << ','
                 << E.counterValue << ',';
         tstream << '"';
         if (E.formatter)
            E.formatter->traceName(E.entity, tstream);
         tstream << '"' << ',';
         tstream << '"';
         if (E.formatter)
            E.formatter->traceLoc(E.entity, m_sourceMgr, m_clangSourceMgr, tstream);
         tstream << '"' << '\n';
      }
   }

   if (m_eventProfilers || m_entityProfilers) {
      std::error_code errorCode = llvm::sys::fs::create_directories(m_profileDirname);
      if (errorCode) {
         llvm::errs() << "Failed to create directory '" << m_profileDirname << "': "
                      << errorCode.message() << "\n";
         return;
      }
      if (m_eventProfilers) {
         auto D = m_profileDirname;
         m_eventProfilers->userTime.printToFile(D, "Time.User.events");
         m_eventProfilers->systemTime.printToFile(D, "Time.System.events");
         m_eventProfilers->processTime.printToFile(D, "Time.Process.events");
         m_eventProfilers->wallTime.printToFile(D, "Time.Wall.events");
         #define FRONTEND_STATISTIC(TY, NAME)                                    \
            m_eventProfilers->NAME.printToFile(m_profileDirname,                  \
         #TY "." #NAME ".events");
         #include "polarphp/basic/StatisticsDef.h"
         #undef FRONTEND_STATISTIC
      }
      if (m_entityProfilers) {
         auto D = m_profileDirname;
         m_entityProfilers->userTime.printToFile(D, "Time.User.entities");
         m_entityProfilers->systemTime.printToFile(D, "Time.System.entities");
         m_entityProfilers->processTime.printToFile(D, "Time.Process.entities");
         m_entityProfilers->wallTime.printToFile(D, "Time.Wall.entities");
         #define FRONTEND_STATISTIC(TY, NAME)                                    \
            m_entityProfilers->NAME.printToFile(m_profileDirname,                 \
         #TY "." #NAME ".entities");
         #include "polarphp/basic/StatisticsDef.h"
         #undef FRONTEND_STATISTIC
      }
   }
   m_lastTracedFrontendCounters.reset();
   m_frontendStatsEvents.reset();
   m_eventProfilers.reset();
   m_entityProfilers.reset();
}

} // polar::basic
