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
// This file implements a target parser to recognise aarch64 hardware features
// such as FPU/cpu/ARCH and extension names.
//
//===----------------------------------------------------------------------===//

#include "polarphp/utils/AArch64TargetParser.h"
#include "polarphp/basic/adt/StringRef.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include <cctype>
#include <vector>

namespace polar {
namespace aarch64 {

using polar::basic::StringRef;
using polar::basic::StringSwitch;

namespace {
unsigned check_arch_version(StringRef arch)
{
   if (arch.size() >= 2 && arch[0] == 'v' && std::isdigit(arch[1])) {
      return (arch[1] - 48);
   }
   return 0;
}
} // anonymous namespace

unsigned get_default_fpu(StringRef cpu, aarch64::ArchKind archKind)
{
   if (cpu == "generic") {
      return sg_aarch64ARCHNames[static_cast<unsigned>(archKind)].DefaultFPU;
   }
   return StringSwitch<unsigned>(cpu)
      #define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)       \
         .cond(NAME, arm::DEFAULT_FPU)
      #include "polarphp/utils/AArch64TargetParserDefs.h"
         .defaultCond(arm::FK_INVALID);
}

unsigned get_default_extensions(StringRef cpu, aarch64::ArchKind archKind)
{
   if (cpu == "generic") {
      return sg_aarch64ARCHNames[static_cast<unsigned>(archKind)].ArchBaseExtensions;
   }
   return StringSwitch<unsigned>(cpu)
      #define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)       \
         .cond(NAME, sg_aarch64ARCHNames[static_cast<unsigned>(ArchKind::ID)]            \
         .ArchBaseExtensions |                                    \
         DEFAULT_EXT)
      #include "polarphp/utils/AArch64TargetParserDefs.h"
         .defaultCond(aarch64::AEK_INVALID);
}

ArchKind get_cpu_arch_kind(StringRef cpu)
{
   if (cpu == "generic") {
      return ArchKind::ARMV8A;
   }
   return StringSwitch<aarch64::ArchKind>(cpu)
      #define AARCH64_CPU_NAME(NAME, ID, DEFAULT_FPU, IS_DEFAULT, DEFAULT_EXT)       \
         .cond(NAME, ArchKind::ID)
      #include "polarphp/utils/AArch64TargetParserDefs.h"
         .defaultCond(ArchKind::INVALID);
}

bool get_extension_features(unsigned extensions,
                            std::vector<StringRef> &features)
{
   if (extensions == aarch64::AEK_INVALID) {
      return false;
   }
   if (extensions & AEK_FP) {
      features.push_back("+fp-armv8");
   }
   if (extensions & AEK_SIMD) {
      features.push_back("+neon");
   }
   if (extensions & AEK_CRC) {
      features.push_back("+crc");
   }
   if (extensions & AEK_CRYPTO) {
      features.push_back("+crypto");
   }
   if (extensions & AEK_DOTPROD) {
      features.push_back("+dotprod");
   }
   if (extensions & AEK_FP16FML) {
      features.push_back("+fp16fml");
   }
   if (extensions & AEK_FP16) {
      features.push_back("+fullfp16");
   }
   if (extensions & AEK_PROFILE) {
      features.push_back("+spe");
   }
   if (extensions & AEK_RAS) {
      features.push_back("+ras");
   }
   if (extensions & AEK_LSE) {
      features.push_back("+lse");
   }
   if (extensions & AEK_RDM) {
      features.push_back("+rdm");
   }
   if (extensions & AEK_SVE) {
      features.push_back("+sve");
   }
   if (extensions & AEK_RCPC) {
      features.push_back("+rcpc");
   }
   return true;
}

bool get_arch_features(aarch64::ArchKind archKind,
                       std::vector<StringRef> &features)
{
   if (archKind == ArchKind::ARMV8_1A)
      features.push_back("+v8.1a");
   if (archKind == ArchKind::ARMV8_2A)
      features.push_back("+v8.2a");
   if (archKind == ArchKind::ARMV8_3A)
      features.push_back("+v8.3a");
   if (archKind == ArchKind::ARMV8_4A)
      features.push_back("+v8.4a");
   if (archKind == ArchKind::ARMV8_5A)
      features.push_back("+v8.5a");

   return archKind != ArchKind::INVALID;
}

StringRef get_arch_name(aarch64::ArchKind archKind)
{
   return sg_aarch64ARCHNames[static_cast<unsigned>(archKind)].getName();
}

StringRef get_cpu_attr(aarch64::ArchKind archKind)
{
   return sg_aarch64ARCHNames[static_cast<unsigned>(archKind)].getCPUAttr();
}

StringRef get_sub_arch(aarch64::ArchKind archKind)
{
   return sg_aarch64ARCHNames[static_cast<unsigned>(archKind)].getSubArch();
}

unsigned get_arch_attr(aarch64::ArchKind archKind)
{
   return sg_aarch64ARCHNames[static_cast<unsigned>(archKind)].ArchAttr;
}

StringRef get_arch_ext_name(unsigned archExtKind)
{
   for (const auto &item : sg_aarch64ARCHExtNames) {
      if (archExtKind == item.ID) {
         return item.getName();
      }
   }
   return StringRef();
}

StringRef get_arch_ext_feature(StringRef archExt)
{
   if (archExt.startsWith("no")) {
      StringRef archExtBase(archExt.substr(2));
      for (const auto &item : sg_aarch64ARCHExtNames) {
         if (item.NegFeature && archExtBase == item.getName()) {
            return StringRef(item.NegFeature);
         }
      }
   }

   for (const auto &item : sg_aarch64ARCHExtNames) {
      if (item.Feature && archExt == item.getName()) {
         return StringRef(item.Feature);
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
   for (const auto &cpu : sg_aarch64CPUNames) {
      if (cpu.ArchID == archKind && cpu.Default) {
          return cpu.getName();
      }
   }
   // If we can't find a default then target the architecture instead
   return "generic";
}

void fill_valid_cpu_arch_list(SmallVectorImpl<StringRef> &values)
{
   for (const auto &arch : sg_aarch64CPUNames) {
      if (arch.ArchID != ArchKind::INVALID) {
         values.push_back(arch.getName());
      }
   }
}

bool is_x18_reserved_by_default(const Triple &tt)
{
   return tt.isAndroid() || tt.isOSDarwin() || tt.isOSFuchsia() ||
         tt.isOSWindows();
}

// Allows partial match, ex. "v8a" matches "armv8a".
ArchKind parse_arch(StringRef arch)
{
   arch = arm::get_canonical_arch_name(arch);
   if (check_arch_version(arch) < 8) {
      return ArchKind::INVALID;
   }

   StringRef Syn = arm::get_arch_synonym(arch);
   for (const auto item : sg_aarch64ARCHNames) {
      if (item.getName().endsWith(Syn)) {
         return item.ID;
      }
   }
   return ArchKind::INVALID;
}

ArchExtKind parse_arch_ext(StringRef archExt)
{
   for (const auto item : sg_aarch64ARCHExtNames) {
      if (archExt == item.getName()) {
         return static_cast<ArchExtKind>(item.ID);
      }
   }
   return aarch64::AEK_INVALID;
}

ArchKind parse_cpu_arch(StringRef cpu)
{
   for (const auto item : sg_aarch64CPUNames) {
      if (cpu == item.getName()) {
         return item.ArchID;
      }
   }
   return ArchKind::INVALID;
}

} // aarch64
} // polar
