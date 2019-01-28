// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/08.

#include "polarphp/basic/adt/IntrusiveRefCountPtr.h"
#include "gtest/gtest.h"

using namespace polar::basic;

namespace {

struct SimpleRefCounted : public RefCountedBase<SimpleRefCounted>
{
   SimpleRefCounted() { ++NumInstances; }
   SimpleRefCounted(const SimpleRefCounted &) : RefCountedBase() {
      ++NumInstances;
   }
   ~SimpleRefCounted() { --NumInstances; }

   static int NumInstances;
};
int SimpleRefCounted::NumInstances = 0;

TEST(IntrusiveRefCountPtrTest, testRefCountedBaseCopyDoesNotLeak)
{
   EXPECT_EQ(0, SimpleRefCounted::NumInstances);
   {
      SimpleRefCounted *S1 = new SimpleRefCounted;
      IntrusiveRefCountPtr<SimpleRefCounted> R1 = S1;
      SimpleRefCounted *S2 = new SimpleRefCounted(*S1);
      IntrusiveRefCountPtr<SimpleRefCounted> R2 = S2;
      EXPECT_EQ(2, SimpleRefCounted::NumInstances);
   }
   EXPECT_EQ(0, SimpleRefCounted::NumInstances);
}

struct InterceptRefCounted : public RefCountedBase<InterceptRefCounted>
{
   InterceptRefCounted(bool *Released, bool *Retained)
      : Released(Released), Retained(Retained) {}
   bool * const Released;
   bool * const Retained;
};

} // anonymous namespace

namespace polar {
namespace basic {
template <>
struct IntrusiveRefCountPtrInfo<InterceptRefCounted>
{
   static void retain(InterceptRefCounted *I)
   {
      *I->Retained = true;
      I->retain();
   }
   static void release(InterceptRefCounted *I)
   {
      *I->Released = true;
      I->release();
   }
};
} // polar
} // utils

namespace {

TEST(IntrusiveRefCountPtrTest, testUsesTraitsToRetainAndRelease)
{
   bool Released = false;
   bool Retained = false;
   {
      InterceptRefCounted *I = new InterceptRefCounted(&Released, &Retained);
      IntrusiveRefCountPtr<InterceptRefCounted> R = I;
   }
   EXPECT_TRUE(Released);
   EXPECT_TRUE(Retained);
}

} // anonymous namespace
