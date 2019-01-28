// This source file is part of the polarphp.org open source project
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/13.

#include "polarphp/utils/ManagedStatics.h"
#include "polarphp/utils/Allocator.h"
#include "polarphp/global/Global.h"
#include <pthread.h>

#include "gtest/gtest.h"

using namespace polar::utils;

namespace {

#if !__has_feature(memory_sanitizer)
namespace test1 {
ManagedStatic<int> ms;
void *helper(void*)
{
   *ms;
   return nullptr;
}

// Valgrind's leak checker complains glibc's stack allocation.
// To appease valgrind, we provide our own stack for each thread.
void *allocate_stack(pthread_attr_t &a, size_t n = 65536) {
   void *stack = safe_malloc(n);
   pthread_attr_init(&a);
#if defined(__linux__)
   pthread_attr_setstack(&a, stack, n);
#endif
   return stack;
}
}

TEST(ManagedStaticTest, testMultipleThreads)
{
   // Run this test under tsan: http://code.google.com/p/data-race-test/

   pthread_attr_t a1, a2;
   void *p1 = test1::allocate_stack(a1);
   void *p2 = test1::allocate_stack(a2);

   pthread_t t1, t2;
   pthread_create(&t1, &a1, test1::helper, nullptr);
   pthread_create(&t2, &a2, test1::helper, nullptr);
   pthread_join(t1, nullptr);
   pthread_join(t2, nullptr);
   free(p1);
   free(p2);
}
#endif

namespace NestedStatics {
static ManagedStatic<int> Ms1;
struct Nest {
   Nest() {
      ++(*Ms1);
   }

   ~Nest() {
      assert(Ms1.isConstructed());
      ++(*Ms1);
   }
};
static ManagedStatic<Nest> Ms2;

TEST(ManagedStaticTest, NestedStatics)
{
   EXPECT_FALSE(Ms1.isConstructed());
   EXPECT_FALSE(Ms2.isConstructed());

   *Ms2;
   EXPECT_TRUE(Ms1.isConstructed());
   EXPECT_TRUE(Ms2.isConstructed());
}
} // namespace NestedStatics

namespace CustomCreatorDeletor
{
struct CustomCreate
{
   static void *call() {
      void *Mem = std::malloc(sizeof(int));
      *((int *)Mem) = 42;
      return Mem;
   }
};
struct CustomDelete
{
   static void call(void *P) { std::free(P); }
};
static ManagedStatic<int, CustomCreate, CustomDelete> Custom;

TEST(ManagedStaticTest, CustomCreatorDeletor)
{
   EXPECT_EQ(42, *Custom);
}
} // namespace CustomCreatorDeletor

} // anonymous namespace
