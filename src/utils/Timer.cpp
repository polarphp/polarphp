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

#include "polarphp/utils/Timer.h"
#include "polarphp/basic/adt/Statistic.h"
#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/Process.h"
#include "polarphp/utils/yaml/YamlTraits.h"
#include "polarphp/utils/RawOutStream.h"
#include <limits>
#include <mutex>
#include <chrono>

namespace polar {
namespace utils {

namespace cmd = polar::cmd;
using polar::sys::Process;

// This ugly hack is brought to you courtesy of constructor/destructor ordering
// being unspecified by C++.  Basically the problem is that a Statistic object
// gets destroyed, which ends up calling 'GetLibSupportInfoOutputFile()'
// (below), which calls this function.  sg_libSupportsg_infooutputFilename used to be
// a global variable, but sometimes it would get destroyed before the Statistic,
// causing havoc to ensue.  We "fix" this by creating the string the first time
// it is needed and never destroying it.
static ManagedStatic<std::string> sg_libSupportsg_infooutputFilename;
static std::mutex sg_timerLock;

static std::string &get_lib_support_info_output_filename()
{
   return *sg_libSupportsg_infooutputFilename;
}

namespace {
static cmd::Opt<bool>
sg_trackSpace("track-memory", cmd::Desc("Enable -time-passes memory "
                                        "tracking (this may be slow)"),
              cmd::Hidden);

static cmd::Opt<std::string, true>
sg_infooutputFilename("info-output-file", cmd::ValueDesc("filename"),
                      cmd::Desc("File to append -stats and -timer output to"),
                      cmd::Hidden, cmd::location(get_lib_support_info_output_filename()));
}

} // utils

namespace basic {

using polar::utils::error_stream;
using polar::utils::get_lib_support_info_output_filename;

std::unique_ptr<RawFdOutStream> create_info_output_file()
{
   const std::string &outputFilename = get_lib_support_info_output_filename();
   if (outputFilename.empty()) {
      return std::make_unique<RawFdOutStream>(2, false); // stderr.
   }
   if (outputFilename == "-") {
      return std::make_unique<RawFdOutStream>(1, false); // stdout.
   }
   // Append mode is used because the info output file is opened and closed
   // each time -stats or -time-passes wants to print output to it. To
   // compensate for this, the test-suite Makefiles have code to delete the
   // info output file before running commands which write to it.
   std::error_code errorCode;
   auto result = std::make_unique<RawFdOutStream>(
            outputFilename, errorCode, polar::fs::F_Append | polar::fs::F_Text);
   if (!errorCode) {
      return result;
   }
   error_stream() << "Error opening info-output-file '"
                  << outputFilename << " for appending!\n";
   return std::make_unique<RawFdOutStream>(2, false); // stderr.
}

} // basic

namespace utils {

namespace {
struct CreateDefaultTimerGroup
{
   static void *call() {
      return new TimerGroup("misc", "Miscellaneous Ungrouped Timers");
   }
};
} // namespace
static ManagedStatic<TimerGroup, CreateDefaultTimerGroup> sg_defaultTimerGroup;
static TimerGroup *get_default_timer_group()
{
   return &*sg_defaultTimerGroup;
}

//===----------------------------------------------------------------------===//
// Timer Implementation
//===----------------------------------------------------------------------===//

void Timer::init(StringRef name, StringRef description)
{
   init(name, description, *get_default_timer_group());
}

void Timer::init(StringRef name, StringRef description, TimerGroup &tg)
{
   assert(!m_timerGroup && "Timer already initialized");
   this->m_name.assign(name.begin(), name.end());
   this->m_description.assign(description.begin(), description.end());
   m_running = m_triggered = false;
   m_timerGroup = &tg;
   m_timerGroup->addTimer(*this);
}

Timer::~Timer()
{
   if (!m_timerGroup) {
      return;  // Never initialized, or already cleared.
   }
   m_timerGroup->removeTimer(*this);
}

static inline size_t get_mem_usage()
{
   if (!sg_trackSpace) {
      return 0;
   }
   return Process::getMallocUsage();
}

TimeRecord TimeRecord::getCurrentTime(bool start)
{
   using Seconds = std::chrono::duration<double, std::ratio<1>>;
   TimeRecord result;
   sys::TimePoint<> now;
   std::chrono::nanoseconds user, sys;

   if (start) {
      result.m_memUsed = get_mem_usage();
      Process::getTimeUsage(now, user, sys);
   } else {
      sys::Process::getTimeUsage(now, user, sys);
      result.m_memUsed = get_mem_usage();
   }

   result.m_wallTime = Seconds(now.time_since_epoch()).count();
   result.m_userTime = Seconds(user).count();
   result.m_systemTime = Seconds(sys).count();
   return result;
}

void Timer::startTimer()
{
   assert(!m_running && "Cannot start a running timer");
   m_running = m_triggered = true;
   m_startTime = TimeRecord::getCurrentTime(true);
}

void Timer::stopTimer()
{
   assert(m_running && "Cannot stop a paused timer");
   m_running = false;
   m_time += TimeRecord::getCurrentTime(false);
   m_time -= m_startTime;
}

void Timer::clear()
{
   m_running = m_triggered = false;
   m_time = m_startTime = TimeRecord();
}

namespace {

void print_value(double value, double total, RawOutStream &outStream)
{
   if (total < 1e-7) {  // Avoid dividing by zero.
      outStream << "        -----     ";
   } else {
      outStream << format("  %7.4f (%5.1f%%)", value, value * 100 / total);
   }
}

} // anonymous namespace

void TimeRecord::print(const TimeRecord &total, RawOutStream &outStream) const
{
   if (total.getUserTime()) {
      print_value(getUserTime(), total.getUserTime(), outStream);
   }
   if (total.getSystemTime()) {
      print_value(getSystemTime(), total.getSystemTime(), outStream);
   }
   if (total.getProcessTime()) {
      print_value(getProcessTime(), total.getProcessTime(), outStream);
   }
   print_value(getWallTime(), total.getWallTime(), outStream);
   outStream << "  ";
   if (total.getMemUsed()) {
      outStream << format("%9" PRId64 "  ", (int64_t)getMemUsed());
   }
}


//===----------------------------------------------------------------------===//
//   NamedRegionTimer Implementation
//===----------------------------------------------------------------------===//

namespace {

typedef StringMap<Timer> Name2TimerMap;

class Name2PairMap
{
   StringMap<std::pair<TimerGroup*, Name2TimerMap>> m_map;
public:
   ~Name2PairMap()
   {
      for (StringMap<std::pair<TimerGroup*, Name2TimerMap>>::iterator
           iter = m_map.begin(), end = m_map.end(); iter != end; ++iter) {
         delete iter->m_second.first;
      }
   }

