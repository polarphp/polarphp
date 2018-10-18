// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/06/18.

//===----------------------------------------------------------------------===//
//
// This file implements a target parser to recognise hardware features such as
// FPU/cpu/ARCH names as well as specific support such as HDIV, etc.
//
//===----------------------------------------------------------------------===//

#ifndef POLARPHP_UTILS_TARGET_PARSER_H
#define POLARPHP_UTILS_TARGET_PARSER_H

// FIXME: vector is used because that's what clang uses for subtarget feature
// lists, but SmallVector would probably be better
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/basic/adt/SmallVector.h"
#include <vector>

namespace polar {

// forward declare class with namespace
namespace basic {
class StringRef;
class Triple;
} // basic

using polar::basic::StringRef;
using polar::basic::Triple;
using polar::basic::SmallVectorImpl;

// Target specific information into their own namespaces. These should be
// generated from TableGen because the information is already there, and there
// is where new information about targets will be added.
// FIXME: To TableGen this we need to make some table generated files available
// even if the back-end is not compiled with LLVM, plus we need to create a new
// back-end to TableGen to create these clean tables.
namespace arm {

// FPU Version
enum class FPUVersion
{
   NONE,
   VFPV2,
   VFPV3,
   VFPV3_FP16,
   VFPV4,
   VFPV5
};

// An FPU name restricts the FPU in one of three ways:
enum class FPURestriction
{
   None = 0, ///< No restriction
   D16,      ///< Only 16 D registers
   SP_D16    ///< Only single-precision instructions, with 16 D registers
};

// An FPU name implies one of three levels of Neon support:
enum class NeonSupportLevel
{
   None = 0, ///< No Neon
   Neon,     ///< Neon
   Crypto    ///< Neon with Crypto
};

// FPU names.
enum fpuKind
{
#define ARM_FPU(NAME, KIND, VERSION, NEON_SUPPORT, RESTRICTION) KIND,
#include "polarphp/utils/ARMTargetParser.h"
   FK_LAST
};

// arch names.
enum class ArchKind
{
#define ARM_ARCH(NAME, ID, cpu_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT) ID,
#include "polarphp/utils/ARMTargetParser.h"
};

// arch extension modifiers for CPUs.
enum archExtKind : unsigned
{
   AEK_INVALID =     0,
   AEK_NONE =        1,
   AEK_CRC =         1 << 1,
   AEK_CRYPTO =      1 << 2,
   AEK_FP =          1 << 3,
   AEK_HWDIVTHUMB =  1 << 4,
   AEK_HWDIVARM =    1 << 5,
   AEK_MP =          1 << 6,
   AEK_SIMD =        1 << 7,
   AEK_SEC =         1 << 8,
   AEK_VIRT =        1 << 9,
   AEK_DSP =         1 << 10,
   AEK_FP16 =        1 << 11,
   AEK_RAS =         1 << 12,
   AEK_SVE =         1 << 13,
   AEK_DOTPROD =     1 << 14,
   AEK_SHA2    =     1 << 15,
   AEK_AES     =     1 << 16,
   // Unsupported extensions.
   AEK_OS = 0x8000000,
   AEK_IWMMXT = 0x10000000,
   AEK_IWMMXT2 = 0x20000000,
   AEK_MAVERICK = 0x40000000,
   AEK_XSCALE = 0x80000000,
};

// ISA kinds.
enum class ISAKind
{
   INVALID = 0,
   ARM,
   THUMB,
   AARCH64
};

// Endianness
// FIXME: BE8 vs. BE32?
enum class EndianKind
{
   INVALID = 0,
   LITTLE,
   BIG
};

// v6/v7/v8 profile
enum class profileKind
{
   INVALID = 0, A, R, M
};

StringRef get_canonical_arch_name(StringRef arch);

// Information by ID
StringRef get_fpu_name(unsigned fpuKind);
FPUVersion get_fpu_version(unsigned fpuKind);
NeonSupportLevel get_fpu_neon_support_level(unsigned fpuKind);
FPURestriction get_fpu_restriction(unsigned fpuKind);

// FIXME: These should be moved to TargetTuple once it exists
bool get_fpu_features(unsigned fpuKind, std::vector<StringRef> &features);
bool get_hw_div_features(unsigned hwDivKind, std::vector<StringRef> &features);
bool get_extension_features(unsigned extensions,
                            std::vector<StringRef> &features);

StringRef get_arch_name(ArchKind archKind);
unsigned get_arch_attr(ArchKind archKind);
StringRef get_cpu_attr(ArchKind archKind);
StringRef get_sub_arch(ArchKind archKind);
StringRef get_arch_ext_name(unsigned archExtKind);
StringRef get_arch_ext_feature(StringRef archExt);
StringRef get_hw_div_name(unsigned hwDivKind);

// Information by Name
unsigned  get_default_fpu(StringRef cpu, ArchKind archKind);
unsigned  get_default_extensions(StringRef cpu, ArchKind archKind);
StringRef get_default_cpu(StringRef arch);

// Parser
unsigned parse_hw_div(StringRef hwDiv);
unsigned parse_fpu(StringRef FPU);
ArchKind parse_arch(StringRef arch);
unsigned parse_arch_ext(StringRef archExt);
ArchKind parse_cpu_arch(StringRef cpu);
void fill_valid_cpu_arch_list(SmallVectorImpl<StringRef> &values);
ISAKind parse_arch_isa(StringRef arch);
EndianKind parse_arch_endian(StringRef arch);
profileKind parse_arch_profile(StringRef arch);
unsigned parse_arch_version(StringRef arch);

StringRef compute_default_target_abi(const Triple &TT, StringRef cpu);

} // namespace ARM

// FIXME:This should be made into class design,to avoid dupplication.
namespace aarch64 {

// arch names.
enum class ArchKind
{
#define AARCH64_ARCH(NAME, ID, cpu_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT) ID,
#include "AArch64TargetParser.h"
};

// arch extension modifiers for CPUs.
enum archExtKind : unsigned
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
};

StringRef get_canonical_arch_name(StringRef arch);

// Information by ID
StringRef get_fpu_name(unsigned fpuKind);
arm::FPUVersion get_fpu_version(unsigned fpuKind);
arm::NeonSupportLevel get_fpu_neon_support_level(unsigned fpuKind);
arm::FPURestriction get_fpu_restriction(unsigned fpuKind);

// FIXME: These should be moved to TargetTuple once it exists
bool get_fpu_features(unsigned fpuKind, std::vector<StringRef> &features);
bool get_extension_features(unsigned extensions,
                            std::vector<StringRef> &features);
bool get_arch_features(ArchKind ak, std::vector<StringRef> &features);

StringRef get_arch_name(ArchKind ak);
unsigned get_arch_attr(ArchKind ak);
StringRef get_cpu_attr(ArchKind ak);
StringRef get_sub_arch(ArchKind ak);
StringRef get_arch_ext_name(unsigned archExtKind);
StringRef get_arch_ext_feature(StringRef archExt);
unsigned check_arch_version(StringRef arch);

// Information by Name
unsigned  get_default_fpu(StringRef cpu, ArchKind ak);
unsigned  get_default_extensions(StringRef cpu, ArchKind ak);
StringRef get_default_cpu(StringRef arch);
aarch64::ArchKind get_cpu_arch_kind(StringRef cpu);

// Parser
unsigned parse_fpu(StringRef FPU);
aarch64::ArchKind parse_arch(StringRef arch);
unsigned parse_arch_ext(StringRef archExt);
ArchKind parse_cpu_arch(StringRef cpu);
void fill_valid_cpu_arch_list(SmallVectorImpl<StringRef> &values);
arm::ISAKind parse_arch_isa(StringRef arch);
arm::EndianKind parse_arch_endian(StringRef arch);
arm::profileKind parse_arch_profile(StringRef arch);
unsigned parse_arch_version(StringRef arch);

} // namespace AArch64

