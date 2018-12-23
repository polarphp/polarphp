// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/26.
//===----------------------------------------------------------------------===//
//
// This file implements the 'Statistic' class, which is designed to be an easy
// way to expose various success metrics from passes.  These statistics are
// printed at the end of a run, when the -stats command line option is enabled
// on the command line.
//
// This is useful for reporting information like the number of instructions
// simplified, optimized or removed by various transformations, like this:
//
// static Statistic NumInstEliminated("GCSE", "Number of instructions killed");
//
// Later, in the code: ++NumInstEliminated;
//
//===----------------------------------------------------------------------===//

#include "polarphp/basic/adt/Statistic.h"
#include "polarphp/basic/adt/StringExtras.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/Timer.h"
#include "polarphp/utils/yaml/YamlTraits.h"
#include "polarphp/utils/RawOutStream.h"
#include <algorithm>
#include <cstring>
#include <mutex>

namespace polar {
namespace basic {

namespace cmd = polar::cmd;
using polar::utils::TimerGroup;
using polar::utils::ManagedStatic;

namespace {
/// -stats - Command line option to cause transformations to emit stats about
/// what they did.
///
cmd::Opt<bool> sg_stats(
      "stats",
      cmd::Desc("Enable statistics output from program (available with Asserts)"),
      cmd::Hidden);

cmd::Opt<bool> sg_statsAsJson("stats-json",
                              cmd::Desc("Display statistics as json data"),
                              cmd::Hidden);
} // anonymous namespace

static bool sg_enabled;
static bool sg_printOnExit;

namespace {
/// This class is used in a ManagedStatic so that it is created on demand (when
/// the first statistic is bumped) and destroyed only when polar_shutdown is
/// called. We print statistics from the destructor.
/// This class is also used to look up statistic values from applications that
/// use polarphp.
class StatisticInfo
{
   std::vector<Statistic*> m_stats;

   friend void polar::basic::print_statistics();
   friend void polar::basic::print_statistics(RawOutStream &outStream);
   friend void polar::basic::print_statistics_json(RawOutStream &outStream);

   /// Sort statistics by debugtype,name,description.
   void sort();
public:
   using const_iterator = std::vector<Statistic *>::const_iterator;

   StatisticInfo();
   ~StatisticInfo();

   void addStatistic(Statistic *stat)
   {
      m_stats.push_back(stat);
   }

   const_iterator begin() const
   {
      return m_stats.begin();
   }

   const_iterator end() const
   {
      return m_stats.end();
   }

   IteratorRange<const_iterator> getStatistics() const
   {
      return {begin(), end()};
   }

