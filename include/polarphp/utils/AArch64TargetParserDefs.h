// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarPHP software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarPHP project authors
//
// Created by polarboy on 2018/06/18.

//===----------------------------------------------------------------------===//
//
// This file provides defines to build up the AARCH64 target parser's logic.
//
//===----------------------------------------------------------------------===//

// NOTE: NO INCLUDE GUARD DESIRED!


#ifndef AARCH64_ARCH
#define AARCH64_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT)
#endif
AARCH64_ARCH("invalid", INVALID, "", "",
             armbuildattrs::CPUArch::v8_A, FK_NONE, aarch64::AEK_NONE)
AARCH64_ARCH("armv8-a", ARMV8A, "8-A", "v8", armbuildattrs::CPUArch::v8_A,
             FK_CRYPTO_NEON_FP_ARMV8,
             (aarch64::AEK_CRYPTO | aarch64::AEK_FP | aarch64::AEK_SIMD))
AARCH64_ARCH("armv8.1-a", ARMV8_1A, "8.1-A", "v8.1a",
             armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
             (aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
              aarch64::AEK_SIMD | aarch64::AEK_LSE | aarch64::AEK_RDM))
AARCH64_ARCH("armv8.2-a", ARMV8_2A, "8.2-A", "v8.2a",
             armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
             (aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
              aarch64::AEK_SIMD | aarch64::AEK_RAS | aarch64::AEK_LSE |
              aarch64::AEK_RDM))
AARCH64_ARCH("armv8.3-a", ARMV8_3A, "8.3-A", "v8.3a",
             armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
             (aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
              aarch64::AEK_SIMD | aarch64::AEK_RAS | aarch64::AEK_LSE |
              aarch64::AEK_RDM | aarch64::AEK_RCPC))
AARCH64_ARCH("armv8.4-a", ARMV8_4A, "8.4-A", "v8.4a",
             armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
             (aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
              aarch64::AEK_SIMD | aarch64::AEK_RAS | aarch64::AEK_LSE |
              aarch64::AEK_RDM | aarch64::AEK_RCPC | aarch64::AEK_DOTPROD))
AARCH64_ARCH("armv8.5-a", ARMV8_5A, "8.5-A", "v8.5a",
             armbuildattrs::CPUArch::v8_A, FK_CRYPTO_NEON_FP_ARMV8,
             (aarch64::AEK_CRC | aarch64::AEK_CRYPTO | aarch64::AEK_FP |
              aarch64::AEK_SIMD | aarch64::AEK_RAS | aarch64::AEK_LSE |
              aarch64::AEK_RDM | aarch64::AEK_RCPC | aarch64::AEK_DOTPROD))
#undef AARCH64_ARCH

