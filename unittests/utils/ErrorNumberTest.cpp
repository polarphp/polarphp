// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/13.

#include "polarphp/utils/ErrorNumber.h"
#include "gtest/gtest.h"

using namespace polar::utils;

namespace {

TEST(ErrnoTest, testRetryAfterSignal)
{
   EXPECT_EQ(1, retry_after_signal(-1, [] { return 1; }));

   EXPECT_EQ(-1, retry_after_signal(-1, [] {
      errno = EAGAIN;
      return -1;
   }));
   EXPECT_EQ(EAGAIN, errno);

   unsigned calls = 0;
   EXPECT_EQ(1, retry_after_signal(-1, [&calls] {
      errno = EINTR;
      ++calls;
      return calls == 1 ? -1 : 1;
   }));
   EXPECT_EQ(2u, calls);

   EXPECT_EQ(1, retry_after_signal(-1, [](int x) { return x; }, 1));

   std::unique_ptr<int> P(retry_after_signal(nullptr, [] { return new int(47); }));
   EXPECT_EQ(47, *P);
}

} // anonymous namespace