   Timer &get(StringRef name, StringRef description, StringRef groupName,
              StringRef groupDescription)
   {
      std::lock_guard lock(sg_timerLock);
      std::pair<TimerGroup*, Name2TimerMap> &groupEntry = m_map[groupName];
      if (!groupEntry.first) {
         groupEntry.first = new TimerGroup(groupName, groupDescription);
      }
      Timer &timer = groupEntry.second[name];
      if (!timer.isInitialized()) {
         timer.init(name, description, *groupEntry.first);
      }
      return timer;
   }
};

} // anonymous namespace

static ManagedStatic<Name2PairMap> sg_namedGroupedTimers;

NamedRegionTimer::NamedRegionTimer(StringRef name, StringRef description,
                                   StringRef groupName,
                                   StringRef groupDescription, bool enabled)
   : TimeRegion(!enabled ? nullptr
                         : &sg_namedGroupedTimers->get(name, description, groupName,
                                                       groupDescription))
{}

//===----------------------------------------------------------------------===//
//   TimerGroup Implementation
//===----------------------------------------------------------------------===//

/// This is the global list of TimerGroups, maintained by the TimerGroup
/// ctor/dtor and is protected by the TimerLock lock.
static TimerGroup *sg_timerGroupList = nullptr;

TimerGroup::TimerGroup(StringRef name, StringRef description)
   : m_name(name.begin(), name.end()),
     m_description(description.begin(), description.end())
{
   // Add the group to sg_timerGroupList.
   std::lock_guard lock(sg_timerLock);
   if (sg_timerGroupList) {
      sg_timerGroupList->m_prev = &m_next;
   }
   m_next = sg_timerGroupList;
   m_prev = &sg_timerGroupList;
   sg_timerGroupList = this;
}

TimerGroup::TimerGroup(StringRef name, StringRef description,
                       const StringMap<TimeRecord> &records)
   : TimerGroup(name, description)
{
   m_timersToPrint.reserve(records.getSize());
   for (const auto &record : records) {
      m_timersToPrint.emplace_back(record.getValue(), record.getKey(), record.getKey());
   }
   assert(m_timersToPrint.size() == records.getSize() && "Size mismatch");
}

TimerGroup::~TimerGroup()
{
   // If the timer group is destroyed before the timers it owns, accumulate and
   // print the timing data.
   while (m_firstTimer) {
      removeTimer(*m_firstTimer);
   }
   // Remove the group from the sg_timerGroupList.
   std::lock_guard lock(sg_timerLock);
   *m_prev = m_next;
   if (m_next) {
      m_next->m_prev = m_prev;
   }
}

void TimerGroup::removeTimer(Timer &timer)
{
   std::lock_guard lock(sg_timerLock);

   // If the timer was started, move its data to TimersToPrint.
   if (timer.hasTriggered()) {
      m_timersToPrint.emplace_back(timer.m_time, timer.m_name, timer.m_description);
   }
   timer.m_timerGroup = nullptr;

   // Unlink the timer from our list.
   *timer.m_prev = timer.m_next;
   if (timer.m_next) {
      timer.m_next->m_prev = timer.m_prev;
   }
   // Print the report when all timers in this group are destroyed if some of
   // them were started.
   if (m_firstTimer || m_timersToPrint.empty()) {
      return;
   }
   std::unique_ptr<RawOutStream> outStream = polar::basic::create_info_output_file();
   printQueuedTimers(*outStream);
}

void TimerGroup::addTimer(Timer &timer)
{
   std::lock_guard lock(sg_timerLock);
   // Add the timer to our list.
   if (m_firstTimer) {
      m_firstTimer->m_prev = &timer.m_next;
   }
   timer.m_next = m_firstTimer;
   timer.m_prev = &m_firstTimer;
   m_firstTimer = &timer;
}

void TimerGroup::printQueuedTimers(RawOutStream &outstream)
{
   // Sort the timers in descending order by amount of time taken.
   polar::basic::sort(m_timersToPrint);
   TimeRecord total;
   for (const PrintRecord &record : m_timersToPrint) {
      total += record.m_time;
   }
   // Print out timing header.
   outstream << "===" << std::string(73, '-') << "===\n";
   // Figure out how many spaces to indent TimerGroup name.
   unsigned padding = (80-m_description.length())/2;
   if (padding > 80) {
      padding = 0;         // Don't allow "negative" numbers
   }
   outstream.indent(padding) << m_description << '\n';
   outstream << "===" << std::string(73, '-') << "===\n";

   // If this is not an collection of ungrouped times, print the total time.
   // Ungrouped timers don't really make sense to add up.  We still print the
   // TOTAL line to make the percentages make sense.
   if (this != get_default_timer_group()) {
      outstream << format("  Total Execution Time: %5.4f seconds (%5.4f wall clock)\n",
                          total.getProcessTime(), total.getWallTime());
   }
   outstream << '\n';
   if (total.getUserTime()) {
      outstream << "   ---User Time---";
   }
   if (total.getSystemTime()) {
      outstream << "   --System Time--";
   }
   if (total.getProcessTime()) {
      outstream << "   --User+System--";
   }
   outstream << "   ---Wall Time---";
   if (total.getMemUsed()) {
      outstream << "  ---Mem---";
   }
   outstream << "  --- Name ---\n";
   // Loop through all of the timing data, printing it out.
   for (const PrintRecord &record : polar::basic::make_range(m_timersToPrint.rbegin(),
                                                             m_timersToPrint.rend())) {
      record.m_time.print(total, outstream);
      outstream << record.m_description << '\n';
   }

   total.print(total, outstream);
   outstream << "Total\n\n";
   outstream.flush();
   m_timersToPrint.clear();
}

void TimerGroup::prepareToPrintList()
{
   // See if any of our timers were started, if so add them to TimersToPrint.
   for (Timer *timer = m_firstTimer; timer; timer = timer->m_next) {
      if (!timer->hasTriggered()) continue;
      bool wasRunning = timer->isRunning();
      if (wasRunning) {
         timer->stopTimer();
      }
      m_timersToPrint.emplace_back(timer->m_time, timer->m_name, timer->m_description);
      if (wasRunning) {
         timer->startTimer();
      }
   }
}

void TimerGroup::print(RawOutStream &outstream)
{
   std::lock_guard lock(sg_timerLock);
   prepareToPrintList();
   // If any timers were started, print the group.
   if (!m_timersToPrint.empty()) {
      printQueuedTimers(outstream);
   }
}

void TimerGroup::clear()
{
   std::lock_guard lock(sg_timerLock);
   for (Timer *timer = m_firstTimer; timer; timer = timer->m_next) {
      timer->clear();
   }
}


void TimerGroup::printAll(RawOutStream &outstream)
{
   std::lock_guard lock(sg_timerLock);
   for (TimerGroup *timerGroup = sg_timerGroupList; timerGroup; timerGroup = timerGroup->m_next) {
      timerGroup->print(outstream);
   }
}

void TimerGroup::clearAll()
{
   std::lock_guard lock(sg_timerLock);
   for (TimerGroup *timerGroup = sg_timerGroupList; timerGroup; timerGroup = timerGroup->m_next) {
      timerGroup->clear();
   }
}

void TimerGroup::printJSONValue(RawOutStream &outstream, const PrintRecord &record,
                                const char *suffix, double value)
{
   assert(yaml::needs_quotes(m_name) == yaml::QuotingType::None &&
          "TimerGroup name should not need quotes");
   assert(yaml::needs_quotes(record.m_name) == yaml::QuotingType::None &&
          "Timer name should not need quotes");
   constexpr auto max_digits10 = std::numeric_limits<double>::max_digits10;
   outstream << "\t\"time." << m_name << '.' << record.m_name << suffix
             << "\": " << format("%.*e", max_digits10 - 1, value);
}

const char *TimerGroup::printJSONValues(RawOutStream &outstream, const char *delim)
{
   std::lock_guard lock(sg_timerLock);
   prepareToPrintList();
   for (const PrintRecord &record : m_timersToPrint) {
      outstream << delim;
      delim = ",\n";

      const TimeRecord &timeRecord = record.m_time;
      printJSONValue(outstream, record, ".wall", timeRecord.getWallTime());
      outstream << delim;
      printJSONValue(outstream, record, ".user", timeRecord.getUserTime());
      outstream << delim;
      printJSONValue(outstream, record, ".sys", timeRecord.getSystemTime());
      if (timeRecord.getMemUsed()) {
         outstream << delim;
         printJSONValue(outstream, record, ".mem", timeRecord.getMemUsed());
      }
   }
   m_timersToPrint.clear();
   return delim;
}

const char *TimerGroup::printAllJSONValues(RawOutStream &outstream, const char *delim)
{
   std::lock_guard lock(sg_timerLock);
   for (TimerGroup *timerGroup = sg_timerGroupList; timerGroup; timerGroup = timerGroup->m_next) {
      delim = timerGroup->printJSONValues(outstream, delim);
   }
   return delim;
}

void TimerGroup::constructTimerLists()
{
   (void)*sg_namedGroupedTimers;
}

} // utils
} // polar
