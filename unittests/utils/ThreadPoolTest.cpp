// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/23.

#include "polarphp/utils/ThreadPool.h"

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/utils/Host.h"

#include "gtest/gtest.h"

using namespace polar::basic;
using namespace polar::utils;

// Fixture for the unittests, allowing to *temporarily* disable the unittests
// on a particular platform
class ThreadPoolTest : public testing::Test
{
   Triple Host;
   SmallVector<Triple::ArchType, 4> UnsupportedArchs;
   SmallVector<Triple::OSType, 4> UnsupportedOSs;
   SmallVector<Triple::EnvironmentType, 1> UnsupportedEnvironments;
protected:
   // This is intended for platform as a temporary "XFAIL"
   bool isUnsupportedOSOrEnvironment()
   {
      Triple Host(Triple::normalize(polar::sys::get_process_triple()));

      if (find(UnsupportedEnvironments, Host.getEnvironment()) !=
          UnsupportedEnvironments.end())
         return true;

      if (is_contained(UnsupportedOSs, Host.getOS()))
         return true;

      if (is_contained(UnsupportedArchs, Host.getArch()))
         return true;

      return false;
   }

   ThreadPoolTest()
   {
      // Add unsupported configuration here, example:
      //   UnsupportedArchs.push_back(Triple::x86_64);

      // See https://llvm.org/bugs/show_bug.cgi?id=25829
      UnsupportedArchs.push_back(Triple::ArchType::ppc64le);
      UnsupportedArchs.push_back(Triple::ArchType::ppc64);
   }

   /// Make sure this thread not progress faster than the main thread.
   void waitForMainThread()
   {
      std::unique_lock<std::mutex> LockGuard(WaitMainThreadMutex);
      WaitMainThread.wait(LockGuard, [&] { return MainThreadReady; });
   }

   /// Set the readiness of the main thread.
   void setMainThreadReady()
   {
      {
         std::unique_lock<std::mutex> LockGuard(WaitMainThreadMutex);
         MainThreadReady = true;
      }
      WaitMainThread.notify_all();
   }

   void SetUp() override { MainThreadReady = false; }

   std::condition_variable WaitMainThread;
   std::mutex WaitMainThreadMutex;
   bool MainThreadReady;

};

#define CHECK_UNSUPPORTED() \
   do { \
   if (isUnsupportedOSOrEnvironment()) \
   return; \
   } while (0); \

TEST_F(ThreadPoolTest, testAsyncBarrier)
{
   CHECK_UNSUPPORTED();
   // test that async & barrier work together properly.

   std::atomic_int checked_in{0};

   ThreadPool Pool;
   for (size_t i = 0; i < 5; ++i) {
      Pool.async([this, &checked_in] {
         waitForMainThread();
         ++checked_in;
      });
   }
   ASSERT_EQ(0, checked_in);
   setMainThreadReady();
   Pool.wait();
   ASSERT_EQ(5, checked_in);
}

static void TestFunc(std::atomic_int &checked_in, int i) { checked_in += i; }

TEST_F(ThreadPoolTest, testAsyncBarrierArgs)
{
   CHECK_UNSUPPORTED();
   // Test that async works with a function requiring multiple parameters.
   std::atomic_int checked_in{0};

   ThreadPool Pool;
   for (size_t i = 0; i < 5; ++i) {
      Pool.async(TestFunc, std::ref(checked_in), i);
   }
   Pool.wait();
   ASSERT_EQ(10, checked_in);
}

TEST_F(ThreadPoolTest, testAsync)
{
   CHECK_UNSUPPORTED();
   ThreadPool Pool;
   std::atomic_int i{0};
   Pool.async([this, &i] {
      waitForMainThread();
      ++i;
   });
   Pool.async([&i] { ++i; });
   ASSERT_NE(2, i.load());
   setMainThreadReady();
   Pool.wait();
   ASSERT_EQ(2, i.load());
}

TEST_F(ThreadPoolTest, testGetFuture)
{
   CHECK_UNSUPPORTED();
   ThreadPool Pool{2};
   std::atomic_int i{0};
   Pool.async([this, &i] {
      waitForMainThread();
      ++i;
   });
   // Force the future using get()
   Pool.async([&i] { ++i; }).get();
   ASSERT_NE(2, i.load());
   setMainThreadReady();
   Pool.wait();
   ASSERT_EQ(2, i.load());
}

TEST_F(ThreadPoolTest, testPoolDestruction)
{
   CHECK_UNSUPPORTED();
   // Test that we are waiting on destruction
   std::atomic_int checked_in{0};
   {
      ThreadPool Pool;
      for (size_t i = 0; i < 5; ++i) {
         Pool.async([this, &checked_in] {
            waitForMainThread();
            ++checked_in;
         });
      }
      ASSERT_EQ(0, checked_in);
      setMainThreadReady();
   }
   ASSERT_EQ(5, checked_in);
}
