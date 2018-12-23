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

#ifndef POLARPHP_UTILS_TIMER_H
#define POLARPHP_UTILS_TIMER_H

#include "polarphp/basic/adt/StringMap.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/global/DataTypes.h"
#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace polar {
namespace utils {

class Timer;
class TimerGroup;
class RawOutStream;

using polar::basic::StringMap;
using polar::basic::StringRef;

class TimeRecord
{
   double m_wallTime;       ///< Wall clock time elapsed in seconds.
   double m_userTime;       ///< User time elapsed.
   double m_systemTime;     ///< System time elapsed.
   ssize_t m_memUsed;       ///< Memory allocated (in bytes).
public:
   TimeRecord()
      : m_wallTime(0),
        m_userTime(0),
        m_systemTime(0),
        m_memUsed(0)
   {}

   /// Get the current time and memory usage.  If Start is true we get the memory
   /// usage before the time, otherwise we get time before memory usage.  This
   /// matters if the time to get the memory usage is significant and shouldn't
   /// be counted as part of a duration.
   static TimeRecord getCurrentTime(bool start = true);

   double getProcessTime() const
   {
      return m_userTime + m_systemTime;
   }

   double getUserTime() const
   {
      return m_userTime;
   }

   double getSystemTime() const
   {
      return m_systemTime;
   }

   double getWallTime() const
   {
      return m_wallTime;
   }

   ssize_t getMemUsed() const
   {
      return m_memUsed;
   }

   bool operator<(const TimeRecord &recod) const
   {
      // Sort by Wall m_time elapsed, as it is the only thing really accurate
      return m_wallTime < recod.m_wallTime;
   }

   void operator+=(const TimeRecord &other)
   {
      m_wallTime   += other.m_wallTime;
      m_userTime   += other.m_userTime;
      m_systemTime += other.m_systemTime;
      m_memUsed    += other.m_memUsed;
   }

   void operator-=(const TimeRecord &other)
   {
      m_wallTime   -= other.m_wallTime;
      m_userTime   -= other.m_userTime;
      m_systemTime -= other.m_systemTime;
      m_memUsed    -= other.m_memUsed;
   }

   /// Print the current time record to \p OS, with a breakdown showing
   /// contributions to the \p Total time record.
   void print(const TimeRecord &total, RawOutStream &outStream) const;
};

/// This class is used to track the amount of time spent between invocations of
/// its startTimer()/stopTimer() methods.  Given appropriate OS support it can
/// also keep track of the RSS of the program at various points.  By default,
/// the Timer will print the amount of time it has captured to standard error
/// when the last timer is destroyed, otherwise it is printed when its
/// TimerGroup is destroyed.  Timers do not print their information if they are
/// never started.
class Timer
{
   TimeRecord m_time;          ///< The total time captured.
   TimeRecord m_startTime;     ///< The time startTimer() was last called.
   std::string m_name;         ///< The name of this time variable.
   std::string m_description;  ///< description of this time variable.
   bool m_running;             ///< Is the timer currently running?
   bool m_triggered;           ///< Has the timer ever been triggered?
   TimerGroup *m_timerGroup = nullptr; ///< The TimerGroup this Timer is in.

   Timer **m_prev;             ///< Pointer to \p Next of previous timer in group.
   Timer *m_next;              ///< Next timer in the group.
public:
   explicit Timer(StringRef name, StringRef description)
   {
      init(name, description);
   }
   Timer(StringRef name, StringRef description, TimerGroup &tg)
   {
      init(name, description, tg);
   }

   Timer(const Timer &other)
   {
      assert(!other.m_timerGroup && "Can only copy uninitialized timers");
   }

   const Timer &operator=(const Timer &other)
   {
      assert(!m_timerGroup && !other.m_timerGroup && "Can only assign uninit timers");
      return *this;
   }

   ~Timer();

   /// Create an uninitialized timer, client must use 'init'.
   explicit Timer()
   {}

   void init(StringRef name, StringRef description);
   void init(StringRef name, StringRef description, TimerGroup &tg);

   const std::string &getName() const
   {
      return m_name;
   }

   const std::string &getDescription() const
   {
      return m_description;
   }

   bool isInitialized() const
   {
      return m_timerGroup != nullptr;
   }

   /// Check if the timer is currently running.
   bool isRunning() const
   {
      return m_running;
   }

   /// Check if startTimer() has ever been called on this timer.
   bool hasTriggered() const
   {
      return m_triggered;
   }

