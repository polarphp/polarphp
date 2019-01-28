// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/09.

#include "polarphp/utils/Chrono.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/utils/FormatVariadic.h"
#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;
using namespace std::chrono;

namespace {

TEST(ChronoTest, testTimeTConversion)
{
   EXPECT_EQ(time_t(0), to_time_t(to_time_point(time_t(0))));
   EXPECT_EQ(time_t(1), to_time_t(to_time_point(time_t(1))));
   EXPECT_EQ(time_t(47), to_time_t(to_time_point(time_t(47))));

   TimePoint<> TP;
   EXPECT_EQ(TP, to_time_point(to_time_t(TP)));
   TP += seconds(1);
   EXPECT_EQ(TP, to_time_point(to_time_t(TP)));
   TP += hours(47);
   EXPECT_EQ(TP, to_time_point(to_time_t(TP)));
}

TEST(ChronoTest, testTimePointFormat)
{
   using namespace std::chrono;
   struct tm TM {};
   TM.tm_year = 106;
   TM.tm_mon = 0;
   TM.tm_mday = 2;
   TM.tm_hour = 15;
   TM.tm_min = 4;
   TM.tm_sec = 5;
   TM.tm_isdst = -1;
   TimePoint<> T =
         system_clock::from_time_t(mktime(&TM)) + nanoseconds(123456789);

   // operator<< uses the format YYYY-MM-DD HH:MM:SS.NNNNNNNNN
   std::string S;
   RawStringOutStream outstream(S);
   outstream << T;
   EXPECT_EQ("2006-01-02 15:04:05.123456789", outstream.getStr());

   // formatv default style matches operator<<.
   EXPECT_EQ("2006-01-02 15:04:05.123456789", formatv("{0}", T).getStr());
   // formatv supports strftime-style format strings.
   EXPECT_EQ("15:04:05", formatv("{0:%H:%M:%S}", T).getStr());
   // formatv supports our strftime extensions for sub-second precision.
   EXPECT_EQ("123", formatv("{0:%L}", T).getStr());
   EXPECT_EQ("123456", formatv("{0:%f}", T).getStr());
   EXPECT_EQ("123456789", formatv("{0:%N}", T).getStr());
   // our extensions don't interfere with %% escaping.
   EXPECT_EQ("%foo", formatv("{0:%%foo}", T).getStr());
}

// Test that to_time_point and to_time_t can be called with a arguments with varying
// precisions.
TEST(ChronoTest, testImplicitConversions)
{
   std::time_t TimeT = 47;
   TimePoint<seconds> Sec = to_time_point(TimeT);
   TimePoint<milliseconds> Milli = to_time_point(TimeT);
   TimePoint<microseconds> Micro = to_time_point(TimeT);
   TimePoint<nanoseconds> Nano = to_time_point(TimeT);
   EXPECT_EQ(Sec, Milli);
   EXPECT_EQ(Sec, Micro);
   EXPECT_EQ(Sec, Nano);
   EXPECT_EQ(TimeT, to_time_t(Sec));
   EXPECT_EQ(TimeT, to_time_t(Milli));
   EXPECT_EQ(TimeT, to_time_t(Micro));
   EXPECT_EQ(TimeT, to_time_t(Nano));
}

TEST(ChronoTest, testDurationFormat)
{
   EXPECT_EQ("1 h", formatv("{0}", hours(1)).getStr());
   EXPECT_EQ("1 m", formatv("{0}", minutes(1)).getStr());
   EXPECT_EQ("1 s", formatv("{0}", seconds(1)).getStr());
   EXPECT_EQ("1 ms", formatv("{0}", milliseconds(1)).getStr());
   EXPECT_EQ("1 us", formatv("{0}", microseconds(1)).getStr());
   EXPECT_EQ("1 ns", formatv("{0}", nanoseconds(1)).getStr());

   EXPECT_EQ("1 s", formatv("{0:+}", seconds(1)).getStr());
   EXPECT_EQ("1", formatv("{0:-}", seconds(1)).getStr());

   EXPECT_EQ("1000 ms", formatv("{0:ms}", seconds(1)).getStr());
   EXPECT_EQ("1000000 us", formatv("{0:us}", seconds(1)).getStr());
   EXPECT_EQ("1000", formatv("{0:ms-}", seconds(1)).getStr());

   EXPECT_EQ("1,000 ms", formatv("{0:+n}", milliseconds(1000)).getStr());
   EXPECT_EQ("0x3e8", formatv("{0:-x}", milliseconds(1000)).getStr());
   EXPECT_EQ("010", formatv("{0:-3}", milliseconds(10)).getStr());
   EXPECT_EQ("10,000", formatv("{0:ms-n}", seconds(10)).getStr());

   EXPECT_EQ("1.00 s", formatv("{0}", duration<float>(1)).getStr());
   EXPECT_EQ("0.123 s", formatv("{0:+3}", duration<float>(0.123f)).getStr());
   EXPECT_EQ("1.230e-01 s", formatv("{0:+e3}", duration<float>(0.123f)).getStr());

   typedef duration<float, std::ratio<60 * 60 * 24 * 14, 1000000>>
         microfortnights;
   EXPECT_EQ("1.00", formatv("{0:-}", microfortnights(1)).getStr());
   EXPECT_EQ("1209.60 ms", formatv("{0:ms}", microfortnights(1)).getStr());
}

} // anonymous namespace
