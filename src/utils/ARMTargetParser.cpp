// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/12/05.
//===----------------------------------------------------------------------===//
//
// This file implements a target parser to recognise ARM hardware features
// such as fpu/cpu/ARCH/extensions and specific support such as HWDIV.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/ARMTargetParser.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include <cctype>
#include <vector>

namespace polar {
namespace arm {

using polar::basic::StringRef;
using polar::basic::StringSwitch;

namespace {
StringRef get_hw_div_synonym(StringRef hwDiv)
{
   return StringSwitch<StringRef>(hwDiv)
         .cond("thumb,arm", "arm,thumb")
         .defaultCond(hwDiv);
}
} // anonymous namespace

// Allows partial match, ex. "v7a" matches "armv7a".
ArchKind parse_arch(StringRef arch)
{
   arch = get_canonical_arch_name(arch);
   StringRef syn = get_arch_synonym(arch);
   for (const auto item : sg_archNames) {
      if (item.getName().endsWith(syn)) {
         return item.ID;
      }
   }
   return ArchKind::INVALID;
}

// Version number (ex. v7 = 7).
unsigned parse_arch_version(StringRef arch)
{
   arch = get_canonical_arch_name(arch);
   switch (parse_arch(arch)) {
   case ArchKind::ARMV2:
   case ArchKind::ARMV2A:
      return 2;
   case ArchKind::ARMV3:
   case ArchKind::ARMV3M:
      return 3;
   case ArchKind::ARMV4:
   case ArchKind::ARMV4T:
      return 4;
   case ArchKind::ARMV5T:
   case ArchKind::ARMV5TE:
   case ArchKind::IWMMXT:
   case ArchKind::IWMMXT2:
   case ArchKind::XSCALE:
   case ArchKind::ARMV5TEJ:
      return 5;
   case ArchKind::ARMV6:
   case ArchKind::ARMV6K:
   case ArchKind::ARMV6T2:
   case ArchKind::ARMV6KZ:
   case ArchKind::ARMV6M:
      return 6;
   case ArchKind::ARMV7A:
   case ArchKind::ARMV7VE:
   case ArchKind::ARMV7R:
   case ArchKind::ARMV7M:
   case ArchKind::ARMV7S:
   case ArchKind::ARMV7EM:
   case ArchKind::ARMV7K:
      return 7;
   case ArchKind::ARMV8A:
   case ArchKind::ARMV8_1A:
   case ArchKind::ARMV8_2A:
   case ArchKind::ARMV8_3A:
   case ArchKind::ARMV8_4A:
   case ArchKind::ARMV8_5A:
   case ArchKind::ARMV8R:
   case ArchKind::ARMV8MBaseline:
   case ArchKind::ARMV8MMainline:
      return 8;
   case ArchKind::INVALID:
      return 0;
   }
   polar_unreachable("Unhandled architecture");
}

// Profile A/R/M
ProfileKind parse_arch_profile(StringRef arch)
{
   arch = get_canonical_arch_name(arch);
   switch (parse_arch(arch)) {
   case ArchKind::ARMV6M:
   case ArchKind::ARMV7M:
   case ArchKind::ARMV7EM:
   case ArchKind::ARMV8MMainline:
   case ArchKind::ARMV8MBaseline:
      return ProfileKind::M;
   case ArchKind::ARMV7R:
   case ArchKind::ARMV8R:
      return ProfileKind::R;
   case ArchKind::ARMV7A:
   case ArchKind::ARMV7VE:
   case ArchKind::ARMV7K:
   case ArchKind::ARMV8A:
   case ArchKind::ARMV8_1A:
   case ArchKind::ARMV8_2A:
   case ArchKind::ARMV8_3A:
   case ArchKind::ARMV8_4A:
   case ArchKind::ARMV8_5A:
      return ProfileKind::A;
   case ArchKind::ARMV2:
   case ArchKind::ARMV2A:
   case ArchKind::ARMV3:
   case ArchKind::ARMV3M:
   case ArchKind::ARMV4:
   case ArchKind::ARMV4T:
   case ArchKind::ARMV5T:
   case ArchKind::ARMV5TE:
   case ArchKind::ARMV5TEJ:
   case ArchKind::ARMV6:
   case ArchKind::ARMV6K:
   case ArchKind::ARMV6T2:
   case ArchKind::ARMV6KZ:
   case ArchKind::ARMV7S:
   case ArchKind::IWMMXT:
   case ArchKind::IWMMXT2:
   case ArchKind::XSCALE:
   case ArchKind::INVALID:
      return ProfileKind::INVALID;
   }
   polar_unreachable("Unhandled architecture");
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
         .conds("v8", "v8a", "v8l", "aarch64", "arm64", "v8-a")
         .cond("v8.1a", "v8.1-a")
         .cond("v8.2a", "v8.2-a")
         .cond("v8.3a", "v8.3-a")
         .cond("v8.4a", "v8.4-a")
         .cond("v8.5a", "v8.5-a")
         .cond("v8r", "v8-r")
         .cond("v8m.base", "v8-m.base")
         .cond("v8m.main", "v8-m.main")
         .defaultCond(arch);
}

bool get_fpu_features(unsigned fpuKind, std::vector<StringRef> &features)
{
   if (fpuKind >= FK_LAST || fpuKind == FK_INVALID) {
      return false;
   }

   // fp-only-sp and d16 subtarget features are independent of each other, so we
   // must enable/disable both.
   switch (sg_fpuNames[fpuKind].Restriction) {
   case FPURestriction::SP_D16:
      features.push_back("+fp-only-sp");
      features.push_back("+d16");
      break;
   case FPURestriction::D16:
      features.push_back("-fp-only-sp");
      features.push_back("+d16");
      break;
   case FPURestriction::None:
      features.push_back("-fp-only-sp");
      features.push_back("-d16");
      break;
   }

   // fpu version subtarget features are inclusive of lower-numbered ones, so
   // enable the one corresponding to this version and disable all that are
   // higher. We also have to make sure to disable fp16 when vfp4 is disabled,
   // as +vfp4 implies +fp16 but -vfp4 does not imply -fp16.
   switch (sg_fpuNames[fpuKind].FPUVer) {
   case FPUVersion::VFPV5:
      features.push_back("+fp-armv8");
      break;
   case FPUVersion::VFPV4:
      features.push_back("+vfp4");
      features.push_back("-fp-armv8");
      break;
   case FPUVersion::VFPV3_FP16:
      features.push_back("+vfp3");
      features.push_back("+fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   case FPUVersion::VFPV3:
      features.push_back("+vfp3");
      features.push_back("-fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   case FPUVersion::VFPV2:
      features.push_back("+vfp2");
      features.push_back("-vfp3");
      features.push_back("-fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   case FPUVersion::NONE:
      features.push_back("-vfp2");
      features.push_back("-vfp3");
      features.push_back("-fp16");
      features.push_back("-vfp4");
      features.push_back("-fp-armv8");
      break;
   }

   // crypto includes neon, so we handle this similarly to fpu version.
   switch (sg_fpuNames[fpuKind].NeonSupport) {
   case NeonSupportLevel::Crypto:
      features.push_back("+neon");
      features.push_back("+crypto");
      break;
   case NeonSupportLevel::Neon:
      features.push_back("+neon");
      features.push_back("-crypto");
      break;
   case NeonSupportLevel::None:
      features.push_back("-neon");
      features.push_back("-crypto");
      break;
   }

   return true;
}

// Little/Big endian
EndianKind parse_arch_endian(StringRef arch)
{
   if (arch.startsWith("armeb") || arch.startsWith("thumbeb") ||
       arch.startsWith("aarch64_be"))
      return EndianKind::BIG;

   if (arch.startsWith("arm") || arch.startsWith("thumb")) {
      if (arch.endsWith("eb")) {
         return EndianKind::BIG;
      } else {
         return EndianKind::LITTLE;
      }
   }
   if (arch.startsWith("aarch64")) {
      return EndianKind::LITTLE;
   }
   return EndianKind::INVALID;
}

// ARM, Thumb, AArch64
ISAKind parse_arch_isa(StringRef arch)
{
   return StringSwitch<ISAKind>(arch)
         .startsWith("aarch64", ISAKind::AARCH64)
         .startsWith("arm64", ISAKind::AARCH64)
         .startsWith("thumb", ISAKind::THUMB)
         .startsWith("arm", ISAKind::ARM)
         .defaultCond(ISAKind::INVALID);
}

unsigned parse_fpu(StringRef fpu)
{
   StringRef syn = get_fpu_synonym(fpu);
   for (const auto F : sg_fpuNames) {
      if (syn == F.getName()) {
         return F.ID;
      }
   }
   return FK_INVALID;
}

NeonSupportLevel get_fpu_neon_support_level(unsigned fpuKind)
{
   if (fpuKind >= FK_LAST) {
      return NeonSupportLevel::None;
   }
   return sg_fpuNames[fpuKind].NeonSupport;
}

// MArch is expected to be of the form (arm|thumb)?(eb)?(v.+)?(eb)?, but
// (iwmmxt|xscale)(eb)? is also permitted. If the former, return
// "v.+", if the latter, return unmodified string, minus 'eb'.
// If invalid, return empty string.
StringRef get_canonical_arch_name(StringRef arch)
{
   size_t offset = StringRef::npos;
   StringRef A = arch;
   StringRef errorMsg = "";

   // Begins with "arm" / "thumb", move past it.
   if (A.startsWith("arm64")) {
      offset = 5;
   } else if (A.startsWith("arm")) {
      offset = 3;
   } else if (A.startsWith("thumb")) {
      offset = 5;
   } else if (A.startsWith("aarch64")) {
      offset = 7;
      // AArch64 uses "_be", not "eb" suffix.
      if (A.find("eb") != StringRef::npos) {
         return errorMsg;
      }
      if (A.substr(offset, 3) == "_be") {
         offset += 3;
      }
   }

   // Ex. "armebv7", move past the "eb".
   if (offset != StringRef::npos && A.substr(offset, 2) == "eb") {
      offset += 2;
   }
   // Or, if it ends with eb ("armv7eb"), chop it off.
   else if (A.endsWith("eb")) {
      A = A.substr(0, A.size() - 2);
   }
   // Trim the head
   if (offset != StringRef::npos) {
      A = A.substr(offset);
   }
   // Empty string means offset reached the end, which means it's valid.
   if (A.empty()) {
      return arch;
   }
   // Only match non-marketing names
   if (offset != StringRef::npos) {
      // Must start with 'vN'.
      if (A.size() >= 2 && (A[0] != 'v' || !std::isdigit(A[1]))) {
         return errorMsg;
      }
      // Can't have an extra 'eb'.
      if (A.find("eb") != StringRef::npos) {
         return errorMsg;
      }
   }
   // arch will either be a 'v' name (v7a) or a marketing name (xscale).
   return A;
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

StringRef get_fpu_name(unsigned fpuKind)
{
   if (fpuKind >= FK_LAST) {
      return StringRef();
   }
   return sg_fpuNames[fpuKind].getName();
}

FPUVersion get_fpu_version(unsigned fpuKind)
{
   if (fpuKind >= FK_LAST) {
      return FPUVersion::NONE;
   }
   return sg_fpuNames[fpuKind].FPUVer;
}

FPURestriction get_fpu_restriction(unsigned fpuKind)
{
   if (fpuKind >= FK_LAST) {
      return FPURestriction::None;
   }
   return sg_fpuNames[fpuKind].Restriction;
}

unsigned get_default_fpu(StringRef cpu, ArchKind archKind)
{
   if (cpu == "generic") {
      return sg_archNames[static_cast<unsigned>(archKind)].DefaultFPU;
   }
   return StringSwitch<unsigned>(cpu)
      #define ARM_CPU_NAME(NAME, ID, DEFAULT_fpu, IS_DEFAULT, DEFAULT_EXT)           \
         .cond(NAME, DEFAULT_fpu)
      #include "polarphp/utils/ARMTargetParserDefs.h"
         .defaultCond(arm::FK_INVALID);
}

unsigned get_default_extensions(StringRef cpu, ArchKind archKind)
{
   if (cpu == "generic") {
      return sg_archNames[static_cast<unsigned>(archKind)].ArchBaseExtensions;
   }
   return StringSwitch<unsigned>(cpu)
      #define ARM_CPU_NAME(NAME, ID, DEFAULT_fpu, IS_DEFAULT, DEFAULT_EXT)           \
         .cond(NAME,                                                                  \
         sg_archNames[static_cast<unsigned>(ArchKind::ID)].ArchBaseExtensions |    \
         DEFAULT_EXT)
      #include "polarphp/utils/ARMTargetParserDefs.h"
         .defaultCond(AEK_INVALID);
}

bool get_hw_div_features(unsigned hwDivKind,
                         std::vector<StringRef> &features)
{

   if (hwDivKind == AEK_INVALID) {
      return false;
   }
   if (hwDivKind & AEK_HWDIVARM) {
      features.push_back("+hwdiv-arm");
   } else {
      features.push_back("-hwdiv-arm");
   }
   if (hwDivKind & AEK_HWDIVTHUMB) {
      features.push_back("+hwdiv");
   } else {
      features.push_back("-hwdiv");
   }
   return true;
}

bool get_extension_features(unsigned extensions,
                            std::vector<StringRef> &features)
{
   if (extensions == AEK_INVALID) {
      return false;
   }
   if (extensions & AEK_CRC) {
      features.push_back("+crc");
   } else {
      features.push_back("-crc");
   }
   if (extensions & AEK_DSP) {
      features.push_back("+dsp");
   } else {
      features.push_back("-dsp");
   }

   if (extensions & AEK_FP16FML) {
      features.push_back("+fp16fml");
   } else {
      features.push_back("-fp16fml");
   }

   if (extensions & AEK_RAS) {
      features.push_back("+ras");
   } else {
      features.push_back("-ras");
   }

   if (extensions & AEK_DOTPROD) {
      features.push_back("+dotprod");
   } else {
      features.push_back("-dotprod");
   }
   return get_hw_div_features(extensions, features);
}

StringRef get_arch_name(ArchKind archKind)
{
   return sg_archNames[static_cast<unsigned>(archKind)].getName();
}

StringRef get_cpu_attr(ArchKind archKind)
{
   return sg_archNames[static_cast<unsigned>(archKind)].getCPUAttr();
}

StringRef get_sub_arch(ArchKind archKind)
{
   return sg_archNames[static_cast<unsigned>(archKind)].getSubArch();
}

unsigned get_arch_attr(ArchKind archKind)
{
   return sg_archNames[static_cast<unsigned>(archKind)].ArchAttr;
}

StringRef get_arch_ext_name(unsigned archExtKind)
{
   for (const auto item : sg_archExtNames) {
      if (archExtKind == item.ID) {
         return item.getName();
      }
   }
   return StringRef();
}

StringRef get_arch_ext_feature(StringRef archExt)
{
   if (archExt.startsWith("no")) {
      StringRef ArchExtBase(archExt.substr(2));
      for (const auto item : sg_archExtNames) {
         if (item.NegFeature && ArchExtBase == item.getName()) {
            return StringRef(item.NegFeature);
         }
      }
   }
   for (const auto item : sg_archExtNames) {
      if (item.Feature && archExt == item.getName()) {
         return StringRef(item.Feature);
      }
   }
   return StringRef();
}

StringRef get_hw_div_name(unsigned hwDivKind)
{
   for (const auto item : sg_hwDivNames) {
      if (hwDivKind == item.ID) {
         return item.getName();
      }
   }
   return StringRef();
}

StringRef get_default_cpu(StringRef arch)
{
   ArchKind archKind = parse_arch(arch);
   if (archKind == ArchKind::INVALID) {
      return StringRef();
   }
   // Look for multiple AKs to find the default for pair archKind+Name.
   for (const auto cpu : sg_cpuNames) {
      if (cpu.ArchID == archKind && cpu.Default) {
         return cpu.getName();
      }
   }
   // If we can't find a default then target the architecture instead
   return "generic";
}

unsigned parse_hw_div(StringRef hwDiv)
{
   StringRef syn = get_hw_div_synonym(hwDiv);
   for (const auto item : sg_hwDivNames) {
      if (syn == item.getName()) {
         return item.ID;
      }
   }
   return AEK_INVALID;
}

unsigned parse_arch_ext(StringRef archExt)
{
   for (const auto item : sg_archExtNames) {
      if (archExt == item.getName()) {
         return item.ID;
      }
   }
   return AEK_INVALID;
}

ArchKind parse_cpu_arch(StringRef cpu)
{
   for (const auto C : sg_cpuNames) {
      if (cpu == C.getName())
         return C.ArchID;
   }
   return ArchKind::INVALID;
}

void fill_valid_cpu_arch_list(SmallVectorImpl<StringRef> &values)
{
   for (const CpuNames<ArchKind> &arch : sg_cpuNames) {
      if (arch.ArchID != ArchKind::INVALID) {
         values.push_back(arch.getName());
      }
   }
}

StringRef compute_default_target_abi(const Triple &triple, StringRef cpu)
{
   StringRef archName =
         cpu.empty() ? triple.getArchName() : get_arch_name(parse_cpu_arch(cpu));

   if (triple.isOSBinFormatMachO()) {
      if (triple.getEnvironment() == Triple::EnvironmentType::EABI ||
          triple.getOS() == Triple::OSType::UnknownOS ||
          parse_arch_profile(archName) == ProfileKind::M) {
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
      if (triple.isOSNetBSD())
         return "apcs-gnu";
      if (triple.isOSOpenBSD())
         return "aapcs-linux";
      return "aapcs";
   }
}

} // arm
} // polar