namespace x86 {

// This should be kept in sync with libcc/compiler-rt as its included by clang
// as a proxy for what's in libgcc/compiler-rt.
enum ProcessorVendors : unsigned
{
   VENDOR_DUMMY,
#define X86_VENDOR(ENUM, STRING) \
   ENUM,
#include "polarphp/utils/X86TargetParser.h"
   VENDOR_OTHER
};

// This should be kept in sync with libcc/compiler-rt as its included by clang
// as a proxy for what's in libgcc/compiler-rt.
enum ProcessorTypes : unsigned
{
   cpu_TYPE_DUMMY,
#define X86_CPU_TYPE(ARCHNAME, ENUM) \
   ENUM,
#include "polarphp/utils/X86TargetParser.h"
   cpu_TYPE_MAX
};

// This should be kept in sync with libcc/compiler-rt as its included by clang
// as a proxy for what's in libgcc/compiler-rt.
enum ProcessorSubtypes : unsigned
{
   cpu_SUBTYPE_DUMMY,
#define X86_CPU_SUBTYPE(ARCHNAME, ENUM) \
   ENUM,
#include "polarphp/utils/X86TargetParser.h"
   cpu_SUBTYPE_MAX
};

// This should be kept in sync with libcc/compiler-rt as it should be used
// by clang as a proxy for what's in libgcc/compiler-rt.
enum ProcessorFeatures
{
#define X86_FEATURE(VAL, ENUM) \
   ENUM = VAL,
#include "polarphp/utils/X86TargetParser.h"

};

} // namespace X86
} // polar

#endif // POLARPHP_UTILS_TARGET_PARSER_H
