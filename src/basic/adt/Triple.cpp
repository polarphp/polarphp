// This source file is part of the polarphp.org open source project
//
// Copyright (c) 2017 - 2018 polarphp software foundation
// Copyright (c) 2017 - 2018 zzu_softboy <zzu_softboy@163.com>
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://polarphp.org/LICENSE.txt for license information
// See https://polarphp.org/CONTRIBUTORS.txt for the list of polarphp project authors
//
// Created by polarboy on 2018/06/11.

#include "polarphp/basic/adt/Triple.h"
#include "polarphp/basic/adt/StlExtras.h"
#include "polarphp/basic/adt/SmallString.h"
#include "polarphp/basic/adt/StringSwitch.h"
#include "polarphp/utils/ErrorHandling.h"
#include "polarphp/utils/TargetParser.h"
#include "polarphp/utils/Host.h"
#include <cstring>

namespace polar {
namespace basic {

namespace arm = polar::arm;

StringRef Triple::getArchTypeName(Triple::ArchType kind)
{
   switch (kind) {
   case ArchType::UnknownArch:    return "unknown";

   case ArchType::aarch64:        return "aarch64";
   case ArchType::aarch64_be:     return "aarch64_be";
   case ArchType::arm:            return "arm";
   case ArchType::armeb:          return "armeb";
   case ArchType::arc:            return "arc";
   case ArchType::avr:            return "avr";
   case ArchType::bpfel:          return "bpfel";
   case ArchType::bpfeb:          return "bpfeb";
   case ArchType::hexagon:        return "hexagon";
   case ArchType::mips:           return "mips";
   case ArchType::mipsel:         return "mipsel";
   case ArchType::mips64:         return "mips64";
   case ArchType::mips64el:       return "mips64el";
   case ArchType::msp430:         return "msp430";
   case ArchType::nios2:          return "nios2";
   case ArchType::ppc64:          return "powerpc64";
   case ArchType::ppc64le:        return "powerpc64le";
   case ArchType::ppc:            return "powerpc";
   case ArchType::r600:           return "r600";
   case ArchType::amdgcn:         return "amdgcn";
   case ArchType::riscv32:        return "riscv32";
   case ArchType::riscv64:        return "riscv64";
   case ArchType::sparc:          return "sparc";
   case ArchType::sparcv9:        return "sparcv9";
   case ArchType::sparcel:        return "sparcel";
   case ArchType::systemz:        return "s390x";
   case ArchType::tce:            return "tce";
   case ArchType::tcele:          return "tcele";
   case ArchType::thumb:          return "thumb";
   case ArchType::thumbeb:        return "thumbeb";
   case ArchType::x86:            return "i386";
   case ArchType::x86_64:         return "x86_64";
   case ArchType::xcore:          return "xcore";
   case ArchType::nvptx:          return "nvptx";
   case ArchType::nvptx64:        return "nvptx64";
   case ArchType::le32:           return "le32";
   case ArchType::le64:           return "le64";
   case ArchType::amdil:          return "amdil";
   case ArchType::amdil64:        return "amdil64";
   case ArchType::hsail:          return "hsail";
   case ArchType::hsail64:        return "hsail64";
   case ArchType::spir:           return "spir";
   case ArchType::spir64:         return "spir64";
   case ArchType::kalimba:        return "kalimba";
   case ArchType::lanai:          return "lanai";
   case ArchType::shave:          return "shave";
   case ArchType::wasm32:         return "wasm32";
   case ArchType::wasm64:         return "wasm64";
   case ArchType::renderscript32: return "renderscript32";
   case ArchType::renderscript64: return "renderscript64";
   }
   polar_unreachable("Invalid ArchType!");
}

StringRef Triple::getArchTypePrefix(ArchType kind)
{
   switch (kind) {
   default:
      return StringRef();

   case ArchType::aarch64:
   case ArchType::aarch64_be:  return "aarch64";

   case ArchType::arc:         return "arc";

   case ArchType::arm:
   case ArchType::armeb:
   case ArchType::thumb:
   case ArchType::thumbeb:     return "arm";

   case ArchType::avr:         return "avr";

   case ArchType::ppc64:
   case ArchType::ppc64le:
   case ArchType::ppc:         return "ppc";

   case ArchType::mips:
   case ArchType::mipsel:
   case ArchType::mips64:
   case ArchType::mips64el:    return "mips";

   case ArchType::nios2:       return "nios2";

   case ArchType::hexagon:     return "hexagon";

   case ArchType::amdgcn:      return "amdgcn";
   case ArchType::r600:        return "r600";

   case ArchType::bpfel:
   case ArchType::bpfeb:       return "bpf";

   case ArchType::sparcv9:
   case ArchType::sparcel:
   case ArchType::sparc:       return "sparc";

   case ArchType::systemz:     return "s390";

   case ArchType::x86:
   case ArchType::x86_64:      return "x86";

   case ArchType::xcore:       return "xcore";

      // NVPTX intrinsics are namespaced under nvvm.
   case ArchType::nvptx:       return "nvvm";
   case ArchType::nvptx64:     return "nvvm";

   case ArchType::le32:        return "le32";
   case ArchType::le64:        return "le64";

   case ArchType::amdil:
   case ArchType::amdil64:     return "amdil";

   case ArchType::hsail:
   case ArchType::hsail64:     return "hsail";

   case ArchType::spir:
   case ArchType::spir64:      return "spir";
   case ArchType::kalimba:     return "kalimba";
   case ArchType::lanai:       return "lanai";
   case ArchType::shave:       return "shave";
   case ArchType::wasm32:
   case ArchType::wasm64:      return "wasm";

   case ArchType::riscv32:
   case ArchType::riscv64:     return "riscv";
   }
}

StringRef Triple::getVendorTypeName(VendorType kind)
{
   switch (kind) {
   case VendorType::UnknownVendor: return "unknown";

   case VendorType::Apple: return "apple";
   case VendorType::PC: return "pc";
   case VendorType::SCEI: return "scei";
   case VendorType::BGP: return "bgp";
   case VendorType::BGQ: return "bgq";
   case VendorType::Freescale: return "fsl";
   case VendorType::IBM: return "ibm";
   case VendorType::ImaginationTechnologies: return "img";
   case VendorType::MipsTechnologies: return "mti";
   case VendorType::NVIDIA: return "nvidia";
   case VendorType::CSR: return "csr";
   case VendorType::Myriad: return "myriad";
   case VendorType::AMD: return "amd";
   case VendorType::Mesa: return "mesa";
   case VendorType::SUSE: return "suse";
   case VendorType::OpenEmbedded: return "oe";
   }

   polar_unreachable("Invalid VendorType!");
}

StringRef Triple::getOSTypeName(OSType kind)
{
   switch (kind) {
   case OSType::UnknownOS: return "unknown";
   case OSType::Ananas: return "ananas";
   case OSType::CloudABI: return "cloudabi";
   case OSType::Darwin: return "darwin";
   case OSType::DragonFly: return "dragonfly";
   case OSType::FreeBSD: return "freebsd";
   case OSType::Fuchsia: return "fuchsia";
   case OSType::IOS: return "ios";
   case OSType::KFreeBSD: return "kfreebsd";
   case OSType::Linux: return "linux";
   case OSType::Lv2: return "lv2";
   case OSType::MacOSX: return "macosx";
   case OSType::NetBSD: return "netbsd";
   case OSType::OpenBSD: return "openbsd";
   case OSType::Solaris: return "solaris";
   case OSType::Win32: return "windows";
   case OSType::Haiku: return "haiku";
   case OSType::Minix: return "minix";
   case OSType::RTEMS: return "rtems";
   case OSType::NaCl: return "nacl";
   case OSType::CNK: return "cnk";
   case OSType::AIX: return "aix";
   case OSType::CUDA: return "cuda";
   case OSType::NVCL: return "nvcl";
   case OSType::AMDHSA: return "amdhsa";
   case OSType::PS4: return "ps4";
   case OSType::ELFIAMCU: return "elfiamcu";
   case OSType::TvOS: return "tvos";
   case OSType::WatchOS: return "watchos";
   case OSType::Mesa3D: return "mesa3d";
   case OSType::Contiki: return "contiki";
   case OSType::AMDPAL: return "amdpal";
   case OSType::HermitCore: return "hermit";
   case OSType::Hurd: return "hurd";
   }

   polar_unreachable("Invalid OSType");
}

StringRef Triple::getEnvironmentTypeName(EnvironmentType kind)
{
   switch (kind) {
   case Triple::EnvironmentType::UnknownEnvironment: return "unknown";
   case Triple::EnvironmentType::GNU: return "gnu";
   case Triple::EnvironmentType::GNUABIN32: return "gnuabin32";
   case Triple::EnvironmentType::GNUABI64: return "gnuabi64";
   case Triple::EnvironmentType::GNUEABIHF: return "gnueabihf";
   case Triple::EnvironmentType::GNUEABI: return "gnueabi";
   case Triple::EnvironmentType::GNUX32: return "gnux32";
   case Triple::EnvironmentType::CODE16: return "code16";
   case Triple::EnvironmentType::EABI: return "eabi";
   case Triple::EnvironmentType::EABIHF: return "eabihf";
   case Triple::EnvironmentType::Android: return "android";
   case Triple::EnvironmentType::Musl: return "musl";
   case Triple::EnvironmentType::MuslEABI: return "musleabi";
   case Triple::EnvironmentType::MuslEABIHF: return "musleabihf";
   case Triple::EnvironmentType::MSVC: return "msvc";
   case Triple::EnvironmentType::Itanium: return "itanium";
   case Triple::EnvironmentType::Cygnus: return "cygnus";
   case Triple::EnvironmentType::CoreCLR: return "coreclr";
   case Triple::EnvironmentType::Simulator: return "simulator";
   }

   polar_unreachable("Invalid Triple::EnvironmentType!");
}

namespace {

Triple::ArchType parse_bpf_arch(StringRef archName)
{
   if (archName.equals("bpf")) {
      if (polar::sys::sg_isLittleEndianHost) {
         return Triple::ArchType::bpfel;
      } else {
         return Triple::ArchType::bpfeb;
      }
   } else if (archName.equals("bpf_be") || archName.equals("bpfeb")) {
      return Triple::ArchType::bpfeb;
   } else if (archName.equals("bpf_le") || archName.equals("bpfel")) {
      return Triple::ArchType::bpfel;
   } else {
      return Triple::ArchType::UnknownArch;
   }
}

} // anonymous namespace

Triple::ArchType Triple::getArchTypeForPolarName(StringRef name)
{
   ArchType BPFArch(parse_bpf_arch(name));
   return StringSwitch<ArchType>(name)
         .cond("aarch64", ArchType::aarch64)
         .cond("aarch64_be", ArchType::aarch64_be)
         .cond("arc", ArchType::arc)
         .cond("arm64", ArchType::aarch64) // "arm64" is an alias for "aarch64"
         .cond("arm", ArchType::arm)
         .cond("armeb", ArchType::armeb)
         .cond("avr", ArchType::avr)
         .startsWith("bpf", BPFArch)
         .cond("mips", ArchType::mips)
         .cond("mipsel", ArchType::mipsel)
         .cond("mips64", ArchType::mips64)
         .cond("mips64el", ArchType::mips64el)
         .cond("msp430", ArchType::msp430)
         .cond("nios2", ArchType::nios2)
         .cond("ppc64", ArchType::ppc64)
         .cond("ppc32", ArchType::ppc)
         .cond("ppc", ArchType::ppc)
         .cond("ppc64le", ArchType::ppc64le)
         .cond("r600", ArchType::r600)
         .cond("amdgcn", ArchType::amdgcn)
         .cond("riscv32", ArchType::riscv32)
         .cond("riscv64", ArchType::riscv64)
         .cond("hexagon", ArchType::hexagon)
         .cond("sparc", ArchType::sparc)
         .cond("sparcel", ArchType::sparcel)
         .cond("sparcv9", ArchType::sparcv9)
         .cond("systemz", ArchType::systemz)
         .cond("tce", ArchType::tce)
         .cond("tcele", ArchType::tcele)
         .cond("thumb", ArchType::thumb)
         .cond("thumbeb", ArchType::thumbeb)
         .cond("x86", ArchType::x86)
         .cond("x86-64", ArchType::x86_64)
         .cond("xcore", ArchType::xcore)
         .cond("nvptx", ArchType::nvptx)
         .cond("nvptx64", ArchType::nvptx64)
         .cond("le32", ArchType::le32)
         .cond("le64", ArchType::le64)
         .cond("amdil", ArchType::amdil)
         .cond("amdil64", ArchType::amdil64)
         .cond("hsail", ArchType::hsail)
         .cond("hsail64", ArchType::hsail64)
         .cond("spir", ArchType::spir)
         .cond("spir64", ArchType::spir64)
         .cond("kalimba", ArchType::kalimba)
         .cond("lanai", ArchType::lanai)
         .cond("shave", ArchType::shave)
         .cond("wasm32", ArchType::wasm32)
         .cond("wasm64", ArchType::wasm64)
         .cond("renderscript32", ArchType::renderscript32)
         .cond("renderscript64", ArchType::renderscript64)
         .defaultCond(ArchType::UnknownArch);
}

namespace {

Triple::ArchType parse_arm_arch(StringRef archName)
{
   arm::ISAKind isa = arm::parse_arch_isa(archName);
   arm::EndianKind endian = arm::parse_arch_endian(archName);

   Triple::ArchType arch = Triple::ArchType::UnknownArch;
   switch (endian) {
   case arm::EndianKind::LITTLE: {
      switch (isa) {
      case arm::ISAKind::ARM:
         arch = Triple::ArchType::arm;
         break;
      case arm::ISAKind::THUMB:
         arch = Triple::ArchType::thumb;
         break;
      case arm::ISAKind::AARCH64:
         arch = Triple::ArchType::aarch64;
         break;
      case arm::ISAKind::INVALID:
         break;
      }
      break;
   }
   case arm::EndianKind::BIG: {
      switch (isa) {
      case arm::ISAKind::ARM:
         arch = Triple::ArchType::armeb;
         break;
      case arm::ISAKind::THUMB:
         arch = Triple::ArchType::thumbeb;
         break;
      case arm::ISAKind::AARCH64:
         arch = Triple::ArchType::aarch64_be;
         break;
      case arm::ISAKind::INVALID:
         break;
      }
      break;
   }
   case arm::EndianKind::INVALID: {
      break;
   }
   }

   archName = arm::get_canonical_arch_name(archName);
   if (archName.empty()) {
      return Triple::ArchType::UnknownArch;
   }

   // Thumb only exists in v4+
   if (isa == arm::ISAKind::THUMB &&
       (archName.startsWith("v2") || archName.startsWith("v3"))) {
      return Triple::ArchType::UnknownArch;
   }
   // Thumb only for v6m
   arm::ProfileKind profile = arm::parse_arch_profile(archName);
   unsigned version = arm::parse_arch_version(archName);
   if (profile == arm::ProfileKind::M && version == 6) {
      if (endian == arm::EndianKind::BIG) {
         return Triple::ArchType::thumbeb;
      } else {
         return Triple::ArchType::thumb;
      }
   }
   return arch;
}

Triple::ArchType parse_arch(StringRef archName)
{
   auto archType = StringSwitch<Triple::ArchType>(archName)
         .conds("i386", "i486", "i586", "i686", Triple::ArchType::x86)
         // FIXME: Do we need to support these?
         .conds("i786", "i886", "i986", Triple::ArchType::x86)
         .conds("amd64", "x86_64", "x86_64h", Triple::ArchType::x86_64)
         .conds("powerpc", "ppc", "ppc32", Triple::ArchType::ppc)
         .conds("powerpc64", "ppu", "ppc64", Triple::ArchType::ppc64)
         .conds("powerpc64le", "ppc64le", Triple::ArchType::ppc64le)
         .cond("xscale", Triple::ArchType::arm)
         .cond("xscaleeb", Triple::ArchType::armeb)
         .cond("aarch64", Triple::ArchType::aarch64)
         .cond("aarch64_be", Triple::ArchType::aarch64_be)
         .cond("arc", Triple::ArchType::arc)
         .cond("arm64", Triple::ArchType::aarch64)
         .cond("arm", Triple::ArchType::arm)
         .cond("armeb", Triple::ArchType::armeb)
         .cond("thumb", Triple::ArchType::thumb)
         .cond("thumbeb", Triple::ArchType::thumbeb)
         .cond("avr", Triple::ArchType::avr)
         .cond("msp430", Triple::ArchType::msp430)
         .conds("mips", "mipseb", "mipsallegrex", "mipsisa32r6",
                "mipsr6", Triple::ArchType::mips)
         .conds("mipsel", "mipsallegrexel", "mipsisa32r6el", "mipsr6el",
                Triple::ArchType::mipsel)
         .conds("mips64", "mips64eb", "mipsn32", "mipsisa64r6",
                "mips64r6", "mipsn32r6", Triple::ArchType::mips64)
         .conds("mips64el", "mipsn32el", "mipsisa64r6el", "mips64r6el",
                "mipsn32r6el", Triple::ArchType::mips64el)
         .cond("nios2", Triple::ArchType::nios2)
         .cond("r600", Triple::ArchType::r600)
         .cond("amdgcn", Triple::ArchType::amdgcn)
         .cond("riscv32", Triple::ArchType::riscv32)
         .cond("riscv64", Triple::ArchType::riscv64)
         .cond("hexagon", Triple::ArchType::hexagon)
         .conds("s390x", "systemz", Triple::ArchType::systemz)
         .cond("sparc", Triple::ArchType::sparc)
         .cond("sparcel", Triple::ArchType::sparcel)
         .conds("sparcv9", "sparc64", Triple::ArchType::sparcv9)
         .cond("tce", Triple::ArchType::tce)
         .cond("tcele", Triple::ArchType::tcele)
         .cond("xcore", Triple::ArchType::xcore)
         .cond("nvptx", Triple::ArchType::nvptx)
         .cond("nvptx64", Triple::ArchType::nvptx64)
         .cond("le32", Triple::ArchType::le32)
         .cond("le64", Triple::ArchType::le64)
         .cond("amdil", Triple::ArchType::amdil)
         .cond("amdil64", Triple::ArchType::amdil64)
         .cond("hsail", Triple::ArchType::hsail)
         .cond("hsail64", Triple::ArchType::hsail64)
         .cond("spir", Triple::ArchType::spir)
         .cond("spir64", Triple::ArchType::spir64)
         .startsWith("kalimba", Triple::ArchType::kalimba)
         .cond("lanai", Triple::ArchType::lanai)
         .cond("shave", Triple::ArchType::shave)
         .cond("wasm32", Triple::ArchType::wasm32)
         .cond("wasm64", Triple::ArchType::wasm64)
         .cond("renderscript32", Triple::ArchType::renderscript32)
         .cond("renderscript64", Triple::ArchType::renderscript64)
         .defaultCond(Triple::ArchType::UnknownArch);

   // Some architectures require special parsing logic just to compute the
   // ArchType result.
   if (archType == Triple::ArchType::UnknownArch) {
      if (archName.startsWith("arm") || archName.startsWith("thumb") ||
          archName.startsWith("aarch64")) {
         return parse_arm_arch(archName);
      }
      if (archName.startsWith("bpf")) {
         return parse_bpf_arch(archName);
      }
   }
   return archType;
}

Triple::VendorType parse_vendor(StringRef vendorName)
{
   return StringSwitch<Triple::VendorType>(vendorName)
         .cond("apple", Triple::VendorType::Apple)
         .cond("pc", Triple::VendorType::PC)
         .cond("scei", Triple::VendorType::SCEI)
         .cond("bgp", Triple::VendorType::BGP)
         .cond("bgq", Triple::VendorType::BGQ)
         .cond("fsl", Triple::VendorType::Freescale)
         .cond("ibm", Triple::VendorType::IBM)
         .cond("img", Triple::VendorType::ImaginationTechnologies)
         .cond("mti", Triple::VendorType::MipsTechnologies)
         .cond("nvidia", Triple::VendorType::NVIDIA)
         .cond("csr", Triple::VendorType::CSR)
         .cond("myriad", Triple::VendorType::Myriad)
         .cond("amd", Triple::VendorType::AMD)
         .cond("mesa", Triple::VendorType::Mesa)
         .cond("suse", Triple::VendorType::SUSE)
         .cond("oe", Triple::VendorType::OpenEmbedded)
         .defaultCond(Triple::VendorType::UnknownVendor);
}

Triple::OSType parse_os(StringRef osName)
{
   return StringSwitch<Triple::OSType>(osName)
         .startsWith("ananas", Triple::OSType::Ananas)
         .startsWith("cloudabi", Triple::OSType::CloudABI)
         .startsWith("darwin", Triple::OSType::Darwin)
         .startsWith("dragonfly", Triple::OSType::DragonFly)
         .startsWith("freebsd", Triple::OSType::FreeBSD)
         .startsWith("fuchsia", Triple::OSType::Fuchsia)
         .startsWith("ios", Triple::OSType::IOS)
         .startsWith("kfreebsd", Triple::OSType::KFreeBSD)
         .startsWith("linux", Triple::OSType::Linux)
         .startsWith("lv2", Triple::OSType::Lv2)
         .startsWith("macos", Triple::OSType::MacOSX)
         .startsWith("netbsd", Triple::OSType::NetBSD)
         .startsWith("openbsd", Triple::OSType::OpenBSD)
         .startsWith("solaris", Triple::OSType::Solaris)
         .startsWith("win32", Triple::OSType::Win32)
         .startsWith("windows", Triple::OSType::Win32)
         .startsWith("haiku", Triple::OSType::Haiku)
         .startsWith("minix", Triple::OSType::Minix)
         .startsWith("rtems", Triple::OSType::RTEMS)
         .startsWith("nacl", Triple::OSType::NaCl)
         .startsWith("cnk", Triple::OSType::CNK)
         .startsWith("aix", Triple::OSType::AIX)
         .startsWith("cuda", Triple::OSType::CUDA)
         .startsWith("nvcl", Triple::OSType::NVCL)
         .startsWith("amdhsa", Triple::OSType::AMDHSA)
         .startsWith("ps4", Triple::OSType::PS4)
         .startsWith("elfiamcu", Triple::OSType::ELFIAMCU)
         .startsWith("tvos", Triple::OSType::TvOS)
         .startsWith("watchos", Triple::OSType::WatchOS)
         .startsWith("mesa3d", Triple::OSType::Mesa3D)
         .startsWith("contiki", Triple::OSType::Contiki)
         .startsWith("amdpal", Triple::OSType::AMDPAL)
         .startsWith("hermit", Triple::OSType::HermitCore)
         .startsWith("hurd", Triple::OSType::Hurd)
         .defaultCond(Triple::OSType::UnknownOS);
}

Triple::EnvironmentType parse_environment(StringRef environmentName)
{
   return StringSwitch<Triple::EnvironmentType>(environmentName)
         .startsWith("eabihf", Triple::EnvironmentType::EABIHF)
         .startsWith("eabi", Triple::EnvironmentType::EABI)
         .startsWith("gnuabin32", Triple::EnvironmentType::GNUABIN32)
         .startsWith("gnuabi64", Triple::EnvironmentType::GNUABI64)
         .startsWith("gnueabihf", Triple::EnvironmentType::GNUEABIHF)
         .startsWith("gnueabi", Triple::EnvironmentType::GNUEABI)
         .startsWith("gnux32", Triple::EnvironmentType::GNUX32)
         .startsWith("code16", Triple::EnvironmentType::CODE16)
         .startsWith("gnu", Triple::EnvironmentType::GNU)
         .startsWith("android", Triple::EnvironmentType::Android)
         .startsWith("musleabihf", Triple::EnvironmentType::MuslEABIHF)
         .startsWith("musleabi", Triple::EnvironmentType::MuslEABI)
         .startsWith("musl", Triple::EnvironmentType::Musl)
         .startsWith("msvc", Triple::EnvironmentType::MSVC)
         .startsWith("itanium", Triple::EnvironmentType::Itanium)
         .startsWith("cygnus", Triple::EnvironmentType::Cygnus)
         .startsWith("coreclr", Triple::EnvironmentType::CoreCLR)
         .startsWith("simulator", Triple::EnvironmentType::Simulator)
         .defaultCond(Triple::EnvironmentType::UnknownEnvironment);
}

Triple::ObjectFormatType parse_format(StringRef environmentName)
{
   return StringSwitch<Triple::ObjectFormatType>(environmentName)
         .endsWith("coff", Triple::ObjectFormatType::COFF)
         .endsWith("elf", Triple::ObjectFormatType::ELF)
         .endsWith("macho", Triple::ObjectFormatType::MachO)
         .endsWith("wasm", Triple::ObjectFormatType::Wasm)
         .defaultCond(Triple::ObjectFormatType::UnknownObjectFormat);
}

Triple::SubArchType parse_sub_arch(StringRef subArchName)
{
   if (subArchName.startsWith("mips") &&
       (subArchName.endsWith("r6el") || subArchName.endsWith("r6"))) {
      return Triple::SubArchType::MipsSubArch_r6;
   }

   StringRef armSubArch = arm::get_canonical_arch_name(subArchName);

   // For now, this is the small part. Early return.
   if (armSubArch.empty())
      return StringSwitch<Triple::SubArchType>(subArchName)
            .endsWith("kalimba3", Triple::SubArchType::KalimbaSubArch_v3)
            .endsWith("kalimba4", Triple::SubArchType::KalimbaSubArch_v4)
            .endsWith("kalimba5", Triple::SubArchType::KalimbaSubArch_v5)
            .defaultCond(Triple::SubArchType::NoSubArch);

   // ARM sub arch.
   switch(arm::parse_arch(armSubArch)) {
   case arm::ArchKind::ARMV4:
      return Triple::SubArchType::NoSubArch;
   case arm::ArchKind::ARMV4T:
      return Triple::SubArchType::ARMSubArch_v4t;
   case arm::ArchKind::ARMV5T:
      return Triple::SubArchType::ARMSubArch_v5;
   case arm::ArchKind::ARMV5TE:
   case arm::ArchKind::IWMMXT:
   case arm::ArchKind::IWMMXT2:
   case arm::ArchKind::XSCALE:
   case arm::ArchKind::ARMV5TEJ:
      return Triple::SubArchType::ARMSubArch_v5te;
   case arm::ArchKind::ARMV6:
      return Triple::SubArchType::ARMSubArch_v6;
   case arm::ArchKind::ARMV6K:
   case arm::ArchKind::ARMV6KZ:
      return Triple::SubArchType::ARMSubArch_v6k;
   case arm::ArchKind::ARMV6T2:
      return Triple::SubArchType::ARMSubArch_v6t2;
   case arm::ArchKind::ARMV6M:
      return Triple::SubArchType::ARMSubArch_v6m;
   case arm::ArchKind::ARMV7A:
   case arm::ArchKind::ARMV7R:
      return Triple::SubArchType::ARMSubArch_v7;
   case arm::ArchKind::ARMV7VE:
      return Triple::SubArchType::ARMSubArch_v7ve;
   case arm::ArchKind::ARMV7K:
      return Triple::SubArchType::ARMSubArch_v7k;
   case arm::ArchKind::ARMV7M:
      return Triple::SubArchType::ARMSubArch_v7m;
   case arm::ArchKind::ARMV7S:
      return Triple::SubArchType::ARMSubArch_v7s;
   case arm::ArchKind::ARMV7EM:
      return Triple::SubArchType::ARMSubArch_v7em;
   case arm::ArchKind::ARMV8A:
      return Triple::SubArchType::ARMSubArch_v8;
   case arm::ArchKind::ARMV8_1A:
      return Triple::SubArchType::ARMSubArch_v8_1a;
   case arm::ArchKind::ARMV8_2A:
      return Triple::SubArchType::ARMSubArch_v8_2a;
   case arm::ArchKind::ARMV8_3A:
      return Triple::SubArchType::ARMSubArch_v8_3a;
   case arm::ArchKind::ARMV8_4A:
      return Triple::SubArchType::ARMSubArch_v8_4a;
   case arm::ArchKind::ARMV8_5A:
      return Triple::SubArchType::ARMSubArch_v8_5a;
   case arm::ArchKind::ARMV8R:
      return Triple::SubArchType::ARMSubArch_v8r;
   case arm::ArchKind::ARMV8MBaseline:
      return Triple::SubArchType::ARMSubArch_v8m_baseline;
   case arm::ArchKind::ARMV8MMainline:
      return Triple::SubArchType::ARMSubArch_v8m_mainline;
   default:
      return Triple::SubArchType::NoSubArch;
   }
}

StringRef getObjectFormatTypeName(Triple::ObjectFormatType kind)
{
   switch (kind) {
   case Triple::ObjectFormatType::UnknownObjectFormat: return "";
   case Triple::ObjectFormatType::COFF: return "coff";
   case Triple::ObjectFormatType::ELF: return "elf";
   case Triple::ObjectFormatType::MachO: return "macho";
   case Triple::ObjectFormatType::Wasm: return "wasm";
   }
   polar_unreachable("unknown object format type");
}

Triple::ObjectFormatType get_default_format(const Triple &triple)
{
   switch (triple.getArch()) {
   case Triple::ArchType::UnknownArch:
   case Triple::ArchType::aarch64:
   case Triple::ArchType::arm:
   case Triple::ArchType::thumb:
   case Triple::ArchType::x86:
   case Triple::ArchType::x86_64:
      if (triple.isOSDarwin()) {
         return Triple::ObjectFormatType::MachO;
      } else if (triple.isOSWindows()) {
         return Triple::ObjectFormatType::COFF;
      }
      return Triple::ObjectFormatType::ELF;

   case Triple::ArchType::aarch64_be:
   case Triple::ArchType::arc:
   case Triple::ArchType::amdgcn:
   case Triple::ArchType::amdil:
   case Triple::ArchType::amdil64:
   case Triple::ArchType::armeb:
   case Triple::ArchType::avr:
   case Triple::ArchType::bpfeb:
   case Triple::ArchType::bpfel:
   case Triple::ArchType::hexagon:
   case Triple::ArchType::lanai:
   case Triple::ArchType::hsail:
   case Triple::ArchType::hsail64:
   case Triple::ArchType::kalimba:
   case Triple::ArchType::le32:
   case Triple::ArchType::le64:
   case Triple::ArchType::mips:
   case Triple::ArchType::mips64:
   case Triple::ArchType::mips64el:
   case Triple::ArchType::mipsel:
   case Triple::ArchType::msp430:
   case Triple::ArchType::nios2:
   case Triple::ArchType::nvptx:
   case Triple::ArchType::nvptx64:
   case Triple::ArchType::ppc64le:
   case Triple::ArchType::r600:
   case Triple::ArchType::renderscript32:
   case Triple::ArchType::renderscript64:
   case Triple::ArchType::riscv32:
   case Triple::ArchType::riscv64:
   case Triple::ArchType::shave:
   case Triple::ArchType::sparc:
   case Triple::ArchType::sparcel:
   case Triple::ArchType::sparcv9:
   case Triple::ArchType::spir:
   case Triple::ArchType::spir64:
   case Triple::ArchType::systemz:
   case Triple::ArchType::tce:
   case Triple::ArchType::tcele:
   case Triple::ArchType::thumbeb:
   case Triple::ArchType::xcore:
      return Triple::ObjectFormatType::ELF;

   case Triple::ArchType::ppc:
   case Triple::ArchType::ppc64:
      if (triple.isOSDarwin())
         return Triple::ObjectFormatType::MachO;
      return Triple::ObjectFormatType::ELF;
   case Triple::ArchType::wasm32:
   case Triple::ArchType::wasm64:
       return Triple::ObjectFormatType::Wasm;
   }
   polar_unreachable("unknown architecture");
}

} // anonymous namespace

/// \brief Construct a triple from the string representation provided.
///
/// This stores the string representation and parses the various pieces into
/// enum members.
Triple::Triple(const Twine &str)
   : m_data(str.getStr()), m_arch(ArchType::UnknownArch),
     m_subArch(SubArchType::NoSubArch),
     m_vendor(VendorType::UnknownVendor),
     m_os(OSType::UnknownOS),
     m_environment(EnvironmentType::UnknownEnvironment),
     m_objectFormat(ObjectFormatType::UnknownObjectFormat)
{
   // Do minimal parsing by hand here.
   SmallVector<StringRef, 4> components;
   StringRef(m_data).split(components, '-', /*MaxSplit*/ 3);
   if (components.getSize() > 0) {
      m_arch = parse_arch(components[0]);
      m_subArch = parse_sub_arch(components[0]);
      if (components.getSize() > 1) {
         m_vendor = parse_vendor(components[1]);
         if (components.getSize() > 2) {
            m_os = parse_os(components[2]);
            if (components.getSize() > 3) {
               m_environment = parse_environment(components[3]);
               m_objectFormat = parse_format(components[3]);
            }
         }
      } else {
         m_environment =
               StringSwitch<Triple::EnvironmentType>(components[0])
               .startsWith("mipsn32", Triple::EnvironmentType::GNUABIN32)
               .startsWith("mips64", Triple::EnvironmentType::GNUABI64)
               .startsWith("mipsisa64", Triple::EnvironmentType::GNUABI64)
               .startsWith("mipsisa32", Triple::EnvironmentType::GNU)
               .conds("mips", "mipsel", "mipsr6", "mipsr6el", Triple::EnvironmentType::GNU)
               .defaultCond(Triple::EnvironmentType::UnknownEnvironment);
      }
   }
   if (m_objectFormat == ObjectFormatType::UnknownObjectFormat) {
      m_objectFormat = get_default_format(*this);
   }
}

/// \brief Construct a triple from string representations of the architecture,
/// vendor, and OS.
///
/// This joins each argument into a canonical string representation and parses
/// them into enum members. It leaves the environment unknown and omits it from
/// the string representation.
Triple::Triple(const Twine &archStr, const Twine &vendorStr, const Twine &osStr)
   : m_data((archStr + Twine('-') + vendorStr + Twine('-') + osStr).getStr()),
     m_arch(parse_arch(archStr.getStr())),
     m_subArch(parse_sub_arch(archStr.getStr())),
     m_vendor(parse_vendor(vendorStr.getStr())),
     m_os(parse_os(osStr.getStr())),
     m_environment(), m_objectFormat(ObjectFormatType::UnknownObjectFormat)
{
   m_objectFormat = get_default_format(*this);
}

/// \brief Construct a triple from string representations of the architecture,
/// vendor, OS, and environment.
///
/// This joins each argument into a canonical string representation and parses
/// them into enum members.
Triple::Triple(const Twine &archStr, const Twine &vendorStr, const Twine &osStr,
               const Twine &environmentStr)
   : m_data((archStr + Twine('-') + vendorStr + Twine('-') + osStr + Twine('-') +
             environmentStr).getStr()),
     m_arch(parse_arch(archStr.getStr())),
     m_subArch(parse_sub_arch(archStr.getStr())),
     m_vendor(parse_vendor(vendorStr.getStr())),
     m_os(parse_os(osStr.getStr())),
     m_environment(parse_environment(environmentStr.getStr())),
     m_objectFormat(parse_format(environmentStr.getStr()))
{
   if (m_objectFormat == ObjectFormatType::UnknownObjectFormat) {
      m_objectFormat = get_default_format(*this);
   }
}

std::string Triple::normalize(StringRef str)
{
   bool isMinGW32 = false;
   bool isCygwin = false;

   // Parse into components.
   SmallVector<StringRef, 4> components;
   str.split(components, '-');

   // If the first component corresponds to a known architecture, preferentially
   // use it for the architecture.  If the second component corresponds to a
   // known vendor, preferentially use it for the vendor, etc.  This avoids silly
   // component movement when a component parses as (eg) both a valid arch and a
   // valid os.
   ArchType arch = ArchType::UnknownArch;
   if (components.getSize() > 0) {
      arch = parse_arch(components[0]);
   }

   VendorType vendor = VendorType::UnknownVendor;
   if (components.getSize() > 1) {
      vendor = parse_vendor(components[1]);
   }

   OSType os = OSType::UnknownOS;
   if (components.getSize() > 2) {
      os = parse_os(components[2]);
      isCygwin = components[2].startsWith("cygwin");
      isMinGW32 = components[2].startsWith("mingw");
   }
   EnvironmentType environment = EnvironmentType::UnknownEnvironment;
   if (components.getSize() > 3) {
      environment = parse_environment(components[3]);
   }

   ObjectFormatType objectFormat = ObjectFormatType::UnknownObjectFormat;
   if (components.getSize() > 4) {
      objectFormat = parse_format(components[4]);
   }
   // Note which components are already in their final position.  These will not
   // be moved.
   bool found[4];
   found[0] = arch != ArchType::UnknownArch;
   found[1] = vendor != VendorType::UnknownVendor;
   found[2] = os != OSType::UnknownOS;
   found[3] = environment != EnvironmentType::UnknownEnvironment;

   // If they are not there already, permute the components into their canonical
   // positions by seeing if they parse as a valid architecture, and if so moving
   // the component to the architecture position etc.
   for (unsigned pos = 0; pos != array_lengthof(found); ++pos) {
      if (found[pos]) {
         continue; // Already in the canonical position.
      }
      for (unsigned idx = 0; idx != components.getSize(); ++idx) {
         // Do not reparse any components that already matched.
         if (idx < array_lengthof(found) && found[idx]) {
            continue;
         }
         // Does this component parse as valid for the target position?
         bool valid = false;
         StringRef comp = components[idx];
         switch (pos) {
         default: polar_unreachable("unexpected component type!");
         case 0:
            arch = parse_arch(comp);
            valid = arch != ArchType::UnknownArch;
            break;
         case 1:
            vendor = parse_vendor(comp);
            valid = vendor != VendorType::UnknownVendor;
            break;
         case 2:
            os = parse_os(comp);
            isCygwin = comp.startsWith("cygwin");
            isMinGW32 = comp.startsWith("mingw");
            valid = os != OSType::UnknownOS || isCygwin || isMinGW32;
            break;
         case 3:
            environment = parse_environment(comp);
            valid = environment != EnvironmentType::UnknownEnvironment;
            if (!valid) {
               objectFormat = parse_format(comp);
               valid = objectFormat != ObjectFormatType::UnknownObjectFormat;
            }
            break;
         }
         if (!valid) {
            continue; // Nope, try the next component.
         }
         // Move the component to the target position, pushing any non-fixed
         // components that are in the way to the right.  This tends to give
         // good results in the common cases of a forgotten vendor component
         // or a wrongly positioned environment.
         if (pos < idx) {
            // Insert left, pushing the existing components to the right.  For
            // example, a-b-i386 -> i386-a-b when moving i386 to the front.
            StringRef currentComponent(""); // The empty component.
            // Replace the component we are moving with an empty component.
            std::swap(currentComponent, components[idx]);
            // Insert the component being moved at pos, displacing any existing
            // components to the right.
            for (unsigned i = pos; !currentComponent.empty(); ++i) {
               // Skip over any fixed components.
               while (i < array_lengthof(found) && found[i])
                  ++i;
               // Place the component at the new position, getting the component
               // that was at this position - it will be moved right.
               std::swap(currentComponent, components[i]);
            }
         } else if (pos > idx) {
            // Push right by inserting empty components until the component at idx
            // reaches the target position pos.  For example, pc-a -> -pc-a when
            // moving pc to the secase position.
            do {
               // Insert one empty component at idx.
               StringRef currentComponent(""); // The empty component.
               for (unsigned i = idx; i < components.getSize();) {
                  // Place the component at the new position, getting the component
                  // that was at this position - it will be moved right.
                  std::swap(currentComponent, components[i]);
                  // If it was placed on top of an empty component then we are done.
                  if (currentComponent.empty()) {
                     break;
                  }
                  // Advance to the next component, skipping any fixed components.
                  while (++i < array_lengthof(found) && found[i])
                     ;
               }
               // The last component was pushed off the end - append it.
               if (!currentComponent.empty()) {
                  components.push_back(currentComponent);
               }
               // Advance idx to the component's new position.
               while (++idx < array_lengthof(found) && found[idx])
                  ;
            } while (idx < pos); // Add more until the final position is reached.
         }
         assert(pos < components.getSize() && components[pos] == comp &&
                "Component moved wrong!");
         found[pos] = true;
         break;
      }
   }

   // Replace empty components with "unknown" value.
   for (unsigned i = 0, e = components.size(); i < e; ++i) {
      if (components[i].empty()) {
         components[i] = "unknown";
      }
   }

   // Special case logic goes here.  At this point Arch, Vendor and OS have the
   // correct values for the computed components.
   std::string normalizedEnvironment;
   if (environment == EnvironmentType::Android && components[3].startsWith("androideabi")) {
      StringRef androidVersion = components[3].dropFront(strlen("androideabi"));
      if (androidVersion.empty()) {
         components[3] = "android";
      } else {
         normalizedEnvironment = Twine("android", androidVersion).getStr();
         components[3] = normalizedEnvironment;
      }
   }

   // SUSE uses "gnueabi" to mean "gnueabihf"
   if (vendor == VendorType::SUSE && environment == EnvironmentType::GNUEABI) {
      components[3] = "gnueabihf";
   }
   if (os == OSType::Win32) {
      components.resize(4);
      components[2] = "windows";
      if (environment == EnvironmentType::UnknownEnvironment) {
         if (objectFormat == ObjectFormatType::UnknownObjectFormat || objectFormat == ObjectFormatType::COFF) {
            components[3] = "msvc";
         } else {
            components[3] = getObjectFormatTypeName(objectFormat);
         }
      }
   } else if (isMinGW32) {
      components.resize(4);
      components[2] = "windows";
      components[3] = "gnu";
   } else if (isCygwin) {
      components.resize(4);
      components[2] = "windows";
      components[3] = "cygnus";
   }
   if (isMinGW32 || isCygwin ||
       (os == OSType::Win32 && environment != EnvironmentType::UnknownEnvironment)) {
      if (objectFormat != ObjectFormatType::UnknownObjectFormat && objectFormat != ObjectFormatType::COFF) {
         components.resize(5);
         components[4] = getObjectFormatTypeName(objectFormat);
      }
   }

   // Stick the corrected components back together to form the normalized string.
   std::string normalized;
   for (unsigned i = 0, e = components.getSize(); i != e; ++i) {
      if (i) normalized += '-';
      normalized += components[i];
   }
   return normalized;
}

StringRef Triple::getArchName() const
{
   return StringRef(m_data).split('-').first;           // Isolate first component
}

StringRef Triple::getVendorName() const
{
   StringRef temp = StringRef(m_data).split('-').second; // Strip first component
   return temp.split('-').first;                       // Isolate second component
}

StringRef Triple::getOSName() const
{
   StringRef temp = StringRef(m_data).split('-').second; // Strip first component
   temp = temp.split('-').second;                       // Strip second component
   return temp.split('-').first;                       // Isolate third component
}

StringRef Triple::getEnvironmentName() const
{
   StringRef temp = StringRef(m_data).split('-').second; // Strip first component
   temp = temp.split('-').second;                       // Strip second component
   return temp.split('-').second;                      // Strip third component
}

StringRef Triple::getOSAndEnvironmentName() const
{
   StringRef temp = StringRef(m_data).split('-').second; // Strip first component
   return temp.split('-').second;                      // Strip second component
}

namespace {

unsigned eat_number(StringRef &str)
{
   assert(!str.empty() && str[0] >= '0' && str[0] <= '9' && "Not a number");
   unsigned result = 0;
   do {
      // Consume the leading digit.
      result = result * 10 + (str[0] - '0');
      // Eat the digit.
      str = str.substr(1);
   } while (!str.empty() && str[0] >= '0' && str[0] <= '9');
   return result;
}

void parse_version_from_name(StringRef name, unsigned &major,
                             unsigned &minor, unsigned &micro)
{
   // Any unset version defaults to 0.
   major = minor = micro = 0;

   // Parse up to three components.
   unsigned *components[3] = {&major, &minor, &micro};
   for (unsigned i = 0; i != 3; ++i) {
      if (name.empty() || name[0] < '0' || name[0] > '9') {
         break;
      }
      // Consume the leading number.
      *components[i] = eat_number(name);
      // Consume the separator, if present.
      if (name.startsWith(".")) {
         name = name.substr(1);
      }
   }
}


} // anonymous namespacd

void Triple::getEnvironmentVersion(unsigned &major, unsigned &minor,
                                   unsigned &micro) const {
   StringRef environmentName = getEnvironmentName();
   StringRef environmentTypeName = getEnvironmentTypeName(getEnvironment());
   if (environmentName.startsWith(environmentTypeName)) {
      environmentName = environmentName.substr(environmentTypeName.getSize());
   }
   parse_version_from_name(environmentName, major, minor, micro);
}

void Triple::getOSVersion(unsigned &major, unsigned &minor,
                          unsigned &micro) const
{
   StringRef osName = getOSName();
   // Assume that the OS portion of the triple starts with the canonical name.
   StringRef osTypeName = getOSTypeName(getOS());
   if (osName.startsWith(osTypeName)) {
      osName = osName.substr(osTypeName.getSize());
   } else if (getOS() == OSType::MacOSX) {
      osName.consumeFront("macos");
   }
   parse_version_from_name(osName, major, minor, micro);
}

bool Triple::getMacOSXVersion(unsigned &major, unsigned &minor,
                              unsigned &micro) const
{
   getOSVersion(major, minor, micro);
   switch (getOS()) {
   default: polar_unreachable("unexpected OS for Darwin triple");
   case OSType::Darwin:
      // Default to darwin8, i.e., MacOSX 10.4.
      if (major == 0) {
         major = 8;
      }
      // Darwin version numbers are skewed from OS X versions.
      if (major < 4) {
         return false;
      }
      micro = 0;
      minor = major - 4;
      major = 10;
      break;
   case OSType::MacOSX:
      // Default to 10.4.
      if (major == 0) {
         major = 10;
         minor = 4;
      }
      if (major != 10) {
         return false;
      }
      break;
   case OSType::IOS:
   case OSType::TvOS:
   case OSType::WatchOS:
      // Ignore the version from the triple.  This is only handled because the
      // the clang driver combines OS X and IOS support into a common Darwin
      // toolchain that wants to know the OS X version number even when targeting
      // IOS.
      major = 10;
      minor = 4;
      micro = 0;
      break;
   }
   return true;
}

void Triple::getiOSVersion(unsigned &major, unsigned &minor,
                           unsigned &micro) const
{
   switch (getOS()) {
   default: polar_unreachable("unexpected OS for Darwin triple");
   case OSType::Darwin:
   case OSType::MacOSX:
      // Ignore the version from the triple.  This is only handled because the
      // the clang driver combines OS X and IOS support into a common Darwin
      // toolchain that wants to know the iOS version number even when targeting
      // OS X.
      major = 5;
      minor = 0;
      micro = 0;
      break;
   case OSType::IOS:
   case OSType::TvOS:
      getOSVersion(major, minor, micro);
      // Default to 5.0 (or 7.0 for arm64).
      if (major == 0) {
         major = (getArch() == ArchType::aarch64) ? 7 : 5;
      }
      break;
   case OSType::WatchOS:
      polar_unreachable("conflicting triple info");
   }
}

void Triple::getWatchOSVersion(unsigned &major, unsigned &minor,
                               unsigned &micro) const
{
   switch (getOS()) {
   default: polar_unreachable("unexpected OS for Darwin triple");
   case OSType::Darwin:
   case OSType::MacOSX:
      // Ignore the version from the triple.  This is only handled because the
      // the clang driver combines OS X and IOS support into a common Darwin
      // toolchain that wants to know the iOS version number even when targeting
      // OS X.
      major = 2;
      minor = 0;
      micro = 0;
      break;
   case OSType::WatchOS:
      getOSVersion(major, minor, micro);
      if (major == 0)
         major = 2;
      break;
   case OSType::IOS:
      polar_unreachable("conflicting triple info");
   }
}

void Triple::setTriple(const Twine &str)
{
   *this = Triple(str);
}

void Triple::setArch(ArchType kind)
{
   setArchName(getArchTypeName(kind));
}

void Triple::setVendor(VendorType kind)
{
   setVendorName(getVendorTypeName(kind));
}

void Triple::setOS(OSType kind)
{
   setOSName(getOSTypeName(kind));
}

void Triple::setEnvironment(EnvironmentType kind)
{
   if (m_objectFormat == get_default_format(*this)) {
      return setEnvironmentName(getEnvironmentTypeName(kind));
   }
   setEnvironmentName((getEnvironmentTypeName(kind) + Twine("-") +
                       getObjectFormatTypeName(m_objectFormat)).getStr());
}

void Triple::setObjectFormat(ObjectFormatType kind) {
   if (m_environment == EnvironmentType::UnknownEnvironment) {
      return setEnvironmentName(getObjectFormatTypeName(kind));
   }
   setEnvironmentName((getEnvironmentTypeName(m_environment) + Twine("-") +
                       getObjectFormatTypeName(kind)).getStr());
}

void Triple::setArchName(StringRef str)
{
   // Work around a miscompilation bug for Twines in gcc 4.0.3.
   SmallString<64> triple;
   triple += str;
   triple += "-";
   triple += getVendorName();
   triple += "-";
   triple += getOSAndEnvironmentName();
   setTriple(triple);
}

void Triple::setVendorName(StringRef str)
{
   setTriple(getArchName() + "-" + str + "-" + getOSAndEnvironmentName());
}

void Triple::setOSName(StringRef str)
{
   if (hasEnvironment()) {
      setTriple(getArchName() + "-" + getVendorName() + "-" + str +
                "-" + getEnvironmentName());
   } else {
      setTriple(getArchName() + "-" + getVendorName() + "-" + str);
   }
}

void Triple::setEnvironmentName(StringRef str)
{
   setTriple(getArchName() + "-" + getVendorName() + "-" + getOSName() +
             "-" + str);
}

void Triple::setOSAndEnvironmentName(StringRef str)
{
   setTriple(getArchName() + "-" + getVendorName() + "-" + str);
}

namespace {
unsigned get_arch_pointer_bit_width(Triple::ArchType arch)
{
   switch (arch) {
   case Triple::ArchType::UnknownArch:
      return 0;

   case Triple::ArchType::avr:
   case Triple::ArchType::msp430:
      return 16;

   case Triple::ArchType::arc:
   case Triple::ArchType::arm:
   case Triple::ArchType::armeb:
   case Triple::ArchType::hexagon:
   case Triple::ArchType::le32:
   case Triple::ArchType::mips:
   case Triple::ArchType::mipsel:
   case Triple::ArchType::nios2:
   case Triple::ArchType::nvptx:
   case Triple::ArchType::ppc:
   case Triple::ArchType::r600:
   case Triple::ArchType::riscv32:
   case Triple::ArchType::sparc:
   case Triple::ArchType::sparcel:
   case Triple::ArchType::tce:
   case Triple::ArchType::tcele:
   case Triple::ArchType::thumb:
   case Triple::ArchType::thumbeb:
   case Triple::ArchType::x86:
   case Triple::ArchType::xcore:
   case Triple::ArchType::amdil:
   case Triple::ArchType::hsail:
   case Triple::ArchType::spir:
   case Triple::ArchType::kalimba:
   case Triple::ArchType::lanai:
   case Triple::ArchType::shave:
   case Triple::ArchType::wasm32:
   case Triple::ArchType::renderscript32:
      return 32;

   case Triple::ArchType::aarch64:
   case Triple::ArchType::aarch64_be:
   case Triple::ArchType::amdgcn:
   case Triple::ArchType::bpfel:
   case Triple::ArchType::bpfeb:
   case Triple::ArchType::le64:
   case Triple::ArchType::mips64:
   case Triple::ArchType::mips64el:
   case Triple::ArchType::nvptx64:
   case Triple::ArchType::ppc64:
   case Triple::ArchType::ppc64le:
   case Triple::ArchType::riscv64:
   case Triple::ArchType::sparcv9:
   case Triple::ArchType::systemz:
   case Triple::ArchType::x86_64:
   case Triple::ArchType::amdil64:
   case Triple::ArchType::hsail64:
   case Triple::ArchType::spir64:
   case Triple::ArchType::wasm64:
   case Triple::ArchType::renderscript64:
      return 64;
   }
   polar_unreachable("Invalid architecture value");
}
} // anonymous namespace

bool Triple::isArch64Bit() const
{
   return get_arch_pointer_bit_width(getArch()) == 64;
}

bool Triple::isArch32Bit() const
{
   return get_arch_pointer_bit_width(getArch()) == 32;
}

bool Triple::isArch16Bit() const
{
   return get_arch_pointer_bit_width(getArch()) == 16;
}

Triple Triple::get32BitArchVariant() const
{
   Triple temp(*this);
   switch (getArch()) {
   case ArchType::UnknownArch:
   case ArchType::amdgcn:
   case ArchType::avr:
   case ArchType::bpfel:
   case ArchType::bpfeb:
   case ArchType::msp430:
   case ArchType::systemz:
   case ArchType::ppc64le:
      temp.setArch(ArchType::UnknownArch);
      break;

   case ArchType::amdil:
   case ArchType::hsail:
   case ArchType::spir:
   case ArchType::arc:
   case ArchType::arm:
   case ArchType::armeb:
   case ArchType::hexagon:
   case ArchType::kalimba:
   case ArchType::le32:
   case ArchType::mips:
   case ArchType::mipsel:
   case ArchType::nios2:
   case ArchType::nvptx:
   case ArchType::ppc:
   case ArchType::r600:
   case ArchType::riscv32:
   case ArchType::sparc:
   case ArchType::sparcel:
   case ArchType::tce:
   case ArchType::tcele:
   case ArchType::thumb:
   case ArchType::thumbeb:
   case ArchType::x86:
   case ArchType::xcore:
   case ArchType::lanai:
   case ArchType::shave:
   case ArchType::wasm32:
   case ArchType::renderscript32:
      // Already 32-bit.
      break;

   case ArchType::aarch64:        temp.setArch(ArchType::arm);     break;
   case ArchType::aarch64_be:     temp.setArch(ArchType::armeb);   break;
   case ArchType::le64:           temp.setArch(ArchType::le32);    break;
   case ArchType::mips64:         temp.setArch(ArchType::mips);    break;
   case ArchType::mips64el:       temp.setArch(ArchType::mipsel);  break;
   case ArchType::nvptx64:        temp.setArch(ArchType::nvptx);   break;
   case ArchType::ppc64:          temp.setArch(ArchType::ppc);     break;
   case ArchType::sparcv9:        temp.setArch(ArchType::sparc);   break;
   case ArchType::riscv64:        temp.setArch(ArchType::riscv32); break;
   case ArchType::x86_64:         temp.setArch(ArchType::x86);     break;
   case ArchType::amdil64:        temp.setArch(ArchType::amdil);   break;
   case ArchType::hsail64:        temp.setArch(ArchType::hsail);   break;
   case ArchType::spir64:         temp.setArch(ArchType::spir);    break;
   case ArchType::wasm64:         temp.setArch(ArchType::wasm32);  break;
   case ArchType::renderscript64: temp.setArch(ArchType::renderscript32); break;
   }
   return temp;
}

Triple Triple::get64BitArchVariant() const
{
   Triple temp(*this);
   switch (getArch()) {
   case ArchType::UnknownArch:
   case ArchType::arc:
   case ArchType::avr:
   case ArchType::hexagon:
   case ArchType::kalimba:
   case ArchType::lanai:
   case ArchType::msp430:
   case ArchType::nios2:
   case ArchType::r600:
   case ArchType::tce:
   case ArchType::tcele:
   case ArchType::xcore:
   case ArchType::sparcel:
   case ArchType::shave:
      temp.setArch(ArchType::UnknownArch);
      break;

   case ArchType::aarch64:
   case ArchType::aarch64_be:
   case ArchType::bpfel:
   case ArchType::bpfeb:
   case ArchType::le64:
   case ArchType::amdil64:
   case ArchType::amdgcn:
   case ArchType::hsail64:
   case ArchType::spir64:
   case ArchType::mips64:
   case ArchType::mips64el:
   case ArchType::nvptx64:
   case ArchType::ppc64:
   case ArchType::ppc64le:
   case ArchType::riscv64:
   case ArchType::sparcv9:
   case ArchType::systemz:
   case ArchType::x86_64:
   case ArchType::wasm64:
   case ArchType::renderscript64:
      // Already 64-bit.
      break;

   case ArchType::arm:             temp.setArch(ArchType::aarch64);    break;
   case ArchType::armeb:           temp.setArch(ArchType::aarch64_be); break;
   case ArchType::le32:            temp.setArch(ArchType::le64);       break;
   case ArchType::mips:            temp.setArch(ArchType::mips64);     break;
   case ArchType::mipsel:          temp.setArch(ArchType::mips64el);   break;
   case ArchType::nvptx:           temp.setArch(ArchType::nvptx64);    break;
   case ArchType::ppc:             temp.setArch(ArchType::ppc64);      break;
   case ArchType::sparc:           temp.setArch(ArchType::sparcv9);    break;
   case ArchType::riscv32:         temp.setArch(ArchType::riscv64);    break;
   case ArchType::x86:             temp.setArch(ArchType::x86_64);     break;
   case ArchType::amdil:           temp.setArch(ArchType::amdil64);    break;
   case ArchType::hsail:           temp.setArch(ArchType::hsail64);    break;
   case ArchType::spir:            temp.setArch(ArchType::spir64);     break;
   case ArchType::thumb:           temp.setArch(ArchType::aarch64);    break;
   case ArchType::thumbeb:         temp.setArch(ArchType::aarch64_be); break;
   case ArchType::wasm32:          temp.setArch(ArchType::wasm64);     break;
   case ArchType::renderscript32:  temp.setArch(ArchType::renderscript64);     break;
   }
   return temp;
}

Triple Triple::getBigEndianArchVariant() const
{
   Triple temp(*this);
   // Already big endian.
   if (!isLittleEndian()) {
      return temp;
   }

   switch (getArch()) {
   case ArchType::UnknownArch:
   case ArchType::amdgcn:
   case ArchType::amdil64:
   case ArchType::amdil:
   case ArchType::avr:
   case ArchType::hexagon:
   case ArchType::hsail64:
   case ArchType::hsail:
   case ArchType::kalimba:
   case ArchType::le32:
   case ArchType::le64:
   case ArchType::msp430:
   case ArchType::nios2:
   case ArchType::nvptx64:
   case ArchType::nvptx:
   case ArchType::r600:
   case ArchType::riscv32:
   case ArchType::riscv64:
   case ArchType::shave:
   case ArchType::spir64:
   case ArchType::spir:
   case ArchType::wasm32:
   case ArchType::wasm64:
   case ArchType::x86:
   case ArchType::x86_64:
   case ArchType::xcore:
   case ArchType::renderscript32:
   case ArchType::renderscript64:

      // ARM is intentionally unsupported here, changing the architecture would
      // drop any arch suffixes.
   case ArchType::arm:
   case ArchType::thumb:
      temp.setArch(ArchType::UnknownArch);
      break;

   case ArchType::tcele:   temp.setArch(ArchType::tce);        break;
   case ArchType::aarch64: temp.setArch(ArchType::aarch64_be); break;
   case ArchType::bpfel:   temp.setArch(ArchType::bpfeb);      break;
   case ArchType::mips64el:temp.setArch(ArchType::mips64);     break;
   case ArchType::mipsel:  temp.setArch(ArchType::mips);       break;
   case ArchType::ppc64le: temp.setArch(ArchType::ppc64);      break;
   case ArchType::sparcel: temp.setArch(ArchType::sparc);      break;
   default:
      polar_unreachable("getBigEndianArchVariant: unknown ArchType.");
   }
   return temp;
}

Triple Triple::getLittleEndianArchVariant() const
{
   Triple temp(*this);
   if (isLittleEndian()) {
      return temp;
   }
   switch (getArch()) {
   case ArchType::UnknownArch:
   case ArchType::lanai:
   case ArchType::ppc:
   case ArchType::sparcv9:
   case ArchType::systemz:

      // ARM is intentionally unsupported here, changing the architecture would
      // drop any arch suffixes.
   case ArchType::armeb:
   case ArchType::thumbeb:
      temp.setArch(ArchType::UnknownArch);
      break;

   case ArchType::tce:        temp.setArch(ArchType::tcele);    break;
   case ArchType::aarch64_be: temp.setArch(ArchType::aarch64);  break;
   case ArchType::bpfeb:      temp.setArch(ArchType::bpfel);    break;
   case ArchType::mips64:     temp.setArch(ArchType::mips64el); break;
   case ArchType::mips:       temp.setArch(ArchType::mipsel);   break;
   case ArchType::ppc64:      temp.setArch(ArchType::ppc64le);  break;
   case ArchType::sparc:      temp.setArch(ArchType::sparcel);  break;
   default:
      polar_unreachable("getLittleEndianArchVariant: unknown ArchType.");
   }
   return temp;
}

bool Triple::isLittleEndian() const
{
   switch (getArch()) {
   case ArchType::aarch64:
   case ArchType::amdgcn:
   case ArchType::amdil64:
   case ArchType::amdil:
   case ArchType::arm:
   case ArchType::avr:
   case ArchType::bpfel:
   case ArchType::hexagon:
   case ArchType::hsail64:
   case ArchType::hsail:
   case ArchType::kalimba:
   case ArchType::le32:
   case ArchType::le64:
   case ArchType::mips64el:
   case ArchType::mipsel:
   case ArchType::msp430:
   case ArchType::nios2:
   case ArchType::nvptx64:
   case ArchType::nvptx:
   case ArchType::ppc64le:
   case ArchType::r600:
   case ArchType::riscv32:
   case ArchType::riscv64:
   case ArchType::shave:
   case ArchType::sparcel:
   case ArchType::spir64:
   case ArchType::spir:
   case ArchType::thumb:
   case ArchType::wasm32:
   case ArchType::wasm64:
   case ArchType::x86:
   case ArchType::x86_64:
   case ArchType::xcore:
   case ArchType::tcele:
   case ArchType::renderscript32:
   case ArchType::renderscript64:
      return true;
   default:
      return false;
   }
}

bool Triple::isCompatibleWith(const Triple &other) const
{
   // ARM and Thumb ArchTypes are compatible, if subarch, vendor and OS match.
   if ((getArch() == ArchType::thumb && other.getArch() == ArchType::arm) ||
       (getArch() == ArchType::arm && other.getArch() == ArchType::thumb) ||
       (getArch() == ArchType::thumbeb && other.getArch() == ArchType::armeb) ||
       (getArch() == ArchType::armeb && other.getArch() == ArchType::thumbeb)) {
      if (getVendor() == VendorType::Apple) {
         return getSubArch() == other.getSubArch() &&
               getVendor() == other.getVendor() && getOS() == other.getOS();
      } else {
         return getSubArch() == other.getSubArch() &&
               getVendor() == other.getVendor() && getOS() == other.getOS() &&
               getEnvironment() == other.getEnvironment() &&
               getObjectFormat() == other.getObjectFormat();
      }
   }

   // If vendor is apple, ignore the version number.
   if (getVendor() == VendorType::Apple) {
      return getArch() == other.getArch() && getSubArch() == other.getSubArch() &&
            getVendor() == other.getVendor() && getOS() == other.getOS();
   }
   return *this == other;
}

std::string Triple::merge(const Triple &other) const
{
   // If vendor is apple, pick the triple with the larger version number.
   if (getVendor() == VendorType::Apple) {
      if (other.isOSVersionLT(*this)) {
         return getStr();
      }
   }
   return other.getStr();
}

StringRef Triple::getARMCPUForArch(StringRef mArch) const
{
   if (mArch.empty()) {
      mArch = getArchName();
   }
   mArch = arm::get_canonical_arch_name(mArch);
   // Some defaults are forced.
   switch (getOS()) {
   case OSType::FreeBSD:
   case OSType::NetBSD:
      if (!mArch.empty() && mArch == "v6")
         return "arm1176jzf-s";
      break;
   case OSType::Win32:
      // FIXME: this is invalid for WindowsCE
      return "cortex-a9";
   case OSType::MacOSX:
   case OSType::IOS:
   case OSType::WatchOS:
   case OSType::TvOS:
      if (mArch == "v7k")
         return "cortex-a7";
      break;
   default:
      break;
   }
   if (mArch.empty()) {
      return StringRef();
   }
   StringRef cpu = arm::get_default_cpu(mArch);
   if (!cpu.empty() && !cpu.equals("invalid"))
      return cpu;

   // If no specific architecture version is requested, return the minimum CPU
   // required by the OS and environment.
   switch (getOS()) {
   case OSType::NetBSD:
      switch (getEnvironment()) {
      case EnvironmentType::GNUEABIHF:
      case EnvironmentType::GNUEABI:
      case EnvironmentType::EABIHF:
      case EnvironmentType::EABI:
         return "arm926ej-s";
      default:
         return "strongarm";
      }
   case OSType::NaCl:
   case OSType::OpenBSD:
      return "cortex-a8";
   default:
      switch (getEnvironment()) {
      case EnvironmentType::EABIHF:
      case EnvironmentType::GNUEABIHF:
      case EnvironmentType::MuslEABIHF:
         return "arm1176jzf-s";
      default:
         return "arm7tdmi";
      }
   }

   polar_unreachable("invalid arch name");
}

} // basic
} // polar
