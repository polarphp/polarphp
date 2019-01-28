// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/10/17.

//===- ARMTargetParser.def - ARM target parsing defines ---------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file provides defines to build up the ARM target parser's logic.
//
//===----------------------------------------------------------------------===//

// NOTE: NO INCLUDE GUARD DESIRED!

#ifndef ARM_FPU
#define ARM_FPU(NAME, KIND, VERSION, NEON_SUPPORT, RESTRICTION)
#endif
ARM_FPU("invalid", FK_INVALID, FPUVersion::NONE, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("none", FK_NONE, FPUVersion::NONE, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("vfp", FK_VFP, FPUVersion::VFPV2, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("vfpv2", FK_VFPV2, FPUVersion::VFPV2, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("vfpv3", FK_VFPV3, FPUVersion::VFPV3, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("vfpv3-fp16", FK_VFPV3_FP16, FPUVersion::VFPV3_FP16, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("vfpv3-d16", FK_VFPV3_D16, FPUVersion::VFPV3, NeonSupportLevel::None, FPURestriction::D16)
ARM_FPU("vfpv3-d16-fp16", FK_VFPV3_D16_FP16, FPUVersion::VFPV3_FP16, NeonSupportLevel::None, FPURestriction::D16)
ARM_FPU("vfpv3xd", FK_VFPV3XD, FPUVersion::VFPV3, NeonSupportLevel::None, FPURestriction::SP_D16)
ARM_FPU("vfpv3xd-fp16", FK_VFPV3XD_FP16, FPUVersion::VFPV3_FP16, NeonSupportLevel::None, FPURestriction::SP_D16)
ARM_FPU("vfpv4", FK_VFPV4, FPUVersion::VFPV4, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("vfpv4-d16", FK_VFPV4_D16, FPUVersion::VFPV4, NeonSupportLevel::None, FPURestriction::D16)
ARM_FPU("fpv4-sp-d16", FK_FPV4_SP_D16, FPUVersion::VFPV4, NeonSupportLevel::None, FPURestriction::SP_D16)
ARM_FPU("fpv5-d16", FK_FPV5_D16, FPUVersion::VFPV5, NeonSupportLevel::None, FPURestriction::D16)
ARM_FPU("fpv5-sp-d16", FK_FPV5_SP_D16, FPUVersion::VFPV5, NeonSupportLevel::None, FPURestriction::SP_D16)
ARM_FPU("fp-armv8", FK_FP_ARMV8, FPUVersion::VFPV5, NeonSupportLevel::None, FPURestriction::None)
ARM_FPU("neon", FK_NEON, FPUVersion::VFPV3, NeonSupportLevel::Neon, FPURestriction::None)
ARM_FPU("neon-fp16", FK_NEON_FP16, FPUVersion::VFPV3_FP16, NeonSupportLevel::Neon, FPURestriction::None)
ARM_FPU("neon-vfpv4", FK_NEON_VFPV4, FPUVersion::VFPV4, NeonSupportLevel::Neon, FPURestriction::None)
ARM_FPU("neon-fp-armv8", FK_NEON_FP_ARMV8, FPUVersion::VFPV5, NeonSupportLevel::Neon, FPURestriction::None)
ARM_FPU("crypto-neon-fp-armv8", FK_CRYPTO_NEON_FP_ARMV8, FPUVersion::VFPV5, NeonSupportLevel::Crypto,
        FPURestriction::None)
ARM_FPU("softvfp", FK_SOFTVFP, FPUVersion::NONE, NeonSupportLevel::None, FPURestriction::None)
#undef ARM_FPU

#ifndef ARM_ARCH
#define ARM_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT)
#endif
ARM_ARCH("invalid", INVALID, "", "",
          armbuildattrs::CPUArch::Pre_v4, FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv2", ARMV2, "2", "v2", armbuildattrs::CPUArch::Pre_v4,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv2a", ARMV2A, "2A", "v2a", armbuildattrs::CPUArch::Pre_v4,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv3", ARMV3, "3", "v3", armbuildattrs::CPUArch::Pre_v4,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv3m", ARMV3M, "3M", "v3m", armbuildattrs::CPUArch::Pre_v4,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv4", ARMV4, "4", "v4", armbuildattrs::CPUArch::v4,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv4t", ARMV4T, "4T", "v4t", armbuildattrs::CPUArch::v4T,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv5t", ARMV5T, "5T", "v5", armbuildattrs::CPUArch::v5T,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv5te", ARMV5TE, "5TE", "v5e", armbuildattrs::CPUArch::v5TE,
          FK_NONE, arm::AEK_DSP)
ARM_ARCH("armv5tej", ARMV5TEJ, "5TEJ", "v5e", armbuildattrs::CPUArch::v5TEJ,
          FK_NONE, arm::AEK_DSP)
ARM_ARCH("armv6", ARMV6, "6", "v6", armbuildattrs::CPUArch::v6,
          FK_VFPV2, arm::AEK_DSP)
ARM_ARCH("armv6k", ARMV6K, "6K", "v6k", armbuildattrs::CPUArch::v6K,
          FK_VFPV2, arm::AEK_DSP)
ARM_ARCH("armv6t2", ARMV6T2, "6T2", "v6t2", armbuildattrs::CPUArch::v6T2,
          FK_NONE, arm::AEK_DSP)
ARM_ARCH("armv6kz", ARMV6KZ, "6KZ", "v6kz", armbuildattrs::CPUArch::v6KZ,
          FK_VFPV2, (arm::AEK_SEC | arm::AEK_DSP))
ARM_ARCH("armv6-m", ARMV6M, "6-M", "v6m", armbuildattrs::CPUArch::v6_M,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv7-a", ARMV7A, "7-A", "v7", armbuildattrs::CPUArch::v7,
          FK_NEON, arm::AEK_DSP)
ARM_ARCH("armv7ve", ARMV7VE, "7VE", "v7ve", armbuildattrs::CPUArch::v7,
          FK_NEON, (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT |
          arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB | arm::AEK_DSP))
ARM_ARCH("armv7-r", ARMV7R, "7-R", "v7r", armbuildattrs::CPUArch::v7,
          FK_NONE, (arm::AEK_HWDIVTHUMB | arm::AEK_DSP))
ARM_ARCH("armv7-m", ARMV7M, "7-M", "v7m", armbuildattrs::CPUArch::v7,
          FK_NONE, arm::AEK_HWDIVTHUMB)
ARM_ARCH("armv7e-m", ARMV7EM, "7E-M", "v7em", armbuildattrs::CPUArch::v7E_M,
          FK_NONE, (arm::AEK_HWDIVTHUMB | arm::AEK_DSP))
ARM_ARCH("armv8-a", ARMV8A, "8-A", "v8", armbuildattrs::CPUArch::v8_A,
         FK_CRYPTO_NEON_FP_ARMV8,
         (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
          arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_CRC))
ARM_ARCH("armv8.1-a", ARMV8_1A, "8.1-A", "v8.1a",
         armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
         (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
          arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_CRC))
ARM_ARCH("armv8.2-a", ARMV8_2A, "8.2-A", "v8.2a",
         armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
         (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
          arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_CRC | arm::AEK_RAS))
ARM_ARCH("armv8.3-a", ARMV8_3A, "8.3-A", "v8.3a",
         armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
         (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
          arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_CRC | arm::AEK_RAS))
ARM_ARCH("armv8.4-a", ARMV8_4A, "8.4-A", "v8.4a",
         armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
         (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
          arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_CRC | arm::AEK_RAS |
          arm::AEK_DOTPROD))
ARM_ARCH("armv8.5-a", ARMV8_5A, "8.5-A", "v8.5a",
         armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
         (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
          arm::AEK_HWDIVTHUMB | arm::AEK_DSP | arm::AEK_CRC | arm::AEK_RAS |
          arm::AEK_DOTPROD))
ARM_ARCH("armv8-r", ARMV8R, "8-R", "v8r", armbuildattrs::CPUArch::v8_R,
          FK_NEON_FP_ARMV8,
          (arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB |
           arm::AEK_DSP | arm::AEK_CRC))
ARM_ARCH("armv8-m.base", ARMV8MBaseline, "8-M.Baseline", "v8m.base",
          armbuildattrs::CPUArch::v8_M_Base, FK_NONE, arm::AEK_HWDIVTHUMB)
ARM_ARCH("armv8-m.main", ARMV8MMainline, "8-M.Mainline", "v8m.main",
          armbuildattrs::CPUArch::v8_M_Main, FK_FPV5_D16, arm::AEK_HWDIVTHUMB)
// Non-standard Arch names.
ARM_ARCH("iwmmxt", IWMMXT, "iwmmxt", "", armbuildattrs::CPUArch::v5TE,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("iwmmxt2", IWMMXT2, "iwmmxt2", "", armbuildattrs::CPUArch::v5TE,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("xscale", XSCALE, "xscale", "v5e", armbuildattrs::CPUArch::v5TE,
          FK_NONE, arm::AEK_NONE)
ARM_ARCH("armv7s", ARMV7S, "7-S", "v7s", armbuildattrs::CPUArch::v7,
          FK_NEON_VFPV4, arm::AEK_DSP)
ARM_ARCH("armv7k", ARMV7K, "7-K", "v7k", armbuildattrs::CPUArch::v7,
          FK_NONE, arm::AEK_DSP)
#undef ARM_ARCH

#ifndef ARM_ARCH_EXT_NAME
#define ARM_ARCH_EXT_NAME(NAME, ID, FEATURE, NEGFEATURE)
#endif
// FIXME: This would be nicer were it tablegen
ARM_ARCH_EXT_NAME("invalid",  arm::AEK_INVALID,  nullptr,  nullptr)
ARM_ARCH_EXT_NAME("none",     arm::AEK_NONE,     nullptr,  nullptr)
ARM_ARCH_EXT_NAME("crc",      arm::AEK_CRC,      "+crc",   "-crc")
ARM_ARCH_EXT_NAME("crypto",   arm::AEK_CRYPTO,   "+crypto","-crypto")
ARM_ARCH_EXT_NAME("sha2",     arm::AEK_SHA2,     "+sha2",  "-sha2")
ARM_ARCH_EXT_NAME("aes",      arm::AEK_AES,      "+aes",   "-aes")
ARM_ARCH_EXT_NAME("dotprod",  arm::AEK_DOTPROD,  "+dotprod","-dotprod")
ARM_ARCH_EXT_NAME("dsp",      arm::AEK_DSP,      "+dsp",   "-dsp")
ARM_ARCH_EXT_NAME("fp",       arm::AEK_FP,       nullptr,  nullptr)
ARM_ARCH_EXT_NAME("idiv",     (arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB), nullptr, nullptr)
ARM_ARCH_EXT_NAME("mp",       arm::AEK_MP,       nullptr,  nullptr)
ARM_ARCH_EXT_NAME("simd",     arm::AEK_SIMD,     nullptr,  nullptr)
ARM_ARCH_EXT_NAME("sec",      arm::AEK_SEC,      nullptr,  nullptr)
ARM_ARCH_EXT_NAME("virt",     arm::AEK_VIRT,     nullptr,  nullptr)
ARM_ARCH_EXT_NAME("fp16",     arm::AEK_FP16,     "+fullfp16",  "-fullfp16")
ARM_ARCH_EXT_NAME("ras",      arm::AEK_RAS,      "+ras", "-ras")
ARM_ARCH_EXT_NAME("os",       arm::AEK_OS,       nullptr,  nullptr)
ARM_ARCH_EXT_NAME("iwmmxt",   arm::AEK_IWMMXT,   nullptr,  nullptr)
ARM_ARCH_EXT_NAME("iwmmxt2",  arm::AEK_IWMMXT2,  nullptr,  nullptr)
ARM_ARCH_EXT_NAME("maverick", arm::AEK_MAVERICK, nullptr,  nullptr)
ARM_ARCH_EXT_NAME("xscale",   arm::AEK_XSCALE,   nullptr,  nullptr)
ARM_ARCH_EXT_NAME("fp16fml",  arm::AEK_FP16FML,  "+fp16fml", "-fp16fml")
#undef ARM_ARCH_EXT_NAME

#ifndef ARM_HW_DIV_NAME
#define ARM_HW_DIV_NAME(NAME, ID)
#endif
ARM_HW_DIV_NAME("invalid", arm::AEK_INVALID)
ARM_HW_DIV_NAME("none", arm::AEK_NONE)
ARM_HW_DIV_NAME("thumb", arm::AEK_HWDIVTHUMB)
ARM_HW_DIV_NAME("arm", arm::AEK_HWDIVARM)
ARM_HW_DIV_NAME("arm,thumb", (arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB))
#undef ARM_HW_DIV_NAME

#ifndef ARM_CPU_NAME
#define ARM_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)
#endif
ARM_CPU_NAME("arm2", ARMV2, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm3", ARMV2A, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm6", ARMV3, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm7m", ARMV3M, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm8", ARMV4, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm810", ARMV4, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("strongarm", ARMV4, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("strongarm110", ARMV4, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("strongarm1100", ARMV4, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("strongarm1110", ARMV4, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm7tdmi", ARMV4T, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm7tdmi-s", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm710t", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm720t", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm9", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm9tdmi", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm920", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm920t", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm922t", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm9312", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm940t", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("ep9312", ARMV4T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm10tdmi", ARMV5T, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm1020t", ARMV5T, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm9e", ARMV5TE, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm946e-s", ARMV5TE, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm966e-s", ARMV5TE, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm968e-s", ARMV5TE, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm10e", ARMV5TE, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm1020e", ARMV5TE, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm1022e", ARMV5TE, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm926ej-s", ARMV5TEJ, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm1136j-s", ARMV6, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm1136jf-s", ARMV6, FK_VFPV2, true, arm::AEK_NONE)
ARM_CPU_NAME("arm1136jz-s", ARMV6, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("mpcore", ARMV6K, FK_VFPV2, true, arm::AEK_NONE)
ARM_CPU_NAME("mpcorenovfp", ARMV6K, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm1176jz-s", ARMV6KZ, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("arm1176jzf-s", ARMV6KZ, FK_VFPV2, true, arm::AEK_NONE)
ARM_CPU_NAME("arm1156t2-s", ARMV6T2, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("arm1156t2f-s", ARMV6T2, FK_VFPV2, false, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m0", ARMV6M, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m0plus", ARMV6M, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m1", ARMV6M, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("sc000", ARMV6M, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("cortex-a5", ARMV7A, FK_NEON_VFPV4, false,
             (arm::AEK_SEC | arm::AEK_MP))
ARM_CPU_NAME("cortex-a7", ARMV7A, FK_NEON_VFPV4, false,
             (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
              arm::AEK_HWDIVTHUMB))
ARM_CPU_NAME("cortex-a8", ARMV7A, FK_NEON, false, arm::AEK_SEC)
ARM_CPU_NAME("cortex-a9", ARMV7A, FK_NEON_FP16, false, (arm::AEK_SEC | arm::AEK_MP))
ARM_CPU_NAME("cortex-a12", ARMV7A, FK_NEON_VFPV4, false,
             (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
              arm::AEK_HWDIVTHUMB))
ARM_CPU_NAME("cortex-a15", ARMV7A, FK_NEON_VFPV4, false,
             (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
              arm::AEK_HWDIVTHUMB))
ARM_CPU_NAME("cortex-a17", ARMV7A, FK_NEON_VFPV4, false,
             (arm::AEK_SEC | arm::AEK_MP | arm::AEK_VIRT | arm::AEK_HWDIVARM |
              arm::AEK_HWDIVTHUMB))
ARM_CPU_NAME("krait", ARMV7A, FK_NEON_VFPV4, false,
             (arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB))
ARM_CPU_NAME("cortex-r4", ARMV7R, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("cortex-r4f", ARMV7R, FK_VFPV3_D16, false, arm::AEK_NONE)
ARM_CPU_NAME("cortex-r5", ARMV7R, FK_VFPV3_D16, false,
             (arm::AEK_MP | arm::AEK_HWDIVARM))
ARM_CPU_NAME("cortex-r7", ARMV7R, FK_VFPV3_D16_FP16, false,
             (arm::AEK_MP | arm::AEK_HWDIVARM))
ARM_CPU_NAME("cortex-r8", ARMV7R, FK_VFPV3_D16_FP16, false,
             (arm::AEK_MP | arm::AEK_HWDIVARM))
ARM_CPU_NAME("cortex-r52", ARMV8R, FK_NEON_FP_ARMV8, true, arm::AEK_NONE)
ARM_CPU_NAME("sc300", ARMV7M, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m3", ARMV7M, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m4", ARMV7EM, FK_FPV4_SP_D16, true, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m7", ARMV7EM, FK_FPV5_D16, false, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m23", ARMV8MBaseline, FK_NONE, false, arm::AEK_NONE)
ARM_CPU_NAME("cortex-m33", ARMV8MMainline, FK_FPV5_SP_D16, false, arm::AEK_DSP)
ARM_CPU_NAME("cortex-a32", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("cortex-a35", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("cortex-a53", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("cortex-a55", ARMV8_2A, FK_CRYPTO_NEON_FP_ARMV8, false,
            (arm::AEK_FP16 | arm::AEK_DOTPROD))
ARM_CPU_NAME("cortex-a57", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("cortex-a72", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("cortex-a73", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("cortex-a75", ARMV8_2A, FK_CRYPTO_NEON_FP_ARMV8, false,
            (arm::AEK_FP16 | arm::AEK_DOTPROD))
ARM_CPU_NAME("cyclone", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("exynos-m1", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("exynos-m2", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("exynos-m3", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("exynos-m4", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
ARM_CPU_NAME("kryo", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false, arm::AEK_CRC)
// Non-standard Arch names.
ARM_CPU_NAME("iwmmxt", IWMMXT, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("xscale", XSCALE, FK_NONE, true, arm::AEK_NONE)
ARM_CPU_NAME("swift", ARMV7S, FK_NEON_VFPV4, true,
             (arm::AEK_HWDIVARM | arm::AEK_HWDIVTHUMB))
// Invalid CPU
ARM_CPU_NAME("invalid", INVALID, FK_INVALID, true, arm::AEK_INVALID)
#undef ARM_CPU_NAME
