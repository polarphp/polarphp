// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/08.

#include "polarphp/basic/adt/SetVector.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(SetVectorTest, testErase)
{
   SetVector<int> setVector;
   setVector.insert(0);
   setVector.insert(1);
   setVector.insert(2);

   auto I = setVector.erase(std::next(setVector.begin()));

   // Test that the returned iterator is the expected one-after-erase
   // and the size/contents is the expected sequence {0, 2}.
   EXPECT_EQ(std::next(setVector.begin()), I);
   EXPECT_EQ(2u, setVector.size());
   EXPECT_EQ(0, *setVector.begin());
   EXPECT_EQ(2, *std::next(setVector.begin()));
}

}
