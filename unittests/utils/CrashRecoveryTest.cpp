// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/12.

#include "polarphp/utils/CrashRecoveryContext.h"
#include "gtest/gtest.h"

#ifdef POLAR_ON_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#endif

using namespace polar::basic;
using namespace polar::utils;

namespace {

using polar::utils::CrashRecoveryContext;

static int GlobalInt = 0;
static void nullDeref() { *(volatile int *)0x10 = 0; }
static void incrementGlobal() { ++GlobalInt; }
static void polarTrap() { POLAR_BUILTIN_TRAP; }

TEST(CrashRecoveryTest, testBasic)
{
   CrashRecoveryContext::enable();
   GlobalInt = 0;
   EXPECT_TRUE(CrashRecoveryContext().runSafely(incrementGlobal));
   EXPECT_EQ(1, GlobalInt);
   EXPECT_FALSE(CrashRecoveryContext().runSafely(nullDeref));
   EXPECT_FALSE(CrashRecoveryContext().runSafely(polarTrap));
}

struct IncrementGlobalCleanup : CrashRecoveryContextCleanup {
   IncrementGlobalCleanup(CrashRecoveryContext *CRC)
      : CrashRecoveryContextCleanup(CRC) {}
   virtual void recoverResources() { ++GlobalInt; }
};

static void noop() {}

TEST(CrashRecoveryTest, testCleanup)
{
   CrashRecoveryContext::enable();
   GlobalInt = 0;
   {
      CrashRecoveryContext CRC;
      CRC.registerCleanup(new IncrementGlobalCleanup(&CRC));
      EXPECT_TRUE(CRC.runSafely(noop));
   } // run cleanups
   EXPECT_EQ(1, GlobalInt);

   GlobalInt = 0;
   {
      CrashRecoveryContext CRC;
      CRC.registerCleanup(new IncrementGlobalCleanup(&CRC));
      EXPECT_FALSE(CRC.runSafely(nullDeref));
   } // run cleanups
   EXPECT_EQ(1, GlobalInt);
}

#ifdef POLAR_ON_WIN32
static void raiseIt()
{
   RaiseException(123, EXCEPTION_NONCONTINUABLE, 0, NULL);
}

TEST(CrashRecoveryTest, testRaiseException)
{
   polar::utils::CrashRecoveryContext::enable();
   EXPECT_FALSE(CrashRecoveryContext().runSafely(raiseIt));
}

static void outputString()
{
   OutputDebugStringA("output for debugger\n");
}

TEST(CrashRecoveryTest, CallOutputDebugString)
{
   polar::utils::CrashRecoveryContext::enable();
   EXPECT_TRUE(CrashRecoveryContext().runSafely(outputString));
}

#endif

} // anonymous namespace
