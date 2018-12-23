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

#ifndef POLARPHP_BASIC_ADT_STATICS_H
#define POLARPHP_BASIC_ADT_STATICS_H

#include "polarphp/global/Global.h"
#include <atomic>
#include <memory>
#include <vector>

// Determine whether statistics should be enabled. We must do it here rather
// than in CMake because multi-config generators cannot determine this at
// configure time.
#if !defined(NDEBUG) || POLAR_FORCE_ENABLE_STATS
#define POLAR_ENABLE_STATS 1
#endif

namespace polar {

// forward declare class with namespace
namespace utils {
class RawOutStream;
class RawFdOutStream;
} // utils

namespace basic {

class StringRef;
using polar::utils::RawOutStream;
using polar::utils::RawFdOutStream;

class Statistic
{
public:
   const char *m_debugType;
   const char *m_name;
   const char *m_desc;
   std::atomic<unsigned> m_value;
   std::atomic<bool> m_initialized;

   unsigned getValue() const
   {
      return m_value.load(std::memory_order_relaxed);
   }

   const char *getDebugType() const
   {
      return m_debugType;
   }

   const char *getName() const
   {
      return m_name;
   }

   const char *getDesc() const
   {
      return m_desc;
   }

   /// construct - This should only be called for non-global statistics.
   void construct(const char *debugtype, const char *name, const char *desc)
   {
      m_debugType = debugtype;
      m_name = name;
      m_desc = desc;
      m_value = 0;
      m_initialized = false;
   }

   // Allow use of this class as the value itself.
   operator unsigned() const
   {
      return getValue();
   }

#if POLAR_ENABLE_STATS
   const Statistic &operator=(unsigned value)
   {
      m_value.store(value, std::memory_order_relaxed);
      return init();
   }

   const Statistic &operator++()
   {
      m_value.fetch_add(1, std::memory_order_relaxed);
      return init();
   }

   unsigned operator++(int)
   {
      init();
      return m_value.fetch_add(1, std::memory_order_relaxed);
   }

   const Statistic &operator--()
   {
      m_value.fetch_sub(1, std::memory_order_relaxed);
      return init();
   }

   unsigned operator--(int)
   {
      init();
      return m_value.fetch_sub(1, std::memory_order_relaxed);
   }

   const Statistic &operator+=(unsigned value)
   {
      if (value == 0) {
         return *this;
      }
      m_value.fetch_add(value, std::memory_order_relaxed);
      return init();
   }

   const Statistic &operator-=(unsigned value)
   {
      if (value == 0) {
         return *this;
      }
      m_value.fetch_sub(value, std::memory_order_relaxed);
      return init();
   }

   void updateMax(unsigned value)
   {
      unsigned prevMax = m_value.load(std::memory_order_relaxed);
      // Keep trying to update max until we succeed or another thread produces
      // a bigger max than us.
      while (value > prevMax && !m_value.compare_exchange_weak(
                prevMax, value, std::memory_order_relaxed)) {
      }
      init();
   }

#else  // Statistics are disabled in release builds.

   const Statistic &operator=(unsigned value)
   {
      return *this;
   }

   const Statistic &operator++()
   {
      return *this;
   }

   unsigned operator++(int)
   {
      return 0;
   }

   const Statistic &operator--()
   {
      return *this;
   }

   unsigned operator--(int)
   {
      return 0;
   }

   const Statistic &operator+=(const unsigned &value)
   {
      return *this;
   }

   const Statistic &operator-=(const unsigned &value)
   {
      return *this;
   }

   void updateMax(unsigned value)
   {}

#endif  // POLAR_ENABLE_STATS

protected:
   Statistic &init()
   {
      if (!m_initialized.load(std::memory_order_acquire)) {
         registerStatistic();
      }
      return *this;
   }

   void registerStatistic();
};

// STATISTIC - A macro to make definition of statistics really simple.  This
// automatically passes the DEBUG_TYPE of the file into the statistic.
#define STATISTIC(VARNAME, DESC)                                               \
   static ::polar::basic::Statistic VARNAME = {DEBUG_TYPE, #VARNAME, DESC, {0}, {false}}

/// Enable the collection and printing of statistics.
void enable_statistics(bool printOnExit = true);

/// Check if statistics are enabled.
bool are_statistics_enabled();

/// Return a file stream to print our output on.
std::unique_ptr<RawFdOutStream> create_info_output_file();

/// Print statistics to the file returned by CreateInfoOutputFile().
void print_statistics();

/// Print statistics to the given output stream.
void print_statistics(RawOutStream &outStream);

/// Print statistics in JSON format. This does include all global timers (\see
/// Timer, TimerGroup). Note that the timers are cleared after printing and will
/// not be printed in human readable form or in a second call of
/// PrintStatisticsJSON().
void print_statistics_json(RawOutStream &outStream);

/// Get the statistics. This can be used to look up the value of
/// statistics without needing to parse JSON.
///
/// This function does not prevent statistics being updated by other threads
/// during it's execution. It will return the value at the point that it is
/// read. However, it will prevent new statistics from registering until it
/// completes.
const std::vector<std::pair<StringRef, unsigned>> get_statistics();

/// Reset the statistics. This can be used to zero and de-register the
/// statistics in order to measure a compilation.
///
/// When this function begins to call destructors prior to returning, all
/// statistics will be zero and unregistered. However, that might not remain the
/// case by the time this function finishes returning. Whether update from other
/// threads are lost or merely deferred until during the function return is
/// timing sensitive.
///
/// Callers who intend to use this to measure statistics for a single
/// compilation should ensure that no compilations are in progress at the point
/// this function is called and that only one compilation executes until calling
/// GetStatistics().
void reset_statistics();

} // basic
} // polar

#endif // POLARPHP_BASIC_ADT_STATICS_H
