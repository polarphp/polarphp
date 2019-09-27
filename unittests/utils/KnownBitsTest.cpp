//===- llvm/unittest/Support/KnownBitsTest.cpp - KnownBits tests ----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements unit tests for KnownBits functions.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/KnownBits.h"
#include "gtest/gtest.h"

using namespace polar::utils;

namespace {

template<typename FnTy>
void ForeachKnownBits(unsigned Bits, FnTy Fn) {
  unsigned Max = 1 << Bits;
  KnownBits Known(Bits);
  for (unsigned zero = 0; zero < Max; ++zero) {
    for (unsigned one = 0; one < Max; ++one) {
      Known.zero = zero;
      Known.one = one;
      if (Known.hasConflict())
        continue;
      Fn(Known);
    }
  }
}

template<typename FnTy>
void ForeachNumInKnownBits(const KnownBits &Known, FnTy Fn) {
  unsigned Bits = Known.getBitWidth();
  unsigned Max = 1 << Bits;
  for (unsigned N = 0; N < Max; ++N) {
    ApInt Num(Bits, N);
    if ((Num & Known.zero) != 0 || (~Num & Known.one) != 0)
      continue;

    Fn(Num);
  }
}

TEST(KnownBitsTest, testAddCarryExhaustive) {
  unsigned Bits = 4;
  ForeachKnownBits(Bits, [&](const KnownBits &Known1) {
    ForeachKnownBits(Bits, [&](const KnownBits &Known2) {
      ForeachKnownBits(1, [&](const KnownBits &KnownCarry) {
        // Explicitly compute known bits of the addition by trying all
        // possibilities.
        KnownBits Known(Bits);
        Known.zero.setAllBits();
        Known.one.setAllBits();
        ForeachNumInKnownBits(Known1, [&](const ApInt &N1) {
          ForeachNumInKnownBits(Known2, [&](const ApInt &N2) {
            ForeachNumInKnownBits(KnownCarry, [&](const ApInt &Carry) {
              ApInt Add = N1 + N2;
              if (Carry.getBoolValue())
                ++Add;

              Known.one &= Add;
              Known.zero &= ~Add;
            });
          });
        });

        KnownBits KnownComputed = KnownBits::computeForAddCarry(
            Known1, Known2, KnownCarry);
        EXPECT_EQ(Known.zero, KnownComputed.zero);
        EXPECT_EQ(Known.one, KnownComputed.one);
      });
    });
  });
}

static void TestAddSubExhaustive(bool IsAdd) {
  unsigned Bits = 4;
  ForeachKnownBits(Bits, [&](const KnownBits &Known1) {
    ForeachKnownBits(Bits, [&](const KnownBits &Known2) {
      KnownBits Known(Bits), KnownNSW(Bits);
      Known.zero.setAllBits();
      Known.one.setAllBits();
      KnownNSW.zero.setAllBits();
      KnownNSW.one.setAllBits();

      ForeachNumInKnownBits(Known1, [&](const ApInt &N1) {
        ForeachNumInKnownBits(Known2, [&](const ApInt &N2) {
          bool Overflow;
          ApInt Res;
          if (IsAdd)
            Res = N1.saddOverflow(N2, Overflow);
          else
            Res = N1.ssubOverflow(N2, Overflow);

          Known.one &= Res;
          Known.zero &= ~Res;

          if (!Overflow) {
            KnownNSW.one &= Res;
            KnownNSW.zero &= ~Res;
          }
        });
      });

      KnownBits KnownComputed = KnownBits::computeForAddSub(
          IsAdd, /*NSW*/false, Known1, Known2);
      EXPECT_EQ(Known.zero, KnownComputed.zero);
      EXPECT_EQ(Known.one, KnownComputed.one);

      // The NSW calculation is not precise, only check that it's
      // conservatively correct.
      KnownBits KnownNSWComputed = KnownBits::computeForAddSub(
          IsAdd, /*NSW*/true, Known1, Known2);
      EXPECT_TRUE(KnownNSWComputed.zero.isSubsetOf(KnownNSW.zero));
      EXPECT_TRUE(KnownNSWComputed.one.isSubsetOf(KnownNSW.one));
    });
  });
}

TEST(KnownBitsTest, testAddSubExhaustive) {
  TestAddSubExhaustive(true);
  TestAddSubExhaustive(false);
}

} // end anonymous namespace
