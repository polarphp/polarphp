//===- llvm/unittest/Support/DebugTest.cpp --------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

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
