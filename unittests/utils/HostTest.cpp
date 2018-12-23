// This source file is part of the polarphp.org open source project

// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception

// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors

// Created by polarboy on 2018/07/13.

#include "polarphp/utils/Host.h"
#include "polarphp/basic/adt/SmallVector.h"
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/utils/FileSystem.h"
#include "polarphp/utils/Path.h"
#include "polarphp/utils/Program.h"

#include "gtest/gtest.h"
#include <iostream>

using namespace polar;
using namespace polar::utils;
using namespace polar::basic;

#define ASSERT_NO_ERROR(x)                                                     \
   if (std::error_code ASSERT_NO_ERROR_ec = x) {                                \
   SmallString<128> MessageStorage;                                           \
   RawSvectorOutStream Message(MessageStorage);                               \
   Message << #x ": did not return errc::success.\n"                          \
   << "error number: " << ASSERT_NO_ERROR_ec.value() << "\n"          \
   << "error message: " << ASSERT_NO_ERROR_ec.message() << "\n";      \
   GTEST_FATAL_FAILURE_(MessageStorage.getCStr());                              \
   } else {                                                                     \
   }

namespace {

class HostTestFix : public testing::Test {
   Triple Host;

protected:
   bool isSupportedArchAndOS() {
      // Initially this is only testing detection of the number of
      // physical cores, which is currently only supported/tested for
      // x86_64 Linux and Darwin.
      return (Host.getArch() == Triple::ArchType::x86_64 &&
              (Host.isOSDarwin() || Host.getOS() == Triple::OSType::Linux));
   }

   HostTestFix() : Host(Triple::normalize(polar::sys::get_process_triple())) {}
};

TEST_F(HostTestFix, testNumPhysicalCores)
{
   int Num = polar::sys::get_host_num_physical_cores();

   if (isSupportedArchAndOS()) {
      ASSERT_GT(Num, 0);
   } else {
      ASSERT_EQ(Num, -1);
   }
}

TEST(HostTest, testLinuxHostCPUNamARM)
{
   StringRef CortexA9ProcCpuinfo =
         R"(
processor       : 0
model name      : ARMv7 Processor rev 10 (v7l)
BogoMIPS        : 1393.66
Features        : half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpd32
CPU implementer : 0x41
CPU architecture: 7
CPU variant     : 0x2
CPU part        : 0xc09
CPU revision    : 10

processor       : 1
model name      : ARMv7 Processor rev 10 (v7l)
BogoMIPS        : 1393.66
Features        : half thumb fastmult vfp edsp thumbee neon vfpv3 tls vfpd32
CPU implementer : 0x41
CPU architecture: 7
CPU variant     : 0x2
CPU part        : 0xc09
CPU revision    : 10

Hardware        : Generic OMAP4 (Flattened Device Tree)
Revision        : 0000
Serial          : 0000000000000000
)";

   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(CortexA9ProcCpuinfo),
             "cortex-a9");
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x41\n"
                                                      "CPU part        : 0xc0f"),
             "cortex-a15");
   // Verify that both CPU implementer and CPU part are checked:
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x40\n"
                                                      "CPU part        : 0xc0f"),
             "generic");
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x51\n"
                                                      "CPU part        : 0x06f"),
             "krait");
}

TEST(HostTest, testLinuxHostCPUNameAArch64)
{
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x41\n"
                                                      "CPU part        : 0xd03"),
             "cortex-a53");
   // Verify that both CPU implementer and CPU part are checked:
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x40\n"
                                                      "CPU part        : 0xd03"),
             "generic");
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x51\n"
                                                      "CPU part        : 0x201"),
             "kryo");
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x51\n"
                                                      "CPU part        : 0x800"),
             "cortex-a73");
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x51\n"
                                                      "CPU part        : 0x801"),
             "cortex-a73");
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x51\n"
                                                      "CPU part        : 0xc00"),
             "falkor");
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x51\n"
                                                      "CPU part        : 0xc01"),
             "saphira");

   // MSM8992/4 weirdness
   StringRef MSM8992ProcCpuInfo =
         R"(
Processor       : AArch64 Processor rev 3 (aarch64)
processor       : 0
processor       : 1
processor       : 2
processor       : 3
processor       : 4
processor       : 5
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32
CPU implementer : 0x41
CPU architecture: 8
CPU variant     : 0x0
CPU part        : 0xd03
CPU revision    : 3

Hardware        : Qualcomm Technologies, Inc MSM8992
)";

   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(MSM8992ProcCpuInfo),
             "cortex-a53");

   // Exynos big.LITTLE weirdness
   const std::string ExynosProcCpuInfo =
         R"(
processor       : 0
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32
CPU implementer : 0x41
CPU architecture: 8
CPU variant     : 0x0
CPU part        : 0xd03