   void reset();
};
} // end anonymous namespace

static ManagedStatic<StatisticInfo> sg_statInfo;
static ManagedStatic<std::mutex> sg_statLock;

/// RegisterStatistic - The first time a statistic is bumped, this method is
/// called.
void Statistic::registerStatistic()
{
   // If stats are enabled, inform sg_statInfo that this statistic should be
   // printed.
   // polar_shutdown calls destructors while holding the ManagedStatic mutex.
   // These destructors end up calling print_statistics, which takes sg_statLock.
   // Since dereferencing sg_statInfo and sg_statLock can require taking the
   // ManagedStatic mutex, doing so with sg_statLock held would lead to a lock
   // order inversion. To avoid that, we dereference the ManagedStatics first,
   // and only take sg_statLock afterwards.
   if (!m_initialized.load(std::memory_order_relaxed)) {
      StatisticInfo &sinfo = *sg_statInfo;
      std::lock_guard Writer(*sg_statLock);
      // Check m_initialized again after acquiring the lock.
      if (m_initialized.load(std::memory_order_relaxed)) {
         return;
      }
      if (sg_stats || sg_enabled) {
         sinfo.addStatistic(this);
      }
      // Remember we have been registered.
      m_initialized.store(true, std::memory_order_release);
   }
}

StatisticInfo::StatisticInfo()
{
   // Ensure timergroup lists are created first so they are destructed after us.
   TimerGroup::constructTimerLists();
}

// Print information when destroyed, iff command line option is specified.
StatisticInfo::~StatisticInfo()
{
   if (sg_stats || sg_printOnExit) {
      print_statistics();
   }
}

void enable_statistics(bool printOnExit)
{
   sg_enabled = true;
   sg_printOnExit = printOnExit;
}

bool are_statisticssg_enabled()
{
   return sg_enabled || sg_stats;
}

void StatisticInfo::sort()
{
   std::stable_sort(m_stats.begin(), m_stats.end(),
                    [](const Statistic *lhs, const Statistic *rhs) {
      if (int cmp = std::strcmp(lhs->getDebugType(), rhs->getDebugType())) {
         return cmp < 0;
      }
      if (int cmp = std::strcmp(lhs->getName(), rhs->getName())) {
         return cmp < 0;
      }
      return std::strcmp(lhs->getDesc(), rhs->getDesc()) < 0;
   });
}

void StatisticInfo::reset()
{
   std::lock_guard writer(*sg_statLock);

   // Tell each statistic that it isn't registered so it has to register
   // again. We're holding the lock so it won't be able to do so until we're
   // finished. Once we've forced it to re-register (after we return), then zero
   // the value.
   for (auto *stat : m_stats) {
      // Value updates to a statistic that complete before this statement in the
      // iteration for that statistic will be lost as intended.
      stat->m_initialized = false;
      stat->m_value = 0;
   }

   // Clear the registration list and release the lock once we're done. Any
   // pending updates from other threads will safely take effect after we return.
   // That might not be what the user wants if they're measuring a compilation
   // but it's their responsibility to prevent concurrent compilations to make
   // a single compilation measurable.
   m_stats.clear();
}

void print_statistics(RawOutStream &outStream)
{
   StatisticInfo &stats = *sg_statInfo;

   // Figure out how long the biggest Value and Name fields are.
   unsigned maxDebugTypeLen = 0, maxValLen = 0;
   for (size_t i = 0, e = stats.m_stats.size(); i != e; ++i) {
      maxValLen = std::max(maxValLen,
                           (unsigned)polar::basic::utostr(stats.m_stats[i]->getValue()).size());
      maxDebugTypeLen = std::max(maxDebugTypeLen,
                                 (unsigned)std::strlen(stats.m_stats[i]->getDebugType()));
   }

   stats.sort();

   // Print out the statistics header...
   outStream << "===" << std::string(73, '-') << "===\n"
             << "                          ... Statistics Collected ...\n"
             << "===" << std::string(73, '-') << "===\n\n";

   // Print all of the statistics.
   for (size_t i = 0, e = stats.m_stats.size(); i != e; ++i)
      outStream << polar::utils::format("%*u %-*s - %s\n",
                                        maxValLen, stats.m_stats[i]->getValue(),
                                        maxDebugTypeLen, stats.m_stats[i]->getDebugType(),
                                        stats.m_stats[i]->getDesc());

   outStream << '\n';  // Flush the output stream.
   outStream.flush();
}

void print_statistics_json(RawOutStream &outStream)
{
   std::lock_guard reader(*sg_statLock);
   StatisticInfo &stats = *sg_statInfo;

   stats.sort();
   // Print all of the statistics.
   outStream << "{\n";
   const char *delim = "";
   for (const Statistic *stat : stats.m_stats) {
      outStream << delim;
      assert(yaml::needs_quotes(stat->getDebugType()) == yaml::QuotingType::None &&
             "Statistic group/type name is simple.");
      assert(yaml::needs_quotes(stat->getName()) == yaml::QuotingType::None &&
             "Statistic name is simple");
      outStream << "\t\"" << stat->getDebugType() << '.' << stat->getName() << "\": "
                << stat->getValue();
      delim = ",\n";
   }
   // Print timers.
   TimerGroup::printAllJSONValues(outStream, delim);

   outStream << "\n}\n";
   outStream.flush();
}

void print_statistics()
{
#if POLAR_ENABLE_STATS
   std::lock_guard reader(*sg_statLock);
   StatisticInfo &stats = *sg_statInfo;

   // Statistics not enabled?
   if (stats.m_stats.empty()) {
      return;
   }

   // Get the stream to write to.
   std::unique_ptr<RawOutStream> outStream = create_info_output_file();
   if (sg_statsAsJson) {
      print_statistics_json(*outStream);
   } else {
      print_statistics(*outStream);
   }
#else
   // Check if the -stats option is set instead of checking
   // !Stats.m_stats.empty().  In release builds, Statistics operators
   // do nothing, so stats are never Registered.
   if (stats) {
      // Get the stream to write to.
      std::unique_ptr<RawOutStream> outStream = create_info_output_file();
      (*outStream) << "Statistics are disabled.  "
                   << "Build with asserts or with -DPOLAR_ENABLE_STATS\n";
   }
#endif
}

const std::vector<std::pair<StringRef, unsigned>> get_statistics()
{
   std::lock_guard reader(*sg_statLock);
   std::vector<std::pair<StringRef, unsigned>> returnStats;
   for (const auto &stat : sg_statInfo->getStatistics()) {
      returnStats.emplace_back(stat->getName(), stat->getValue());
   }
   return returnStats;
}

void reset_statistics()
{
   sg_statInfo->reset();
}

} // basic
} // polar
