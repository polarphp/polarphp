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

#include "polarphp/global/Config.h"
#include "polarphp/utils/Chrono.h"
#include "polarphp/utils/Format.h"
#include "polarphp/utils/RawOutStream.h"

namespace polar {
namespace utils {

const char internal::Unit<std::ratio<3600>>::value[] = "h";
const char internal::Unit<std::ratio<60>>::value[] = "m";
const char internal::Unit<std::ratio<1>>::value[] = "s";
const char internal::Unit<std::milli>::value[] = "ms";
const char internal::Unit<std::micro>::value[] = "us";
const char internal::Unit<std::nano>::value[] = "ns";

namespace {

inline struct tm get_struct_tm(TimePoint<> timePoint)
{
   struct tm storage;
   std::time_t ourTime = to_time_t(timePoint);

#if defined(POLAR_OS_UNIX)
   struct tm *localTime = ::localtime_r(&ourTime, &storage);
   assert(localTime);
   (void)localTime;
#endif
#if defined(POLAR_OS_WIN)
   int error = ::localtime_s(&storage, &ourTime);
   assert(!error);
   (void)error;
#endif
   return storage;
}

} // anonymous namespace

RawOutStream &operator<<(RawOutStream &outStream, TimePoint<> timePoint)
{
   struct tm localTime = get_struct_tm(timePoint);
   char buffer[sizeof("YYYY-MM-DD HH:MM:SS")];
   strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &localTime);
   return outStream << buffer << '.'
                    << format("%.9lu",
                              long((timePoint.time_since_epoch() % std::chrono::seconds(1))
                                   .count()));
}

void FormatProvider<TimePoint<>>::format(const TimePoint<> &timePoint, RawOutStream &outStream,
                                          StringRef style) {
   using namespace std::chrono;
   TimePoint<seconds> truncated = time_point_cast<seconds>(timePoint);
   auto Fractional = timePoint - truncated;
   struct tm localTime = get_struct_tm(truncated);
   // Handle extensions first. strftime mangles unknown %x on some platforms.
   if (style.empty()) {
       style = "%Y-%m-%d %H:%M:%S.%N";
   }
   std::string format;
   RawStringOutStream fstream(format);
   for (unsigned index = 0; index < style.getSize(); ++index) {
      if (style[index] == '%' && style.getSize() > index + 1) switch (style[index + 1]) {
      case 'L':  // Milliseconds, from Ruby.
         fstream << polar::utils::format(
                       "%.3lu", (long)duration_cast<milliseconds>(Fractional).count());
         ++index;
         continue;
      case 'f':  // Microseconds, from Python.
         fstream << polar::utils::format(
                       "%.6lu", (long)duration_cast<microseconds>(Fractional).count());
         ++index;
         continue;
      case 'N':  // Nanoseconds, from date(1).
         fstream << polar::utils::format(
                       "%.6lu", (long)duration_cast<nanoseconds>(Fractional).count());
         ++index;
         continue;
      case '%':  // Consume %%, so %%f parses as (%%)f not %(%f)
         fstream << "%%";
         ++index;
         continue;
      }
      fstream << style[index];
   }
   fstream.flush();
   char buffer[256];  // Should be enough for anywhen.
   size_t len = strftime(buffer, sizeof(buffer), format.c_str(), &localTime);
   outStream << (len ? buffer : "BAD-DATE-FORMAT");
}

} // utils
} // polar
