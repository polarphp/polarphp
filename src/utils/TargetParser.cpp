// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://polarphp.org/LICENSE.txt for license information
// See http://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by softboy on 2018/06/19.

//===----------------------------------------------------------------------===//
//
// This file implements a target parser to recognise hardware features such as
// FPU/CPU/ARCH names as well as specific support such as HDIV, etc.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/ARMBuildAttributes.h"
#include "polarphp/utils/TargetParser.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include "polarphp/basic/adt/Twine.h"
#include "polarphp/basic/adt/StringRef.h"
#include <cctype>

namespace polar {

using namespace arm;
using namespace aarch64;
using polar::basic::StringSwitch;
using polar::basic::StringRef;

namespace {

// List of canonical FPU names (use get_fpu_synonym) and which architectural
// features they correspond to (use get_fpu_features).
// FIXME: TableGen this.
// The entries must appear in the order listed in arm::FPUKind for correct indexing
static const struct
{
   const char *NameCStr;
   size_t NameLength;
   arm::FPUKind ID;
   arm::FPUVersion FPUVersion;
   arm::NeonSupportLevel NeonSupport;
   arm::FPURestriction Restriction;

   StringRef getName() const
   {
      return StringRef(NameCStr, NameLength);
   }
} sg_fpuNames[] = {
#define ARM_FPU(NAME, KIND, VERSION, NEON_SUPPORT, RESTRICTION) \
   { NAME, sizeof(NAME) - 1, KIND, VERSION, NEON_SUPPORT, RESTRICTION },
#include "polarphp/utils/ARMTargetParser.h"
};

// List of canonical arch names (use get_arch_synonym).
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

   StringRef getName() const { return StringRef(NameCStr, NameLength); }

   // CPU class in build attributes.
   StringRef get_cpu_attr() const { return StringRef(CPUAttrCStr, CPUAttrLength); }

   // Sub-Arch name.
   StringRef get_sub_arch() const { return StringRef(SubArchCStr, SubArchLength); }
};
ArchNames<arm::ArchKind> ARCHNames[] = {
   #define ARM_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT)       \
{NAME, sizeof(NAME) - 1, CPU_ATTR, sizeof(CPU_ATTR) - 1, SUB_ARCH,       \
      sizeof(SUB_ARCH) - 1, ARCH_FPU, ARCH_BASE_EXT, arm::ArchKind::ID, ARCH_ATTR},
   #include "polarphp/utils/ARMTargetParser.h"
};

ArchNames<aarch64::ArchKind> AArch64ARCHNames[] = {
   #define AARCH64_ARCH(NAME, ID, CPU_ATTR, SUB_ARCH, ARCH_ATTR, ARCH_FPU, ARCH_BASE_EXT)       \
{NAME, sizeof(NAME) - 1, CPU_ATTR, sizeof(CPU_ATTR) - 1, SUB_ARCH,       \
      sizeof(SUB_ARCH) - 1, ARCH_FPU, ARCH_BASE_EXT, aarch64::ArchKind::ID, ARCH_ATTR},
   #include "polarphp/utils/AArch64TargetParser.h"
};


// List of Arch Extension names.
// FIXME: TableGen this.
static const struct
{
   const char *NameCStr;
   size_t NameLength;
   unsigned ID;
   const char *Feature;
   const char *NegFeature;

   StringRef getName() const { return StringRef(NameCStr, NameLength); }
} ARCHExtNames[] = {
#define ARM_ARCH_EXT_NAME(NAME, ID, FEATURE, NEGFEATURE) \
   { NAME, sizeof(NAME) - 1, ID, FEATURE, NEGFEATURE },
#include "polarphp/utils/ARMTargetParser.h"
},AArch64ARCHExtNames[] = {
#define AARCH64_ARCH_EXT_NAME(NAME, ID, FEATURE, NEGFEATURE) \
   { NAME, sizeof(NAME) - 1, ID, FEATURE, NEGFEATURE },
#include "polarphp/utils/AArch64TargetParser.h"
};

// List of HWDiv names (use get_h_div_synonym) and which architectural
// features they correspond to (use get_hw_div_features).
// FIXME: TableGen this.
static const struct
{
   const char *NameCStr;
   size_t NameLength;
   unsigned ID;

   StringRef getName() const { return StringRef(NameCStr, NameLength); }
} HWDivNames[] = {
#define ARM_HW_DIV_NAME(NAME, ID) { NAME, sizeof(NAME) - 1, ID },
#include "polarphp/utils/ARMTargetParser.h"
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

   StringRef getName() const { return StringRef(NameCStr, NameLength); }
};
CpuNames<arm::ArchKind> CPUNames[] = {
   #define ARM_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT) \
{ NAME, sizeof(NAME) - 1, arm::ArchKind::ID, IS_DEFAULT, DEFAULT_EXT },
   #include "polarphp/utils/ARMTargetParser.h"
};

CpuNames<aarch64::ArchKind> AArch64CPUNames[] = {
   #define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT) \
{ NAME, sizeof(NAME) - 1, aarch64::ArchKind::ID, IS_DEFAULT, DEFAULT_EXT },
   #include "polarphp/utils/AArch64TargetParser.h"
};

} // namespace