processor       : 1
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32
CPU implementer : 0x53
CPU architecture: 8
)";

   // Verify default for Exynos.
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ExynosProcCpuInfo +
                                                      "CPU variant     : 0xc\n"
                                                      "CPU part        : 0xafe"),
             "exynos-m1");
   // Verify Exynos M1.
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ExynosProcCpuInfo +
                                                      "CPU variant     : 0x1\n"
                                                      "CPU part        : 0x001"),
             "exynos-m1");
   // Verify Exynos M2.
   EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ExynosProcCpuInfo +
                                                      "CPU variant     : 0x4\n"
                                                      "CPU part        : 0x001"),
             "exynos-m2");

   const std::string ThunderX2T99ProcCpuInfo = R"(
processor	: 0
BogoMIPS	: 400.00
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics
CPU implementer	: 0x43
CPU architecture: 8
CPU variant	: 0x1
CPU part	: 0x0af
)";

     // Verify different versions of ThunderX2T99.
     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x42\n"
                                                 "CPU part	: 0x516"),
               "thunderx2t99");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x42\n"
                                                 "CPU part	: 0x0516"),
               "thunderx2t99");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x43\n"
                                                 "CPU part	: 0x516"),
               "thunderx2t99");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x43\n"
                                                 "CPU part	: 0x0516"),
               "thunderx2t99");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x42\n"
                                                 "CPU part	: 0xaf"),
               "thunderx2t99");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x42\n"
                                                 "CPU part	: 0x0af"),
               "thunderx2t99");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x43\n"
                                                 "CPU part	: 0xaf"),
               "thunderx2t99");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderX2T99ProcCpuInfo +
                                                 "CPU implementer	: 0x43\n"
                                                 "CPU part	: 0x0af"),
               "thunderx2t99");

     // Verify ThunderXT88.
     const std::string ThunderXT88ProcCpuInfo = R"(
processor	: 0
BogoMIPS	: 200.00
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32
CPU implementer	: 0x43
CPU architecture: 8
CPU variant	: 0x1
CPU part	: 0x0a1
)";

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderXT88ProcCpuInfo +
                                                 "CPU implementer	: 0x43\n"
                                                 "CPU part	: 0x0a1"),
               "thunderxt88");

     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm(ThunderXT88ProcCpuInfo +
                                                 "CPU implementer	: 0x43\n"
                                                 "CPU part	: 0xa1"),
               "thunderxt88");

     // Verify HiSilicon processors.
     EXPECT_EQ(sys::internal::get_host_cpu_name_for_arm("CPU implementer : 0x48\n"
                                                 "CPU part        : 0xd01"),
               "tsv110");
}

#if defined(__APPLE__)
TEST_F(HostTestFix, testGetMacOSHostVersion)
{
   Triple HostTriple(sys::get_process_triple());
   if (!HostTriple.isMacOSX())
      return;

   SmallString<128> TestDirectory;
   ASSERT_NO_ERROR(fs::create_unique_directory("host_test", TestDirectory));
   SmallString<128> OutputFile(TestDirectory);
   fs::path::append(OutputFile, "out");

   const char *SwVersPath = "/usr/bin/sw_vers";
   StringRef argv[] = {SwVersPath, "-productVersion"};
   StringRef OutputPath = OutputFile.getStr();
   const std::optional<StringRef> Redirects[] = {/*STDIN=*/std::nullopt,
                                                 /*STDOUT=*/OutputPath,
                                                 /*STDERR=*/std::nullopt};
   int RetCode = sys::execute_and_wait(SwVersPath, argv,std::nullopt, /*env=*/std::nullopt, Redirects);
   ASSERT_EQ(0, RetCode);

   int FD = 0;
   ASSERT_NO_ERROR(fs::open_file_for_read(OutputPath, FD));
   off_t Size = ::lseek(FD, 0, SEEK_END);
   ASSERT_NE(-1, Size);
   ::lseek(FD, 0, SEEK_SET);
   std::unique_ptr<char[]> Buffer = std::make_unique<char[]>(Size);
   ASSERT_EQ(::read(FD, Buffer.get(), Size), Size);
   ::close(FD);

   // Ensure that the two versions match.
   StringRef SystemVersion(Buffer.get(), Size);
   unsigned SystemMajor, SystemMinor, SystemMicro;
   ASSERT_EQ(Triple((Twine("x86_64-apple-macos") + SystemVersion))
             .getMacOSXVersion(SystemMajor, SystemMinor, SystemMicro),
             true);
   unsigned HostMajor, HostMinor, HostMicro;
   ASSERT_EQ(HostTriple.getMacOSXVersion(HostMajor, HostMinor, HostMicro), true);

   // Don't compare the 'Micro' version, as it's always '0' for the 'Darwin'
   // triples.
   ASSERT_EQ(std::tie(SystemMajor, SystemMinor), std::tie(HostMajor, HostMinor));

   ASSERT_NO_ERROR(fs::remove(OutputPath));
   ASSERT_NO_ERROR(fs::remove(TestDirectory.getStr()));
}
#endif

} // anonymous namespace
