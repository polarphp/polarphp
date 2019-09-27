//===- llvm/unittest/Support/DebugTest.cpp --------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2019 polarphp software foundation
// Copyright (c) 2017 - 2019 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/10/22.
#include "polarphp/utils/Debug.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

#include <string>

using polar::utils::RawStringOutStream;

#ifndef NDEBUG
TEST(DebugTest, testBasic) {
   std::string s1, s2;
   RawStringOutStream os1(s1), os2(s2);
   static const char *DT[] = {"A", "B"};

   polar::sg_debugFlag = true;
   polar::set_current_debug_types(DT, 2);
   DEBUG_WITH_TYPE("A", os1 << "A");
   DEBUG_WITH_TYPE("B", os1 << "B");
   EXPECT_EQ("AB", os1.getStr());

   polar::set_current_debug_type("A");
   DEBUG_WITH_TYPE("A", os2 << "A");
   DEBUG_WITH_TYPE("B", os2 << "B");
   EXPECT_EQ("A", os2.getStr());
}
#endif
