// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/24.

//========- unittests/Support/TaskQueue.cpp - TaskQueue.h tests ------========//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "polarphp/global/Config.h"
#include "polarphp/utils/TaskQueue.h"

#include "gtest/gtest.h"

using namespace polar::utils;

class TaskQueueTest : public testing::Test
{
protected:
   TaskQueueTest() {}
};

TEST_F(TaskQueueTest, testOrderedFutures)
{
   ThreadPool threadPool(1);
   TaskQueue taskQueue(threadPool);
   std::atomic<int> X{ 0 };
   std::atomic<int> Y{ 0 };
   std::atomic<int> Z{ 0 };

   std::mutex mutex1, mutex2, mutex3;
   std::unique_lock<std::mutex> locker1(mutex1);
   std::unique_lock<std::mutex> locker2(mutex2);
   std::unique_lock<std::mutex> locker3(mutex3);

   std::future<void> future1 = taskQueue.async([&] {
      std::unique_lock<std::mutex> Lock(mutex1);
      ++X;
   });
   std::future<void> future2 = taskQueue.async([&] {
      std::unique_lock<std::mutex> Lock(mutex2);
      ++Y;
   });
   std::future<void> future3 = taskQueue.async([&] {
      std::unique_lock<std::mutex> Lock(mutex3);
      ++Z;
   });

   locker1.unlock();
   future1.wait();
   ASSERT_EQ(1, X);
   ASSERT_EQ(0, Y);
   ASSERT_EQ(0, Z);

   locker2.unlock();
   future2.wait();
   ASSERT_EQ(1, X);
   ASSERT_EQ(1, Y);
   ASSERT_EQ(0, Z);

   locker3.unlock();
   future3.wait();
   ASSERT_EQ(1, X);
   ASSERT_EQ(1, Y);
   ASSERT_EQ(1, Z);
}

TEST_F(TaskQueueTest, testUnOrderedFutures)
{
   ThreadPool threadPool(1);
   TaskQueue taskQueue(threadPool);
   std::atomic<int> X{ 0 };
   std::atomic<int> Y{ 0 };
   std::atomic<int> Z{ 0 };
   std::mutex M;

   std::unique_lock<std::mutex> Lock(M);

   std::future<void> future1 = taskQueue.async([&] { ++X; });
   std::future<void> future2 = taskQueue.async([&] { ++Y; });
   std::future<void> future3 = taskQueue.async([&M, &Z] {
      std::unique_lock<std::mutex> Lock(M);
      ++Z;
   });

   future2.wait();
   ASSERT_EQ(1, X);
   ASSERT_EQ(1, Y);
   ASSERT_EQ(0, Z);

   Lock.unlock();

   future3.wait();
   ASSERT_EQ(1, X);
   ASSERT_EQ(1, Y);
   ASSERT_EQ(1, Z);
}

TEST_F(TaskQueueTest, testFutureWithReturnValue)
{
   ThreadPool threadPool(1);
   TaskQueue taskQueue(threadPool);
   std::future<std::string> future1 = taskQueue.async([&] { return std::string("Hello"); });
   std::future<int> future2 = taskQueue.async([&] { return 42; });

   ASSERT_EQ(42, future2.get());
   ASSERT_EQ("Hello", future1.get());
}

