// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/07/13.

#include "polarphp/utils/Memory.h"
#include "polarphp/utils/Process.h"
#include "gtest/gtest.h"
#include <cassert>
#include <cstdlib>

#if defined(__NetBSD__)
// clang-format off
#include <sys/param.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <err.h>
#include <unistd.h>
// clang-format on
#endif

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;
using namespace polar::sys;

namespace {

bool IsMPROTECT()
{
#if defined(__NetBSD__)
   int mib[3];
   int paxflags;
   size_t len = sizeof(paxflags);

   mib[0] = CTL_PROC;
   mib[1] = getpid();
   mib[2] = PROC_PID_PAXFLAGS;

   if (sysctl(mib, 3, &paxflags, &len, NULL, 0) != 0)
      err(EXIT_FAILURE, "sysctl");

   return !!(paxflags & CTL_PROC_PAXFLAGS_MPROTECT);
#else
   return false;
#endif
}


class MappedMemoryTest : public ::testing::TestWithParam<unsigned>
{
public:
   MappedMemoryTest()
   {
      Flags = GetParam();
      PageSize = sys::Process::getPageSize();
   }

protected:
   // Adds RW flags to permit testing of the resulting memory
   unsigned getTestableEquivalent(unsigned RequestedFlags)
   {
      switch (RequestedFlags) {
      case Memory::MF_READ:
      case Memory::MF_WRITE:
      case Memory::MF_READ|Memory::MF_WRITE:
         return Memory::MF_READ|Memory::MF_WRITE;
      case Memory::MF_READ|Memory::MF_EXEC:
      case Memory::MF_READ|Memory::MF_WRITE|Memory::MF_EXEC:
      case Memory::MF_EXEC:
         return Memory::MF_READ|Memory::MF_WRITE|Memory::MF_EXEC;
      }
      // Default in case values are added to the enum, as required by some compilers
      return Memory::MF_READ|Memory::MF_WRITE;
   }

   // Returns true if the memory blocks overlap
   bool doesOverlap(MemoryBlock M1, MemoryBlock M2)
   {
      if (M1.getBase() == M2.getBase())
         return true;

      if (M1.getBase() > M2.getBase())
         return (unsigned char *)M2.getBase() + M2.getSize() > M1.getBase();

      return (unsigned char *)M1.getBase() + M1.getSize() > M2.getBase();
   }

