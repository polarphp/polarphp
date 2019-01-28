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
// This file implements a target parser to recognise AArch64 hardware features
// such as FPU/CPU/ARCH and extension names.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_AARCH64_TARGET_PARSER_H
#define POLARPHP_UTILS_AARCH64_TARGET_PARSER_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/utils/ARMTargetParser.h"
#include <vector>

namespace polar {
// FIXME:This should be made into class design,to avoid dupplication.
namespace aarch64 {

using polar::basic::SmallVectorImpl;
using polar::basic::StringRef;
using polar::basic::Triple;

// arch extension modifiers for CPUs.
enum ArchExtKind : unsigned
{
   AEK_INVALID =     0,
   AEK_NONE =        1,
   AEK_CRC =         1 << 1,
   AEK_CRYPTO =      1 << 2,
   AEK_FP =          1 << 3,
   AEK_SIMD =        1 << 4,
   AEK_FP16 =        1 << 5,
   AEK_PROFILE =     1 << 6,
   AEK_RAS =         1 << 7,
   AEK_LSE =         1 << 8,
   AEK_SVE =         1 << 9,
   AEK_DOTPROD =     1 << 10,
   AEK_RCPC =        1 << 11,
   AEK_RDM =         1 << 12,
   AEK_SM4 =         1 << 13,
   AEK_SHA3 =        1 << 14,
   AEK_SHA2 =        1 << 15,
   AEK_AES =         1 << 16,
   AEK_FP16FML =     1 << 17,
   AEK_RAND =        1 << 18,
   AEK_MTE =         1 << 19,
   AEK_SSBS =        1 << 20,
};

// arch names.
enum class ArchKind
{
#define AARCH64_ARCH(NAME, ID, cpu_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT) ID,
#include "polarphp/utils/AArch64TargetParserDefs.h"
};

const arm::ArchNames<ArchKind> sg_aarch64ARCHNames[] = {
   #define AARCH64_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU,        \
      ARCH_BASE_EXT)                                            \
   {  NAME,                                                                       \
      sizeof(NAME) - 1,                                                           \
      CPU_ATTR,                                                                   \
      sizeof(CPU_ATTR) - 1,                                                       \
      SUB_ARCH,                                                                   \
      sizeof(SUB_ARCH) - 1,                                                       \
      arm::FPUKind::ARCH_FPU,                                                     \
      ARCH_BASE_EXT,                                                              \
      aarch64::ArchKind::ID,                                                      \
      ARCH_ATTR},
   #include "polarphp/utils/AArch64TargetParserDefs.h"
};

const arm::ExtName sg_aarch64ARCHExtNames[] = {
   #define AARCH64_ARCH_EXT_NAME(NAME, ID, FEATURE, NEGFEATURE)                   \
{NAME, sizeof(NAME) - 1, ID, FEATURE, NEGFEATURE},
   #include "polarphp/utils/AArch64TargetParserDefs.h"
};

const arm::CpuNames<ArchKind> sg_aarch64CPUNames[] = {
   #define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)       \
{NAME, sizeof(NAME) - 1, aarch64::ArchKind::ID, IS_DEFAULT, DEFAULT_EXT},
   #include "polarphp/utils/AArch64TargetParserDefs.h"
};

const ArchKind sg_archKinds[] = {
   #define AARCH64_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT) \
      ArchKind::ID,
   #include "polarphp/utils/AArch64TargetParserDefs.h"
};

bool get_extension_features(unsigned extensions,
                            std::vector<StringRef> &features);
bool get_arch_features(ArchKind ak, std::vector<StringRef> &features);

StringRef get_arch_name(ArchKind ak);
unsigned get_arch_attr(ArchKind ak);
StringRef get_cpu_attr(ArchKind ak);
StringRef get_sub_arch(ArchKind ak);
StringRef get_arch_ext_name(unsigned archExtKind);
StringRef get_arch_ext_feature(StringRef archExt);

// Information by Name
unsigned  get_default_fpu(StringRef cpu, ArchKind ak);
unsigned  get_default_extensions(StringRef cpu, ArchKind ak);
StringRef get_default_cpu(StringRef arch);
aarch64::ArchKind get_cpu_arch_kind(StringRef cpu);

// Parser
aarch64::ArchKind parse_arch(StringRef arch);
aarch64::ArchExtKind parse_arch_ext(StringRef archExt);
ArchKind parse_cpu_arch(StringRef cpu);
void fill_valid_cpu_arch_list(SmallVectorImpl<StringRef> &values);

// Used by target parser tests
void fill_valid_cpu_arch_list(SmallVectorImpl<StringRef> &Values);
bool is_x18_reserved_by_default(const Triple &tt);

} // namespace AArch64
} // polar

#endif // POLARPHP_UTILS_AARCH64_TARGET_PARSER_H
