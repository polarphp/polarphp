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

#ifndef POLARPHP_UTILS_ARM_TARGET_PARSER_H
#define POLARPHP_UTILS_ARM_TARGET_PARSER_H

#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/Triple.h"
#include "polarphp/utils/ARMBuildAttributes.h"
#include <vector>

namespace polar {
namespace arm {

using polar::basic::StringRef;
using polar::basic::SmallVectorImpl;
using polar::basic::Triple;

// Arch extension modifiers for CPUs.
// Note that this is not the same as the AArch64 list
enum ArchExtKind : unsigned
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
   AEK_FP16FML =     1 << 17,
   // Unsupported extensions.
   AEK_OS = 0x8000000,
   AEK_IWMMXT = 0x10000000,
   AEK_IWMMXT2 = 0x20000000,
   AEK_MAVERICK = 0x40000000,
   AEK_XSCALE = 0x80000000,
};

// List of Arch Extension names.
// FIXME: TableGen this.
struct ExtName
{
   const char *NameCStr;
   size_t NameLength;
   unsigned ID;
   const char *Feature;
   const char *NegFeature;

   StringRef getName() const
   {
      return StringRef(NameCStr, NameLength);
   }
};

const ExtName sg_archExtNames[] = {
   #define ARM_ARCH_EXT_NAME(NAME, ID, FEATURE, NEGFEATURE)                       \
{NAME, sizeof(NAME) - 1, ID, FEATURE, NEGFEATURE},
   #include "polarphp/utils/ARMTargetParserDefs.h"
};

// List of HWDiv names (use getHWDivSynonym) and which architectural
// features they correspond to (use getHWDivFeatures).
// FIXME: TableGen this.
const struct
{
   const char *NameCStr;
   size_t NameLength;
   unsigned ID;

   StringRef getName() const { return StringRef(NameCStr, NameLength); }
} sg_hwDivNames[] = {
#define ARM_HW_DIV_NAME(NAME, ID) {NAME, sizeof(NAME) - 1, ID},
#include "polarphp/utils/ARMTargetParserDefs.h"
};

// Arch names.
enum class ArchKind {
#define ARM_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT) ID,
#include "polarphp/utils/ARMTargetParserDefs.h"
};

// List of CPU names and their arches.
// The same CPU can have multiple arches and can be default on multiple arches.
// When finding the Arch for a CPU, first-found prevails. Sort them accordingly.
// When this becomes table-generated, we'd probably need two tables.
// FIXME: TableGen this.
template <typename T> struct CpuNames
{
   const char *NameCStr;
   size_t NameLength;
   T ArchID;
   bool Default; // is $Name the default CPU for $ArchID ?
   unsigned DefaultExtensions;

   StringRef getName() const
   {
      return StringRef(NameCStr, NameLength);
   }
};

const CpuNames<ArchKind> sg_cpuNames[] = {
   #define ARM_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)           \
{NAME, sizeof(NAME) - 1, arm::ArchKind::ID, IS_DEFAULT, DEFAULT_EXT},
   #include "polarphp/utils/ARMTargetParserDefs.h"
};

// FPU names.
enum FPUKind {
#define ARM_FPU(NAME, KIND, VERSION, NEON_SUPPORT, RESTRICTION) KIND,
#include "polarphp/utils/ARMTargetParserDefs.h"
   FK_LAST
};


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
enum class ProfileKind
{
   INVALID = 0, A, R, M
};

struct FPUName
{
   const char *NameCStr;
   size_t NameLength;
   FPUKind ID;
   FPUVersion FPUVer;
   NeonSupportLevel NeonSupport;
   FPURestriction Restriction;
   StringRef getName() const
   {
      return StringRef(NameCStr, NameLength);
   }
};

static const FPUName sg_fpuNames[] = {
   #define ARM_FPU(NAME, KIND, VERSION, NEON_SUPPORT, RESTRICTION)                \
{NAME, sizeof(NAME) - 1, KIND, VERSION, NEON_SUPPORT, RESTRICTION},
   #include "polarphp/utils/ARMTargetParserDefs.h"
};

// List of canonical arch names (use getArchSynonym).
// This table also provides the build attribute fields for CPU arch
// and Arch ID, according to the Addenda to the ARM ABI, chapters
// 2.4 and 2.3.5.2 respectively.
// FIXME: SubArch values were simplified to fit into the expectations
// of the triples and are not conforming with their official names.
// Check to see if the expectation should be changed.
// FIXME: TableGen this.
template <typename T>
struct ArchNames
{
   const char *NameCStr;
   size_t NameLength;
   const char *CPUAttrCStr;
   size_t CPUAttrLength;
   const char *SubArchCStr;
   size_t SubArchLength;
   unsigned DefaultFPU;
   unsigned ArchBaseExtensions;
   T ID;
   armbuildattrs::CPUArch ArchAttr; // Arch ID in build attributes.

   StringRef getName() const
   {
      return StringRef(NameCStr, NameLength);
   }

   // CPU class in build attributes.
   StringRef getCPUAttr() const
   {
      return StringRef(CPUAttrCStr, CPUAttrLength);
   }

   // Sub-Arch name.
   StringRef getSubArch() const
   {
      return StringRef(SubArchCStr, SubArchLength);
   }
};

static const ArchNames<ArchKind> sg_archNames[] = {
   #define ARM_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU,            \
      ARCH_BASE_EXT)                                                \
{NAME,         sizeof(NAME) - 1,                                             \
      CPU_ATTR,     sizeof(CPU_ATTR) - 1,                                         \
      SUB_ARCH,     sizeof(SUB_ARCH) - 1,                                         \
      ARCH_FPU,     ARCH_BASE_EXT,                                                \
      ArchKind::ID, ARCH_ATTR},
   #include "polarphp/utils/ARMTargetParserDefs.h"
};

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
StringRef get_canonical_arch_name(StringRef arch);
StringRef get_fpu_synonym(StringRef fpu);
StringRef get_arch_synonym(StringRef arch);

// Parser
unsigned parse_hw_div(StringRef hwDiv);
unsigned parse_fpu(StringRef FPU);
ArchKind parse_arch(StringRef arch);
unsigned parse_arch_ext(StringRef archExt);
ArchKind parse_cpu_arch(StringRef cpu);

ISAKind parse_arch_isa(StringRef arch);
EndianKind parse_arch_endian(StringRef arch);
ProfileKind parse_arch_profile(StringRef arch);
unsigned parse_arch_version(StringRef arch);

void fill_valid_cpu_arch_list(SmallVectorImpl<StringRef> &values);
StringRef compute_default_target_abi(const Triple &TT, StringRef cpu);

} // namespace ARM
} // polar

#endif // POLARPHP_UTILS_ARM_TARGET_PARSER_H
