// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/06.

#include "polarphp/basic/adt/Sequence.h"
#include "gtest/gtest.h"

#include <list>

using namespace polar::basic;

namespace {

TEST(SequenceTest, testBasic) {
   int x = 0;
   for (int i : seq(0, 10)) {
      EXPECT_EQ(x, i);
      x++;
   }
   EXPECT_EQ(10, x);

   auto my_seq = seq(0, 4);
   EXPECT_EQ(4, my_seq.end() - my_seq.begin());
   for (int i : {0, 1, 2, 3})
      EXPECT_EQ(i, (int)my_seq.begin()[i]);

   EXPECT_TRUE(my_seq.begin() < my_seq.end());

   auto adjusted_begin = my_seq.begin() + 2;
   auto adjusted_end = my_seq.end() - 2;
   EXPECT_TRUE(adjusted_begin == adjusted_end);
   EXPECT_EQ(2, *adjusted_begin);
   EXPECT_EQ(2, *adjusted_end);
}

} // anonymous namespace
