// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/07.

#ifndef POLARPHP_UTILS_CHRONO_H
#define POLARPHP_UTILS_CHRONO_H

#include "polarphp/global/Global.h"
#include "polarphp/utils/FormatProviders.h"

#include <chrono>
#include <ctime>

namespace polar {
namespace utils {

class RawOutStream;

/// A time point on the system clock. This is provided for two reasons:
/// - to insulate us agains subtle differences in behavoir to differences in
///   system clock precision (which is implementation-defined and differs between
///   platforms).
/// - to shorten the type name
/// The default precision is nanoseconds. If need a specific precision specify
/// it explicitly. If unsure, use the default. If you need a time point on a
/// clock other than the system_clock, use std::chrono directly.
template <typename D = std::chrono::nanoseconds>
using TimePoint = std::chrono::time_point<std::chrono::system_clock, D>;

/// Convert a TimePoint to std::time_t
POLAR_ATTRIBUTE_ALWAYS_INLINE inline std::time_t to_time_t(TimePoint<> timePoint)
{
   using namespace std::chrono;
   return system_clock::to_time_t(
            time_point_cast<system_clock::time_point::duration>(timePoint));
}

/// Convert a std::time_t to a TimePoint
POLAR_ATTRIBUTE_ALWAYS_INLINE inline TimePoint<std::chrono::seconds>
to_time_point(std::time_t time)
{
   using namespace std::chrono;
   return time_point_cast<seconds>(system_clock::from_time_t(time));
}


/// Convert a std::time_t + nanoseconds to a TimePoint
POLAR_ATTRIBUTE_ALWAYS_INLINE inline TimePoint<>
to_time_point(std::time_t T, uint32_t nsec)
{
   using namespace std::chrono;
   return time_point_cast<nanoseconds>(system_clock::from_time_t(T))
         + nanoseconds(nsec);
}

RawOutStream &operator<<(RawOutStream &outStream, TimePoint<> timePoint);

/// Format provider for TimePoint<>
///
/// The options string is a strftime format string, with extensions:
///   - %L is millis: 000-999
///   - %f is micros: 000000-999999
///   - %N is nanos: 000000000 - 999999999
///
/// If no options are given, the default format is "%Y-%m-%d %H:%M:%S.%N".
template <>
struct FormatProvider<TimePoint<>>
{
   static void format(const TimePoint<> &TP, RawOutStream &outStream,
                      StringRef style);
};

/// Implementation of FormatProvider<T> for duration types.
///
/// The options string of a duration  type has the grammar:
///
///   duration_options  ::= [Unit][show_Unit [number_options]]
///   Unit              ::= `h`|`m`|`s`|`ms|`us`|`ns`
///   show_Unit         ::= `+` | `-`
///   number_options    ::= options string for a integral or floating point type
///
///   Examples
///   =================================
///   |  options  | Input | Output    |
///   =================================
///   | ""        | 1s    | 1 s       |
///   | "ms"      | 1s    | 1000 ms   |
///   | "ms-"     | 1s    | 1000      |
///   | "ms-n"    | 1s    | 1,000     |
///   | ""        | 1.0s  | 1.00 s    |
///   =================================
///
///  If the Unit of the duration type is not one of the Units specified above,
///  it is still possible to format it, provided you explicitly request a
///  display Unit or you request that the Unit is not displayed.

namespace internal {
template <typename Period>
struct Unit
{
   static const char value[];
};

template <typename Period>
const char Unit<Period>::value[] = "";

template <> struct Unit<std::ratio<3600>> { static const char value[]; };
template <> struct Unit<std::ratio<60>> { static const char value[]; };
template <> struct Unit<std::ratio<1>> { static const char value[]; };
template <> struct Unit<std::milli> { static const char value[]; };
template <> struct Unit<std::micro> { static const char value[]; };
template <> struct Unit<std::nano> { static const char value[]; };

} // namespace internal

template <typename Rep, typename Period>
struct FormatProvider<std::chrono::duration<Rep, Period>>
{
private:
   using DurationType = std::chrono::duration<Rep, Period>;
   using InternalRep = typename std::conditional<
   std::chrono::treat_as_floating_point<Rep>::value, double, intmax_t>::type;

   template <typename AsPeriod>
   static InternalRep getAs(const DurationType &duration)
   {
      return std::chrono::duration_cast<std::chrono::duration<InternalRep, AsPeriod>>(duration).count();
   }

   static std::pair<InternalRep, StringRef> consumeUnit(StringRef &style,
                                                        const DurationType &duration)
   {
      using namespace std::chrono;
      if (style.consumeFront("ns")) {
         return {getAs<std::nano>(duration), "ns"};
      }
      if (style.consumeFront("us")) {
         return {getAs<std::micro>(duration), "us"};
      }
      if (style.consumeFront("ms")) {
         return {getAs<std::milli>(duration), "ms"};
      }
      if (style.consumeFront("s")) {
         return {getAs<std::ratio<1>>(duration), "s"};
      }
      if (style.consumeFront("m")) {
         return {getAs<std::ratio<60>>(duration), "m"};
      }
      if (style.consumeFront("h")) {
         return {getAs<std::ratio<3600>>(duration), "h"};
      }
      return {duration.count(), internal::Unit<Period>::value};
   }

   static bool consumeShowUnit(StringRef &style)
   {
      if (style.empty()) {
         return true;
      }
      if (style.consumeFront("-")) {
         return false;
      }
      if (style.consumeFront("+")) {
         return true;
      }
      assert(0 && "Unrecognised duration format");
      return true;
   }

public:
   static void format(const DurationType &duration, RawOutStream &stream, StringRef style)
   {
      InternalRep count;
      StringRef unit;
      std::tie(count, unit) = consumeUnit(style, duration);
      bool showUnit = consumeShowUnit(style);
      FormatProvider<InternalRep>::format(count, stream, style);
      if (showUnit) {
         assert(!unit.empty());
         stream << " " << unit;
      }
   }
};

} // utils
} // polar

#endif // POLARPHP_UTILS_CHRONO_H
