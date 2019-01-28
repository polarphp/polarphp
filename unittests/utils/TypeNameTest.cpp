// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/utils/TypeName.h"
#include "polarphp/utils/RawOutStream.h"
#include "gtest/gtest.h"

using namespace polar;
using namespace polar::utils;
using namespace polar::basic;

namespace {
namespace N1 {
struct S1 {};
class C1 {};
union U1 {};
}

TEST(TypeNameTest, testNames)
{
   struct S2 {};

   StringRef S1Name = getTypeName<N1::S1>();
   StringRef C1Name = getTypeName<N1::C1>();
   StringRef U1Name = getTypeName<N1::U1>();
   StringRef S2Name = getTypeName<S2>();

#if defined(__clang__) || defined(__GNUC__) || defined(__INTEL_COMPILER) ||    \
   defined(_MSC_VER)
   EXPECT_TRUE(S1Name.endsWith("::N1::S1")) << S1Name.getStr();
   EXPECT_TRUE(C1Name.endsWith("::N1::C1")) << C1Name.getStr();
   EXPECT_TRUE(U1Name.endsWith("::N1::U1")) << U1Name.getStr();
#ifdef __clang__
   EXPECT_TRUE(S2Name.endsWith("S2")) << S2Name.getStr();
#else
   EXPECT_TRUE(S2Name.endsWith("::S2")) << S2Name.getStr();
#endif
#else
   EXPECT_EQ("UNKNOWN_TYPE", S1Name);
   EXPECT_EQ("UNKNOWN_TYPE", C1Name);
   EXPECT_EQ("UNKNOWN_TYPE", U1Name);
   EXPECT_EQ("UNKNOWN_TYPE", S2Name);
#endif
}

} // end anonymous namespace
