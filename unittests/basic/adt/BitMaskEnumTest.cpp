// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/06.

#include "polarphp/basic/adt/BitmaskEnum.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {
enum Flags
{
   F0 = 0,
   F1 = 1,
   F2 = 2,
   F3 = 4,
   F4 = 8,
   POLAR_MARK_AS_BITMASK_ENUM(F4)
};

TEST(BitMaskEnumTest, testBitwiseOr)
{
   Flags f = F1 | F2;
   EXPECT_EQ(3, f);

   f = f | F3;
   EXPECT_EQ(7, f);
}

TEST(BitMaskEnumTest, testBitwiseOrEquals)
{
   Flags f = F1;
   f |= F3;
   EXPECT_EQ(5, f);

   // |= should return a reference to the LHS.
   f = F2;
   (f |= F3) = F1;
   EXPECT_EQ(F1, f);
}

TEST(BitMaskEnumTest, testBitwiseAnd)
{
   Flags f = static_cast<Flags>(3) & F2;
   EXPECT_EQ(F2, f);

   f = (f | F3) & (F1 | F2 | F3);
   EXPECT_EQ(6, f);
}

TEST(BitMaskEnumTest, testBitwiseAndEquals)
{
   Flags f = F1 | F2 | F3;
   f &= F1 | F2;
   EXPECT_EQ(3, f);

   // &= should return a reference to the LHS.
   (f &= F1) = F3;
   EXPECT_EQ(F3, f);
}

TEST(BitMaskEnumTest, testBitwiseXor)
{
   Flags f = (F1 | F2) ^ (F2 | F3);
   EXPECT_EQ(5, f);

   f = f ^ F1;
   EXPECT_EQ(4, f);
}

TEST(BitMaskEnumTest, testBitwiseXorEquals)
{
   Flags f = (F1 | F2);
   f ^= (F2 | F4);
   EXPECT_EQ(9, f);

   // ^= should return a reference to the LHS.
   (f ^= F4) = F3;
   EXPECT_EQ(F3, f);
}

TEST(BitMaskEnumTest, testBitwiseNot)
{
   Flags f = ~F1;
   EXPECT_EQ(14, f); // Largest value for f is 15.
   EXPECT_EQ(15, ~F0);
}

enum class FlagsClass
{
   F0 = 0,
   F1 = 1,
   F2 = 2,
   F3 = 4,
   POLAR_MARK_AS_BITMASK_ENUM(F3)
};

TEST(BitMaskEnumTest, testScopedEnum) {
   FlagsClass f = (FlagsClass::F1 & ~FlagsClass::F0) | FlagsClass::F2;
   f |= FlagsClass::F3;
   EXPECT_EQ(7, static_cast<int>(f));
}

struct Container {
   enum Flags { F0 = 0, F1 = 1, F2 = 2, F3 = 4, POLAR_MARK_AS_BITMASK_ENUM(F3) };

   static Flags getFlags() {
      Flags f = F0 | F1;
      f |= F2;
      return f;
   }
};

TEST(BitMaskEnumTest, testEnumInStruct) { EXPECT_EQ(3, Container::getFlags()); }

} // namespace

namespace foo {
namespace bar {
namespace {
enum FlagsInNamespace {
   F0 = 0,
   F1 = 1,
   F2 = 2,
   F3 = 4,
   POLAR_MARK_AS_BITMASK_ENUM(F3)
};
} // namespace
} // namespace foo
} // namespace bar

namespace {
TEST(BitMaskEnumTest, testEnumInNamespace)
{
   foo::bar::FlagsInNamespace f = ~foo::bar::F0 & (foo::bar::F1 | foo::bar::F2);
   f |= foo::bar::F3;
   EXPECT_EQ(7, f);
}
} // namespace
