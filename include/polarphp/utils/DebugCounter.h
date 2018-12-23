// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/29.

#ifndef POLARPHP_UTILS_DEBUG_COUNTER_H
#define POLARPHP_UTILS_DEBUG_COUNTER_H

#include "polarphp/basic/adt/DenseMap.h"
#include "polarphp/basic/adt/UniqueVector.h"
#include "polarphp/utils/CommandLine.h"
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"
#include <string>

namespace polar {
namespace utils {

using polar::basic::DenseMap;
using polar::basic::UniqueVector;

class DebugCounter
{
public:
   ~DebugCounter();
   /// \brief Returns a reference to the singleton getInstance.
   static DebugCounter &getInstance();

   // Used by the command line option parser to push a new value it parsed.
   void pushBack(const std::string &);

   // Register a counter with the specified name.
   //
   // FIXME: Currently, counter registration is required to happen before command
   // line option parsing. The main reason to register counters is to produce a
   // nice list of them on the command line, but i'm not sure this is worth it.
   static unsigned registerCounter(StringRef name, StringRef desc)
   {
      return getInstance().addCounter(name, desc);
   }

   inline static bool shouldExecute(unsigned counterName)
   {
      if (!isCountingEnabled()) {
         return true;
      }
      auto &us = getInstance();
      auto result = us.m_counters.find(counterName);
      if (result != us.m_counters.end()) {
         auto &counterInfo = result->second;
         ++counterInfo.Count;

         // We only execute while the Skip is not smaller than Count,
         // and the stopAfter + skip is larger than Count.
         // Negative counters always execute.
         if (counterInfo.Skip < 0)
            return true;
         if (counterInfo.Skip >= counterInfo.Count)
            return false;
         if (counterInfo.StopAfter < 0)
            return true;
         return counterInfo.StopAfter + counterInfo.Skip >= counterInfo.Count;
      }
      // Didn't find the counter, should we warn?
      return true;
   }

   // Return true if a given counter had values set (either programatically or on
   // the command line).  This will return true even if those values are
   // currently in a state where the counter will always execute.
   static bool isCounterSet(unsigned id)
   {
      return getInstance().m_counters[id].IsSet;
   }

   // Return the skip and count for a counter. This only works for set counters.
   static int64_t getCounterValue(unsigned id)
   {
      auto &us = getInstance();
      auto result = us.m_counters.find(id);
      assert(result != us.m_counters.end() && "Asking about a non-set counter");
      return result->second.Count;
   }

   // Set a registered counter to a given value.
   static void setCounterValue(unsigned id, int64_t count)
   {
      auto &us = getInstance();
      us.m_counters[id].Count = count;
   }

   // Dump or print the current counter set into polar::debug_stream().
   POLAR_DUMP_METHOD void dump() const;

   void print(RawOutStream &outstream) const;

   // Get the counter id for a given named counter, or return 0 if none is found.
   unsigned getCounterId(const std::string &name) const
   {
      return m_registeredCounters.idFor(name);
   }

   // Return the number of registered counters.
   unsigned int getNumCounters() const
   {
      return m_registeredCounters.getSize();
   }

   // Return the name and description of the counter with the given id.
   std::pair<std::string, std::string> getCounterInfo(unsigned id) const
   {
      return std::make_pair(m_registeredCounters[id], m_counters.lookup(id).Desc);
   }

   // Iterate through the registered counters
   typedef UniqueVector<std::string> CounterVector;
   CounterVector::const_iterator begin() const
   {
      return m_registeredCounters.begin();
   }

   CounterVector::const_iterator end() const
   {
      return m_registeredCounters.end();
   }

   // Force-enables counting all DebugCounters.
   //
   // Since DebugCounters are incompatible with threading (not only do they not
   // make sense, but we'll also see data races), this should only be used in
   // contexts where we're certain we won't spawn threads.
   static void enableAllCounters()
   {
      getInstance().m_enabled = true;
   }

private:
   static bool isCountingEnabled()
   {
      // Compile to nothing when debugging is off
#ifdef NDEBUG
      return false;
#else
      return getInstance().m_enabled;
#endif
   }

   unsigned addCounter(const std::string &name, const std::string &desc)
   {
      unsigned result = m_registeredCounters.insert(name);
      m_counters[result] = {};
      m_counters[result].Desc = desc;
      return result;
   }
   // Struct to store counter info.
   struct CounterInfo
   {
      int64_t Count = 0;
      int64_t Skip = 0;
      int64_t StopAfter = -1;
      bool IsSet = false;
      std::string Desc;
   };
   DenseMap<unsigned, CounterInfo> m_counters;
   CounterVector m_registeredCounters;
   // Whether we should do DebugCounting at all. DebugCounters aren't
   // thread-safe, so this should always be false in multithreaded scenarios.
   bool m_enabled = false;
};

#define DEBUG_COUNTER(VARNAME, COUNTERNAME, DESC)                              \
   static const unsigned VARNAME =                                              \
   DebugCounter::registerCounter(COUNTERNAME, DESC)

} // utils
} // polar

#endif // POLARPHP_UTILS_DEBUG_COUNTER_H