// ======================================================= //
// Information by ID
// ======================================================= //

StringRef arm::get_fpu_name(unsigned fpuKind)
{
   if (fpuKind >= arm::FK_LAST)
      return StringRef();
   return sg_fpuNames[fpuKind].getName();
}

FPUVersion arm::get_fpu_version(unsigned fpuKind)
{
   if (fpuKind >= arm::FK_LAST) {
      return FPUVersion::NONE;
   }
   return sg_fpuNames[fpuKind].FPUVersion;
}

arm::NeonSupportLevel arm::get_fpu_neon_support_level(unsigned fpuKind)
{
   if (fpuKind >= arm::FK_LAST) {
      return arm::NeonSupportLevel::None;
   }
   return sg_fpuNames[fpuKind].NeonSupport;
}

arm::FPURestriction arm::get_fpu_restriction(unsigned fpuKind)
{
   if (fpuKind >= arm::FK_LAST) {
      return arm::FPURestriction::None;
   }
   return sg_fpuNames[fpuKind].Restriction;
}

unsigned arm::get_default_fpu(StringRef cpu, ArchKind archKind)
{
   if (cpu == "generic") {
      return ARCHNames[static_cast<unsigned>(archKind)].DefaultFPU;
   }

   return StringSwitch<unsigned>(cpu)
      #define ARM_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT) \
         .cond(NAME, DEFAULT_FPU)
      #include "polarphp/utils/ARMTargetParser.h"
         .defaultCond(arm::FK_INVALID);
}

unsigned arm::get_default_extensions(StringRef cpu, ArchKind archKind)
{
   if (cpu == "generic") {
      return ARCHNames[static_cast<unsigned>(archKind)].ArchBaseExtensions;
   }
   return StringSwitch<unsigned>(cpu)
      #define ARM_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT) \
         .cond(NAME, ARCHNames[static_cast<unsigned>(arm::ArchKind::ID)]\
         .ArchBaseExtensions | DEFAULT_EXT)
      #include "polarphp/utils/ARMTargetParser.h"
         .defaultCond(arm::AEK_INVALID);
}

bool arm::get_hw_div_features(unsigned hwDivKind,
                              std::vector<StringRef> &features)
{

   if (hwDivKind == arm::AEK_INVALID)
      return false;

   if (hwDivKind & arm::AEK_HWDIVARM) {
      features.push_back("+hwdiv-arm");
   } else {
      features.push_back("-hwdiv-arm");
   }
   if (hwDivKind & arm::AEK_HWDIVTHUMB) {
      features.push_back("+hwdiv");
   } else {
      features.push_back("-hwdiv");
   }
   return true;
}

bool arm::get_extension_features(unsigned extensions,
                                 std::vector<StringRef> &features) {

   if (extensions == arm::AEK_INVALID)
      return false;

   if (extensions & arm::AEK_CRC) {
      features.push_back("+crc");
   } else {
      features.push_back("-crc");
   }

   if (extensions & arm::AEK_DSP) {
      features.push_back("+dsp");
   } else {
      features.push_back("-dsp");
   }
   if (extensions & arm::AEK_RAS) {
      features.push_back("+ras");
   } else {
      features.push_back("-ras");
   }

   if (extensions & arm::AEK_DOTPROD) {
      features.push_back("+dotprod");
   } else {
      features.push_back("-dotprod");
   }
   return get_hw_div_features(extensions, features);
}