#ifndef AARCH64_ARCH_EXT_NAME
#define AARCH64_ARCH_EXT_NAME(NAME, ID, FEATURE, NEGFEATURE)
#endif
// FIXME: This would be nicer were it tablegen
AARCH64_ARCH_EXT_NAME("invalid",  aarch64::AEK_INVALID,  nullptr,  nullptr)
AARCH64_ARCH_EXT_NAME("none",     aarch64::AEK_NONE,     nullptr,  nullptr)
AARCH64_ARCH_EXT_NAME("crc",      aarch64::AEK_CRC,      "+crc",   "-crc")
AARCH64_ARCH_EXT_NAME("lse",      aarch64::AEK_LSE,      "+lse",   "-lse")
AARCH64_ARCH_EXT_NAME("rdm",      aarch64::AEK_RDM,      "+rdm",   "-rdm")
AARCH64_ARCH_EXT_NAME("crypto",   aarch64::AEK_CRYPTO,   "+crypto","-crypto")
AARCH64_ARCH_EXT_NAME("sm4",      aarch64::AEK_SM4,      "+sm4",   "-sm4")
AARCH64_ARCH_EXT_NAME("sha3",     aarch64::AEK_SHA3,     "+sha3",  "-sha3")
AARCH64_ARCH_EXT_NAME("sha2",     aarch64::AEK_SHA2,     "+sha2",  "-sha2")
AARCH64_ARCH_EXT_NAME("aes",      aarch64::AEK_AES,      "+aes",   "-aes")
AARCH64_ARCH_EXT_NAME("dotprod",  aarch64::AEK_DOTPROD,  "+dotprod","-dotprod")
AARCH64_ARCH_EXT_NAME("fp",       aarch64::AEK_FP,       "+fp-armv8",  "-fp-armv8")
AARCH64_ARCH_EXT_NAME("simd",     aarch64::AEK_SIMD,     "+neon",  "-neon")
AARCH64_ARCH_EXT_NAME("fp16",     aarch64::AEK_FP16,     "+fullfp16",  "-fullfp16")
AARCH64_ARCH_EXT_NAME("fp16fml",  aarch64::AEK_FP16FML,  "+fp16fml", "-fp16fml")
AARCH64_ARCH_EXT_NAME("profile",  aarch64::AEK_PROFILE,  "+spe",  "-spe")
AARCH64_ARCH_EXT_NAME("ras",      aarch64::AEK_RAS,      "+ras",  "-ras")
AARCH64_ARCH_EXT_NAME("sve",      aarch64::AEK_SVE,      "+sve",  "-sve")
AARCH64_ARCH_EXT_NAME("rcpc",     aarch64::AEK_RCPC,     "+rcpc", "-rcpc")
AARCH64_ARCH_EXT_NAME("rng",      aarch64::AEK_RAND,     "+rand",  "-rand")
AARCH64_ARCH_EXT_NAME("memtag",   aarch64::AEK_MTE,      "+mte",   "-mte")
AARCH64_ARCH_EXT_NAME("ssbs",     aarch64::AEK_SSBS,     "+ssbs",  "-ssbs")
#undef AARCH64_ARCH_EXT_NAME

#ifndef AARCH64_CPU_NAME
#define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)
#endif
AARCH64_CPU_NAME("cortex-a35", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("cortex-a53", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, true,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("cortex-a55", ARMV8_2A, FK_CRYPTO_NEON_FP_ARMV8, false,
                 (aarch64::AEK_FP16 | aarch64::AEK_DOTPROD | aarch64::AEK_RCPC))
AARCH64_CPU_NAME("cortex-a57", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("cortex-a72", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("cortex-a73", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("cortex-a75", ARMV8_2A, FK_CRYPTO_NEON_FP_ARMV8, false,
                 (aarch64::AEK_FP16 | aarch64::AEK_DOTPROD | aarch64::AEK_RCPC))
AARCH64_CPU_NAME("cyclone", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_NONE))
AARCH64_CPU_NAME("exynos-m1", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("exynos-m2", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("exynos-m3", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("exynos-m4", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("falkor", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC | aarch64::AEK_RDM))
AARCH64_CPU_NAME("saphira", ARMV8_3A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_PROFILE))
AARCH64_CPU_NAME("kryo", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC))
AARCH64_CPU_NAME("thunderx2t99", ARMV8_1A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_NONE))
AARCH64_CPU_NAME("thunderx", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC | aarch64::AEK_PROFILE))
AARCH64_CPU_NAME("thunderxt88", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC | aarch64::AEK_PROFILE))
AARCH64_CPU_NAME("thunderxt81", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC | aarch64::AEK_PROFILE))
AARCH64_CPU_NAME("thunderxt83", ARMV8A, FK_CRYPTO_NEON_FP_ARMV8, false,
                (aarch64::AEK_CRC | aarch64::AEK_PROFILE))
AARCH64_CPU_NAME("tsv110", ARMV8_2A, FK_CRYPTO_NEON_FP_ARMV8, false,
                 (aarch64::AEK_PROFILE | aarch64::AEK_FP16 | aarch64::AEK_FP16FML |
                  aarch64::AEK_DOTPROD))
// Invalid CPU
AARCH64_CPU_NAME("invalid", INVALID, FK_INVALID, true, aarch64::AEK_INVALID)
#undef AARCH64_CPU_NAME