   /// Start the timer running.  m_time between calls to startTimer/stopTimer is
   /// counted by the Timer class.  Note that these calls must be correctly
   /// paired.
   void startTimer();

   /// Stop the timer.
   void stopTimer();

   /// Clear the timer state.
   void clear();

   /// Return the duration for which this timer has been running.
   TimeRecord getTotalTime() const
   {
      return m_time;
   }

private:
   friend class TimerGroup;
};

/// The TimeRegion class is used as a helper class to call the startTimer() and
/// stopTimer() methods of the Timer class.  When the object is constructed, it
/// starts the timer specified as its argument.  When it is destroyed, it stops
/// the relevant timer.  This makes it easy to time a region of code.
class TimeRegion
{
   Timer *m_timer;
   TimeRegion(const TimeRegion &) = delete;

public:
   explicit TimeRegion(Timer &timer) : m_timer(&timer)
   {
      m_timer->startTimer();
   }

   explicit TimeRegion(Timer *timer) : m_timer(timer)
   {
      if (m_timer) {
         m_timer->startTimer();
      }
   }

   ~TimeRegion()
   {
      if (m_timer) {
         m_timer->stopTimer();
      }
   }
};

/// This class is basically a combination of TimeRegion and Timer.  It allows
/// you to declare a new timer, AND specify the region to time, all in one
/// statement.  All timers with the same name are merged.  This is primarily
/// used for debugging and for hunting performance problems.
struct NamedRegionTimer : public TimeRegion
{
   explicit NamedRegionTimer(StringRef name, StringRef description,
                             StringRef groupName,
                             StringRef groupDescription, bool enabled = true);
};

/// The TimerGroup class is used to group together related timers into a single
/// report that is printed when the TimerGroup is destroyed.  It is illegal to
/// destroy a TimerGroup object before all of the Timers in it are gone.  A
/// TimerGroup can be specified for a newly created timer in its constructor.
class TimerGroup
{
   struct PrintRecord
   {
      TimeRecord m_time;
      std::string m_name;
      std::string m_description;

      PrintRecord(const PrintRecord &other) = default;
      PrintRecord(const TimeRecord &time, const std::string &name,
                  const std::string &description)
         : m_time(time),
           m_name(name),
           m_description(description)
      {}

      bool operator <(const PrintRecord &other) const
      {
         return m_time < other.m_time;
      }
   };
   std::string m_name;
   std::string m_description;
   Timer *m_firstTimer = nullptr; ///< First timer in the group.
   std::vector<PrintRecord> m_timersToPrint;

   TimerGroup **m_prev; ///< Pointer to Next field of previous timergroup in list.
   TimerGroup *m_next;  ///< Pointer to next timergroup in list.
   TimerGroup(const TimerGroup &timerGroup) = delete;
   void operator=(const TimerGroup &timerGroup) = delete;

public:
   explicit TimerGroup(StringRef name, StringRef description);

   explicit TimerGroup(StringRef name, StringRef description,
                       const StringMap<TimeRecord> &records);

   ~TimerGroup();

   void setName(StringRef newName, StringRef newDescription)
   {
      m_name.assign(newName.begin(), newName.end());
      m_description.assign(newDescription.begin(), newDescription.end());
   }

   /// Print any started timers in this group.
   void print(RawOutStream &outStream);

   /// Clear all timers in this group.
   void clear();

   /// This static method prints all timers.
   static void printAll(RawOutStream &outStream);

   /// Clear out all timers. This is mostly used to disable automatic
   /// printing on shutdown, when timers have already been printed explicitly
   /// using \c printAll or \c printJSONValues.
   static void clearAll();

   const char *printJSONValues(RawOutStream &outStream, const char *delim);

   /// Prints all timers as JSON key/value pairs.
   static const char *printAllJSONValues(RawOutStream &outStream, const char *delim);

   /// Ensure global timer group lists are initialized. This function is mostly
   /// used by the Statistic code to influence the construction and destruction
   /// order of the global timer lists.
   static void constructTimerLists();
private:
   friend class Timer;
   friend void print_statistics_json(RawOutStream &outStream);
   void addTimer(Timer &T);
   void removeTimer(Timer &T);
   void prepareToPrintList();
   void printQueuedTimers(RawOutStream &outStream);
   void printJSONValue(RawOutStream &outStream, const PrintRecord &R,
                       const char *suffix, double value);
};

} // utils
} // polar

#endif // POLARPHP_UTILS_TIMER_H