bool arm::get_fpu_features(unsigned fpuKind,
                           std::vector<StringRef> &features)
{
   if (fpuKind >= arm::FK_LAST || fpuKind == arm::FK_INVALID) {
      return false;
   }

   // fp-only-sp and d16 subtarget features are independent of each other, so we
   // must enable/disable both.
   switch (sg_fpuNames[fpuKind].Restriction) {
   case arm::FPURestriction::SP_D16:
      features.push_back("+fp-only-sp");
      features.push_back("+d16");
      break;
   case arm::FPURestriction::D16:
      features.push_back("-fp-only-sp");
      features.push_back("+d16");
      break;
   case arm::FPURestriction::None:
      features.push_back("-fp-only-sp");
      features.push_back("-d16");
      break;
   }

   // FPU version subtarget features are inclusive of lower-numbered ones, so
   // enable the one corresponding to this version and disable all that are
   // higher. We also have to make sure to disable fp16 when vfp4 is disabled,
   // as +vfp4 implies +fp16 but -vfp4 does not imply -fp16.
   switch (sg_fpuNames[fpuKind].FPUVersion) {
   case arm::FPUVersion::VFPV5:
      features.push_back("+fp-armv8");
      break;
   case arm::FPUVersion::VFPV4:
      features.push_back("+vfp4");
      features.push_back("-fp-armv8");
      break;
   case arm::FPUVersion::VFPV3_FP16:
      features.push_back("+vfp3");
      features.push_back("+fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   case arm::FPUVersion::VFPV3:
      features.push_back("+vfp3");
      features.push_back("-fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   case arm::FPUVersion::VFPV2:
      features.push_back("+vfp2");
      features.push_back("-vfp3");
      features.push_back("-fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   case arm::FPUVersion::NONE:
      features.push_back("-vfp2");
      features.push_back("-vfp3");
      features.push_back("-fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   }

   // crypto includes neon, so we handle this similarly to FPU version.
   switch (sg_fpuNames[fpuKind].NeonSupport) {
   case arm::NeonSupportLevel::Crypto:
      features.push_back("+neon");
      features.push_back("+crypto");
      break;
   case arm::NeonSupportLevel::Neon:
      features.push_back("+neon");
      features.push_back("-crypto");
      break;
   case arm::NeonSupportLevel::None:
      features.push_back("-neon");
      features.push_back("-crypto");
      break;
   }

   return true;
}

StringRef arm::get_arch_name(ArchKind archExtKind)
{
   return ARCHNames[static_cast<unsigned>(archExtKind)].getName();
}

StringRef arm::get_cpu_attr(ArchKind archExtKind)
{
   return ARCHNames[static_cast<unsigned>(archExtKind)].get_cpu_attr();
}

StringRef arm::get_sub_arch(ArchKind archExtKind)
{
   return ARCHNames[static_cast<unsigned>(archExtKind)].get_sub_arch();
}

unsigned arm::get_arch_attr(ArchKind archExtKind)
{
   return ARCHNames[static_cast<unsigned>(archExtKind)].ArchAttr;
}

StringRef arm::get_arch_ext_name(unsigned archExtKind)
{
   for (const auto ae : ARCHExtNames) {
      if (archExtKind == ae.ID) {
         return ae.getName();
      }
   }
   return StringRef();
}

StringRef arm::get_arch_ext_feature(StringRef archExt)
{
   if (archExt.startsWith("no")) {
      StringRef archExtBase(archExt.substr(2));
      for (const auto ae : ARCHExtNames) {
         if (ae.NegFeature && archExtBase == ae.getName()) {
            return StringRef(ae.NegFeature);
         }
      }
   }
   for (const auto ae : ARCHExtNames) {
      if (ae.Feature && archExt == ae.getName()) {
         return StringRef(ae.Feature);
      }
   }

   return StringRef();
}

StringRef arm::get_hw_div_name(unsigned hwDivKind)
{
   for (const auto item : HWDivNames) {
      if (hwDivKind == item.ID) {
         return item.getName();
      }
   }
   return StringRef();
}

StringRef arm::get_default_cpu(StringRef arch)
{
   ArchKind archKind = parse_arch(arch);
   if (archKind == arm::ArchKind::INVALID) {
      return StringRef();
   }

   // Look for multiple AKs to find the default for pair AK+Name.
   for (const auto cpu : CPUNames) {
      if (cpu.ArchID == archKind && cpu.Default) {
         return cpu.getName();
      }
   }

   // If we can't find a default then target the architecture instead
   return "generic";
}

StringRef aarch64::get_fpu_name(unsigned fpuKind)
{
   return arm::get_fpu_name(fpuKind);
}

arm::FPUVersion aarch64::get_fpu_version(unsigned fpuKind)
{
   return arm::get_fpu_version(fpuKind);
}

arm::NeonSupportLevel aarch64::get_fpu_neon_support_level(unsigned fpuKind)
{
   return arm::get_fpu_neon_support_level(fpuKind);
}

arm::FPURestriction aarch64::get_fpu_restriction(unsigned fpuKind)
{
   return arm::get_fpu_restriction(fpuKind);
}

unsigned aarch64::get_default_fpu(StringRef cpu, ArchKind archKind)
{
   if (cpu == "generic") {
      return AArch64ARCHNames[static_cast<unsigned>(archKind)].DefaultFPU;
   }
   return StringSwitch<unsigned>(cpu)
      #define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT) \
         .cond(NAME, DEFAULT_FPU)
      #include "polarphp/utils/AArch64TargetParser.h"
         .defaultCond(arm::FK_INVALID);
}

unsigned aarch64::get_default_extensions(StringRef cpu, ArchKind archKind)
{
   if (cpu == "generic") {
      return AArch64ARCHNames[static_cast<unsigned>(archKind)].ArchBaseExtensions;
   }
   return StringSwitch<unsigned>(cpu)
      #define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)       \
         .cond(NAME,                                                                  \
         AArch64ARCHNames[static_cast<unsigned>(aarch64::ArchKind::ID)] \
         .ArchBaseExtensions | \
         DEFAULT_EXT)
      #include "polarphp/utils/AArch64TargetParser.h"
         .defaultCond(aarch64::AEK_INVALID);
}

bool aarch64::get_extension_features(unsigned Extensions,
                                     std::vector<StringRef> &Features)
{

   if (Extensions == aarch64::AEK_INVALID)
      return false;

   if (Extensions & aarch64::AEK_FP)
      Features.push_back("+fp-armv8");
   if (Extensions & aarch64::AEK_SIMD)
      Features.push_back("+neon");
   if (Extensions & aarch64::AEK_CRC)
      Features.push_back("+crc");
   if (Extensions & aarch64::AEK_CRYPTO)
      Features.push_back("+crypto");
   if (Extensions & aarch64::AEK_DOTPROD)
      Features.push_back("+dotprod");
   if (Extensions & aarch64::AEK_FP16)
      Features.push_back("+fullfp16");
   if (Extensions & aarch64::AEK_PROFILE)
      Features.push_back("+spe");
   if (Extensions & aarch64::AEK_RAS)
      Features.push_back("+ras");
   if (Extensions & aarch64::AEK_LSE)
      Features.push_back("+lse");
   if (Extensions & aarch64::AEK_RDM)
      Features.push_back("+rdm");
   if (Extensions & aarch64::AEK_SVE)
      Features.push_back("+sve");
   if (Extensions & aarch64::AEK_RCPC)
      Features.push_back("+rcpc");

   return true;
}

bool aarch64::get_fpu_features(unsigned fpuKind,
                               std::vector<StringRef> &Features) {
   return arm::get_fpu_features(fpuKind, Features);
}

bool aarch64::get_arch_features(aarch64::ArchKind ak,
                                std::vector<StringRef> &features)
{
   if (ak == aarch64::ArchKind::ARMV8_1A) {
      features.push_back("+v8.1a");
   }
   if (ak == aarch64::ArchKind::ARMV8_2A) {
      features.push_back("+v8.2a");
   }
   if (ak == aarch64::ArchKind::ARMV8_3A) {
      features.push_back("+v8.3a");
   }
   return ak != aarch64::ArchKind::INVALID;
}

StringRef aarch64::get_arch_name(ArchKind archKind)
{
   return AArch64ARCHNames[static_cast<unsigned>(archKind)].getName();
}

StringRef aarch64::get_cpu_attr(ArchKind archKind)
{
   return AArch64ARCHNames[static_cast<unsigned>(archKind)].get_cpu_attr();
}

StringRef aarch64::get_sub_arch(ArchKind archKind)
{
   return AArch64ARCHNames[static_cast<unsigned>(archKind)].get_sub_arch();
}

unsigned aarch64::get_arch_attr(ArchKind archKind)
{
   return AArch64ARCHNames[static_cast<unsigned>(archKind)].ArchAttr;
}

StringRef aarch64::get_arch_ext_name(unsigned archExtKind)
{
   for (const auto &ae : AArch64ARCHExtNames) {
      if (archExtKind == ae.ID) {
         return ae.getName();
      }
   }
   return StringRef();
}

StringRef aarch64::get_arch_ext_feature(StringRef archExt)
{
   if (archExt.startsWith("no")) {
      StringRef archExtBase(archExt.substr(2));
      for (const auto &ae : AArch64ARCHExtNames) {
         if (ae.NegFeature && archExtBase == ae.getName()) {
            return StringRef(ae.NegFeature);
         }
      }
   }

   for (const auto &ae : AArch64ARCHExtNames) {
      if (ae.Feature && archExt == ae.getName()) {
         return StringRef(ae.Feature);
      }
   }
   return StringRef();
}

StringRef aarch64::get_default_cpu(StringRef arch)
{
   aarch64::ArchKind archKind = parse_arch(arch);
   if (archKind == ArchKind::INVALID) {
      return StringRef();
   }
   // Look for multiple AKs to find the default for pair AK+Name.
   for (const auto &cpu : AArch64CPUNames) {
      if (cpu.ArchID == archKind && cpu.Default) {
         return cpu.getName();
      }
   }
   // If we can't find a default then target the architecture instead
   return "generic";
}

unsigned aarch64::check_arch_version(StringRef arch)
{
   if (arch.getSize() >= 2 && arch[0] == 'v' && std::isdigit(arch[1])) {
      return (arch[1] - 48);
   }
   return 0;
}

// ======================================================= //
// Parsers
// ======================================================= //

namespace {

StringRef get_h_div_synonym(StringRef hwdiv)
{
   return StringSwitch<StringRef>(hwdiv)
         .cond("thumb,arm", "arm,thumb")
         .defaultCond(hwdiv);
}

StringRef get_fpu_synonym(StringRef fpu)
{
   return StringSwitch<StringRef>(fpu)
         .conds("fpa", "fpe2", "fpe3", "maverick", "invalid") // Unsupported
         .cond("vfp2", "vfpv2")
         .cond("vfp3", "vfpv3")
         .cond("vfp4", "vfpv4")
         .cond("vfp3-d16", "vfpv3-d16")
         .cond("vfp4-d16", "vfpv4-d16")
         .conds("fp4-sp-d16", "vfpv4-sp-d16", "fpv4-sp-d16")
         .conds("fp4-dp-d16", "fpv4-dp-d16", "vfpv4-d16")
         .cond("fp5-sp-d16", "fpv5-sp-d16")
         .conds("fp5-dp-d16", "fpv5-dp-d16", "fpv5-d16")
         // FIXME: Clang uses it, but it's bogus, since neon defaults to vfpv3.
         .cond("neon-vfpv3", "neon")
         .defaultCond(fpu);
}

StringRef get_arch_synonym(StringRef arch)
{
   return StringSwitch<StringRef>(arch)
         .cond("v5", "v5t")
         .cond("v5e", "v5te")
         .cond("v6j", "v6")
         .cond("v6hl", "v6k")
         .conds("v6m", "v6sm", "v6s-m", "v6-m")
         .conds("v6z", "v6zk", "v6kz")
         .conds("v7", "v7a", "v7hl", "v7l", "v7-a")
         .cond("v7r", "v7-r")
         .cond("v7m", "v7-m")
         .cond("v7em", "v7e-m")
         .conds("v8", "v8a", "aarch64", "arm64", "v8-a")
         .cond("v8.1a", "v8.1-a")
         .cond("v8.2a", "v8.2-a")
         .cond("v8.3a", "v8.3-a")
         .cond("v8r", "v8-r")
         .cond("v8m.base", "v8-m.base")
         .cond("v8m.main", "v8-m.main")
         .defaultCond(arch);
}


} // anonymous namespace

// MArch is expected to be of the form (arm|thumb)?(eb)?(v.+)?(eb)?, but
// (iwmmxt|xscale)(eb)? is also permitted. If the former, return
// "v.+", if the latter, return unmodified string, minus 'eb'.
// If invalid, return empty string.
StringRef arm::get_canonical_arch_name(StringRef arch)
{
   size_t offset = StringRef::npos;
   StringRef archCopy = arch;
   StringRef error = "";

   // Begins with "arm" / "thumb", move past it.
   if (archCopy.startsWith("arm64")) {
      offset = 5;
   } else if (archCopy.startsWith("arm")) {
      offset = 3;
   } else if (archCopy.startsWith("thumb")) {
      offset = 5;
   } else if (archCopy.startsWith("aarch64")) {
      offset = 7;
      // AArch64 uses "_be", not "eb" suffix.
      if (archCopy.find("eb") != StringRef::npos) {
         return error;
      }

      if (archCopy.substr(offset, 3) == "_be") {
         offset += 3;
      }
   }

   // Ex. "armebv7", move past the "eb".
   if (offset != StringRef::npos && archCopy.substr(offset, 2) == "eb") {
      offset += 2;
   } else if (archCopy.endsWith("eb")) {
      // Or, if it ends with eb ("armv7eb"), chop it off.
      archCopy = archCopy.substr(0, archCopy.getSize() - 2);
   }
   // Trim the head
   if (offset != StringRef::npos)
      archCopy = archCopy.substr(offset);

   // Empty string means offset reached the end, which means it's valid.
   if (archCopy.empty()) {
      return arch;
   }
   // Only match non-marketing names
   if (offset != StringRef::npos) {
      // Must start with 'vN'.
      if (archCopy.getSize() >= 2 && (archCopy[0] != 'v' || !std::isdigit(archCopy[1]))) {
         return error;
      }
      // Can't have an extra 'eb'.
      if (archCopy.find("eb") != StringRef::npos) {
         return error;
      }
   }
   // Arch will either be a 'v' name (v7a) or a marketing name (xscale).
   return archCopy;
}

unsigned arm::parse_hw_div(StringRef hwDiv)
{
   StringRef syn = get_h_div_synonym(hwDiv);
   for (const auto item : HWDivNames) {
      if (syn == item.getName()) {
         return item.ID;
      }
   }
   return arm::AEK_INVALID;
}

unsigned arm::parse_fpu(StringRef fpu)
{
   StringRef syn = get_fpu_synonym(fpu);
   for (const auto item : sg_fpuNames) {
      if (syn == item.getName()) {
         return item.ID;
      }
   }
   return arm::FK_INVALID;
}

// Allows partial match, ex. "v7a" matches "armv7a".
arm::ArchKind arm::parse_arch(StringRef arch)
{
   arch = get_canonical_arch_name(arch);
   StringRef syn = get_arch_synonym(arch);
   for (const auto item : ARCHNames) {
      if (item.getName().endsWith(syn)) {
         return item.ID;
      }
   }
   return arm::ArchKind::INVALID;
}

unsigned arm::parse_arch_ext(StringRef archExt)
{
   for (const auto item : ARCHExtNames) {
      if (archExt == item.getName()) {
         return item.ID;
      }
   }
   return arm::AEK_INVALID;
}

arm::ArchKind arm::parse_cpu_arch(StringRef cpu)
{
   for (const auto item : CPUNames) {
      if (cpu == item.getName()) {
         return item.ArchID;
      }
   }
   return arm::ArchKind::INVALID;
}

// ARM, Thumb, AArch64
arm::ISAKind arm::parse_arch_isa(StringRef arch)
{
   return StringSwitch<arm::ISAKind>(arch)
         .startsWith("aarch64", arm::ISAKind::AARCH64)
         .startsWith("arm64", arm::ISAKind::AARCH64)
         .startsWith("thumb", arm::ISAKind::THUMB)
         .startsWith("arm", arm::ISAKind::ARM)
         .defaultCond(arm::ISAKind::INVALID);
}

// Little/Big endian
arm::EndianKind arm::parse_arch_endian(StringRef arch)
{
   if (arch.startsWith("armeb") || arch.startsWith("thumbeb") ||
       arch.startsWith("aarch64_be")) {
      return arm::EndianKind::BIG;
   }
   if (arch.startsWith("arm") || arch.startsWith("thumb")) {
      if (arch.endsWith("eb")) {
         return arm::EndianKind::BIG;
      } else {
         return arm::EndianKind::LITTLE;
      }
   }

   if (arch.startsWith("aarch64")) {
      return arm::EndianKind::LITTLE;
   }
   return arm::EndianKind::INVALID;
}

// profile A/R/M
arm::ProfileKind arm::parse_arch_profile(StringRef arch)
{
   arch = get_canonical_arch_name(arch);
   switch (parse_arch(arch)) {
   case arm::ArchKind::ARMV6M:
   case arm::ArchKind::ARMV7M:
   case arm::ArchKind::ARMV7EM:
   case arm::ArchKind::ARMV8MMainline:
   case arm::ArchKind::ARMV8MBaseline:
      return arm::ProfileKind::M;
   case arm::ArchKind::ARMV7R:
   case arm::ArchKind::ARMV8R:
      return arm::ProfileKind::R;
   case arm::ArchKind::ARMV7A:
   case arm::ArchKind::ARMV7VE:
   case arm::ArchKind::ARMV7K:
   case arm::ArchKind::ARMV8A:
   case arm::ArchKind::ARMV8_1A:
   case arm::ArchKind::ARMV8_2A:
   case arm::ArchKind::ARMV8_3A:
   case arm::ArchKind::ARMV8_4A:
      return arm::ProfileKind::A;
   case arm::ArchKind::ARMV2:
   case arm::ArchKind::ARMV2A:
   case arm::ArchKind::ARMV3:
   case arm::ArchKind::ARMV3M:
   case arm::ArchKind::ARMV4:
   case arm::ArchKind::ARMV4T:
   case arm::ArchKind::ARMV5T:
   case arm::ArchKind::ARMV5TE:
   case arm::ArchKind::ARMV5TEJ:
   case arm::ArchKind::ARMV6:
   case arm::ArchKind::ARMV6K:
   case arm::ArchKind::ARMV6T2:
   case arm::ArchKind::ARMV6KZ:
   case arm::ArchKind::ARMV7S:
   case arm::ArchKind::IWMMXT:
   case arm::ArchKind::IWMMXT2:
   case arm::ArchKind::XSCALE:
   case arm::ArchKind::INVALID:
      return arm::ProfileKind::INVALID;
   }
   polar_unreachable("Unhandled architecture");
}

// Version number (ex. v7 = 7).
unsigned arm::parse_arch_version(StringRef Arch) {
   Arch = get_canonical_arch_name(Arch);
   switch (parse_arch(Arch)) {
   case arm::ArchKind::ARMV2:
   case arm::ArchKind::ARMV2A:
      return 2;
   case arm::ArchKind::ARMV3:
   case arm::ArchKind::ARMV3M:
      return 3;
   case arm::ArchKind::ARMV4:
   case arm::ArchKind::ARMV4T:
      return 4;
   case arm::ArchKind::ARMV5T:
   case arm::ArchKind::ARMV5TE:
   case arm::ArchKind::IWMMXT:
   case arm::ArchKind::IWMMXT2:
   case arm::ArchKind::XSCALE:
   case arm::ArchKind::ARMV5TEJ:
      return 5;
   case arm::ArchKind::ARMV6:
   case arm::ArchKind::ARMV6K:
   case arm::ArchKind::ARMV6T2:
   case arm::ArchKind::ARMV6KZ:
   case arm::ArchKind::ARMV6M:
      return 6;
   case arm::ArchKind::ARMV7A:
   case arm::ArchKind::ARMV7VE:
   case arm::ArchKind::ARMV7R:
   case arm::ArchKind::ARMV7M:
   case arm::ArchKind::ARMV7S:
   case arm::ArchKind::ARMV7EM:
   case arm::ArchKind::ARMV7K:
      return 7;
   case arm::ArchKind::ARMV8A:
   case arm::ArchKind::ARMV8_1A:
   case arm::ArchKind::ARMV8_2A:
   case arm::ArchKind::ARMV8_3A:
   case arm::ArchKind::ARMV8_4A:
   case arm::ArchKind::ARMV8R:
   case arm::ArchKind::ARMV8MBaseline:
   case arm::ArchKind::ARMV8MMainline:
      return 8;
   case arm::ArchKind::INVALID:
      return 0;
   }
   polar_unreachable("Unhandled architecture");
}

StringRef arm::compute_default_target_abi(const Triple &triple, StringRef cpu)
{
   StringRef archName =
         cpu.empty() ? triple.getArchName() : arm::get_arch_name(arm::parse_cpu_arch(cpu));

   if (triple.isOSBinFormatMachO()) {
      if (triple.getEnvironment() == Triple::EnvironmentType::EABI ||
          triple.getOS() == Triple::OSType::UnknownOS ||
          arm::parse_arch_profile(archName) == arm::ProfileKind::M) {
         return "aapcs";
      }
      if (triple.isWatchABI()) {
         return "aapcs16";
      }
      return "apcs-gnu";
   } else if (triple.isOSWindows()) {
      // FIXME: this is invalid for WindowsCE.
      return "aapcs";
   }

   // Select the default based on the platform.
   switch (triple.getEnvironment()) {
   case Triple::EnvironmentType::Android:
   case Triple::EnvironmentType::GNUEABI:
   case Triple::EnvironmentType::GNUEABIHF:
   case Triple::EnvironmentType::MuslEABI:
   case Triple::EnvironmentType::MuslEABIHF:
      return "aapcs-linux";
   case Triple::EnvironmentType::EABIHF:
   case Triple::EnvironmentType::EABI:
      return "aapcs";
   default:
      if (triple.isOSNetBSD()) {
         return "apcs-gnu";
      }
      if (triple.isOSOpenBSD()) {
         return "aapcs-linux";
      }

      return "aapcs";
   }
}

StringRef aarch64::get_canonical_arch_name(StringRef arch)
{
   return arm::get_canonical_arch_name(arch);
}

unsigned aarch64::parse_fpu(StringRef fpu)
{
   return arm::parse_fpu(fpu);
}

// Allows partial match, ex. "v8a" matches "armv8a".
aarch64::ArchKind aarch64::parse_arch(StringRef arch)
{
   arch = get_canonical_arch_name(arch);
   if (check_arch_version(arch) < 8) {
      return ArchKind::INVALID;
   }
   StringRef syn = get_arch_synonym(arch);
   for (const auto item : AArch64ARCHNames) {
      if (item.getName().endsWith(syn)) {
         return item.ID;
      }
   }
   return ArchKind::INVALID;
}

unsigned aarch64::parse_arch_ext(StringRef archExt)
{
   for (const auto item : AArch64ARCHExtNames) {
      if (archExt == item.getName()) {
         return item.ID;
      }
   }
   return aarch64::AEK_INVALID;
}

aarch64::ArchKind aarch64::parse_cpu_arch(StringRef cpu)
{
   for (const auto item : AArch64CPUNames) {
      if (cpu == item.getName()) {
         return item.ArchID;
      }
   }
   return ArchKind::INVALID;
}

// ARM, Thumb, AArch64
arm::ISAKind aarch64::parse_arch_isa(StringRef arch)
{
   return arm::parse_arch_isa(arch);
}

// Little/Big endian
arm::EndianKind aarch64::parse_arch_endian(StringRef arch)
{
   return arm::parse_arch_endian(arch);
}

// profile A/R/M
arm::ProfileKind aarch64::parse_arch_profile(StringRef arch)
{
   return arm::parse_arch_profile(arch);
}

// Version number (ex. v8 = 8).
unsigned aarch64::parse_arch_version(StringRef arch)
{
   return arm::parse_arch_version(arch);
}

} // polar