   unsigned Flags;
   size_t   PageSize;
};

// MPROTECT prevents W+X mmaps
#define CHECK_UNSUPPORTED() \
   do { \
   if ((Flags & Memory::MF_WRITE) && (Flags & Memory::MF_EXEC) && \
   IsMPROTECT()) \
   return; \
} while (0)

TEST_P(MappedMemoryTest, testAllocAndRelease)
{
   CHECK_UNSUPPORTED();
   std::error_code errorCode;
   MemoryBlock M1 = Memory::allocateMappedMemory(sizeof(int), nullptr, Flags,errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(sizeof(int), M1.getSize());

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
}

TEST_P(MappedMemoryTest, testMultipleAllocAndRelease)
{
   CHECK_UNSUPPORTED();
   std::error_code errorCode;
   MemoryBlock M1 = Memory::allocateMappedMemory(16, nullptr, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M2 = Memory::allocateMappedMemory(64, nullptr, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M3 = Memory::allocateMappedMemory(32, nullptr, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(16U, M1.getSize());
   EXPECT_NE((void*)nullptr, M2.getBase());
   EXPECT_LE(64U, M2.getSize());
   EXPECT_NE((void*)nullptr, M3.getBase());
   EXPECT_LE(32U, M3.getSize());

   EXPECT_FALSE(doesOverlap(M1, M2));
   EXPECT_FALSE(doesOverlap(M2, M3));
   EXPECT_FALSE(doesOverlap(M1, M3));

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
   EXPECT_FALSE(Memory::releaseMappedMemory(M3));
   MemoryBlock M4 = Memory::allocateMappedMemory(16, nullptr, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   EXPECT_NE((void*)nullptr, M4.getBase());
   EXPECT_LE(16U, M4.getSize());
   EXPECT_FALSE(Memory::releaseMappedMemory(M4));
   EXPECT_FALSE(Memory::releaseMappedMemory(M2));
}

TEST_P(MappedMemoryTest, testBasicWrite) {
   // This test applies only to readable and writeable combinations
   if (Flags &&
       !((Flags & Memory::MF_READ) && (Flags & Memory::MF_WRITE)))
      return;
   CHECK_UNSUPPORTED();
   std::error_code errorCode;
   MemoryBlock M1 = Memory::allocateMappedMemory(sizeof(int), nullptr, Flags,errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(sizeof(int), M1.getSize());

   int *a = (int*)M1.getBase();
   *a = 1;
   EXPECT_EQ(1, *a);

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
}

TEST_P(MappedMemoryTest, testMultipleWrite)
{
   // This test applies only to readable and writeable combinations
   if (Flags &&
       !((Flags & Memory::MF_READ) && (Flags & Memory::MF_WRITE)))
      return;
   CHECK_UNSUPPORTED();
   std::error_code errorCode;
   MemoryBlock M1 = Memory::allocateMappedMemory(sizeof(int), nullptr, Flags,
                                                 errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M2 = Memory::allocateMappedMemory(8 * sizeof(int), nullptr, Flags,
                                                 errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M3 = Memory::allocateMappedMemory(4 * sizeof(int), nullptr, Flags,
                                                 errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_FALSE(doesOverlap(M1, M2));
   EXPECT_FALSE(doesOverlap(M2, M3));
   EXPECT_FALSE(doesOverlap(M1, M3));

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(1U * sizeof(int), M1.getSize());
   EXPECT_NE((void*)nullptr, M2.getBase());
   EXPECT_LE(8U * sizeof(int), M2.getSize());
   EXPECT_NE((void*)nullptr, M3.getBase());
   EXPECT_LE(4U * sizeof(int), M3.getSize());

   int *x = (int*)M1.getBase();
   *x = 1;

   int *y = (int*)M2.getBase();
   for (int i = 0; i < 8; i++) {
      y[i] = i;
   }

   int *z = (int*)M3.getBase();
   *z = 42;

   EXPECT_EQ(1, *x);
   EXPECT_EQ(7, y[7]);
   EXPECT_EQ(42, *z);

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
   EXPECT_FALSE(Memory::releaseMappedMemory(M3));

   MemoryBlock M4 = Memory::allocateMappedMemory(64 * sizeof(int), nullptr,
                                                 Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   EXPECT_NE((void*)nullptr, M4.getBase());
   EXPECT_LE(64U * sizeof(int), M4.getSize());
   x = (int*)M4.getBase();
   *x = 4;
   EXPECT_EQ(4, *x);
   EXPECT_FALSE(Memory::releaseMappedMemory(M4));

   // Verify that M2 remains unaffected by other activity
   for (int i = 0; i < 8; i++) {
      EXPECT_EQ(i, y[i]);
   }
   EXPECT_FALSE(Memory::releaseMappedMemory(M2));
}

TEST_P(MappedMemoryTest, testEnabledWrite)
{
   // MPROTECT prevents W+X, and since this test always adds W we need
   // to block any variant with X.
   if ((Flags & Memory::MF_EXEC) && IsMPROTECT())
      return;
   std::error_code errorCode;
   MemoryBlock M1 = Memory::allocateMappedMemory(2 * sizeof(int), nullptr, Flags,
                                                 errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M2 = Memory::allocateMappedMemory(8 * sizeof(int), nullptr, Flags,
                                                 errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M3 = Memory::allocateMappedMemory(4 * sizeof(int), nullptr, Flags,
                                                 errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(2U * sizeof(int), M1.getSize());
   EXPECT_NE((void*)nullptr, M2.getBase());
   EXPECT_LE(8U * sizeof(int), M2.getSize());
   EXPECT_NE((void*)nullptr, M3.getBase());
   EXPECT_LE(4U * sizeof(int), M3.getSize());

   EXPECT_FALSE(Memory::protectMappedMemory(M1, getTestableEquivalent(Flags)));
   EXPECT_FALSE(Memory::protectMappedMemory(M2, getTestableEquivalent(Flags)));
   EXPECT_FALSE(Memory::protectMappedMemory(M3, getTestableEquivalent(Flags)));

   EXPECT_FALSE(doesOverlap(M1, M2));
   EXPECT_FALSE(doesOverlap(M2, M3));
   EXPECT_FALSE(doesOverlap(M1, M3));

   int *x = (int*)M1.getBase();
   *x = 1;
   int *y = (int*)M2.getBase();
   for (unsigned int i = 0; i < 8; i++) {
      y[i] = i;
   }
   int *z = (int*)M3.getBase();
   *z = 42;

   EXPECT_EQ(1, *x);
   EXPECT_EQ(7, y[7]);
   EXPECT_EQ(42, *z);

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
   EXPECT_FALSE(Memory::releaseMappedMemory(M3));
   EXPECT_EQ(6, y[6]);

   MemoryBlock M4 = Memory::allocateMappedMemory(16, nullptr, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   EXPECT_NE((void*)nullptr, M4.getBase());
   EXPECT_LE(16U, M4.getSize());
   EXPECT_EQ(std::error_code(),
             Memory::protectMappedMemory(M4, getTestableEquivalent(Flags)));
   x = (int*)M4.getBase();
   *x = 4;
   EXPECT_EQ(4, *x);
   EXPECT_FALSE(Memory::releaseMappedMemory(M4));
   EXPECT_FALSE(Memory::releaseMappedMemory(M2));
}

TEST_P(MappedMemoryTest, testSuccessiveNear)
{
   std::error_code errorCode;
   MemoryBlock M1 = Memory::allocateMappedMemory(16, nullptr, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M2 = Memory::allocateMappedMemory(64, &M1, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M3 = Memory::allocateMappedMemory(32, &M2, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(16U, M1.getSize());
   EXPECT_NE((void*)nullptr, M2.getBase());
   EXPECT_LE(64U, M2.getSize());
   EXPECT_NE((void*)nullptr, M3.getBase());
   EXPECT_LE(32U, M3.getSize());

   EXPECT_FALSE(doesOverlap(M1, M2));
   EXPECT_FALSE(doesOverlap(M2, M3));
   EXPECT_FALSE(doesOverlap(M1, M3));

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
   EXPECT_FALSE(Memory::releaseMappedMemory(M3));
   EXPECT_FALSE(Memory::releaseMappedMemory(M2));
}

TEST_P(MappedMemoryTest, testDuplicateNear)
{
   CHECK_UNSUPPORTED();
   std::error_code errorCode;
   MemoryBlock Near((void*)(3*PageSize), 16);
   MemoryBlock M1 = Memory::allocateMappedMemory(16, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M2 = Memory::allocateMappedMemory(64, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M3 = Memory::allocateMappedMemory(32, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(16U, M1.getSize());
   EXPECT_NE((void*)nullptr, M2.getBase());
   EXPECT_LE(64U, M2.getSize());
   EXPECT_NE((void*)nullptr, M3.getBase());
   EXPECT_LE(32U, M3.getSize());

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
   EXPECT_FALSE(Memory::releaseMappedMemory(M3));
   EXPECT_FALSE(Memory::releaseMappedMemory(M2));
}

TEST_P(MappedMemoryTest, testZeroNear)
{
   CHECK_UNSUPPORTED();
   std::error_code errorCode;
   MemoryBlock Near(nullptr, 0);
   MemoryBlock M1 = Memory::allocateMappedMemory(16, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M2 = Memory::allocateMappedMemory(64, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M3 = Memory::allocateMappedMemory(32, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(16U, M1.getSize());
   EXPECT_NE((void*)nullptr, M2.getBase());
   EXPECT_LE(64U, M2.getSize());
   EXPECT_NE((void*)nullptr, M3.getBase());
   EXPECT_LE(32U, M3.getSize());

   EXPECT_FALSE(doesOverlap(M1, M2));
   EXPECT_FALSE(doesOverlap(M2, M3));
   EXPECT_FALSE(doesOverlap(M1, M3));

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
   EXPECT_FALSE(Memory::releaseMappedMemory(M3));
   EXPECT_FALSE(Memory::releaseMappedMemory(M2));
}

TEST_P(MappedMemoryTest, testZeroSizeNear)
{
   CHECK_UNSUPPORTED();
   std::error_code errorCode;
   MemoryBlock Near((void*)(4*PageSize), 0);
   MemoryBlock M1 = Memory::allocateMappedMemory(16, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M2 = Memory::allocateMappedMemory(64, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);
   MemoryBlock M3 = Memory::allocateMappedMemory(32, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(16U, M1.getSize());
   EXPECT_NE((void*)nullptr, M2.getBase());
   EXPECT_LE(64U, M2.getSize());
   EXPECT_NE((void*)nullptr, M3.getBase());
   EXPECT_LE(32U, M3.getSize());

   EXPECT_FALSE(doesOverlap(M1, M2));
   EXPECT_FALSE(doesOverlap(M2, M3));
   EXPECT_FALSE(doesOverlap(M1, M3));

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
   EXPECT_FALSE(Memory::releaseMappedMemory(M3));
   EXPECT_FALSE(Memory::releaseMappedMemory(M2));
}

TEST_P(MappedMemoryTest, testUnalignedNear)
{
   std::error_code errorCode;
   MemoryBlock Near((void*)(2*PageSize+5), 0);
   MemoryBlock M1 = Memory::allocateMappedMemory(15, &Near, Flags, errorCode);
   EXPECT_EQ(std::error_code(), errorCode);

   EXPECT_NE((void*)nullptr, M1.getBase());
   EXPECT_LE(sizeof(int), M1.getSize());

   EXPECT_FALSE(Memory::releaseMappedMemory(M1));
}

// Note that Memory::MF_WRITE is not supported exclusively across
// operating systems and architectures and can imply MF_READ|MF_WRITE
unsigned MemoryFlags[] =
{
   Memory::MF_READ,
   Memory::MF_WRITE,
   Memory::MF_READ|Memory::MF_WRITE,
   Memory::MF_EXEC,
   Memory::MF_READ|Memory::MF_EXEC,
   Memory::MF_READ|Memory::MF_WRITE|Memory::MF_EXEC
};

INSTANTIATE_TEST_CASE_P(AllocationTests,
                        MappedMemoryTest,
                        ::testing::ValuesIn(MemoryFlags),);

} // anonymous namespace
