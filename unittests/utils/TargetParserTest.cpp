// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/07/15.

#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/utils/ARMBuildAttributes.h"
#include "polarphp/utils/TargetParser.h"
#include "gtest/gtest.h"
#include <string>

using namespace polar;
using namespace polar::basic;
using namespace polar::utils;

namespace {

const char *ARMArch[] = {
   "armv2",       "armv2a",       "armv3",       "armv3m",       "armv4",
   "armv4t",      "armv5",        "armv5t",      "armv5e",       "armv5te",
   "armv5tej",    "armv6",        "armv6j",      "armv6k",       "armv6hl",
   "armv6t2",     "armv6kz",      "armv6z",      "armv6zk",      "armv6-m",
   "armv6m",      "armv6sm",      "armv6s-m",    "armv7-a",      "armv7",
   "armv7a",      "armv7ve",      "armv7hl",     "armv7l",       "armv7-r",
   "armv7r",      "armv7-m",      "armv7m",      "armv7k",       "armv7s",
   "armv7e-m",    "armv7em",      "armv8-a",     "armv8",        "armv8a",
   "armv8l",      "armv8.1-a",    "armv8.1a",    "armv8.2-a",    "armv8.2a",
   "armv8.3-a",   "armv8.3a",     "armv8.4-a",   "armv8.4a",     "armv8.5-a",
   "armv8.5a",     "armv8-r",     "armv8r",      "armv8-m.base", "armv8m.base",
   "armv8-m.main", "armv8m.main", "iwmmxt",      "iwmmxt2",      "xscale"
};

bool test_arm_cpu(StringRef CPUName, StringRef ExpectedArch,
                  StringRef ExpectedFPU, unsigned ExpectedFlags,
                  StringRef CPUAttr) {
   arm::ArchKind AK = arm::parse_cpu_arch(CPUName);
   bool pass = arm::get_arch_name(AK).equals(ExpectedArch);
   unsigned FPUKind = arm::get_default_fpu(CPUName, AK);
   pass &= arm::get_fpu_name(FPUKind).equals(ExpectedFPU);

   unsigned ExtKind = arm::get_default_extensions(CPUName, AK);
   if (ExtKind > 1 && (ExtKind & arm::AEK_NONE))
      pass &= ((ExtKind ^ arm::AEK_NONE) == ExpectedFlags);
   else
      pass &= (ExtKind == ExpectedFlags);

   pass &= arm::get_cpu_attr(AK).equals(CPUAttr);

   return pass;
}

TEST(TargetParserTest, testARMCPU)
{
   EXPECT_TRUE(test_arm_cpu("invalid", "invalid", "invalid",
                            arm::AEK_NONE, ""));
   EXPECT_TRUE(test_arm_cpu("generic", "invalid", "none",
                            arm::AEK_NONE, ""));

   EXPECT_TRUE(test_arm_cpu("arm2", "armv2", "none",
                            arm::AEK_NONE, "2"));
   EXPECT_TRUE(test_arm_cpu("arm3", "armv2a", "none",
                            arm::AEK_NONE, "2A"));
   EXPECT_TRUE(test_arm_cpu("arm6", "armv3", "none",
                            arm::AEK_NONE, "3"));
   EXPECT_TRUE(test_arm_cpu("arm7m", "armv3m", "none",
                            arm::AEK_NONE, "3M"));
   EXPECT_TRUE(test_arm_cpu("arm8", "armv4", "none",
                            arm::AEK_NONE, "4"));
   EXPECT_TRUE(test_arm_cpu("arm810", "armv4", "none",
                            arm::AEK_NONE, "4"));
   EXPECT_TRUE(test_arm_cpu("strongarm", "armv4", "none",
                            arm::AEK_NONE, "4"));
   EXPECT_TRUE(test_arm_cpu("strongarm110", "armv4", "none",
                            arm::AEK_NONE, "4"));
   EXPECT_TRUE(test_arm_cpu("strongarm1100", "armv4", "none",
                            arm::AEK_NONE, "4"));
   EXPECT_TRUE(test_arm_cpu("strongarm1110", "armv4", "none",
                            arm::AEK_NONE, "4"));
   EXPECT_TRUE(test_arm_cpu("arm7tdmi", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm7tdmi-s", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm710t", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm720t", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm9", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm9tdmi", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm920", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm920t", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm922t", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm9312", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm940t", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("ep9312", "armv4t", "none",
                            arm::AEK_NONE, "4T"));
   EXPECT_TRUE(test_arm_cpu("arm10tdmi", "armv5t", "none",
                            arm::AEK_NONE, "5T"));
   EXPECT_TRUE(test_arm_cpu("arm1020t", "armv5t", "none",
                            arm::AEK_NONE, "5T"));
   EXPECT_TRUE(test_arm_cpu("arm9e", "armv5te", "none",
                            arm::AEK_DSP, "5TE"));
   EXPECT_TRUE(test_arm_cpu("arm946e-s", "armv5te", "none",
                            arm::AEK_DSP, "5TE"));
   EXPECT_TRUE(test_arm_cpu("arm966e-s", "armv5te", "none",
                            arm::AEK_DSP, "5TE"));
   EXPECT_TRUE(test_arm_cpu("arm968e-s", "armv5te", "none",
                            arm::AEK_DSP, "5TE"));
   EXPECT_TRUE(test_arm_cpu("arm10e", "armv5te", "none",
                            arm::AEK_DSP, "5TE"));
   EXPECT_TRUE(test_arm_cpu("arm1020e", "armv5te", "none",
                            arm::AEK_DSP, "5TE"));
   EXPECT_TRUE(test_arm_cpu("arm1022e", "armv5te", "none",
                            arm::AEK_DSP, "5TE"));
   EXPECT_TRUE(test_arm_cpu("arm926ej-s", "armv5tej", "none",
                            arm::AEK_DSP, "5TEJ"));
   EXPECT_TRUE(test_arm_cpu("arm1136j-s", "armv6", "none",
                            arm::AEK_DSP, "6"));
   EXPECT_TRUE(test_arm_cpu("arm1136jf-s", "armv6", "vfpv2",
                            arm::AEK_DSP, "6"));
   EXPECT_TRUE(test_arm_cpu("arm1136jz-s", "armv6", "none",
                            arm::AEK_DSP, "6"));
   EXPECT_TRUE(test_arm_cpu("arm1176jz-s", "armv6kz", "none",
                            arm::AEK_SEC | arm::AEK_DSP, "6KZ"));
   EXPECT_TRUE(test_arm_cpu("mpcore", "armv6k", "vfpv2",
                            arm::AEK_DSP, "6K"));
   EXPECT_TRUE(test_arm_cpu("mpcorenovfp", "armv6k", "none",
                            arm::AEK_DSP, "6K"));
   EXPECT_TRUE(test_arm_cpu("arm1176jzf-s", "armv6kz", "vfpv2",
                            arm::AEK_SEC | arm::AEK_DSP, "6KZ"));
   EXPECT_TRUE(test_arm_cpu("arm1156t2-s", "armv6t2", "none",
                            arm::AEK_DSP, "6T2"));
   EXPECT_TRUE(test_arm_cpu("arm1156t2f-s", "armv6t2", "vfpv2",
                            arm::AEK_DSP, "6T2"));
   EXPECT_TRUE(test_arm_cpu("cortex-m0", "armv6-m", "none",
                            arm::AEK_NONE, "6-M"));
   EXPECT_TRUE(test_arm_cpu("cortex-m0plus", "armv6-m", "none",
                            arm::AEK_NONE, "6-M"));
   EXPECT_TRUE(test_arm_cpu("cortex-m1", "armv6-m", "none",
                            arm::AEK_NONE, "6-M"));
   EXPECT_TRUE(test_arm_cpu("sc000", "armv6-m", "none",
                            arm::AEK_NONE, "6-M"));
   EXPECT_TRUE(test_arm_cpu("cortex-a5", "armv7-a", "neon-vfpv4",
                            arm::AEK_MP | arm::AEK_SEC | arm::AEK_DSP, "7-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a7", "armv7-a", "neon-vfpv4",
                            arm::AEK_HWDIVTHUMB | arm::AEK_HWDIVARM | arm::AEK_MP |
                            arm::AEK_SEC | arm::AEK_VIRT | arm::AEK_DSP,
                            "7-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a8", "armv7-a", "neon",
                            arm::AEK_SEC | arm::AEK_DSP, "7-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a9", "armv7-a", "neon-fp16",
                            arm::AEK_MP | arm::AEK_SEC | arm::AEK_DSP, "7-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a12", "armv7-a", "neon-vfpv4",
                            arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT |
                            arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
                            arm::AEK_DSP,
                            "7-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a15", "armv7-a", "neon-vfpv4",
                            arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT |
                            arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
                            arm::AEK_DSP,
                            "7-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a17", "armv7-a", "neon-vfpv4",
                            arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT |
                            arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
                            arm::AEK_DSP,
                            "7-A"));
   EXPECT_TRUE(test_arm_cpu("krait", "armv7-a", "neon-vfpv4",
                            arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "7-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-r4", "armv7-r", "none",
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP, "7-R"));
   EXPECT_TRUE(test_arm_cpu("cortex-r4f", "armv7-r", "vfpv3-d16",
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP, "7-R"));
   EXPECT_TRUE(test_arm_cpu("cortex-r5", "armv7-r", "vfpv3-d16",
                            arm::AEK_MP | arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
                            arm::AEK_DSP,
                            "7-R"));
   EXPECT_TRUE(test_arm_cpu("cortex-r7", "armv7-r", "vfpv3-d16-fp16",
                            arm::AEK_MP | arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
                            arm::AEK_DSP,
                            "7-R"));
   EXPECT_TRUE(test_arm_cpu("cortex-r8", "armv7-r", "vfpv3-d16-fp16",
                            arm::AEK_MP | arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
                            arm::AEK_DSP,
                            "7-R"));
   EXPECT_TRUE(test_arm_cpu("cortex-r52", "armv8-r", "neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_MP | arm::AEK_VIRT |
                            arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
                            arm::AEK_DSP,
                            "8-R"));
   EXPECT_TRUE(
            test_arm_cpu("sc300", "armv7-m", "none", arm::AEK_HWDIVTHUMB, "7-M"));
   EXPECT_TRUE(
            test_arm_cpu("cortex-m3", "armv7-m", "none", arm::AEK_HWDIVTHUMB, "7-M"));
   EXPECT_TRUE(test_arm_cpu("cortex-m4", "armv7e-m", "fpv4-sp-d16",
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP, "7E-M"));
   EXPECT_TRUE(test_arm_cpu("cortex-m7", "armv7e-m", "fpv5-d16",
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP, "7E-M"));
   EXPECT_TRUE(test_arm_cpu("cortex-a32", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a35", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a53", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a55", "armv8.2-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_FP16 |
                            arm::AEK_RAS | arm::AEK_DOTPROD,
                            "8.2-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a57", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a72", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a73", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-a75", "armv8.2-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_FP16 |
                            arm::AEK_RAS | arm::AEK_DOTPROD,
                            "8.2-A"));
   EXPECT_TRUE(test_arm_cpu("cyclone", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("exynos-m1", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("exynos-m2", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("exynos-m3", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("exynos-m4", "armv8-a", "crypto-neon-fp-armv8",
                            arm::AEK_CRC | arm::AEK_SEC | arm::AEK_MP |
                            arm::AEK_VIRT | arm::AEK_HWDIVARM |
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "8-A"));
   EXPECT_TRUE(test_arm_cpu("cortex-m23", "armv8-m.base", "none",
                            arm::AEK_HWDIVTHUMB, "8-M.Baseline"));
   EXPECT_TRUE(test_arm_cpu("cortex-m33", "armv8-m.main", "fpv5-sp-d16",
                            arm::AEK_HWDIVTHUMB | arm::AEK_DSP, "8-M.Mainline"));
   EXPECT_TRUE(test_arm_cpu("iwmmxt", "iwmmxt", "none",
                            arm::AEK_NONE, "iwmmxt"));
   EXPECT_TRUE(test_arm_cpu("xscale", "xscale", "none",
                            arm::AEK_NONE, "xscale"));
   EXPECT_TRUE(test_arm_cpu("swift", "armv7s", "neon-vfpv4",
                            arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB | arm::AEK_DSP,
                            "7-S"));
}

static constexpr unsigned NumARMCPUArchs = 82;

TEST(TargetParserTest, testARMCPUArchList)
{
   SmallVector<StringRef, NumARMCPUArchs> List;
   arm::fill_valid_cpu_arch_list(List);

   // No list exists for these in this test suite, so ensure all are
   // valid, and match the expected 'magic' count.
   EXPECT_EQ(List.size(), NumARMCPUArchs);
   for(StringRef CPU : List) {
      EXPECT_NE(arm::parse_cpu_arch(CPU), arm::ArchKind::INVALID);
   }
}

TEST(TargetParserTest, testInvalidARMArch)
{
   auto InvalidArchStrings = {"armv", "armv99", "noarm"};
   for (const char* InvalidArch : InvalidArchStrings) {
      EXPECT_EQ(arm::parse_arch(InvalidArch), arm::ArchKind::INVALID);
   }
}

bool test_arm_arch(StringRef Arch, StringRef DefaultCPU, StringRef SubArch,
                   unsigned ArchAttr)
{
   arm::ArchKind AK = arm::parse_arch(Arch);
   return (AK!= arm::ArchKind::INVALID) &
         arm::get_default_cpu(Arch).equals(DefaultCPU) &
         arm::get_sub_arch(AK).equals(SubArch) &
         (arm::get_arch_attr(AK) == ArchAttr);
}

TEST(TargetParserTest, testARMArch)
{
   EXPECT_TRUE(
            test_arm_arch("armv2", "arm2", "v2",
                          armbuildattrs::CPUArch::Pre_v4));
   EXPECT_TRUE(
            test_arm_arch("armv2a", "arm3", "v2a",
                          armbuildattrs::CPUArch::Pre_v4));
   EXPECT_TRUE(
            test_arm_arch("armv3", "arm6", "v3",
                          armbuildattrs::CPUArch::Pre_v4));
   EXPECT_TRUE(
            test_arm_arch("armv3m", "arm7m", "v3m",
                          armbuildattrs::CPUArch::Pre_v4));
   EXPECT_TRUE(
            test_arm_arch("armv4", "strongarm", "v4",
                          armbuildattrs::CPUArch::v4));
   EXPECT_TRUE(
            test_arm_arch("armv4t", "arm7tdmi", "v4t",
                          armbuildattrs::CPUArch::v4T));
   EXPECT_TRUE(
            test_arm_arch("armv5t", "arm10tdmi", "v5",
                          armbuildattrs::CPUArch::v5T));
   EXPECT_TRUE(
            test_arm_arch("armv5te", "arm1022e", "v5e",
                          armbuildattrs::CPUArch::v5TE));
   EXPECT_TRUE(
            test_arm_arch("armv5tej", "arm926ej-s", "v5e",
                          armbuildattrs::CPUArch::v5TEJ));
   EXPECT_TRUE(
            test_arm_arch("armv6", "arm1136jf-s", "v6",
                          armbuildattrs::CPUArch::v6));
   EXPECT_TRUE(
            test_arm_arch("armv6t2", "arm1156t2-s", "v6t2",
                          armbuildattrs::CPUArch::v6T2));
   EXPECT_TRUE(
            test_arm_arch("armv6kz", "arm1176jzf-s", "v6kz",
                          armbuildattrs::CPUArch::v6KZ));
   EXPECT_TRUE(
            test_arm_arch("armv6-m", "cortex-m0", "v6m",
                          armbuildattrs::CPUArch::v6_M));
   EXPECT_TRUE(
            test_arm_arch("armv7-a", "generic", "v7",
                          armbuildattrs::CPUArch::v7));
   EXPECT_TRUE(
            test_arm_arch("armv7ve", "generic", "v7ve",
                          armbuildattrs::CPUArch::v7));
   EXPECT_TRUE(
            test_arm_arch("armv7-r", "cortex-r4", "v7r",
                          armbuildattrs::CPUArch::v7));
   EXPECT_TRUE(
            test_arm_arch("armv7-m", "cortex-m3", "v7m",
                          armbuildattrs::CPUArch::v7));
   EXPECT_TRUE(
            test_arm_arch("armv7e-m", "cortex-m4", "v7em",
                          armbuildattrs::CPUArch::v7E_M));
   EXPECT_TRUE(
            test_arm_arch("armv8-a", "generic", "v8",
                          armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(
            test_arm_arch("armv8.1-a", "generic", "v8.1a",
                          armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(
            test_arm_arch("armv8.2-a", "generic", "v8.2a",
                          armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(
            test_arm_arch("armv8.3-a", "generic", "v8.3a",
                          armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(
            test_arm_arch("armv8.4-a", "generic", "v8.4a",
                          armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(
            test_arm_arch("armv8.5-a", "generic", "v8.5a",
                          armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(
            test_arm_arch("armv8-r", "cortex-r52", "v8r",
                          armbuildattrs::CPUArch::v8_R));
   EXPECT_TRUE(
            test_arm_arch("armv8-m.base", "generic", "v8m.base",
                          armbuildattrs::CPUArch::v8_M_Base));
   EXPECT_TRUE(
            test_arm_arch("armv8-m.main", "generic", "v8m.main",
                          armbuildattrs::CPUArch::v8_M_Main));
   EXPECT_TRUE(
            test_arm_arch("iwmmxt", "iwmmxt", "",
                          armbuildattrs::CPUArch::v5TE));
   EXPECT_TRUE(
            test_arm_arch("iwmmxt2", "generic", "",
                          armbuildattrs::CPUArch::v5TE));
   EXPECT_TRUE(
            test_arm_arch("xscale", "xscale", "v5e",
                          armbuildattrs::CPUArch::v5TE));
   EXPECT_TRUE(
            test_arm_arch("armv7s", "swift", "v7s",
                          armbuildattrs::CPUArch::v7));
   EXPECT_TRUE(
            test_arm_arch("armv7k", "generic", "v7k",
                          armbuildattrs::CPUArch::v7));
}

bool test_arm_extension(StringRef CPUName,arm::ArchKind ArchKind, StringRef ArchExt)
{
   return arm::get_default_extensions(CPUName, ArchKind) &
         arm::parse_arch_ext(ArchExt);
}

TEST(TargetParserTest, testARMExtension)
{
   EXPECT_FALSE(test_arm_extension("arm2", arm::ArchKind::INVALID, "thumb"));
   EXPECT_FALSE(test_arm_extension("arm3", arm::ArchKind::INVALID, "thumb"));
   EXPECT_FALSE(test_arm_extension("arm6", arm::ArchKind::INVALID, "thumb"));
   EXPECT_FALSE(test_arm_extension("arm7m", arm::ArchKind::INVALID, "thumb"));
   EXPECT_FALSE(test_arm_extension("strongarm", arm::ArchKind::INVALID, "dsp"));
   EXPECT_FALSE(test_arm_extension("arm7tdmi", arm::ArchKind::INVALID, "dsp"));
   EXPECT_FALSE(test_arm_extension("arm10tdmi",
                                   arm::ArchKind::INVALID, "simd"));
   EXPECT_FALSE(test_arm_extension("arm1022e", arm::ArchKind::INVALID, "simd"));
   EXPECT_FALSE(test_arm_extension("arm926ej-s",
                                   arm::ArchKind::INVALID, "simd"));
   EXPECT_FALSE(test_arm_extension("arm1136jf-s",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("arm1176j-s",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("arm1156t2-s",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("arm1176jzf-s",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("cortex-m0",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("cortex-a8",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("cortex-r4",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("cortex-m3",
                                   arm::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_arm_extension("cortex-a53",
                                   arm::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_arm_extension("cortex-a53",
                                   arm::ArchKind::INVALID, "fp16"));
   EXPECT_TRUE(test_arm_extension("cortex-a55",
                                  arm::ArchKind::INVALID, "fp16"));
   EXPECT_FALSE(test_arm_extension("cortex-a55",
                                   arm::ArchKind::INVALID, "fp16fml"));
   EXPECT_TRUE(test_arm_extension("cortex-a75",
                                  arm::ArchKind::INVALID, "fp16"));
   EXPECT_FALSE(test_arm_extension("cortex-a75",
                                   arm::ArchKind::INVALID, "fp16fml"));
   EXPECT_FALSE(test_arm_extension("cortex-r52",
                                   arm::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_arm_extension("iwmmxt", arm::ArchKind::INVALID, "crc"));
   EXPECT_FALSE(test_arm_extension("xscale", arm::ArchKind::INVALID, "crc"));
   EXPECT_FALSE(test_arm_extension("swift", arm::ArchKind::INVALID, "crc"));

   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV2, "thumb"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV2A, "thumb"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV3, "thumb"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV3M, "thumb"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV4, "dsp"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV4T, "dsp"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV5T, "simd"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV5TE, "simd"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV5TEJ, "simd"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV6, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV6K, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic",
                                   arm::ArchKind::ARMV6T2, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic",
                                   arm::ArchKind::ARMV6KZ, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV6M, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV7A, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV7R, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV7M, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic",
                                   arm::ArchKind::ARMV7EM, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8A, "ras"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_1A, "ras"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_2A, "profile"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_2A, "fp16"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_2A, "fp16fml"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_3A, "fp16"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_3A, "fp16fml"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_4A, "fp16"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8_4A, "fp16fml"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV8R, "ras"));
   EXPECT_FALSE(test_arm_extension("generic",
                                   arm::ArchKind::ARMV8MBaseline, "crc"));
   EXPECT_FALSE(test_arm_extension("generic",
                                   arm::ArchKind::ARMV8MMainline, "crc"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::IWMMXT, "crc"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::IWMMXT2, "crc"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::XSCALE, "crc"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV7S, "crypto"));
   EXPECT_FALSE(test_arm_extension("generic", arm::ArchKind::ARMV7K, "crypto"));
}

TEST(TargetParserTest, testARMFPUVersion)
{
   for (arm::FPUKind FK = static_cast<arm::FPUKind>(0);
        FK <= arm::FPUKind::FK_LAST;
        FK = static_cast<arm::FPUKind>(static_cast<unsigned>(FK) + 1)) {
      if (FK == arm::FK_LAST || arm::get_fpu_name(FK) == "invalid" ||
          arm::get_fpu_name(FK) == "none" || arm::get_fpu_name(FK) == "softvfp") {
         EXPECT_EQ(arm::FPUVersion::NONE, arm::get_fpu_version(FK));
      } else {
         EXPECT_NE(arm::FPUVersion::NONE, arm::get_fpu_version(FK));
      }
   }
}

TEST(TargetParserTest, testARMFPUNeonSupportLevel)
{
   for (arm::FPUKind FK = static_cast<arm::FPUKind>(0);
        FK <= arm::FPUKind::FK_LAST;
        FK = static_cast<arm::FPUKind>(static_cast<unsigned>(FK) + 1))
      if (FK == arm::FK_LAST ||
          arm::get_fpu_name(FK).find("neon") == std::string::npos)
         EXPECT_EQ(arm::NeonSupportLevel::None,
                   arm::get_fpu_neon_support_level(FK));
      else
         EXPECT_NE(arm::NeonSupportLevel::None,
                   arm::get_fpu_neon_support_level(FK));
}

TEST(TargetParserTest, testARMFPURestriction)
{
   for (arm::FPUKind FK = static_cast<arm::FPUKind>(0);
        FK <= arm::FPUKind::FK_LAST;
        FK = static_cast<arm::FPUKind>(static_cast<unsigned>(FK) + 1)) {
      if (FK == arm::FK_LAST ||
          (arm::get_fpu_name(FK).find("d16") == std::string::npos &&
           arm::get_fpu_name(FK).find("vfpv3xd") == std::string::npos))
         EXPECT_EQ(arm::FPURestriction::None, arm::get_fpu_restriction(FK));
      else
         EXPECT_NE(arm::FPURestriction::None, arm::get_fpu_restriction(FK));
   }
}

TEST(TargetParserTest, testARMExtensionFeatures)
{
   std::vector<StringRef> Features;
   unsigned extensions =  arm::AEK_CRC | arm::AEK_CRYPTO | arm::AEK_DSP |
         arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB | arm::AEK_MP |
         arm::AEK_SEC | arm::AEK_VIRT | arm::AEK_RAS | arm::AEK_FP16 |
         arm::AEK_FP16FML;

   for (unsigned i = 0; i <= extensions; i++) {
      EXPECT_TRUE(i == 0 ? !arm::get_extension_features(i, Features)
                         : arm::get_extension_features(i, Features));
   }
}

TEST(TargetParserTest, testARMFPUFeatures)
{
   std::vector<StringRef> Features;
   for (arm::FPUKind FK = static_cast<arm::FPUKind>(0);
        FK <= arm::FPUKind::FK_LAST;
        FK = static_cast<arm::FPUKind>(static_cast<unsigned>(FK) + 1))
      EXPECT_TRUE((FK == arm::FK_INVALID || FK >= arm::FK_LAST)
                  ? !arm::get_fpu_features(FK, Features)
                  : arm::get_fpu_features(FK, Features));
}

TEST(TargetParserTest, testARMArchExtFeature)
{
   const char *ArchExt[][4] = {{"crc", "nocrc", "+crc", "-crc"},
                               {"crypto", "nocrypto", "+crypto", "-crypto"},
                               {"dsp", "nodsp", "+dsp", "-dsp"},
                               {"fp", "nofp", nullptr, nullptr},
                               {"idiv", "noidiv", nullptr, nullptr},
                               {"mp", "nomp", nullptr, nullptr},
                               {"simd", "nosimd", nullptr, nullptr},
                               {"sec", "nosec", nullptr, nullptr},
                               {"virt", "novirt", nullptr, nullptr},
                               {"fp16", "nofp16", "+fullfp16", "-fullfp16"},
                               {"fp16fml", "nofp16fml", "+fp16fml", "-fp16fml"},
                               {"ras", "noras", "+ras", "-ras"},
                               {"dotprod", "nodotprod", "+dotprod", "-dotprod"},
                               {"os", "noos", nullptr, nullptr},
                               {"iwmmxt", "noiwmmxt", nullptr, nullptr},
                               {"iwmmxt2", "noiwmmxt2", nullptr, nullptr},
                               {"maverick", "maverick", nullptr, nullptr},
                               {"xscale", "noxscale", nullptr, nullptr}};

   for (unsigned i = 0; i < array_lengthof(ArchExt); i++) {
      EXPECT_EQ(StringRef(ArchExt[i][2]), arm::get_arch_ext_feature(ArchExt[i][0]));
      EXPECT_EQ(StringRef(ArchExt[i][3]), arm::get_arch_ext_feature(ArchExt[i][1]));
   }
}

TEST(TargetParserTest, testARMparseHWDiv)
{
   const char *hwdiv[] = {"thumb", "arm", "arm,thumb", "thumb,arm"};
   for (unsigned i = 0; i < array_lengthof(hwdiv); i++) {
      EXPECT_NE(arm::AEK_INVALID, arm::parse_hw_div((StringRef)hwdiv[i]));
   }
}

TEST(TargetParserTest, testARMparseArchEndianAndISA)
{
   const char *Arch[] = {
      "v2",   "v2a",    "v3",    "v3m",    "v4",    "v4t",    "v5",    "v5t",
      "v5e",  "v5te",   "v5tej", "v6",     "v6j",   "v6k",    "v6hl",  "v6t2",
      "v6kz", "v6z",    "v6zk",  "v6-m",   "v6m",   "v6sm",   "v6s-m", "v7-a",
      "v7",   "v7a",    "v7ve",  "v7hl",   "v7l",   "v7-r",   "v7r",   "v7-m",
      "v7m",  "v7k",    "v7s",   "v7e-m",  "v7em",  "v8-a",   "v8",    "v8a",
      "v8l",  "v8.1-a", "v8.1a", "v8.2-a", "v8.2a", "v8.3-a", "v8.3a", "v8.4-a",
      "v8.4a", "v8.5-a","v8.5a", "v8-r"
   };

   for (unsigned i = 0; i < array_lengthof(Arch); i++) {
      std::string arm_1 = "armeb" + (std::string)(Arch[i]);
      std::string arm_2 = "arm" + (std::string)(Arch[i]) + "eb";
      std::string arm_3 = "arm" + (std::string)(Arch[i]);
      std::string thumb_1 = "thumbeb" + (std::string)(Arch[i]);
      std::string thumb_2 = "thumb" + (std::string)(Arch[i]) + "eb";
      std::string thumb_3 = "thumb" + (std::string)(Arch[i]);

      EXPECT_EQ(arm::EndianKind::BIG, arm::parse_arch_endian(arm_1));
      EXPECT_EQ(arm::EndianKind::BIG, arm::parse_arch_endian(arm_2));
      EXPECT_EQ(arm::EndianKind::LITTLE, arm::parse_arch_endian(arm_3));

      EXPECT_EQ(arm::ISAKind::ARM, arm::parse_arch_isa(arm_1));
      EXPECT_EQ(arm::ISAKind::ARM, arm::parse_arch_isa(arm_2));
      EXPECT_EQ(arm::ISAKind::ARM, arm::parse_arch_isa(arm_3));
      if (i >= 4) {
         EXPECT_EQ(arm::EndianKind::BIG, arm::parse_arch_endian(thumb_1));
         EXPECT_EQ(arm::EndianKind::BIG, arm::parse_arch_endian(thumb_2));
         EXPECT_EQ(arm::EndianKind::LITTLE, arm::parse_arch_endian(thumb_3));

         EXPECT_EQ(arm::ISAKind::THUMB, arm::parse_arch_isa(thumb_1));
         EXPECT_EQ(arm::ISAKind::THUMB, arm::parse_arch_isa(thumb_2));
         EXPECT_EQ(arm::ISAKind::THUMB, arm::parse_arch_isa(thumb_3));
      }
   }

   EXPECT_EQ(arm::EndianKind::LITTLE, arm::parse_arch_endian("aarch64"));
   EXPECT_EQ(arm::EndianKind::BIG, arm::parse_arch_endian("aarch64_be"));

   EXPECT_EQ(arm::ISAKind::AARCH64, arm::parse_arch_isa("aarch64"));
   EXPECT_EQ(arm::ISAKind::AARCH64, arm::parse_arch_isa("aarch64_be"));
   EXPECT_EQ(arm::ISAKind::AARCH64, arm::parse_arch_isa("arm64"));
   EXPECT_EQ(arm::ISAKind::AARCH64, arm::parse_arch_isa("arm64_be"));
}

TEST(TargetParserTest, testARMParseArchProfile) {
   for (unsigned i = 0; i < array_lengthof(ARMArch); i++) {
      switch (arm::parse_arch(ARMArch[i])) {
      case arm::ArchKind::ARMV6M:
      case arm::ArchKind::ARMV7M:
      case arm::ArchKind::ARMV7EM:
      case arm::ArchKind::ARMV8MMainline:
      case arm::ArchKind::ARMV8MBaseline:
         EXPECT_EQ(arm::ProfileKind::M, arm::parse_arch_profile(ARMArch[i]));
         break;
      case arm::ArchKind::ARMV7R:
      case arm::ArchKind::ARMV8R:
         EXPECT_EQ(arm::ProfileKind::R, arm::parse_arch_profile(ARMArch[i]));
         break;
      case arm::ArchKind::ARMV7A:
      case arm::ArchKind::ARMV7VE:
      case arm::ArchKind::ARMV7K:
      case arm::ArchKind::ARMV8A:
      case arm::ArchKind::ARMV8_1A:
      case arm::ArchKind::ARMV8_2A:
      case arm::ArchKind::ARMV8_3A:
      case arm::ArchKind::ARMV8_4A:
      case arm::ArchKind::ARMV8_5A:
         EXPECT_EQ(arm::ProfileKind::A, arm::parse_arch_profile(ARMArch[i]));
         break;
      default:
         EXPECT_EQ(arm::ProfileKind::INVALID, arm::parse_arch_profile(ARMArch[i]));
         break;
      }
   }
}

TEST(TargetParserTest, testARMparseArchVersion)
{
   for (unsigned i = 0; i < array_lengthof(ARMArch); i++) {
      if (((std::string)ARMArch[i]).substr(0, 4) == "armv") {
         EXPECT_EQ((ARMArch[i][4] - 48u), arm::parse_arch_version(ARMArch[i]));
      } else {
         EXPECT_EQ(5u, arm::parse_arch_version(ARMArch[i]));
      }
   }
}

bool test_aarch64_cpu(StringRef CPUName, StringRef ExpectedArch,
                      StringRef ExpectedFPU, unsigned ExpectedFlags,
                      StringRef CPUAttr)
{
   aarch64::ArchKind AK = aarch64::parse_cpu_arch(CPUName);
   bool pass = aarch64::get_arch_name(AK).equals(ExpectedArch);

   unsigned ExtKind = aarch64::get_default_extensions(CPUName, AK);
   if (ExtKind > 1 && (ExtKind & aarch64::AEK_NONE)) {
      pass &= ((ExtKind ^ aarch64::AEK_NONE) == ExpectedFlags);
   } else {
      pass &= (ExtKind == ExpectedFlags);
   }
   pass &= aarch64::get_cpu_attr(AK).equals(CPUAttr);

   return pass;
}

TEST(TargetParserTest, testAArch64CPU)
{
   EXPECT_TRUE(test_aarch64_cpu(
                  "invalid", "invalid", "invalid",
                  aarch64::AEK_NONE, ""));
   EXPECT_TRUE(test_aarch64_cpu(
                  "generic", "invalid", "none",
                  aarch64::AEK_NONE, ""));

   EXPECT_TRUE(test_aarch64_cpu(
                  "cortex-a35", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "cortex-a53", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "cortex-a55", "armv8.2-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD | aarch64::AEK_RAS | aarch64::AEK_LSE |
                  aarch64::AEK_RDM | aarch64::AEK_FP16 | aarch64::AEK_DOTPROD |
                  aarch64::AEK_RCPC, "8.2-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "cortex-a57", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "cortex-a72", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "cortex-a73", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "cortex-a75", "armv8.2-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD | aarch64::AEK_RAS | aarch64::AEK_LSE |
                  aarch64::AEK_RDM | aarch64::AEK_FP16 | aarch64::AEK_DOTPROD |
                  aarch64::AEK_RCPC, "8.2-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "cyclone", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRYPTO | aarch64::AEK_FP | aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "exynos-m1", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "exynos-m2", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "exynos-m3", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "exynos-m4", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "falkor", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD | aarch64::AEK_RDM, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "kryo", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD, "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "thunderx2t99", "armv8.1-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_LSE |
                  aarch64::AEK_RDM | aarch64::AEK_FP | aarch64::AEK_SIMD, "8.1-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "thunderx", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_SIMD |
                  aarch64::AEK_FP | aarch64::AEK_PROFILE,
                  "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "thunderxt81", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_SIMD |
                  aarch64::AEK_FP | aarch64::AEK_PROFILE,
                  "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "thunderxt83", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_SIMD |
                  aarch64::AEK_FP | aarch64::AEK_PROFILE,
                  "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "thunderxt88", "armv8-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_SIMD |
                  aarch64::AEK_FP | aarch64::AEK_PROFILE,
                  "8-A"));
   EXPECT_TRUE(test_aarch64_cpu(
                  "tsv110", "armv8.2-a", "crypto-neon-fp-armv8",
                  aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
                  aarch64::AEK_SIMD | aarch64::AEK_RAS | aarch64::AEK_LSE |
                  aarch64::AEK_RDM | aarch64::AEK_PROFILE | aarch64::AEK_FP16 |
                  aarch64::AEK_FP16FML | aarch64::AEK_DOTPROD,
                  "8.2-A"));
}

static constexpr unsigned NumAArch64CPUArchs = 21;

TEST(TargetParserTest, testAArch64CPUArchList)
{
   SmallVector<StringRef, NumAArch64CPUArchs> List;
   aarch64::fill_valid_cpu_arch_list(List);
   // No list exists for these in this test suite, so ensure all are
   // valid, and match the expected 'magic' count.
   EXPECT_EQ(List.size(), NumAArch64CPUArchs);
   for(StringRef CPU : List) {
      EXPECT_NE(aarch64::parse_cpu_arch(CPU), aarch64::ArchKind::INVALID);
   }
}

bool test_aarch64_arch(StringRef Arch, StringRef DefaultCPU, StringRef SubArch,
                       unsigned ArchAttr)
{
   aarch64::ArchKind AK = aarch64::parse_arch(Arch);
   return (AK != aarch64::ArchKind::INVALID) &
         aarch64::get_default_cpu(Arch).equals(DefaultCPU) &
         aarch64::get_sub_arch(AK).equals(SubArch) &
         (aarch64::get_arch_attr(AK) == ArchAttr);
}

TEST(TargetParserTest, testAArch64Arch)
{
   EXPECT_TRUE(test_aarch64_arch("armv8-a", "cortex-a53", "v8",
                                 armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(test_aarch64_arch("armv8.1-a", "generic", "v8.1a",
                                 armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(test_aarch64_arch("armv8.2-a", "generic", "v8.2a",
                                 armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(test_aarch64_arch("armv8.3-a", "generic", "v8.3a",
                                 armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(test_aarch64_arch("armv8.4-a", "generic", "v8.4a",
                                 armbuildattrs::CPUArch::v8_A));
   EXPECT_TRUE(test_aarch64_arch("armv8.5-a", "generic", "v8.5a",
                                 armbuildattrs::CPUArch::v8_A));
}

bool test_aarch64_extension(StringRef CPUName, aarch64::ArchKind AK,
                            StringRef ArchExt)
{
   return aarch64::get_default_extensions(CPUName, AK) &
         aarch64::parse_arch_ext(ArchExt);
}

TEST(TargetParserTest, test_aarch64_extension)
{
   EXPECT_FALSE(test_aarch64_extension("cortex-a35",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("cortex-a53",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_TRUE(test_aarch64_extension("cortex-a55",
                                      aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("cortex-a57",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("cortex-a72",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("cortex-a73",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_TRUE(test_aarch64_extension("cortex-a75",
                                      aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("cyclone",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("exynos-m1",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("exynos-m2",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("exynos-m3",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("exynos-m4",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_TRUE(test_aarch64_extension("falkor",
                                      aarch64::ArchKind::INVALID, "rdm"));
   EXPECT_FALSE(test_aarch64_extension("kryo",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_TRUE(test_aarch64_extension("saphira",
                                      aarch64::ArchKind::INVALID, "crc"));
   EXPECT_TRUE(test_aarch64_extension("saphira",
                                      aarch64::ArchKind::INVALID, "lse"));
   EXPECT_TRUE(test_aarch64_extension("saphira",
                                      aarch64::ArchKind::INVALID, "rdm"));
   EXPECT_TRUE(test_aarch64_extension("saphira",
                                      aarch64::ArchKind::INVALID, "ras"));
   EXPECT_TRUE(test_aarch64_extension("saphira",
                                      aarch64::ArchKind::INVALID, "rcpc"));
   EXPECT_TRUE(test_aarch64_extension("saphira",
                                      aarch64::ArchKind::INVALID, "profile"));
   EXPECT_FALSE(test_aarch64_extension("saphira",
                                       aarch64::ArchKind::INVALID, "fp16"));
   EXPECT_TRUE(test_aarch64_extension("cortex-a55",
                                      aarch64::ArchKind::INVALID, "fp16"));
   EXPECT_FALSE(test_aarch64_extension("cortex-a55",
                                       aarch64::ArchKind::INVALID, "fp16fml"));
   EXPECT_TRUE(test_aarch64_extension("cortex-a55",
                                      aarch64::ArchKind::INVALID, "dotprod"));
   EXPECT_TRUE(test_aarch64_extension("cortex-a75",
                                      aarch64::ArchKind::INVALID, "fp16"));
   EXPECT_FALSE(test_aarch64_extension("cortex-a75",
                                       aarch64::ArchKind::INVALID, "fp16fml"));
   EXPECT_TRUE(test_aarch64_extension("cortex-a75",
                                      aarch64::ArchKind::INVALID, "dotprod"));
   EXPECT_FALSE(test_aarch64_extension("thunderx2t99",
                                       aarch64::ArchKind::INVALID, "ras"));
   EXPECT_FALSE(test_aarch64_extension("thunderx",
                                       aarch64::ArchKind::INVALID, "lse"));
   EXPECT_FALSE(test_aarch64_extension("thunderxt81",
                                       aarch64::ArchKind::INVALID, "lse"));
   EXPECT_FALSE(test_aarch64_extension("thunderxt83",
                                       aarch64::ArchKind::INVALID, "lse"));
   EXPECT_FALSE(test_aarch64_extension("thunderxt88",
                                       aarch64::ArchKind::INVALID, "lse"));

   EXPECT_TRUE(test_aarch64_extension("tsv110",
                                      aarch64::ArchKind::INVALID, "crypto"));
   EXPECT_FALSE(test_aarch64_extension("tsv110",
                                       aarch64::ArchKind::INVALID, "sha3"));
   EXPECT_FALSE(test_aarch64_extension("tsv110",
                                       aarch64::ArchKind::INVALID, "sm4"));
   EXPECT_TRUE(test_aarch64_extension("tsv110",
                                      aarch64::ArchKind::INVALID, "ras"));
   EXPECT_TRUE(test_aarch64_extension("tsv110",
                                      aarch64::ArchKind::INVALID, "profile"));
   EXPECT_TRUE(test_aarch64_extension("tsv110",
                                      aarch64::ArchKind::INVALID, "fp16"));
   EXPECT_TRUE(test_aarch64_extension("tsv110",
                                      aarch64::ArchKind::INVALID, "fp16fml"));
   EXPECT_TRUE(test_aarch64_extension("tsv110",
                                      aarch64::ArchKind::INVALID, "dotprod"));

   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8A, "ras"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_1A, "ras"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_2A, "profile"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_2A, "fp16"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_2A, "fp16fml"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_3A, "fp16"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_3A, "fp16fml"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_4A, "fp16"));
   EXPECT_FALSE(test_aarch64_extension(
                   "generic", aarch64::ArchKind::ARMV8_4A, "fp16fml"));
}

TEST(TargetParserTest, testAArch64ExtensionFeatures)
{
   std::vector<StringRef> Features;
   unsigned extensions = aarch64::AEK_CRC | aarch64::AEK_CRYPTO |
         aarch64::AEK_FP | aarch64::AEK_SIMD |
         aarch64::AEK_FP16 | aarch64::AEK_PROFILE |
         aarch64::AEK_RAS | aarch64::AEK_LSE |
         aarch64::AEK_RDM | aarch64::AEK_SVE |
         aarch64::AEK_DOTPROD | aarch64::AEK_RCPC |
         aarch64::AEK_FP16FML;

   for (unsigned i = 0; i <= extensions; i++) {
      EXPECT_TRUE(i == 0 ? !aarch64::get_extension_features(i, Features)
                         : aarch64::get_extension_features(i, Features));
   }
}

TEST(TargetParserTest, testAArch64ArchFeatures)
{
   std::vector<StringRef> Features;
   aarch64::ArchKind ArchKinds[] = {
   #define AARCH64_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT)       \
      aarch64::ArchKind::ID,
   #include "polarphp/utils/AArch64TargetParserDefs.h"
   };

   for (auto AK : ArchKinds) {
      EXPECT_TRUE((AK == aarch64::ArchKind::INVALID)
                  ? !aarch64::get_arch_features(AK, Features)
                  : aarch64::get_arch_features(AK, Features));
   }
}

TEST(TargetParserTest, testAArch64ArchExtFeature)
{
   const char *ArchExt[][4] = {{"crc", "nocrc", "+crc", "-crc"},
                               {"crypto", "nocrypto", "+crypto", "-crypto"},
                               {"fp", "nofp", "+fp-armv8", "-fp-armv8"},
                               {"simd", "nosimd", "+neon", "-neon"},
                               {"fp16", "nofp16", "+fullfp16", "-fullfp16"},
                               {"fp16fml", "nofp16fml", "+fp16fml", "-fp16fml"},
                               {"profile", "noprofile", "+spe", "-spe"},
                               {"ras", "noras", "+ras", "-ras"},
                               {"lse", "nolse", "+lse", "-lse"},
                               {"rdm", "nordm", "+rdm", "-rdm"},
                               {"sve", "nosve", "+sve", "-sve"},
                               {"dotprod", "nodotprod", "+dotprod", "-dotprod"},
                               {"rcpc", "norcpc", "+rcpc", "-rcpc" },
                               {"rng", "norng", "+rand", "-rand"},
                               {"memtag", "nomemtag", "+mte", "-mte"},
                               {"ssbs", "nossbs", "+ssbs", "-ssbs"}};

   for (unsigned i = 0; i < array_lengthof(ArchExt); i++) {
      EXPECT_EQ(StringRef(ArchExt[i][2]),
            aarch64::get_arch_ext_feature(ArchExt[i][0]));
      EXPECT_EQ(StringRef(ArchExt[i][3]),
            aarch64::get_arch_ext_feature(ArchExt[i][1]));
   }
}

} // anonymous namespace
