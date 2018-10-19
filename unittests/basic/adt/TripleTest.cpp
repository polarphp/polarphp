// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/07/07.

#include "polarphp/basic/adt/Triple.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

TEST(TripleTest, testBasicParsing)
{
   Triple T;

   T = Triple("");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("-");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("--");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("---");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("----");
   EXPECT_EQ("", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("-", T.getEnvironmentName().getStr());

   T = Triple("a");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("a-b");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("b", T.getVendorName().getStr());
   EXPECT_EQ("", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("a-b-c");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("b", T.getVendorName().getStr());
   EXPECT_EQ("c", T.getOSName().getStr());
   EXPECT_EQ("", T.getEnvironmentName().getStr());

   T = Triple("a-b-c-d");
   EXPECT_EQ("a", T.getArchName().getStr());
   EXPECT_EQ("b", T.getVendorName().getStr());
   EXPECT_EQ("c", T.getOSName().getStr());
   EXPECT_EQ("d", T.getEnvironmentName().getStr());
}

} // anonymous namespace
